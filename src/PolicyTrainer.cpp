#include "PolicyTrainer.h"

#include "BuildingDefinition.h"
#include "Game.h"
#include "PolicyModel.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr std::size_t PolicyActionCount = policy::ActionCountSize;
constexpr std::size_t PolicyFeatureCount = 13;

struct PolicyChoice
{
    int action = 0;
    GameOperation operation;
};

struct PolicyEvent
{
    std::array<float, PolicyFeatureCount> features{};
    int action = 0;
    bool success = false;
};

std::array<float, PolicyFeatureCount> policyFeatures(const Game& game, int team)
{
    const int enemy = team == PLAYER ? AI : PLAYER;
    const auto& ownUnits = team == PLAYER ? game.myunits : game.enemys;
    const auto& enemyUnits = team == PLAYER ? game.enemys : game.myunits;
    const DisMoveableUnit* ownBase = team == PLAYER ? game.Base_red.get() : game.Base_blue.get();
    const DisMoveableUnit* enemyBase = team == PLAYER ? game.Base_blue.get() : game.Base_red.get();
    const int ownBarracksCap = std::max(1, game.buildingCap(team, building::Barracks));
    const int ownTowerCap = std::max(1, game.buildingCap(team, building::DefenseTower));
    return {
        1.f,
        std::clamp(game.gameTimeSeconds / 900.f, 0.f, 1.5f),
        static_cast<float>(game.commandForTeam(team)) / static_cast<float>(config::MaxCommand),
        static_cast<float>(game.economyLevelForTeam(team)) / static_cast<float>(config::MaxEconomyLevel),
        static_cast<float>(team == PLAYER ? game.playerUpgradeLevel : game.aiUpgradeLevel) / static_cast<float>(config::MaxTechLevel),
        static_cast<float>(game.completedBuildingCount(team, building::Barracks)) / static_cast<float>(ownBarracksCap),
        static_cast<float>(game.totalBuildingCount(team, building::DefenseTower)) / static_cast<float>(ownTowerCap),
        static_cast<float>(ownUnits.size()) / static_cast<float>(config::MaxUnits),
        static_cast<float>(enemyUnits.size()) / static_cast<float>(config::MaxUnits),
        static_cast<float>(ownBase ? ownBase->Health : 0) / 4000.f,
        static_cast<float>(enemyBase ? enemyBase->Health : 0) / 4000.f,
        static_cast<float>(game.unitsNearPoint(enemy, team == PLAYER ? game.Red_baseP : game.Blue_baseP, 13)) / 20.f,
        static_cast<float>(game.unitsNearPoint(team, team == PLAYER ? game.Blue_baseP : game.Red_baseP, 13)) / 20.f
    };
}

std::vector<PolicyChoice> legalPolicyChoices(const Game& game, int team, std::mt19937& rng)
{
    std::vector<PolicyChoice> choices;
    std::uniform_int_distribution<int> laneDist(0, lane::Count - 1);
    const auto push = [&](policy::Action action, const GameOperation& operation) {
        choices.push_back(PolicyChoice{static_cast<int>(action), operation});
    };

    push(policy::Action::Wait, GameOperation(gameop::SelectLane, laneDist(rng)));
    if (game.economyUpgradeCost(team) > 0 && game.commandForTeam(team) >= game.economyUpgradeCost(team)) {
        push(policy::Action::Economy, GameOperation(gameop::UpgradeEconomy));
    }
    if (game.upgradeCostForNextLevel(team) > 0 && game.commandForTeam(team) >= game.upgradeCostForNextLevel(team)) {
        push(policy::Action::Tech, GameOperation(gameop::UpgradeTech));
    }
    if (game.totalBuildingCount(team, building::Barracks) < game.buildingCap(team, building::Barracks)
        && game.commandForTeam(team) >= buildingDefinition(building::Barracks).commandCost) {
        push(policy::Action::Barracks, GameOperation(gameop::BuildBarracks, laneDist(rng)));
    }
    if (game.totalBuildingCount(team, building::DefenseTower) < game.buildingCap(team, building::DefenseTower)
        && game.commandForTeam(team) >= buildingDefinition(building::DefenseTower).commandCost) {
        push(policy::Action::Tower, GameOperation(gameop::BuildTower, laneDist(rng)));
    }
    for (policy::Action action : {policy::Action::Infantry, policy::Action::Shooter, policy::Action::Cavalry, policy::Action::Siege, policy::Action::Guardian}) {
        const int unit = policy::unitForAction(action);
        if (game.canQueueUnit(team, unit)) {
            push(action, GameOperation(gameop::QueueUnit, laneDist(rng), unit));
        }
    }
    return choices;
}

class TrainablePolicy
{
public:
    explicit TrainablePolicy(unsigned int seed)
    {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-0.04f, 0.04f);
        for (auto& row : weights) {
            for (float& value : row) {
                value = dist(rng);
            }
        }
    }

    PolicyChoice choose(const Game& game, int team, std::mt19937& rng, float exploration)
    {
        const auto choices = legalPolicyChoices(game, team, rng);
        const auto features = policyFeatures(game, team);
        std::uniform_real_distribution<float> unitDist(0.f, 1.f);
        std::size_t selected = 0;
        if (unitDist(rng) < exploration) {
            std::uniform_int_distribution<std::size_t> pick(0, choices.size() - 1);
            selected = pick(rng);
        }
        else {
            float bestScore = -1e9f;
            for (std::size_t i = 0; i < choices.size(); ++i) {
                const int action = choices[i].action;
                float score = 0.f;
                for (std::size_t f = 0; f < PolicyFeatureCount; ++f) {
                    score += weights[static_cast<std::size_t>(action)][f] * features[f];
                }
                if (score > bestScore) {
                    bestScore = score;
                    selected = i;
                }
            }
        }
        pendingFeatures = features;
        return choices[selected];
    }

    void record(int action, bool success)
    {
        history.push_back(PolicyEvent{pendingFeatures, action, success});
        ++actionCounts[static_cast<std::size_t>(action)];
    }

    void learn(float reward)
    {
        constexpr float learningRate = 0.035f;
        for (const auto& event : history) {
            const float signal = reward + (event.success ? 0.03f : -0.12f);
            auto& row = weights[static_cast<std::size_t>(event.action)];
            for (std::size_t f = 0; f < PolicyFeatureCount; ++f) {
                row[f] = std::clamp(row[f] + learningRate * signal * event.features[f], -2.5f, 2.5f);
            }
        }
        history.clear();
    }

    std::string countSummary() const
    {
        std::string out;
        for (std::size_t i = 0; i < PolicyActionCount; ++i) {
            if (!out.empty()) {
                out += " ";
            }
            out += policy::actionName(static_cast<policy::Action>(i));
            out += "=";
            out += std::to_string(actionCounts[i]);
        }
        return out;
    }

private:
    std::array<std::array<float, PolicyFeatureCount>, PolicyActionCount> weights{};
    std::array<float, PolicyFeatureCount> pendingFeatures{};
    std::array<int, PolicyActionCount> actionCounts{};
    std::vector<PolicyEvent> history;
};

float sideScore(const Game& game, int team)
{
    const int enemy = team == PLAYER ? AI : PLAYER;
    const DisMoveableUnit* ownBase = team == PLAYER ? game.Base_red.get() : game.Base_blue.get();
    const DisMoveableUnit* enemyBase = team == PLAYER ? game.Base_blue.get() : game.Base_red.get();
    const auto& ownUnits = team == PLAYER ? game.myunits : game.enemys;
    const auto& enemyUnits = team == PLAYER ? game.enemys : game.myunits;
    float score = static_cast<float>((ownBase ? ownBase->Health : 0) - (enemyBase ? enemyBase->Health : 0)) / 4000.f;
    score += static_cast<float>(ownUnits.size()) * 0.012f - static_cast<float>(enemyUnits.size()) * 0.012f;
    score += static_cast<float>(game.economyLevelForTeam(team) - game.economyLevelForTeam(enemy)) * 0.05f;
    score += static_cast<float>((team == PLAYER ? game.playerUpgradeLevel : game.aiUpgradeLevel)
        - (enemy == PLAYER ? game.playerUpgradeLevel : game.aiUpgradeLevel)) * 0.045f;
    if (enemyBase && enemyBase->Health <= 0) {
        score += 1.25f;
    }
    if (ownBase && ownBase->Health <= 0) {
        score -= 1.25f;
    }
    return std::clamp(score, -2.f, 2.f);
}

void runPolicyTraining(int episodes, float seconds, float dt, unsigned int seed)
{
    TrainablePolicy playerPolicy(seed + 17);
    TrainablePolicy aiPolicy(seed + 31);
    std::mt19937 rng(seed);
    Game game;
    game.window.setVisible(false);
    game.autoChooseRewards = true;
    game.debugLogging = false;
    game.gameSceneState = SCENE_GAME;
    game.externalAIControl = true;

    int playerWins = 0;
    int aiWins = 0;
    float rewardSum = 0.f;
    const int reportEvery = std::max(1, episodes / 8);
    for (int episode = 1; episode <= episodes; ++episode) {
        game.clear();
        game.externalAIControl = true;
        game.autoChooseRewards = true;
        const float exploration = std::max(0.08f, 0.65f * (1.f - static_cast<float>(episode - 1) / std::max(1, episodes)));
        float playerTimer = 0.f;
        float aiTimer = 0.f;
        const int ticks = static_cast<int>(seconds / dt);
        for (int tick = 0; tick < ticks && !game.gameOver; ++tick) {
            playerTimer += dt;
            aiTimer += dt;
            if (playerTimer >= realtime::AIThinkSeconds) {
                playerTimer = 0.f;
                PolicyChoice choice = playerPolicy.choose(game, PLAYER, rng, exploration);
                playerPolicy.record(choice.action, game.executeOperation(PLAYER, choice.operation));
            }
            if (aiTimer >= realtime::AIThinkSeconds) {
                aiTimer = 0.f;
                PolicyChoice choice = aiPolicy.choose(game, AI, rng, exploration);
                aiPolicy.record(choice.action, game.executeOperation(AI, choice.operation));
            }
            game.updateRealtime(dt);
        }

        const float reward = sideScore(game, PLAYER);
        rewardSum += reward;
        if (reward > 0.f) {
            ++playerWins;
        }
        else if (reward < 0.f) {
            ++aiWins;
        }
        playerPolicy.learn(reward);
        aiPolicy.learn(-reward);

        if (episode == 1 || episode == episodes || episode % reportEvery == 0) {
            std::clog << "[train " << episode << "/" << episodes << "]"
                << " reward=" << reward
                << " pWin=" << playerWins
                << " aiWin=" << aiWins
                << " pEco=" << game.playerEconomyLevel
                << " pTech=" << game.playerUpgradeLevel
                << " pArmy=" << game.myunits.size()
                << " aiEco=" << game.aiEconomyLevel
                << " aiTech=" << game.aiUpgradeLevel
                << " aiArmy=" << game.enemys.size()
                << " pBase=" << (game.Base_red ? game.Base_red->Health : 0)
                << " aiBase=" << (game.Base_blue ? game.Base_blue->Health : 0)
                << '\n';
        }
    }

    std::clog << "[train done] episodes=" << episodes
        << " seconds=" << seconds
        << " avgReward=" << (rewardSum / std::max(1, episodes))
        << " playerWins=" << playerWins
        << " aiWins=" << aiWins
        << '\n'
        << "[train player actions] " << playerPolicy.countSummary() << '\n'
        << "[train ai actions] " << aiPolicy.countSummary() << '\n';
}

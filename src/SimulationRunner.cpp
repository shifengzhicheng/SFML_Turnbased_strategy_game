#include "SimulationRunner.h"

#include "Game.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    bool looksLikeNumber(const std::string& value)
    {
        if (value.empty()) {
            return false;
        }
        char* end = nullptr;
        std::strtof(value.c_str(), &end);
        return end != value.c_str() && *end == '\0';
    }

    ScriptedPlan parseScriptedPlan(const std::string& value)
    {
        if (value == "rush") {
            return ScriptedPlan::Rush;
        }
        if (value == "greedy" || value == "eco") {
            return ScriptedPlan::Greedy;
        }
        return ScriptedPlan::Balanced;
    }

    const char* scriptedPlanName(ScriptedPlan plan)
    {
        switch (plan) {
        case ScriptedPlan::Rush:
            return "rush";
        case ScriptedPlan::Greedy:
            return "greedy";
        case ScriptedPlan::Balanced:
        default:
            return "balanced";
        }
    }

    int estimatedOperationCost(const Game& game, int team, const GameOperation& operation)
    {
        switch (operation.type) {
        case gameop::UpgradeEconomy:
            return game.economyUpgradeCost(team);
        case gameop::UpgradeTech:
            return game.upgradeCostForNextLevel(team);
        case gameop::BuildBarracks:
            return config::BarracksCost;
        case gameop::BuildTower:
            return config::TowerCost;
        case gameop::QueueUnit:
            return game.unitCost(operation.unitName);
        case gameop::SelectLane:
        default:
            return 0;
        }
    }

    int desiredEconomyForPlan(ScriptedPlan plan, float timeSeconds)
    {
        int desired = 1;
        if (plan == ScriptedPlan::Rush) {
            if (timeSeconds > 120.f) desired = 2;
            if (timeSeconds > 300.f) desired = 3;
            if (timeSeconds > 520.f) desired = 5;
        }
        else if (plan == ScriptedPlan::Greedy) {
            if (timeSeconds > 25.f) desired = 2;
            if (timeSeconds > 70.f) desired = 3;
            if (timeSeconds > 130.f) desired = 4;
            if (timeSeconds > 230.f) desired = 5;
            if (timeSeconds > 360.f) desired = 7;
            if (timeSeconds > 560.f) desired = 9;
        }
        else {
            if (timeSeconds > 45.f) desired = 2;
            if (timeSeconds > 115.f) desired = 3;
            if (timeSeconds > 220.f) desired = 4;
            if (timeSeconds > 360.f) desired = 5;
            if (timeSeconds > 540.f) desired = 7;
        }
        return std::min(desired, config::MaxEconomyLevel);
    }

    int desiredTechForPlan(const Game& game, ScriptedPlan plan)
    {
        const float delay = plan == ScriptedPlan::Rush ? 88.f : (plan == ScriptedPlan::Greedy ? 56.f : 68.f);
        int allowed = static_cast<int>(game.gameTimeSeconds / delay);
        const int economyBonus = plan == ScriptedPlan::Greedy ? 5 : 3;
        if (plan == ScriptedPlan::Rush && game.gameTimeSeconds < 160.f) {
            allowed = std::min(allowed, 1);
        }
        allowed = std::min(allowed, game.playerEconomyLevel + economyBonus);
        if (game.gameTimeSeconds > 720.f) {
            allowed = std::max(allowed, 10 + static_cast<int>((game.gameTimeSeconds - 720.f) / 55.f));
        }
        return std::clamp(allowed, 0, config::MaxTechLevel);
    }

    int desiredBarracksForPlan(const Game& game, ScriptedPlan plan)
    {
        int desired = 1;
        if (plan == ScriptedPlan::Rush) {
            if (game.gameTimeSeconds > 85.f && game.playerEconomyLevel >= 1) desired = 2;
            if (game.gameTimeSeconds > 220.f && game.playerUpgradeLevel >= 3) desired = 3;
            if (game.gameTimeSeconds > 430.f && game.playerUpgradeLevel >= 6) desired = 4;
        }
        else if (plan == ScriptedPlan::Greedy) {
            if (game.gameTimeSeconds > 170.f && game.playerEconomyLevel >= 3) desired = 2;
            if (game.gameTimeSeconds > 360.f && game.playerUpgradeLevel >= 5) desired = 3;
            if (game.gameTimeSeconds > 620.f && game.playerUpgradeLevel >= 9) desired = 4;
        }
        else {
            if (game.gameTimeSeconds > 100.f && game.playerEconomyLevel >= 2) desired = 2;
            if (game.gameTimeSeconds > 250.f && game.playerUpgradeLevel >= 4) desired = 3;
            if (game.gameTimeSeconds > 520.f && game.playerUpgradeLevel >= 5) desired = 4;
        }
        return std::min(desired, game.buildingCap(PLAYER, building::Barracks));
    }

    int laneWithMostEnemyUnits(const Game& game)
    {
        std::array<int, lane::Count> counts{};
        for (const auto& unit : game.enemys) {
            if (unit->Health > 0) {
                ++counts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
            }
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (counts[i] > counts[bestLane]) {
                bestLane = i;
            }
        }
        return bestLane;
    }

    int chooseScriptedLane(const Game& game, ScriptedPlan plan, int orderIndex)
    {
        const int pressure = game.unitsNearPoint(AI, game.Red_baseP, 13);
        if (pressure >= 4) {
            return laneWithMostEnemyUnits(game);
        }
        if (plan == ScriptedPlan::Rush && game.gameTimeSeconds < 190.f) {
            return lane::Mid;
        }
        return (static_cast<int>(game.gameTimeSeconds / 42.f) + orderIndex) % lane::Count;
    }

    std::vector<int> unitPriorityForPlan(const Game& game, ScriptedPlan plan)
    {
        const bool underPressure = game.unitsNearPoint(AI, game.Red_baseP, 13) >= 4
            || game.myunits.size() + 4 < game.enemys.size();
        if (underPressure) {
            return {UName::GUARDIAN, UName::INFANTARY, UName::CAVALRY, UName::SHOOTER, UName::SIEGE};
        }
        if (plan == ScriptedPlan::Rush) {
            return {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
        }
        if (plan == ScriptedPlan::Greedy) {
            return {UName::GUARDIAN, UName::SIEGE, UName::SHOOTER, UName::CAVALRY, UName::INFANTARY};
        }
        if (game.gameTimeSeconds > 360.f || game.totalBuildingCount(AI, building::DefenseTower) > 0) {
            return {UName::SIEGE, UName::GUARDIAN, UName::CAVALRY, UName::SHOOTER, UName::INFANTARY};
        }
        return {UName::SHOOTER, UName::INFANTARY, UName::CAVALRY, UName::SIEGE, UName::GUARDIAN};
    }

    class ScriptedOperationQueue
    {
    public:
        explicit ScriptedOperationQueue(ScriptedPlan plan) :
            plan(plan)
        {
        }

        void update(Game& game)
        {
            if (pending.empty()) {
                refill(game);
            }

            int clicksLeft = plan == ScriptedPlan::Rush ? 6 : 5;
            while (!pending.empty() && clicksLeft > 0) {
                const GameOperation operation = pending.front();
                pending.pop_front();
                --clicksLeft;
                recordAttempt(operation);
                const bool ok = game.executeOperation(PLAYER, operation);
                if (ok) {
                    recordSuccess(operation);
                }
                game.logEvent(std::string("script ") + scriptedPlanName(plan) + " "
                    + game.describeOperation(operation) + (ok ? " ok" : " blocked"));
            }
        }

        void printSummary() const
        {
            std::clog << "[ops] plan=" << scriptedPlanName(plan)
                << " attempted=" << attempted
                << " succeeded=" << succeeded
                << " successRate=" << (attempted > 0 ? (succeeded * 100 / attempted) : 0)
                << "% macro=" << successfulMacroOps
                << " units=" << successfulUnitOps
                << '\n';
        }

    private:
        ScriptedPlan plan;
        std::deque<GameOperation> pending;
        std::array<int, gameop::Count> attempts{};
        std::array<int, gameop::Count> successes{};
        int attempted = 0;
        int succeeded = 0;
        int successfulMacroOps = 0;
        int successfulUnitOps = 0;

        void recordAttempt(const GameOperation& operation)
        {
            ++attempted;
            const int index = std::clamp(operation.type, 0, gameop::Count - 1);
            ++attempts[index];
        }

        void recordSuccess(const GameOperation& operation)
        {
            ++succeeded;
            const int index = std::clamp(operation.type, 0, gameop::Count - 1);
            ++successes[index];
            if (operation.type == gameop::QueueUnit) {
                ++successfulUnitOps;
            }
            else if (operation.type != gameop::SelectLane) {
                ++successfulMacroOps;
            }
        }

        bool pushIfAffordable(const Game& game, int& budget, const GameOperation& operation)
        {
            const int cost = estimatedOperationCost(game, PLAYER, operation);
            if (cost > budget) {
                return false;
            }
            pending.push_back(operation);
            budget -= std::max(0, cost);
            return true;
        }

        void refill(Game& game)
        {
            int budget = game.commandForTeam(PLAYER);
            const int pressure = game.unitsNearPoint(AI, game.Red_baseP, 13);
            const bool armyBehind = game.myunits.size() + 4 < game.enemys.size();
            const bool baseHurt = game.Base_red && game.Base_red->Health < 2450;
            const bool needsFirstBarracks = game.totalBuildingCount(PLAYER, building::Barracks) < 1;

            if (needsFirstBarracks) {
                pushIfAffordable(game, budget, GameOperation(gameop::BuildBarracks, lane::Mid));
                return;
            }

            const int desiredEconomy = desiredEconomyForPlan(plan, game.gameTimeSeconds);
            const int desiredTech = desiredTechForPlan(game, plan);
            const int desiredBarracks = desiredBarracksForPlan(game, plan);
            int desiredTowers = (plan == ScriptedPlan::Rush || game.gameTimeSeconds < 260.f) ? 0 : 1;
            if (pressure >= 5 || baseHurt) {
                desiredTowers = std::max(desiredTowers, 1 + (pressure >= 9 ? 1 : 0));
            }

            // Only one macro operation is queued per think tick so the script
            // still resembles a human clicking a small number of obvious buttons.
            bool queuedMacro = false;
            const auto pushMacro = [&](bool condition, const GameOperation& operation) {
                if (queuedMacro || !condition) {
                    return;
                }
                queuedMacro = pushIfAffordable(game, budget, operation);
            };

            if (game.completedBuildingCount(PLAYER, building::Barracks) < 1) {
                pushMacro(game.playerEconomyLevel < desiredEconomy,
                          GameOperation(gameop::UpgradeEconomy));
                return;
            }

            pushMacro(game.totalBuildingCount(PLAYER, building::DefenseTower)
                < std::min(desiredTowers, game.buildingCap(PLAYER, building::DefenseTower)),
                GameOperation(gameop::BuildTower, chooseScriptedLane(game, plan, 0)));

            if (plan == ScriptedPlan::Rush) {
                pushMacro(game.totalBuildingCount(PLAYER, building::Barracks) < desiredBarracks,
                          GameOperation(gameop::BuildBarracks, chooseScriptedLane(game, plan, 0)));
                pushMacro(game.playerUpgradeLevel < desiredTech,
                          GameOperation(gameop::UpgradeTech));
                pushMacro(game.playerEconomyLevel < desiredEconomy,
                          GameOperation(gameop::UpgradeEconomy));
            }
            else {
                pushMacro(game.playerEconomyLevel < desiredEconomy,
                          GameOperation(gameop::UpgradeEconomy));
                pushMacro(game.playerUpgradeLevel < desiredTech,
                          GameOperation(gameop::UpgradeTech));
                pushMacro(game.totalBuildingCount(PLAYER, building::Barracks) < desiredBarracks,
                          GameOperation(gameop::BuildBarracks, chooseScriptedLane(game, plan, 0)));
            }

            int reserve = 0;
            if (game.playerEconomyLevel < desiredEconomy) {
                reserve = std::max(reserve, game.economyUpgradeCost(PLAYER));
            }
            if (game.playerUpgradeLevel < desiredTech) {
                reserve = std::max(reserve, game.upgradeCostForNextLevel(PLAYER));
            }
            if (game.totalBuildingCount(PLAYER, building::Barracks) < desiredBarracks) {
                reserve = std::max(reserve, config::BarracksCost);
            }
            if (plan == ScriptedPlan::Rush) {
                reserve /= 2;
            }
            if (pressure >= 4 || armyBehind || game.myunits.size() < 6) {
                reserve = 0;
            }
            if (!game.hasUnitCapacity(PLAYER)) {
                return;
            }

            const int currentBarracks = game.completedBuildingCount(PLAYER, building::Barracks);
            int orders = std::min(currentBarracks, std::max(1, game.playerUpgradeLevel / 4 + 1));
            if (plan == ScriptedPlan::Rush) {
                ++orders;
            }
            if (pressure >= 4) {
                ++orders;
            }
            if (armyBehind) {
                ++orders;
            }
            orders = std::min(orders, 5);

            const auto priorities = unitPriorityForPlan(game, plan);
            for (int i = 0; i < orders; ++i) {
                bool queuedUnit = false;
                for (int unit : priorities) {
                    const int cost = game.unitCost(unit);
                    if (cost <= 0 || !game.isUnitUnlocked(PLAYER, unit)) {
                        continue;
                    }
                    if (budget < cost + reserve) {
                        continue;
                    }
                    queuedUnit = pushIfAffordable(game, budget,
                        GameOperation(gameop::QueueUnit, chooseScriptedLane(game, plan, i), unit));
                    if (queuedUnit) {
                        break;
                    }
                }
                if (!queuedUnit && reserve > 0 && game.myunits.size() < 8) {
                    for (int unit : priorities) {
                        const int cost = game.unitCost(unit);
                        if (cost > 0 && game.isUnitUnlocked(PLAYER, unit) && budget >= cost) {
                            queuedUnit = pushIfAffordable(game, budget,
                                GameOperation(gameop::QueueUnit, chooseScriptedLane(game, plan, i), unit));
                            break;
                        }
                    }
                }
                if (!queuedUnit) {
                    break;
                }
            }
        }
    };

    enum class PolicyAction
    {
        Economy,
        Tech,
        Barracks,
        Tower,
        Infantry,
        Shooter,
        Cavalry,
        Siege,
        Guardian,
        Wait,
        Count
    };

    constexpr std::size_t PolicyActionCount = static_cast<std::size_t>(PolicyAction::Count);
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

    const char* policyActionName(int action)
    {
        switch (static_cast<PolicyAction>(action)) {
        case PolicyAction::Economy:
            return "eco";
        case PolicyAction::Tech:
            return "tech";
        case PolicyAction::Barracks:
            return "rax";
        case PolicyAction::Tower:
            return "tower";
        case PolicyAction::Infantry:
            return "inf";
        case PolicyAction::Shooter:
            return "shoot";
        case PolicyAction::Cavalry:
            return "cav";
        case PolicyAction::Siege:
            return "siege";
        case PolicyAction::Guardian:
            return "guard";
        case PolicyAction::Wait:
            return "wait";
        case PolicyAction::Count:
        default:
            return "unknown";
        }
    }

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

    int unitForPolicyAction(PolicyAction action)
    {
        switch (action) {
        case PolicyAction::Shooter:
            return UName::SHOOTER;
        case PolicyAction::Cavalry:
            return UName::CAVALRY;
        case PolicyAction::Siege:
            return UName::SIEGE;
        case PolicyAction::Guardian:
            return UName::GUARDIAN;
        case PolicyAction::Infantry:
        default:
            return UName::INFANTARY;
        }
    }

    std::vector<PolicyChoice> legalPolicyChoices(const Game& game, int team, std::mt19937& rng)
    {
        std::vector<PolicyChoice> choices;
        std::uniform_int_distribution<int> laneDist(0, lane::Count - 1);
        const auto push = [&](PolicyAction action, const GameOperation& operation) {
            choices.push_back(PolicyChoice{static_cast<int>(action), operation});
        };

        push(PolicyAction::Wait, GameOperation(gameop::SelectLane, laneDist(rng)));
        if (game.economyUpgradeCost(team) > 0 && game.commandForTeam(team) >= game.economyUpgradeCost(team)) {
            push(PolicyAction::Economy, GameOperation(gameop::UpgradeEconomy));
        }
        if (game.upgradeCostForNextLevel(team) > 0 && game.commandForTeam(team) >= game.upgradeCostForNextLevel(team)) {
            push(PolicyAction::Tech, GameOperation(gameop::UpgradeTech));
        }
        if (game.totalBuildingCount(team, building::Barracks) < game.buildingCap(team, building::Barracks)
            && game.commandForTeam(team) >= config::BarracksCost) {
            push(PolicyAction::Barracks, GameOperation(gameop::BuildBarracks, laneDist(rng)));
        }
        if (game.totalBuildingCount(team, building::DefenseTower) < game.buildingCap(team, building::DefenseTower)
            && game.commandForTeam(team) >= config::TowerCost) {
            push(PolicyAction::Tower, GameOperation(gameop::BuildTower, laneDist(rng)));
        }
        for (PolicyAction action : {PolicyAction::Infantry, PolicyAction::Shooter, PolicyAction::Cavalry, PolicyAction::Siege, PolicyAction::Guardian}) {
            const int unit = unitForPolicyAction(action);
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
                out += policyActionName(static_cast<int>(i));
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
}


CliOptions parseCliOptions(int argc, char* argv[])
{
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--simulate") {
            options.simulate = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-player") {
            options.simulate = true;
            options.simulatePlayer = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                options.scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-plan") {
            options.simulate = true;
            options.simulatePlayer = true;
            if (i + 1 < argc) {
                options.scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-ignore-gameover") {
            options.simulate = true;
            options.simulateIgnoreGameOver = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-dt" && i + 1 < argc && looksLikeNumber(argv[i + 1])) {
            options.simulateDt = std::clamp(std::stof(argv[++i]), 0.02f, 0.25f);
        }
        else if (arg == "--train-policies") {
            options.trainPolicies = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainEpisodes = std::max(1, std::stoi(argv[++i]));
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainSeconds = std::max(60.f, std::stof(argv[++i]));
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainSeed = static_cast<unsigned int>(std::stoul(argv[++i]));
            }
        }
    }
    return options;
}

int runPolicyTrainingCommand(const CliOptions& options)
{
    runPolicyTraining(options.trainEpisodes, options.trainSeconds, options.simulateDt, options.trainSeed);
    return 0;
}

int runSimulationCommand(const CliOptions& options)
{
    Game game;
    game.debugLogging = true;
    game.autoChooseRewards = true;
    game.window.setVisible(false);
    game.gameSceneState = SCENE_GAME;
    game.clear();

    const float dt = options.simulateDt;
    const int ticks = static_cast<int>(options.simulateSeconds / dt);
    float playerScriptTimer = 0.f;
    ScriptedOperationQueue playerQueue(options.scriptedPlan);
    for (int i = 0; i < ticks && (options.simulateIgnoreGameOver || !game.gameOver); ++i) {
        if (options.simulatePlayer) {
            playerScriptTimer += dt;
            if (playerScriptTimer >= realtime::AIThinkSeconds) {
                playerScriptTimer = 0.f;
                playerQueue.update(game);
            }
        }
        game.updateRealtime(dt);
    }
    game.logDebugSummary();
    if (options.simulatePlayer) {
        playerQueue.printSummary();
    }
    return 0;
}

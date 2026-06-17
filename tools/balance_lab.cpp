#include "AutoCombat.h"
#include "Game.h"
#include "GameNames.h"
#include "Tile.h"
#include "UnitDefinition.h"
#include "UnitUpgradeDefinition.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace
{
    constexpr int PlayerLeft = 0;
    constexpr int PlayerRight = 1;

    struct ArmyPlan
    {
        std::string name;
        std::vector<int> units;
        UnitMasteryState mastery;
        int spent = 0;
    };

    struct BattleResult
    {
        bool monoWon = false;
        bool draw = false;
        float monoValue = 0.f;
        float mixValue = 0.f;
        float duration = 0.f;
    };

    struct Aggregate
    {
        int trials = 0;
        int monoWins = 0;
        int draws = 0;
        float valueEdge = 0.f;
        float totalSeconds = 0.f;
    };

    int masteryCostToLevel(int unitName, int targetLevel)
    {
        int cost = 0;
        for (int level = 0; level < targetLevel; ++level) {
            cost += unitMasteryUpgradeCost(unitName, level);
        }
        return cost;
    }

    int affordableMasteryLevel(int unitName, int budget, float upgradeShare)
    {
        int level = 0;
        int spent = 0;
        const int unitCost = unitDefinition(unitName).commandCost;
        const int upgradeBudget = static_cast<int>(std::floor(static_cast<float>(budget) * upgradeShare));
        while (spent + unitMasteryUpgradeCost(unitName, level) <= upgradeBudget
               && budget - spent - unitMasteryUpgradeCost(unitName, level) >= unitCost) {
            spent += unitMasteryUpgradeCost(unitName, level);
            ++level;
        }
        return level;
    }

    ArmyPlan makeMonoPlan(int unitName, int budget, float upgradeShare)
    {
        ArmyPlan plan;
        const UnitDefinition& definition = unitDefinition(unitName);
        const int masteryLevel = affordableMasteryLevel(unitName, budget, upgradeShare);
        setUnitMasteryLevel(plan.mastery, unitName, masteryLevel);
        plan.spent = masteryCostToLevel(unitName, masteryLevel);
        const int count = std::max(1, (budget - plan.spent) / definition.commandCost);
        plan.units.assign(count, unitName);
        plan.spent += count * definition.commandCost;
        plan.name = std::string(definition.debugName) + "_mono_L" + std::to_string(masteryLevel);
        return plan;
    }

    void buyRoundRobinMastery(ArmyPlan& plan, const std::vector<int>& pattern, int budget, float upgradeShare)
    {
        const int upgradeBudget = static_cast<int>(std::floor(static_cast<float>(budget) * upgradeShare));
        bool bought = true;
        while (bought) {
            bought = false;
            for (int unitName : pattern) {
                const int level = unitMasteryLevel(plan.mastery, unitName);
                const int nextCost = unitMasteryUpgradeCost(unitName, level);
                if (plan.spent + nextCost <= upgradeBudget) {
                    setUnitMasteryLevel(plan.mastery, unitName, level + 1);
                    plan.spent += nextCost;
                    bought = true;
                }
            }
        }
    }

    ArmyPlan makePatternPlan(const std::string& name, const std::vector<int>& pattern, int budget, float upgradeShare)
    {
        ArmyPlan plan;
        plan.name = name;
        buyRoundRobinMastery(plan, pattern, budget, upgradeShare);

        std::size_t cursor = 0;
        while (true) {
            bool bought = false;
            for (std::size_t attempt = 0; attempt < pattern.size(); ++attempt) {
                const int unitName = pattern[(cursor + attempt) % pattern.size()];
                const int cost = unitDefinition(unitName).commandCost;
                if (plan.spent + cost <= budget) {
                    plan.units.push_back(unitName);
                    plan.spent += cost;
                    cursor = (cursor + attempt + 1) % pattern.size();
                    bought = true;
                    break;
                }
            }
            if (!bought || plan.units.size() >= config::MaxUnits) {
                break;
            }
        }
        return plan;
    }

    const char* unitName(int unitName)
    {
        return game_internal::unitDebugName(unitName);
    }

    std::string planComposition(const ArmyPlan& plan)
    {
        std::array<int, TrainableUnitCount> counts{};
        for (int unit : plan.units) {
            ++counts[trainableUnitIndex(unit)];
        }
        std::string out;
        for (const UnitDefinition& definition : unitDefinitions()) {
            const int count = counts[trainableUnitIndex(definition.unitName)];
            const int mastery = unitMasteryLevel(plan.mastery, definition.unitName);
            if (count <= 0 && mastery <= 0) {
                continue;
            }
            if (!out.empty()) {
                out += " ";
            }
            out += definition.debugName;
            out += "x" + std::to_string(count);
            if (mastery > 0) {
                out += "+L" + std::to_string(mastery);
            }
        }
        return out.empty() ? "empty" : out;
    }

    void clearArena(Game& game)
    {
        for (int y = 4; y <= 25; ++y) {
            for (int x = 8; x <= 38; ++x) {
                if (game.isMapCell(x, y)) {
                    game.setTileID(x, y, tile::Empty);
                }
            }
        }
    }

    void applyMastery(Game& game, int team, const UnitMasteryState& mastery)
    {
        UnitMasteryState& target = team == PLAYER ? game.playerMastery : game.aiMastery;
        target = mastery;
    }

    bool spawnArmy(Game& game, int team, const ArmyPlan& plan, int side)
    {
        applyMastery(game, team, plan.mastery);
        const int frontX = side == PlayerLeft ? 17 : 29;
        const int direction = side == PlayerLeft ? -1 : 1;
        const int topY = 7;
        constexpr int rows = 17;
        for (std::size_t i = 0; i < plan.units.size(); ++i) {
            const int row = static_cast<int>(i % rows);
            const int col = static_cast<int>(i / rows);
            const int x = frontX + col * direction;
            const int y = topY + row;
            if (!game.createUnit(team, plan.units[i], x, y, lane::Mid)) {
                std::cerr << "balance_lab: failed to spawn " << unitName(plan.units[i])
                          << " for " << plan.name << " at " << x << "," << y << '\n';
                return false;
            }
            MoveableUnit* unit = team == PLAYER ? game.myunits.back().get() : game.enemys.back().get();
            unit->nextRallyStage = 1;
        }
        return true;
    }

    float remainingArmyValue(const Game& game, int team)
    {
        const auto& roster = team == PLAYER ? game.myunits : game.enemys;
        float value = 0.f;
        for (const auto& unit : roster) {
            if (unit->Health <= 0 || !isTrainableUnit(unit->unitName)) {
                continue;
            }
            const UnitDefinition& definition = unitDefinition(unit->unitName);
            const float maxHealth = std::max(1.f, static_cast<float>(definition.maxHealth)
                * game.unitHealthMultiplier(team, unit->unitName));
            value += static_cast<float>(definition.commandCost) * std::clamp(static_cast<float>(unit->Health) / maxHealth, 0.f, 1.f);
        }
        return value;
    }

    BattleResult runBattle(Game& game, const ArmyPlan& mono, const ArmyPlan& mix, bool monoAsPlayer)
    {
        game.clear();
        game.window.setVisible(false);
        game.externalAIControl = true;
        game.autoChooseRewards = true;
        game.debugLogging = false;
        game.gameSceneState = SCENE_GAME;
        game.gameTimeSeconds = 720.f;
        clearArena(game);

        const ArmyPlan& playerPlan = monoAsPlayer ? mono : mix;
        const ArmyPlan& aiPlan = monoAsPlayer ? mix : mono;
        if (!spawnArmy(game, PLAYER, playerPlan, PlayerLeft) || !spawnArmy(game, AI, aiPlan, PlayerRight)) {
            return BattleResult{false, true, 0.f, 0.f, 0.f};
        }

        constexpr float dt = 0.25f;
        constexpr float maxSeconds = 150.f;
        float elapsed = 0.f;
        while (elapsed < maxSeconds && !game.myunits.empty() && !game.enemys.empty()) {
            game.updateRealtime(dt);
            elapsed += dt;
        }

        const bool playerAlive = !game.myunits.empty();
        const bool aiAlive = !game.enemys.empty();
        const bool playerWon = playerAlive && !aiAlive;
        const bool aiWon = aiAlive && !playerAlive;
        const bool monoWon = monoAsPlayer ? playerWon : aiWon;
        const float monoValue = remainingArmyValue(game, monoAsPlayer ? PLAYER : AI);
        const float mixValue = remainingArmyValue(game, monoAsPlayer ? AI : PLAYER);
        return BattleResult{monoWon, !playerWon && !aiWon, monoValue, mixValue, elapsed};
    }

    std::vector<std::pair<std::string, std::vector<int>>> mixedPatterns()
    {
        return {
            {"core", {UName::INFANTARY, UName::SHOOTER, UName::INFANTARY, UName::SHOOTER, UName::CAVALRY}},
            {"anti_cavalry", {UName::INFANTARY, UName::INFANTARY, UName::INFANTARY, UName::SHOOTER, UName::GUARDIAN}},
            {"frontline", {UName::GUARDIAN, UName::SHOOTER, UName::SIEGE, UName::INFANTARY, UName::CAVALRY}},
            {"breach", {UName::GUARDIAN, UName::SIEGE, UName::SHOOTER, UName::INFANTARY, UName::SIEGE}},
        };
    }
}

int main(int argc, char* argv[])
{
    const bool csv = argc > 1 && std::string(argv[1]) == "--csv";
    const std::vector<int> budgets = {240, 360, 480, 720, 960, 1200};
    const std::vector<float> monoUpgradeShares = {0.0f, 0.25f, 0.45f};
    constexpr float mixUpgradeShare = 0.15f;

    Game game;
    game.window.setVisible(false);

    if (csv) {
        std::cout << "budget,mono_unit,upgrade_share,mix,mirror,mono_spent,mix_spent,mono_won,draw,mono_value,mix_value,duration,mono_comp,mix_comp\n";
    }

    std::array<Aggregate, TrainableUnitCount> perUnit{};
    int totalTrials = 0;
    int totalMonoWins = 0;
    int totalDraws = 0;
    float totalEdge = 0.f;

    for (int budget : budgets) {
        for (const UnitDefinition& definition : unitDefinitions()) {
            for (float monoShare : monoUpgradeShares) {
                const ArmyPlan mono = makeMonoPlan(definition.unitName, budget, monoShare);
                for (const auto& [mixName, pattern] : mixedPatterns()) {
                    const ArmyPlan mix = makePatternPlan(mixName, pattern, budget, mixUpgradeShare);
                    for (int mirror = 0; mirror < 2; ++mirror) {
                        const bool monoAsPlayer = mirror == 0;
                        const BattleResult result = runBattle(game, mono, mix, monoAsPlayer);

                        Aggregate& aggregate = perUnit[trainableUnitIndex(definition.unitName)];
                        ++aggregate.trials;
                        ++totalTrials;
                        if (result.monoWon) {
                            ++aggregate.monoWins;
                            ++totalMonoWins;
                        }
                        if (result.draw) {
                            ++aggregate.draws;
                            ++totalDraws;
                        }
                        aggregate.valueEdge += result.monoValue - result.mixValue;
                        aggregate.totalSeconds += result.duration;
                        totalEdge += result.monoValue - result.mixValue;

                        if (csv) {
                            std::cout << budget << ','
                                      << definition.debugName << ','
                                      << std::fixed << std::setprecision(2) << monoShare << ','
                                      << mixName << ','
                                      << (monoAsPlayer ? "left" : "right") << ','
                                      << mono.spent << ','
                                      << mix.spent << ','
                                      << (result.monoWon ? 1 : 0) << ','
                                      << (result.draw ? 1 : 0) << ','
                                      << std::setprecision(1) << result.monoValue << ','
                                      << result.mixValue << ','
                                      << result.duration << ','
                                      << '"' << planComposition(mono) << "\","
                                      << '"' << planComposition(mix) << "\"\n";
                        }
                    }
                }
            }
        }
    }

    if (!csv) {
        std::cout << "Balance lab: mono upgraded armies vs equal-budget mixed armies\n";
        std::cout << "Trials: " << totalTrials
                  << "  monoWinRate=" << std::fixed << std::setprecision(1)
                  << (100.f * static_cast<float>(totalMonoWins) / static_cast<float>(std::max(1, totalTrials))) << "%"
                  << "  drawRate=" << (100.f * static_cast<float>(totalDraws) / static_cast<float>(std::max(1, totalTrials))) << "%"
                  << "  avgValueEdge=" << (totalEdge / static_cast<float>(std::max(1, totalTrials))) << "\n\n";

        for (const UnitDefinition& definition : unitDefinitions()) {
            const Aggregate& aggregate = perUnit[trainableUnitIndex(definition.unitName)];
            std::cout << std::setw(9) << definition.debugName
                      << " trials=" << std::setw(3) << aggregate.trials
                      << " monoWinRate=" << std::setw(5) << std::setprecision(1)
                      << (100.f * static_cast<float>(aggregate.monoWins) / static_cast<float>(std::max(1, aggregate.trials))) << "%"
                      << " drawRate=" << std::setw(5)
                      << (100.f * static_cast<float>(aggregate.draws) / static_cast<float>(std::max(1, aggregate.trials))) << "%"
                      << " avgValueEdge=" << std::setw(7)
                      << (aggregate.valueEdge / static_cast<float>(std::max(1, aggregate.trials)))
                      << " avgSeconds=" << std::setw(5)
                      << (aggregate.totalSeconds / static_cast<float>(std::max(1, aggregate.trials)))
                      << '\n';
        }
    }

    return 0;
}

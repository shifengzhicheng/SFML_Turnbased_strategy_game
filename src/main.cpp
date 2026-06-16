#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>

#include "Game.h"
#include "RealtimeConfig.h"

namespace
{
    enum class ScriptedPlan
    {
        Balanced,
        Rush,
        Greedy
    };

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
}

int main(int argc, char* argv[])
{
    bool simulate = false;
    bool simulatePlayer = false;
    bool simulateIgnoreGameOver = false;
    ScriptedPlan scriptedPlan = ScriptedPlan::Balanced;
    float simulateSeconds = 120.f;
    float simulateDt = 0.10f;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--simulate") {
            simulate = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-player") {
            simulate = true;
            simulatePlayer = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                simulateSeconds = std::stof(argv[++i]);
            }
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-plan") {
            simulate = true;
            simulatePlayer = true;
            if (i + 1 < argc) {
                scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-ignore-gameover") {
            simulate = true;
            simulateIgnoreGameOver = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-dt" && i + 1 < argc && looksLikeNumber(argv[i + 1])) {
            simulateDt = std::clamp(std::stof(argv[++i]), 0.02f, 0.25f);
        }
    }

    if (argc > 0) {
        const auto executablePath = std::filesystem::weakly_canonical(argv[0]);
        const auto executableDir = executablePath.parent_path();
        if (!executableDir.empty()) {
            std::filesystem::current_path(executableDir);
        }
    }

    Game game;
    if (simulate) {
        game.debugLogging = true;
        game.autoChooseRewards = true;
        game.window.setVisible(false);
        game.gameSceneState = SCENE_GAME;
        game.clear();

        const float dt = simulateDt;
        const int ticks = static_cast<int>(simulateSeconds / dt);
        float playerScriptTimer = 0.f;
        ScriptedOperationQueue playerQueue(scriptedPlan);
        for (int i = 0; i < ticks && (simulateIgnoreGameOver || !game.gameOver); ++i) {
            if (simulatePlayer) {
                playerScriptTimer += dt;
                if (playerScriptTimer >= realtime::AIThinkSeconds) {
                    playerScriptTimer = 0.f;
                    playerQueue.update(game);
                }
            }
            game.updateRealtime(dt);
        }
        game.logDebugSummary();
        if (simulatePlayer) {
            playerQueue.printSummary();
        }
        return 0;
    }

    game.run();
    return 0;
}

#include "AIController.h"

#include "AllUnit.h"
#include "Building.h"
#include "Config.h"
#include "Game.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <list>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace
{
    enum class ModelAction
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

    struct Candidate
    {
        ModelAction action = ModelAction::Wait;
        GameOperation operation;
    };

    constexpr int ActionCount = static_cast<int>(ModelAction::Count);
    constexpr int FeatureCount = 12;

    int actionIndex(ModelAction action)
    {
        return static_cast<int>(action);
    }

    int unitForAction(ModelAction action)
    {
        switch (action) {
        case ModelAction::Shooter:
            return UName::SHOOTER;
        case ModelAction::Cavalry:
            return UName::CAVALRY;
        case ModelAction::Siege:
            return UName::SIEGE;
        case ModelAction::Guardian:
            return UName::GUARDIAN;
        case ModelAction::Infantry:
        default:
            return UName::INFANTARY;
        }
    }

    bool isMacro(ModelAction action)
    {
        return action == ModelAction::Economy
            || action == ModelAction::Tech
            || action == ModelAction::Barracks
            || action == ModelAction::Tower;
    }

    int countUnitsNamed(const std::list<std::unique_ptr<MoveableUnit>>& units, int name)
    {
        return static_cast<int>(std::count_if(units.begin(), units.end(), [name](const std::unique_ptr<MoveableUnit>& unit) {
            return unit->Health > 0 && unit->unitName == name;
        }));
    }

    int nearestLaneForY(const Game& game, int y)
    {
        int bestLane = lane::Mid;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < lane::Count; ++i) {
            const int laneY = game.laneWaypoint(AI, i, 1).y;
            const int distance = std::abs(y - laneY);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestLane = i;
            }
        }
        return bestLane;
    }

    int laneWithMostPlayerUnits(const Game& game)
    {
        int counts[lane::Count] = {};
        for (const auto& unit : game.myunits) {
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

    int laneWithMostPlayerStructures(const Game& game)
    {
        int scores[lane::Count] = {};
        for (const auto& building : game.buildings) {
            if (building.team != PLAYER || !building.complete) {
                continue;
            }
            const int laneIndex = nearestLaneForY(game, building.point.y);
            scores[laneIndex] += building.type == building::DefenseTower ? 4 : 2;
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (scores[i] > scores[bestLane]) {
                bestLane = i;
            }
        }
        return scores[bestLane] > 0 ? bestLane : laneWithMostPlayerUnits(game);
    }

    int chooseModelLane(const Game& game, ModelAction action, int orderIndex)
    {
        const int playerPressure = game.unitsNearPoint(PLAYER, game.Blue_baseP, 13);
        const int aiPressure = game.unitsNearPoint(AI, game.Red_baseP, 13);
        if (action == ModelAction::Tower || playerPressure >= 4) {
            return laneWithMostPlayerUnits(game);
        }
        if (action == ModelAction::Siege || aiPressure > 0 || game.totalBuildingCount(PLAYER, building::DefenseTower) > 0) {
            return laneWithMostPlayerStructures(game);
        }
        return (static_cast<int>(game.gameTimeSeconds / 34.f) + orderIndex) % lane::Count;
    }

    std::array<float, FeatureCount> featuresForAI(const Game& game)
    {
        const float playerPressure = static_cast<float>(game.unitsNearPoint(PLAYER, game.Blue_baseP, 13));
        const float aiPressure = static_cast<float>(game.unitsNearPoint(AI, game.Red_baseP, 13));
        return {
            1.f,
            std::clamp(game.gameTimeSeconds / 900.f, 0.f, 1.5f),
            static_cast<float>(game.aiCommand) / static_cast<float>(config::MaxCommand),
            static_cast<float>(game.aiEconomyLevel) / static_cast<float>(config::MaxEconomyLevel),
            static_cast<float>(game.aiUpgradeLevel) / static_cast<float>(config::MaxTechLevel),
            static_cast<float>(game.completedBuildingCount(AI, building::Barracks)) / static_cast<float>(std::max(1, game.buildingCap(AI, building::Barracks))),
            static_cast<float>(game.totalBuildingCount(AI, building::DefenseTower)) / static_cast<float>(std::max(1, game.buildingCap(AI, building::DefenseTower))),
            static_cast<float>(game.enemys.size()) / static_cast<float>(config::MaxUnits),
            static_cast<float>(game.myunits.size()) / static_cast<float>(config::MaxUnits),
            static_cast<float>(game.Base_blue ? game.Base_blue->Health : 0) / 4000.f,
            playerPressure / 16.f,
            aiPressure / 16.f
        };
    }

    int desiredEconomy(const Game& game)
    {
        int desired = 1;
        if (game.gameTimeSeconds > 25.f) desired = 2;
        if (game.gameTimeSeconds > 70.f) desired = 3;
        if (game.gameTimeSeconds > 135.f) desired = 4;
        if (game.gameTimeSeconds > 230.f) desired = 5;
        if (game.gameTimeSeconds > 360.f) desired = 7;
        if (game.gameTimeSeconds > 520.f) desired = 9;
        if (game.gameTimeSeconds > 700.f) desired = 11;
        if (game.gameTimeSeconds > 840.f) desired = config::MaxEconomyLevel;
        if (game.playerEconomyLevel > game.aiEconomyLevel + 1 || game.enemys.size() >= 14) {
            ++desired;
        }
        return std::clamp(desired, 0, config::MaxEconomyLevel);
    }

    int desiredTech(const Game& game)
    {
        int allowed = static_cast<int>(game.gameTimeSeconds / 46.f);
        allowed = std::min(allowed, game.aiEconomyLevel + (game.gameTimeSeconds > 600.f ? 7 : 5));
        if (game.playerUpgradeLevel > game.aiUpgradeLevel) {
            allowed = std::max(allowed, game.playerUpgradeLevel + 1);
        }
        if (game.gameTimeSeconds > 620.f) {
            allowed = std::max(allowed, 10 + static_cast<int>((game.gameTimeSeconds - 620.f) / 46.f));
        }
        if (game.gameTimeSeconds > 840.f) {
            allowed = config::MaxTechLevel;
        }
        return std::clamp(allowed, 0, config::MaxTechLevel);
    }

    int desiredBarracks(const Game& game)
    {
        int desired = 1;
        if (game.gameTimeSeconds > 80.f && game.aiEconomyLevel >= 1) desired = 2;
        if (game.gameTimeSeconds > 210.f && game.aiUpgradeLevel >= 2) desired = 3;
        if (game.gameTimeSeconds > 360.f && game.aiUpgradeLevel >= 5) desired = 4;
        if (game.gameTimeSeconds > 560.f && game.aiUpgradeLevel >= 8) desired = 5;
        if (game.gameTimeSeconds > 760.f) desired = 6;
        if (game.enemys.size() + 4 < game.myunits.size()) {
            ++desired;
        }
        return std::min(desired, game.buildingCap(AI, building::Barracks));
    }

    bool isUnitAction(ModelAction action)
    {
        return action == ModelAction::Infantry
            || action == ModelAction::Shooter
            || action == ModelAction::Cavalry
            || action == ModelAction::Siege
            || action == ModelAction::Guardian;
    }

    std::vector<Candidate> legalCandidates(Game& game, bool allowMacro, int orderIndex)
    {
        std::vector<Candidate> candidates;
        const auto push = [&](ModelAction action, const GameOperation& operation) {
            candidates.push_back(Candidate{action, operation});
        };

        push(ModelAction::Wait, GameOperation(gameop::SelectLane, chooseModelLane(game, ModelAction::Wait, orderIndex)));
        if (allowMacro && game.economyUpgradeCost(AI) > 0 && game.aiCommand >= game.economyUpgradeCost(AI)) {
            push(ModelAction::Economy, GameOperation(gameop::UpgradeEconomy));
        }
        if (allowMacro && game.upgradeCostForNextLevel(AI) > 0 && game.aiCommand >= game.upgradeCostForNextLevel(AI)) {
            push(ModelAction::Tech, GameOperation(gameop::UpgradeTech));
        }
        if (allowMacro
            && game.totalBuildingCount(AI, building::Barracks) < game.buildingCap(AI, building::Barracks)
            && game.aiCommand >= config::BarracksCost) {
            push(ModelAction::Barracks, GameOperation(gameop::BuildBarracks, chooseModelLane(game, ModelAction::Barracks, orderIndex)));
        }
        if (allowMacro
            && game.totalBuildingCount(AI, building::DefenseTower) < game.buildingCap(AI, building::DefenseTower)
            && game.aiCommand >= config::TowerCost) {
            push(ModelAction::Tower, GameOperation(gameop::BuildTower, chooseModelLane(game, ModelAction::Tower, orderIndex)));
        }
        for (ModelAction action : {ModelAction::Infantry, ModelAction::Shooter, ModelAction::Cavalry, ModelAction::Siege, ModelAction::Guardian}) {
            const int unit = unitForAction(action);
            if (game.canQueueUnit(AI, unit)) {
                push(action, GameOperation(gameop::QueueUnit, chooseModelLane(game, action, orderIndex), unit));
            }
        }
        return candidates;
    }

    float modelScore(const Game& game, const Candidate& candidate, const std::array<float, FeatureCount>& f,
                     const std::array<int, ActionCount>& recentActions)
    {
        // Stable baked policy weights: the trainer can keep exploring, while
        // live games use deterministic-enough scores with a few pacing guards.
        static const std::array<std::array<float, FeatureCount>, ActionCount> weights = {{
            {{ 0.10f,  0.70f,  1.45f, -1.35f, -0.28f,  0.08f, -0.10f,  0.18f, -0.12f,  0.18f, -0.65f,  0.16f}},
            {{-0.05f,  0.92f,  1.18f,  0.10f, -1.18f,  0.12f, -0.05f,  0.10f,  0.03f,  0.10f, -0.30f,  0.20f}},
            {{ 0.18f,  0.62f,  0.95f,  0.38f,  0.20f, -1.25f, -0.10f, -0.15f,  0.28f,  0.16f, -0.22f,  0.24f}},
            {{-0.70f,  0.35f,  0.70f,  0.02f,  0.18f,  0.05f, -1.00f, -0.08f,  0.15f, -0.35f,  1.35f, -0.18f}},
            {{ 0.20f, -0.35f,  0.62f, -0.10f, -0.08f,  0.50f,  0.00f, -0.70f,  0.70f, -0.08f,  0.32f, -0.10f}},
            {{ 0.08f, -0.20f,  0.82f,  0.20f,  0.04f,  0.45f,  0.00f, -0.58f,  0.58f, -0.05f,  0.12f,  0.24f}},
            {{-0.08f,  0.12f,  1.02f,  0.35f,  0.24f,  0.55f, -0.03f, -0.48f,  0.45f,  0.02f,  0.18f,  0.35f}},
            {{-0.42f,  1.10f,  1.05f,  0.50f,  0.80f,  0.78f, -0.10f, -0.35f,  0.35f,  0.00f, -0.05f,  1.10f}},
            {{-0.58f,  0.95f,  1.02f,  0.55f,  0.82f,  0.80f, -0.05f, -0.42f,  0.52f, -0.02f,  0.60f,  0.25f}},
            {{-0.48f,  0.05f, -0.95f,  0.06f,  0.02f, -0.18f,  0.00f,  0.82f, -0.70f,  0.22f, -0.50f,  0.12f}}
        }};

        const int index = actionIndex(candidate.action);
        float score = 0.f;
        for (int i = 0; i < FeatureCount; ++i) {
            score += weights[static_cast<std::size_t>(index)][static_cast<std::size_t>(i)] * f[static_cast<std::size_t>(i)];
        }

        const int economyTarget = desiredEconomy(game);
        const int techTarget = desiredTech(game);
        const int barracksTarget = desiredBarracks(game);
        const bool pressure = game.unitsNearPoint(PLAYER, game.Blue_baseP, 13) >= 4 || (game.Base_blue && game.Base_blue->Health < 2700);
        const bool openingNeedsUnits = game.enemys.size() < 4 && game.gameTimeSeconds < 115.f && !pressure;
        const bool playerTurtling = game.totalBuildingCount(PLAYER, building::DefenseTower) > 0
            || game.totalBuildingCount(PLAYER, building::Barracks) >= 3;
        const int techCost = game.upgradeCostForNextLevel(AI);
        const int economyCost = game.economyUpgradeCost(AI);
        const bool needsFirstBarracks = game.totalBuildingCount(AI, building::Barracks) < 1;
        const bool needsTech = techCost > 0
            && game.aiUpgradeLevel < techTarget
            && (game.gameTimeSeconds > 80.f || game.aiEconomyLevel >= 1);
        const bool needsEconomy = economyCost > 0 && game.aiEconomyLevel < economyTarget;
        const bool safeToBank = !pressure
            && (game.enemys.size() + 10 >= game.myunits.size() || game.enemys.size() >= 24);
        const bool economyLag = needsEconomy
            && (game.aiEconomyLevel < std::min(4, economyTarget) || game.aiEconomyLevel + 2 < economyTarget);
        const bool techLag = needsTech
            && (game.aiUpgradeLevel < std::min(3, techTarget)
                || game.aiUpgradeLevel + 2 < techTarget
                || game.playerUpgradeLevel > game.aiUpgradeLevel);
        const bool prioritizeEconomy = economyLag
            && (game.aiUpgradeLevel >= 3 || game.aiEconomyLevel < 2 || !techLag);
        const bool prioritizeTech = techLag && !prioritizeEconomy;
        const bool bankForTech = prioritizeTech
            && game.aiCommand < techCost
            && safeToBank
            && !openingNeedsUnits
            && game.gameTimeSeconds > 95.f;
        const bool bankForEconomy = needsEconomy
            && prioritizeEconomy
            && game.aiCommand < economyCost
            && safeToBank
            && !openingNeedsUnits;

        switch (candidate.action) {
        case ModelAction::Economy:
            score += needsEconomy ? 1.35f : -1.0f;
            if (prioritizeEconomy) {
                score += 0.85f;
            }
            if (needsTech && game.gameTimeSeconds > 130.f) {
                score -= 0.35f;
            }
            break;
        case ModelAction::Tech:
            score += needsTech ? 1.85f : -0.8f;
            if (prioritizeTech) {
                score += 0.85f;
            }
            if (needsTech && game.aiCommand >= techCost) {
                score += 0.45f;
            }
            score += game.aiEconomyLevel >= 1 ? 0.25f : -0.20f;
            break;
        case ModelAction::Barracks:
            score += game.totalBuildingCount(AI, building::Barracks) < barracksTarget ? 1.25f : -0.9f;
            if (needsFirstBarracks) {
                score += 2.15f;
            }
            break;
        case ModelAction::Tower:
            score += pressure ? 1.5f : -0.9f;
            break;
        case ModelAction::Infantry:
            score += game.gameTimeSeconds < 80.f ? 0.7f : 0.08f;
            break;
        case ModelAction::Shooter:
            score += game.gameTimeSeconds < 220.f ? 0.65f : 0.20f;
            score += countUnitsNamed(game.myunits, UName::INFANTARY) * 0.02f;
            break;
        case ModelAction::Cavalry:
            score += game.gameTimeSeconds > 180.f ? 0.75f : -0.25f;
            score += countUnitsNamed(game.myunits, UName::SHOOTER) * 0.04f;
            score += countUnitsNamed(game.myunits, UName::SIEGE) * 0.08f;
            break;
        case ModelAction::Siege:
            score += (playerTurtling || game.gameTimeSeconds > 520.f) ? 1.45f : -0.75f;
            break;
        case ModelAction::Guardian:
            score += (pressure || game.gameTimeSeconds > 620.f) ? 0.9f : -0.55f;
            break;
        case ModelAction::Wait: {
            const int macroCost = needsFirstBarracks ? config::BarracksCost
                : (needsTech ? techCost : (needsEconomy ? economyCost : 0));
            score += (macroCost > 0 && game.aiCommand + 18 >= macroCost) ? 1.05f : -0.8f;
            if (bankForTech) {
                score += 1.65f;
            }
            if (bankForEconomy) {
                score += 1.45f;
            }
            break;
        }
        case ModelAction::Count:
            break;
        }

        // A model-only AI tends to greedily spend every small budget on units.
        // This reserve term keeps it banking toward tech/economy timings unless
        // the base is under real pressure.
        if (isUnitAction(candidate.action) && !openingNeedsUnits) {
            const int unitCost = game.unitCost(candidate.operation.unitName);
            if (bankForTech || (prioritizeTech && safeToBank && game.aiCommand < techCost + unitCost)) {
                score -= 1.18f;
            }
            else if (bankForEconomy || (prioritizeEconomy && safeToBank
                && game.aiCommand < economyCost + unitCost)) {
                score -= 0.82f;
            }
        }

        if (candidate.action != ModelAction::Wait) {
            score -= static_cast<float>(recentActions[static_cast<std::size_t>(index)]) * 0.18f;
        }
        return score;
    }
}

void AIController::reset()
{
    thinkTimer = 0.f;
    decisionStep = 0;
    rng.seed(0xA11CEu);
    recentActions.fill(0);
}

void AIController::update(Game& game, float dt)
{
    thinkTimer += dt;
    if (thinkTimer < realtime::AIThinkSeconds) {
        return;
    }
    thinkTimer = 0.f;

    // The official AI now uses the same operation model that the self-play
    // trainer exercises: score legal actions, execute a short queue, then let
    // auto-pathing/combat resolve the consequences.
    const int maxActions = std::clamp(2 + game.aiEconomyLevel / 4 + game.aiUpgradeLevel / 6, 2, 5);
    bool macroUsed = false;
    for (int i = 0; i < maxActions; ++i) {
        auto candidates = legalCandidates(game, !macroUsed, decisionStep + i);
        if (candidates.empty()) {
            break;
        }

        const auto features = featuresForAI(game);
        std::normal_distribution<float> noise(0.f, 0.055f);
        Candidate best = candidates.front();
        float bestScore = -1e9f;
        for (const auto& candidate : candidates) {
            float score = modelScore(game, candidate, features, recentActions) + noise(rng);
            if (score > bestScore) {
                bestScore = score;
                best = candidate;
            }
        }

        if (best.action == ModelAction::Wait) {
            game.logEvent("ai policy waits to bank command");
            break;
        }

        const bool ok = game.executeOperation(AI, best.operation);
        game.logEvent(std::string("ai policy ") + game.describeOperation(best.operation) + (ok ? " ok" : " blocked"));
        if (!ok) {
            break;
        }
        if (isMacro(best.action)) {
            macroUsed = true;
        }
        for (int& count : recentActions) {
            count = std::max(0, count - 1);
        }
        recentActions[static_cast<std::size_t>(actionIndex(best.action))] += 3;
    }
    ++decisionStep;
}

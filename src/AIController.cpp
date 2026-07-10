#include "AIController.h"

#include "AllUnit.h"
#include "Building.h"
#include "BuildingDefinition.h"
#include "Config.h"
#include "Game.h"
#include "PolicyModel.h"
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
    constexpr int ActionCount = policy::ActionCount;

    struct Candidate
    {
        policy::Action action = policy::Action::Wait;
        GameOperation operation;
    };

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

    int chooseModelLane(const Game& game, policy::Action action, int orderIndex)
    {
        const int playerPressure = game.unitsNearPoint(PLAYER, game.Blue_baseP, 13);
        const int aiPressure = game.unitsNearPoint(AI, game.Red_baseP, 13);
        if (action == policy::Action::Tower || playerPressure >= 4) {
            return laneWithMostPlayerUnits(game);
        }
        if (action == policy::Action::Siege || aiPressure > 0 || game.totalBuildingCount(PLAYER, building::DefenseTower) > 0) {
            return laneWithMostPlayerStructures(game);
        }
        return (static_cast<int>(game.gameTimeSeconds / 34.f) + orderIndex) % lane::Count;
    }

    int desiredEconomy(const Game& game)
    {
        int desired = 1;
        if (game.gameTimeSeconds > 40.f) desired = 2;
        if (game.gameTimeSeconds > 105.f) desired = 3;
        if (game.gameTimeSeconds > 190.f) desired = 4;
        if (game.gameTimeSeconds > 300.f) desired = 5;
        if (game.gameTimeSeconds > 430.f) desired = 6;
        if (game.gameTimeSeconds > 570.f) desired = 7;
        if (game.gameTimeSeconds > 710.f) desired = 8;
        if (game.gameTimeSeconds > 840.f) desired = 9;
        if (game.playerEconomyLevel > game.aiEconomyLevel + 1 || game.enemys.size() >= 14) {
            ++desired;
        }
        return std::clamp(desired, 0, config::MaxEconomyLevel);
    }

    int desiredTech(const Game& game)
    {
        int allowed = static_cast<int>(game.gameTimeSeconds / 60.f);
        allowed = std::min(allowed, game.aiEconomyLevel + (game.gameTimeSeconds > 600.f ? 7 : 5));
        if (game.playerUpgradeLevel > game.aiUpgradeLevel) {
            allowed = std::max(allowed, game.playerUpgradeLevel + 1);
        }
        if (game.gameTimeSeconds > 720.f) {
            allowed = std::max(allowed, 12 + static_cast<int>((game.gameTimeSeconds - 720.f) / 60.f));
        }
        return std::clamp(allowed, 0, config::MaxTechLevel);
    }

    int desiredBarracks(const Game& game)
    {
        int desired = 1;
        if (game.gameTimeSeconds > 120.f && game.aiEconomyLevel >= 1) desired = 2;
        if (game.gameTimeSeconds > 290.f && game.aiUpgradeLevel >= 2) desired = 3;
        if (game.gameTimeSeconds > 470.f && game.aiUpgradeLevel >= 5) desired = 4;
        if (game.gameTimeSeconds > 680.f && game.aiUpgradeLevel >= 8) desired = 5;
        if (game.gameTimeSeconds > 850.f) desired = 6;
        if (game.enemys.size() + 4 < game.myunits.size()) {
            ++desired;
        }
        return std::min(desired, game.buildingCap(AI, building::Barracks));
    }

    std::vector<Candidate> legalCandidates(Game& game, bool allowMacro, int orderIndex)
    {
        std::vector<Candidate> candidates;
        const auto push = [&](policy::Action action, const GameOperation& operation) {
            candidates.push_back(Candidate{action, operation});
        };

        push(policy::Action::Wait, GameOperation(gameop::SelectLane, chooseModelLane(game, policy::Action::Wait, orderIndex)));
        if (allowMacro && game.economyUpgradeCost(AI) > 0 && game.aiCommand >= game.economyUpgradeCost(AI)) {
            push(policy::Action::Economy, GameOperation(gameop::UpgradeEconomy));
        }
        if (allowMacro && game.upgradeCostForNextLevel(AI) > 0 && game.aiCommand >= game.upgradeCostForNextLevel(AI)) {
            push(policy::Action::Tech, GameOperation(gameop::UpgradeTech));
        }
        if (allowMacro
            && game.totalBuildingCount(AI, building::Barracks) < game.buildingCap(AI, building::Barracks)
            && game.aiCommand >= buildingDefinition(building::Barracks).commandCost) {
            const int buildLane = chooseModelLane(game, policy::Action::Barracks, orderIndex);
            if (game.canRebuildLane(AI, buildLane)) {
                push(policy::Action::Barracks, GameOperation(gameop::BuildBarracks, buildLane));
            }
        }
        if (allowMacro
            && game.totalBuildingCount(AI, building::DefenseTower) < game.buildingCap(AI, building::DefenseTower)
            && game.aiCommand >= buildingDefinition(building::DefenseTower).commandCost) {
            const int buildLane = chooseModelLane(game, policy::Action::Tower, orderIndex);
            if (game.canRebuildLane(AI, buildLane)) {
                push(policy::Action::Tower, GameOperation(gameop::BuildTower, buildLane));
            }
        }
        for (policy::Action action : {policy::Action::Infantry, policy::Action::Shooter, policy::Action::Cavalry, policy::Action::Siege, policy::Action::Guardian}) {
            const int unit = policy::unitForAction(action);
            if (game.canQueueUnit(AI, unit)) {
                push(action, GameOperation(gameop::QueueUnit, chooseModelLane(game, action, orderIndex), unit));
            }
        }
        if (allowMacro && game.gameTimeSeconds > 150.f) {
            for (int unit : {UName::INFANTARY, UName::SHOOTER, UName::CAVALRY, UName::SIEGE, UName::GUARDIAN}) {
                if (game.canUpgradeUnitMastery(AI, unit)) {
                    push(policy::Action::Mastery, GameOperation(gameop::UpgradeUnitMastery, lane::Mid, unit));
                }
            }
        }
        return candidates;
    }

    float modelScore(const Game& game, const Candidate& candidate, const policy::FeatureVector& features,
                     const std::array<int, ActionCount>& recentActions)
    {
        const int index = policy::actionIndex(candidate.action);
        float score = policy::scoreAction(policy::baselineWeights(), candidate.action, features);

        const int economyTarget = desiredEconomy(game);
        const int techTarget = desiredTech(game);
        const int barracksTarget = desiredBarracks(game);
        const bool pressure = game.unitsNearPoint(PLAYER, game.Blue_baseP, 13) >= 4
            || (game.Base_blue && game.Base_blue->Health < config::BaseHealth * 2 / 3);
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
        case policy::Action::Economy:
            score += needsEconomy ? 1.35f : -1.0f;
            if (prioritizeEconomy) {
                score += 0.85f;
            }
            if (needsTech && game.gameTimeSeconds > 130.f) {
                score -= 0.35f;
            }
            break;
        case policy::Action::Tech:
            score += needsTech ? 1.85f : -0.8f;
            if (prioritizeTech) {
                score += 0.85f;
            }
            if (needsTech && game.aiCommand >= techCost) {
                score += 0.45f;
            }
            score += game.aiEconomyLevel >= 1 ? 0.25f : -0.20f;
            break;
        case policy::Action::Barracks:
            score += game.totalBuildingCount(AI, building::Barracks) < barracksTarget ? 1.25f : -0.9f;
            if (needsFirstBarracks) {
                score += 2.15f;
            }
            break;
        case policy::Action::Tower:
            score += pressure ? 1.5f : -0.9f;
            break;
        case policy::Action::Infantry:
            score += game.gameTimeSeconds < 80.f ? 0.7f : 0.08f;
            break;
        case policy::Action::Shooter:
            score += game.gameTimeSeconds < 220.f ? 0.65f : 0.20f;
            score += countUnitsNamed(game.myunits, UName::INFANTARY) * 0.02f;
            break;
        case policy::Action::Cavalry:
            score += game.gameTimeSeconds > 180.f ? 0.75f : -0.25f;
            score += countUnitsNamed(game.myunits, UName::SHOOTER) * 0.04f;
            score += countUnitsNamed(game.myunits, UName::SIEGE) * 0.08f;
            break;
        case policy::Action::Siege:
            score += (playerTurtling || game.gameTimeSeconds > 520.f) ? 1.45f : -0.75f;
            break;
        case policy::Action::Guardian:
            score += (pressure || game.gameTimeSeconds > 620.f) ? 0.9f : -0.55f;
            break;
        case policy::Action::Mastery: {
            const int unit = candidate.operation.unitName;
            const int ownCount = countUnitsNamed(game.enemys, unit);
            score += game.gameTimeSeconds > 260.f ? 0.95f : -0.70f;
            score += static_cast<float>(ownCount) * 0.07f;
            score -= static_cast<float>(game.unitMasteryLevel(AI, unit)) * 0.10f;
            if (prioritizeTech || prioritizeEconomy || needsFirstBarracks) {
                score -= 0.85f;
            }
            if (game.aiCommand > config::CommandFeatureScale * 2 / 3) {
                score += 0.45f;
            }
            break;
        }
        case policy::Action::Wait: {
            const int macroCost = needsFirstBarracks ? buildingDefinition(building::Barracks).commandCost
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
        case policy::Action::Count:
            break;
        }

        // A model-only AI tends to greedily spend every small budget on units.
        // This reserve term keeps it banking toward tech/economy timings unless
        // the base is under real pressure.
        if (policy::isUnitAction(candidate.action) && !openingNeedsUnits) {
            const int unitCost = game.unitCost(candidate.operation.unitName);
            if (bankForTech || (prioritizeTech && safeToBank && game.aiCommand < techCost + unitCost)) {
                score -= 1.18f;
            }
            else if (bankForEconomy || (prioritizeEconomy && safeToBank
                && game.aiCommand < economyCost + unitCost)) {
                score -= 0.82f;
            }
        }

        if (candidate.action != policy::Action::Wait) {
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

    const int overtimeTechTarget = desiredTech(game);
    if (game.gameTimeSeconds >= config::TechOvertimeDiscountStart
        && game.aiUpgradeLevel < overtimeTechTarget) {
        const int techCost = game.upgradeCostForNextLevel(AI);
        const bool armyCanCoverBanking = game.enemys.size() >= 5
            && game.enemys.size() + 6 >= game.myunits.size()
            && game.unitsNearPoint(PLAYER, game.Blue_baseP, 13) < 4;
        if (techCost > 0 && game.aiCommand >= techCost) {
            game.executeOperation(AI, GameOperation(gameop::UpgradeTech));
            game.logEvent("ai overtime tech priority");
            ++decisionStep;
            return;
        }
        if (techCost > 0 && armyCanCoverBanking) {
            game.logEvent("ai banks for overtime tech");
            ++decisionStep;
            return;
        }
    }

    // The official AI now uses the same operation model that the self-play
    // trainer exercises: score legal actions, execute a short queue, then let
    // auto-pathing/combat resolve the consequences.
    const int maxActions = std::clamp(1 + game.aiEconomyLevel / 7 + game.aiUpgradeLevel / 10, 1, 2);
    bool macroUsed = false;
    for (int i = 0; i < maxActions; ++i) {
        auto candidates = legalCandidates(game, !macroUsed, decisionStep + i);
        if (candidates.empty()) {
            break;
        }

        const auto features = policy::extractFeatures(game, AI);
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

        if (best.action == policy::Action::Wait) {
            game.logEvent("ai policy waits to bank command");
            break;
        }

        const bool ok = game.executeOperation(AI, best.operation);
        game.logEvent(std::string("ai policy ") + game.describeOperation(best.operation) + (ok ? " ok" : " blocked"));
        if (!ok) {
            break;
        }
        if (policy::isMacroAction(best.action)) {
            macroUsed = true;
        }
        for (int& count : recentActions) {
            count = std::max(0, count - 1);
        }
        recentActions[static_cast<std::size_t>(policy::actionIndex(best.action))] += 3;
    }
    ++decisionStep;
}

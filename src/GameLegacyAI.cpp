#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::runAIProduction()
{
    if (!Base_blue) {
        return;
    }

    const int playerPressure = unitsNearPoint(PLAYER, Blue_baseP, 13);
    const int aiPressure = unitsNearPoint(AI, Red_baseP, 13);
    const int playerTowers = totalBuildingCount(PLAYER, building::DefenseTower);
    const int playerBarracks = totalBuildingCount(PLAYER, building::Barracks);
    const bool playerTurtling = playerTowers > 0 || playerBarracks >= 3;
    const bool defenseMode = playerPressure >= 4 || (Base_blue && Base_blue->Health < 2800);
    const bool armyBehind = static_cast<int>(enemys.size()) + 4 < static_cast<int>(myunits.size());
    const bool siegeMode = !defenseMode && (playerTurtling || (gameTimeSeconds > 540.f && isUnitUnlocked(AI, UName::SIEGE)));
    const bool macroMode = !defenseMode && !armyBehind && enemys.size() >= 14;

    const auto nearestLaneForY = [this](int y) {
        int bestLane = lane::Mid;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < lane::Count; ++i) {
            const int laneY = laneWaypoint(AI, i, 1).y;
            const int distance = std::abs(y - laneY);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestLane = i;
            }
        }
        return bestLane;
    };

    const auto laneWithMostPlayerUnits = [this]() {
        int counts[lane::Count] = {};
        for (const auto& unit : myunits) {
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
    };

    const auto laneWithMostPlayerStructures = [&nearestLaneForY, &laneWithMostPlayerUnits, this]() {
        int scores[lane::Count] = {};
        for (const auto& building : buildings) {
            if (building.team != PLAYER || !building.complete) {
                continue;
            }
            const int laneIndex = nearestLaneForY(building.point.y);
            scores[laneIndex] += building.type == building::DefenseTower ? 4 : 2;
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (scores[i] > scores[bestLane]) {
                bestLane = i;
            }
        }
        return scores[bestLane] > 0 ? bestLane : laneWithMostPlayerUnits();
    };

    const auto chooseLane = [&](int orderIndex) {
        if (defenseMode) {
            return laneWithMostPlayerUnits();
        }
        if (siegeMode) {
            return laneWithMostPlayerStructures();
        }
        return (static_cast<int>(gameTimeSeconds / 34.f) + orderIndex) % lane::Count;
    };

    int desiredEconomy = 1;
    if (gameTimeSeconds > 25.f) desiredEconomy = 2;
    if (gameTimeSeconds > 70.f) desiredEconomy = 3;
    if (gameTimeSeconds > 135.f) desiredEconomy = 4;
    if (gameTimeSeconds > 230.f) desiredEconomy = 5;
    if (gameTimeSeconds > 360.f) desiredEconomy = 7;
    if (gameTimeSeconds > 520.f) desiredEconomy = 9;
    if (gameTimeSeconds > 700.f) desiredEconomy = 11;
    if (gameTimeSeconds > 840.f) desiredEconomy = config::MaxEconomyLevel;
    if (playerEconomyLevel > aiEconomyLevel + 1 || macroMode) {
        ++desiredEconomy;
    }
    desiredEconomy = std::clamp(desiredEconomy, 0, config::MaxEconomyLevel);

    int allowedTech = static_cast<int>(gameTimeSeconds / 46.f);
    allowedTech = std::min(allowedTech, aiEconomyLevel + (gameTimeSeconds > 600.f ? 7 : 5));
    if (playerUpgradeLevel > aiUpgradeLevel) {
        allowedTech = std::max(allowedTech, playerUpgradeLevel + 1);
    }
    if (gameTimeSeconds > 620.f) {
        allowedTech = std::max(allowedTech, 10 + static_cast<int>((gameTimeSeconds - 620.f) / 46.f));
    }
    if (gameTimeSeconds > 840.f) {
        allowedTech = config::MaxTechLevel;
    }
    allowedTech = std::clamp(allowedTech, 0, config::MaxTechLevel);

    int desiredBarracks = 1;
    if (gameTimeSeconds > 80.f && aiEconomyLevel >= 1) desiredBarracks = 2;
    if (gameTimeSeconds > 210.f && aiUpgradeLevel >= 2) desiredBarracks = 3;
    if (gameTimeSeconds > 360.f && aiUpgradeLevel >= 5) desiredBarracks = 4;
    if (gameTimeSeconds > 560.f && aiUpgradeLevel >= 8) desiredBarracks = 5;
    if (gameTimeSeconds > 760.f) desiredBarracks = 6;
    if (armyBehind || siegeMode) ++desiredBarracks;
    desiredBarracks = std::min(desiredBarracks, buildingCap(AI, building::Barracks));

    int desiredTowers = 0;
    if (playerPressure >= 4) desiredTowers = 1;
    if (playerPressure >= 8 || (Base_blue && Base_blue->Health < 2200)) desiredTowers = 2;
    if (gameTimeSeconds > 700.f && playerPressure >= 6 && aiUpgradeLevel >= 10) desiredTowers = 3;
    desiredTowers = std::min(desiredTowers, buildingCap(AI, building::DefenseTower));

    if (totalBuildingCount(AI, building::Barracks) < 1) {
        if (commandForTeam(AI) >= config::BarracksCost) {
            executeOperations(AI, {GameOperation(gameop::BuildBarracks, lane::Mid)});
        }
        return;
    }
    if (completedBuildingCount(AI, building::Barracks) < 1) {
        return;
    }

    bool majorActionTaken = false;
    const auto tryMajorAction = [&](bool condition, const GameOperation& operation) {
        if (majorActionTaken || !condition) {
            return;
        }
        if (executeOperations(AI, {operation}) > 0) {
            majorActionTaken = true;
        }
    };

    const bool openingNeedsUnits = enemys.size() < 4 && gameTimeSeconds < 110.f && playerPressure < 4;
    if (!openingNeedsUnits) {
        tryMajorAction(aiEconomyLevel < desiredEconomy && commandForTeam(AI) >= economyUpgradeCost(AI),
                       GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < allowedTech && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
                       GameOperation(gameop::UpgradeTech));
    }

    tryMajorAction(totalBuildingCount(AI, building::Barracks) < desiredBarracks
        && commandForTeam(AI) >= config::BarracksCost + (defenseMode ? 0 : config::InfantryCost),
        GameOperation(gameop::BuildBarracks, chooseLane(0)));

    tryMajorAction(defenseMode
        && totalBuildingCount(AI, building::DefenseTower) < desiredTowers
        && commandForTeam(AI) >= config::TowerCost + config::InfantryCost,
        GameOperation(gameop::BuildTower, laneWithMostPlayerUnits()));

    if (openingNeedsUnits && !majorActionTaken) {
        tryMajorAction(aiEconomyLevel < std::min(2, desiredEconomy)
            && enemys.size() >= 3
            && commandForTeam(AI) >= economyUpgradeCost(AI),
            GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < std::min(1, allowedTech)
            && enemys.size() >= 3
            && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
            GameOperation(gameop::UpgradeTech));
    }

    int reserve = 0;
    const bool armyEmergency = defenseMode || armyBehind || enemys.size() < 10;
    if (!armyEmergency) {
        if (aiEconomyLevel < desiredEconomy) {
            reserve = std::max(reserve, std::min(economyUpgradeCost(AI), macroMode ? 230 : 170));
        }
        if (aiUpgradeLevel < allowedTech) {
            reserve = std::max(reserve, std::min(upgradeCostForNextLevel(AI), macroMode ? 270 : 190));
        }
        if (totalBuildingCount(AI, building::Barracks) < desiredBarracks) {
            reserve = std::max(reserve, config::BarracksCost);
        }
    }
    if (majorActionTaken) {
        reserve = std::min(reserve, 55);
    }
    if (!hasUnitCapacity(AI)) {
        if (majorActionTaken) {
            logEvent(std::string("ai plan=")
                + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
                + " ecoTarget=" + std::to_string(desiredEconomy)
                + " techTarget=" + std::to_string(allowedTech)
                + " raxTarget=" + std::to_string(desiredBarracks));
        }
        return;
    }

    const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
    const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
    const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
    const int playerSiege = countUnitsNamed(myunits, UName::SIEGE);
    const int playerGuardian = countUnitsNamed(myunits, UName::GUARDIAN);

    int counterPick = UName::SHOOTER;
    if (playerShooters > playerInfantry + 1 || playerSiege > 0) {
        counterPick = UName::CAVALRY;
    }
    else if (playerCavalry + playerGuardian > playerShooters) {
        counterPick = UName::INFANTARY;
    }

    std::vector<int> priorities;
    priorities.reserve(8);
    if (defenseMode) {
        if (gameTimeSeconds < 420.f) {
            priorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, counterPick, UName::GUARDIAN};
        }
        else {
            priorities = {UName::GUARDIAN, UName::INFANTARY, UName::CAVALRY, counterPick, UName::SHOOTER};
        }
    }
    else if (siegeMode) {
        priorities = {UName::SIEGE, UName::GUARDIAN, UName::CAVALRY, counterPick, UName::SHOOTER, UName::INFANTARY};
    }
    else if (gameTimeSeconds < 300.f) {
        // Rotate a small opening roster so the AI does not look like it only
        // understands infantry spam before siege/guardian tech comes online.
        priorities = {UName::SHOOTER, UName::INFANTARY, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    else {
        priorities = {counterPick, UName::SHOOTER, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    if (aiPressure > 0 || playerTurtling) {
        priorities.insert(priorities.begin(), UName::SIEGE);
    }

    const auto leastLoadedBarracks = [this]() {
        int bestLoad = std::numeric_limits<int>::max();
        for (const auto& building : buildings) {
            if (building.team == AI && building.type == building::Barracks && building.complete) {
                bestLoad = std::min(bestLoad, building.production.load());
            }
        }
        return bestLoad == std::numeric_limits<int>::max() ? 0 : bestLoad;
    };
    const auto aiPlannedUnitCount = [this](int unitName) {
        int total = countUnitsNamed(enemys, unitName);
        for (const auto& building : buildings) {
            if (building.team != AI || building.type != building::Barracks) {
                continue;
            }
            if (building.production.activeUnit == unitName) {
                ++total;
            }
            total += static_cast<int>(std::count_if(
                building.production.orders.begin(),
                building.production.orders.end(),
                [unitName](const ProductionOrder& order) { return order.unit == unitName; }));
        }
        return total;
    };

    if (gameTimeSeconds < 120.f && !defenseMode && leastLoadedBarracks() >= 2) {
        const int openingMacroCost = aiEconomyLevel < 1
            ? economyUpgradeCost(AI)
            : (aiUpgradeLevel < 1 ? upgradeCostForNextLevel(AI) : 0);
        if (openingMacroCost > 0 && commandForTeam(AI) < openingMacroCost) {
            logEvent("ai opening banks CMD for early economy/tech");
            return;
        }
    }

    int orders = realtime::AIUnitsPerBurst + aiUpgradeLevel / 5;
    if (aiEconomyLevel >= 4) ++orders;
    if (aiEconomyLevel >= 8) ++orders;
    if (armyBehind || defenseMode) ++orders;
    if (siegeMode) ++orders;
    if (gameTimeSeconds > 760.f) ++orders;
    orders = std::min({orders, completedBuildingCount(AI, building::Barracks), 5});

    const int queueLoadLimit = macroMode ? 3 : (defenseMode ? 6 : 4);
    for (int i = 0; i < orders; ++i) {
        if (leastLoadedBarracks() >= queueLoadLimit && commandForTeam(AI) < reserve + 160 && !armyEmergency) {
            break;
        }
        if (gameTimeSeconds > 75.f && gameTimeSeconds < 155.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::SHOOTER)
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::ShooterCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first shooter");
            break;
        }
        const bool wantsOpeningCavalry = gameTimeSeconds > 185.f && gameTimeSeconds < 380.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::CAVALRY)
            && aiPlannedUnitCount(UName::CAVALRY) < 1;
        if (wantsOpeningCavalry
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::CavalryCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first cavalry");
            break;
        }

        std::vector<int> orderPriorities = priorities;
        if (wantsOpeningCavalry && commandForTeam(AI) >= config::CavalryCost) {
            orderPriorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
        }
        if (!wantsOpeningCavalry && gameTimeSeconds < 300.f && !defenseMode && !siegeMode && !orderPriorities.empty()) {
            const auto shift = static_cast<std::ptrdiff_t>(
                (static_cast<int>(gameTimeSeconds / realtime::AIThinkSeconds) + i)
                % static_cast<int>(orderPriorities.size()));
            std::rotate(orderPriorities.begin(), orderPriorities.begin() + shift, orderPriorities.end());
        }

        bool queued = false;
        for (int code : orderPriorities) {
            const int cost = unitCost(code);
            if (cost <= 0 || !canQueueUnit(AI, code)) {
                continue;
            }
            if (!armyEmergency && commandForTeam(AI) < cost + reserve) {
                continue;
            }
            if (executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), code)}) > 0) {
                queued = true;
                break;
            }
        }
        if (!queued && armyEmergency && commandForTeam(AI) >= config::InfantryCost) {
            queued = executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), UName::INFANTARY)}) > 0;
        }
        if (!queued) {
            break;
        }
    }

    if (majorActionTaken) {
        logEvent(std::string("ai plan=")
            + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
            + " ecoTarget=" + std::to_string(desiredEconomy)
            + " techTarget=" + std::to_string(allowedTech)
            + " raxTarget=" + std::to_string(desiredBarracks));
    }
}

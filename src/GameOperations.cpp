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

bool Game::canQueueUnit(int team, int name) const
{
    return unitCost(name) > 0
        && hasUnitCapacity(team)
        && isUnitUnlocked(team, name)
        && commandForTeam(team) >= unitCost(name)
        && completedBuildingCount(team, building::Barracks) > 0;
}

bool Game::enqueueUnit(int team, int name)
{
    if (!canQueueUnit(team, name)) {
        if (team == PLAYER) {
            std::string reason = "Need Barracks";
            if (completedBuildingCount(team, building::Barracks) > 0 && !isUnitUnlocked(team, name)) {
                reason = "Locked";
            }
            else if (commandForTeam(team) < unitCost(name)) {
                reason = "Need CMD";
            }
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildInfantryY) - 18.f),
                            reason, sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    Building* best = nullptr;
    for (auto& building : buildings) {
        if (building.team != team || building.type != building::Barracks || !building.complete) {
            continue;
        }
        if (best == nullptr || building.production.load() < best->production.load()) {
            best = &building;
        }
    }
    if (best == nullptr) {
        return false;
    }

    commandPool(*this, team) -= unitCost(name);
    const int orderLane = selectedLaneForTeam(team);
    best->production.orders.push_back(ProductionOrder{name, orderLane});
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(best->point.x * SqureSize, best->point.y * SqureSize - 8.f),
                        std::string("Queued ") + laneName(orderLane), sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued unit=" + std::to_string(name)
        + " lane=" + laneName(orderLane) + " rax=" + std::to_string(best->id)
        + " load=" + std::to_string(best->production.load()));
    return true;
}

bool Game::executeOperation(int team, const GameOperation& operation)
{
    // Central command dispatcher: UI clicks, scripted playtests, and AI plans
    // all go through this path so command validation stays identical.
    int& selectedLane = team == PLAYER ? playerSelectedLane : aiSelectedLane;
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);

    switch (operation.type) {
    case gameop::SelectLane:
        selectedLane = safeLane;
        return true;
    case gameop::UpgradeEconomy:
        return upgradeEconomy(team);
    case gameop::UpgradeTech:
        return upgradeTeam(team);
    case gameop::BuildBarracks:
        selectedLane = safeLane;
        return requestAutoBuildBarracks(team);
    case gameop::BuildTower:
        selectedLane = safeLane;
        return requestAutoBuildTower(team);
    case gameop::QueueUnit:
        selectedLane = safeLane;
        return enqueueUnit(team, operation.unitName);
    case gameop::UpgradeUnitMastery:
        return upgradeUnitMastery(team, operation.unitName);
    default:
        return false;
    }
}

std::size_t Game::executeOperations(int team, const std::vector<GameOperation>& operations)
{
    std::size_t successes = 0;
    for (const auto& operation : operations) {
        const bool ok = executeOperation(team, operation);
        if (ok) {
            ++successes;
        }
        logEvent(std::string(team == PLAYER ? "player" : "ai")
            + " op " + describeOperation(operation) + (ok ? " ok" : " blocked"));
    }
    return successes;
}

std::string Game::describeOperation(const GameOperation& operation) const
{
    std::string label(operationTypeName(operation.type));
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);
    if (operation.type == gameop::BuildBarracks
        || operation.type == gameop::BuildTower
        || operation.type == gameop::QueueUnit
        || operation.type == gameop::SelectLane) {
        label += "[";
        label += laneName(safeLane);
        label += "]";
    }
    if (operation.type == gameop::QueueUnit) {
        label += ":";
        label += unitDebugName(operation.unitName);
    }
    if (operation.type == gameop::UpgradeUnitMastery) {
        label += ":";
        label += unitDebugName(operation.unitName);
    }
    return label;
}

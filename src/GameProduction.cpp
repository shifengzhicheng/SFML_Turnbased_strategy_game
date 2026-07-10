#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "UnitFactory.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::updateProduction(float dt)
{
    for (auto& building : buildings) {
        if (!building.complete || building.type != building::Barracks) {
            continue;
        }
        if (building.production.activeUnit < 0 && !building.production.orders.empty()) {
            const auto order = building.production.orders.front();
            building.production.activeUnit = order.unit;
            building.production.activeLane = order.lane;
            building.production.orders.pop_front();
            building.production.progress = 0.f;
        }
        if (building.production.activeUnit < 0) {
            continue;
        }

        building.production.progress += dt;
        const float trainSeconds = unitTrainSeconds(building.production.activeUnit) * teamTrainTimeMultiplier(building.team);
        if (building.production.progress < trainSeconds) {
            continue;
        }

        const Point spawn = findSpawnPointAround(building.point);
        if (spawn.x >= 0 && createUnit(building.team, building.production.activeUnit, spawn.x, spawn.y, building.production.activeLane)) {
            building.production.activeUnit = -1;
            building.production.activeLane = lane::Mid;
            building.production.progress = 0.f;
        }
    }
}

void Game::updateEmergencyBaseTraining(float dt)
{
    const auto trainFromBase = [this, dt](int team, float& timer) {
        const bool enabled = gameTimeSeconds > 360.f && completedBuildingCount(team, building::Barracks) == 0;
        if (!enabled) {
            timer = std::min(timer + dt, 4.f);
            return;
        }

        timer += dt;
        const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
        int unitName = UName::INFANTARY;
        if (level >= 8) {
            unitName = UName::GUARDIAN;
        }
        else if (level >= 5) {
            unitName = UName::SIEGE;
        }
        else if (level >= 3) {
            unitName = UName::CAVALRY;
        }
        else if (level >= 1) {
            unitName = UName::SHOOTER;
        }

        const float interval = unitTrainSeconds(unitName) * 1.55f * teamTrainTimeMultiplier(team);
        if (timer < interval || commandForTeam(team) < unitCost(unitName) || !hasUnitCapacity(team)) {
            return;
        }

        const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
        const Point spawn = findSpawnPointAround(base);
        if (spawn.x < 0) {
            return;
        }

        commandPool(*this, team) -= unitCost(unitName);
        timer = 0.f;
        // Slow HQ conscription is a comeback valve; barracks remain the only
        // efficient way to mass units, but a raided side is not permanently dead.
        createUnit(team, unitName, spawn.x, spawn.y, selectedLaneForTeam(team));
        logEvent(std::string(team == PLAYER ? "player" : "ai") + " emergency trained unit="
            + std::to_string(unitName));
    };

    trainFromBase(PLAYER, playerEmergencyTrainTimer);
    trainFromBase(AI, aiEmergencyTrainTimer);
}

void Game::cleanupDestroyedBuildings()
{
    for (auto it = buildings.begin(); it != buildings.end(); ) {
        if (it->health > 0) {
            ++it;
            continue;
        }

        const Building destroyed = *it;
        auto& rebuildTimes = destroyed.team == PLAYER ? playerLaneRebuildReady : aiLaneRebuildReady;
        const int destroyedLane = std::clamp(destroyed.laneIndex, 0, lane::Count - 1);
        rebuildTimes[static_cast<std::size_t>(destroyedLane)] = std::max(
            rebuildTimes[static_cast<std::size_t>(destroyedLane)],
            gameTimeSeconds + config::LaneRebuildLockSeconds);
        resetWorkersForBuilding(destroyed.id);
        setTileID(destroyed.point.x, destroyed.point.y, tile::Empty);
        addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 8.f),
                        std::string(buildingName(destroyed.type)) + " down", sf::Color(255, 218, 112), 12);
        applyStructureLossRelief(destroyed);
        const int destroyer = destroyed.team == PLAYER ? AI : PLAYER;
        commandPool(*this, destroyer) = std::min(config::MaxCommand, commandPool(*this, destroyer) + 12);
        if (destroyer == PLAYER) {
            addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 24.f),
                            "Raid +12 CMD", sf::Color(255, 224, 99), 12);
        }
        logEvent(std::string(destroyed.team == PLAYER ? "player" : "ai") + " "
            + buildingName(destroyed.type) + " destroyed id=" + std::to_string(destroyed.id));
        it = buildings.erase(it);
    }
}

void Game::applyStructureLossRelief(const Building& destroyed)
{
    const int team = destroyed.team;
    const int baseCost = buildingCommandCost(destroyed.type);
    if (baseCost <= 0) {
        return;
    }

    int salvage = std::max(8, baseCost * config::StructureSalvagePercent / 100);
    const bool losingLastBarracks = destroyed.type == building::Barracks
        && totalBuildingCount(team, building::Barracks) <= 1;
    if (losingLastBarracks) {
        salvage += config::LastBarracksReliefBonus;
    }
    commandPool(*this, team) = std::min(config::MaxCommand, commandPool(*this, team) + salvage);

    DisMoveableUnit* base = team == PLAYER ? Base_red.get() : Base_blue.get();
    int& reliefCharges = team == PLAYER ? playerReliefCharges : aiReliefCharges;
    const int repair = destroyed.type == building::Barracks
        ? config::EmergencyBarracksRepair
        : config::EmergencyTowerRepair;
    int appliedRepair = 0;
    const bool reliefNeeded = base != nullptr && base->Health > 0
        && (base->Health < config::BaseHealth || losingLastBarracks);
    bool reliefTriggered = false;
    if (reliefNeeded && reliefCharges > 0) {
        const int before = base->Health;
        base->Health = std::min(config::BaseHealth, base->Health + repair);
        appliedRepair = base->Health - before;
        float& shieldTimer = team == PLAYER ? playerBaseShieldTimer : aiBaseShieldTimer;
        shieldTimer = std::max(shieldTimer, config::EmergencyShieldSeconds);
        --reliefCharges;
        reliefTriggered = true;
        base->playFlash(team == PLAYER ? sf::Color(255, 232, 132, 255) : sf::Color(142, 196, 255, 255), 0.35f);
    }

    if (team == PLAYER) {
        const sf::Vector2f textPos(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 26.f);
        addFloatingText(textPos, "Rebuild +" + std::to_string(salvage) + " CMD",
                        sf::Color(255, 232, 132), 12);
        if (reliefTriggered) {
            addFloatingText(sf::Vector2f(Red_baseP.x * SqureSize, Red_baseP.y * SqureSize - 36.f),
                            appliedRepair > 0 ? "HQ Repair +" + std::to_string(appliedRepair) : "HQ Shield",
                            sf::Color(255, 244, 178), 13);
        }
    }

    logEvent(std::string(team == PLAYER ? "player" : "ai")
        + " comeback salvage=+" + std::to_string(salvage)
        + " repair=+" + std::to_string(appliedRepair)
        + " shield=" + std::to_string(static_cast<int>(std::ceil(baseShieldSecondsForTeam(team))))
        + " charges=" + std::to_string(reliefCharges)
        + " after " + buildingName(destroyed.type) + " loss");
}

bool Game::spawnUnit(int team, int name, int x, int y)
{
    if (!canSpawnUnit(team, name) || !canCreateUnitAt(team, name, x, y)) {
        return false;
    }
    commandPool(*this, team) -= unitCost(name);
    return createUnit(team, name, x, y, selectedLaneForTeam(team));
}

bool Game::canCreateUnitAt(int team, int name, int x, int y) const
{
    if (unitCost(name) <= 0 || !hasUnitCapacity(team)) {
        return false;
    }
    if (!isCellWalkableForUnit(x, y) || isCellReservedForSpawn(x, y)) {
        return false;
    }
    return true;
}

bool Game::createUnit(int team, int name, int x, int y, int laneIndex)
{
    if (!canCreateUnitAt(team, name, x, y)) {
        return false;
    }

    std::unique_ptr<MoveableUnit> unit = createMoveableUnit(team, name, x, y, this);
    if (!unit) {
        return false;
    }

    unit->entityId = nextEntityId++;
    unit->laneIndex = std::clamp(laneIndex, 0, lane::Count - 1);
    unit->deploymentReadyTime = (std::floor(gameTimeSeconds / config::ArmyWaveIntervalSeconds) + 1.f)
        * config::ArmyWaveIntervalSeconds;
    unit->scaleMaxHealth(unitHealthMultiplier(team, name));
    if (team == PLAYER) {
        myunits.push_back(std::move(unit));
    }
    else {
        enemys.push_back(std::move(unit));
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " spawned unit=" + std::to_string(name)
        + " lane=" + laneName(laneIndex) + " at " + std::to_string(x) + "," + std::to_string(y));
    return true;
}

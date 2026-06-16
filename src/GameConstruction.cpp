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

void Game::resetWorkersForBuilding(int buildingId)
{
    for (auto& worker : workers) {
        if (worker.buildingId != buildingId) {
            continue;
        }
        worker.state = worker::Idle;
        worker.buildingId = 0;
        worker.path.clear();
        worker.pendingPathRequest = 0;
    }
}

int Game::workerCount(int team) const
{
    return static_cast<int>(std::count_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team;
    }));
}

int Game::assignedWorkerCount(int buildingId) const
{
    return static_cast<int>(std::count_if(workers.begin(), workers.end(), [buildingId](const Worker& worker) {
        return worker.buildingId == buildingId && worker.state != worker::Idle;
    }));
}

Worker* Game::findIdleWorker(int team)
{
    const auto it = std::find_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team && worker.state == worker::Idle;
    });
    return it == workers.end() ? nullptr : &(*it);
}

Worker* Game::findAvailableWorker(int team)
{
    if (Worker* worker = findIdleWorker(team)) {
        return worker;
    }
    return nullptr;
}

void Game::assignWorkerToBuilding(Worker& worker, Building& building)
{
    worker.state = worker::MovingToBuild;
    worker.buildingId = building.id;
    worker.target = findBuildStandPoint(building);
    worker.path.clear();
    worker.pendingPathRequest = 0;
    worker.pathTimer = realtime::WorkerPathRefreshSeconds;
    worker.moveTimer = 0.f;
}

void Game::createWorker(int team, Point point)
{
    Worker worker;
    worker.id = nextEntityId++;
    worker.team = team;
    worker.point = point;
    worker.target = point;
    workers.push_back(worker);
}

void Game::createStartingWorkers()
{
    for (int i = 0; i < realtime::StartingWorkers; ++i) {
        createWorker(PLAYER, workerSpawnPoint(PLAYER));
        createWorker(AI, workerSpawnPoint(AI));
    }
}

bool Game::requestBuildBarracks(int team, Point point)
{
    if (totalBuildingCount(team, building::Barracks) >= buildingCap(team, building::Barracks)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tech for more Rax", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::Barracks)
        || commandPool(*this, team) < config::BarracksCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::BarracksCost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::Barracks) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= config::BarracksCost;
    Building building;
    building.id = nextEntityId++;
    building.team = team;
    building.type = building::Barracks;
    building.point = point;
    building.buildSeconds = buildingSeconds(building.type);
    building.maxHealth = buildingMaxHealth(building.type);
    building.health = building.maxHealth;
    buildings.push_back(building);
    setTileID(point.x, point.y, buildingTileId(team, building.type));
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                        "Barracks queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued barracks id=" + std::to_string(building.id));
    return true;
}

bool Game::requestBuildTower(int team, Point point)
{
    if (totalBuildingCount(team, building::DefenseTower) >= buildingCap(team, building::DefenseTower)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tower cap", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::DefenseTower)
        || commandPool(*this, team) < config::TowerCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::TowerCost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::DefenseTower) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= config::TowerCost;
    Building building;
    building.id = nextEntityId++;
    building.team = team;
    building.type = building::DefenseTower;
    building.point = point;
    building.buildSeconds = buildingSeconds(building.type);
    building.maxHealth = buildingMaxHealth(building.type);
    building.health = building.maxHealth;
    buildings.push_back(building);
    setTileID(point.x, point.y, buildingTileId(team, building.type));
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                        "Tower queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued tower id=" + std::to_string(building.id));
    return true;
}

Point Game::findAutoBuildSite(int team, int type, int laneIndex) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const Point anchor = type == building::DefenseTower
        ? laneDefensePoint(team, laneIndex)
        : Point(base.x + 4, base.y + (laneIndex - 1) * 3);

    for (int radius = 1; radius <= 9; ++radius) {
        for (int y = anchor.y - radius; y <= anchor.y + radius; ++y) {
            for (int x = anchor.x - radius; x <= anchor.x + radius; ++x) {
                const Point candidate(x, y);
                if (isBuildableCell(x, y) && isBuildSiteInInfluence(team, candidate, type)) {
                    return candidate;
                }
            }
        }
    }
    return findBuildableNear(base, 12);
}

bool Game::requestAutoBuildBarracks(int team)
{
    const Point site = findAutoBuildSite(team, building::Barracks, selectedLaneForTeam(team));
    if (site.x < 0) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildBarracksY) - 18.f),
                            "No build site", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    return requestBuildBarracks(team, site);
}

bool Game::requestAutoBuildTower(int team)
{
    const Point site = findAutoBuildSite(team, building::DefenseTower, selectedLaneForTeam(team));
    if (site.x < 0) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildTowerY) - 18.f),
                            "No build site", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    return requestBuildTower(team, site);
}

void Game::assignWorkers()
{
    // Worker routing is demand based: visible drones build nearby structures;
    // economy upgrades simply add more drones around the base.
    for (auto& building : buildings) {
        if (building.complete) {
            continue;
        }
        if (assignedWorkerCount(building.id) > 0) {
            continue;
        }

        if (Worker* worker = findAvailableWorker(building.team)) {
            assignWorkerToBuilding(*worker, building);
        }
    }
}

void Game::updateWorkerTravel(Worker& worker, Point target, float dt)
{
    worker.target = target;
    worker.pathTimer += dt;
    if ((worker.pathTimer >= realtime::WorkerPathRefreshSeconds || worker.path.empty()) && worker.pendingPathRequest == 0) {
        worker.pathTimer = 0.f;
        requestPathForWorker(worker, target);
    }

    worker.moveTimer += dt;
    if (worker.moveTimer < realtime::WorkerStepSeconds || worker.path.empty()) {
        return;
    }

    worker.moveTimer -= realtime::WorkerStepSeconds;
    const Point next = worker.path.front();
    worker.path.pop_front();
    if (isCellWalkableForUnit(next.x, next.y)) {
        worker.point = next;
    }
    else {
        worker.path.clear();
        worker.pathTimer = realtime::WorkerPathRefreshSeconds;
    }
}

void Game::updateWorkers(float dt)
{
    for (auto& worker : workers) {
        if (worker.state == worker::Idle) {
            continue;
        }

        Building* building = findBuildingById(worker.buildingId);
        if (building == nullptr) {
            worker.state = worker::Idle;
            worker.buildingId = 0;
            worker.path.clear();
            worker.pendingPathRequest = 0;
            continue;
        }

        if (building->complete) {
            worker.state = worker::Idle;
            worker.buildingId = 0;
            worker.path.clear();
            worker.pendingPathRequest = 0;
            continue;
        }

        const bool nearBuildSite = nearPoint(worker.point, building->point, 1);
        if (nearBuildSite) {
            worker.state = worker::Building;
            building->buildProgress = std::min(building->buildSeconds, building->buildProgress + dt);
            if (building->buildProgress >= building->buildSeconds) {
                building->complete = true;
                worker.path.clear();
                worker.pendingPathRequest = 0;
                worker.state = worker::Idle;
                worker.buildingId = 0;
                const sf::Vector2f pos(building->point.x * SqureSize, building->point.y * SqureSize - 10.f);
                addFloatingText(pos, building->type == building::DefenseTower ? "Tower ready" : "Barracks ready",
                                building->team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 12);
                logEvent(std::string(building->team == PLAYER ? "player" : "ai") + " completed " + buildingName(building->type)
                    + " id=" + std::to_string(building->id));
            }
            continue;
        }

        worker.state = worker::MovingToBuild;
        updateWorkerTravel(worker, findBuildStandPoint(*building), dt);
    }
}

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
    const int repair = destroyed.type == building::Barracks
        ? config::EmergencyBarracksRepair
        : config::EmergencyTowerRepair;
    int appliedRepair = 0;
    if (base != nullptr && base->Health > 0) {
        const int before = base->Health;
        base->Health = std::min(4000, base->Health + repair);
        appliedRepair = base->Health - before;
        float& shieldTimer = team == PLAYER ? playerBaseShieldTimer : aiBaseShieldTimer;
        shieldTimer = std::max(shieldTimer, config::EmergencyShieldSeconds);
        base->playFlash(team == PLAYER ? sf::Color(255, 232, 132, 255) : sf::Color(142, 196, 255, 255), 0.35f);
    }

    if (team == PLAYER) {
        const sf::Vector2f textPos(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 26.f);
        addFloatingText(textPos, "Rebuild +" + std::to_string(salvage) + " CMD",
                        sf::Color(255, 232, 132), 12);
        if (appliedRepair > 0) {
            addFloatingText(sf::Vector2f(Red_baseP.x * SqureSize, Red_baseP.y * SqureSize - 36.f),
                            "HQ Shield +" + std::to_string(appliedRepair),
                            sf::Color(255, 244, 178), 13);
        }
    }

    logEvent(std::string(team == PLAYER ? "player" : "ai")
        + " comeback salvage=+" + std::to_string(salvage)
        + " repair=+" + std::to_string(appliedRepair)
        + " shield=" + std::to_string(static_cast<int>(std::ceil(baseShieldSecondsForTeam(team))))
        + " after " + buildingName(destroyed.type) + " loss");
}

bool Game::spawnUnit(int team, int name, int x, int y)
{
    if (!spendCommand(team, name)) {
        return false;
    }
    return createUnit(team, name, x, y, selectedLaneForTeam(team));
}

bool Game::createUnit(int team, int name, int x, int y, int laneIndex)
{
    if (!hasUnitCapacity(team)) {
        return false;
    }
    std::unique_ptr<MoveableUnit> unit;
    switch (name)
    {
    case UName::SHOOTER:
        unit = make_unique<Shooter>(team, x, y, this);
        break;
    case UName::INFANTARY:
        unit = make_unique<Infantry>(team, x, y, this);
        break;
    case UName::CAVALRY:
        unit = make_unique<Cavalry>(team, x, y, this);
        break;
    case UName::SIEGE:
        unit = make_unique<Siege>(team, x, y, this);
        break;
    case UName::GUARDIAN:
        unit = make_unique<Guardian>(team, x, y, this);
        break;
    default:
        return false;
    }

    unit->entityId = nextEntityId++;
    unit->laneIndex = std::clamp(laneIndex, 0, lane::Count - 1);
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

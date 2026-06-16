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

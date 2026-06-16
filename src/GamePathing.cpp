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

MoveableUnit* Game::findMoveableUnitById(int id)
{
    for (auto& unit : myunits) {
        if (unit->entityId == id) {
            return unit.get();
        }
    }
    for (auto& unit : enemys) {
        if (unit->entityId == id) {
            return unit.get();
        }
    }
    return nullptr;
}

void Game::requestPathForUnit(MoveableUnit& unit, Point goal)
{
    if (!isMapCell(unit.x, unit.y) || !isCellWalkableForUnit(goal.x, goal.y)) {
        return;
    }
    if (unit.pendingPathRequest != 0
        && unit.pendingPathGoal.x == goal.x
        && unit.pendingPathGoal.y == goal.y) {
        return;
    }

    PathRequest request;
    request.requestId = nextPathRequestId++;
    request.generation = pathGeneration;
    request.ownerId = unit.entityId;
    request.start = Point(unit.x, unit.y);
    request.goal = goal;
    request.allowDiagonal = false;
    request.maze = maze;
    const auto blockUnits = [&request, this, &unit](const std::list<std::unique_ptr<MoveableUnit>>& units) {
        for (const auto& other : units) {
            if (other->entityId == unit.entityId || other->Health <= 0 || !isMapCell(other->x, other->y)) {
                continue;
            }
            request.maze[other->y][other->x] = 1;
        }
    };
    // Realtime units do not write to tile IDs, so the path snapshot must mark
    // occupied cells explicitly to prevent same-cell stacking.
    blockUnits(myunits);
    blockUnits(enemys);
    request.maze[request.start.y][request.start.x] = 0;
    request.maze[goal.y][goal.x] = 0;
    unit.pendingPathRequest = request.requestId;
    unit.pendingPathGoal = goal;
    pathfinding.submit(std::move(request));
}

void Game::requestPathForWorker(Worker& worker, Point goal)
{
    if (!isMapCell(worker.point.x, worker.point.y) || !isCellWalkableForUnit(goal.x, goal.y)) {
        return;
    }
    if (worker.pendingPathRequest != 0) {
        return;
    }

    PathRequest request;
    request.requestId = nextPathRequestId++;
    request.generation = pathGeneration;
    request.ownerId = worker.id;
    request.start = worker.point;
    request.goal = goal;
    request.allowDiagonal = false;
    request.maze = maze;
    request.maze[request.start.y][request.start.x] = 0;
    request.maze[goal.y][goal.x] = 0;
    worker.pendingPathRequest = request.requestId;
    pathfinding.submit(std::move(request));
}

void Game::applyPathResults()
{
    for (auto& result : pathfinding.collectResults()) {
        if (result.generation != pathGeneration) {
            continue;
        }

        if (MoveableUnit* unit = findMoveableUnitById(result.ownerId)) {
            if (unit->pendingPathRequest == result.requestId) {
                unit->mypath = std::move(result.path);
                unit->pendingPathRequest = 0;
                unit->pendingPathGoal = result.goal;
            }
            continue;
        }

        if (Worker* worker = findWorkerById(result.ownerId)) {
            if (worker->pendingPathRequest == result.requestId) {
                worker->path = std::move(result.path);
                worker->pendingPathRequest = 0;
            }
        }
    }
}

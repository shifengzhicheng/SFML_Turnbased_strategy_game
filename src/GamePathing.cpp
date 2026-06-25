#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    bool samePoint(Point a, Point b)
    {
        return a.x == b.x && a.y == b.y;
    }

    bool isAdjacentCardinalStep(Point a, Point b)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
    }

    bool trimPathToCurrent(std::deque<Point>& path, Point requestStart, Point current)
    {
        while (!path.empty() && samePoint(path.front(), current)) {
            path.pop_front();
        }
        if (samePoint(requestStart, current) || path.empty()) {
            return true;
        }

        const auto currentInPath = std::find_if(path.begin(), path.end(), [current](Point point) {
            return samePoint(point, current);
        });
        if (currentInPath != path.end()) {
            path.erase(path.begin(), std::next(currentInPath));
            return true;
        }

        // The unit moved while the worker thread was solving. If the returned
        // path no longer connects to the current tile, discard it instead of
        // letting the unit snap backward or teleport to an old step.
        return isAdjacentCardinalStep(current, path.front());
    }
}

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
    // Unit bodies are deliberately not copied into the maze. The path worker
    // should solve only stable terrain/building blockers; other soldiers move
    // concurrently and may share cells for smoother large-army flow.
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
                unit->pendingPathRequest = 0;
                unit->pendingPathGoal = result.goal;
                Point current(unit->x, unit->y);
                if (trimPathToCurrent(result.path, result.start, current)) {
                    unit->mypath = std::move(result.path);
                }
                else {
                    unit->mypath.clear();
                    unit->realtimePathTimer = realtime::PathRefreshSeconds;
                }
            }
            continue;
        }

        if (Worker* worker = findWorkerById(result.ownerId)) {
            if (worker->pendingPathRequest == result.requestId) {
                worker->pendingPathRequest = 0;
                if (trimPathToCurrent(result.path, result.start, worker->point)) {
                    worker->path = std::move(result.path);
                }
                else {
                    worker->path.clear();
                    worker->pathTimer = realtime::WorkerPathRefreshSeconds;
                }
            }
        }
    }
}

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

void Game::clearSelection()
{
    // Keep selection exclusive so UI hints, build buttons, and path previews
    // cannot be overwritten by multiple selected actors in one frame.
    drawPaths.clear();
    if (Base_red) {
        Base_red->setState(UState::UNITNORMAL);
    }
    if (Base_blue) {
        Base_blue->setState(UState::UNITNORMAL);
    }
    for (auto& unit : myunits) {
        unit->setState(UState::UNITNORMAL);
    }
    for (auto& unit : enemys) {
        unit->setState(UState::UNITNORMAL);
    }
}

void Game::selectOnly(Unit* unit)
{
    const bool wasSelected = unit != nullptr && unit->UnitState == UState::UNITCLICK;
    clearSelection();
    if (unit != nullptr && !wasSelected) {
        unit->setState(UState::UNITCLICK);
    }
}

bool Game::isBlockingTile(tile::ID id) const
{
    return id == tile::Mount
        || id == tile::River
        || id == tile::Tree
        || (!realtimeMode && id == tile::Unit)
        || id == tile::Red_Base
        || id == tile::Blue_Base
        || id == tile::Player_Barracks
        || id == tile::Enemy_Barracks
        || id == tile::Player_Tower
        || id == tile::Enemy_Tower;
}

bool Game::isMapCell(int x, int y) const
{
    return y >= 0
        && y < static_cast<int>(maze.size())
        && x >= 0
        && x < static_cast<int>(maze[y].size());
}

bool Game::isRealtimeMode() const
{
    return realtimeMode;
}

bool Game::isCellWalkableForUnit(int x, int y) const
{
    if (!isMapCell(x, y)) {
        return false;
    }
    const tile::ID id = tiles[y * horizontalTiles + x].getID();
    return id == tile::Empty || id == tile::Path || id == tile::Choosen || id == tile::Unit || id == tile::Resource;
}

bool Game::isCellReservedForSpawn(int x, int y) const
{
    const auto matchesCell = [x, y](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->x == x && unit->y == y;
    };
    return std::any_of(myunits.begin(), myunits.end(), matchesCell)
        || std::any_of(enemys.begin(), enemys.end(), matchesCell);
}

bool Game::isCellOccupiedByUnit(int x, int y, int ignoredEntityId) const
{
    const auto matchesCell = [x, y, ignoredEntityId](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->entityId != ignoredEntityId && unit->Health > 0 && unit->x == x && unit->y == y;
    };
    return std::any_of(myunits.begin(), myunits.end(), matchesCell)
        || std::any_of(enemys.begin(), enemys.end(), matchesCell);
}

bool Game::canUnitStepInto(const MoveableUnit& unit, Point point) const
{
    return isCellWalkableForUnit(point.x, point.y)
        && !isCellOccupiedByUnit(point.x, point.y, unit.entityId);
}

bool Game::isBuildableCell(int x, int y) const
{
    if (!isMapCell(x, y)) {
        return false;
    }
    const bool workerOnCell = std::any_of(workers.begin(), workers.end(), [x, y](const Worker& worker) {
        return worker.point.x == x && worker.point.y == y;
    });
    return tiles[y * horizontalTiles + x].getID() == tile::Empty && !isCellReservedForSpawn(x, y) && !workerOnCell;
}

bool Game::isBuildSiteInInfluence(int team, Point point, int type) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const int radiusSquared = config::BuildInfluenceRadius * config::BuildInfluenceRadius;
    if (distanceSquared(point, base) <= radiusSquared) {
        return true;
    }

    for (const auto& building : buildings) {
        if (building.team != team || !building.complete) {
            continue;
        }
        if (distanceSquared(point, building.point) <= radiusSquared) {
            return true;
        }
    }
    return false;
}

void Game::setTileID(int x, int y, tile::ID id)
{
    const auto index = y * horizontalTiles + x;
    if (!isMapCell(x, y) || index < 0 || index >= static_cast<int>(tiles.size())) {
        return;
    }
    // Tiles are the render source and maze is the pathfinding source; update
    // them together to avoid one-frame desyncs.
    tiles[index].setID(id);
    maze[y][x] = isBlockingTile(id) ? 1 : 0;
}

void Game::syncMazeFromTiles()
{
    // Rebuild pathing from visible map state before input/AI mutates gameplay.
    for (const auto& tile : tiles) {
        const auto p = tile.getIndex();
        if (isMapCell(p.x, p.y)) {
            maze[p.y][p.x] = isBlockingTile(tile.getID()) ? 1 : 0;
        }
    }
}

Building* Game::findBuildingById(int id)
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [id](const Building& building) {
        return building.id == id;
    });
    return it == buildings.end() ? nullptr : &(*it);
}

const Building* Game::findBuildingById(int id) const
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [id](const Building& building) {
        return building.id == id;
    });
    return it == buildings.end() ? nullptr : &(*it);
}

Worker* Game::findWorkerById(int id)
{
    const auto it = std::find_if(workers.begin(), workers.end(), [id](const Worker& worker) {
        return worker.id == id;
    });
    return it == workers.end() ? nullptr : &(*it);
}

Point Game::workerSpawnPoint(int team) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const auto occupiedByWorker = [this](Point point) {
        return std::any_of(workers.begin(), workers.end(), [point](const Worker& worker) {
            return worker.point.x == point.x && worker.point.y == point.y;
        });
    };
    const Point candidates[] = {
        Point(base.x - 1, base.y),
        Point(base.x, base.y - 1),
        Point(base.x + 2, base.y + 1),
        Point(base.x + 1, base.y + 2),
        Point(base.x - 1, base.y + 1),
        Point(base.x + 1, base.y - 1)
    };
    for (const auto& candidate : candidates) {
        if (isCellWalkableForUnit(candidate.x, candidate.y) && !occupiedByWorker(candidate) && !isCellReservedForSpawn(candidate.x, candidate.y)) {
            return candidate;
        }
    }
    for (int radius = 2; radius <= 4; ++radius) {
        for (int y = base.y - radius; y <= base.y + radius; ++y) {
            for (int x = base.x - radius; x <= base.x + radius; ++x) {
                const Point candidate(x, y);
                if (isCellWalkableForUnit(x, y) && !occupiedByWorker(candidate) && !isCellReservedForSpawn(x, y)) {
                    return candidate;
                }
            }
        }
    }
    return base;
}

Point Game::findBuildStandPoint(const Building& building) const
{
    const Point offsets[] = {
        Point(-1, 0), Point(1, 0), Point(0, -1), Point(0, 1),
        Point(-1, -1), Point(1, -1), Point(-1, 1), Point(1, 1)
    };
    for (const auto& offset : offsets) {
        const Point candidate(building.point.x + offset.x, building.point.y + offset.y);
        if (isCellWalkableForUnit(candidate.x, candidate.y)) {
            return candidate;
        }
    }
    return building.point;
}

Point Game::findAttackStandPoint(const MoveableUnit& unit, const Building& building) const
{
    const int range = std::max(1, unit.myAttackRange());
    const int rangeSquared = range * range;
    const Point current(unit.x, unit.y);
    Point best(-1, -1);
    int bestScore = std::numeric_limits<int>::max();

    for (int y = building.point.y - range; y <= building.point.y + range; ++y) {
        for (int x = building.point.x - range; x <= building.point.x + range; ++x) {
            const Point candidate(x, y);
            if (!isCellWalkableForUnit(x, y) || isCellOccupiedByUnit(x, y, unit.entityId)) {
                continue;
            }
            const int toTarget = distanceSquared(candidate, building.point);
            if (toTarget > rangeSquared) {
                continue;
            }

            int score = distanceSquared(current, candidate);
            if (building.type == building::DefenseTower && unit.unitName == UName::SIEGE) {
                const int towerRange = defenseTowerRange(building.team);
                const int towerRangeSquared = towerRange * towerRange;
                // Siege engines should naturally set up just outside tower
                // range, forcing the defender to send units instead of turtling.
                score += toTarget > towerRangeSquared ? -1200 - toTarget : 1800;
            }
            if (score < bestScore) {
                bestScore = score;
                best = candidate;
            }
        }
    }

    return best.x >= 0 ? best : findBuildStandPoint(building);
}

Point Game::findBuildableNear(Point anchor, int radius) const
{
    for (int r = 1; r <= radius; ++r) {
        for (int y = anchor.y - r; y <= anchor.y + r; ++y) {
            for (int x = anchor.x - r; x <= anchor.x + r; ++x) {
                if (isBuildableCell(x, y)) {
                    return Point(x, y);
                }
            }
        }
    }
    return Point(-1, -1);
}

Point Game::findSpawnPointAround(Point anchor) const
{
    for (int r = 1; r <= 3; ++r) {
        for (int y = anchor.y - r; y <= anchor.y + r; ++y) {
            for (int x = anchor.x - r; x <= anchor.x + r; ++x) {
                if (isCellWalkableForUnit(x, y) && !isCellReservedForSpawn(x, y)) {
                    return Point(x, y);
                }
            }
        }
    }
    return Point(-1, -1);
}

int Game::completedBuildingCount(int team, int type) const
{
    return static_cast<int>(std::count_if(buildings.begin(), buildings.end(), [team, type](const Building& building) {
        return building.team == team && building.type == type && building.complete;
    }));
}

int Game::totalBuildingCount(int team, int type) const
{
    return static_cast<int>(std::count_if(buildings.begin(), buildings.end(), [team, type](const Building& building) {
        return building.team == team && building.type == type;
    }));
}

int Game::selectedLaneForTeam(int team) const
{
    return team == PLAYER ? playerSelectedLane : aiSelectedLane;
}

Point Game::laneWaypoint(int team, int laneIndex, int stage) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const int laneY[] = {
        std::max(4, mapH / 4),
        mapH / 2,
        std::min(mapH - 5, mapH * 3 / 4)
    };
    const int safeLane = std::clamp(laneIndex, 0, lane::Count - 1);
    const int playerX[] = {mapW / 4, mapW / 2, mapW * 3 / 4};
    const int aiX[] = {mapW * 3 / 4, mapW / 2, mapW / 4};
    const int safeStage = std::clamp(stage, 0, 2);
    return Point(team == PLAYER ? playerX[safeStage] : aiX[safeStage], laneY[safeLane]);
}

Point Game::laneDefensePoint(int team, int laneIndex) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const int laneY[] = {
        std::max(5, mapH / 4),
        mapH / 2,
        std::min(mapH - 6, mapH * 3 / 4)
    };
    const int safeLane = std::clamp(laneIndex, 0, lane::Count - 1);
    return Point(team == PLAYER ? mapW / 5 : mapW * 4 / 5, laneY[safeLane]);
}

int Game::unitsNearPoint(int team, Point point, int radius) const
{
    const auto& units = team == PLAYER ? myunits : enemys;
    const int radiusSquared = radius * radius;
    return static_cast<int>(std::count_if(units.begin(), units.end(), [point, radiusSquared](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->Health > 0 && distanceSquared(Point(unit->x, unit->y), point) <= radiusSquared;
    }));
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

int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}

#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "LaneGeometry.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

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
            if (!isCellWalkableForUnit(x, y)) {
                continue;
            }
            const int toTarget = distanceSquared(candidate, building.point);
            if (toTarget > rangeSquared) {
                continue;
            }

            const int score = distanceSquared(current, candidate);
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
    return lane_geometry::laneWaypoint(mapW, mapH, laneIndex, stage, team == AI);
}

Point Game::laneRallyPoint(int team, int laneIndex, int stage) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    return lane_geometry::laneRallyWaypoint(mapW, mapH, laneIndex, stage, team == AI);
}

Point Game::laneDefensePoint(int team, int laneIndex) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    return lane_geometry::laneWaypoint(mapW, mapH, laneIndex, 0, team == AI);
}

int Game::unitsNearPoint(int team, Point point, int radius) const
{
    const auto& units = team == PLAYER ? myunits : enemys;
    const int radiusSquared = radius * radius;
    return static_cast<int>(std::count_if(units.begin(), units.end(), [point, radiusSquared](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->Health > 0 && distanceSquared(Point(unit->x, unit->y), point) <= radiusSquared;
    }));
}

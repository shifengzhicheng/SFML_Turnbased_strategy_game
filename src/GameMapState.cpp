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

bool Game::isBlockingTile(tile::ID id) const
{
    return id == tile::Mount
        || id == tile::River
        || id == tile::Tree
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

bool Game::canUnitStepInto(const MoveableUnit& unit, Point point) const
{
    (void)unit;
    // Combat units intentionally ignore each other's footprint. This keeps
    // concurrent path results from turning short-lived crowds into deadlocks.
    return isCellWalkableForUnit(point.x, point.y);
}

bool Game::hasLineOfSight(Point from, Point to) const
{
    int currentX = from.x;
    int currentY = from.y;
    const int dx = std::abs(to.x - from.x);
    const int dy = std::abs(to.y - from.y);
    const int stepX = from.x < to.x ? 1 : -1;
    const int stepY = from.y < to.y ? 1 : -1;
    int error = dx - dy;

    while (currentX != to.x || currentY != to.y) {
        const int doubledError = error * 2;
        if (doubledError > -dy) {
            error -= dy;
            currentX += stepX;
        }
        if (doubledError < dx) {
            error += dx;
            currentY += stepY;
        }

        if (currentX == to.x && currentY == to.y) {
            break;
        }
        if (!isMapCell(currentX, currentY)) {
            return false;
        }
        const tile::ID id = tiles[currentY * horizontalTiles + currentX].getID();
        if (isBlockingTile(id)) {
            return false;
        }
    }
    return true;
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

int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}

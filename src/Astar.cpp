#include "Astar.h"
#include "Config.h"

#include <cmath>
#include <queue>

namespace
{
    constexpr int height = config::MapHeight;
    constexpr int width = config::MapWidth;
    constexpr int tileSize = config::TileSize;
}

struct OpenEntry
{
    int nodeIndex = -1;
    int f = 0;
    int h = 0;
};

struct LowerScoreFirst
{
    bool operator()(const OpenEntry& lhs, const OpenEntry& rhs) const
    {
        if (lhs.f != rhs.f) {
            return lhs.f > rhs.f;
        }
        return lhs.h > rhs.h;
    }
};

struct Astar::PathNode
{
    Point point;
    int parent = -1;
    int g = 0;
    int h = 0;
    int f = 0;
};

Astar::Astar() :
    maze(makeEmptyMaze())
{
}

Astar::Astar(const std::vector<std::vector<int>>& _maze) :
    maze(_maze)
{
}

std::vector<std::vector<int>> Astar::makeEmptyMaze()
{
    return std::vector<std::vector<int>>(height / tileSize, std::vector<int>(width / tileSize, 0));
}

void Astar::setMaze(const std::vector<std::vector<int>>& _maze)
{
    maze = _maze;
}

const std::vector<Point>& Astar::getSearchPath() const
{
    return searchPath;
}

std::deque<Point> Astar::GetPath(Point startPoint, Point endPoint, bool allowDiagonal)
{
    searchPath.clear();

    if (!isInside(startPoint) || !isInside(endPoint) || isBlocked(endPoint)) {
        return {};
    }

    std::vector<PathNode> nodes;
    std::priority_queue<OpenEntry, std::vector<OpenEntry>, LowerScoreFirst> open;
    std::vector<std::vector<int>> bestIndex(maze.size(), std::vector<int>(maze.front().size(), -1));
    std::vector<std::vector<bool>> closed(maze.size(), std::vector<bool>(maze.front().size(), false));

    // `nodes` owns the search graph; the priority queue stores only indices so
    // path reconstruction is stable and no per-node dynamic allocation leaks.
    PathNode start;
    start.point = startPoint;
    start.h = calcH(startPoint, endPoint);
    start.f = calcF(start.g, start.h);
    nodes.push_back(start);
    open.push(OpenEntry{0, start.f, start.h});
    bestIndex[startPoint.y][startPoint.x] = 0;

    while (!open.empty()) {
        const int currentIndex = open.top().nodeIndex;
        open.pop();

        const Point currentPoint = nodes[currentIndex].point;
        if (closed[currentPoint.y][currentPoint.x]) {
            continue;
        }

        closed[currentPoint.y][currentPoint.x] = true;
        searchPath.push_back(currentPoint);

        if (currentPoint.x == endPoint.x && currentPoint.y == endPoint.y) {
            return reconstructPath(nodes, currentIndex);
        }

        for (const auto& nextPoint : getSurroundPoints(currentPoint, allowDiagonal)) {
            if (closed[nextPoint.y][nextPoint.x]) {
                continue;
            }

            const int tentativeG = nodes[currentIndex].g + calcG(currentPoint, nextPoint);
            int& existingIndex = bestIndex[nextPoint.y][nextPoint.x];
            if (existingIndex == -1) {
                PathNode node;
                node.point = nextPoint;
                node.parent = currentIndex;
                node.g = tentativeG;
                node.h = calcH(nextPoint, endPoint);
                node.f = calcF(node.g, node.h);
                nodes.push_back(node);
                existingIndex = static_cast<int>(nodes.size() - 1);
                open.push(OpenEntry{existingIndex, node.f, node.h});
            }
            else if (tentativeG < nodes[existingIndex].g) {
                nodes[existingIndex].parent = currentIndex;
                nodes[existingIndex].g = tentativeG;
                nodes[existingIndex].f = calcF(nodes[existingIndex].g, nodes[existingIndex].h);
                open.push(OpenEntry{existingIndex, nodes[existingIndex].f, nodes[existingIndex].h});
            }
        }
    }

    return {};
}

bool Astar::isInside(Point point) const
{
    return !maze.empty()
        && !maze.front().empty()
        && point.y >= 0
        && point.y < static_cast<int>(maze.size())
        && point.x >= 0
        && point.x < static_cast<int>(maze.front().size());
}

bool Astar::isBlocked(Point point) const
{
    return maze[point.y][point.x] == 1;
}

bool Astar::isCanReach(Point from, Point target, bool allowDiagonal) const
{
    if (!isInside(target) || isBlocked(target) || (target.x == from.x && target.y == from.y)) {
        return false;
    }

    const int dx = std::abs(from.x - target.x);
    const int dy = std::abs(from.y - target.y);
    if (dx + dy == 1) {
        return true;
    }

    if (!allowDiagonal || dx != 1 || dy != 1) {
        return false;
    }

    return !isBlocked(Point(target.x, from.y)) && !isBlocked(Point(from.x, target.y));
}

std::vector<Point> Astar::getSurroundPoints(Point point, bool allowDiagonal) const
{
    std::vector<Point> surroundPoints;
    surroundPoints.reserve(allowDiagonal ? 8 : 4);

    for (int y = point.y - 1; y <= point.y + 1; ++y) {
        for (int x = point.x - 1; x <= point.x + 1; ++x) {
            Point target(x, y);
            if (isCanReach(point, target, allowDiagonal)) {
                surroundPoints.push_back(target);
            }
        }
    }

    return surroundPoints;
}

int Astar::calcG(Point from, Point target) const
{
    return (std::abs(from.x - target.x) + std::abs(from.y - target.y) == 1)
        ? config::PathStraightCost
        : config::PathDiagonalCost;
}

int Astar::calcH(Point point, Point end) const
{
    const auto dx = static_cast<double>(end.x - point.x);
    const auto dy = static_cast<double>(end.y - point.y);
    return static_cast<int>(std::sqrt(dx * dx + dy * dy) * config::PathStraightCost);
}

int Astar::calcF(int g, int h) const
{
    return g + h;
}

std::deque<Point> Astar::reconstructPath(const std::vector<PathNode>& nodes, int endIndex) const
{
    std::deque<Point> path;
    for (int index = endIndex; index != -1; index = nodes[index].parent) {
        path.push_front(nodes[index].point);
    }
    return path;
}

#include "Map.h"

#include "Config.h"
#include "LaneGeometry.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool isOpen(int value)
    {
        return value == 0;
    }

    bool hasPath(const std::vector<std::vector<int>>& map, int sx, int sy, int tx, int ty)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::vector<std::vector<bool>> seen(lines, std::vector<bool>(cols, false));
        std::queue<std::pair<int, int>> q;
        q.push({sx, sy});
        seen[sy][sx] = true;

        constexpr int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!q.empty()) {
            const auto [x, y] = q.front();
            q.pop();
            if (x == tx && y == ty) {
                return true;
            }
            for (const auto& dir : dirs) {
                const int nx = x + dir[0];
                const int ny = y + dir[1];
                if (nx < 0 || ny < 0 || nx >= cols || ny >= lines || seen[ny][nx] || !isOpen(map[ny][nx])) {
                    continue;
                }
                seen[ny][nx] = true;
                q.push({nx, ny});
            }
        }
        return false;
    }

    int countInArea(const std::vector<std::vector<int>>& map, int cx, int cy, int radius, int value)
    {
        int count = 0;
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                if (y >= 0 && y < static_cast<int>(map.size())
                    && x >= 0 && x < static_cast<int>(map.front().size())
                    && map[y][x] == value) {
                    ++count;
                }
            }
        }
        return count;
    }

    int countValue(const std::vector<std::vector<int>>& map, int value)
    {
        int count = 0;
        for (const auto& row : map) {
            for (int cell : row) {
                if (cell == value) {
                    ++count;
                }
            }
        }
        return count;
    }

    int countBlockersInHalf(const std::vector<std::vector<int>>& map, bool leftHalf)
    {
        const int cols = static_cast<int>(map.front().size());
        int count = 0;
        for (int y = 1; y < static_cast<int>(map.size()) - 1; ++y) {
            const int begin = leftHalf ? 1 : cols / 2;
            const int end = leftHalf ? cols / 2 : cols - 1;
            for (int x = begin; x < end; ++x) {
                if (map[y][x] != 0) {
                    ++count;
                }
            }
        }
        return count;
    }

    int countLaneDividerBlockers(const std::vector<std::vector<int>>& map)
    {
        const int cols = static_cast<int>(map.front().size());
        const int lines = static_cast<int>(map.size());
        int count = 0;
        for (int band = 0; band < 2; ++band) {
            const int upperLane = band == 0 ? lane::Top : lane::Mid;
            const int lowerLane = band == 0 ? lane::Mid : lane::Bot;
            for (int x = 5; x < cols - 5; ++x) {
                const int upperY = static_cast<int>(std::round(lane_geometry::laneYAtX(cols, lines, upperLane, x)));
                const int lowerY = static_cast<int>(std::round(lane_geometry::laneYAtX(cols, lines, lowerLane, x)));
                const int beginY = std::min(upperY, lowerY) + 1;
                const int endY = std::max(upperY, lowerY) - 1;
                for (int y = beginY; y <= endY; ++y) {
                    if (y > 0 && y < lines - 1 && map[y][x] != 0) {
                        ++count;
                    }
                }
            }
        }
        return count;
    }

    int failMapCheck(const std::string& reason, const std::vector<std::vector<int>>& map, int cols, int lines)
    {
        std::cerr << "map_tests: " << reason
                  << " terrain mount=" << countValue(map, 1)
                  << " river=" << countValue(map, 2)
                  << " forest=" << countValue(map, 3)
                  << " half=" << countBlockersInHalf(map, true)
                  << "/" << countBlockersInHalf(map, false)
                  << " divider=" << countLaneDividerBlockers(map)
                  << " baseOpen=" << countInArea(map, 5, lines / 2, 3, 0)
                  << "/" << countInArea(map, cols - 7, lines / 2, 3, 0)
                  << " centerOpen=" << countInArea(map, cols / 2, lines / 2, 2, 0)
                  << '\n';
        return 1;
    }
}

int main()
{
    constexpr int cols = config::MapTilesX;
    constexpr int lines = config::MapTilesY;
    mapgenerator generator;

    for (int i = 0; i < 20; ++i) {
        std::vector<std::vector<int>> map;
        const unsigned int seed = 20260710u + static_cast<unsigned int>(i);
        generator.gmap(map, cols, lines, seed);

        if (static_cast<int>(map.size()) != lines || map.empty()
            || static_cast<int>(map.front().size()) != cols) {
            std::cerr << "map_tests: map dimensions changed\n";
            return 1;
        }

        std::vector<std::vector<int>> repeated;
        generator.gmap(repeated, cols, lines, seed);
        if (repeated != map) {
            return failMapCheck("same seed should reproduce the same map", map, cols, lines);
        }

        for (int x = 0; x < cols; ++x) {
            if (map.front()[x] != 1 || map.back()[x] != 1) {
                return failMapCheck("top or bottom border opened", map, cols, lines);
            }
        }
        for (int y = 0; y < lines; ++y) {
            if (map[y].front() != 1 || map[y].back() != 1) {
                return failMapCheck("left or right border opened", map, cols, lines);
            }
        }

        if (countInArea(map, 5, lines / 2, 3, 0) <= 35) {
            return failMapCheck("red base plaza too cramped", map, cols, lines);
        }
        if (countInArea(map, cols - 7, lines / 2, 3, 0) <= 35) {
            return failMapCheck("blue base plaza too cramped", map, cols, lines);
        }
        const auto midRoute = lane_geometry::laneRoute(cols, lines, lane::Mid);
        const auto topRoute = lane_geometry::laneRoute(cols, lines, lane::Top);
        const auto botRoute = lane_geometry::laneRoute(cols, lines, lane::Bot);

        if (!hasPath(map, midRoute.front().x, midRoute.front().y, midRoute.back().x, midRoute.back().y)) {
            return failMapCheck("main lane disconnected", map, cols, lines);
        }
        if (!hasPath(map, midRoute.front().x, midRoute.front().y, midRoute[2].x, midRoute[2].y)
            || !hasPath(map, midRoute.back().x, midRoute.back().y, midRoute[2].x, midRoute[2].y)
            || !hasPath(map, topRoute.front().x, topRoute.front().y, topRoute[2].x, topRoute[2].y)
            || !hasPath(map, botRoute.front().x, botRoute.front().y, botRoute[2].x, botRoute[2].y)) {
            return failMapCheck("lane network disconnected", map, cols, lines);
        }
        if (countInArea(map, cols / 2, lines / 2, 2, 0) <= 20) {
            return failMapCheck("center plaza too cramped", map, cols, lines);
        }
        const int area = cols * lines;
        if (countValue(map, 1) < area / 25 || countValue(map, 2) < area / 110 || countValue(map, 3) < area / 22) {
            return failMapCheck("terrain lacks visual variety", map, cols, lines);
        }
        if (countLaneDividerBlockers(map) < 16) {
            return failMapCheck("lane divider blockers too sparse", map, cols, lines);
        }
        if (std::abs(countBlockersInHalf(map, true) - countBlockersInHalf(map, false)) > 18) {
            return failMapCheck("terrain halves are unfair", map, cols, lines);
        }
    }

    return 0;
}

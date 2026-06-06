#include "Map.h"

#include <cassert>
#include <queue>
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
}

int main()
{
    constexpr int cols = 56;
    constexpr int lines = 36;
    mapgenerator generator;

    for (int i = 0; i < 20; ++i) {
        std::vector<std::vector<int>> map;
        generator.gmap(map, cols, lines);

        assert(static_cast<int>(map.size()) == lines);
        assert(static_cast<int>(map.front().size()) == cols);

        for (int x = 0; x < cols; ++x) {
            assert(map.front()[x] == 1);
            assert(map.back()[x] == 1);
        }
        for (int y = 0; y < lines; ++y) {
            assert(map[y].front() == 1);
            assert(map[y].back() == 1);
        }

        if (countInArea(map, 5, 5, 3, 0) <= 35) {
            return 1;
        }
        if (countInArea(map, cols - 7, lines - 7, 3, 0) <= 35) {
            return 1;
        }
        if (!hasPath(map, 5, 5, cols - 7, lines - 7)) {
            return 1;
        }
        if (!hasPath(map, 5, 5, cols / 2, lines / 2)
            || !hasPath(map, cols - 7, lines - 7, cols / 2, lines / 2)) {
            return 1;
        }
        if (countInArea(map, cols / 2, lines / 2, 2, 0) <= 20) {
            return 1;
        }
    }

    return 0;
}

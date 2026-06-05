#include "Map.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <random>
#include <vector>

namespace
{
    struct GridPoint
    {
        int x = 0;
        int y = 0;
    };

    bool inside(int x, int y, int cols, int lines)
    {
        return x >= 0 && x < cols && y >= 0 && y < lines;
    }

    void setIfInside(std::vector<std::vector<int>>& map, int x, int y, int value)
    {
        if (inside(x, y, static_cast<int>(map.front().size()), static_cast<int>(map.size()))) {
            map[y][x] = value;
        }
    }

    void clearDisc(std::vector<std::vector<int>>& map, int cx, int cy, int radius)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                if (!inside(x, y, cols, lines)) {
                    continue;
                }
                const int dx = x - cx;
                const int dy = y - cy;
                if (dx * dx + dy * dy <= radius * radius) {
                    map[y][x] = 0;
                }
            }
        }
    }

    void markDisc(std::vector<std::vector<bool>>& mask, int cx, int cy, int radius)
    {
        const int lines = static_cast<int>(mask.size());
        const int cols = static_cast<int>(mask.front().size());
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                if (!inside(x, y, cols, lines)) {
                    continue;
                }
                const int dx = x - cx;
                const int dy = y - cy;
                if (dx * dx + dy * dy <= radius * radius) {
                    mask[y][x] = true;
                }
            }
        }
    }

    bool nearMask(const std::vector<std::vector<bool>>& mask, int x, int y, int radius)
    {
        const int lines = static_cast<int>(mask.size());
        const int cols = static_cast<int>(mask.front().size());
        for (int yy = y - radius; yy <= y + radius; ++yy) {
            for (int xx = x - radius; xx <= x + radius; ++xx) {
                if (inside(xx, yy, cols, lines) && mask[yy][xx]) {
                    return true;
                }
            }
        }
        return false;
    }

    bool nearBase(GridPoint p, GridPoint red, GridPoint blue, int radius)
    {
        const auto closeTo = [radius](GridPoint a, GridPoint b) {
            const int dx = a.x - b.x;
            const int dy = a.y - b.y;
            return dx * dx + dy * dy <= radius * radius;
        };
        return closeTo(p, red) || closeTo(p, blue);
    }

    std::vector<GridPoint> carveMainRoute(std::vector<std::vector<int>>& map,
                                          std::vector<std::vector<bool>>& routeMask,
                                          GridPoint start, GridPoint end,
                                          std::mt19937& rng)
    {
        std::vector<GridPoint> route;
        GridPoint p = start;
        std::uniform_int_distribution<int> jitter(-1, 1);
        std::uniform_int_distribution<int> chance(0, 99);

        route.push_back(p);
        markDisc(routeMask, p.x, p.y, 2);
        clearDisc(map, p.x, p.y, 2);

        int guard = 0;
        while ((p.x != end.x || p.y != end.y) && guard++ < 4096) {
            const int dx = end.x - p.x;
            const int dy = end.y - p.y;
            const bool moveX = std::abs(dx) > std::abs(dy)
                ? chance(rng) < 72
                : chance(rng) < 28;

            if (moveX && dx != 0) {
                p.x += dx > 0 ? 1 : -1;
            }
            else if (dy != 0) {
                p.y += dy > 0 ? 1 : -1;
            }
            else if (dx != 0) {
                p.x += dx > 0 ? 1 : -1;
            }

            if (chance(rng) < 18) {
                if (std::abs(dx) > std::abs(dy)) {
                    p.y += jitter(rng);
                }
                else {
                    p.x += jitter(rng);
                }
            }

            p.x = std::clamp(p.x, 2, static_cast<int>(map.front().size()) - 3);
            p.y = std::clamp(p.y, 2, static_cast<int>(map.size()) - 3);
            route.push_back(p);
            markDisc(routeMask, p.x, p.y, 1);
            clearDisc(map, p.x, p.y, 1);
        }

        return route;
    }

    void placeCluster(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                      GridPoint center, int radius, int value, GridPoint red, GridPoint blue,
                      std::mt19937& rng)
    {
        std::uniform_int_distribution<int> chance(0, 99);
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        for (int y = center.y - radius; y <= center.y + radius; ++y) {
            for (int x = center.x - radius; x <= center.x + radius; ++x) {
                if (!inside(x, y, cols, lines) || map[y][x] != 0) {
                    continue;
                }
                if (nearMask(routeMask, x, y, value == 1 ? 2 : 1) || nearBase(GridPoint{x, y}, red, blue, 7)) {
                    continue;
                }
                const int dx = x - center.x;
                const int dy = y - center.y;
                const int dist2 = dx * dx + dy * dy;
                const int radius2 = radius * radius;
                const int edgePenalty = static_cast<int>(45.f * static_cast<float>(dist2) / static_cast<float>(radius2));
                if (dist2 <= radius2 && chance(rng) > edgePenalty) {
                    map[y][x] = value;
                }
            }
        }
    }

    void placeRiver(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                    GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> drift(-1, 1);
        std::uniform_int_distribution<int> chance(0, 99);
        int y = lines / 2 + drift(rng) * 3;

        for (int x = 1; x < cols - 1; ++x) {
            if (chance(rng) < 35) {
                y = std::clamp(y + drift(rng), 3, lines - 4);
            }
            const bool ford = (x % 13 == 0) || nearMask(routeMask, x, y, 2) || nearBase(GridPoint{x, y}, red, blue, 8);
            if (ford) {
                continue;
            }
            setIfInside(map, x, y, 2);
            if (chance(rng) < 35) {
                setIfInside(map, x, y + 1, 2);
            }
        }
    }

    void addBorder(std::vector<std::vector<int>>& map)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        for (int y = 0; y < lines; ++y) {
            for (int x = 0; x < cols; ++x) {
                if (x == 0 || y == 0 || x == cols - 1 || y == lines - 1) {
                    map[y][x] = 1;
                }
            }
        }
    }
}

void mapgenerator::gmap(std::vector<std::vector<int>>& initMap, int cols, int lines)
{
    if (cols <= 0 || lines <= 0) {
        return;
    }

    initMap.assign(lines, std::vector<int>(cols, 0));
    const GridPoint red{5, 5};
    const GridPoint blue{std::max(5, cols - 7), std::max(5, lines - 7)};

    const auto seed = static_cast<unsigned int>(std::time(nullptr))
        ^ static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&initMap));
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> xDist(2, std::max(2, cols - 3));
    std::uniform_int_distribution<int> yDist(2, std::max(2, lines - 3));
    std::uniform_int_distribution<int> mountainRadius(2, 4);
    std::uniform_int_distribution<int> forestRadius(2, 5);

    std::vector<std::vector<bool>> routeMask(lines, std::vector<bool>(cols, false));
    carveMainRoute(initMap, routeMask, red, blue, rng);

    placeRiver(initMap, routeMask, red, blue, rng);

    const int mountainClusters = std::max(5, cols * lines / 360);
    for (int i = 0; i < mountainClusters; ++i) {
        placeCluster(initMap, routeMask, GridPoint{xDist(rng), yDist(rng)}, mountainRadius(rng), 1, red, blue, rng);
    }

    const int forestClusters = std::max(8, cols * lines / 240);
    for (int i = 0; i < forestClusters; ++i) {
        placeCluster(initMap, routeMask, GridPoint{xDist(rng), yDist(rng)}, forestRadius(rng), 3, red, blue, rng);
    }

    for (int y = 0; y < lines; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (routeMask[y][x]) {
                initMap[y][x] = 0;
            }
        }
    }

    clearDisc(initMap, red.x, red.y, 5);
    clearDisc(initMap, blue.x, blue.y, 5);
    addBorder(initMap);
}

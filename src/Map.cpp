#include "Map.h"

#include "LaneGeometry.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <vector>

namespace
{
    struct GridPoint
    {
        int x = 0;
        int y = 0;
    };

    GridPoint toGrid(Point point)
    {
        return GridPoint{point.x, point.y};
    }

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

    void mirrorTerrainHorizontally(std::vector<std::vector<int>>& map,
                                   const std::vector<std::vector<bool>>& routeMask,
                                   GridPoint red, GridPoint blue)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        for (int y = 1; y < lines - 1; ++y) {
            for (int x = 1; x < cols / 2; ++x) {
                const int mirrorX = cols - 1 - x;
                const GridPoint left{x, y};
                const GridPoint right{mirrorX, y};
                if (routeMask[y][x] || routeMask[y][mirrorX]
                    || nearBase(left, red, blue, 7)
                    || nearBase(right, red, blue, 7)) {
                    continue;
                }
                map[y][mirrorX] = map[y][x];
            }
        }
    }

    void mirrorMaskHorizontally(std::vector<std::vector<bool>>& mask)
    {
        const int lines = static_cast<int>(mask.size());
        const int cols = static_cast<int>(mask.front().size());
        for (int y = 1; y < lines - 1; ++y) {
            for (int x = 1; x < cols / 2; ++x) {
                const int mirrorX = cols - 1 - x;
                const bool safe = mask[y][x] || mask[y][mirrorX];
                mask[y][x] = safe;
                mask[y][mirrorX] = safe;
            }
        }
    }

    std::vector<GridPoint> carveMainRoute(std::vector<std::vector<int>>& map,
                                          std::vector<std::vector<bool>>& routeMask,
                                          GridPoint start, GridPoint end,
                                          int corridorRadius,
                                          std::mt19937& rng)
    {
        std::vector<GridPoint> route;
        GridPoint p = start;
        std::uniform_int_distribution<int> jitter(-1, 1);
        std::uniform_int_distribution<int> chance(0, 99);

        route.push_back(p);
        markDisc(routeMask, p.x, p.y, corridorRadius + 1);
        clearDisc(map, p.x, p.y, corridorRadius + 1);

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
            markDisc(routeMask, p.x, p.y, corridorRadius);
            clearDisc(map, p.x, p.y, corridorRadius);
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

    void placeTributary(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                        GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> drift(-1, 1);
        std::uniform_int_distribution<int> chance(0, 99);
        int x = cols / 2 + drift(rng) * 4;

        for (int y = 2; y < lines - 2; ++y) {
            if (chance(rng) < 42) {
                x = std::clamp(x + drift(rng), 3, cols - 4);
            }
            const bool ford = (y % 11 == 0) || nearMask(routeMask, x, y, 2) || nearBase(GridPoint{x, y}, red, blue, 9);
            if (ford) {
                continue;
            }
            setIfInside(map, x, y, 2);
            if (chance(rng) < 28) {
                setIfInside(map, x + 1, y, 2);
            }
        }
    }

    void placeResourceCover(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                            const std::vector<GridPoint>& plazas, GridPoint red, GridPoint blue,
                            std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> chance(0, 99);

        for (const auto& plaza : plazas) {
            for (int y = plaza.y - 6; y <= plaza.y + 6; ++y) {
                for (int x = plaza.x - 6; x <= plaza.x + 6; ++x) {
                    if (!inside(x, y, cols, lines) || map[y][x] != 0 || routeMask[y][x] || nearBase(GridPoint{x, y}, red, blue, 8)) {
                        continue;
                    }
                    const int dx = x - plaza.x;
                    const int dy = y - plaza.y;
                    const int dist2 = dx * dx + dy * dy;
                    if (dist2 < 16 || dist2 > 36) {
                        continue;
                    }

                    // Broken rings create tactical cover around resources while
                    // preserving clear lanes into the plaza.
                    const bool onCardinalGate = std::abs(dx) <= 1 || std::abs(dy) <= 1;
                    if (!onCardinalGate && chance(rng) < 42) {
                        map[y][x] = chance(rng) < 72 ? 3 : 1;
                    }
                }
            }
        }
    }

    void placeRidgeFingers(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                           GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> chance(0, 99);
        const std::vector<GridPoint> anchors = {
            GridPoint{cols / 4, lines / 3},
            GridPoint{cols * 3 / 4, lines * 2 / 3},
            GridPoint{cols / 3, lines * 3 / 4},
            GridPoint{cols * 2 / 3, lines / 4}
        };

        for (const auto& anchor : anchors) {
            int x = anchor.x;
            int y = anchor.y;
            for (int step = 0; step < 12; ++step) {
                x += step % 2 == 0 ? 1 : 0;
                y += step % 2 == 0 ? 0 : 1;
                if (!inside(x, y, cols, lines) || map[y][x] != 0 || routeMask[y][x] || nearBase(GridPoint{x, y}, red, blue, 8)) {
                    continue;
                }
                if (chance(rng) < 70) {
                    map[y][x] = 1;
                }
                if (inside(x + 1, y, cols, lines) && map[y][x + 1] == 0 && !routeMask[y][x + 1] && chance(rng) < 35) {
                    map[y][x + 1] = 1;
                }
            }
        }
    }

    void placeLaneShoulders(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                            GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> jitter(-1, 1);

        for (int lane = 0; lane < 3; ++lane) {
            for (int x = 8; x < cols / 2 - 2; x += 6) {
                const int side = ((x / 6 + lane) % 2 == 0) ? -1 : 1;
                const int laneY = static_cast<int>(std::round(lane_geometry::laneYAtX(cols, lines, lane, x)));
                const GridPoint center{
                    std::clamp(x + jitter(rng), 3, cols - 4),
                    std::clamp(laneY + side * (4 + lane % 2) + jitter(rng), 3, lines - 4)
                };
                const int value = lane == 1 ? 3 : ((x / 6) % 2 == 0 ? 1 : 3);
                placeCluster(map, routeMask, center, 2, value, red, blue, rng);
            }
        }
    }

    bool placeDividerTile(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                          int x, int y, int value, GridPoint red, GridPoint blue)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        if (!inside(x, y, cols, lines) || map[y][x] != 0) {
            return false;
        }
        if (nearMask(routeMask, x, y, 0) || nearBase(GridPoint{x, y}, red, blue, 8)) {
            return false;
        }
        map[y][x] = value;
        return true;
    }

    void placeDividerPatch(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                           GridPoint center, int value, int slope, bool dense,
                           GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        std::uniform_int_distribution<int> chance(0, 99);
        const GridPoint offsets[] = {
            GridPoint{0, 0},
            GridPoint{1, 0},
            GridPoint{-1, 0},
            GridPoint{0, slope},
            GridPoint{1, slope},
            GridPoint{-1, -slope},
            GridPoint{2, 0},
            GridPoint{2, slope},
        };
        const int required = dense ? 8 : 5;

        for (int i = 0; i < required; ++i) {
            if (!dense && i > 2 && chance(rng) < 25) {
                continue;
            }
            const int patchValue = (i % 3 == 0) ? value : (value == 1 ? 3 : value);
            placeDividerTile(map, routeMask, center.x + offsets[i].x, center.y + offsets[i].y,
                             patchValue, red, blue);
        }
    }

    int findDividerY(const std::vector<std::vector<bool>>& routeMask, int x, int upperY, int lowerY, int preferredY)
    {
        const int beginY = std::min(upperY, lowerY) + 1;
        const int endY = std::max(upperY, lowerY) - 1;
        // Narrow lane gaps may have only one safe row after route carving; find
        // that row instead of letting jitter drop divider tiles onto the road.
        for (int delta = 0; delta <= std::max(1, endY - beginY); ++delta) {
            const int candidates[] = {preferredY - delta, preferredY + delta};
            for (int y : candidates) {
                if (y >= beginY && y <= endY && !nearMask(routeMask, x, y, 0)) {
                    return y;
                }
            }
        }
        return preferredY;
    }

    void placeLaneDividerBelts(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                               GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> jitter(-1, 1);
        std::uniform_int_distribution<int> chance(0, 99);

        // Broken forests/ridges between lanes make TOP/MID/BOT read as three
        // spaces, not one empty field. The gaps are intentional flank windows.
        for (int band = 0; band < 2; ++band) {
            const int upperLane = band == 0 ? lane::Top : lane::Mid;
            const int lowerLane = band == 0 ? lane::Mid : lane::Bot;
            for (int x = 6; x < cols / 2 - 1; x += 3) {
                if ((x / 3 + band) % 7 == 4) {
                    continue;
                }
                const float upperY = lane_geometry::laneYAtX(cols, lines, upperLane, x);
                const float lowerY = lane_geometry::laneYAtX(cols, lines, lowerLane, x);
                const float gap = lowerY - upperY;
                const int centerX = std::clamp(x + jitter(rng), 3, cols - 4);
                const int preferredY = std::clamp(static_cast<int>(std::round(upperY + gap * 0.5f)),
                                                  3, lines - 4);
                const int centerY = findDividerY(routeMask, centerX,
                                                 static_cast<int>(std::round(upperY)),
                                                 static_cast<int>(std::round(lowerY)),
                                                 preferredY);
                const int value = (x / 3 + band) % 3 == 0 ? 1 : 3;
                const int slope = ((x / 3 + band) % 2 == 0) ? 1 : -1;
                placeDividerPatch(map, routeMask, GridPoint{centerX, centerY}, value, slope, true, red, blue, rng);
                if (std::abs(gap) >= 7.f) {
                    const float secondaryT = band == 0 ? 0.68f : 0.32f;
                    const int secondaryPreferredY = std::clamp(static_cast<int>(std::round(upperY + gap * secondaryT)),
                                                               3, lines - 4);
                    const int secondaryY = findDividerY(routeMask, centerX + 1,
                                                        static_cast<int>(std::round(upperY)),
                                                        static_cast<int>(std::round(lowerY)),
                                                        secondaryPreferredY);
                    if (std::abs(secondaryY - centerY) >= 2) {
                        placeDividerPatch(map, routeMask, GridPoint{centerX + 1, secondaryY},
                                          value == 1 ? 3 : value, -slope, false, red, blue, rng);
                    }
                }
                if (chance(rng) < 70) {
                    placeDividerPatch(map, routeMask, GridPoint{centerX + 3, centerY + slope},
                                      value == 1 ? 3 : 1, -slope, false, red, blue, rng);
                }
            }
        }

        for (int band = 0; band < 2; ++band) {
            const int upperLane = band == 0 ? lane::Top : lane::Mid;
            const int lowerLane = band == 0 ? lane::Mid : lane::Bot;
            for (int x = 8; x < cols / 2 - 2; x += 2) {
                if ((x / 2 + band) % 6 == 3) {
                    continue;
                }
                const float upperY = lane_geometry::laneYAtX(cols, lines, upperLane, x);
                const float lowerY = lane_geometry::laneYAtX(cols, lines, lowerLane, x);
                const float gap = lowerY - upperY;
                const int preferredY = std::clamp(static_cast<int>(std::round(upperY + gap * 0.5f)),
                                                  3, lines - 4);
                const int y = findDividerY(routeMask, x,
                                           static_cast<int>(std::round(upperY)),
                                           static_cast<int>(std::round(lowerY)),
                                           preferredY);
                const int value = (x / 4 + band) % 2 == 0 ? 3 : 1;
                const int dyAttempts[] = {0, -1, 1, -2, 2, -3, 3};
                int placed = 0;
                for (int dy : dyAttempts) {
                    if (placeDividerTile(map, routeMask, x, y + dy, value, red, blue)) {
                        ++placed;
                    }
                    if (placed >= 2) {
                        break;
                    }
                }
            }
        }
    }

    void placePonds(std::vector<std::vector<int>>& map, const std::vector<std::vector<bool>>& routeMask,
                    GridPoint red, GridPoint blue, std::mt19937& rng)
    {
        const int lines = static_cast<int>(map.size());
        const int cols = static_cast<int>(map.front().size());
        std::uniform_int_distribution<int> jitter(-2, 2);
        const auto paintPond = [&](GridPoint center) {
            for (int y = center.y - 2; y <= center.y + 2; ++y) {
                for (int x = center.x - 2; x <= center.x + 2; ++x) {
                    if (!inside(x, y, cols, lines)
                        || routeMask[y][x]
                        || nearBase(GridPoint{x, y}, red, blue, 8)) {
                        continue;
                    }
                    const int dx = x - center.x;
                    const int dy = y - center.y;
                    if (dx * dx + dy * dy <= 4) {
                        map[y][x] = 2;
                    }
                }
            }
        };
        const std::vector<GridPoint> anchors = {
            GridPoint{cols / 4, std::max(5, lines / 2 - 8)},
            GridPoint{cols / 3, std::min(lines - 6, lines / 2 + 9)},
            GridPoint{std::max(4, cols / 5), std::max(5, lines / 4 - 2)}
        };

        for (const auto& anchor : anchors) {
            const GridPoint center{
                std::clamp(anchor.x + jitter(rng), 3, cols - 4),
                std::clamp(anchor.y + jitter(rng), 3, lines - 4)
            };
            paintPond(center);
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

void mapgenerator::gmap(std::vector<std::vector<int>>& initMap, int cols, int lines, unsigned int seed)
{
    if (cols <= 0 || lines <= 0) {
        return;
    }

    initMap.assign(lines, std::vector<int>(cols, 0));
    const GridPoint red{5, std::max(5, lines / 2)};
    const GridPoint blue{std::max(5, cols - 7), std::max(5, lines / 2)};

    const unsigned int mapSeed = seed != 0 ? seed : std::random_device{}();
    std::mt19937 rng(mapSeed);
    std::uniform_int_distribution<int> xDist(2, std::max(2, cols - 3));
    std::uniform_int_distribution<int> yDist(2, std::max(2, lines - 3));
    std::uniform_int_distribution<int> mountainRadius(2, 4);
    std::uniform_int_distribution<int> forestRadius(2, 5);

    std::vector<std::vector<bool>> routeMask(lines, std::vector<bool>(cols, false));
    std::vector<GridPoint> plazas;
    for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
        const auto route = lane_geometry::laneRoute(cols, lines, laneIndex);
        const int corridorRadius = laneIndex == lane::Mid ? 2 : 1;
        for (std::size_t i = 1; i < route.size(); ++i) {
            carveMainRoute(initMap, routeMask, toGrid(route[i - 1]), toGrid(route[i]), corridorRadius, rng);
        }
        plazas.push_back(toGrid(route[2]));
        if (laneIndex != lane::Mid) {
            plazas.push_back(toGrid(route[1]));
            plazas.push_back(toGrid(route[3]));
        }
    }

    plazas.push_back(GridPoint{cols / 3, lines / 2});
    plazas.push_back(GridPoint{cols * 2 / 3, lines / 2});
    plazas.push_back(GridPoint{std::min(cols - 4, red.x + 8), std::min(lines - 4, red.y + 4)});
    plazas.push_back(GridPoint{std::max(3, blue.x - 8), std::max(3, blue.y - 4)});
    for (const auto& plaza : plazas) {
        // Resource fights need readable open ground; mark these pockets as
        // route-safe so later obstacle passes do not seal them off.
        clearDisc(initMap, plaza.x, plaza.y, 3);
        markDisc(routeMask, plaza.x, plaza.y, 3);
    }
    mirrorMaskHorizontally(routeMask);

    placeRiver(initMap, routeMask, red, blue, rng);
    placeTributary(initMap, routeMask, red, blue, rng);

    const int mountainClusters = std::max(9, cols * lines / 260);
    for (int i = 0; i < mountainClusters; ++i) {
        placeCluster(initMap, routeMask, GridPoint{xDist(rng), yDist(rng)}, mountainRadius(rng), 1, red, blue, rng);
    }

    const int forestClusters = std::max(21, cols * lines / 112);
    for (int i = 0; i < forestClusters; ++i) {
        placeCluster(initMap, routeMask, GridPoint{xDist(rng), yDist(rng)}, forestRadius(rng), 3, red, blue, rng);
    }
    placeRidgeFingers(initMap, routeMask, red, blue, rng);
    placeLaneShoulders(initMap, routeMask, red, blue, rng);
    placeLaneDividerBelts(initMap, routeMask, red, blue, rng);
    placePonds(initMap, routeMask, red, blue, rng);
    placeResourceCover(initMap, routeMask, plazas, red, blue, rng);
    mirrorTerrainHorizontally(initMap, routeMask, red, blue);

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

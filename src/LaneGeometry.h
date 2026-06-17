#pragma once

#include "Config.h"
#include "Point.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace lane_geometry
{
    inline int safeLane(int laneIndex)
    {
        return std::clamp(laneIndex, 0, lane::Count - 1);
    }

    inline int centerY(int mapH)
    {
        return std::max(5, mapH / 2);
    }

    inline int laneAnchorY(int mapH, int laneIndex)
    {
        switch (safeLane(laneIndex)) {
        case lane::Top:
            return std::max(4, mapH / 4);
        case lane::Bot:
            return std::min(mapH - 5, mapH * 3 / 4);
        case lane::Mid:
        default:
            return centerY(mapH);
        }
    }

    inline std::array<Point, 3> laneWaypoints(int mapW, int mapH, int laneIndex)
    {
        const int laneY = laneAnchorY(mapH, laneIndex);
        const int middle = centerY(mapH);
        const int x0 = std::clamp(mapW / 4, 7, mapW - 8);
        const int x1 = std::clamp(mapW / 2, 8, mapW - 9);
        const int x2 = std::clamp(mapW * 3 / 4, 9, mapW - 8);

        switch (safeLane(laneIndex)) {
        case lane::Top: {
            const int shoulderY = std::min(laneY + 2, middle - 2);
            const int crestY = std::max(3, laneY - 2);
            return {Point(x0, shoulderY), Point(x1, crestY), Point(x2, shoulderY)};
        }
        case lane::Bot: {
            const int shoulderY = std::max(laneY - 2, middle + 2);
            const int crestY = std::min(mapH - 4, laneY + 2);
            return {Point(x0, shoulderY), Point(x1, crestY), Point(x2, shoulderY)};
        }
        case lane::Mid:
        default:
            return {Point(x0, middle), Point(x1, middle), Point(x2, middle)};
        }
    }

    inline Point laneWaypoint(int mapW, int mapH, int laneIndex, int stage, bool reverse)
    {
        const auto points = laneWaypoints(mapW, mapH, laneIndex);
        const int safeStage = std::clamp(stage, 0, 2);
        return reverse ? points[2 - safeStage] : points[safeStage];
    }

    inline std::array<Point, 5> laneRoute(int mapW, int mapH, int laneIndex)
    {
        const auto points = laneWaypoints(mapW, mapH, laneIndex);
        return {
            Point(5, centerY(mapH)),
            points[0],
            points[1],
            points[2],
            Point(std::max(5, mapW - 7), centerY(mapH))
        };
    }

    inline float laneYAtX(int mapW, int mapH, int laneIndex, int x)
    {
        const auto route = laneRoute(mapW, mapH, laneIndex);
        for (std::size_t i = 1; i < route.size(); ++i) {
            const Point a = route[i - 1];
            const Point b = route[i];
            const int minX = std::min(a.x, b.x);
            const int maxX = std::max(a.x, b.x);
            if (x < minX || x > maxX || minX == maxX) {
                continue;
            }
            const float t = static_cast<float>(x - a.x) / static_cast<float>(b.x - a.x);
            return static_cast<float>(a.y) + (static_cast<float>(b.y) - static_cast<float>(a.y)) * t;
        }
        return static_cast<float>(laneAnchorY(mapH, laneIndex));
    }
}

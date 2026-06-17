#pragma once

#include "Config.h"
#include "Point.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace lane_geometry
{
    inline constexpr int RallyStageCount = 5;

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

    inline std::array<Point, RallyStageCount> laneRallyWaypoints(int mapW, int mapH, int laneIndex)
    {
        const int middle = centerY(mapH);
        const int laneY = laneAnchorY(mapH, laneIndex);
        const int x0 = std::clamp(mapW / 5, 7, mapW - 8);
        const int x1 = std::clamp(mapW * 3 / 10, 8, mapW - 9);
        const int x2 = std::clamp(mapW / 2, 9, mapW - 10);
        const int x3 = std::clamp(mapW * 7 / 10, 10, mapW - 9);
        const int x4 = std::clamp(mapW - 1 - x0, 11, mapW - 7);

        switch (safeLane(laneIndex)) {
        case lane::Top: {
            const int baseExitY = std::max(3, middle - 3);
            const int shoulderY = std::min(laneY + 2, middle - 2);
            const int crestY = std::max(3, laneY - 2);
            return {Point(x0, baseExitY), Point(x1, shoulderY), Point(x2, crestY),
                    Point(x3, shoulderY), Point(x4, baseExitY)};
        }
        case lane::Bot: {
            const int baseExitY = std::min(mapH - 4, middle + 3);
            const int shoulderY = std::max(laneY - 2, middle + 2);
            const int crestY = std::min(mapH - 4, laneY + 2);
            return {Point(x0, baseExitY), Point(x1, shoulderY), Point(x2, crestY),
                    Point(x3, shoulderY), Point(x4, baseExitY)};
        }
        case lane::Mid:
        default:
            return {Point(x0, middle), Point(x1, middle), Point(x2, middle),
                    Point(x3, middle), Point(x4, middle)};
        }
    }

    inline Point laneWaypoint(int mapW, int mapH, int laneIndex, int stage, bool reverse)
    {
        const auto points = laneWaypoints(mapW, mapH, laneIndex);
        const int safeStage = std::clamp(stage, 0, 2);
        return reverse ? points[2 - safeStage] : points[safeStage];
    }

    inline Point laneRallyWaypoint(int mapW, int mapH, int laneIndex, int stage, bool reverse)
    {
        const auto points = laneRallyWaypoints(mapW, mapH, laneIndex);
        const int safeStage = std::clamp(stage, 0, RallyStageCount - 1);
        return reverse ? points[RallyStageCount - 1 - safeStage] : points[safeStage];
    }

    inline std::array<Point, RallyStageCount + 2> laneRoute(int mapW, int mapH, int laneIndex)
    {
        const auto points = laneRallyWaypoints(mapW, mapH, laneIndex);
        return {
            Point(5, centerY(mapH)),
            points[0],
            points[1],
            points[2],
            points[3],
            points[4],
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

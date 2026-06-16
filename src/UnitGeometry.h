#pragma once

#include "AllUnit.h"
#include "Config.h"

#include <algorithm>
#include <cmath>

namespace unit_geometry
{
    inline int footprintSize(const Unit& unit)
    {
        return unit.unitName == UName::BASE ? config::BaseFootprintSize : 1;
    }

    inline Point closestFootprintCell(Point point, const Unit& unit)
    {
        const int size = footprintSize(unit);
        return Point(
            std::clamp(point.x, unit.x, unit.x + size - 1),
            std::clamp(point.y, unit.y, unit.y + size - 1));
    }

    inline int distanceSquaredToFootprint(Point point, const Unit& unit)
    {
        const Point closest = closestFootprintCell(point, unit);
        const int dx = point.x - closest.x;
        const int dy = point.y - closest.y;
        return dx * dx + dy * dy;
    }

    inline bool isInAttackRange(Point attackerPoint, const Unit& target, int range)
    {
        if (range < 0) {
            return false;
        }

        // Keep the historical integer-grid range feel, but measure from the
        // closest occupied footprint tile so 2x2 bases can be attacked from
        // their right and bottom edges instead of only their top-left anchor.
        return static_cast<int>(std::sqrt(static_cast<float>(distanceSquaredToFootprint(attackerPoint, target)))) <= range;
    }
}

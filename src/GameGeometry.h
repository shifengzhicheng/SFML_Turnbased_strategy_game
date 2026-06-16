#pragma once

#include "AllUnit.h"
#include "Config.h"
#include "Point.h"

#include <SFML/Graphics.hpp>

namespace game_internal
{
    inline bool nearPoint(Point a, Point b, int radius)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy <= radius * radius;
    }

    inline int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    inline sf::Vector2f unitCenter(const Unit& unit)
    {
        const auto bounds = unit.getGlobalBounds();
        if (bounds.width > 0.f && bounds.height > 0.f) {
            return sf::Vector2f(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        }
        const float baseOffset = unit.unitName == UName::BASE ? config::TileSize : config::TileSize / 2.f;
        return sf::Vector2f(unit.x * config::TileSize + baseOffset, unit.y * config::TileSize + baseOffset);
    }
}

#pragma once

#include "Config.h"
#include "GameTypes.h"
#include "Point.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

namespace game_internal
{
    inline bool loadFont(sf::Font& font, const std::string& path)
    {
        if (!font.loadFromFile(path)) {
            std::cerr << "Failed to load font: " << path << std::endl;
            return false;
        }
        return true;
    }

    inline void setupText(sf::Text& text, const sf::Font& font, unsigned int size, sf::Color color,
                   const std::string& value, float x, float y)
    {
        text.setFont(font);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setString(value);
        text.setPosition(x, y);
    }

    inline void drawUnitBase(sf::RenderWindow& window, Point point, sf::Color color)
    {
        sf::CircleShape marker(9.f, 28);
        marker.setOrigin(9.f, 9.f);
        marker.setScale(1.15f, 0.62f);
        marker.setPosition(point.x * config::TileSize + config::TileSize / 2.f,
                           point.y * config::TileSize + config::TileSize / 2.f + 7.f);
        marker.setFillColor(sf::Color(color.r, color.g, color.b, 96));
        marker.setOutlineColor(sf::Color(color.r, color.g, color.b, 210));
        marker.setOutlineThickness(1.4f);
        window.draw(marker);
    }

    inline sf::Color ownerColor(int owner)
    {
        if (owner == PLAYER) {
            return sf::Color(218, 76, 60);
        }
        if (owner == AI) {
            return sf::Color(61, 128, 206);
        }
        if (owner == -2) {
            return sf::Color(236, 111, 72);
        }
        return sf::Color(226, 180, 63);
    }
}

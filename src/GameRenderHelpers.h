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

    inline void drawUnitBase(sf::RenderTarget& target, Point point, sf::Color color)
    {
        const sf::Vector2f origin(point.x * config::TileSize, point.y * config::TileSize);
        sf::RectangleShape shadow(sf::Vector2f(18.f, 4.f));
        shadow.setPosition(origin + sf::Vector2f(1.f, 16.f));
        shadow.setFillColor(sf::Color(color.r, color.g, color.b, 86));
        target.draw(shadow);
        sf::RectangleShape core(sf::Vector2f(12.f, 2.f));
        core.setPosition(origin + sf::Vector2f(4.f, 17.f));
        core.setFillColor(sf::Color(color.r, color.g, color.b, 150));
        target.draw(core);
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

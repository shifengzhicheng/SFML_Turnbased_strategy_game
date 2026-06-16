#pragma once

#include "Config.h"

#include <SFML/Graphics.hpp>

namespace unit_visual
{
    inline constexpr int TileSize = config::TileSize;
    inline constexpr int MapWidth = config::MapWidth;
    inline constexpr int MapHeight = config::MapHeight;

    inline sf::Vector2f centeredUnitPosition(const sf::Sprite& sprite,
                                             int tileX,
                                             int tileY,
                                             float scale,
                                             sf::Vector2f offset = sf::Vector2f(0.f, 0.f))
    {
        const sf::Texture* texture = sprite.getTexture();
        const sf::Vector2u size = texture != nullptr
            ? texture->getSize()
            : sf::Vector2u(config::UnitTextureSize, config::UnitTextureSize);
        return sf::Vector2f(
            tileX * TileSize + (TileSize - static_cast<float>(size.x) * scale) * 0.5f + offset.x,
            tileY * TileSize + (TileSize - static_cast<float>(size.y) * scale) * 0.5f + offset.y);
    }

    inline void placeUnitSprite(sf::Sprite& sprite,
                                int tileX,
                                int tileY,
                                float scale,
                                sf::Vector2f offset = sf::Vector2f(0.f, 0.f))
    {
        sprite.setScale(scale, scale);
        sprite.setPosition(centeredUnitPosition(sprite, tileX, tileY, scale, offset));
    }

    inline void placeHealthLabel(sf::Text& text,
                                 int tileX,
                                 int tileY,
                                 sf::Vector2f offset = sf::Vector2f(0.f, 0.f))
    {
        const auto bounds = text.getLocalBounds();
        text.setPosition(
            tileX * TileSize + TileSize * 0.5f - bounds.left - bounds.width * 0.5f + offset.x,
            tileY * TileSize - 12.f + offset.y * 0.35f);
    }
}

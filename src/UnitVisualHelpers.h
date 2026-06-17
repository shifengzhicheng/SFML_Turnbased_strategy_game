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
        // Gameplay still uses tile coordinates; the sprite is anchored to the
        // unit's feet so 2.5D art can rise above its occupied grid cell.
        const float footX = tileX * TileSize + TileSize * 0.5f;
        const float footY = tileY * TileSize + TileSize + 1.f;
        return sf::Vector2f(
            footX - static_cast<float>(size.x) * scale * 0.5f + offset.x,
            footY - static_cast<float>(size.y) * scale + offset.y);
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
        const float visualTop = tileY * TileSize + TileSize + 1.f
            - static_cast<float>(config::UnitTextureSize) * config::UnitSpriteScale;
        text.setPosition(
            tileX * TileSize + TileSize * 0.5f - bounds.left - bounds.width * 0.5f + offset.x,
            visualTop - 11.f + offset.y * 0.35f);
    }
}

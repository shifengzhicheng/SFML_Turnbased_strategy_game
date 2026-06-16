#pragma once

#include "ArtAssets.h"

#include <SFML/Graphics.hpp>
#include <string>

namespace art_internal
{
    sf::Color mix(sf::Color a, sf::Color b, float t);
    void commitTexture(sf::Texture& texture, const sf::RenderTexture& renderTexture);
    bool createCanvas(sf::RenderTexture& canvas, sf::Vector2u size);
    void drawPill(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size,
                  float radius, sf::Color fill, sf::Color outline, float outlineThickness);
    void drawSword(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color metal, sf::Color accent);
    void drawBow(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color accent, sf::Color stringColor);
    void drawBase(sf::RenderTarget& target, sf::Vector2f pos, float scale, art::Team team);
    void drawUnitIcon(sf::RenderTarget& target, art::UnitKind kind, art::Team team, sf::Vector2f center, float scale);
    const char* costForIcon(art::UnitKind icon);
    void drawTextCentered(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                          unsigned int size, sf::Color color, sf::FloatRect box);
}

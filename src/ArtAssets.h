#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace art
{
    enum class Team
    {
        Player,
        Enemy,
        Neutral
    };

    enum class UnitKind
    {
        Infantry,
        Shooter,
        Cavalry,
        Base,
        None
    };

    enum class ButtonState
    {
        Normal,
        Hover,
        Pressed
    };

    sf::Color teamColor(Team team);
    sf::Color teamAccent(Team team);

    void makeIntroTexture(sf::Texture& texture, const sf::Font& font);
    void makeUnitTexture(sf::Texture& texture, UnitKind kind, Team team);
    void makeButtonTexture(sf::Texture& texture, const sf::Font& font, const std::string& label,
                           ButtonState state, sf::Vector2u size, UnitKind icon = UnitKind::None,
                           Team team = Team::Neutral);
}

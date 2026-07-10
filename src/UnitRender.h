#pragma once

#include <SFML/Graphics/RenderTarget.hpp>

class MoveableUnit;

namespace unit_render
{
    void drawCombatCue(sf::RenderTarget& target, const MoveableUnit& unit, float gameTimeSeconds);
    void drawHealthBar(sf::RenderTarget& target, const MoveableUnit& unit, bool showNumber);
}

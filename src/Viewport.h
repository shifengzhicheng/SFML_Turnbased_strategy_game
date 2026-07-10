#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

namespace viewport
{
    struct Layout
    {
        sf::FloatRect normalized;
        sf::FloatRect pixels;
    };

    Layout calculate(sf::Vector2u windowSize, sf::Vector2f logicalSize);
    sf::View makeView(sf::Vector2u windowSize, sf::Vector2f logicalSize,
                      sf::Vector2f logicalOffset = sf::Vector2f());
    sf::Vector2f mapPixelToLogical(sf::Vector2i pixel, sf::Vector2u windowSize,
                                   sf::Vector2f logicalSize);
}

#pragma once

#include "Point.h"

#include <SFML/System/Clock.hpp>

struct ResourceNode
{
    Point point;
    sf::Clock pulseClock;
};

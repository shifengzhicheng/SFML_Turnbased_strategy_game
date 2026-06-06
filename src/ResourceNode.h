#pragma once

#include "Point.h"

#include <SFML/System/Clock.hpp>

struct ResourceNode
{
    Point point;
    int owner = -1;
    sf::Clock pulseClock;
};

#pragma once

#include "Point.h"

#include <SFML/System/Clock.hpp>

struct ResourceNode
{
    Point point;
    int owner = -1;
    int income = 0;
    int contestingTeam = -1;
    float captureProgress = 0.f;
    sf::Clock pulseClock;
};

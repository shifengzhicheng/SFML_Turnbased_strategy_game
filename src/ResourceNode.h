#pragma once

#include "Point.h"

#include <SFML/System/Clock.hpp>

namespace resource
{
    enum Kind
    {
        Gold,
        Crystal,
        Foundry,
        Shrine
    };
}

struct ResourceNode
{
    Point point;
    int owner = -1;
    int kind = resource::Gold;
    int income = 0;
    int contestingTeam = -1;
    float captureProgress = 0.f;
    sf::Clock pulseClock;
};

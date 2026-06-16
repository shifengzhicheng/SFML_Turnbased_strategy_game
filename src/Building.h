#pragma once

#include "Point.h"
#include "ProductionQueue.h"

namespace building
{
    enum Type
    {
        Barracks,
        DefenseTower
    };
}

struct Building
{
    int id = 0;
    int team = 0;
    int type = building::Barracks;
    Point point;
    bool complete = false;
    int health = 1;
    int maxHealth = 1;
    float buildProgress = 0.f;
    float buildSeconds = 1.f;
    float attackTimer = 0.f;
    ProductionQueue production;
};

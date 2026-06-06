#pragma once

#include "Point.h"
#include "ProductionQueue.h"

namespace building
{
    enum Type
    {
        Extractor,
        Barracks
    };
}

struct Building
{
    int id = 0;
    int team = 0;
    int type = building::Extractor;
    Point point;
    int resourceIndex = -1;
    bool complete = false;
    float buildProgress = 0.f;
    float buildSeconds = 1.f;
    ProductionQueue production;
};

#pragma once

#include "Point.h"

#include <deque>

namespace worker
{
    enum State
    {
        Idle,
        MovingToBuild,
        Building,
        MovingToHarvest,
        Harvesting
    };
}

struct Worker
{
    int id = 0;
    int team = 0;
    Point point;
    Point target;
    int state = worker::Idle;
    int buildingId = 0;
    float moveTimer = 0.f;
    float pathTimer = 0.f;
    int pendingPathRequest = 0;
    std::deque<Point> path;
};

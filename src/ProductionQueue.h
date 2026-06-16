#pragma once

#include <deque>

struct ProductionOrder
{
    int unit = -1;
    int lane = 1;
};

struct ProductionQueue
{
    std::deque<ProductionOrder> orders;
    int activeUnit = -1;
    int activeLane = 1;
    float progress = 0.f;

    bool empty() const
    {
        return activeUnit < 0 && orders.empty();
    }

    int load() const
    {
        return static_cast<int>(orders.size()) + (activeUnit >= 0 ? 1 : 0);
    }
};

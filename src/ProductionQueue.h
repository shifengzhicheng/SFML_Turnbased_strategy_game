#pragma once

#include <deque>

struct ProductionQueue
{
    std::deque<int> orders;
    int activeUnit = -1;
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

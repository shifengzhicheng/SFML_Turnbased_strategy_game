#pragma once

struct Point
{
    int x = 0;
    int y = 0;

    Point() = default;
    Point(int _x, int _y) : x(_x), y(_y) {}
};

#pragma once
#include <memory>

using namespace std;

// 边的权重，1为直移，2为斜移
const int kCost1 = 10;
const int kCost2 = 14;

struct Point
{
    int x, y;
    int F, G, H;
    shared_ptr<Point> parent;
    shared_ptr<Point> next;
    Point(int _x, int _y) {
        x = _x;
        y = _y;
        F = G = H = 0;
        parent = NULL;
        next = NULL;
    }
    Point() 
    {
        x = y = F = G = H = 0;
        parent = next = NULL;
    }
};
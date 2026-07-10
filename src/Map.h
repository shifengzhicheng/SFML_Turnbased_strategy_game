#pragma once

#include <vector>

class mapgenerator
{
public:
    void gmap(std::vector<std::vector<int>>& initMap, int cols, int lines, unsigned int seed = 0);
};

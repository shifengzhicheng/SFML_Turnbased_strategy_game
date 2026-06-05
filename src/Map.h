#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
class mapgenerator
{
public:
    mapgenerator() {}
    int checkNeighborWalls(std::vector<std::vector <int> >& M, int lines, int cols, int line, int col, int cycle);
    void gmap(std::vector<std::vector <int> >& initMap, int cols, int lines);
};
#include "Map.h"

int mapgenerator::checkNeighborWalls
(std::vector<std::vector <int> >& M, int lines, int cols, int line, int col, int cycle = 1)
{
    int count = 0;
    for (int i = line - cycle; i <= line + cycle; ++i)
        for (int j = col - cycle; j <= col + cycle; ++j)
        {
            if ((i >= 0 && i < lines) && (j >= 0 && j < cols))
                if (M[i][j] != M[line][col])
                    count++;
        }

    if (M[line][col] != 0)
        count--;

    return count;
}

void mapgenerator::gmap
(std::vector<std::vector <int> >& initMap, int cols, int lines) {

    std::default_random_engine e(time(0));
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (int line = 0; line < lines; ++line)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                initMap[line][col] = 1;
            else
                initMap[line][col] = u(e) < 0.40 ? 1 : 0;
        }
    }

    for (int loop = 0; loop < 5; ++loop)
    {
        for (int line = 0; line < lines; ++line)
        {
            for (int col = 0; col < cols; ++col)
            {
                int wallCount1 = checkNeighborWalls(initMap, lines, cols, line, col, 1);
                int wallCount2 = checkNeighborWalls(initMap, lines, cols, line, col, 2);

                if (initMap[line][col] == 1)
                {
                    initMap[line][col] = wallCount1 >= 4 ? 1 : 0;
                }
                else
                {
                    if (loop < 2)
                        initMap[line][col] = (wallCount1 >= 5) ? 1 : 0;
                    else
                        initMap[line][col] = (wallCount1 >= 5 || wallCount2 <= 2) ? 1 : 0;
                }
                if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                    initMap[line][col] = 1;
            }
        }
        for (int loop = 0; loop < 1; loop++) {
            for (int line = 0; line < lines; ++line)
            {
                for (int col = 0; col < cols; ++col)
                {
                    int wallCount1 = checkNeighborWalls(initMap, lines, cols, line, col, 1);
                    int wallCount2 = checkNeighborWalls(initMap, lines, cols, line, col, 2);
                    if (initMap[line][col] == 1)
                    {
                        initMap[line][col] = wallCount1 >= 3 ? 1 : rand() % 2 + 2;
                    }
                    else if (initMap[line][col] == 2) {
                        initMap[line][col] = wallCount1 >= 2 ? 2 : 3;
                    }
                    else if (initMap[line][col] == 3) {
                        const bool isolatedTile = wallCount1 <= 1;
                        initMap[line][col] = isolatedTile ? 1 : 0;
                        if (isolatedTile) break;
                    }
                    else {
                        initMap[line][col] = (wallCount1 >= 5 || wallCount2 <= 2) ? 1 : 0;
                    }
                    if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                        initMap[line][col] = 1;
                }
            }
        }
    }
}

#include "Astar.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    std::vector<std::vector<int>> grid(int width, int height)
    {
        return std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    }
}

int main()
{
    {
        auto maze = grid(5, 5);
        for (int y = 0; y < 4; ++y) {
            maze[y][1] = 1;
        }

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(2, 0), false);

        require(!path.empty(), "straight path should exist");
        require(path.front().x == 0 && path.front().y == 0, "path should include its start");
        require(path.back().x == 2 && path.back().y == 0, "path should reach its goal");
    }

    {
        auto maze = grid(3, 3);
        maze[0][1] = 1;
        maze[1][0] = 1;

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), true);
        require(path.empty(), "blocked diagonal corner should be unreachable");
    }

    {
        auto maze = grid(2, 2);

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), true);
        require(path.size() == 2, "open diagonal move should use one step");
    }

    {
        auto maze = grid(3, 3);
        maze[1][1] = 1;

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), false);
        require(path.empty(), "cardinal mode should reject a diagonal-only route");
    }

    return 0;
}

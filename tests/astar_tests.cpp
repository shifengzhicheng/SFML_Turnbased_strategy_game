#include "Astar.h"

#include <cassert>
#include <vector>

namespace
{
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

        assert(!path.empty());
        assert(path.front().x == 0 && path.front().y == 0);
        assert(path.back().x == 2 && path.back().y == 0);
    }

    {
        auto maze = grid(3, 3);
        maze[0][1] = 1;
        maze[1][0] = 1;

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), true);
        assert(path.empty());
    }

    {
        auto maze = grid(2, 2);

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), true);
        assert(path.size() == 2);
    }

    {
        auto maze = grid(3, 3);
        maze[1][1] = 1;

        Astar astar(maze);
        const auto path = astar.GetPath(Point(0, 0), Point(1, 1), false);
        assert(path.empty());
    }

    return 0;
}

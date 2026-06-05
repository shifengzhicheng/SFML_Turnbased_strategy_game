#pragma once
#include <deque>
#include <vector>
#include "Point.h"

class Astar
{
private:
    struct PathNode;

    std::vector<std::vector<int>> maze;
    std::vector<Point> searchPath;

    bool isInside(Point point) const;
    bool isBlocked(Point point) const;
    bool isCanReach(Point from, Point target, bool allowDiagonal) const;
    std::vector<Point> getSurroundPoints(Point point, bool allowDiagonal) const;
    int calcG(Point from, Point target) const;
    int calcH(Point point, Point end) const;
    int calcF(int g, int h) const;
    std::deque<Point> reconstructPath(const std::vector<PathNode>& nodes, int endIndex) const;

public:
    Astar();
    explicit Astar(const std::vector<std::vector<int>>& _maze);

    static std::vector<std::vector<int>> makeEmptyMaze();

    void setMaze(const std::vector<std::vector<int>>& _maze);
    std::deque<Point> GetPath(Point startPoint, Point endPoint, bool allowDiagonal);
    const std::vector<Point>& getSearchPath() const;
};

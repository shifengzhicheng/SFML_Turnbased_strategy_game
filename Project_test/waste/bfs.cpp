#include "BFS.h"
#include "Point.h"
BFS::BFS(vector<vector<int> > _maze)
{
    this->maze = _maze;
}

void BFS::setMaze(vector<vector<int> > _maze)
{
    this->maze = _maze;
}

list<shared_ptr<Point> > BFS::GetPath(Point startPoint, Point endPoint, bool isIgnoreCorner)
{
    shared_ptr<Point> result = findPath(startPoint, endPoint, isIgnoreCorner);
    list<shared_ptr<Point>> path;
    while (result)
    {
        path.push_front(result);
        result = result->parent;
    }

    return path;
}

shared_ptr<Point> BFS::findPath(Point startPoint, Point endPoint, bool isIgnoreCorner)
{
    openList.clear();
    closeList.clear();
    shared_ptr<Point> newEndPoint(new Point(endPoint.x, endPoint.y));
    openList.push_back(shared_ptr<Point>(new Point(startPoint.x, startPoint.y)));

    while (!openList.empty())
    {
        auto curPoint = openList.front();
        openList.remove(curPoint);
        auto surroundPoints = getSurroundPoints(curPoint, isIgnoreCorner);
        for (auto& target : surroundPoints)
        {
            if (!isInList(closeList, target))
            {
                target->parent = curPoint;
                openList.push_back(target);
                closeList.push_back(target);
            }
            shared_ptr<Point> resPoint = isInList(openList, newEndPoint);
            if (resPoint)
                return resPoint;
        }
    }
    return NULL;
}
list<shared_ptr<Point>> BFS::getSearchPath()
{
    return closeList;
}
vector<shared_ptr<Point> > BFS::getSurroundPoints(shared_ptr<Point> point, bool isIgnoreCorner) const
{
    vector<shared_ptr<Point>> surroundPoints;
    for (int x = point->x - 1; x <= point->x + 1; x++)
        for (int y = point->y - 1; y <= point->y + 1; y++)
            if (isCanReach(point, shared_ptr<Point>(new Point(x, y)), isIgnoreCorner))
                surroundPoints.push_back(shared_ptr<Point>(new Point(x, y)));

    return surroundPoints;
}
shared_ptr<Point> BFS::isInList(list<shared_ptr<Point>> list, shared_ptr<Point> point) const
{
    for (auto p : list)
        if (p->x == point->x && p->y == point->y)
            return p;
    return NULL;
}
bool BFS::isCanReach(shared_ptr<Point> point, shared_ptr<Point> target, bool isIgnoreCorner) const
{
    if ((target->x<0 || target->x>maze.size() - 1)
        || (target->y<0 && target->y>maze[0].size() - 1)
        || (maze[target->x][target->y] == 1)
        || (target->x == point->x && target->y == point->y))
    {
        return false;
    }
    else
    {
        if (abs(point->x - target->x) + abs(point->y - target->y) == 1)
            return true;
        else
        {
            if (maze[point->x][target->y] == 0 && maze[target->x][point->y] == 0)
                return true;
            else
                return isIgnoreCorner;
        }
    }
}
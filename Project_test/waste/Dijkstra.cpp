#include "Dijkstra.h"

Dijkstra::Dijkstra(vector<vector<int>> _maze)
{
    this->maze = _maze;
    for (int i = 0; i < this->maze.size() * this->maze[0].size(); ++i)
        cost.push_back(INT_MAX);
}
void Dijkstra::setMaze(vector<vector<int>> _maze)
{
    this->maze = _maze;
}
list<shared_ptr<Point> > Dijkstra::GetPath(Point startPoint, Point endPoint, bool isIgnoreCorner)
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
shared_ptr<Point> Dijkstra::findPath(Point startPoint, Point endPoint, bool isIgnoreCorner)
{
    openList.clear();
    closeList.clear();
    cost.clear();
    for (int i = 0; i < this->maze.size() * this->maze[0].size(); ++i)
        cost.push_back(INT_MAX);
    shared_ptr<Point> resPoint = NULL;
    shared_ptr<Point> newEndPoint(new Point(endPoint.x, endPoint.y));
    openList.push_back(shared_ptr<Point>(new Point(startPoint.x, startPoint.y)));
    cost[startPoint.y * maze[0].size() + startPoint.x] = 0;

    while (!resPoint)
    {
        auto curPoint = openList.front();
        openList.remove(curPoint);
        auto surroundPoints = getSurroundPoints(curPoint, isIgnoreCorner);

        for (auto& target : surroundPoints)
        {
            auto new_cost = cost[curPoint->y * maze[0].size() + curPoint->x] + costCurToNext(curPoint, target);
            if (!isInList(closeList, target) || (new_cost < cost[target->y * maze[0].size() + target->x]))
            {
                shared_ptr<Point> oldTarget = isInList(openList, target);
                if (oldTarget)
                    target = oldTarget;

                target->parent = curPoint;
                cost[target->y * maze[0].size() + target->x] = new_cost;
                openList.push_back(target);
                closeList.push_back(target);
            }
        }
        resPoint = isInList(closeList, newEndPoint);
    }

    return resPoint;
}
list<shared_ptr<Point> > Dijkstra::getSearchPath()
{
    return closeList;
}
vector<shared_ptr<Point> > Dijkstra::getSurroundPoints(shared_ptr<Point> point, bool isIgnoreCorner) const
{
    vector<shared_ptr<Point>> surroundPoints;

    for (int x = point->x - 1; x <= point->x + 1; x++)
        for (int y = point->y - 1; y <= point->y + 1; y++)
            if (isCanReach(point, unique_ptr<Point>(new Point(x, y)), isIgnoreCorner))
                surroundPoints.push_back(shared_ptr<Point>(new Point(x, y)));

    return surroundPoints;
}

bool Dijkstra::isCanReach(shared_ptr<Point> point, unique_ptr<Point> target, bool isIgnoreCorner) const
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
shared_ptr<Point> Dijkstra::isInList(list<shared_ptr<Point>> list, shared_ptr<Point> point) const
{
    for (auto p : list)
        if (p->x == point->x && p->y == point->y)
            return p;
    return NULL;
}

int Dijkstra::costCurToNext(shared_ptr<Point> temp_point, shared_ptr<Point> point)
{
    int extra;
    if ((abs(point->x - temp_point->x) + abs(point->y - temp_point->y)) == 1)
    {
        extra = kCost1;
    }
    else
    {
        extra = kCost2;
    }
    return extra;
}
#include "Astar.h"
#include "Point.h"
#define height 720
#define width 1280
#define SqureSize 20

Astar::Astar(vector<vector<int>> &_maze)
{
    this->maze = _maze;
    this->emptymaze = vector<vector<int>>(height / SqureSize, vector<int>(width / SqureSize, 0));
}

Astar::Astar() {
        this->emptymaze = vector<vector<int>>(height / SqureSize, vector<int>(width / SqureSize, 0));
        this->maze = emptymaze;
}

// 重置地图
void Astar::setMaze(vector<vector<int>> &_maze)
{
    this->maze = _maze;
}

//计算路径返回指针列表
list<shared_ptr<Point> > Astar::GetPath(Point startPoint, Point endPoint, bool isIgnoreCorner=false)
{

    shared_ptr<Point> result = findPath(startPoint, endPoint, isIgnoreCorner);
    list<shared_ptr<Point>> path;
    //返回路径，如果没找到路径则返回空链表
    while (result)
    {
        path.push_front(result);
        result = result->parent;
    }

    return path;
}

//A*算法
shared_ptr<Point> Astar::findPath(Point startPoint, Point endPoint, bool isIgnoreCorner)
{
    //先清空openlist和closelist
    openList.clear();
    closeList.clear();

    shared_ptr<Point> newEndPoint(new Point(endPoint.x, endPoint.y));
    openList.push_back(shared_ptr<Point>(new Point(startPoint.x, startPoint.y)));

    while (!openList.empty())
    {
        auto curPoint = getLeastFpoint();
        openList.remove(curPoint);
        closeList.push_back(curPoint);
        //1，找到当前周围的八个格子中可以通过的格子
        auto surroundPoints = getSurroundPoints(curPoint, isIgnoreCorner);
        for (auto& target : surroundPoints)
        {
            //2，对于某个格子如果不在列表中就加入到开启列表，设置当前格子为其父节点，计算FGH
            if (!isInList(openList, target))
            {
                target->parent = curPoint;

                target->G = calcG(curPoint, target);

                target->H = calcH(target, newEndPoint);

                target->F = calcF(target);

                openList.push_back(target);
            }
            //对于某个格子，它在开启列表中就计算G值，比原来的大就什么都不做，否则设置它的父节点为当前节点并更新GF
            else
            {
                int tempG = calcG(curPoint, target);
                if (tempG < target->G)
                {
                    target->parent = curPoint;

                    target->G = tempG;

                    target->F = calcF(target);
                }
            }
            //如果找到了目标节点就直接返回
            shared_ptr<Point> resPoint = isInList(openList, newEndPoint);
            if (resPoint)
                //返回指针节点
                return resPoint;
        }
    }
    //失败返回NULL
    return NULL;
}

list<shared_ptr<Point> > Astar::getSearchPath()
{
    return closeList;
}

/*  找到F值最小的下一个节点 */
shared_ptr<Point> Astar::getLeastFpoint()
{
    if (!openList.empty())
    {
        auto resPoint = openList.front();
        for (auto& point : openList)
        {
            if (point->F < resPoint->F)
            {
                resPoint = point;
            }
        }
        return resPoint;
    }
    return NULL;
}

vector<shared_ptr<Point> > Astar::getSurroundPoints(shared_ptr<Point> point, bool isIgnoreCorner) const
{
    vector<shared_ptr<Point>> surroundPoints;
    for (int x = point->x - 1; x < (int)maze[0].size() && x <= point->x + 1; x++) {
        if (x < 0) continue;
        int y = point->y - 1;
        for (; y < (int)maze.size() && y <= point->y + 1; y++) {
            if (y < 0) continue;
            else if (isCanReach(point, shared_ptr<Point>(new Point(x, y)), isIgnoreCorner))
                surroundPoints.push_back(shared_ptr<Point>(new Point(x, y)));
        }
    }
    return surroundPoints;
}

// 判断是否可以通过
bool Astar::isCanReach(shared_ptr<Point> point, shared_ptr<Point> target, bool isIgnoreCorner) const
{
    //如果点与当前节点重合、超出地图、是障碍物、或者在关闭列表中，返回false
    if ((target->x<0 || target->x>=maze[0].size())
        || (target->y<0 && target->y>=maze.size())
        || (maze[target->y][target->x] == 1)
        || (target->x == point->x && target->y == point->y)
        || (isInList(closeList, target)))
    {
        return false;
    }
    else
    {
        if (abs(point->x - target->x) + abs(point->y - target->y) == 1) //仅仅非斜角可以
            return true;
        else
        {
 /*           return false;*/
            //斜对角要判断是否绊住
            if (isIgnoreCorner && maze[point->y][target->x] == 0 && maze[target->y][point->x] == 0)
                return true;
            else return false;
        }
    }
}


shared_ptr<Point> Astar::isInList(list<shared_ptr<Point>> list, shared_ptr<Point> point) const
{
    //判断某个节点是否在列表中，这里不能比较指针，因为每次加入列表是新开辟的节点，只能比较坐标
    for (auto p : list)
        if (p->x == point->x && p->y == point->y)
            return p;
    return NULL;
}

/* 曼哈顿距离计算G */
int Astar::calcG(shared_ptr<Point> temp_point, shared_ptr<Point> point)
{
    int extraG = ((abs(point->x - temp_point->x) + abs(point->y - temp_point->y)) == 1) ? kCost1 : kCost2;
    int parentG = point->parent == NULL ? 0 : point->parent->G;
    return extraG + parentG;
}

/* 欧几里的距离计算H */
int Astar::calcH(shared_ptr<Point> point, shared_ptr<Point> end)
{
    //用简单的欧几里得距离计算H，这个H的计算是关键，还有很多算法
    return sqrt((double)(end->x - point->x) * (double)(end->x - point->x) + (double)(end->y - point->y) * (double)(end->y - point->y)) * kCost1;
}

/* F = G + H */
int Astar::calcF(shared_ptr<Point> point)
{
    return point->G + point->H;
}
Point Astar::findone(shared_ptr<Point> point, Point end) {
    openList.clear();
    closeList.clear();
    shared_ptr<Point> newend = shared_ptr<Point>(new Point(end));
    auto surroundingpoints=getSurroundPoints(point,true);
    for (auto& target : surroundingpoints) {
        target->G = calcG(point, target);

        target->H = calcH(target, newend);

        target->F = calcF(target);
    }
    Point temp;
    for (auto& target : surroundingpoints) {
        if (target->F < temp.F) {
            temp = *target;
        }
    }
    return temp;
}
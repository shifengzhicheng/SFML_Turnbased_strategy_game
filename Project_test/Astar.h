#pragma once
#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include "Point.h"
using namespace std;

class Astar
{
private:
    vector<vector<int> >       maze;
    list<shared_ptr<Point> >   openList;
    list<shared_ptr<Point> >   closeList;
    shared_ptr<Point>          findPath(Point startPoint, Point endPoint, bool isIgnoreCorner);
    shared_ptr<Point>          isInList(list<shared_ptr<Point> > list, shared_ptr<Point> point) const;
public:
    Astar();
    Astar(vector<vector<int> > &_maze);
    vector<vector<int> >       emptymaze;
    void                       setMaze(vector<vector<int> > &_maze);
    list<shared_ptr<Point> >   GetPath(Point startPoint, Point endPoint, bool isIgnoreCorner);
    list<shared_ptr<Point> >   getSearchPath();
    vector<shared_ptr<Point> > getSurroundPoints(shared_ptr<Point> point, bool isIgnoreCorner) const;
    shared_ptr<Point>          getLeastFpoint();
    bool                       isCanReach(shared_ptr<Point> point, shared_ptr<Point> target, bool isIgnoreCorner) const;
    int                        calcG(shared_ptr<Point> temp_start, shared_ptr<Point> point);
    int                        calcH(shared_ptr<Point> point, shared_ptr<Point> end);
    int                        calcF(shared_ptr<Point> point);
    Point findone(shared_ptr<Point> point, Point end);
};

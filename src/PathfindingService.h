#pragma once

#include "Point.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct PathRequest
{
    int requestId = 0;
    int generation = 0;
    int ownerId = 0;
    Point start;
    Point goal;
    bool allowDiagonal = false;
    std::vector<std::vector<int>> maze;
};

struct PathResult
{
    int requestId = 0;
    int generation = 0;
    int ownerId = 0;
    Point start;
    Point goal;
    std::deque<Point> path;
};

class PathfindingService
{
public:
    PathfindingService();
    ~PathfindingService();

    PathfindingService(const PathfindingService&) = delete;
    PathfindingService& operator=(const PathfindingService&) = delete;

    void submit(PathRequest request);
    std::vector<PathResult> collectResults();

private:
    void workerLoop();

    std::mutex mutex;
    std::condition_variable cv;
    bool stopping = false;
    std::queue<PathRequest> requests;
    std::vector<PathResult> results;
    std::thread worker;
};

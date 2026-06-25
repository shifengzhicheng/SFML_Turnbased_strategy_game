#include "PathfindingService.h"

#include "Astar.h"

#include <utility>

PathfindingService::PathfindingService() :
    worker(&PathfindingService::workerLoop, this)
{
}

PathfindingService::~PathfindingService()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopping = true;
    }
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}

void PathfindingService::submit(PathRequest request)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        requests.push(std::move(request));
    }
    cv.notify_one();
}

std::vector<PathResult> PathfindingService::collectResults()
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<PathResult> out;
    out.swap(results);
    return out;
}

void PathfindingService::workerLoop()
{
    while (true) {
        PathRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return stopping || !requests.empty(); });
            if (stopping && requests.empty()) {
                return;
            }
            request = std::move(requests.front());
            requests.pop();
        }

        Astar astar(request.maze);
        PathResult result;
        result.requestId = request.requestId;
        result.generation = request.generation;
        result.ownerId = request.ownerId;
        result.start = request.start;
        result.goal = request.goal;
        result.path = astar.GetPath(request.start, request.goal, request.allowDiagonal);
        if (!result.path.empty()) {
            result.path.pop_front();
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            results.push_back(std::move(result));
        }
    }
}

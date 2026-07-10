#include "PathfindingService.h"

#include "Astar.h"

#include <algorithm>
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
        requests.clear();
        results.clear();
    }
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}

void PathfindingService::setExecutionMode(ExecutionMode mode)
{
    std::lock_guard<std::mutex> lock(mutex);
    executionMode = mode;
    requests.clear();
    results.clear();
}

void PathfindingService::clearPending()
{
    std::lock_guard<std::mutex> lock(mutex);
    requests.clear();
    results.clear();
}

void PathfindingService::submit(PathRequest request)
{
    bool solveImmediately = false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping) {
            return;
        }
        solveImmediately = executionMode == ExecutionMode::Synchronous;
        if (!solveImmediately) {
            // Only the newest route for an entity can still be useful. This
            // bounds the queue when combat repeatedly changes tactical goals.
            requests.erase(std::remove_if(requests.begin(), requests.end(), [&request](const PathRequest& pending) {
                return pending.generation == request.generation && pending.ownerId == request.ownerId;
            }), requests.end());
            requests.push_back(std::move(request));
        }
    }

    if (solveImmediately) {
        PathResult result = solve(std::move(request));
        std::lock_guard<std::mutex> lock(mutex);
        if (!stopping) {
            results.push_back(std::move(result));
        }
        return;
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

PathResult PathfindingService::solve(PathRequest request)
{
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
    return result;
}

void PathfindingService::workerLoop()
{
    while (true) {
        PathRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return stopping || !requests.empty(); });
            if (stopping) {
                return;
            }
            request = std::move(requests.front());
            requests.pop_front();
        }

        PathResult result = solve(std::move(request));

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!stopping) {
                results.push_back(std::move(result));
            }
        }
    }
}

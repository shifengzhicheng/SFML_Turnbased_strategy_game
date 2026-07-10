#include "PathfindingService.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) {
            std::cerr << "pathfinding_service_tests: " << message << '\n';
            std::exit(1);
        }
    }

    PathRequest openGridRequest(int requestId, Point start, Point goal)
    {
        PathRequest request;
        request.requestId = requestId;
        request.generation = 7;
        request.ownerId = 42;
        request.start = start;
        request.goal = goal;
        request.maze.assign(5, std::vector<int>(7, 0));
        return request;
    }
}

int main()
{
    PathfindingService service;
    service.setExecutionMode(PathfindingService::ExecutionMode::Synchronous);

    service.submit(openGridRequest(1, Point(1, 2), Point(5, 2)));
    auto results = service.collectResults();
    require(results.size() == 1, "synchronous mode should finish before submit returns");
    require(results.front().requestId == 1 && results.front().ownerId == 42,
            "result identity should match the request");
    require(results.front().start.x == 1 && results.front().start.y == 2,
            "result should preserve the request start for stale-path trimming");
    require(results.front().path.size() == 4,
            "returned path should omit the occupied starting tile");
    require(results.front().path.front().x == 2 && results.front().path.back().x == 5,
            "path should connect the first legal step to the goal");

    service.submit(openGridRequest(2, Point(1, 1), Point(5, 1)));
    service.clearPending();
    require(service.collectResults().empty(), "clearPending should discard completed and queued stale work");

    service.setExecutionMode(PathfindingService::ExecutionMode::Asynchronous);
    service.submit(openGridRequest(3, Point(1, 3), Point(5, 3)));
    service.clearPending();

    return 0;
}

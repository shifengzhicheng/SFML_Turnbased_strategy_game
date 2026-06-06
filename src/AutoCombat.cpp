#include "AutoCombat.h"

#include "AllUnit.h"
#include "Game.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    int distanceSquared(const Unit& a, const Unit& b)
    {
        return distanceSquared(Point(a.x, a.y), Point(b.x, b.y));
    }

    Unit* betterTarget(MoveableUnit& unit, Unit* current, Unit* candidate)
    {
        if (candidate == nullptr || candidate->Health <= 0 || candidate->myteam == unit.myteam) {
            return current;
        }
        if (current == nullptr) {
            return candidate;
        }

        const bool candidateInRange = unit.canAutoAttack(candidate);
        const bool currentInRange = unit.canAutoAttack(current);
        if (candidateInRange != currentInRange) {
            return candidateInRange ? candidate : current;
        }

        return distanceSquared(unit, *candidate) < distanceSquared(unit, *current) ? candidate : current;
    }

    Unit* chooseTarget(Game& game, MoveableUnit& unit)
    {
        Unit* best = nullptr;
        if (unit.myteam == PLAYER) {
            for (auto& enemy : game.enemys) {
                best = betterTarget(unit, best, enemy.get());
            }
            best = betterTarget(unit, best, game.Base_blue.get());
        }
        else {
            for (auto& playerUnit : game.myunits) {
                best = betterTarget(unit, best, playerUnit.get());
            }
            best = betterTarget(unit, best, game.Base_red.get());
        }
        return best;
    }

    Point fallbackEnemyBase(Game& game, MoveableUnit& unit)
    {
        return unit.myteam == PLAYER ? game.Blue_baseP : game.Red_baseP;
    }

    Point chooseApproachPoint(Game& game, MoveableUnit& unit, Unit* target)
    {
        const Point targetPoint = target != nullptr ? Point(target->x, target->y) : fallbackEnemyBase(game, unit);
        if (game.isCellWalkableForUnit(targetPoint.x, targetPoint.y)) {
            return targetPoint;
        }

        Point best = Point(unit.x, unit.y);
        int bestScore = std::numeric_limits<int>::max();
        const int searchRadius = std::max(2, std::min(unit.myAttackRange(), 6));
        for (int radius = 1; radius <= searchRadius; ++radius) {
            for (int y = targetPoint.y - radius; y <= targetPoint.y + radius; ++y) {
                for (int x = targetPoint.x - radius; x <= targetPoint.x + radius; ++x) {
                    if (!game.isCellWalkableForUnit(x, y)) {
                        continue;
                    }
                    const int toTarget = distanceSquared(Point(x, y), targetPoint);
                    if (toTarget > unit.myAttackRange() * unit.myAttackRange()) {
                        continue;
                    }
                    const int score = distanceSquared(Point(unit.x, unit.y), Point(x, y));
                    if (score < bestScore) {
                        bestScore = score;
                        best = Point(x, y);
                    }
                }
            }
            if (bestScore != std::numeric_limits<int>::max()) {
                return best;
            }
        }
        return best;
    }

    void refreshPathIfNeeded(Game& game, MoveableUnit& unit, Point goal)
    {
        if (unit.realtimePathTimer < realtime::PathRefreshSeconds && !unit.mypath.empty()) {
            return;
        }
        if (!game.isCellWalkableForUnit(goal.x, goal.y)) {
            return;
        }
        if (unit.pendingPathRequest != 0
            && unit.pendingPathGoal.x == goal.x
            && unit.pendingPathGoal.y == goal.y) {
            return;
        }

        unit.realtimePathTimer = 0.f;
        // A* runs on the pathfinding service; the main thread only applies the
        // returned path so SFML rendering/window access stays single-threaded.
        game.requestPathForUnit(unit, goal);
    }

    void updateUnit(Game& game, MoveableUnit& unit, float dt)
    {
        if (unit.Health <= 0) {
            return;
        }

        unit.realtimeAttackTimer += dt;
        unit.realtimeMoveTimer += dt;
        unit.realtimePathTimer += dt;

        Unit* target = chooseTarget(game, unit);
        if (target != nullptr && unit.canAutoAttack(target)) {
            unit.UnitState = UState::UNITNORMAL;
            unit.mypath.clear();
            unit.pendingPathRequest = 0;
            if (unit.realtimeAttackTimer >= unit.realtimeAttackCooldownSeconds()) {
                unit.realtimeAttackTimer = 0.f;
                unit.autoAttack(target);
            }
            return;
        }

        Building* buildingTarget = game.chooseBuildingTarget(unit);
        if (buildingTarget != nullptr && game.canAttackBuilding(unit, *buildingTarget)) {
            unit.UnitState = UState::UNITNORMAL;
            unit.mypath.clear();
            unit.pendingPathRequest = 0;
            if (unit.realtimeAttackTimer >= unit.realtimeAttackCooldownSeconds()) {
                unit.realtimeAttackTimer = 0.f;
                game.autoAttackBuilding(unit, *buildingTarget);
            }
            return;
        }

        if (unit.realtimeMoveTimer < unit.realtimeMoveStepSeconds()) {
            return;
        }
        unit.realtimeMoveTimer -= unit.realtimeMoveStepSeconds();

        const Point rallyPoint = game.chooseStrategicRallyPoint(unit);
        const Point goal = buildingTarget != nullptr
            ? game.findBuildStandPoint(*buildingTarget)
            : (rallyPoint.x >= 0 ? rallyPoint : chooseApproachPoint(game, unit, target));
        refreshPathIfNeeded(game, unit, goal);
        if (unit.mypath.empty()) {
            unit.UnitState = UState::UNITNORMAL;
            return;
        }

        const Point next = unit.mypath.front();
        unit.mypath.pop_front();
        if (game.isCellWalkableForUnit(next.x, next.y)) {
            unit.UnitState = UState::MOVING;
            unit.move(next);
        }
        else {
            unit.mypath.clear();
        }
    }
}

namespace realtime
{
    void updateAutoCombat(Game& game, float dt)
    {
        for (auto& unit : game.myunits) {
            updateUnit(game, *unit, dt);
        }
        for (auto& unit : game.enemys) {
            updateUnit(game, *unit, dt);
        }
    }
}

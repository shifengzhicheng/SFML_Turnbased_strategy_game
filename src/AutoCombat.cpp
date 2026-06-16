#include "AutoCombat.h"

#include "AllUnit.h"
#include "Game.h"
#include "RealtimeConfig.h"
#include "UnitGeometry.h"

#include <algorithm>
#include <array>
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
        if (unit.aggroSeconds > 0.f && unit.aggroTargetId != 0) {
            MoveableUnit* attacker = game.findMoveableUnitById(unit.aggroTargetId);
            if (attacker != nullptr && attacker->Health > 0 && attacker->myteam != unit.myteam) {
                return attacker;
            }
            unit.clearAggro();
        }

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
        if (target != nullptr && game.isCellWalkableForUnit(targetPoint.x, targetPoint.y)) {
            return targetPoint;
        }

        Point best = Point(unit.x, unit.y);
        int bestScore = std::numeric_limits<int>::max();
        const int footprintPadding = target != nullptr ? unit_geometry::footprintSize(*target) - 1 : 0;
        const int searchRadius = std::max(2, std::min(unit.myAttackRange() + footprintPadding, 12));
        for (int radius = 1; radius <= searchRadius; ++radius) {
            for (int y = targetPoint.y - radius; y <= targetPoint.y + radius; ++y) {
                for (int x = targetPoint.x - radius; x <= targetPoint.x + radius; ++x) {
                    if (!game.isCellWalkableForUnit(x, y)) {
                        continue;
                    }
                    if (target != nullptr && !unit_geometry::isInAttackRange(Point(x, y), *target, unit.myAttackRange())) {
                        continue;
                    }
                    if (target == nullptr && distanceSquared(Point(x, y), targetPoint) > unit.myAttackRange() * unit.myAttackRange()) {
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

    bool tryMove(Game& game, MoveableUnit& unit, Point next)
    {
        if (!game.canUnitStepInto(unit, next)) {
            return false;
        }

        unit.UnitState = UState::MOVING;
        unit.move(next);
        return true;
    }

    Point chooseAlternateStep(Game& game, MoveableUnit& unit, Point goal)
    {
        const Point current(unit.x, unit.y);
        const int currentScore = distanceSquared(current, goal);
        const std::array<Point, 4> candidates = {{
            Point(current.x + 1, current.y),
            Point(current.x - 1, current.y),
            Point(current.x, current.y + 1),
            Point(current.x, current.y - 1),
        }};

        Point best(-1, -1);
        int bestScore = std::numeric_limits<int>::max();
        for (const Point candidate : candidates) {
            if (!game.canUnitStepInto(unit, candidate)) {
                continue;
            }

            const int score = distanceSquared(candidate, goal);
            if (score > currentScore + 2) {
                continue;
            }
            if (score < bestScore) {
                bestScore = score;
                best = candidate;
            }
        }
        return best;
    }

    void updateUnit(Game& game, MoveableUnit& unit, float dt)
    {
        if (unit.Health <= 0) {
            return;
        }

        unit.realtimeAttackTimer += dt;
        unit.realtimeMoveTimer += dt;
        unit.realtimePathTimer += dt;
        if (unit.aggroSeconds > 0.f) {
            unit.aggroSeconds = std::max(0.f, unit.aggroSeconds - dt);
            if (unit.aggroSeconds == 0.f) {
                unit.clearAggro();
            }
        }

        Unit* target = chooseTarget(game, unit);
        if (target != nullptr && unit.canAutoAttack(target)) {
            unit.UnitState = UState::UNITNORMAL;
            unit.mypath.clear();
            unit.pendingPathRequest = 0;
            // Do not bank movement time while attacking; otherwise a unit can
            // sprint through several frames immediately after its target dies.
            unit.realtimeMoveTimer = 0.f;
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
            unit.realtimeMoveTimer = 0.f;
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
        const Point goal = rallyPoint.x >= 0
            ? rallyPoint
            : (buildingTarget != nullptr ? game.findAttackStandPoint(unit, *buildingTarget) : chooseApproachPoint(game, unit, target));
        refreshPathIfNeeded(game, unit, goal);
        if (unit.mypath.empty()) {
            unit.UnitState = UState::UNITNORMAL;
            return;
        }

        const Point next = unit.mypath.front();
        unit.mypath.pop_front();
        if (tryMove(game, unit, next)) {
            return;
        }

        const Point alternate = chooseAlternateStep(game, unit, goal);
        if (alternate.x >= 0 && tryMove(game, unit, alternate)) {
            // The previous path became invalid because terrain/buildings
            // changed. Step aside once, then force a fresh path next tick.
            unit.mypath.clear();
            unit.realtimePathTimer = realtime::PathRefreshSeconds;
        }
        else {
            unit.mypath.clear();
            unit.realtimePathTimer = realtime::PathRefreshSeconds;
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

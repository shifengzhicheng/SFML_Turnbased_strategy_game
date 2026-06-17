#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "PerkMechanics.h"
#include "UnitDefinition.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::updateDefenseTowers(float dt)
{
    const auto distanceSquared = [](Point a, Point b) {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    };
    for (auto& building : buildings) {
        if (!building.complete || building.type != building::DefenseTower) {
            continue;
        }
        const int range = defenseTowerRange(building.team);
        const int rangeSquared = range * range;
        const TowerMechanics towerMechanics = towerMechanicsFor(building.team == PLAYER ? playerPerkLevels : aiPerkLevels);

        building.attackTimer += dt;
        if (building.attackTimer < realtime::DefenseTowerAttackCooldown) {
            continue;
        }

        Unit* target = nullptr;
        int bestScore = std::numeric_limits<int>::max();
        auto considerTarget = [&](Unit* candidate) {
            if (candidate == nullptr || candidate->Health <= 0 || candidate->myteam == building.team) {
                return;
            }
            const int dist = distanceSquared(building.point, Point(candidate->x, candidate->y));
            if (dist > rangeSquared) {
                return;
            }
            int score = dist * 10;
            if (candidate->unitName == UName::SIEGE) {
                score -= towerMechanics.prioritizesSiege ? 420 : 260;
            }
            if (candidate->unitName == UName::CAVALRY) {
                score -= 80;
            }
            if (score < bestScore) {
                bestScore = score;
                target = candidate;
            }
        };

        if (building.team == PLAYER) {
            for (auto& enemy : enemys) {
                considerTarget(enemy.get());
            }
        }
        else {
            for (auto& playerUnit : myunits) {
                considerTarget(playerUnit.get());
            }
        }

        if (target == nullptr) {
            continue;
        }

        building.attackTimer = 0.f;
        const float towerMultiplier = damageMultiplier(building.team);
        const int targetMaxHealth = target->unitName == UName::BASE ? 4000
            : (isTrainableUnit(target->unitName)
                ? static_cast<int>(std::round(static_cast<float>(unitDefinition(target->unitName).maxHealth)
                    * unitHealthMultiplier(target->myteam, target->unitName)))
                : std::max(1, target->Health));
        const int flatDamage = std::max(1, static_cast<int>(std::round(static_cast<float>(config::DefenseTowerDamage) * towerMultiplier)));
        const int percentDamage = std::max(1, static_cast<int>(std::round(
            static_cast<float>(targetMaxHealth) * towerMechanics.maxHealthDamagePercent)));
        const int damage = flatDamage + percentDamage;
        target->Health -= damage;

        const sf::Vector2f towerCenter(
            building.point.x * SqureSize + SqureSize / 2.f,
            building.point.y * SqureSize + SqureSize / 2.f);
        addAttackEffect(towerCenter, unitCenter(*target), building.team == PLAYER ? sf::Color(255, 211, 84) : sf::Color(118, 178, 255));
        target->playFlash(sf::Color(255, 146, 112, 255), 0.18f);
        addFloatingText(unitCenter(*target) + sf::Vector2f(0.f, -14.f),
                        "-" + std::to_string(damage), sf::Color(255, 218, 112), 13);
    }
}

void Game::updateBaseDefenses(float dt)
{
    const auto fireFromBase = [this, dt](DisMoveableUnit* base, int team, float& timer) {
        if (base == nullptr || base->Health <= 0) {
            return;
        }

        timer += dt;
        const float cooldown = realtime::DefenseTowerAttackCooldown * 0.86f;
        if (timer < cooldown) {
            return;
        }

        Unit* target = nullptr;
        int bestScore = std::numeric_limits<int>::max();
        const Point basePoint(base->x, base->y);
        const int rangeSquared = config::BaseDefenseRange * config::BaseDefenseRange;
        auto considerTarget = [&](Unit* candidate) {
            if (candidate == nullptr || candidate->Health <= 0 || candidate->myteam == team) {
                return;
            }
            const int score = distanceSquared(basePoint, Point(candidate->x, candidate->y));
            if (score <= rangeSquared && score < bestScore) {
                bestScore = score;
                target = candidate;
            }
        };

        if (team == PLAYER) {
            for (auto& enemy : enemys) {
                considerTarget(enemy.get());
            }
        }
        else {
            for (auto& playerUnit : myunits) {
                considerTarget(playerUnit.get());
            }
        }
        if (target == nullptr) {
            return;
        }

        // A modest HQ laser prevents early cavalry blobs from deleting the
        // player's whole production line, while siege pushes still break bases.
        timer = 0.f;
        const int damage = std::max(1, static_cast<int>(std::round(
            static_cast<float>(config::BaseDefenseDamage) * damageMultiplier(team))));
        target->Health -= damage;
        const sf::Vector2f origin(base->x * SqureSize + SqureSize, base->y * SqureSize + SqureSize);
        addAttackEffect(origin, unitCenter(*target), team == PLAYER ? sf::Color(255, 231, 142) : sf::Color(126, 184, 255));
        target->playFlash(sf::Color(255, 170, 105, 255), 0.16f);
    };

    fireFromBase(Base_red.get(), PLAYER, playerBaseAttackTimer);
    fireFromBase(Base_blue.get(), AI, aiBaseAttackTimer);
}

Building* Game::chooseBuildingTarget(MoveableUnit& unit)
{
    Building* best = nullptr;
    int bestScore = std::numeric_limits<int>::max();
    const Point current(unit.x, unit.y);
    for (auto& building : buildings) {
        if (building.team == unit.myteam || building.health <= 0 || !building.complete) {
            continue;
        }
        int priority = 400;
        if (building.type == building::DefenseTower) {
            priority = 0;
        }
        else if (building.type == building::Barracks) {
            priority = 220;
        }

        const Point laneMid = laneWaypoint(unit.myteam, unit.laneIndex, 1);
        const int lanePenalty = std::abs(building.point.y - laneMid.y) * 12;
        const int score = priority + lanePenalty + distanceSquared(current, building.point);
        if (score < bestScore) {
            bestScore = score;
            best = &building;
        }
    }
    return best;
}

Point Game::chooseStrategicRallyPoint(MoveableUnit& unit)
{
    const Point current(unit.x, unit.y);
    const int mapW = width / SqureSize;
    const int enemyTeam = unit.myteam == PLAYER ? AI : PLAYER;
    const bool enemyProductionBroken = gameTimeSeconds > 420.f
        && completedBuildingCount(enemyTeam, building::Barracks) == 0
        && completedBuildingCount(enemyTeam, building::DefenseTower) == 0;
    const bool onEnemyHalf = unit.myteam == PLAYER ? current.x > mapW / 2 : current.x < mapW / 2;
    if (enemyProductionBroken && onEnemyHalf) {
        unit.nextRallyStage = 3;
        return Point(-1, -1);
    }

    // Rally progress is one-way per unit. Early lane gates require both forward
    // progress and lane alignment so fresh units do not skip the Top/Bot exit
    // anchor just because their barracks spawned slightly ahead of it. The
    // final gate stays x-based: once a unit has crossed the enemy-side anchor,
    // it should commit to the base instead of walking backward after a detour.
    while (unit.nextRallyStage <= 2) {
        const int stage = unit.nextRallyStage;
        const Point laneGoal = laneWaypoint(unit.myteam, unit.laneIndex, stage);
        const bool crossedStage = unit.myteam == PLAYER
            ? current.x >= laneGoal.x - 1
            : current.x <= laneGoal.x + 1;
        const bool alignedWithGate = std::abs(current.y - laneGoal.y) <= (stage == 0 ? 4 : 3);
        const bool passedStage = crossedStage && (stage == 2 || alignedWithGate);
        if (nearPoint(current, laneGoal, stage == 2 ? 2 : 3) || passedStage) {
            ++unit.nextRallyStage;
            continue;
        }
        if (isCellWalkableForUnit(laneGoal.x, laneGoal.y)) {
            return laneGoal;
        }
        ++unit.nextRallyStage;
    }

    return Point(-1, -1);
}

bool Game::canAttackBuilding(const MoveableUnit& unit, const Building& building) const
{
    if (building.team == unit.myteam || building.health <= 0 || !building.complete) {
        return false;
    }
    const int range = std::max(1, unit.myAttackRange());
    return distanceSquared(Point(unit.x, unit.y), building.point) <= range * range;
}

void Game::autoAttackBuilding(MoveableUnit& unit, Building& building)
{
    if (!canAttackBuilding(unit, building)) {
        return;
    }

    float typeFactor = 1.f;
    if (unit.unitName == UName::CAVALRY) {
        typeFactor = 1.15f;
    }
    else if (unit.unitName == UName::SHOOTER) {
        typeFactor = 0.82f;
    }
    else if (unit.unitName == UName::SIEGE) {
        typeFactor = 2.18f * unitBuildingDamageMultiplier(unit.myteam, unit.unitName);
    }
    else if (unit.unitName == UName::GUARDIAN) {
        typeFactor = 1.24f;
    }
    const int damage = std::max(1, static_cast<int>(std::round(static_cast<float>(unit.myattack())
        * unitDamageMultiplier(unit.myteam, unit.unitName) * config::BuildingDamageFactor * typeFactor)));
    building.health -= damage;

    const sf::Vector2f origin(building.point.x * SqureSize + SqureSize / 2.f, building.point.y * SqureSize + SqureSize / 2.f);
    const sf::Vector2f attackVector = origin - unitCenter(unit);
    unit.playFlash(sf::Color(255, 245, 180, 255), 0.14f);
    unit.playAction(attackVector, 0.16f);
    addAttackEffect(unitCenter(unit), origin, unit.myteam == PLAYER ? sf::Color(255, 211, 84) : sf::Color(118, 178, 255));
    addFloatingText(origin + sf::Vector2f(0.f, -14.f), "-" + std::to_string(damage), sf::Color(255, 218, 112), 12);
}

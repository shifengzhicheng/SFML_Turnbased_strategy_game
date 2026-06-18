#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "LaneGeometry.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <functional>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    constexpr int BlockedSiteScore = 1000000;

    int teamDirection(int team)
    {
        return team == PLAYER ? 1 : -1;
    }

    Point teamBase(const Game& game, int team)
    {
        return team == PLAYER ? game.Red_baseP : game.Blue_baseP;
    }

    int openNeighborCount(const Game& game, Point point)
    {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const int x = point.x + dx;
                const int y = point.y + dy;
                if (game.isCellWalkableForUnit(x, y) && !game.isCellReservedForSpawn(x, y)) {
                    ++count;
                }
            }
        }
        return count;
    }

    int buildingSpacingPenalty(const Game& game, int team, int type, Point candidate,
                               int minSpacing, int preferredSpacing)
    {
        int penalty = 0;
        for (const auto& existing : game.buildings) {
            if (existing.team != team) {
                continue;
            }
            const int deltaX = std::abs(candidate.x - existing.point.x);
            const int deltaY = std::abs(candidate.y - existing.point.y);
            const int chebyshev = std::max(deltaX, deltaY);
            if (existing.type == type && chebyshev < minSpacing) {
                return BlockedSiteScore;
            }
            if (existing.type == type && chebyshev < preferredSpacing) {
                penalty += 420;
            }
            if (existing.type != type && chebyshev < 2) {
                penalty += 260;
            }
        }
        return penalty;
    }

    int frontDistanceFromBase(const Game& game, int team, Point candidate)
    {
        const Point base = teamBase(game, team);
        // Bases use a 2x2 footprint; measure from the forward edge so mirrored
        // build pockets score identically for player and AI.
        const int frontEdgeX = team == PLAYER ? base.x + 1 : base.x;
        return (candidate.x - frontEdgeX) * teamDirection(team);
    }

    int scoreBarracksSite(const Game& game, int team, int laneIndex, Point anchor, Point candidate)
    {
        if (!game.isBuildableCell(candidate.x, candidate.y)
            || !game.isBuildSiteInInfluence(team, candidate, building::Barracks)) {
            return BlockedSiteScore;
        }

        int score = distanceSquared(candidate, anchor) * 18;
        const int openNeighbors = openNeighborCount(game, candidate);
        if (openNeighbors < 2) {
            return BlockedSiteScore;
        }
        if (openNeighbors < 4) {
            score += (4 - openNeighbors) * 90;
        }

        const int front = frontDistanceFromBase(game, team, candidate);
        if (front < 1 || front > 7) {
            score += 650 + std::abs(front - 1) * 70;
        }
        score += std::abs(front - 1) * 14;

        const int mapW = config::MapTilesX;
        const int mapH = config::MapTilesY;
        const int laneY = static_cast<int>(std::round(lane_geometry::laneYAtX(mapW, mapH, laneIndex, candidate.x)));
        if (std::abs(candidate.y - laneY) == 0) {
            score += 700;
        }

        const int spacingPenalty = buildingSpacingPenalty(game, team, building::Barracks, candidate, 2, 3);
        if (spacingPenalty >= BlockedSiteScore) {
            return BlockedSiteScore;
        }
        return score + spacingPenalty;
    }

    int towerLaneSidePreference(const Game& game, int team, int laneIndex)
    {
        if (laneIndex == lane::Top) {
            return -1;
        }
        if (laneIndex == lane::Bot) {
            return 1;
        }

        int sameLaneTowers = 0;
        const int mapW = config::MapTilesX;
        const int mapH = config::MapTilesY;
        for (const auto& building : game.buildings) {
            if (building.team != team || building.type != building::DefenseTower) {
                continue;
            }
            const int laneY = static_cast<int>(std::round(lane_geometry::laneYAtX(mapW, mapH, laneIndex, building.point.x)));
            if (std::abs(building.point.y - laneY) <= 3) {
                ++sameLaneTowers;
            }
        }
        return sameLaneTowers % 2 == 0 ? -1 : 1;
    }

    int scoreTowerSite(const Game& game, int team, int laneIndex, Point anchor, Point candidate)
    {
        if (!game.isBuildableCell(candidate.x, candidate.y)
            || !game.isBuildSiteInInfluence(team, candidate, building::DefenseTower)) {
            return BlockedSiteScore;
        }

        int score = distanceSquared(candidate, anchor) * 14;
        const int openNeighbors = openNeighborCount(game, candidate);
        if (openNeighbors < 2) {
            return BlockedSiteScore;
        }
        if (openNeighbors < 4) {
            score += (4 - openNeighbors) * 70;
        }

        const int mapW = config::MapTilesX;
        const int mapH = config::MapTilesY;
        const int laneY = static_cast<int>(std::round(lane_geometry::laneYAtX(mapW, mapH, laneIndex, candidate.x)));
        const int shoulderGap = std::abs(candidate.y - laneY);
        if (shoulderGap == 0) {
            score += 340;
        }
        score += std::abs(shoulderGap - 1) * 26;

        const int desiredSide = towerLaneSidePreference(game, team, laneIndex);
        const int actualSide = candidate.y == laneY ? 0 : (candidate.y > laneY ? 1 : -1);
        if (actualSide != 0 && actualSide != desiredSide) {
            score += 110;
        }

        const int front = frontDistanceFromBase(game, team, candidate);
        if (front < 3) {
            score += 520 + (3 - front) * 80;
        }
        if (front > 11) {
            score += 360 + (front - 11) * 80;
        }
        score += std::abs(front - 5) * 12;

        const Point coverage = game.laneRallyPoint(team, laneIndex, 1);
        const int range = game.defenseTowerRange(team);
        const int rangeSquared = range * range;
        const int coverageDistance = distanceSquared(candidate, coverage);
        if (coverageDistance > rangeSquared) {
            score += 1000 + (coverageDistance - rangeSquared) * 8;
        }
        if (!game.hasLineOfSightForTower(candidate, coverage, team)) {
            score += 900;
        }

        const int spacingPenalty = buildingSpacingPenalty(game, team, building::DefenseTower, candidate,
                                                          config::TowerPlacementMinSpacing,
                                                          config::TowerPlacementPreferredSpacing);
        if (spacingPenalty >= BlockedSiteScore) {
            return BlockedSiteScore;
        }

        int batteryPenalty = 0;
        for (const auto& existing : game.buildings) {
            if (existing.team != team || existing.type != building::DefenseTower) {
                continue;
            }
            const int deltaX = std::abs(candidate.x - existing.point.x);
            const int deltaY = std::abs(candidate.y - existing.point.y);
            if ((deltaX == 0 || deltaY == 0 || deltaX == deltaY)
                && deltaX + deltaY <= config::DefenseTowerRange) {
                batteryPenalty += 160;
            }
        }

        return score + spacingPenalty + batteryPenalty;
    }

    Point bestScoredSiteNear(const Game& game, Point center, int maxRadius,
                             const std::function<int(Point)>& scoreCandidate)
    {
        Point best(-1, -1);
        int bestScore = BlockedSiteScore;
        for (int radius = 0; radius <= maxRadius; ++radius) {
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                        continue;
                    }
                    const Point candidate(center.x + dx, center.y + dy);
                    const int score = scoreCandidate(candidate);
                    if (score < bestScore) {
                        bestScore = score;
                        best = candidate;
                    }
                }
            }
        }
        return best;
    }
}

bool Game::requestBuildBarracks(int team, Point point)
{
    const int cost = buildingCommandCost(building::Barracks);
    if (totalBuildingCount(team, building::Barracks) >= buildingCap(team, building::Barracks)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tech for more Rax", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::Barracks)
        || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < cost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::Barracks) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    Building building;
    building.id = nextEntityId++;
    building.team = team;
    building.type = building::Barracks;
    building.point = point;
    building.buildSeconds = buildingSeconds(building.type);
    building.maxHealth = buildingMaxHealth(building.type);
    building.health = building.maxHealth;
    buildings.push_back(building);
    setTileID(point.x, point.y, buildingTileId(team, building.type));
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                        "Barracks queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued barracks id=" + std::to_string(building.id));
    return true;
}

bool Game::requestBuildTower(int team, Point point)
{
    const int cost = buildingCommandCost(building::DefenseTower);
    if (totalBuildingCount(team, building::DefenseTower) >= buildingCap(team, building::DefenseTower)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tower cap", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::DefenseTower)
        || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < cost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::DefenseTower) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    Building building;
    building.id = nextEntityId++;
    building.team = team;
    building.type = building::DefenseTower;
    building.point = point;
    building.buildSeconds = buildingSeconds(building.type);
    building.maxHealth = buildingMaxHealth(building.type);
    building.health = building.maxHealth;
    buildings.push_back(building);
    setTileID(point.x, point.y, buildingTileId(team, building.type));
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                        "Tower queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued tower id=" + std::to_string(building.id));
    return true;
}

Point Game::findAutoBuildSite(int team, int type, int laneIndex) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const int safeLane = std::clamp(laneIndex, 0, lane::Count - 1);
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const auto protectedBarracksAnchor = [this, team, safeLane, mapW, mapH]() {
        const int laneOffset = safeLane == lane::Top ? -4 : (safeLane == lane::Bot ? 4 : -1);
        const int playerAnchorX = std::clamp(Red_baseP.x + 2, 2, mapW - 3);
        const int anchorX = team == PLAYER ? playerAnchorX : mapW - 1 - playerAnchorX;
        const int anchorY = std::clamp(Red_baseP.y + laneOffset, 2, mapH - 3);
        return Point(anchorX, anchorY);
    };
    const Point anchor = type == building::DefenseTower
        ? laneDefensePoint(team, safeLane)
        : protectedBarracksAnchor();

    // Barracks are production lifelines, so they stay in the protected base
    // pocket. Towers use lane shoulders and must actually cover the approach.
    const Point primarySite = type == building::DefenseTower
        ? bestScoredSiteNear(*this, anchor, 10, [this, team, safeLane, anchor](Point candidate) {
              return scoreTowerSite(*this, team, safeLane, anchor, candidate);
          })
        : bestScoredSiteNear(*this, anchor, 7, [this, team, safeLane, anchor](Point candidate) {
              return scoreBarracksSite(*this, team, safeLane, anchor, candidate);
          });
    if (primarySite.x >= 0) {
        return primarySite;
    }

    const Point baseSite = type == building::DefenseTower
        ? bestScoredSiteNear(*this, base, 12, [this, team, safeLane, anchor](Point candidate) {
              return scoreTowerSite(*this, team, safeLane, anchor, candidate);
          })
        : bestScoredSiteNear(*this, base, 12, [this, team, safeLane, anchor](Point candidate) {
              return scoreBarracksSite(*this, team, safeLane, anchor, candidate);
          });
    if (baseSite.x >= 0) {
        return baseSite;
    }

    for (int radius = 1; radius <= 12; ++radius) {
        for (int y = base.y - radius; y <= base.y + radius; ++y) {
            for (int x = base.x - radius; x <= base.x + radius; ++x) {
                const Point candidate(x, y);
                if (isBuildableCell(x, y) && isBuildSiteInInfluence(team, candidate, type)) {
                    return candidate;
                }
            }
        }
    }
    return Point(-1, -1);
}

bool Game::requestAutoBuildBarracks(int team)
{
    const Point site = findAutoBuildSite(team, building::Barracks, selectedLaneForTeam(team));
    if (site.x < 0) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildBarracksY) - 18.f),
                            "No build site", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    return requestBuildBarracks(team, site);
}

bool Game::requestAutoBuildTower(int team)
{
    const Point site = findAutoBuildSite(team, building::DefenseTower, selectedLaneForTeam(team));
    if (site.x < 0) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildTowerY) - 18.f),
                            "No build site", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    return requestBuildTower(team, site);
}

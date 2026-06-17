#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

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
        const int laneOffset = safeLane == lane::Top ? -3 : (safeLane == lane::Bot ? 4 : 1);
        const int playerAnchorX = std::clamp(Red_baseP.x + 3, 2, mapW - 3);
        const int anchorX = team == PLAYER ? playerAnchorX : mapW - 1 - playerAnchorX;
        const int anchorY = std::clamp(Red_baseP.y + laneOffset, 2, mapH - 3);
        return Point(anchorX, anchorY);
    };
    const Point anchor = type == building::DefenseTower
        ? laneDefensePoint(team, safeLane)
        : protectedBarracksAnchor();

    const int mirrorX = team == PLAYER ? 1 : -1;
    const auto findNearAnchor = [this, team, type, mirrorX](Point center, int maxRadius) {
        for (int radius = 0; radius <= maxRadius; ++radius) {
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                        continue;
                    }
                    const Point candidate(center.x + dx * mirrorX, center.y + dy);
                    if (isBuildableCell(candidate.x, candidate.y) && isBuildSiteInInfluence(team, candidate, type)) {
                        return candidate;
                    }
                }
            }
        }
        return Point(-1, -1);
    };
    const auto findTowerNearAnchor = [this, team, mirrorX](Point center, int maxRadius) {
        Point bestSpaced(-1, -1);
        int bestSpacedScore = std::numeric_limits<int>::max();
        Point bestRelaxed(-1, -1);
        int bestRelaxedScore = std::numeric_limits<int>::max();

        for (int radius = 0; radius <= maxRadius; ++radius) {
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                        continue;
                    }
                    const Point candidate(center.x + dx * mirrorX, center.y + dy);
                    if (!isBuildableCell(candidate.x, candidate.y)
                        || !isBuildSiteInInfluence(team, candidate, building::DefenseTower)) {
                        continue;
                    }

                    bool tooClose = false;
                    int spacingPenalty = 0;
                    for (const auto& existing : buildings) {
                        if (existing.team != team || existing.type != building::DefenseTower) {
                            continue;
                        }
                        const int deltaX = std::abs(candidate.x - existing.point.x);
                        const int deltaY = std::abs(candidate.y - existing.point.y);
                        const int chebyshev = std::max(deltaX, deltaY);
                        if (chebyshev < config::TowerPlacementMinSpacing) {
                            tooClose = true;
                            break;
                        }
                        if (chebyshev < config::TowerPlacementPreferredSpacing) {
                            spacingPenalty += 480;
                        }
                        // Prefer a staggered battery instead of lining towers up
                        // so tightly that they visually or tactically mask each other.
                        if ((deltaX == 0 || deltaY == 0 || deltaX == deltaY)
                            && deltaX + deltaY <= config::DefenseTowerRange) {
                            spacingPenalty += 120;
                        }
                    }
                    if (tooClose) {
                        continue;
                    }

                    const int score = distanceSquared(candidate, center) * 10
                        + std::abs(candidate.y - center.y) * 3
                        + spacingPenalty;
                    if (score < bestRelaxedScore) {
                        bestRelaxedScore = score;
                        bestRelaxed = candidate;
                    }
                    if (spacingPenalty == 0 && score < bestSpacedScore) {
                        bestSpacedScore = score;
                        bestSpaced = candidate;
                    }
                }
            }
        }
        return bestSpaced.x >= 0 ? bestSpaced : bestRelaxed;
    };

    // Barracks are production lifelines, so they stay in the protected base
    // pocket instead of auto-placing near the first contested lane anchor.
    const Point primarySite = type == building::DefenseTower
        ? findTowerNearAnchor(anchor, 10)
        : findNearAnchor(anchor, 7);
    if (primarySite.x >= 0) {
        return primarySite;
    }

    const Point baseSite = type == building::DefenseTower
        ? findTowerNearAnchor(base, 12)
        : findNearAnchor(base, 12);
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

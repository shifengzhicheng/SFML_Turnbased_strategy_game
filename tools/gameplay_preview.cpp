#include "BuildingDefinition.h"
#include "Config.h"
#include "Game.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>

#include <algorithm>
#include <iostream>
#include <string>

namespace
{
    void addCompletedBuilding(Game& game, int team, int type, int laneIndex)
    {
        const Point point = game.findAutoBuildSite(team, type, laneIndex);
        if (point.x < 0) {
            return;
        }

        Building building;
        building.id = game.nextEntityId++;
        building.team = team;
        building.type = type;
        building.point = point;
        building.laneIndex = laneIndex;
        building.complete = true;
        building.maxHealth = buildingDefinition(type).maxHealth;
        building.health = building.maxHealth;
        game.buildings.push_back(building);
        game.setTileID(point.x, point.y,
                       type == building::Barracks
                           ? (team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks)
                           : (team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower));
    }

    MoveableUnit* addUnit(Game& game, int team, int unitName, int laneIndex, Point preferred)
    {
        Point point(-1, -1);
        for (int radius = 0; radius <= 3 && point.x < 0; ++radius) {
            for (int y = preferred.y - radius; y <= preferred.y + radius && point.x < 0; ++y) {
                for (int x = preferred.x - radius; x <= preferred.x + radius; ++x) {
                    if (game.canCreateUnitAt(team, unitName, x, y)) {
                        point = Point(x, y);
                        break;
                    }
                }
            }
        }
        if (point.x < 0 || !game.createUnit(team, unitName, point.x, point.y, laneIndex)) {
            return nullptr;
        }
        MoveableUnit* unit = team == PLAYER ? game.myunits.back().get() : game.enemys.back().get();
        unit->deploymentReadyTime = 0.f;
        unit->stationarySeconds = unitName == UName::SIEGE ? 0.45f : 2.f;
        return unit;
    }
}

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : "build/gameplay_preview.png";

    Game game;
    game.window.setVisible(false);
    game.debugLogging = false;
    game.externalAIControl = true;
    game.autoChooseRewards = true;
    game.matchSeedOverride = 303u;
    game.gameSceneState = SCENE_GAME;
    game.clear();

    game.gameTimeSeconds = 485.f;
    game.playerCommand = 860;
    game.aiCommand = 720;
    game.playerEconomyLevel = 7;
    game.aiEconomyLevel = 7;
    game.playerUpgradeLevel = 10;
    game.aiUpgradeLevel = 10;
    game.playerSelectedLane = lane::Top;
    game.playerReliefCharges = 1;
    game.aiReliefCharges = 2;
    game.playerBaseShieldTimer = 7.f;
    game.playerPerkLevels[perk::Volley] = 3;
    game.playerPerkLevels[perk::Charge] = 2;
    game.playerPerkLevels[perk::Logistics] = 2;
    game.aiPerkLevels[perk::TowerCraft] = 3;
    game.aiPerkLevels[perk::Fortitude] = 2;
    game.playerMastery.levels[trainableUnitIndex(UName::INFANTARY)] = 3;
    game.playerMastery.levels[trainableUnitIndex(UName::SHOOTER)] = 4;
    game.playerMastery.levels[trainableUnitIndex(UName::CAVALRY)] = 2;
    game.playerMastery.levels[trainableUnitIndex(UName::SIEGE)] = 2;
    game.playerMastery.levels[trainableUnitIndex(UName::GUARDIAN)] = 1;

    for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
        addCompletedBuilding(game, PLAYER, building::Barracks, laneIndex);
        addCompletedBuilding(game, AI, building::Barracks, laneIndex);
        addCompletedBuilding(game, PLAYER, building::DefenseTower, laneIndex);
        addCompletedBuilding(game, AI, building::DefenseTower, laneIndex);

        const Point center = game.laneRallyPoint(PLAYER, laneIndex, 2);
        addUnit(game, PLAYER, UName::INFANTARY, laneIndex, Point(center.x - 4, center.y));
        addUnit(game, PLAYER, UName::SHOOTER, laneIndex, Point(center.x - 6, center.y + 1));
        addUnit(game, AI, UName::INFANTARY, laneIndex, Point(center.x + 4, center.y));
        addUnit(game, AI, UName::SHOOTER, laneIndex, Point(center.x + 6, center.y - 1));
    }

    MoveableUnit* cavalry = addUnit(game, PLAYER, UName::CAVALRY, lane::Top,
                                    Point(game.laneRallyPoint(PLAYER, lane::Top, 2).x - 1,
                                          game.laneRallyPoint(PLAYER, lane::Top, 2).y + 1));
    MoveableUnit* siege = addUnit(game, PLAYER, UName::SIEGE, lane::Mid,
                                  Point(game.laneRallyPoint(PLAYER, lane::Mid, 2).x - 5,
                                        game.laneRallyPoint(PLAYER, lane::Mid, 2).y + 1));
    MoveableUnit* guardian = addUnit(game, AI, UName::GUARDIAN, lane::Mid,
                                     Point(game.laneRallyPoint(AI, lane::Mid, 2).x + 2,
                                           game.laneRallyPoint(AI, lane::Mid, 2).y + 1));
    if (cavalry != nullptr) {
        cavalry->UnitState = UState::MOVING;
        cavalry->tilesMovedSinceAttack = config::CavalryChargeTiles;
    }

    const sf::Vector2f topStart(430.f, 165.f);
    const sf::Vector2f topEnd(575.f, 150.f);
    game.addUnitAttackEffect(UName::SHOOTER, topStart, topEnd, sf::Color(255, 236, 152));
    if (siege != nullptr && guardian != nullptr) {
        game.addUnitAttackEffect(UName::SIEGE,
                                 sf::Vector2f((siege->x + 0.5f) * config::TileSize, (siege->y + 0.5f) * config::TileSize),
                                 sf::Vector2f((guardian->x + 0.5f) * config::TileSize, (guardian->y + 0.5f) * config::TileSize),
                                 sf::Color(255, 138, 48));
    }
    game.addFloatingText(sf::Vector2f(565.f, 132.f), "COUNTER!", sf::Color(255, 238, 126), 13);

    if (game.Base_red) {
        game.Base_red->Health = 2460;
        game.Base_red->setState(UState::UNITCLICK);
    }
    if (game.Base_blue) {
        game.Base_blue->Health = 1880;
    }

    sf::RenderTexture canvas;
    if (!canvas.create(config::WindowWidth, config::WindowHeight)) {
        std::cerr << "Failed to create gameplay preview target\n";
        return 1;
    }
    canvas.clear(sf::Color(8, 11, 10));
    sf::sleep(sf::milliseconds(90));
    game.Draw(canvas);
    canvas.display();
    if (!canvas.getTexture().copyToImage().saveToFile(out)) {
        std::cerr << "Failed to save gameplay preview to " << out << '\n';
        return 1;
    }
    std::cout << out << '\n';
    return 0;
}

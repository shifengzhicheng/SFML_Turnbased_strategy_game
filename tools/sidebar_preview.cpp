#include "Game.h"
#include "BuildingDefinition.h"
#include "Config.h"
#include "UnitDefinition.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

namespace
{
    void addCompletedBuilding(Game& game, int team, int type, int laneIndex)
    {
        Point point = game.findAutoBuildSite(team, type, laneIndex);
        if (point.x < 0) {
            point = team == PLAYER ? Point(game.Red_baseP.x + 3 + laneIndex, game.Red_baseP.y - 2 + laneIndex)
                                   : Point(game.Blue_baseP.x - 4 - laneIndex, game.Blue_baseP.y - 2 + laneIndex);
        }

        Building building;
        building.id = game.nextEntityId++;
        building.team = team;
        building.type = type;
        building.point = point;
        building.complete = true;
        building.maxHealth = buildingDefinition(type).maxHealth;
        building.health = building.maxHealth;
        if (type == building::Barracks) {
            building.production.orders.push_back(ProductionOrder{UName::INFANTARY, laneIndex});
        }
        game.buildings.push_back(building);

        if (game.isMapCell(point.x, point.y)) {
            if (type == building::Barracks) {
                game.setTileID(point.x, point.y, team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks);
            }
            else {
                game.setTileID(point.x, point.y, team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower);
            }
        }
    }

    void addUnits(Game& game, int team, int unitName, int count, int laneIndex)
    {
        const Point anchor = team == PLAYER ? game.Red_baseP : game.Blue_baseP;
        for (int i = 0; i < count; ++i) {
            Point spawn = game.findSpawnPointAround(anchor);
            if (spawn.x < 0) {
                const int dir = team == PLAYER ? 1 : -1;
                spawn = Point(anchor.x + dir * (3 + i % 4), anchor.y - 3 + (i % 7));
            }
            game.createUnit(team, unitName, spawn.x, spawn.y, laneIndex);
        }
    }

    bool saveSidebarPreview(Game& game, const std::string& path)
    {
        sf::RenderTexture canvas;
        if (!canvas.create(config::WindowWidth, config::WindowHeight)) {
            return false;
        }
        canvas.clear(sf::Color(24, 32, 28));

        // The preview renders through the same HUD code path as the game, then
        // crops the sidebar so text density can be judged at actual pixels.
        game.DrawSidePanel(canvas);
        canvas.display();

        const sf::Image full = canvas.getTexture().copyToImage();
        sf::Image sidebar;
        sidebar.create(config::PanelWidth, config::WindowHeight, sf::Color::Transparent);
        sidebar.copy(full,
                     0,
                     0,
                     sf::IntRect(config::PanelX, 0, config::PanelWidth, config::WindowHeight),
                     true);
        return sidebar.saveToFile(path);
    }
}

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : "/tmp/tbs_sidebar_preview.png";

    Game game;
    game.window.setVisible(false);
    game.debugLogging = false;
    game.autoChooseRewards = true;
    game.externalAIControl = true;
    game.gameSceneState = SCENE_GAME;
    game.clear();

    // Populate a mid-game board so the sidebar is tested with real counts,
    // unlocks, mastery, and lane pressure instead of an empty start state.
    game.playerCommand = 760;
    game.aiCommand = 930;
    game.playerEconomyLevel = 6;
    game.aiEconomyLevel = 7;
    game.playerUpgradeLevel = 8;
    game.aiUpgradeLevel = 9;
    game.playerSelectedLane = lane::Bot;
    game.aiSelectedLane = lane::Top;
    game.playerPerkLevels[perk::Volley] = 3;
    game.playerPerkLevels[perk::Logistics] = 2;
    game.playerPerkLevels[perk::Mining] = 2;
    game.aiPerkLevels[perk::Charge] = 3;
    game.aiPerkLevels[perk::TowerCraft] = 2;
    game.playerMastery.levels[trainableUnitIndex(UName::INFANTARY)] = 3;
    game.playerMastery.levels[trainableUnitIndex(UName::SHOOTER)] = 4;
    game.playerMastery.levels[trainableUnitIndex(UName::CAVALRY)] = 2;
    game.playerMastery.levels[trainableUnitIndex(UName::SIEGE)] = 1;
    game.playerMastery.levels[trainableUnitIndex(UName::GUARDIAN)] = 2;
    game.aiMastery.levels[trainableUnitIndex(UName::INFANTARY)] = 2;
    game.aiMastery.levels[trainableUnitIndex(UName::SHOOTER)] = 2;
    game.aiMastery.levels[trainableUnitIndex(UName::CAVALRY)] = 3;

    addCompletedBuilding(game, PLAYER, building::Barracks, lane::Top);
    addCompletedBuilding(game, PLAYER, building::Barracks, lane::Bot);
    addCompletedBuilding(game, PLAYER, building::DefenseTower, lane::Mid);
    addCompletedBuilding(game, AI, building::Barracks, lane::Top);
    addCompletedBuilding(game, AI, building::Barracks, lane::Mid);
    addCompletedBuilding(game, AI, building::DefenseTower, lane::Bot);

    addUnits(game, PLAYER, UName::INFANTARY, 3, lane::Top);
    addUnits(game, PLAYER, UName::SHOOTER, 4, lane::Mid);
    addUnits(game, PLAYER, UName::CAVALRY, 2, lane::Bot);
    addUnits(game, PLAYER, UName::SIEGE, 1, lane::Bot);
    addUnits(game, AI, UName::INFANTARY, 4, lane::Top);
    addUnits(game, AI, UName::SHOOTER, 3, lane::Mid);
    addUnits(game, AI, UName::GUARDIAN, 2, lane::Bot);

    if (game.Base_red) {
        game.Base_red->setState(UState::UNITCLICK);
    }

    game.gameTimeSeconds = 245.f;
    if (!saveSidebarPreview(game, out)) {
        std::cerr << "Failed to save sidebar preview to " << out << '\n';
        return 1;
    }

    std::cout << out << '\n';
    return 0;
}

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

bool Game::MousePosChanged()
{
    Vector2i mouse = sf::Mouse::getPosition(window);
    int x = mouse.x / SqureSize;
    int y = mouse.y / SqureSize;
    if (MousePoint.x != x || MousePoint.y != y) {
        MousePoint.x = x;
        MousePoint.y = y;
        return true;
    }
    return false;
}

void Game::loadpic()
{
    art::makeIntroTexture(background, myfont);
    art::makeButtonTexture(tStartBtnNormal, myfont, "START", art::ButtonState::Normal, sf::Vector2u(210, 72));
    art::makeButtonTexture(tStartBtnHover, myfont, "START", art::ButtonState::Hover, sf::Vector2u(210, 72));
    art::makeButtonTexture(tStartBtnClick, myfont, "START", art::ButtonState::Pressed, sf::Vector2u(210, 72));
    art::makeButtonTexture(tStartHelpNormal, myfont, "HELP", art::ButtonState::Normal, sf::Vector2u(170, 72));
    art::makeButtonTexture(tStartHelpHover, myfont, "HELP", art::ButtonState::Hover, sf::Vector2u(170, 72));
    art::makeButtonTexture(tStartHelpClick, myfont, "HELP", art::ButtonState::Pressed, sf::Vector2u(170, 72));
    const sf::Vector2u sideButtonSize(config::SideButtonWidth, config::SideButtonHeight);
    art::makeButtonTexture(tOverBtnNormal, myfont, "PLAY AGAIN", art::ButtonState::Normal, sf::Vector2u(240, 84));
    art::makeButtonTexture(tOverBtnHover, myfont, "PLAY AGAIN", art::ButtonState::Hover, sf::Vector2u(240, 84));
    art::makeButtonTexture(tOverBtnClick, myfont, "PLAY AGAIN", art::ButtonState::Pressed, sf::Vector2u(240, 84));
    const sf::Vector2u unitButtonSize(config::SideButtonWidth, config::SideButtonHeight);
    art::makeButtonTexture(tinf, myfont, "Infantry", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfHover, myfont, "Infantry", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfClick, myfont, "Infantry", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tcav, myfont, "Cavalry", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavHover, myfont, "Cavalry", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavClick, myfont, "Cavalry", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tsho, myfont, "Shooter", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoHover, myfont, "Shooter", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoClick, myfont, "Shooter", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tSiege, myfont, "Siege", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tSiegeHover, myfont, "Siege", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tSiegeClick, myfont, "Siege", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tGuardian, myfont, "Guard", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
    art::makeButtonTexture(tGuardianHover, myfont, "Guard", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
    art::makeButtonTexture(tGuardianClick, myfont, "Guard", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
    art::makeButtonTexture(tUpgrade, myfont, "UPGRADE", art::ButtonState::Normal, sideButtonSize);
    art::makeButtonTexture(tUpgradeHover, myfont, "UPGRADE", art::ButtonState::Hover, sideButtonSize);
    art::makeButtonTexture(tUpgradeClick, myfont, "UPGRADE", art::ButtonState::Pressed, sideButtonSize);
    art::makeButtonTexture(tEconomy, myfont, "ECONOMY", art::ButtonState::Normal, sideButtonSize);
    art::makeButtonTexture(tEconomyHover, myfont, "ECONOMY", art::ButtonState::Hover, sideButtonSize);
    art::makeButtonTexture(tEconomyClick, myfont, "ECONOMY", art::ButtonState::Pressed, sideButtonSize);
    art::makeButtonTexture(tBarracks, myfont, "Barracks", art::ButtonState::Normal, sideButtonSize);
    art::makeButtonTexture(tBarracksHover, myfont, "Barracks", art::ButtonState::Hover, sideButtonSize);
    art::makeButtonTexture(tBarracksClick, myfont, "Barracks", art::ButtonState::Pressed, sideButtonSize);
    art::makeButtonTexture(tHelp, myfont, "HELP", art::ButtonState::Normal, sf::Vector2u(config::SideButtonWidth, 34));
    art::makeButtonTexture(tHelpHover, myfont, "HELP", art::ButtonState::Hover, sf::Vector2u(config::SideButtonWidth, 34));
    art::makeButtonTexture(tHelpClick, myfont, "HELP", art::ButtonState::Pressed, sf::Vector2u(config::SideButtonWidth, 34));
    art::makeButtonTexture(tTower, myfont, "Tower", art::ButtonState::Normal, sideButtonSize);
    art::makeButtonTexture(tTowerHover, myfont, "Tower", art::ButtonState::Hover, sideButtonSize);
    art::makeButtonTexture(tTowerClick, myfont, "Tower", art::ButtonState::Pressed, sideButtonSize);

    startBtn.setTextures(tStartBtnNormal, tStartBtnHover, tStartBtnClick);
    startHelpBtn.setTextures(tStartHelpNormal, tStartHelpHover, tStartHelpClick);
    endGame.setTextures(tOverBtnNormal, tOverBtnHover, tOverBtnClick);
    inf.setTextures(tinf, tinfHover, tinfClick);
    cav.setTextures(tcav, tcavHover, tcavClick);
    sho.setTextures(tsho, tshoHover, tshoClick);
    siegeBtn.setTextures(tSiege, tSiegeHover, tSiegeClick);
    guardianBtn.setTextures(tGuardian, tGuardianHover, tGuardianClick);
    upgradeBtn.setTextures(tUpgrade, tUpgradeHover, tUpgradeClick);
    economyBtn.setTextures(tEconomy, tEconomyHover, tEconomyClick);
    barracksBtn.setTextures(tBarracks, tBarracksHover, tBarracksClick);
    helpBtn.setTextures(tHelp, tHelpHover, tHelpClick);
    towerBtn.setTextures(tTower, tTowerHover, tTowerClick);
    back.setTexture(background);
}

void Game::Initial()
{
    loadFont(myfont, "data/ttf/arial.ttf");
    loadMediaData();
    gameSceneState = SCENE_START;
    gameOver = false;
    const float panelTextX = static_cast<float>(config::PanelX + config::PanelPadding);
    setupText(Globle_text, myfont, 17, sf::Color(229, 221, 189), "GameStart", panelTextX, 48.f);
    setupText(CommandText, myfont, 14, sf::Color(255, 218, 112), "", panelTextX, 176.f);
    setupText(panelTitle, myfont, 19, sf::Color(255, 246, 208), "AUTO WAR", panelTextX, 16.f);
    setupText(panelHint, myfont, 12, sf::Color(211, 199, 165), "Pick lane, queue units", panelTextX, 184.f);
    setupText(economyLabel, myfont, 10, sf::Color(228, 218, 185), "natural CMD", panelTextX, config::EconomyButtonY - 12.f);
    setupText(barracksLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(buildingCommandCost(building::Barracks)) + " auto near base", panelTextX, config::BuildBarracksY + 43.f);
    setupText(infantryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::InfantryCost) + " core / steady", panelTextX, config::BuildInfantryY + 43.f);
    setupText(shooterLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::ShooterCost) + " ranged / slow", panelTextX, config::BuildShooterY + 43.f);
    setupText(cavalryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::CavalryCost) + " fast dive", panelTextX, config::BuildCavalryY + 43.f);
    setupText(siegeLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::SiegeCost) + " slow anti-tower", panelTextX, config::BuildSiegeY + 43.f);
    setupText(guardianLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::GuardianCost) + " heavy / slow", panelTextX, config::BuildGuardianY + 43.f);
    setupText(towerLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(buildingCommandCost(building::DefenseTower)) + " auto lane fort", panelTextX, config::BuildTowerY + 43.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(35, 43, 39));
    sidePanel.setOutlineColor(sf::Color(19, 24, 22));
    sidePanel.setOutlineThickness(2.f);

    helpBtn.setPosition(config::ButtonX, config::HelpButtonY);
    upgradeBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    economyBtn.setPosition(config::ButtonX, config::EconomyButtonY);
    barracksBtn.setPosition(config::ButtonX, config::BuildBarracksY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
    siegeBtn.setPosition(config::ButtonX, config::BuildSiegeY);
    guardianBtn.setPosition(config::ButtonX, config::BuildGuardianY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);
}

void Game::loadMediaData()
{
    loadpic();
}

void Game::clear()
{
    effects.clear();
    running = false;
    gameWin = false;
    MosOnUnit = nullptr;
    playerCommand = config::StartingCommand;
    aiCommand = config::StartingCommand;
    playerIncomeTimer = 0.f;
    aiIncomeTimer = 0.f;
    playerBaseAttackTimer = 0.f;
    aiBaseAttackTimer = 0.f;
    playerBaseShieldTimer = 0.f;
    aiBaseShieldTimer = 0.f;
    playerEmergencyTrainTimer = 0.f;
    aiEmergencyTrainTimer = 0.f;
    gameTimeSeconds = 0.f;
    debugSummaryTimer = 0.f;
    towerPlacementMode = false;
    perkOverlayVisible = false;
    rewardSequence = 0;
    playerSelectedLane = lane::Mid;
    aiSelectedLane = lane::Mid;
    playerPerkLevels.fill(0);
    aiPerkLevels.fill(0);
    aiController.reset();
    ++pathGeneration;
    nextPathRequestId = 1;
    nextEntityId = 1;
    playerUpgradeLevel = 0;
    aiUpgradeLevel = 0;
    playerEconomyLevel = 0;
    aiEconomyLevel = 0;
    realtimeFrameClock.restart();
    resources.clear();
    workers.clear();
    buildings.clear();
    Globle_text.setString("RealTime");
    Base_red.reset();
    Base_blue.reset();
    myunits.clear();
    enemys.clear();
    tiles.clear();
    maze = vector<vector<int>>(height / SqureSize, vector<int>(width / SqureSize));
    gm.gmap(maze, width / SqureSize, height / SqureSize);
    int yp = 0;
    int xp = 0;
    for (int y = yp = 0; y < height; y += SqureSize, yp++)
    {
        for (int x = xp = 0; x < width; x += SqureSize, xp++)
        {
            tile::ID id;
            switch (maze[yp][xp])
            {
            case 1:
                id = tile::Mount;
                break;
            case 2:
                id = tile::River;
                break;
            case 3:
                id = tile::Tree;
                break;
            default:
                id = tile::Empty;
                break;
            }
            MapPos t(sf::IntRect(x, y, SqureSize, SqureSize), id);
            tiles.push_back(t);
        }
    }
    setBase();
    placeResourceNodes();
    createStartingWorkers();

    astar = Astar(maze);

}

void Game::setBase()
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    Red_baseP = Point(5, mapH / 2 - 1);
    Blue_baseP = Point(mapW - 7, mapH / 2 - 1);

    const auto clearBaseArea = [this](Point base) {
        for (int y = base.y - 4; y <= base.y + 5; ++y) {
            for (int x = base.x - 4; x <= base.x + 5; ++x) {
                if (isMapCell(x, y)) {
                    setTileID(x, y, tile::Empty);
                }
            }
        }
    };
    const auto stampBase = [this](Point base, int team) {
        const tile::ID id = team == PLAYER ? tile::Red_Base : tile::Blue_Base;
        setTileID(base.x, base.y, id);
        setTileID(base.x + 1, base.y, id);
        setTileID(base.x, base.y + 1, id);
        setTileID(base.x + 1, base.y + 1, id);
    };

    clearBaseArea(Red_baseP);
    clearBaseArea(Blue_baseP);
    stampBase(Red_baseP, PLAYER);
    stampBase(Blue_baseP, AI);

    Base_red = make_unique<DisMoveableUnit>(Red_baseP.x, Red_baseP.y, PLAYER, this);
    art::makeUnitTexture(Base_red->mytexture, art::UnitKind::Base, art::Team::Player);
    Base_red->setTexture(Base_red->mytexture);

    Base_blue = make_unique<DisMoveableUnit>(Blue_baseP.x, Blue_baseP.y, AI, this);
    art::makeUnitTexture(Base_blue->mytexture, art::UnitKind::Base, art::Team::Enemy);
    Base_blue->setTexture(Base_blue->mytexture);
}

void Game::placeResourceNodes()
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    struct ResourceTarget
    {
        Point point;
    };
    const std::vector<ResourceTarget> targets = {
        {Point(Red_baseP.x + 8, Red_baseP.y + 4)},
        {Point(Blue_baseP.x - 8, Blue_baseP.y - 4)},
        {Point(mapW / 2, mapH / 2)},
        {Point(mapW / 2, mapH / 4)},
        {Point(mapW / 2, mapH * 3 / 4)},
        {Point(mapW / 3, mapH / 2)},
        {Point(mapW * 2 / 3, mapH / 2)}
    };

    for (const auto& target : targets) {
        Point best = target.point;
        bool found = false;
        for (int radius = 0; radius < 18 && !found; ++radius) {
            for (int y = target.point.y - radius; y <= target.point.y + radius && !found; ++y) {
                for (int x = target.point.x - radius; x <= target.point.x + radius; ++x) {
                    if (!isMapCell(x, y)) {
                        continue;
                    }
                    const auto id = tiles[y * horizontalTiles + x].getID();
                    const bool duplicate = std::any_of(resources.begin(), resources.end(), [x, y](const ResourceNode& node) {
                        return nearPoint(node.point, Point(x, y), 4);
                    });
                    if (!duplicate && id == tile::Empty && !nearPoint(Point(x, y), Red_baseP, 4) && !nearPoint(Point(x, y), Blue_baseP, 4)) {
                        best = Point(x, y);
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            continue;
        }

        // Clear a small plaza around CMD markers so lane objectives remain readable.
        for (int y = best.y - 2; y <= best.y + 2; ++y) {
            for (int x = best.x - 2; x <= best.x + 2; ++x) {
                if (isMapCell(x, y)) {
                    setTileID(x, y, tile::Empty);
                }
            }
        }
        ResourceNode node;
        node.point = best;
        resources.push_back(node);
        setTileID(best.x, best.y, tile::Resource);
    }
}

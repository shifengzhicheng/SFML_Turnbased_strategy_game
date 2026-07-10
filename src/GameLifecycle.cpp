#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "LaneGeometry.h"
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
    const sf::Vector2u unitButtonSize(config::UnitButtonWidth, config::SideButtonHeight);
    art::makeButtonTexture(tinf, myfont, "INF", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfHover, myfont, "INF", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfClick, myfont, "INF", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tcav, myfont, "CAV", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavHover, myfont, "CAV", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavClick, myfont, "CAV", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tsho, myfont, "BOW", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoHover, myfont, "BOW", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoClick, myfont, "BOW", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tSiege, myfont, "SGE", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tSiegeHover, myfont, "SGE", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tSiegeClick, myfont, "SGE", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(tGuardian, myfont, "GRD", art::ButtonState::Normal, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
    art::makeButtonTexture(tGuardianHover, myfont, "GRD", art::ButtonState::Hover, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
    art::makeButtonTexture(tGuardianClick, myfont, "GRD", art::ButtonState::Pressed, unitButtonSize, art::UnitKind::Guardian, art::Team::Player);
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
    setupText(panelTitle, myfont, 19, sf::Color(255, 246, 208), "WAR ROOM", panelTextX, 16.f);
    setupText(panelHint, myfont, 12, sf::Color(211, 199, 165), "Pick lane, queue units", panelTextX, 184.f);
    setupText(economyLabel, myfont, 10, sf::Color(228, 218, 185), "natural CMD", panelTextX, config::EconomyButtonY - 12.f);
    setupText(barracksLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(buildingCommandCost(building::Barracks)) + " auto near base", panelTextX, config::BuildBarracksY + 43.f);
    setupText(infantryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(unitCost(UName::INFANTARY)) + " core / steady", panelTextX, config::BuildInfantryY + 43.f);
    setupText(shooterLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(unitCost(UName::SHOOTER)) + " ranged / slow", panelTextX, config::BuildShooterY + 43.f);
    setupText(cavalryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(unitCost(UName::CAVALRY)) + " fast dive", panelTextX, config::BuildCavalryY + 43.f);
    setupText(siegeLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(unitCost(UName::SIEGE)) + " slow anti-tower", panelTextX, config::BuildSiegeY + 43.f);
    setupText(guardianLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(unitCost(UName::GUARDIAN)) + " heavy / slow", panelTextX, config::BuildGuardianY + 43.f);
    setupText(towerLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(buildingCommandCost(building::DefenseTower)) + " auto lane fort", panelTextX, config::BuildTowerY + 43.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(24, 31, 28));
    sidePanel.setOutlineColor(sf::Color(10, 14, 12));
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
    pathfinding.clearPending();
    effects.clear();
    gameWin = false;
    gameOver = false;
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
    playerReliefCharges = config::ComebackReliefCharges;
    aiReliefCharges = config::ComebackReliefCharges;
    playerLaneRebuildReady.fill(0.f);
    aiLaneRebuildReady.fill(0.f);
    gameTimeSeconds = 0.f;
    realtimeAccumulator = 0.0;
    debugSummaryTimer = 0.f;
    towerPlacementMode = false;
    perkOverlayVisible = false;
    rewardSequence = 0;
    rewardChoicesGenerated = false;
    currentMatchSeed = matchSeedOverride;
    if (currentMatchSeed == 0) {
        if (const char* seedValue = std::getenv("TBS_MAP_SEED")) {
            char* end = nullptr;
            const unsigned long parsed = std::strtoul(seedValue, &end, 10);
            if (end != seedValue && *end == '\0') {
                currentMatchSeed = static_cast<unsigned int>(parsed);
            }
        }
    }
    if (currentMatchSeed == 0) {
        currentMatchSeed = std::random_device{}();
    }
    rewardRng.seed(currentMatchSeed ^ 0x9E3779B9u);
    playerRewardRerolls = 0;
    playerSelectedLane = lane::Mid;
    aiSelectedLane = lane::Mid;
    playerPerkLevels.fill(0);
    aiPerkLevels.fill(0);
    playerMastery.levels.fill(0);
    aiMastery.levels.fill(0);
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
    gm.gmap(maze, width / SqureSize, height / SqureSize, currentMatchSeed);
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
    paintLanePathTiles();
    setBase();
    placeResourceNodes();
    createStartingWorkers();

    astar = Astar(maze);

}

void Game::paintLanePathTiles()
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;

    const auto stamp = [this](Point point, int radius) {
        for (int y = point.y - radius; y <= point.y + radius; ++y) {
            for (int x = point.x - radius; x <= point.x + radius; ++x) {
                if (!isMapCell(x, y)) {
                    continue;
                }
                const int dx = x - point.x;
                const int dy = y - point.y;
                if (dx * dx + dy * dy <= radius * radius) {
                    setTileID(x, y, tile::Path);
                }
            }
        }
    };

    const auto stampSegment = [stamp](Point a, Point b, int radius) {
        const int steps = std::max(std::abs(b.x - a.x), std::abs(b.y - a.y)) * 2 + 1;
        for (int i = 0; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const int x = static_cast<int>(std::round(static_cast<float>(a.x) + static_cast<float>(b.x - a.x) * t));
            const int y = static_cast<int>(std::round(static_cast<float>(a.y) + static_cast<float>(b.y - a.y) * t));
            stamp(Point(x, y), radius);
        }
    };

    for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
        const auto route = lane_geometry::laneRoute(mapW, mapH, laneIndex);
        const int radius = laneIndex == lane::Mid ? 2 : 1;
        for (std::size_t i = 1; i < route.size(); ++i) {
            stampSegment(route[i - 1], route[i], radius);
        }
    }
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
    const auto carveHorizontal = [this](int fromX, int toX, int y, tile::ID id) {
        const int begin = std::min(fromX, toX);
        const int end = std::max(fromX, toX);
        for (int x = begin; x <= end; ++x) {
            if (isMapCell(x, y)) {
                setTileID(x, y, id);
            }
        }
    };
    const auto setTemplateTile = [this](int x, int y, tile::ID id, Point base) {
        const bool isBaseFootprint = x >= base.x && x <= base.x + 1 && y >= base.y && y <= base.y + 1;
        if (!isBaseFootprint && isMapCell(x, y)) {
            setTileID(x, y, id);
        }
    };
    const auto applyBaseApproachTemplate = [&](Point base, int team) {
        const int dir = team == PLAYER ? 1 : -1;
        const int frontX = base.x + dir * 5;
        const int farFrontX = base.x + dir * 8;
        const int midY = base.y + 1;
        const int topY = base.y - 3;
        const int botY = base.y + 4;

        // Fixed base approaches make map strategy readable: the middle road is
        // a short fortified throat, while top/bot roads stay open as side gates.
        carveHorizontal(base.x + dir * 2, farFrontX, midY, tile::Path);
        carveHorizontal(base.x + dir * 2, farFrontX, topY, tile::Path);
        carveHorizontal(base.x + dir * 2, farFrontX, botY, tile::Path);
        carveHorizontal(base.x + dir * 1, base.x + dir * 3, base.y - 1, tile::Empty);
        carveHorizontal(base.x + dir * 1, base.x + dir * 3, base.y + 2, tile::Empty);

        const Point blockers[] = {
            Point(frontX, base.y - 2),
            Point(frontX + dir, base.y - 2),
            Point(frontX, base.y + 3),
            Point(frontX + dir, base.y + 3),
            Point(frontX + dir, base.y - 1),
            Point(frontX + dir, base.y + 2),
            Point(farFrontX, topY - 1),
            Point(farFrontX, botY + 1),
        };
        for (std::size_t i = 0; i < sizeof(blockers) / sizeof(blockers[0]); ++i) {
            setTemplateTile(blockers[i].x, blockers[i].y, i < 4 ? tile::Mount : tile::Tree, base);
        }
    };

    clearBaseArea(Red_baseP);
    clearBaseArea(Blue_baseP);
    applyBaseApproachTemplate(Red_baseP, PLAYER);
    applyBaseApproachTemplate(Blue_baseP, AI);
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
        {Point(Red_baseP.x + 3, Red_baseP.y + 6)},
        {Point(Blue_baseP.x - 3, Blue_baseP.y - 6)},
        {lane_geometry::laneWaypoint(mapW, mapH, lane::Mid, 1, false)},
        {lane_geometry::laneWaypoint(mapW, mapH, lane::Top, 1, false)},
        {lane_geometry::laneWaypoint(mapW, mapH, lane::Bot, 1, false)},
        {Point(mapW / 3, mapH / 2 - 5)},
        {Point(mapW * 2 / 3, mapH / 2 + 5)}
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

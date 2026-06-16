#include "Game.h"
#include "AllUnit.h"
#include "Config.h"
#include "Map.h"
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

namespace
{
    constexpr int SqureSize = config::TileSize;
    constexpr int width = config::MapWidth;
    constexpr int height = config::MapHeight;
    constexpr int MaxUnit = config::MaxUnits;

    bool loadFont(sf::Font& font, const std::string& path)
    {
        if (!font.loadFromFile(path)) {
            std::cerr << "Failed to load font: " << path << std::endl;
            return false;
        }
        return true;
    }

    void setupText(sf::Text& text, const sf::Font& font, unsigned int size, sf::Color color,
                   const std::string& value, float x, float y)
    {
        text.setFont(font);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setString(value);
        text.setPosition(x, y);
    }

    void drawUnitBase(sf::RenderWindow& window, Point point, sf::Color color)
    {
        sf::CircleShape marker(9.f, 28);
        marker.setOrigin(9.f, 9.f);
        marker.setScale(1.15f, 0.62f);
        marker.setPosition(point.x * SqureSize + SqureSize / 2.f, point.y * SqureSize + SqureSize / 2.f + 7.f);
        marker.setFillColor(sf::Color(color.r, color.g, color.b, 96));
        marker.setOutlineColor(sf::Color(color.r, color.g, color.b, 210));
        marker.setOutlineThickness(1.4f);
        window.draw(marker);
    }

    sf::Color ownerColor(int owner)
    {
        if (owner == PLAYER) {
            return sf::Color(218, 76, 60);
        }
        if (owner == AI) {
            return sf::Color(61, 128, 206);
        }
        if (owner == -2) {
            return sf::Color(236, 111, 72);
        }
        return sf::Color(226, 180, 63);
    }

    const char* perkTitle(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Iron Wall";
        case perk::Volley:
            return "Volley Drill";
        case perk::Charge:
            return "Shock Charge";
        case perk::SiegeCraft:
            return "Siege Craft";
        case perk::TowerCraft:
            return "Watchtowers";
        case perk::Logistics:
            return "Logistics";
        case perk::Mining:
            return "Supply Crew";
        case perk::WarChest:
            return "War Chest";
        case perk::Drill:
        default:
            return "Blade Drill";
        }
    }

    const char* perkDescription(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Infantry and Guard +9% HP.\nStronger front line.";
        case perk::Volley:
            return "Shooters +8% damage.\nAlso shoot slightly faster.";
        case perk::Charge:
            return "Cavalry +8% damage.\nBetter dives.";
        case perk::SiegeCraft:
            return "Siege +8% damage.\nBuildings take extra pain.";
        case perk::TowerCraft:
            return "Towers +9% damage.\nRange scales carefully.";
        case perk::Logistics:
            return "Barracks train 6% faster.\nTurns economy into tempo.";
        case perk::Mining:
            return "Natural CMD +7%.\nSimple scaling.";
        case perk::WarChest:
            return "Instant CMD burst.\nBuild or queue now.";
        case perk::Drill:
        default:
            return "Infantry +8%, Guard +6%.\nFrontline stays useful.";
        }
    }

    int maxPerkLevel(int type)
    {
        if (type == perk::WarChest) {
            return 99;
        }
        if (type == perk::TowerCraft || type == perk::SiegeCraft) {
            return 4;
        }
        return 5;
    }

    const char* laneName(int laneIndex)
    {
        switch (laneIndex) {
        case lane::Top:
            return "Top";
        case lane::Bot:
            return "Bot";
        case lane::Mid:
        default:
            return "Mid";
        }
    }

    const char* operationTypeName(int type)
    {
        switch (type) {
        case gameop::UpgradeEconomy:
            return "UpgradeEconomy";
        case gameop::UpgradeTech:
            return "UpgradeTech";
        case gameop::BuildBarracks:
            return "BuildBarracks";
        case gameop::BuildTower:
            return "BuildTower";
        case gameop::QueueUnit:
            return "QueueUnit";
        case gameop::SelectLane:
        default:
            return "SelectLane";
        }
    }

    const char* unitDebugName(int name)
    {
        switch (name) {
        case UName::SHOOTER:
            return "Shooter";
        case UName::CAVALRY:
            return "Cavalry";
        case UName::SIEGE:
            return "Siege";
        case UName::GUARDIAN:
            return "Guardian";
        case UName::INFANTARY:
        default:
            return "Infantry";
        }
    }

    const char* perkShortName(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Wall";
        case perk::Volley:
            return "Volley";
        case perk::Charge:
            return "Charge";
        case perk::SiegeCraft:
            return "Siege";
        case perk::TowerCraft:
            return "Tower";
        case perk::Logistics:
            return "Logi";
        case perk::Mining:
            return "Eco";
        case perk::WarChest:
            return "Chest";
        case perk::Drill:
        default:
            return "Blade";
        }
    }

    bool nearPoint(Point a, Point b, int radius)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy <= radius * radius;
    }

    int& commandPool(Game& game, int team)
    {
        return team == PLAYER ? game.playerCommand : game.aiCommand;
    }

    int countUnitsNamed(const std::list<std::unique_ptr<MoveableUnit>>& units, int name)
    {
        return static_cast<int>(std::count_if(units.begin(), units.end(), [name](const std::unique_ptr<MoveableUnit>& unit) {
            return unit->unitName == name;
        }));
    }

    tile::ID buildingTileId(int team, int type)
    {
        if (type == building::DefenseTower) {
            return team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower;
        }
        return team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks;
    }

    float buildingSeconds(int type)
    {
        if (type == building::DefenseTower) {
            return realtime::DefenseTowerBuildSeconds;
        }
        return realtime::BarracksBuildSeconds;
    }

    const char* buildingName(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return "Tower";
        case building::Barracks:
        default:
            return "Barracks";
        }
    }

    int buildingMaxHealth(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return config::DefenseTowerHealth;
        case building::Barracks:
        default:
            return config::BarracksHealth;
        }
    }

    int buildingCommandCost(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return config::TowerCost;
        case building::Barracks:
        default:
            return config::BarracksCost;
        }
    }

    int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    sf::Vector2f unitCenter(const Unit& unit)
    {
        const auto bounds = unit.getGlobalBounds();
        if (bounds.width > 0.f && bounds.height > 0.f) {
            return sf::Vector2f(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        }
        const float baseOffset = unit.unitName == UName::BASE ? SqureSize : SqureSize / 2.f;
        return sf::Vector2f(unit.x * SqureSize + baseOffset, unit.y * SqureSize + baseOffset);
    }

    float unitTrainSeconds(int name)
    {
        switch (name) {
        case UName::SHOOTER:
            return realtime::ShooterTrainSeconds;
        case UName::CAVALRY:
            return realtime::CavalryTrainSeconds;
        case UName::SIEGE:
            return realtime::SiegeTrainSeconds;
        case UName::GUARDIAN:
            return realtime::GuardianTrainSeconds;
        case UName::INFANTARY:
        default:
            return realtime::InfantryTrainSeconds;
        }
    }

    bool isResourceClick(const ResourceNode& node, int tileX, int tileY)
    {
        return node.point.x == tileX && node.point.y == tileY;
    }
}

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

Game::Game() :
    gameWin(false),
    MosOnUnit(nullptr),
    horizontalTiles(width / SqureSize),
    debugLogging(std::getenv("TBS_LOG") != nullptr),
    playerturn(true),
    running(false)
{
    window.create(sf::VideoMode{ config::WindowWidth, config::WindowHeight }, "Project_War");
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight)));
    window.setFramerateLimit(60);
    Initial();
}

Game::~Game() = default;
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
    art::makeButtonTexture(tEndBtnNormal, myfont, "END TURN", art::ButtonState::Normal, sideButtonSize);
    art::makeButtonTexture(tEndBtnHover, myfont, "END TURN", art::ButtonState::Hover, sideButtonSize);
    art::makeButtonTexture(tEndBtnClick, myfont, "END TURN", art::ButtonState::Pressed, sideButtonSize);
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
    EndTurnBtn.setTextures(tEndBtnNormal, tEndBtnHover, tEndBtnClick);
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
    setupText(UnitText, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 96.f);
    setupText(UnitAttack, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 118.f);
    setupText(UnitHP, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 140.f);
    setupText(CommandText, myfont, 14, sf::Color(255, 218, 112), "", panelTextX, 176.f);
    setupText(panelTitle, myfont, 19, sf::Color(255, 246, 208), "AUTO WAR", panelTextX, 16.f);
    setupText(panelHint, myfont, 12, sf::Color(211, 199, 165), "Pick lane, queue units", panelTextX, 184.f);
    setupText(economyLabel, myfont, 10, sf::Color(228, 218, 185), "natural CMD", panelTextX, config::EconomyButtonY - 12.f);
    setupText(barracksLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::BarracksCost) + " auto near base", panelTextX, config::BuildBarracksY + 43.f);
    setupText(infantryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::InfantryCost) + " core / steady", panelTextX, config::BuildInfantryY + 43.f);
    setupText(shooterLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::ShooterCost) + " ranged / slow", panelTextX, config::BuildShooterY + 43.f);
    setupText(cavalryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::CavalryCost) + " fast dive", panelTextX, config::BuildCavalryY + 43.f);
    setupText(siegeLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::SiegeCost) + " slow anti-tower", panelTextX, config::BuildSiegeY + 43.f);
    setupText(guardianLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::GuardianCost) + " heavy / slow", panelTextX, config::BuildGuardianY + 43.f);
    setupText(towerLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::TowerCost) + " auto lane fort", panelTextX, config::BuildTowerY + 43.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(35, 43, 39));
    sidePanel.setOutlineColor(sf::Color(19, 24, 22));
    sidePanel.setOutlineThickness(2.f);

    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
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

void Game::logicBeforeInput()
{
    syncMazeFromTiles();
    astar.setMaze(maze);
    if (!realtimeMode && playerturn == false) {
        AIlogic();
    }
}

void Game::AIUnitreset()
{
    Unitsreset(enemys);
    if (Base_blue) {
        Base_blue->reset();
    }
}

void Game::clearSelection()
{
    // Keep selection exclusive so UI hints, build buttons, and path previews
    // cannot be overwritten by multiple selected actors in one frame.
    drawPaths.clear();
    if (Base_red) {
        Base_red->setState(UState::UNITNORMAL);
    }
    if (Base_blue) {
        Base_blue->setState(UState::UNITNORMAL);
    }
    for (auto& unit : myunits) {
        unit->setState(UState::UNITNORMAL);
    }
    for (auto& unit : enemys) {
        unit->setState(UState::UNITNORMAL);
    }
}

void Game::selectOnly(Unit* unit)
{
    const bool wasSelected = unit != nullptr && unit->UnitState == UState::UNITCLICK;
    clearSelection();
    if (unit != nullptr && !wasSelected) {
        unit->setState(UState::UNITCLICK);
    }
}

bool Game::isBlockingTile(tile::ID id) const
{
    return id == tile::Mount
        || id == tile::River
        || id == tile::Tree
        || (!realtimeMode && id == tile::Unit)
        || id == tile::Red_Base
        || id == tile::Blue_Base
        || id == tile::Player_Barracks
        || id == tile::Enemy_Barracks
        || id == tile::Player_Tower
        || id == tile::Enemy_Tower;
}

bool Game::isMapCell(int x, int y) const
{
    return y >= 0
        && y < static_cast<int>(maze.size())
        && x >= 0
        && x < static_cast<int>(maze[y].size());
}

bool Game::isRealtimeMode() const
{
    return realtimeMode;
}

bool Game::isCellWalkableForUnit(int x, int y) const
{
    if (!isMapCell(x, y)) {
        return false;
    }
    const tile::ID id = tiles[y * horizontalTiles + x].getID();
    return id == tile::Empty || id == tile::Path || id == tile::Choosen || id == tile::Unit || id == tile::Resource;
}

bool Game::isCellReservedForSpawn(int x, int y) const
{
    const auto matchesCell = [x, y](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->x == x && unit->y == y;
    };
    return std::any_of(myunits.begin(), myunits.end(), matchesCell)
        || std::any_of(enemys.begin(), enemys.end(), matchesCell);
}

bool Game::isCellOccupiedByUnit(int x, int y, int ignoredEntityId) const
{
    const auto matchesCell = [x, y, ignoredEntityId](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->entityId != ignoredEntityId && unit->Health > 0 && unit->x == x && unit->y == y;
    };
    return std::any_of(myunits.begin(), myunits.end(), matchesCell)
        || std::any_of(enemys.begin(), enemys.end(), matchesCell);
}

bool Game::canUnitStepInto(const MoveableUnit& unit, Point point) const
{
    return isCellWalkableForUnit(point.x, point.y)
        && !isCellOccupiedByUnit(point.x, point.y, unit.entityId);
}

bool Game::isBuildableCell(int x, int y) const
{
    if (!isMapCell(x, y)) {
        return false;
    }
    const bool workerOnCell = std::any_of(workers.begin(), workers.end(), [x, y](const Worker& worker) {
        return worker.point.x == x && worker.point.y == y;
    });
    return tiles[y * horizontalTiles + x].getID() == tile::Empty && !isCellReservedForSpawn(x, y) && !workerOnCell;
}

bool Game::isBuildSiteInInfluence(int team, Point point, int type) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const int radiusSquared = config::BuildInfluenceRadius * config::BuildInfluenceRadius;
    if (distanceSquared(point, base) <= radiusSquared) {
        return true;
    }

    for (const auto& building : buildings) {
        if (building.team != team || !building.complete) {
            continue;
        }
        if (distanceSquared(point, building.point) <= radiusSquared) {
            return true;
        }
    }
    return false;
}

void Game::setTileID(int x, int y, tile::ID id)
{
    const auto index = y * horizontalTiles + x;
    if (!isMapCell(x, y) || index < 0 || index >= static_cast<int>(tiles.size())) {
        return;
    }
    // Tiles are the render source and maze is the pathfinding source; update
    // them together to avoid one-frame desyncs.
    tiles[index].setID(id);
    maze[y][x] = isBlockingTile(id) ? 1 : 0;
}

void Game::syncMazeFromTiles()
{
    // Rebuild pathing from visible map state before input/AI mutates gameplay.
    for (const auto& tile : tiles) {
        const auto p = tile.getIndex();
        if (isMapCell(p.x, p.y)) {
            maze[p.y][p.x] = isBlockingTile(tile.getID()) ? 1 : 0;
        }
    }
}

void Game::AIlogic() {
    bool timepass = clock2.getElapsedTime().asMilliseconds() > 30.f;
    if (!aiProductionDone) {
        runAIProduction();
        aiProductionDone = true;
    }
    if (enemys.empty()) {
        playerturn = true;
        Globle_text.setString("YourTurn");
        running = false;
        AIUnitreset();
        addTurnIncome(PLAYER);
        aiProductionDone = false;
        return;
    }
    if (timepass) {
        for (auto& u : enemys) {
            u->decide();
        }
        clock2.restart();
    }
    std::size_t n = 0;
    for (auto& u : enemys) { 
        if (u->myActionPoint <= 0) {
            n++;
            if (n == enemys.size()) {
                playerturn = true;
                Globle_text.setString("YourTurn");
                running = false;
                AIUnitreset();
                addTurnIncome(PLAYER);
                aiProductionDone = false;
            }
        }
    }
}

void Game::updateRealtime(float dt)
{
    gameTimeSeconds += dt;
    syncMazeFromTiles();
    astar.setMaze(maze);
    applyPathResults();
    cleanupDestroyedBuildings();
    updateComebackTimers(dt);
    updateRealtimeEconomy(dt);
    if (!externalAIControl) {
        aiController.update(*this, dt);
    }
    assignWorkers();
    updateWorkers(dt);
    updateProduction(dt);
    updateEmergencyBaseTraining(dt);
    updateDefenseTowers(dt);
    updateBaseDefenses(dt);
    realtime::updateAutoCombat(*this, dt);
    if ((Base_red && Base_red->Health <= 0) || (Base_blue && Base_blue->Health <= 0)) {
        gameOver = true;
    }
    cleanupDestroyedBuildings();
    updateDebugSummary(dt);
}

void Game::updateComebackTimers(float dt)
{
    playerBaseShieldTimer = std::max(0.f, playerBaseShieldTimer - dt);
    aiBaseShieldTimer = std::max(0.f, aiBaseShieldTimer - dt);
}

void Game::updateTimedRewards()
{
    // Rogue choices are intentionally tied to tech upgrades, not timers.
    // The method stays as a hook for future event-driven rewards.
}

void Game::updateRealtimeEconomy(float dt)
{
    playerIncomeTimer += dt;
    aiIncomeTimer += dt;

    if (playerIncomeTimer >= realtime::EconomyTickSeconds) {
        playerIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(PLAYER);
    }
    if (aiIncomeTimer >= realtime::EconomyTickSeconds) {
        aiIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(AI);
    }
}

void Game::logEvent(const std::string& message) const
{
    if (!debugLogging) {
        return;
    }
    std::clog << "[tbs " << static_cast<int>(std::round(gameTimeSeconds)) << "s] " << message << '\n';
}

void Game::logDebugSummary() const
{
    if (!debugLogging) {
        return;
    }
    std::clog << "[tbs " << static_cast<int>(std::round(gameTimeSeconds)) << "s] "
        << "player cmd=" << playerCommand
        << " eco=" << playerEconomyLevel
        << " rax=" << completedBuildingCount(PLAYER, building::Barracks) << "/" << buildingCap(PLAYER, building::Barracks)
        << " tower=" << totalBuildingCount(PLAYER, building::DefenseTower)
        << " level=" << playerUpgradeLevel
        << " base=" << (Base_red ? Base_red->Health : 0)
        << " shield=" << static_cast<int>(std::ceil(playerBaseShieldTimer))
        << " army=" << myunits.size()
        << " | ai cmd=" << aiCommand
        << " eco=" << aiEconomyLevel
        << " rax=" << completedBuildingCount(AI, building::Barracks) << "/" << buildingCap(AI, building::Barracks)
        << " tower=" << totalBuildingCount(AI, building::DefenseTower)
        << " level=" << aiUpgradeLevel
        << " base=" << (Base_blue ? Base_blue->Health : 0)
        << " shield=" << static_cast<int>(std::ceil(aiBaseShieldTimer))
        << " army=" << enemys.size()
        << '\n';
}

void Game::updateDebugSummary(float dt)
{
    if (!debugLogging) {
        return;
    }
    debugSummaryTimer += dt;
    if (debugSummaryTimer >= 10.f) {
        debugSummaryTimer -= 10.f;
        logDebugSummary();
    }
}

Building* Game::findBuildingById(int id)
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [id](const Building& building) {
        return building.id == id;
    });
    return it == buildings.end() ? nullptr : &(*it);
}

const Building* Game::findBuildingById(int id) const
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [id](const Building& building) {
        return building.id == id;
    });
    return it == buildings.end() ? nullptr : &(*it);
}

Worker* Game::findWorkerById(int id)
{
    const auto it = std::find_if(workers.begin(), workers.end(), [id](const Worker& worker) {
        return worker.id == id;
    });
    return it == workers.end() ? nullptr : &(*it);
}

void Game::resetWorkersForBuilding(int buildingId)
{
    for (auto& worker : workers) {
        if (worker.buildingId != buildingId) {
            continue;
        }
        worker.state = worker::Idle;
        worker.buildingId = 0;
        worker.path.clear();
        worker.pendingPathRequest = 0;
    }
}

int Game::workerCount(int team) const
{
    return static_cast<int>(std::count_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team;
    }));
}

int Game::assignedWorkerCount(int buildingId) const
{
    return static_cast<int>(std::count_if(workers.begin(), workers.end(), [buildingId](const Worker& worker) {
        return worker.buildingId == buildingId && worker.state != worker::Idle;
    }));
}

Worker* Game::findIdleWorker(int team)
{
    const auto it = std::find_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team && worker.state == worker::Idle;
    });
    return it == workers.end() ? nullptr : &(*it);
}

Worker* Game::findAvailableWorker(int team)
{
    if (Worker* worker = findIdleWorker(team)) {
        return worker;
    }
    return nullptr;
}

void Game::assignWorkerToBuilding(Worker& worker, Building& building)
{
    worker.state = worker::MovingToBuild;
    worker.buildingId = building.id;
    worker.target = findBuildStandPoint(building);
    worker.path.clear();
    worker.pendingPathRequest = 0;
    worker.pathTimer = realtime::WorkerPathRefreshSeconds;
    worker.moveTimer = 0.f;
}

Point Game::workerSpawnPoint(int team) const
{
    const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
    const auto occupiedByWorker = [this](Point point) {
        return std::any_of(workers.begin(), workers.end(), [point](const Worker& worker) {
            return worker.point.x == point.x && worker.point.y == point.y;
        });
    };
    const Point candidates[] = {
        Point(base.x - 1, base.y),
        Point(base.x, base.y - 1),
        Point(base.x + 2, base.y + 1),
        Point(base.x + 1, base.y + 2),
        Point(base.x - 1, base.y + 1),
        Point(base.x + 1, base.y - 1)
    };
    for (const auto& candidate : candidates) {
        if (isCellWalkableForUnit(candidate.x, candidate.y) && !occupiedByWorker(candidate) && !isCellReservedForSpawn(candidate.x, candidate.y)) {
            return candidate;
        }
    }
    for (int radius = 2; radius <= 4; ++radius) {
        for (int y = base.y - radius; y <= base.y + radius; ++y) {
            for (int x = base.x - radius; x <= base.x + radius; ++x) {
                const Point candidate(x, y);
                if (isCellWalkableForUnit(x, y) && !occupiedByWorker(candidate) && !isCellReservedForSpawn(x, y)) {
                    return candidate;
                }
            }
        }
    }
    return base;
}

void Game::createWorker(int team, Point point)
{
    Worker worker;
    worker.id = nextEntityId++;
    worker.team = team;
    worker.point = point;
    worker.target = point;
    workers.push_back(worker);
}

void Game::createStartingWorkers()
{
    for (int i = 0; i < realtime::StartingWorkers; ++i) {
        createWorker(PLAYER, workerSpawnPoint(PLAYER));
        createWorker(AI, workerSpawnPoint(AI));
    }
}

Point Game::findBuildStandPoint(const Building& building) const
{
    const Point offsets[] = {
        Point(-1, 0), Point(1, 0), Point(0, -1), Point(0, 1),
        Point(-1, -1), Point(1, -1), Point(-1, 1), Point(1, 1)
    };
    for (const auto& offset : offsets) {
        const Point candidate(building.point.x + offset.x, building.point.y + offset.y);
        if (isCellWalkableForUnit(candidate.x, candidate.y)) {
            return candidate;
        }
    }
    return building.point;
}

Point Game::findAttackStandPoint(const MoveableUnit& unit, const Building& building) const
{
    const int range = std::max(1, unit.myAttackRange());
    const int rangeSquared = range * range;
    const Point current(unit.x, unit.y);
    Point best(-1, -1);
    int bestScore = std::numeric_limits<int>::max();

    for (int y = building.point.y - range; y <= building.point.y + range; ++y) {
        for (int x = building.point.x - range; x <= building.point.x + range; ++x) {
            const Point candidate(x, y);
            if (!isCellWalkableForUnit(x, y) || isCellOccupiedByUnit(x, y, unit.entityId)) {
                continue;
            }
            const int toTarget = distanceSquared(candidate, building.point);
            if (toTarget > rangeSquared) {
                continue;
            }

            int score = distanceSquared(current, candidate);
            if (building.type == building::DefenseTower && unit.unitName == UName::SIEGE) {
                const int towerRange = defenseTowerRange(building.team);
                const int towerRangeSquared = towerRange * towerRange;
                // Siege engines should naturally set up just outside tower
                // range, forcing the defender to send units instead of turtling.
                score += toTarget > towerRangeSquared ? -1200 - toTarget : 1800;
            }
            if (score < bestScore) {
                bestScore = score;
                best = candidate;
            }
        }
    }

    return best.x >= 0 ? best : findBuildStandPoint(building);
}

Point Game::findBuildableNear(Point anchor, int radius) const
{
    for (int r = 1; r <= radius; ++r) {
        for (int y = anchor.y - r; y <= anchor.y + r; ++y) {
            for (int x = anchor.x - r; x <= anchor.x + r; ++x) {
                if (isBuildableCell(x, y)) {
                    return Point(x, y);
                }
            }
        }
    }
    return Point(-1, -1);
}

Point Game::findSpawnPointAround(Point anchor) const
{
    for (int r = 1; r <= 3; ++r) {
        for (int y = anchor.y - r; y <= anchor.y + r; ++y) {
            for (int x = anchor.x - r; x <= anchor.x + r; ++x) {
                if (isCellWalkableForUnit(x, y) && !isCellReservedForSpawn(x, y)) {
                    return Point(x, y);
                }
            }
        }
    }
    return Point(-1, -1);
}

int Game::completedBuildingCount(int team, int type) const
{
    return static_cast<int>(std::count_if(buildings.begin(), buildings.end(), [team, type](const Building& building) {
        return building.team == team && building.type == type && building.complete;
    }));
}

int Game::totalBuildingCount(int team, int type) const
{
    return static_cast<int>(std::count_if(buildings.begin(), buildings.end(), [team, type](const Building& building) {
        return building.team == team && building.type == type;
    }));
}

int Game::buildingCap(int team, int type) const
{
    const int tech = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int economy = economyLevelForTeam(team);
    if (type == building::Barracks) {
        return std::min(config::BarracksCap, config::BarracksBaseCap + tech / 4 + economy / 2);
    }
    if (type == building::DefenseTower) {
        return std::min(config::TowerCap, config::TowerBaseCap + tech / 5 + perkLevel(team, perk::TowerCraft) / 3);
    }
    return static_cast<int>(resources.size());
}

float Game::damageMultiplier(int team) const
{
    const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    return 1.f + static_cast<float>(level) * config::TechDamageBonus;
}

float Game::unitDamageMultiplier(int team, int unitName) const
{
    float multiplier = damageMultiplier(team);
    switch (unitName) {
    case UName::INFANTARY:
        multiplier += static_cast<float>(perkLevel(team, perk::Drill)) * 0.08f;
        break;
    case UName::GUARDIAN:
        multiplier += static_cast<float>(perkLevel(team, perk::Drill)) * 0.06f;
        break;
    case UName::SHOOTER:
        multiplier += static_cast<float>(perkLevel(team, perk::Volley)) * 0.08f;
        break;
    case UName::CAVALRY:
        multiplier += static_cast<float>(perkLevel(team, perk::Charge)) * 0.08f;
        break;
    case UName::SIEGE:
        multiplier += static_cast<float>(perkLevel(team, perk::SiegeCraft)) * 0.08f;
        break;
    default:
        break;
    }
    return multiplier;
}

float Game::unitHealthMultiplier(int team, int unitName) const
{
    float multiplier = 1.f + static_cast<float>(team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * config::TechHealthBonus;
    if (unitName == UName::INFANTARY || unitName == UName::GUARDIAN) {
        multiplier += static_cast<float>(perkLevel(team, perk::Fortitude)) * 0.09f;
    }
    if (unitName == UName::CAVALRY) {
        multiplier += static_cast<float>(perkLevel(team, perk::Charge)) * 0.04f;
    }
    if (unitName == UName::SIEGE) {
        multiplier += static_cast<float>(perkLevel(team, perk::SiegeCraft)) * 0.04f;
    }
    return multiplier;
}

float Game::unitAttackCooldownMultiplier(int team, int unitName) const
{
    float multiplier = 1.f;
    if (unitName == UName::SHOOTER) {
        multiplier -= static_cast<float>(perkLevel(team, perk::Volley)) * 0.035f;
    }
    if (unitName == UName::CAVALRY) {
        multiplier -= static_cast<float>(perkLevel(team, perk::Charge)) * 0.018f;
    }
    return std::clamp(multiplier, 0.76f, 1.f);
}

float Game::baseDamageTakenMultiplier(int attackerUnitName, int defenderTeam) const
{
    float shield = 1.f;
    if (gameTimeSeconds < 480.f) {
        shield = 0.25f;
    }
    else if (gameTimeSeconds < 600.f) {
        shield = 0.45f;
    }
    else if (gameTimeSeconds < 720.f) {
        shield = 0.70f;
    }

    // Siege should feel like the correct finisher, but not fully erase the
    // pacing shield that keeps normal wins near the 10-minute target.
    if (attackerUnitName == UName::SIEGE) {
        shield += 0.18f;
    }
    else if (attackerUnitName == UName::GUARDIAN) {
        shield += 0.08f;
    }
    if (baseShieldSecondsForTeam(defenderTeam) > 0.f) {
        shield *= config::EmergencyShieldDamageMultiplier;
    }
    return std::clamp(shield, 0.25f, 1.f);
}

float Game::baseShieldSecondsForTeam(int team) const
{
    return team == PLAYER ? playerBaseShieldTimer : aiBaseShieldTimer;
}

float Game::teamTrainTimeMultiplier(int team) const
{
    const float logistics = static_cast<float>(perkLevel(team, perk::Logistics)) * 0.06f;
    return std::clamp(1.f - logistics, 0.74f, 1.f);
}

float Game::miningIncomeMultiplier(int team) const
{
    return 1.f + static_cast<float>(perkLevel(team, perk::Mining)) * 0.07f;
}

int Game::defenseTowerRange(int team) const
{
    return config::DefenseTowerRange + std::min(1, perkLevel(team, perk::TowerCraft) / 3);
}

int Game::selectedLaneForTeam(int team) const
{
    return team == PLAYER ? playerSelectedLane : aiSelectedLane;
}

Point Game::laneWaypoint(int team, int laneIndex, int stage) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const int laneY[] = {
        std::max(4, mapH / 4),
        mapH / 2,
        std::min(mapH - 5, mapH * 3 / 4)
    };
    const int safeLane = std::clamp(laneIndex, 0, lane::Count - 1);
    const int playerX[] = {mapW / 4, mapW / 2, mapW * 3 / 4};
    const int aiX[] = {mapW * 3 / 4, mapW / 2, mapW / 4};
    const int safeStage = std::clamp(stage, 0, 2);
    return Point(team == PLAYER ? playerX[safeStage] : aiX[safeStage], laneY[safeLane]);
}

Point Game::laneDefensePoint(int team, int laneIndex) const
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const int laneY[] = {
        std::max(5, mapH / 4),
        mapH / 2,
        std::min(mapH - 6, mapH * 3 / 4)
    };
    const int safeLane = std::clamp(laneIndex, 0, lane::Count - 1);
    return Point(team == PLAYER ? mapW / 5 : mapW * 4 / 5, laneY[safeLane]);
}

int Game::upgradeCostForNextLevel(int team) const
{
    const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    if (level >= config::MaxTechLevel) {
        return 0;
    }

    const int costs[config::MaxTechLevel] = {
        65, 85, 110, 140, 175,
        215, 260, 310, 370, 440,
        520, 610, 710, 820, 950
    };
    int rawCost = costs[level];
    if (team == AI && gameTimeSeconds > 420.f && economyLevelForTeam(AI) <= 1) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.62f));
    }
    if (team == AI && gameTimeSeconds > 840.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.42f));
    }
    else if (team == AI && gameTimeSeconds > 780.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.58f));
    }
    else if (team == AI && gameTimeSeconds > 660.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.74f));
    }
    return std::max(25, rawCost);
}

int Game::unitsNearPoint(int team, Point point, int radius) const
{
    const auto& units = team == PLAYER ? myunits : enemys;
    const int radiusSquared = radius * radius;
    return static_cast<int>(std::count_if(units.begin(), units.end(), [point, radiusSquared](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->Health > 0 && distanceSquared(Point(unit->x, unit->y), point) <= radiusSquared;
    }));
}

bool Game::requestBuildBarracks(int team, Point point)
{
    if (totalBuildingCount(team, building::Barracks) >= buildingCap(team, building::Barracks)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tech for more Rax", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::Barracks)
        || commandPool(*this, team) < config::BarracksCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::BarracksCost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::Barracks) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= config::BarracksCost;
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
    if (totalBuildingCount(team, building::DefenseTower) >= buildingCap(team, building::DefenseTower)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tower cap", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y)
        || !isBuildSiteInInfluence(team, point, building::DefenseTower)
        || commandPool(*this, team) < config::TowerCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::TowerCost ? "Need CMD" : (!isBuildSiteInInfluence(team, point, building::DefenseTower) ? "Too far" : "Bad site"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= config::TowerCost;
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
    const Point anchor = type == building::DefenseTower
        ? laneDefensePoint(team, laneIndex)
        : Point(base.x + 4, base.y + (laneIndex - 1) * 3);

    for (int radius = 1; radius <= 9; ++radius) {
        for (int y = anchor.y - radius; y <= anchor.y + radius; ++y) {
            for (int x = anchor.x - radius; x <= anchor.x + radius; ++x) {
                const Point candidate(x, y);
                if (isBuildableCell(x, y) && isBuildSiteInInfluence(team, candidate, type)) {
                    return candidate;
                }
            }
        }
    }
    return findBuildableNear(base, 12);
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

void Game::assignWorkers()
{
    // Worker routing is demand based: visible drones build nearby structures;
    // economy upgrades simply add more drones around the base.
    for (auto& building : buildings) {
        if (building.complete) {
            continue;
        }
        if (assignedWorkerCount(building.id) > 0) {
            continue;
        }

        if (Worker* worker = findAvailableWorker(building.team)) {
            assignWorkerToBuilding(*worker, building);
        }
    }
}

void Game::updateWorkerTravel(Worker& worker, Point target, float dt)
{
    worker.target = target;
    worker.pathTimer += dt;
    if ((worker.pathTimer >= realtime::WorkerPathRefreshSeconds || worker.path.empty()) && worker.pendingPathRequest == 0) {
        worker.pathTimer = 0.f;
        requestPathForWorker(worker, target);
    }

    worker.moveTimer += dt;
    if (worker.moveTimer < realtime::WorkerStepSeconds || worker.path.empty()) {
        return;
    }

    worker.moveTimer -= realtime::WorkerStepSeconds;
    const Point next = worker.path.front();
    worker.path.pop_front();
    if (isCellWalkableForUnit(next.x, next.y)) {
        worker.point = next;
    }
    else {
        worker.path.clear();
        worker.pathTimer = realtime::WorkerPathRefreshSeconds;
    }
}

void Game::updateWorkers(float dt)
{
    for (auto& worker : workers) {
        if (worker.state == worker::Idle) {
            continue;
        }

        Building* building = findBuildingById(worker.buildingId);
        if (building == nullptr) {
            worker.state = worker::Idle;
            worker.buildingId = 0;
            worker.path.clear();
            worker.pendingPathRequest = 0;
            continue;
        }

        if (building->complete) {
            worker.state = worker::Idle;
            worker.buildingId = 0;
            worker.path.clear();
            worker.pendingPathRequest = 0;
            continue;
        }

        const bool nearBuildSite = nearPoint(worker.point, building->point, 1);
        if (nearBuildSite) {
            worker.state = worker::Building;
            building->buildProgress = std::min(building->buildSeconds, building->buildProgress + dt);
            if (building->buildProgress >= building->buildSeconds) {
                building->complete = true;
                worker.path.clear();
                worker.pendingPathRequest = 0;
                worker.state = worker::Idle;
                worker.buildingId = 0;
                const sf::Vector2f pos(building->point.x * SqureSize, building->point.y * SqureSize - 10.f);
                addFloatingText(pos, building->type == building::DefenseTower ? "Tower ready" : "Barracks ready",
                                building->team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 12);
                logEvent(std::string(building->team == PLAYER ? "player" : "ai") + " completed " + buildingName(building->type)
                    + " id=" + std::to_string(building->id));
            }
            continue;
        }

        worker.state = worker::MovingToBuild;
        updateWorkerTravel(worker, findBuildStandPoint(*building), dt);
    }
}

void Game::updateProduction(float dt)
{
    for (auto& building : buildings) {
        if (!building.complete || building.type != building::Barracks) {
            continue;
        }
        if (building.production.activeUnit < 0 && !building.production.orders.empty()) {
            const auto order = building.production.orders.front();
            building.production.activeUnit = order.unit;
            building.production.activeLane = order.lane;
            building.production.orders.pop_front();
            building.production.progress = 0.f;
        }
        if (building.production.activeUnit < 0) {
            continue;
        }

        building.production.progress += dt;
        const float trainSeconds = unitTrainSeconds(building.production.activeUnit) * teamTrainTimeMultiplier(building.team);
        if (building.production.progress < trainSeconds) {
            continue;
        }

        const Point spawn = findSpawnPointAround(building.point);
        if (spawn.x >= 0 && createUnit(building.team, building.production.activeUnit, spawn.x, spawn.y, building.production.activeLane)) {
            building.production.activeUnit = -1;
            building.production.activeLane = lane::Mid;
            building.production.progress = 0.f;
        }
    }
}

void Game::updateEmergencyBaseTraining(float dt)
{
    const auto trainFromBase = [this, dt](int team, float& timer) {
        const bool enabled = gameTimeSeconds > 360.f && completedBuildingCount(team, building::Barracks) == 0;
        if (!enabled) {
            timer = std::min(timer + dt, 4.f);
            return;
        }

        timer += dt;
        const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
        int unitName = UName::INFANTARY;
        if (level >= 8) {
            unitName = UName::GUARDIAN;
        }
        else if (level >= 5) {
            unitName = UName::SIEGE;
        }
        else if (level >= 3) {
            unitName = UName::CAVALRY;
        }
        else if (level >= 1) {
            unitName = UName::SHOOTER;
        }

        const float interval = unitTrainSeconds(unitName) * 1.55f * teamTrainTimeMultiplier(team);
        if (timer < interval || commandForTeam(team) < unitCost(unitName) || !hasUnitCapacity(team)) {
            return;
        }

        const Point base = team == PLAYER ? Red_baseP : Blue_baseP;
        const Point spawn = findSpawnPointAround(base);
        if (spawn.x < 0) {
            return;
        }

        commandPool(*this, team) -= unitCost(unitName);
        timer = 0.f;
        // Slow HQ conscription is a comeback valve; barracks remain the only
        // efficient way to mass units, but a raided side is not permanently dead.
        createUnit(team, unitName, spawn.x, spawn.y, selectedLaneForTeam(team));
        logEvent(std::string(team == PLAYER ? "player" : "ai") + " emergency trained unit="
            + std::to_string(unitName));
    };

    trainFromBase(PLAYER, playerEmergencyTrainTimer);
    trainFromBase(AI, aiEmergencyTrainTimer);
}

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
            const int score = distanceSquared(building.point, Point(candidate->x, candidate->y));
            if (score <= rangeSquared && score < bestScore) {
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
        const float towerMultiplier = damageMultiplier(building.team)
            + static_cast<float>(perkLevel(building.team, perk::TowerCraft)) * 0.09f;
        const int damage = std::max(1, static_cast<int>(std::round(static_cast<float>(config::DefenseTowerDamage) * towerMultiplier)));
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

void Game::cleanupDestroyedBuildings()
{
    for (auto it = buildings.begin(); it != buildings.end(); ) {
        if (it->health > 0) {
            ++it;
            continue;
        }

        const Building destroyed = *it;
        resetWorkersForBuilding(destroyed.id);
        setTileID(destroyed.point.x, destroyed.point.y, tile::Empty);
        addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 8.f),
                        std::string(buildingName(destroyed.type)) + " down", sf::Color(255, 218, 112), 12);
        applyStructureLossRelief(destroyed);
        const int destroyer = destroyed.team == PLAYER ? AI : PLAYER;
        commandPool(*this, destroyer) = std::min(config::MaxCommand, commandPool(*this, destroyer) + 12);
        if (destroyer == PLAYER) {
            addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 24.f),
                            "Raid +12 CMD", sf::Color(255, 224, 99), 12);
        }
        logEvent(std::string(destroyed.team == PLAYER ? "player" : "ai") + " "
            + buildingName(destroyed.type) + " destroyed id=" + std::to_string(destroyed.id));
        it = buildings.erase(it);
    }
}

void Game::applyStructureLossRelief(const Building& destroyed)
{
    const int team = destroyed.team;
    const int baseCost = buildingCommandCost(destroyed.type);
    if (baseCost <= 0) {
        return;
    }

    int salvage = std::max(8, baseCost * config::StructureSalvagePercent / 100);
    const bool losingLastBarracks = destroyed.type == building::Barracks
        && totalBuildingCount(team, building::Barracks) <= 1;
    if (losingLastBarracks) {
        salvage += config::LastBarracksReliefBonus;
    }
    commandPool(*this, team) = std::min(config::MaxCommand, commandPool(*this, team) + salvage);

    DisMoveableUnit* base = team == PLAYER ? Base_red.get() : Base_blue.get();
    const int repair = destroyed.type == building::Barracks
        ? config::EmergencyBarracksRepair
        : config::EmergencyTowerRepair;
    int appliedRepair = 0;
    if (base != nullptr && base->Health > 0) {
        const int before = base->Health;
        base->Health = std::min(4000, base->Health + repair);
        appliedRepair = base->Health - before;
        float& shieldTimer = team == PLAYER ? playerBaseShieldTimer : aiBaseShieldTimer;
        shieldTimer = std::max(shieldTimer, config::EmergencyShieldSeconds);
        base->playFlash(team == PLAYER ? sf::Color(255, 232, 132, 255) : sf::Color(142, 196, 255, 255), 0.35f);
    }

    if (team == PLAYER) {
        const sf::Vector2f textPos(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 26.f);
        addFloatingText(textPos, "Rebuild +" + std::to_string(salvage) + " CMD",
                        sf::Color(255, 232, 132), 12);
        if (appliedRepair > 0) {
            addFloatingText(sf::Vector2f(Red_baseP.x * SqureSize, Red_baseP.y * SqureSize - 36.f),
                            "HQ Shield +" + std::to_string(appliedRepair),
                            sf::Color(255, 244, 178), 13);
        }
    }

    logEvent(std::string(team == PLAYER ? "player" : "ai")
        + " comeback salvage=+" + std::to_string(salvage)
        + " repair=+" + std::to_string(appliedRepair)
        + " shield=" + std::to_string(static_cast<int>(std::ceil(baseShieldSecondsForTeam(team))))
        + " after " + buildingName(destroyed.type) + " loss");
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

Point Game::chooseStrategicRallyPoint(const MoveableUnit& unit) const
{
    const Point current(unit.x, unit.y);
    const int mapW = width / SqureSize;
    const int enemyTeam = unit.myteam == PLAYER ? AI : PLAYER;
    const bool enemyProductionBroken = gameTimeSeconds > 420.f
        && completedBuildingCount(enemyTeam, building::Barracks) == 0
        && completedBuildingCount(enemyTeam, building::DefenseTower) == 0;
    const bool onEnemyHalf = unit.myteam == PLAYER ? current.x > mapW / 2 : current.x < mapW / 2;
    if (enemyProductionBroken && onEnemyHalf) {
        return Point(-1, -1);
    }

    // Each produced unit belongs to a lane. It advances through center and
    // enemy-side waypoints before the normal target picker takes over near the
    // opponent base.
    const int stage = unit.myteam == PLAYER
        ? (unit.x < mapW / 2 ? 1 : (unit.x < mapW * 3 / 4 ? 2 : 3))
        : (unit.x > mapW / 2 ? 1 : (unit.x > mapW / 4 ? 2 : 3));
    if (stage <= 2) {
        const Point laneGoal = laneWaypoint(unit.myteam, unit.laneIndex, stage);
        if (!nearPoint(current, laneGoal, 2) && isCellWalkableForUnit(laneGoal.x, laneGoal.y)) {
            return laneGoal;
        }
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
        typeFactor = 2.18f + static_cast<float>(perkLevel(unit.myteam, perk::SiegeCraft)) * 0.10f;
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

MoveableUnit* Game::findMoveableUnitById(int id)
{
    for (auto& unit : myunits) {
        if (unit->entityId == id) {
            return unit.get();
        }
    }
    for (auto& unit : enemys) {
        if (unit->entityId == id) {
            return unit.get();
        }
    }
    return nullptr;
}

void Game::requestPathForUnit(MoveableUnit& unit, Point goal)
{
    if (!isMapCell(unit.x, unit.y) || !isCellWalkableForUnit(goal.x, goal.y)) {
        return;
    }
    if (unit.pendingPathRequest != 0
        && unit.pendingPathGoal.x == goal.x
        && unit.pendingPathGoal.y == goal.y) {
        return;
    }

    PathRequest request;
    request.requestId = nextPathRequestId++;
    request.generation = pathGeneration;
    request.ownerId = unit.entityId;
    request.start = Point(unit.x, unit.y);
    request.goal = goal;
    request.allowDiagonal = false;
    request.maze = maze;
    const auto blockUnits = [&request, this, &unit](const std::list<std::unique_ptr<MoveableUnit>>& units) {
        for (const auto& other : units) {
            if (other->entityId == unit.entityId || other->Health <= 0 || !isMapCell(other->x, other->y)) {
                continue;
            }
            request.maze[other->y][other->x] = 1;
        }
    };
    // Realtime units do not write to tile IDs, so the path snapshot must mark
    // occupied cells explicitly to prevent same-cell stacking.
    blockUnits(myunits);
    blockUnits(enemys);
    request.maze[request.start.y][request.start.x] = 0;
    request.maze[goal.y][goal.x] = 0;
    unit.pendingPathRequest = request.requestId;
    unit.pendingPathGoal = goal;
    pathfinding.submit(std::move(request));
}

void Game::requestPathForWorker(Worker& worker, Point goal)
{
    if (!isMapCell(worker.point.x, worker.point.y) || !isCellWalkableForUnit(goal.x, goal.y)) {
        return;
    }
    if (worker.pendingPathRequest != 0) {
        return;
    }

    PathRequest request;
    request.requestId = nextPathRequestId++;
    request.generation = pathGeneration;
    request.ownerId = worker.id;
    request.start = worker.point;
    request.goal = goal;
    request.allowDiagonal = false;
    request.maze = maze;
    request.maze[request.start.y][request.start.x] = 0;
    request.maze[goal.y][goal.x] = 0;
    worker.pendingPathRequest = request.requestId;
    pathfinding.submit(std::move(request));
}

void Game::applyPathResults()
{
    for (auto& result : pathfinding.collectResults()) {
        if (result.generation != pathGeneration) {
            continue;
        }

        if (MoveableUnit* unit = findMoveableUnitById(result.ownerId)) {
            if (unit->pendingPathRequest == result.requestId) {
                unit->mypath = std::move(result.path);
                unit->pendingPathRequest = 0;
                unit->pendingPathGoal = result.goal;
            }
            continue;
        }

        if (Worker* worker = findWorkerById(result.ownerId)) {
            if (worker->pendingPathRequest == result.requestId) {
                worker->path = std::move(result.path);
                worker->pendingPathRequest = 0;
            }
        }
    }
}

void Game::logicBeforeDraw()
{

    // Update all units before drawing.

    if (realtimeMode && !perkOverlayVisible) {
        updateRealtime(std::min(realtimeFrameClock.restart().asSeconds(), 0.05f));
    }
    else if (realtimeMode) {
        realtimeFrameClock.restart();
    }

    bool timepassed = !realtimeMode && clock.getElapsedTime().asMilliseconds() > 30.f;
    if (timepassed) {
        for (auto& test : myunits) {
            if (test->UnitState == UState::MOVING)
            {

                if (!test->mypath.empty()) {
                    test->move(test->mypath.front());
                        if(!test->mypath.empty())
                    test->mypath.pop_front();
                }
                else {
                    test->setState(UState::UNITNORMAL);
                    running = false;
                }
            }
        }
        clock.restart();
    }
    if (Base_blue) {
        Base_blue->updatemystate();
    }
    if (Base_red) {
        Base_red->updatemystate();
    }
    for (auto u = myunits.begin(); u != myunits.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            awardKillBounty(AI, (*u)->unitName, Point((*u)->x, (*u)->y));
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = myunits.erase(u);
        }
        else {
            ++u;
        }
    }
    for (auto u = enemys.begin(); u != enemys.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            awardKillBounty(PLAYER, (*u)->unitName, Point((*u)->x, (*u)->y));
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = enemys.erase(u);
        }
        else {
            ++u;
        }
    }
}

void Game::logicAfterDraw()
{
}

void Game::run()
{

    while (window.isOpen())
    {

        window.clear();
        if(gameSceneState==SCENE_GAME)
            logicBeforeInput();


        Input();

        if (gameSceneState == SCENE_GAME)
            logicBeforeDraw();

        Draw();

        logicAfterDraw();


        window.display();


    }
}

void Game::Unitsreset(list<unique_ptr<MoveableUnit>>& us) {
    for (auto& u:us)
    {
        u->setdefalut();
    }
}

void Game::startInput(Vector2i mousePos, Event event) {
    startBtn.setPosition(470.f, 410.f);
    startHelpBtn.setPosition(710.f, 410.f);
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::H) {
            tutorialVisible = !tutorialVisible;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape && tutorialVisible) {
            tutorialVisible = false;
            return;
        }
    }
    if (startHelpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        startHelpBtn.setState(NORMAL);
        return;
    }
    if (tutorialVisible) {
        return;
    }

    if (startBtn.checkMouse(mousePos, event) == RELEASE) {
        gameSceneState = SCENE_GAME;
        startBtn.setState(NORMAL);
        clear();
    }
}

void Game::handleRealtimeMapClick(Vector2i mousePos, Event event)
{
    if (event.type != Event::EventType::MouseButtonPressed || event.mouseButton.button != Mouse::Left) {
        return;
    }
    if (mousePos.x < 0 || mousePos.x >= width || mousePos.y < 0 || mousePos.y >= height) {
        return;
    }

    const int tileX = mousePos.x / SqureSize;
    const int tileY = mousePos.y / SqureSize;
    for (const auto& node : resources) {
        if (isResourceClick(node, tileX, tileY)) {
            addFloatingText(sf::Vector2f(node.point.x * SqureSize, node.point.y * SqureSize - 14.f),
                            "Income: ECONOMY button", sf::Color(255, 226, 112), 12);
            return;
        }
    }

    // Economy is now a single natural-income track upgraded from the side
    // panel. Map clicks are reserved for selection and lane reading.
}

void Game::GameInput(Vector2i mousePos, Event event) {
    const bool mouseInMap = mousePos.x >= 0 && mousePos.x < width && mousePos.y >= 0 && mousePos.y < height;
    if (perkOverlayVisible) {
        handleRewardInput(mousePos, event);
        return;
    }
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::H) {
            tutorialVisible = !tutorialVisible;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape && tutorialVisible) {
            tutorialVisible = false;
            return;
        }
        if (event.key.code == sf::Keyboard::C)
        {
            clear();
            gameSceneState = SCENE_GAME;
        }
    }
    if (tutorialVisible) {
        return;
    }
    if (realtimeMode) {
        handleBuildButtons(mousePos, event);
        if (tutorialVisible) {
            return;
        }
        handleRealtimeMapClick(mousePos, event);
        if (Base_red) {
            Base_red->checkMouse(mousePos, event);
        }
        if (Base_blue) {
            Base_blue->checkHover(mousePos, event);
        }
        for (auto& u : enemys) {
            u->checkHover(mousePos, event);
        }
        if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T))
        {
            setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Tree);
        }
        if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M))
        {
            setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Mount);
        }
        if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        {
            setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::River);
        }
        if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
        {
            setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Empty);
        }
        return;
    }
    if (!running && playerturn) {
        EndTurnBtn.checkMouse(mousePos, event);
        if (EndTurnBtn.btnState == RELEASE) {
            playerturn = false;
            Globle_text.setString("EnemyTurn");
            clearSelection();
            addTurnIncome(AI);
            aiProductionDone = false;
            Unitsreset(myunits);
            if (Base_red) {
                Base_red->reset();
            }
            EndTurnBtn.setState(NORMAL);
        }
        else {
            handleBuildButtons(mousePos, event);
            if (tutorialVisible) {
                return;
            }
            if (Base_red) {
                Base_red->checkMouse(mousePos, event);
            }
            if (Base_blue) {
                Base_blue->checkHover(mousePos,event);
            }
            for (auto& u : enemys) {
                u->checkHover(mousePos, event);
            }
            if (!myunits.empty()) {
                for (auto &u:myunits)
                {
                   u->checkMouse(mousePos, event);
                }
            }
            if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T))
            {
                setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Tree);
            }
            if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M))
            {
                setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Mount);
            }
            if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            {
                setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::River);
            }
            if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            {
                setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Empty);
            }
        }
    }
}


void Game::Input()
{
    sf::Event event;
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    while (window.pollEvent(event))
    {
        if (event.type == Event::Closed) {
            window.close();
        }
        switch (gameSceneState) {
        case SCENE_START:
            startInput(mousePos, event); break;
        case SCENE_GAME:
            GameInput(mousePos, event); break;
        case SCEN_GAMEOVER:
            overinput(mousePos, event); break;
        default:
            break;
        }
    }
}

void Game::overinput(sf::Vector2i mousePos, sf::Event event) {
    endGame.setPosition(550.f, 372.f);
    tutorialVisible = false;
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::C)
        {
            clear();
            gameSceneState = SCENE_GAME;
        }
    }
    if (endGame.checkMouse(mousePos, event) == RELEASE) {
        clear();
        gameSceneState = SCENE_GAME;
        endGame.setState(NORMAL);
    }
}

bool Game::spawnUnit(int team, int name, int x, int y)
{
    if (!spendCommand(team, name)) {
        return false;
    }
    return createUnit(team, name, x, y, selectedLaneForTeam(team));
}

bool Game::createUnit(int team, int name, int x, int y, int laneIndex)
{
    if (!hasUnitCapacity(team)) {
        return false;
    }
    std::unique_ptr<MoveableUnit> unit;
    switch (name)
    {
    case UName::SHOOTER:
        unit = make_unique<Shooter>(team, x, y, this);
        break;
    case UName::INFANTARY:
        unit = make_unique<Infantry>(team, x, y, this);
        break;
    case UName::CAVALRY:
        unit = make_unique<Cavalry>(team, x, y, this);
        break;
    case UName::SIEGE:
        unit = make_unique<Siege>(team, x, y, this);
        break;
    case UName::GUARDIAN:
        unit = make_unique<Guardian>(team, x, y, this);
        break;
    default:
        return false;
    }

    unit->entityId = nextEntityId++;
    unit->laneIndex = std::clamp(laneIndex, 0, lane::Count - 1);
    unit->scaleMaxHealth(unitHealthMultiplier(team, name));
    if (team == PLAYER) {
        myunits.push_back(std::move(unit));
    }
    else {
        enemys.push_back(std::move(unit));
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " spawned unit=" + std::to_string(name)
        + " lane=" + laneName(laneIndex) + " at " + std::to_string(x) + "," + std::to_string(y));
    return true;
}

void Game::handleBuildButtons(Vector2i mousePos, Event event)
{
    handleLaneInput(mousePos, event);

    if (helpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        helpBtn.setState(NORMAL);
        return;
    }

    if (upgradeBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::UpgradeTech));
        upgradeBtn.setState(NORMAL);
    }

    if (economyBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::UpgradeEconomy));
        economyBtn.setState(NORMAL);
        return;
    }

    if (barracksBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, playerSelectedLane));
        barracksBtn.setState(NORMAL);
        return;
    }

    if (towerBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::BuildTower, playerSelectedLane));
        towerBtn.setState(NORMAL);
        return;
    }

    if (inf.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::INFANTARY));
        inf.setState(NORMAL);
    }
    if (sho.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::SHOOTER));
        sho.setState(NORMAL);
    }
    if (cav.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::CAVALRY));
        cav.setState(NORMAL);
    }
    if (siegeBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::SIEGE));
        siegeBtn.setState(NORMAL);
    }
    if (guardianBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::GUARDIAN));
        guardianBtn.setState(NORMAL);
    }
}

void Game::handleLaneInput(Vector2i mousePos, Event event)
{
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const float y = 190.f;
    const float w = 56.f;
    const float h = 24.f;
    for (int i = 0; i < lane::Count; ++i) {
        const sf::FloatRect rect(config::PanelX + 17.f + static_cast<float>(i) * 64.f, y, w, h);
        if (rect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
            playerSelectedLane = i;
            addFloatingText(sf::Vector2f(config::PanelX + 20.f, 146.f),
                            std::string("Lane: ") + laneName(i), sf::Color(218, 255, 134), 12);
            return;
        }
    }
}

void Game::Draw()
{
    switch (gameSceneState)
    {
    case SCENE_START: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        window.setView(defaultView);

        sf::RectangleShape sky(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        sky.setFillColor(sf::Color(24, 35, 31));
        window.draw(sky);

        for (int i = 0; i < 9; ++i) {
            sf::CircleShape haze(130.f + static_cast<float>(i % 3) * 34.f, 64);
            haze.setOrigin(haze.getRadius(), haze.getRadius());
            haze.setPosition(120.f + static_cast<float>(i) * 156.f, 90.f + static_cast<float>((i * 73) % 420));
            haze.setScale(1.45f, 0.48f);
            haze.setRotation(static_cast<float>((i * 19) % 360));
            haze.setFillColor(i % 2 == 0 ? sf::Color(219, 166, 75, 24) : sf::Color(78, 135, 115, 22));
            window.draw(haze);
        }

        sf::RectangleShape horizon(sf::Vector2f(static_cast<float>(config::WindowWidth), 210.f));
        horizon.setPosition(0.f, 510.f);
        horizon.setFillColor(sf::Color(34, 45, 36, 210));
        window.draw(horizon);

        sf::RectangleShape heroShadow(sf::Vector2f(720.f, 360.f));
        heroShadow.setPosition(310.f, 126.f);
        heroShadow.setFillColor(sf::Color(8, 12, 10, 118));
        window.draw(heroShadow);

        sf::RectangleShape hero(sf::Vector2f(720.f, 360.f));
        hero.setPosition(300.f, 112.f);
        hero.setFillColor(sf::Color(45, 58, 50, 232));
        hero.setOutlineColor(sf::Color(224, 171, 82, 210));
        hero.setOutlineThickness(2.2f);
        window.draw(hero);

        sf::Text title("COMMAND LINES", myfont, 48);
        title.setFillColor(sf::Color(255, 236, 176));
        title.setOutlineColor(sf::Color(30, 22, 14, 220));
        title.setOutlineThickness(2.f);
        title.setLetterSpacing(1.25f);
        title.setPosition(365.f, 146.f);
        window.draw(title);

        sf::Text subtitle("A 10-minute rogue RTS auto-battler", myfont, 18);
        subtitle.setFillColor(sf::Color(206, 224, 190));
        subtitle.setPosition(415.f, 214.f);
        window.draw(subtitle);

        const char* cards[] = {"1. Grow CMD", "2. Pick a lane", "3. Draft tactics"};
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape card(sf::Vector2f(188.f, 76.f));
            card.setPosition(368.f + static_cast<float>(i) * 204.f, 274.f);
            card.setFillColor(sf::Color(36, 46, 40, 230));
            card.setOutlineColor(i == 1 ? sf::Color(126, 190, 139, 210) : sf::Color(192, 141, 66, 190));
            card.setOutlineThickness(1.4f);
            window.draw(card);

            sf::CircleShape gem(12.f, 6);
            gem.setOrigin(12.f, 12.f);
            gem.setPosition(card.getPosition() + sf::Vector2f(24.f, 22.f));
            gem.setFillColor(i == 0 ? sf::Color(244, 199, 90) : (i == 1 ? sf::Color(109, 184, 138) : sf::Color(140, 184, 238)));
            window.draw(gem);

            sf::Text cardText(cards[i], myfont, 14);
            cardText.setFillColor(sf::Color(236, 232, 202));
            cardText.setPosition(card.getPosition() + sf::Vector2f(46.f, 15.f));
            window.draw(cardText);
        }

        startBtn.setPosition(470.f, 410.f);
        startHelpBtn.setPosition(710.f, 410.f);
        window.draw(startBtn);
        window.draw(startHelpBtn);

        sf::Text hint("Simple loop: Economy -> Barracks -> Pick a lane -> Draft upgrades", myfont, 13);
        hint.setFillColor(sf::Color(229, 214, 160));
        hint.setPosition(430.f, 516.f);
        window.draw(hint);

        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        break;
    }
    case SCENE_GAME: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        sf::View gameView(defaultView);
        gameView.move(currentShakeOffset());
        window.setView(gameView);

        for (const auto& tile : tiles)
            window.draw(tile);

        if (Base_red && Base_red->UnitState == UState::UNITCLICK) {
            RectangleShape rect;
            rect.setPosition(Base_red->getPosition());
            rect.setSize(sf::Vector2f(2 * SqureSize, 2 * SqureSize));
            rect.setFillColor(sf::Color::Transparent);
            rect.setOutlineColor(sf::Color::Red);
            rect.setOutlineThickness(2.f);
            window.draw(rect);
        }

        for (const auto& pathTile : drawPaths)
            window.draw(pathTile);
        drawGridOverlay();
        drawLaneGuides();
        drawResourceNodes();
        drawBuildings();
        drawWorkers();

        const auto drawBaseShield = [this](const DisMoveableUnit* base, int team) {
            const float seconds = baseShieldSecondsForTeam(team);
            if (base == nullptr || seconds <= 0.f) {
                return;
            }
            const float pulse = 0.5f + 0.5f * std::sin(gameTimeSeconds * 8.f);
            const sf::Color color = team == PLAYER ? sf::Color(255, 224, 112) : sf::Color(116, 184, 255);
            sf::CircleShape shield(31.f, 56);
            shield.setOrigin(31.f, 31.f);
            shield.setScale(1.12f + pulse * 0.05f, 0.88f + pulse * 0.04f);
            shield.setPosition(base->x * SqureSize + SqureSize, base->y * SqureSize + SqureSize);
            shield.setFillColor(sf::Color(color.r, color.g, color.b, 28));
            shield.setOutlineColor(sf::Color(color.r, color.g, color.b, static_cast<sf::Uint8>(150 + pulse * 75.f)));
            shield.setOutlineThickness(2.2f);
            window.draw(shield);
        };
        drawBaseShield(Base_red.get(), PLAYER);
        drawBaseShield(Base_blue.get(), AI);

        if (Base_red) {
            window.draw(*Base_red);
            window.draw(Base_red->UnitText);
        }
        if (Base_blue) {
            window.draw(*Base_blue);
            window.draw(Base_blue->UnitText);
        }
        const auto drawBasePerks = [this](const DisMoveableUnit* base, int team) {
            if (base == nullptr) {
                return;
            }
            const bool show = (team == PLAYER && base->UnitState == UState::UNITCLICK)
                || (team == AI && MosOnUnit == base);
            if (!show) {
                return;
            }
            std::string perks;
            for (int type = 0; type < perk::Count; ++type) {
                const int level = perkLevel(team, type);
                if (level <= 0) {
                    continue;
                }
                if (!perks.empty()) {
                    perks += " ";
                }
                perks += perkShortName(type);
                perks += std::to_string(level);
            }
            if (perks.empty()) {
                perks = "no perks";
            }
            const int shieldSeconds = static_cast<int>(std::ceil(baseShieldSecondsForTeam(team)));
            if (shieldSeconds > 0) {
                perks += " Shield";
                perks += std::to_string(shieldSeconds);
                perks += "s";
            }
            const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
            sf::Text status("Lv" + std::to_string(level) + " " + perks, myfont, 10);
            status.setFillColor(team == PLAYER ? sf::Color(255, 226, 142) : sf::Color(155, 203, 255));
            status.setOutlineColor(sf::Color(31, 24, 18, 220));
            status.setOutlineThickness(1.f);
            status.setPosition(base->x * SqureSize - 10.f, base->y * SqureSize - 28.f);
            window.draw(status);
        };
        drawBasePerks(Base_red.get(), PLAYER);
        drawBasePerks(Base_blue.get(), AI);

        UnitText.setString("");
        UnitAttack.setString("");
        UnitHP.setString("");

        for (const auto& u : enemys) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(61, 128, 206));
            window.draw(*u);
            window.draw(u->UnitText);
        }
        for (const auto& u : myunits) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(218, 76, 60));
            if (u->UnitState == UState::UNITCLICK) {
                MapPos selected(Point(u->x, u->y), tile::Choosen);
                window.draw(selected);
                string temp = "Action: "+to_string(u->myActionPoint);
                UnitText.setString(temp);
                temp = "Attack: " + to_string(u->myattack());
                UnitAttack.setString(temp);
                temp = "HP: " + to_string(u->Health);
                UnitHP.setString(temp);
            }
            window.draw(*u);
            window.draw(u->UnitText);
        }
        effects.draw(window);

        window.setView(defaultView);
        DrawSidePanel();
        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        if (perkOverlayVisible) {
            drawRewardOverlay();
        }
        
        break;
    }
    case SCEN_GAMEOVER: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        window.setView(defaultView);

        sf::RectangleShape backdrop(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        backdrop.setFillColor(gameWin ? sf::Color(30, 54, 43) : sf::Color(55, 39, 34));
        window.draw(backdrop);

        for (int i = 0; i < 7; ++i) {
            sf::CircleShape flare(115.f + static_cast<float>(i % 2) * 42.f, 60);
            flare.setOrigin(flare.getRadius(), flare.getRadius());
            flare.setPosition(168.f + static_cast<float>(i) * 176.f, 110.f + static_cast<float>((i * 91) % 430));
            flare.setScale(1.35f, 0.52f);
            flare.setFillColor(gameWin ? sf::Color(219, 184, 92, 28) : sf::Color(124, 88, 72, 34));
            window.draw(flare);
        }

        sf::RectangleShape panelShadow(sf::Vector2f(620.f, 344.f));
        panelShadow.setPosition(372.f, 154.f);
        panelShadow.setFillColor(sf::Color(8, 10, 9, 120));
        window.draw(panelShadow);

        sf::RectangleShape panel(sf::Vector2f(620.f, 344.f));
        panel.setPosition(360.f, 140.f);
        panel.setFillColor(sf::Color(39, 49, 43, 238));
        panel.setOutlineColor(gameWin ? sf::Color(239, 196, 102) : sf::Color(210, 113, 86));
        panel.setOutlineThickness(2.4f);
        window.draw(panel);

        sf::Text result(gameWin ? "VICTORY" : "DEFEAT", myfont, 50);
        result.setFillColor(gameWin ? sf::Color(255, 236, 168) : sf::Color(255, 184, 142));
        result.setOutlineColor(sf::Color(22, 18, 14, 230));
        result.setOutlineThickness(2.f);
        result.setLetterSpacing(1.25f);
        result.setPosition(gameWin ? 545.f : 560.f, 178.f);
        window.draw(result);

        sf::Text summary(gameWin ? "Your build order broke the enemy core." : "The enemy policy found a stronger timing.", myfont, 18);
        summary.setFillColor(sf::Color(226, 232, 202));
        summary.setPosition(gameWin ? 478.f : 475.f, 255.f);
        window.draw(summary);

        sf::Text stats("Time " + std::to_string(static_cast<int>(gameTimeSeconds)) + "s   Your Lv " + std::to_string(playerUpgradeLevel)
            + " / AI Lv " + std::to_string(aiUpgradeLevel), myfont, 15);
        stats.setFillColor(sf::Color(212, 204, 166));
        stats.setPosition(506.f, 306.f);
        window.draw(stats);

        endGame.setPosition(550.f, 372.f);
        window.draw(endGame);
        break;
    }
    default:
        break;
    }
    
}

void Game::drawGridOverlay()
{
    sf::VertexArray lines(sf::Lines);
    const sf::Color lineColor(108, 119, 115, 175);

    for (int x = 0; x <= width; x += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(height)), lineColor));
    }
    for (int y = 0; y <= height; y += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(width), static_cast<float>(y)), lineColor));
    }

    window.draw(lines);
}

void Game::drawLaneGuides()
{
    const int mapH = height / SqureSize;
    const int laneYs[] = {
        std::max(4, mapH / 4),
        mapH / 2,
        std::min(mapH - 5, mapH * 3 / 4)
    };
    const char* labels[] = {"TOP", "MID", "BOT"};

    for (int i = 0; i < lane::Count; ++i) {
        const float y = static_cast<float>(laneYs[i] * SqureSize + SqureSize / 2);
        sf::RectangleShape ribbon(sf::Vector2f(static_cast<float>(width), 5.f));
        ribbon.setOrigin(0.f, 2.5f);
        ribbon.setPosition(0.f, y);
        ribbon.setFillColor(i == playerSelectedLane ? sf::Color(255, 218, 112, 72) : sf::Color(205, 220, 190, 34));
        window.draw(ribbon);

        sf::CircleShape arrow(7.f, 3);
        arrow.setOrigin(7.f, 7.f);
        arrow.setPosition(static_cast<float>(Red_baseP.x * SqureSize + 95), y);
        arrow.setRotation(90.f);
        arrow.setFillColor(i == playerSelectedLane ? sf::Color(255, 218, 112, 150) : sf::Color(220, 230, 196, 90));
        window.draw(arrow);

        sf::Text laneText(labels[i], myfont, 11);
        laneText.setFillColor(i == playerSelectedLane ? sf::Color(255, 236, 168, 190) : sf::Color(213, 225, 198, 110));
        laneText.setOutlineColor(sf::Color(24, 29, 23, 140));
        laneText.setOutlineThickness(1.f);
        laneText.setPosition(static_cast<float>(Red_baseP.x * SqureSize + 18), y - 18.f);
        window.draw(laneText);
    }
}

void Game::drawResourceNodes()
{
    for (const auto& node : resources) {
        const sf::Vector2f center(
            node.point.x * SqureSize + SqureSize / 2.f,
            node.point.y * SqureSize + SqureSize / 2.f);
        const float t = node.pulseClock.getElapsedTime().asSeconds();
        const float pulse = 1.f + std::sin(t * 4.f) * 0.08f;
        const sf::Color brass = sf::Color(226, 180, 63);
        const sf::Color gold = sf::Color(255, 211, 82);

        sf::CircleShape shadow(11.f, 30);
        shadow.setOrigin(11.f, 11.f);
        shadow.setScale(1.35f, 0.42f);
        shadow.setPosition(center + sf::Vector2f(0.f, 7.f));
        shadow.setFillColor(sf::Color(22, 18, 10, 95));
        window.draw(shadow);

        sf::CircleShape aura(13.f * pulse, 40);
        aura.setOrigin(13.f * pulse, 13.f * pulse);
        aura.setPosition(center);
        aura.setFillColor(sf::Color(brass.r, brass.g, brass.b, 42));
        aura.setOutlineColor(sf::Color(brass.r, brass.g, brass.b, 178));
        aura.setOutlineThickness(1.2f);
        window.draw(aura);

        sf::RectangleShape beam(sf::Vector2f(4.f, 22.f));
        beam.setOrigin(2.f, 20.f);
        beam.setPosition(center + sf::Vector2f(0.f, -5.f));
        beam.setFillColor(sf::Color(255, 230, 122, 48));
        window.draw(beam);

        sf::ConvexShape crystal(8);
        crystal.setPoint(0, center + sf::Vector2f(0.f, -12.f));
        crystal.setPoint(1, center + sf::Vector2f(8.f, -7.f));
        crystal.setPoint(2, center + sf::Vector2f(10.f, 1.f));
        crystal.setPoint(3, center + sf::Vector2f(5.f, 9.f));
        crystal.setPoint(4, center + sf::Vector2f(0.f, 12.f));
        crystal.setPoint(5, center + sf::Vector2f(-5.f, 9.f));
        crystal.setPoint(6, center + sf::Vector2f(-10.f, 1.f));
        crystal.setPoint(7, center + sf::Vector2f(-8.f, -7.f));
        crystal.setFillColor(gold);
        crystal.setOutlineColor(sf::Color(brass.r, brass.g, brass.b, 235));
        crystal.setOutlineThickness(1.4f);
        window.draw(crystal);

        sf::CircleShape core(3.2f, 18);
        core.setOrigin(3.2f, 3.2f);
        core.setPosition(center + sf::Vector2f(0.f, -1.f));
        core.setFillColor(sf::Color(255, 250, 180, 210));
        window.draw(core);

        sf::RectangleShape labelBg(sf::Vector2f(29.f, 11.f));
        labelBg.setPosition(center.x + 9.f, center.y - 17.f);
        labelBg.setFillColor(sf::Color(40, 32, 19, 185));
        labelBg.setOutlineColor(sf::Color(255, 225, 128, 170));
        labelBg.setOutlineThickness(0.7f);
        window.draw(labelBg);

        sf::Text label("CMD", myfont, 8);
        label.setFillColor(sf::Color(255, 235, 145));
        label.setPosition(labelBg.getPosition() + sf::Vector2f(4.f, 0.f));
        window.draw(label);
    }
}

void Game::drawBuildings()
{
    for (const auto& building : buildings) {
        const sf::Vector2f origin(building.point.x * SqureSize, building.point.y * SqureSize);
        if (!building.complete) {
            const float pct = std::clamp(building.buildProgress / std::max(0.1f, building.buildSeconds), 0.f, 1.f);
            sf::RectangleShape barBg(sf::Vector2f(18.f, 3.f));
            barBg.setPosition(origin + sf::Vector2f(1.f, -5.f));
            barBg.setFillColor(sf::Color(25, 25, 22, 170));
            window.draw(barBg);
            sf::RectangleShape bar(sf::Vector2f(18.f * pct, 3.f));
            bar.setPosition(barBg.getPosition());
            bar.setFillColor(building.team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255));
            window.draw(bar);
        }
        else if (building.type == building::Barracks && building.production.activeUnit >= 0) {
            const float trainSeconds = unitTrainSeconds(building.production.activeUnit) * teamTrainTimeMultiplier(building.team);
            const float pct = std::clamp(building.production.progress / trainSeconds, 0.f, 1.f);
            sf::RectangleShape bar(sf::Vector2f(18.f * pct, 2.f));
            bar.setPosition(origin + sf::Vector2f(1.f, -4.f));
            bar.setFillColor(sf::Color(255, 218, 112));
            window.draw(bar);
        }
        if (building.complete && building.type == building::Barracks && building.production.load() > 0) {
            sf::Text queueText("Q" + std::to_string(building.production.load()), myfont, 9);
            queueText.setFillColor(sf::Color(255, 241, 177));
            queueText.setOutlineColor(sf::Color(44, 32, 24, 210));
            queueText.setOutlineThickness(0.8f);
            queueText.setPosition(origin + sf::Vector2f(2.f, 10.f));
            window.draw(queueText);
        }

        if (building.maxHealth > 0 && building.health < building.maxHealth) {
            const float pct = std::clamp(static_cast<float>(building.health) / static_cast<float>(building.maxHealth), 0.f, 1.f);
            sf::RectangleShape hpBg(sf::Vector2f(18.f, 2.f));
            hpBg.setPosition(origin + sf::Vector2f(1.f, 18.f));
            hpBg.setFillColor(sf::Color(42, 23, 20, 190));
            window.draw(hpBg);
            sf::RectangleShape hpBar(sf::Vector2f(18.f * pct, 2.f));
            hpBar.setPosition(hpBg.getPosition());
            hpBar.setFillColor(sf::Color(112, 230, 118));
            window.draw(hpBar);
        }
    }
}

void Game::drawWorkers()
{
    for (const auto& workerUnit : workers) {
        const sf::Vector2f center(workerUnit.point.x * SqureSize + SqureSize / 2.f, workerUnit.point.y * SqureSize + SqureSize / 2.f);
        sf::CircleShape body(5.f, 18);
        body.setOrigin(5.f, 5.f);
        body.setPosition(center);
        body.setFillColor(workerUnit.team == PLAYER ? sf::Color(255, 187, 87) : sf::Color(118, 178, 255));
        body.setOutlineColor(sf::Color(39, 35, 26));
        body.setOutlineThickness(1.f);
        window.draw(body);

        if (workerUnit.state == worker::Building) {
            sf::RectangleShape tool(sf::Vector2f(8.f, 1.6f));
            tool.setOrigin(4.f, 0.8f);
            tool.setPosition(center + sf::Vector2f(3.f, -5.f));
            tool.setRotation(35.f);
            tool.setFillColor(sf::Color(255, 241, 177));
            window.draw(tool);
        }
    }
}

void Game::drawTutorialOverlay()
{
    const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
    window.setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(8, 11, 10, 176));
    window.draw(veil);

    sf::RectangleShape shadow(sf::Vector2f(820.f, 620.f));
    shadow.setPosition(196.f, 56.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 92));
    window.draw(shadow);

    sf::RectangleShape card(sf::Vector2f(820.f, 620.f));
    card.setPosition(184.f, 44.f);
    card.setFillColor(sf::Color(39, 49, 43, 245));
    card.setOutlineColor(sf::Color(224, 170, 76));
    card.setOutlineThickness(3.f);
    window.draw(card);

    sf::RectangleShape header(sf::Vector2f(820.f, 68.f));
    header.setPosition(184.f, 44.f);
    header.setFillColor(sf::Color(86, 61, 35, 238));
    window.draw(header);

    sf::Text title("HOW TO PLAY", myfont, 31);
    title.setFillColor(sf::Color(255, 243, 201));
    title.setPosition(222.f, 59.f);
    window.draw(title);

    sf::Text closeHint("Press H or Esc to close", myfont, 14);
    closeHint.setFillColor(sf::Color(255, 226, 142));
    closeHint.setPosition(790.f, 75.f);
    window.draw(closeHint);

    const std::vector<std::string> lines = {
        "Goal",
        "  Build an economy, pick a lane, draft rogue tactics, then auto-push the enemy base.",
        "",
        "Core controls",
        "  Click ECONOMY: improve natural CMD income and add one visible drone.",
        "  Click Top / Mid / Bot, then click unit buttons to send new troops to that lane.",
        "  Barracks and Tower buttons auto-place buildings near your base or lane defense.",
        "  Click Upgrade: spend CMD to gain a LEVEL and choose one rogue tactic card.",
        "",
        "Automation",
        "  CMD comes from natural income and kill bounties based on enemy unit cost.",
        "  Drones are your economy/readability meter and auto-build nearby structures.",
        "  Combat units auto-path down highlighted lanes, fight enemies, then raid buildings.",
        "  Towers beat basic attacks, but Siege outranges towers and forces a response.",
        "  Main bases have a timed shield; siege pushes are the clean finisher.",
        "  If all Barracks fall, the base slowly drafts emergency troops.",
        "  Lost structures refund CMD and trigger a short HQ shield so you can rebuild.",
        "",
        "Tactics and counters",
        "  Every tech upgrade gives 3 tactic cards. Max tech is LEVEL 15.",
        "  Perks stack, so late builds can bend the soft counter rules.",
        "  Shooter > Infantry, Infantry > Cavalry, Cavalry > Shooter/Siege.",
        "  Cavalry rotates fastest; Siege is very slow and needs escorts.",
        "  Siege cracks Guardians and buildings; Guardians anchor against Cavalry dives.",
        "",
        "Unlocks",
        "  Infantry: 15 CMD, needs 1 Barracks.",
        "  Shooter: 22 CMD, needs 1 Barracks and Economy 1 or LEVEL 1.",
        "  Cavalry: 38 CMD, needs 2 Barracks and Economy 2 or LEVEL 3.",
        "  Siege: 56 CMD, needs LEVEL 5, 2 Barracks, Economy 3; outranges towers.",
        "  Guardian: 74 CMD, needs LEVEL 7, 3 Barracks, Economy 4; anchors pushes.",
        "",
        "Hotkeys",
        "  H: show / hide this guide.  C: restart map.  Esc: back to menu."
    };

    float y = 126.f;
    for (const auto& line : lines) {
        const bool section = !line.empty() && line.front() != ' ';
        sf::Text text(line, myfont, section ? 16 : 12);
        text.setFillColor(section ? sf::Color(255, 218, 112) : sf::Color(224, 232, 203));
        text.setPosition(228.f, y);
        window.draw(text);
        y += line.empty() ? 7.f : (section ? 22.f : 17.f);
    }
}

void Game::drawRewardOverlay()
{
    const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
    window.setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(7, 9, 8, 142));
    window.draw(veil);

    sf::RectangleShape panelShadow(sf::Vector2f(890.f, 330.f));
    panelShadow.setPosition(205.f, 194.f);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 105));
    window.draw(panelShadow);

    sf::RectangleShape panel(sf::Vector2f(890.f, 330.f));
    panel.setPosition(195.f, 184.f);
    panel.setFillColor(sf::Color(37, 48, 41, 248));
    panel.setOutlineColor(sf::Color(232, 177, 77));
    panel.setOutlineThickness(2.4f);
    window.draw(panel);

    sf::Text title("Choose A Battle Tactic", myfont, 29);
    title.setFillColor(sf::Color(255, 239, 190));
    title.setPosition(236.f, 210.f);
    window.draw(title);

    sf::Text hint("Rewards are small and capped, so every unit keeps a role. Press 1/2/3 or click a card.", myfont, 14);
    hint.setFillColor(sf::Color(222, 230, 204));
    hint.setPosition(236.f, 250.f);
    window.draw(hint);

    for (int i = 0; i < static_cast<int>(perkChoices.size()); ++i) {
        const sf::Vector2f pos(235.f + static_cast<float>(i) * 278.f, 295.f);
        sf::RectangleShape card(sf::Vector2f(250.f, 170.f));
        card.setPosition(pos);
        card.setFillColor(i == 1 ? sf::Color(63, 55, 35, 246) : sf::Color(47, 58, 51, 246));
        card.setOutlineColor(i == 1 ? sf::Color(255, 218, 112) : sf::Color(120, 137, 104));
        card.setOutlineThickness(2.f);
        window.draw(card);

        sf::CircleShape badge(17.f, 24);
        badge.setOrigin(17.f, 17.f);
        badge.setPosition(pos + sf::Vector2f(30.f, 30.f));
        const sf::Color badgeColor[] = {
            sf::Color(255, 211, 82),
            sf::Color(136, 207, 255),
            sf::Color(255, 146, 92)
        };
        badge.setFillColor(badgeColor[i]);
        badge.setOutlineColor(sf::Color(39, 35, 26));
        badge.setOutlineThickness(1.4f);
        window.draw(badge);

        sf::Text number(std::to_string(i + 1), myfont, 15);
        number.setFillColor(sf::Color(38, 32, 22));
        number.setPosition(pos.x + 25.f, pos.y + 19.f);
        window.draw(number);

        const auto& choice = perkChoices[static_cast<std::size_t>(i)];
        sf::Text name(choice.title, myfont, 20);
        name.setFillColor(sf::Color(255, 239, 190));
        name.setPosition(pos + sf::Vector2f(56.f, 20.f));
        window.draw(name);

        sf::Text desc(choice.description, myfont, 14);
        desc.setFillColor(sf::Color(224, 232, 203));
        desc.setPosition(pos + sf::Vector2f(22.f, 68.f));
        desc.setLineSpacing(1.2f);
        window.draw(desc);

        const int level = perkLevel(PLAYER, choice.type);
        const std::string levelText = choice.type == perk::WarChest
            ? "Instant tempo"
            : ("Level " + std::to_string(level) + "/" + std::to_string(maxPerkLevel(choice.type)));
        sf::Text meta(levelText, myfont, 12);
        meta.setFillColor(sf::Color(255, 218, 112));
        meta.setPosition(pos + sf::Vector2f(22.f, 138.f));
        window.draw(meta);
    }
}

void Game::handleRewardInput(sf::Vector2i mousePos, sf::Event event)
{
    if (!perkOverlayVisible) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1 || event.key.code == sf::Keyboard::Enter) {
            applyRewardChoice(0);
        }
        else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
            applyRewardChoice(1);
        }
        else if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            applyRewardChoice(2);
        }
        return;
    }

    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const sf::Vector2f point(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    for (int i = 0; i < static_cast<int>(perkChoices.size()); ++i) {
        const sf::FloatRect card(235.f + static_cast<float>(i) * 278.f, 295.f, 250.f, 170.f);
        if (card.contains(point)) {
            applyRewardChoice(i);
            return;
        }
    }
}

void Game::addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    effects.addAttack(start, end, color);
}

void Game::addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size)
{
    effects.addFloatingText(myfont, position, value, color, size);
}

void Game::startScreenShake(float durationSeconds, float intensity)
{
    effects.startShake(durationSeconds, intensity);
}

sf::Vector2f Game::currentShakeOffset() const
{
    return effects.shakeOffset();
}

void Game::DrawSidePanel()
{
    helpBtn.setPosition(config::ButtonX, config::HelpButtonY);
    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    upgradeBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    economyBtn.setPosition(config::ButtonX, config::EconomyButtonY);
    barracksBtn.setPosition(config::ButtonX, config::BuildBarracksY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
    siegeBtn.setPosition(config::ButtonX, config::BuildSiegeY);
    guardianBtn.setPosition(config::ButtonX, config::BuildGuardianY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);

    window.draw(sidePanel);

    sf::RectangleShape accentLine(sf::Vector2f(4.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(219, 166, 75));
    window.draw(accentLine);

    sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(config::PanelWidth), 120.f));
    topGlow.setPosition(static_cast<float>(config::PanelX), 0.f);
    topGlow.setFillColor(sf::Color(255, 222, 138, 14));
    window.draw(topGlow);

    const float panelLeft = static_cast<float>(config::PanelX + 12);
    const float cardWidth = static_cast<float>(config::PanelWidth - 24);
    const auto drawPanelCard = [this, panelLeft, cardWidth](float y, float h, sf::Color fill, sf::Color outline, const std::string& title) {
        sf::RectangleShape shadow(sf::Vector2f(cardWidth, h));
        shadow.setPosition(panelLeft + 2.f, y + 4.f);
        shadow.setFillColor(sf::Color(8, 11, 10, 86));
        window.draw(shadow);

        sf::RectangleShape card(sf::Vector2f(cardWidth, h));
        card.setPosition(panelLeft, y);
        card.setFillColor(fill);
        card.setOutlineColor(outline);
        card.setOutlineThickness(1.4f);
        window.draw(card);

        if (!title.empty()) {
            sf::RectangleShape titleBar(sf::Vector2f(cardWidth - 16.f, 1.4f));
            titleBar.setPosition(panelLeft + 8.f, y + 21.f);
            titleBar.setFillColor(sf::Color(outline.r, outline.g, outline.b, 120));
            window.draw(titleBar);

            sf::Text titleText(title, myfont, 10);
            titleText.setFillColor(sf::Color(255, 232, 156));
            titleText.setLetterSpacing(1.18f);
            titleText.setPosition(panelLeft + 10.f, y + 5.f);
            window.draw(titleText);
        }
    };

    drawPanelCard(8.f, 70.f, sf::Color(47, 58, 51), sf::Color(107, 118, 91), "COMMAND");
    drawPanelCard(84.f, 136.f, sf::Color(40, 50, 45), sf::Color(84, 99, 78), "STATUS");
    drawPanelCard(222.f, 96.f, sf::Color(61, 48, 31), sf::Color(205, 156, 70), "");
    drawPanelCard(322.f, 340.f, sf::Color(42, 49, 44), sf::Color(86, 98, 75), "");
    drawPanelCard(668.f, 44.f, sf::Color(37, 45, 41), sf::Color(86, 98, 75), "");

    panelTitle.setCharacterSize(20);
    panelTitle.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 28.f);
    Globle_text.setCharacterSize(13);
    Globle_text.setFillColor(sf::Color(221, 211, 177));
    Globle_text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 54.f);
    window.draw(panelTitle);
    window.draw(Globle_text);

    if (!realtimeMode) {
        window.draw(UnitText);
        window.draw(UnitAttack);
        window.draw(UnitHP);
    }

    const bool inspectingEnemyBase = MosOnUnit == Base_blue.get();
    const int shownTeam = inspectingEnemyBase ? AI : PLAYER;
    const int shownLevel = shownTeam == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const auto perkLine = [this, shownTeam]() {
        std::string text;
        for (int type = 0; type < perk::Count; ++type) {
            const int level = perkLevel(shownTeam, type);
            if (level <= 0) {
                continue;
            }
            if (!text.empty()) {
                text += " ";
            }
            text += perkShortName(type);
            text += std::to_string(level);
        }
        return text.empty() ? std::string("none") : text;
    };
    const auto clampText = [](std::string text, std::size_t maxChars) {
        if (text.size() <= maxChars) {
            return text;
        }
        if (maxChars <= 2) {
            return text.substr(0, maxChars);
        }
        text.resize(maxChars - 2);
        text += "..";
        return text;
    };

    CommandText.setCharacterSize(realtimeMode ? 11 : 14);
    CommandText.setFillColor(sf::Color(255, 226, 128));
    CommandText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), realtimeMode ? 106.f : 176.f);
    CommandText.setString("CMD " + std::to_string(playerCommand)
        + "/" + std::to_string(config::MaxCommand)
        + "   +" + std::to_string(resourceIncome(PLAYER)) + "/tick"
        + "\nTech P " + std::to_string(playerUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "  AI " + std::to_string(aiUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "\nEco  P " + std::to_string(playerEconomyLevel)
        + "/" + std::to_string(config::MaxEconomyLevel)
        + "  AI " + std::to_string(aiEconomyLevel)
        + "/" + std::to_string(config::MaxEconomyLevel)
        + "\nDrone " + std::to_string(workerCount(PLAYER))
        + "/" + std::to_string(realtime::MaxWorkers)
        + "  Army " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits)
        + "\nRax " + std::to_string(completedBuildingCount(PLAYER, building::Barracks))
        + "/" + std::to_string(buildingCap(PLAYER, building::Barracks))
        + "  Tower " + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower))
        + "/" + std::to_string(buildingCap(PLAYER, building::DefenseTower)));
    window.draw(CommandText);

    sf::Text perkText(std::string(inspectingEnemyBase ? "Enemy" : "Your") + " Lv" + std::to_string(shownLevel)
        + " buffs: " + clampText(perkLine(), 24), myfont, 10);
    perkText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(255, 226, 142));
    perkText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 169.f);
    window.draw(perkText);

    const float laneY = 190.f;
    int playerLaneCounts[lane::Count] = {};
    int aiLaneCounts[lane::Count] = {};
    for (const auto& unit : myunits) {
        if (unit->Health > 0) {
            ++playerLaneCounts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
        }
    }
    for (const auto& unit : enemys) {
        if (unit->Health > 0) {
            ++aiLaneCounts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
        }
    }
    for (int i = 0; i < lane::Count; ++i) {
        sf::RectangleShape laneButton(sf::Vector2f(56.f, 24.f));
        laneButton.setPosition(config::PanelX + 17.f + static_cast<float>(i) * 64.f, laneY);
        laneButton.setFillColor(playerSelectedLane == i ? sf::Color(217, 166, 75) : sf::Color(48, 60, 52));
        laneButton.setOutlineColor(playerSelectedLane == i ? sf::Color(255, 236, 164) : sf::Color(111, 128, 99));
        laneButton.setOutlineThickness(playerSelectedLane == i ? 1.8f : 0.9f);
        window.draw(laneButton);

        sf::Text laneText(laneName(i), myfont, 10);
        laneText.setFillColor(playerSelectedLane == i ? sf::Color(41, 31, 20) : sf::Color(224, 232, 203));
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(7.f, 1.f));
        window.draw(laneText);

        sf::Text laneCount(std::to_string(playerLaneCounts[i]) + "/" + std::to_string(aiLaneCounts[i]), myfont, 8);
        laneCount.setFillColor(playerSelectedLane == i ? sf::Color(64, 45, 23) : sf::Color(205, 214, 188));
        laneCount.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 14.f));
        window.draw(laneCount);
    }

    const auto guideText = [this]() {
        if (playerEconomyLevel == 0) {
            return std::string("Next: ECONOMY first");
        }
        if (completedBuildingCount(PLAYER, building::Barracks) == 0) {
            return std::string("Next: build Barracks");
        }
        if (playerUpgradeLevel < 1 && commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)) {
            return std::string("Next: Upgrade for cards");
        }
        if (totalBuildingCount(AI, building::DefenseTower) > 0 && !isUnitUnlocked(PLAYER, UName::SIEGE)) {
            return std::string("Enemy tower: tech Siege");
        }
        if (myunits.size() + 4 < enemys.size()) {
            return std::string("Under pressure: queue units");
        }
        if (playerEconomyLevel < config::MaxEconomyLevel && commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)) {
            return std::string("Float CMD: buy ECONOMY");
        }
        return std::string("Pick lane, keep queues busy");
    };
    panelHint.setCharacterSize(10);
    panelHint.setFillColor(sf::Color(219, 209, 174));
    panelHint.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 203.f);
    panelHint.setString(guideText());
    window.draw(panelHint);

    if (!realtimeMode) {
        window.draw(EndTurnBtn);
    }
    else {
        upgradeBtn.setColor(commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
            && playerUpgradeLevel < config::MaxTechLevel
            ? sf::Color::White
            : sf::Color(255, 255, 255, 130));
        window.draw(upgradeBtn);
    }

    economyBtn.setColor(commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));
    window.draw(economyBtn);

    economyLabel.setCharacterSize(9);
    economyLabel.setFillColor(sf::Color(244, 221, 150));
    economyLabel.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EconomyButtonY + config::SideButtonHeight - 2.f);
    economyLabel.setString("Cost " + std::to_string(economyUpgradeCost(PLAYER))
        + " | +" + std::to_string(config::EconomyIncomeStep) + "/tick +drone");
    window.draw(economyLabel);

    sf::Text upgradeCost("Cost " + std::to_string(upgradeCostForNextLevel(PLAYER)) + " | Level gives 3 cards", myfont, 9);
    upgradeCost.setFillColor(sf::Color(244, 221, 150));
    upgradeCost.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EndTurnButtonY + config::SideButtonHeight - 2.f);
    window.draw(upgradeCost);

    const bool canBuildBarracks = commandForTeam(PLAYER) >= config::BarracksCost
        && totalBuildingCount(PLAYER, building::Barracks) < buildingCap(PLAYER, building::Barracks);
    inf.setColor(canQueueUnit(PLAYER, UName::INFANTARY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    sho.setColor(canQueueUnit(PLAYER, UName::SHOOTER) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    cav.setColor(canQueueUnit(PLAYER, UName::CAVALRY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    siegeBtn.setColor(canQueueUnit(PLAYER, UName::SIEGE) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    guardianBtn.setColor(canQueueUnit(PLAYER, UName::GUARDIAN) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    barracksBtn.setColor(canBuildBarracks ? sf::Color::White : sf::Color(255, 255, 255, 130));
    const bool canQueueTower = commandForTeam(PLAYER) >= config::TowerCost
        && totalBuildingCount(PLAYER, building::DefenseTower) < buildingCap(PLAYER, building::DefenseTower);
    towerBtn.setColor(canQueueTower ? sf::Color::White : sf::Color(255, 255, 255, 130));

    const auto setLabel = [this](sf::Text& text, const std::string& value, int buttonY) {
        text.setCharacterSize(8);
        text.setFillColor(sf::Color(221, 211, 177));
        text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), static_cast<float>(buttonY + config::SideButtonHeight - 1));
        text.setString(value);
    };
    setLabel(barracksLabel, std::to_string(config::BarracksCost) + " | cap "
        + std::to_string(totalBuildingCount(PLAYER, building::Barracks)) + "/"
        + std::to_string(buildingCap(PLAYER, building::Barracks)) + " auto near base", config::BuildBarracksY);
    setLabel(infantryLabel, std::to_string(config::InfantryCost) + " | steady frontline", config::BuildInfantryY);
    setLabel(shooterLabel, std::to_string(config::ShooterCost) + " | ranged, slower", config::BuildShooterY);
    setLabel(cavalryLabel, std::to_string(config::CavalryCost) + " | fastest dive", config::BuildCavalryY);
    setLabel(siegeLabel, std::to_string(config::SiegeCost) + " | very slow tower-breaker", config::BuildSiegeY);
    setLabel(guardianLabel, std::to_string(config::GuardianCost) + " | slow heavy tank", config::BuildGuardianY);
    setLabel(towerLabel, std::to_string(config::TowerCost) + " | cap "
        + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower)) + "/"
        + std::to_string(buildingCap(PLAYER, building::DefenseTower)) + " anti-rush", config::BuildTowerY);

    window.draw(helpBtn);
    window.draw(barracksBtn);
    window.draw(barracksLabel);
    window.draw(inf);
    window.draw(infantryLabel);
    window.draw(sho);
    window.draw(shooterLabel);
    window.draw(cav);
    window.draw(cavalryLabel);
    window.draw(siegeBtn);
    window.draw(siegeLabel);
    window.draw(guardianBtn);
    window.draw(guardianLabel);
    window.draw(towerBtn);
    window.draw(towerLabel);
}

void Game::clear()
{
    effects.clear();
    drawPaths.clear();
    running = false;
    playerturn = true;
    gameWin = false;
    MosOnUnit = nullptr;
    playerCommand = config::StartingCommand;
    aiCommand = config::StartingCommand;
    aiProductionDone = false;
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
    Globle_text.setString(realtimeMode ? "RealTime" : "YourTurn");
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

int Game::resourceIncome(int team) const
{
    const int level = economyLevelForTeam(team);
    const float multiplier = miningIncomeMultiplier(team);
    int income = config::BaseCommandIncome
        + static_cast<int>(std::round(static_cast<float>(level * config::EconomyIncomeStep) * multiplier));
    if (team == AI) {
        // A mild difficulty stipend keeps heuristic AI competitive without
        // adding hidden resource types for the player to understand.
        income += 1 + static_cast<int>(gameTimeSeconds / 300.f);
    }
    return income;
}

int Game::economyLevelForTeam(int team) const
{
    return team == PLAYER ? playerEconomyLevel : aiEconomyLevel;
}

int Game::economyUpgradeCost(int team) const
{
    const int level = economyLevelForTeam(team);
    if (level >= config::MaxEconomyLevel) {
        return 0;
    }
    int cost = config::EconomyUpgradeCost
        + level * config::EconomyUpgradeCostStep
        + level * level * 3;
    if (team == AI && gameTimeSeconds > 840.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.52f));
    }
    else if (team == AI && gameTimeSeconds > 720.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.65f));
    }
    else if (team == AI && gameTimeSeconds > 420.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.78f));
    }
    return std::max(20, cost);
}

int Game::perkLevel(int team, int type) const
{
    if (type < 0 || type >= perk::Count) {
        return 0;
    }
    return team == PLAYER ? playerPerkLevels[static_cast<std::size_t>(type)]
        : aiPerkLevels[static_cast<std::size_t>(type)];
}

void Game::buildRewardChoices()
{
    const int rotation[] = {
        perk::Drill,
        perk::Fortitude,
        perk::Volley,
        perk::Logistics,
        perk::Mining,
        perk::WarChest,
        perk::Charge,
        perk::SiegeCraft,
        perk::TowerCraft
    };
    const int rotationSize = static_cast<int>(sizeof(rotation) / sizeof(rotation[0]));
    bool used[perk::Count] = {};
    int cursor = rewardSequence % rotationSize;

    for (auto& choice : perkChoices) {
        int selected = perk::WarChest;
        for (int attempts = 0; attempts < rotationSize * 2; ++attempts) {
            const int candidate = rotation[(cursor + attempts) % rotationSize];
            if (!used[candidate] && perkLevel(PLAYER, candidate) < maxPerkLevel(candidate)) {
                selected = candidate;
                cursor = (cursor + attempts + 1) % rotationSize;
                break;
            }
        }
        used[selected] = true;
        choice.type = selected;
        choice.title = perkTitle(selected);
        choice.description = perkDescription(selected);
    }

    ++rewardSequence;
}

void Game::applyPerk(int team, int type)
{
    if (type < 0 || type >= perk::Count) {
        return;
    }

    if (type == perk::WarChest) {
        auto& levels = team == PLAYER ? playerPerkLevels : aiPerkLevels;
        levels[static_cast<std::size_t>(type)] += 1;
        const int bonus = 38 + (team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * 5;
        commandPool(*this, team) = std::min(config::MaxCommand, commandPool(*this, team) + bonus);
        Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
        if (base != nullptr) {
            addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 30.f),
                            "War Chest +" + std::to_string(bonus), team == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 13);
        }
        return;
    }

    auto& levels = team == PLAYER ? playerPerkLevels : aiPerkLevels;
    int& level = levels[static_cast<std::size_t>(type)];
    if (level >= maxPerkLevel(type)) {
        return;
    }
    ++level;

    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        if (type == perk::Fortitude && (unit->unitName == UName::INFANTARY || unit->unitName == UName::GUARDIAN)) {
            unit->scaleMaxHealth(1.09f);
        }
        else if (type == perk::Charge && unit->unitName == UName::CAVALRY) {
            unit->scaleMaxHealth(1.04f);
        }
        else if (type == perk::SiegeCraft && unit->unitName == UName::SIEGE) {
            unit->scaleMaxHealth(1.04f);
        }
    }

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 30.f),
                        std::string(perkTitle(type)) + " Lv" + std::to_string(level),
                        team == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 13);
    }
}

void Game::applyRewardChoice(int index)
{
    if (index < 0 || index >= static_cast<int>(perkChoices.size())) {
        return;
    }
    applyPerk(PLAYER, perkChoices[static_cast<std::size_t>(index)].type);
    perkOverlayVisible = false;
}

void Game::maybeGrantReward(int team, const std::string& reason)
{
    if (team == PLAYER) {
        if (perkOverlayVisible) {
            return;
        }
        buildRewardChoices();
        if (autoChooseRewards) {
            applyRewardChoice(0);
        }
        else {
            perkOverlayVisible = true;
            addFloatingText(sf::Vector2f(config::PanelX + 16.f, 42.f),
                            "Tactic ready", sf::Color(255, 226, 112), 13);
        }
        logEvent("player reward: " + reason);
        return;
    }

    int choice = perk::WarChest;
    const bool playerTurtling = totalBuildingCount(PLAYER, building::DefenseTower) > 0
        || totalBuildingCount(PLAYER, building::Barracks) >= 3;
    const bool aiMacroBehind = aiEconomyLevel < std::min(config::MaxEconomyLevel, 3 + static_cast<int>(gameTimeSeconds / 140.f));
    if (aiMacroBehind && perkLevel(AI, perk::Mining) < maxPerkLevel(perk::Mining)) {
        choice = perk::Mining;
    }
    else if ((playerTurtling || gameTimeSeconds > 720.f) && perkLevel(AI, perk::SiegeCraft) < maxPerkLevel(perk::SiegeCraft)) {
        choice = perk::SiegeCraft;
    }
    else if (completedBuildingCount(AI, building::Barracks) >= 2 && perkLevel(AI, perk::Logistics) < maxPerkLevel(perk::Logistics)) {
        choice = perk::Logistics;
    }
    else if (countUnitsNamed(myunits, UName::SHOOTER) > countUnitsNamed(myunits, UName::INFANTARY)
        && perkLevel(AI, perk::Charge) < maxPerkLevel(perk::Charge)) {
        choice = perk::Charge;
    }
    else if (isUnitUnlocked(AI, UName::SIEGE) && perkLevel(AI, perk::SiegeCraft) < maxPerkLevel(perk::SiegeCraft)) {
        choice = perk::SiegeCraft;
    }
    else if (isUnitUnlocked(AI, UName::GUARDIAN) && perkLevel(AI, perk::Fortitude) < maxPerkLevel(perk::Fortitude)) {
        choice = perk::Fortitude;
    }
    else if (isUnitUnlocked(AI, UName::GUARDIAN) && perkLevel(AI, perk::Drill) < maxPerkLevel(perk::Drill)) {
        choice = perk::Drill;
    }
    else if (completedBuildingCount(AI, building::DefenseTower) > 0 && perkLevel(AI, perk::TowerCraft) < maxPerkLevel(perk::TowerCraft)) {
        choice = perk::TowerCraft;
    }
    else if (perkLevel(AI, perk::Volley) < maxPerkLevel(perk::Volley)) {
        choice = perk::Volley;
    }

    applyPerk(AI, choice);
    logEvent("ai reward: " + reason + " perk=" + std::to_string(choice));
}

int Game::unitCost(int name) const
{
    switch (name) {
    case UName::INFANTARY:
        return config::InfantryCost;
    case UName::SHOOTER:
        return config::ShooterCost;
    case UName::CAVALRY:
        return config::CavalryCost;
    case UName::SIEGE:
        return config::SiegeCost;
    case UName::GUARDIAN:
        return config::GuardianCost;
    default:
        return 0;
    }
}

bool Game::isUnitUnlocked(int team, int name) const
{
    const int economy = economyLevelForTeam(team);
    const int barracks = completedBuildingCount(team, building::Barracks);
    const int upgrade = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    switch (name) {
    case UName::INFANTARY:
        return barracks >= 1;
    case UName::SHOOTER:
        return barracks >= 1 && (economy >= 1 || upgrade >= 1);
    case UName::CAVALRY:
        return barracks >= 2 && (economy >= 2 || upgrade >= 3);
    case UName::SIEGE:
        return barracks >= 2 && economy >= 3 && upgrade >= 5;
    case UName::GUARDIAN:
        return barracks >= 3 && economy >= 4 && upgrade >= 7;
    default:
        return false;
    }
}

bool Game::canQueueUnit(int team, int name) const
{
    return unitCost(name) > 0
        && hasUnitCapacity(team)
        && isUnitUnlocked(team, name)
        && commandForTeam(team) >= unitCost(name)
        && completedBuildingCount(team, building::Barracks) > 0;
}

bool Game::enqueueUnit(int team, int name)
{
    if (!canQueueUnit(team, name)) {
        if (team == PLAYER) {
            std::string reason = "Need Barracks";
            if (completedBuildingCount(team, building::Barracks) > 0 && !isUnitUnlocked(team, name)) {
                reason = "Locked";
            }
            else if (commandForTeam(team) < unitCost(name)) {
                reason = "Need CMD";
            }
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildInfantryY) - 18.f),
                            reason, sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    Building* best = nullptr;
    for (auto& building : buildings) {
        if (building.team != team || building.type != building::Barracks || !building.complete) {
            continue;
        }
        if (best == nullptr || building.production.load() < best->production.load()) {
            best = &building;
        }
    }
    if (best == nullptr) {
        return false;
    }

    commandPool(*this, team) -= unitCost(name);
    const int orderLane = selectedLaneForTeam(team);
    best->production.orders.push_back(ProductionOrder{name, orderLane});
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(best->point.x * SqureSize, best->point.y * SqureSize - 8.f),
                        std::string("Queued ") + laneName(orderLane), sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued unit=" + std::to_string(name)
        + " lane=" + laneName(orderLane) + " rax=" + std::to_string(best->id)
        + " load=" + std::to_string(best->production.load()));
    return true;
}

bool Game::executeOperation(int team, const GameOperation& operation)
{
    // Central command dispatcher: UI clicks, scripted playtests, and AI plans
    // all go through this path so command validation stays identical.
    int& selectedLane = team == PLAYER ? playerSelectedLane : aiSelectedLane;
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);

    switch (operation.type) {
    case gameop::SelectLane:
        selectedLane = safeLane;
        return true;
    case gameop::UpgradeEconomy:
        return upgradeEconomy(team);
    case gameop::UpgradeTech:
        return upgradeTeam(team);
    case gameop::BuildBarracks:
        selectedLane = safeLane;
        return requestAutoBuildBarracks(team);
    case gameop::BuildTower:
        selectedLane = safeLane;
        return requestAutoBuildTower(team);
    case gameop::QueueUnit:
        selectedLane = safeLane;
        return enqueueUnit(team, operation.unitName);
    default:
        return false;
    }
}

std::size_t Game::executeOperations(int team, const std::vector<GameOperation>& operations)
{
    std::size_t successes = 0;
    for (const auto& operation : operations) {
        const bool ok = executeOperation(team, operation);
        if (ok) {
            ++successes;
        }
        logEvent(std::string(team == PLAYER ? "player" : "ai")
            + " op " + describeOperation(operation) + (ok ? " ok" : " blocked"));
    }
    return successes;
}

std::string Game::describeOperation(const GameOperation& operation) const
{
    std::string label(operationTypeName(operation.type));
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);
    if (operation.type == gameop::BuildBarracks
        || operation.type == gameop::BuildTower
        || operation.type == gameop::QueueUnit
        || operation.type == gameop::SelectLane) {
        label += "[";
        label += laneName(safeLane);
        label += "]";
    }
    if (operation.type == gameop::QueueUnit) {
        label += ":";
        label += unitDebugName(operation.unitName);
    }
    return label;
}

bool Game::upgradeTeam(int team)
{
    int& level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int cost = upgradeCostForNextLevel(team);
    if (level >= config::MaxTechLevel || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                            level >= config::MaxTechLevel ? "Max level" : "Need CMD",
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    ++level;
    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        unit->scaleMaxHealth(1.f + config::TechHealthBonus);
    }
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                        "LEVEL " + std::to_string(level), sf::Color(218, 255, 134), 12);
    }
    maybeGrantReward(team, "Tech " + std::to_string(level));
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " upgraded to level " + std::to_string(level));
    return true;
}

void Game::syncWorkersForEconomy(int team)
{
    const int targetWorkers = std::min(realtime::MaxWorkers, realtime::StartingWorkers + economyLevelForTeam(team));
    while (workerCount(team) < targetWorkers) {
        createWorker(team, workerSpawnPoint(team));
    }
}

bool Game::upgradeEconomy(int team)
{
    int& level = team == PLAYER ? playerEconomyLevel : aiEconomyLevel;
    const int cost = economyUpgradeCost(team);
    if (level >= config::MaxEconomyLevel || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EconomyButtonY) - 18.f),
                            level >= config::MaxEconomyLevel ? "Max economy" : "Need CMD",
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    ++level;
    syncWorkersForEconomy(team);
    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 42.f),
                        "Economy Lv" + std::to_string(level),
                        team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 13);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " economy level " + std::to_string(level));
    return true;
}

void Game::awardKillBounty(int receiverTeam, int defeatedUnitName, Point point)
{
    const int cost = unitCost(defeatedUnitName);
    if (cost <= 0) {
        return;
    }
    const int bounty = std::max(config::KillBountyMin,
        static_cast<int>(std::round(static_cast<float>(cost * config::KillBountyPercent) / 100.f)));
    commandPool(*this, receiverTeam) = std::min(config::MaxCommand, commandPool(*this, receiverTeam) + bounty);
    addFloatingText(sf::Vector2f(point.x * SqureSize + SqureSize / 2.f, point.y * SqureSize - 22.f),
                    "+" + std::to_string(bounty) + " CMD",
                    receiverTeam == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 12);
    logEvent(std::string(receiverTeam == PLAYER ? "player" : "ai")
        + " bounty +" + std::to_string(bounty) + " unit=" + std::to_string(defeatedUnitName));
}

int Game::commandForTeam(int team) const
{
    return team == PLAYER ? playerCommand : aiCommand;
}

bool Game::hasUnitCapacity(int team) const
{
    return team == PLAYER ? myunits.size() < MaxUnit : enemys.size() < MaxUnit;
}

bool Game::hasSpawnTile(int team) const
{
    const DisMoveableUnit* base = team == PLAYER ? Base_red.get() : Base_blue.get();
    if (base == nullptr) {
        return false;
    }

    for (int i = base->x - 1; i < base->x + 3; ++i) {
        for (int j = base->y - 1; j < base->y + 3; ++j) {
            if (!isMapCell(i, j)) {
                continue;
            }
            if (tiles[i + horizontalTiles * j].getID() == tile::Empty && !isCellReservedForSpawn(i, j)) {
                return true;
            }
        }
    }
    return false;
}

std::string Game::spawnBlockReason(int team, int name) const
{
    const int cost = unitCost(name);
    if (cost <= 0) {
        return "Bad unit";
    }
    if (!hasUnitCapacity(team)) {
        return "Unit cap";
    }
    if (commandForTeam(team) < cost) {
        return "Need CMD";
    }
    if (!hasSpawnTile(team)) {
        return "No room";
    }
    return "";
}

bool Game::canSpawnUnit(int team, int name) const
{
    return spawnBlockReason(team, name).empty();
}

bool Game::spendCommand(int team, int name)
{
    if (!canSpawnUnit(team, name)) {
        return false;
    }
    commandPool(*this, team) -= unitCost(name);
    return true;
}

void Game::runAIProduction()
{
    if (!Base_blue) {
        return;
    }

    const int playerPressure = unitsNearPoint(PLAYER, Blue_baseP, 13);
    const int aiPressure = unitsNearPoint(AI, Red_baseP, 13);
    const int playerTowers = totalBuildingCount(PLAYER, building::DefenseTower);
    const int playerBarracks = totalBuildingCount(PLAYER, building::Barracks);
    const bool playerTurtling = playerTowers > 0 || playerBarracks >= 3;
    const bool defenseMode = playerPressure >= 4 || (Base_blue && Base_blue->Health < 2800);
    const bool armyBehind = static_cast<int>(enemys.size()) + 4 < static_cast<int>(myunits.size());
    const bool siegeMode = !defenseMode && (playerTurtling || (gameTimeSeconds > 540.f && isUnitUnlocked(AI, UName::SIEGE)));
    const bool macroMode = !defenseMode && !armyBehind && enemys.size() >= 14;

    const auto nearestLaneForY = [this](int y) {
        int bestLane = lane::Mid;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < lane::Count; ++i) {
            const int laneY = laneWaypoint(AI, i, 1).y;
            const int distance = std::abs(y - laneY);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestLane = i;
            }
        }
        return bestLane;
    };

    const auto laneWithMostPlayerUnits = [this]() {
        int counts[lane::Count] = {};
        for (const auto& unit : myunits) {
            if (unit->Health > 0) {
                ++counts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
            }
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (counts[i] > counts[bestLane]) {
                bestLane = i;
            }
        }
        return bestLane;
    };

    const auto laneWithMostPlayerStructures = [&nearestLaneForY, &laneWithMostPlayerUnits, this]() {
        int scores[lane::Count] = {};
        for (const auto& building : buildings) {
            if (building.team != PLAYER || !building.complete) {
                continue;
            }
            const int laneIndex = nearestLaneForY(building.point.y);
            scores[laneIndex] += building.type == building::DefenseTower ? 4 : 2;
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (scores[i] > scores[bestLane]) {
                bestLane = i;
            }
        }
        return scores[bestLane] > 0 ? bestLane : laneWithMostPlayerUnits();
    };

    const auto chooseLane = [&](int orderIndex) {
        if (defenseMode) {
            return laneWithMostPlayerUnits();
        }
        if (siegeMode) {
            return laneWithMostPlayerStructures();
        }
        return (static_cast<int>(gameTimeSeconds / 34.f) + orderIndex) % lane::Count;
    };

    int desiredEconomy = 1;
    if (gameTimeSeconds > 25.f) desiredEconomy = 2;
    if (gameTimeSeconds > 70.f) desiredEconomy = 3;
    if (gameTimeSeconds > 135.f) desiredEconomy = 4;
    if (gameTimeSeconds > 230.f) desiredEconomy = 5;
    if (gameTimeSeconds > 360.f) desiredEconomy = 7;
    if (gameTimeSeconds > 520.f) desiredEconomy = 9;
    if (gameTimeSeconds > 700.f) desiredEconomy = 11;
    if (gameTimeSeconds > 840.f) desiredEconomy = config::MaxEconomyLevel;
    if (playerEconomyLevel > aiEconomyLevel + 1 || macroMode) {
        ++desiredEconomy;
    }
    desiredEconomy = std::clamp(desiredEconomy, 0, config::MaxEconomyLevel);

    int allowedTech = static_cast<int>(gameTimeSeconds / 46.f);
    allowedTech = std::min(allowedTech, aiEconomyLevel + (gameTimeSeconds > 600.f ? 7 : 5));
    if (playerUpgradeLevel > aiUpgradeLevel) {
        allowedTech = std::max(allowedTech, playerUpgradeLevel + 1);
    }
    if (gameTimeSeconds > 620.f) {
        allowedTech = std::max(allowedTech, 10 + static_cast<int>((gameTimeSeconds - 620.f) / 46.f));
    }
    if (gameTimeSeconds > 840.f) {
        allowedTech = config::MaxTechLevel;
    }
    allowedTech = std::clamp(allowedTech, 0, config::MaxTechLevel);

    int desiredBarracks = 1;
    if (gameTimeSeconds > 80.f && aiEconomyLevel >= 1) desiredBarracks = 2;
    if (gameTimeSeconds > 210.f && aiUpgradeLevel >= 2) desiredBarracks = 3;
    if (gameTimeSeconds > 360.f && aiUpgradeLevel >= 5) desiredBarracks = 4;
    if (gameTimeSeconds > 560.f && aiUpgradeLevel >= 8) desiredBarracks = 5;
    if (gameTimeSeconds > 760.f) desiredBarracks = 6;
    if (armyBehind || siegeMode) ++desiredBarracks;
    desiredBarracks = std::min(desiredBarracks, buildingCap(AI, building::Barracks));

    int desiredTowers = 0;
    if (playerPressure >= 4) desiredTowers = 1;
    if (playerPressure >= 8 || (Base_blue && Base_blue->Health < 2200)) desiredTowers = 2;
    if (gameTimeSeconds > 700.f && playerPressure >= 6 && aiUpgradeLevel >= 10) desiredTowers = 3;
    desiredTowers = std::min(desiredTowers, buildingCap(AI, building::DefenseTower));

    if (totalBuildingCount(AI, building::Barracks) < 1) {
        if (commandForTeam(AI) >= config::BarracksCost) {
            executeOperations(AI, {GameOperation(gameop::BuildBarracks, lane::Mid)});
        }
        return;
    }
    if (completedBuildingCount(AI, building::Barracks) < 1) {
        return;
    }

    bool majorActionTaken = false;
    const auto tryMajorAction = [&](bool condition, const GameOperation& operation) {
        if (majorActionTaken || !condition) {
            return;
        }
        if (executeOperations(AI, {operation}) > 0) {
            majorActionTaken = true;
        }
    };

    const bool openingNeedsUnits = enemys.size() < 4 && gameTimeSeconds < 110.f && playerPressure < 4;
    if (!openingNeedsUnits) {
        tryMajorAction(aiEconomyLevel < desiredEconomy && commandForTeam(AI) >= economyUpgradeCost(AI),
                       GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < allowedTech && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
                       GameOperation(gameop::UpgradeTech));
    }

    tryMajorAction(totalBuildingCount(AI, building::Barracks) < desiredBarracks
        && commandForTeam(AI) >= config::BarracksCost + (defenseMode ? 0 : config::InfantryCost),
        GameOperation(gameop::BuildBarracks, chooseLane(0)));

    tryMajorAction(defenseMode
        && totalBuildingCount(AI, building::DefenseTower) < desiredTowers
        && commandForTeam(AI) >= config::TowerCost + config::InfantryCost,
        GameOperation(gameop::BuildTower, laneWithMostPlayerUnits()));

    if (openingNeedsUnits && !majorActionTaken) {
        tryMajorAction(aiEconomyLevel < std::min(2, desiredEconomy)
            && enemys.size() >= 3
            && commandForTeam(AI) >= economyUpgradeCost(AI),
            GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < std::min(1, allowedTech)
            && enemys.size() >= 3
            && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
            GameOperation(gameop::UpgradeTech));
    }

    int reserve = 0;
    const bool armyEmergency = defenseMode || armyBehind || enemys.size() < 10;
    if (!armyEmergency) {
        if (aiEconomyLevel < desiredEconomy) {
            reserve = std::max(reserve, std::min(economyUpgradeCost(AI), macroMode ? 230 : 170));
        }
        if (aiUpgradeLevel < allowedTech) {
            reserve = std::max(reserve, std::min(upgradeCostForNextLevel(AI), macroMode ? 270 : 190));
        }
        if (totalBuildingCount(AI, building::Barracks) < desiredBarracks) {
            reserve = std::max(reserve, config::BarracksCost);
        }
    }
    if (majorActionTaken) {
        reserve = std::min(reserve, 55);
    }
    if (!hasUnitCapacity(AI)) {
        if (majorActionTaken) {
            logEvent(std::string("ai plan=")
                + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
                + " ecoTarget=" + std::to_string(desiredEconomy)
                + " techTarget=" + std::to_string(allowedTech)
                + " raxTarget=" + std::to_string(desiredBarracks));
        }
        return;
    }

    const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
    const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
    const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
    const int playerSiege = countUnitsNamed(myunits, UName::SIEGE);
    const int playerGuardian = countUnitsNamed(myunits, UName::GUARDIAN);

    int counterPick = UName::SHOOTER;
    if (playerShooters > playerInfantry + 1 || playerSiege > 0) {
        counterPick = UName::CAVALRY;
    }
    else if (playerCavalry + playerGuardian > playerShooters) {
        counterPick = UName::INFANTARY;
    }

    std::vector<int> priorities;
    priorities.reserve(8);
    if (defenseMode) {
        if (gameTimeSeconds < 420.f) {
            priorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, counterPick, UName::GUARDIAN};
        }
        else {
            priorities = {UName::GUARDIAN, UName::INFANTARY, UName::CAVALRY, counterPick, UName::SHOOTER};
        }
    }
    else if (siegeMode) {
        priorities = {UName::SIEGE, UName::GUARDIAN, UName::CAVALRY, counterPick, UName::SHOOTER, UName::INFANTARY};
    }
    else if (gameTimeSeconds < 300.f) {
        // Rotate a small opening roster so the AI does not look like it only
        // understands infantry spam before siege/guardian tech comes online.
        priorities = {UName::SHOOTER, UName::INFANTARY, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    else {
        priorities = {counterPick, UName::SHOOTER, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    if (aiPressure > 0 || playerTurtling) {
        priorities.insert(priorities.begin(), UName::SIEGE);
    }

    const auto leastLoadedBarracks = [this]() {
        int bestLoad = std::numeric_limits<int>::max();
        for (const auto& building : buildings) {
            if (building.team == AI && building.type == building::Barracks && building.complete) {
                bestLoad = std::min(bestLoad, building.production.load());
            }
        }
        return bestLoad == std::numeric_limits<int>::max() ? 0 : bestLoad;
    };
    const auto aiPlannedUnitCount = [this](int unitName) {
        int total = countUnitsNamed(enemys, unitName);
        for (const auto& building : buildings) {
            if (building.team != AI || building.type != building::Barracks) {
                continue;
            }
            if (building.production.activeUnit == unitName) {
                ++total;
            }
            total += static_cast<int>(std::count_if(
                building.production.orders.begin(),
                building.production.orders.end(),
                [unitName](const ProductionOrder& order) { return order.unit == unitName; }));
        }
        return total;
    };

    if (gameTimeSeconds < 120.f && !defenseMode && leastLoadedBarracks() >= 2) {
        const int openingMacroCost = aiEconomyLevel < 1
            ? economyUpgradeCost(AI)
            : (aiUpgradeLevel < 1 ? upgradeCostForNextLevel(AI) : 0);
        if (openingMacroCost > 0 && commandForTeam(AI) < openingMacroCost) {
            logEvent("ai opening banks CMD for early economy/tech");
            return;
        }
    }

    int orders = realtime::AIUnitsPerBurst + aiUpgradeLevel / 5;
    if (aiEconomyLevel >= 4) ++orders;
    if (aiEconomyLevel >= 8) ++orders;
    if (armyBehind || defenseMode) ++orders;
    if (siegeMode) ++orders;
    if (gameTimeSeconds > 760.f) ++orders;
    orders = std::min({orders, completedBuildingCount(AI, building::Barracks), 5});

    const int queueLoadLimit = macroMode ? 3 : (defenseMode ? 6 : 4);
    for (int i = 0; i < orders; ++i) {
        if (leastLoadedBarracks() >= queueLoadLimit && commandForTeam(AI) < reserve + 160 && !armyEmergency) {
            break;
        }
        if (gameTimeSeconds > 75.f && gameTimeSeconds < 155.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::SHOOTER)
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::ShooterCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first shooter");
            break;
        }
        const bool wantsOpeningCavalry = gameTimeSeconds > 185.f && gameTimeSeconds < 380.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::CAVALRY)
            && aiPlannedUnitCount(UName::CAVALRY) < 1;
        if (wantsOpeningCavalry
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::CavalryCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first cavalry");
            break;
        }

        std::vector<int> orderPriorities = priorities;
        if (wantsOpeningCavalry && commandForTeam(AI) >= config::CavalryCost) {
            orderPriorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
        }
        if (!wantsOpeningCavalry && gameTimeSeconds < 300.f && !defenseMode && !siegeMode && !orderPriorities.empty()) {
            const auto shift = static_cast<std::ptrdiff_t>(
                (static_cast<int>(gameTimeSeconds / realtime::AIThinkSeconds) + i)
                % static_cast<int>(orderPriorities.size()));
            std::rotate(orderPriorities.begin(), orderPriorities.begin() + shift, orderPriorities.end());
        }

        bool queued = false;
        for (int code : orderPriorities) {
            const int cost = unitCost(code);
            if (cost <= 0 || !canQueueUnit(AI, code)) {
                continue;
            }
            if (!armyEmergency && commandForTeam(AI) < cost + reserve) {
                continue;
            }
            if (executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), code)}) > 0) {
                queued = true;
                break;
            }
        }
        if (!queued && armyEmergency && commandForTeam(AI) >= config::InfantryCost) {
            queued = executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), UName::INFANTARY)}) > 0;
        }
        if (!queued) {
            break;
        }
    }

    if (majorActionTaken) {
        logEvent(std::string("ai plan=")
            + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
            + " ecoTarget=" + std::to_string(desiredEconomy)
            + " techTarget=" + std::to_string(allowedTech)
            + " raxTarget=" + std::to_string(desiredBarracks));
    }
}

void Game::addTurnIncome(int team)
{
    const int income = resourceIncome(team);
    int& command = commandPool(*this, team);
    command = std::min(config::MaxCommand, command + income);

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 8.f),
                        "+" + std::to_string(income) + " CMD",
                        team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 13);
    }
}

int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}

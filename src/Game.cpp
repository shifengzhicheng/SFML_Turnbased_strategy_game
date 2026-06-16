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

    const char* resourceKindName(int kind)
    {
        switch (kind) {
        case resource::Crystal:
            return "Crystal";
        case resource::Foundry:
            return "Foundry";
        case resource::Shrine:
            return "Shrine";
        case resource::Gold:
        default:
            return "Gold";
        }
    }

    const char* resourceKindShortName(int kind)
    {
        switch (kind) {
        case resource::Crystal:
            return "C";
        case resource::Foundry:
            return "F";
        case resource::Shrine:
            return "S";
        case resource::Gold:
        default:
            return "G";
        }
    }

    sf::Color resourceKindColor(int kind)
    {
        switch (kind) {
        case resource::Crystal:
            return sf::Color(112, 214, 255);
        case resource::Foundry:
            return sf::Color(255, 131, 83);
        case resource::Shrine:
            return sf::Color(181, 136, 255);
        case resource::Gold:
        default:
            return sf::Color(255, 211, 82);
        }
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
            return "Mining Crew";
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
            return "Barracks train 6% faster.\nFoundries stack with it.";
        case perk::Mining:
            return "Mines pay +7% CMD.\nGreat for late tech.";
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
            return "Mine";
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
        if (type == building::Extractor) {
            return team == PLAYER ? tile::Player_Extractor : tile::Enemy_Extractor;
        }
        if (type == building::DefenseTower) {
            return team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower;
        }
        return team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks;
    }

    float buildingSeconds(int type)
    {
        if (type == building::Extractor) {
            return realtime::ExtractorBuildSeconds;
        }
        if (type == building::DefenseTower) {
            return realtime::DefenseTowerBuildSeconds;
        }
        return realtime::BarracksBuildSeconds;
    }

    const char* buildingName(int type)
    {
        switch (type) {
        case building::Extractor:
            return "Mine";
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
        case building::Extractor:
            return config::ExtractorHealth;
        case building::DefenseTower:
            return config::DefenseTowerHealth;
        case building::Barracks:
        default:
            return config::BarracksHealth;
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
    art::makeButtonTexture(tStartBtnNormal, myfont, "START", art::ButtonState::Normal, sf::Vector2u(180, 72));
    art::makeButtonTexture(tStartBtnHover, myfont, "START", art::ButtonState::Hover, sf::Vector2u(190, 76));
    art::makeButtonTexture(tStartBtnClick, myfont, "START", art::ButtonState::Pressed, sf::Vector2u(170, 68));
    art::makeButtonTexture(tEndBtnNormal, myfont, "END TURN", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tEndBtnHover, myfont, "END TURN", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tEndBtnClick, myfont, "END TURN", art::ButtonState::Pressed, sf::Vector2u(128, 44));
    art::makeButtonTexture(tOverBtnNormal, myfont, "PLAY AGAIN", art::ButtonState::Normal, sf::Vector2u(240, 96));
    art::makeButtonTexture(tOverBtnHover, myfont, "PLAY AGAIN", art::ButtonState::Hover, sf::Vector2u(250, 100));
    art::makeButtonTexture(tOverBtnClick, myfont, "PLAY AGAIN", art::ButtonState::Pressed, sf::Vector2u(232, 92));
    const sf::Vector2u unitButtonSize(128, 42);
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
    art::makeButtonTexture(tUpgrade, myfont, "UPGRADE", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tUpgradeHover, myfont, "UPGRADE", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tUpgradeClick, myfont, "UPGRADE", art::ButtonState::Pressed, sf::Vector2u(128, 44));
    art::makeButtonTexture(tBarracks, myfont, "Barracks", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tBarracksHover, myfont, "Barracks", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tBarracksClick, myfont, "Barracks", art::ButtonState::Pressed, sf::Vector2u(128, 44));
    art::makeButtonTexture(tHelp, myfont, "HELP", art::ButtonState::Normal, sf::Vector2u(128, 36));
    art::makeButtonTexture(tHelpHover, myfont, "HELP", art::ButtonState::Hover, sf::Vector2u(128, 36));
    art::makeButtonTexture(tHelpClick, myfont, "HELP", art::ButtonState::Pressed, sf::Vector2u(128, 36));
    art::makeButtonTexture(tTower, myfont, "Tower", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tTowerHover, myfont, "Tower", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tTowerClick, myfont, "Tower", art::ButtonState::Pressed, sf::Vector2u(128, 44));

    startBtn.setTextures(tStartBtnNormal, tStartBtnHover, tStartBtnClick);
    EndTurnBtn.setTextures(tEndBtnNormal, tEndBtnHover, tEndBtnClick);
    endGame.setTextures(tOverBtnNormal, tOverBtnHover, tOverBtnClick);
    inf.setTextures(tinf, tinfHover, tinfClick);
    cav.setTextures(tcav, tcavHover, tcavClick);
    sho.setTextures(tsho, tshoHover, tshoClick);
    siegeBtn.setTextures(tSiege, tSiegeHover, tSiegeClick);
    guardianBtn.setTextures(tGuardian, tGuardianHover, tGuardianClick);
    upgradeBtn.setTextures(tUpgrade, tUpgradeHover, tUpgradeClick);
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
    setupText(barracksLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::BarracksCost) + " auto near base", panelTextX, config::BuildBarracksY + 43.f);
    setupText(infantryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::InfantryCost) + " core", panelTextX, config::BuildInfantryY + 43.f);
    setupText(shooterLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::ShooterCost) + " mine/Lv1", panelTextX, config::BuildShooterY + 43.f);
    setupText(cavalryLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::CavalryCost) + " 2 rax", panelTextX, config::BuildCavalryY + 43.f);
    setupText(siegeLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::SiegeCost) + " Lv2 siege", panelTextX, config::BuildSiegeY + 43.f);
    setupText(guardianLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::GuardianCost) + " Lv3 tank", panelTextX, config::BuildGuardianY + 43.f);
    setupText(towerLabel, myfont, 10, sf::Color(228, 218, 185), std::to_string(config::TowerCost) + " auto lane fort", panelTextX, config::BuildTowerY + 43.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(35, 43, 39));
    sidePanel.setOutlineColor(sf::Color(19, 24, 22));
    sidePanel.setOutlineThickness(2.f);

    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    helpBtn.setPosition(config::ButtonX, config::HelpButtonY);
    upgradeBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
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
        || id == tile::Player_Extractor
        || id == tile::Enemy_Extractor
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
    if (type == building::Extractor) {
        return true;
    }

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
    updateResourceCaptures(dt);
    updateResourceControl();
    updateRealtimeEconomy(dt);
    aiController.update(*this, dt);
    playerWorkerTimer = std::min(playerWorkerTimer + dt, realtime::WorkerTrainSeconds);
    aiWorkerTimer = std::min(aiWorkerTimer + dt, realtime::WorkerTrainSeconds);
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
        << " mines=" << completedBuildingCount(PLAYER, building::Extractor)
        << " rax=" << completedBuildingCount(PLAYER, building::Barracks) << "/" << buildingCap(PLAYER, building::Barracks)
        << " tower=" << totalBuildingCount(PLAYER, building::DefenseTower)
        << " level=" << playerUpgradeLevel
        << " base=" << (Base_red ? Base_red->Health : 0)
        << " army=" << myunits.size()
        << " | ai cmd=" << aiCommand
        << " mines=" << completedBuildingCount(AI, building::Extractor)
        << " rax=" << completedBuildingCount(AI, building::Barracks) << "/" << buildingCap(AI, building::Barracks)
        << " tower=" << totalBuildingCount(AI, building::DefenseTower)
        << " level=" << aiUpgradeLevel
        << " base=" << (Base_blue ? Base_blue->Health : 0)
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

Building* Game::findResourceExtractor(int resourceIndex)
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [resourceIndex](const Building& building) {
        return building.type == building::Extractor && building.resourceIndex == resourceIndex;
    });
    return it == buildings.end() ? nullptr : &(*it);
}

const Building* Game::findResourceExtractor(int resourceIndex) const
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [resourceIndex](const Building& building) {
        return building.type == building::Extractor && building.resourceIndex == resourceIndex;
    });
    return it == buildings.end() ? nullptr : &(*it);
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

bool Game::tryAutoRecruitWorker(int team)
{
    if (workerCount(team) >= realtime::MaxWorkers || commandPool(*this, team) < config::WorkerCost) {
        return false;
    }

    float& timer = team == PLAYER ? playerWorkerTimer : aiWorkerTimer;
    if (timer < realtime::WorkerTrainSeconds) {
        return false;
    }

    const Point spawn = workerSpawnPoint(team);
    if (!isCellWalkableForUnit(spawn.x, spawn.y) || isCellReservedForSpawn(spawn.x, spawn.y)) {
        return false;
    }

    commandPool(*this, team) -= config::WorkerCost;
    timer = 0.f;
    createWorker(team, spawn);
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(spawn.x * SqureSize, spawn.y * SqureSize - 8.f),
                        "Drone -" + std::to_string(config::WorkerCost), sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " recruited drone id=" + std::to_string(workers.back().id));
    return true;
}

int Game::assignedWorkerCount(int buildingId) const
{
    return static_cast<int>(std::count_if(workers.begin(), workers.end(), [buildingId](const Worker& worker) {
        return worker.buildingId == buildingId && worker.state != worker::Idle;
    }));
}

bool Game::hasActiveHarvester(const Building& building) const
{
    if (!building.complete || building.type != building::Extractor) {
        return false;
    }
    return std::any_of(workers.begin(), workers.end(), [&building](const Worker& worker) {
        return worker.buildingId == building.id
            && worker.state == worker::Harvesting
            && nearPoint(worker.point, building.point, 1);
    });
}

Worker* Game::findIdleWorker(int team)
{
    const auto it = std::find_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team && worker.state == worker::Idle;
    });
    return it == workers.end() ? nullptr : &(*it);
}

Worker* Game::findAvailableWorker(int team, bool allowHarvesting)
{
    if (Worker* worker = findIdleWorker(team)) {
        return worker;
    }
    if (!allowHarvesting) {
        return nullptr;
    }

    // Construction has priority over mining: temporarily pull a drone from a
    // finished extractor when no idle worker is available.
    const auto it = std::find_if(workers.begin(), workers.end(), [team](const Worker& worker) {
        return worker.team == team && worker.state == worker::Harvesting;
    });
    return it == workers.end() ? nullptr : &(*it);
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

void Game::assignWorkerToHarvest(Worker& worker, Building& building)
{
    worker.buildingId = building.id;
    worker.target = findBuildStandPoint(building);
    worker.path.clear();
    worker.pendingPathRequest = 0;
    worker.pathTimer = realtime::WorkerPathRefreshSeconds;
    worker.moveTimer = 0.f;
    worker.state = nearPoint(worker.point, building.point, 1) ? worker::Harvesting : worker::MovingToHarvest;
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
    if (type == building::Barracks) {
        return std::min(config::BarracksCap, config::BarracksBaseCap + tech / 3 + completedBuildingCount(team, building::Extractor) / 2);
    }
    if (type == building::DefenseTower) {
        return std::min(config::TowerCap, config::TowerBaseCap + tech / 4 + completedBuildingCount(team, building::Extractor) / 3);
    }
    return static_cast<int>(resources.size());
}

int Game::pendingOrCompleteExtractorForResource(int resourceIndex) const
{
    const auto it = std::find_if(buildings.begin(), buildings.end(), [resourceIndex](const Building& building) {
        return building.type == building::Extractor && building.resourceIndex == resourceIndex;
    });
    return it == buildings.end() ? 0 : it->id;
}

int Game::buildingCost(int type) const
{
    if (type == building::Extractor) {
        return config::ExtractorCost;
    }
    if (type == building::DefenseTower) {
        return config::TowerCost;
    }
    return config::BarracksCost;
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

float Game::baseDamageTakenMultiplier(int attackerUnitName) const
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
    return std::clamp(shield, 0.25f, 1.f);
}

float Game::teamTrainTimeMultiplier(int team) const
{
    const float logistics = static_cast<float>(perkLevel(team, perk::Logistics)) * 0.06f;
    const float foundries = static_cast<float>(controlledResourceKindCount(team, resource::Foundry)) * 0.04f;
    return std::clamp(1.f - logistics - foundries, 0.72f, 1.f);
}

float Game::miningIncomeMultiplier(int team) const
{
    return 1.f + static_cast<float>(perkLevel(team, perk::Mining)) * 0.07f;
}

int Game::defenseTowerRange(int team) const
{
    return config::DefenseTowerRange + std::min(2, perkLevel(team, perk::TowerCraft) / 2);
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
    if (team == AI && gameTimeSeconds > 420.f && controlledResourceCount(AI) <= 1) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.62f));
    }
    if (team == AI && gameTimeSeconds > 840.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.42f));
    }
    const int flatDiscount = controlledResourceKindCount(team, resource::Crystal) * 14
        + controlledResourceKindCount(team, resource::Shrine) * 6;
    const int maxDiscount = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.30f));
    return std::max(25, rawCost - std::min(flatDiscount, maxDiscount));
}

int Game::unitsNearPoint(int team, Point point, int radius) const
{
    const auto& units = team == PLAYER ? myunits : enemys;
    const int radiusSquared = radius * radius;
    return static_cast<int>(std::count_if(units.begin(), units.end(), [point, radiusSquared](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->Health > 0 && distanceSquared(Point(unit->x, unit->y), point) <= radiusSquared;
    }));
}

bool Game::requestBuildExtractor(int team, int resourceIndex)
{
    if (resourceIndex < 0 || resourceIndex >= static_cast<int>(resources.size())) {
        return false;
    }
    if (pendingOrCompleteExtractorForResource(resourceIndex) != 0) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(resources[resourceIndex].point.x * SqureSize, resources[resourceIndex].point.y * SqureSize - 8.f),
                            "Mine exists", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (commandPool(*this, team) < config::ExtractorCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(resources[resourceIndex].point.x * SqureSize, resources[resourceIndex].point.y * SqureSize - 8.f),
                            "Need CMD", sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= config::ExtractorCost;
    Building building;
    building.id = nextEntityId++;
    building.team = team;
    building.type = building::Extractor;
    building.point = resources[resourceIndex].point;
    building.resourceIndex = resourceIndex;
    building.buildSeconds = buildingSeconds(building.type);
    building.maxHealth = buildingMaxHealth(building.type);
    building.health = building.maxHealth;
    buildings.push_back(building);
    setTileID(building.point.x, building.point.y, buildingTileId(team, building.type));
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(building.point.x * SqureSize, building.point.y * SqureSize - 8.f),
                        "Mine queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued mine id=" + std::to_string(building.id));
    return true;
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
    // Worker routing is demand based: build tasks borrow idle/mining drones,
    // then completed extractors claim one drone to keep income active.
    for (auto& building : buildings) {
        if (building.complete) {
            continue;
        }
        if (assignedWorkerCount(building.id) > 0) {
            continue;
        }

        Worker* worker = findAvailableWorker(building.team, true);
        if (worker == nullptr && tryAutoRecruitWorker(building.team)) {
            worker = &workers.back();
        }
        if (worker != nullptr) {
            assignWorkerToBuilding(*worker, building);
        }
    }

    for (auto& building : buildings) {
        if (!building.complete || building.type != building::Extractor || assignedWorkerCount(building.id) > 0) {
            continue;
        }

        Worker* worker = findIdleWorker(building.team);
        if (worker == nullptr && tryAutoRecruitWorker(building.team)) {
            worker = &workers.back();
        }
        if (worker != nullptr) {
            assignWorkerToHarvest(*worker, building);
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

        if (worker.state == worker::MovingToHarvest || worker.state == worker::Harvesting) {
            if (!building->complete || building->type != building::Extractor) {
                worker.state = worker::Idle;
                worker.buildingId = 0;
                worker.path.clear();
                worker.pendingPathRequest = 0;
                continue;
            }

            if (nearPoint(worker.point, building->point, 1)) {
                worker.state = worker::Harvesting;
                worker.path.clear();
                continue;
            }

            worker.state = worker::MovingToHarvest;
            updateWorkerTravel(worker, findBuildStandPoint(*building), dt);
            continue;
        }

        if (building->complete) {
            if (building->type == building::Extractor) {
                assignWorkerToHarvest(worker, *building);
            }
            else {
                worker.state = worker::Idle;
                worker.buildingId = 0;
                worker.path.clear();
                worker.pendingPathRequest = 0;
            }
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
                if (building->type == building::Extractor) {
                    assignWorkerToHarvest(worker, *building);
                }
                else {
                    worker.state = worker::Idle;
                    worker.buildingId = 0;
                }
                const sf::Vector2f pos(building->point.x * SqureSize, building->point.y * SqureSize - 10.f);
                addFloatingText(pos, building->type == building::Extractor ? "Mine online" : (building->type == building::DefenseTower ? "Tower ready" : "Barracks ready"),
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

void Game::updateResourceCaptures(float dt)
{
    for (int i = 0; i < static_cast<int>(resources.size()); ++i) {
        auto& node = resources[i];
        Building* extractor = findResourceExtractor(i);
        if (extractor == nullptr || !extractor->complete) {
            node.contestingTeam = -1;
            node.captureProgress = 0.f;
            continue;
        }

        const int owner = extractor->team;
        const int enemy = owner == PLAYER ? AI : PLAYER;
        int defenders = unitsNearPoint(owner, node.point, config::ResourceCaptureRadius);
        const int attackers = unitsNearPoint(enemy, node.point, config::ResourceCaptureRadius);

        // Mines are no longer flipped instantly by one passing unit. An active
        // harvester and nearby tower count as local control, making resource
        // steals a readable skirmish instead of a sudden economy wipe.
        if (hasActiveHarvester(*extractor)) {
            defenders += 1;
        }
        const bool towerCoversMine = std::any_of(buildings.begin(), buildings.end(), [this, owner, &node](const Building& building) {
            if (!building.complete || building.team != owner || building.type != building::DefenseTower) {
                return false;
            }
            const int range = defenseTowerRange(owner);
            return distanceSquared(building.point, node.point) <= range * range;
        });
        if (towerCoversMine) {
            defenders += 2;
        }

        if (attackers <= defenders || attackers <= 0) {
            node.captureProgress = std::max(0.f, node.captureProgress - dt * 1.0f);
            if (node.captureProgress <= 0.f) {
                node.contestingTeam = -1;
            }
            continue;
        }

        if (node.contestingTeam != enemy) {
            node.contestingTeam = enemy;
            node.captureProgress = 0.f;
        }
        const float captureRate = std::clamp(static_cast<float>(attackers - defenders) * 0.55f, 0.45f, 1.35f);
        node.captureProgress += dt * captureRate;
        if (node.captureProgress < config::ResourceCaptureSeconds) {
            continue;
        }

        const int previousTeam = extractor->team;
        extractor->team = enemy;
        node.contestingTeam = -1;
        node.captureProgress = 0.f;
        resetWorkersForBuilding(extractor->id);
        setTileID(extractor->point.x, extractor->point.y, buildingTileId(extractor->team, extractor->type));
        addFloatingText(sf::Vector2f(node.point.x * SqureSize + SqureSize / 2.f, node.point.y * SqureSize - 12.f),
                        enemy == PLAYER ? "CAPTURED" : "ENEMY CAPTURED",
                        enemy == PLAYER ? sf::Color(255, 220, 93) : sf::Color(145, 196, 255), 13);
        logEvent(std::string(previousTeam == PLAYER ? "player" : "ai") + " mine captured by "
            + (enemy == PLAYER ? "player" : "ai") + " id=" + std::to_string(extractor->id));
    }
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
        if (destroyed.type == building::Extractor && destroyed.resourceIndex >= 0
            && destroyed.resourceIndex < static_cast<int>(resources.size())) {
            auto& node = resources[destroyed.resourceIndex];
            node.owner = -1;
            node.contestingTeam = -1;
            node.captureProgress = 0.f;
            setTileID(destroyed.point.x, destroyed.point.y, tile::Resource);
        }
        else {
            setTileID(destroyed.point.x, destroyed.point.y, tile::Empty);
        }
        addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 8.f),
                        std::string(buildingName(destroyed.type)) + " down", sf::Color(255, 218, 112), 12);
        const int destroyer = destroyed.team == PLAYER ? AI : PLAYER;
        if (destroyed.type != building::Extractor) {
            commandPool(*this, destroyer) = std::min(config::MaxCommand, commandPool(*this, destroyer) + 12);
            if (destroyer == PLAYER) {
                addFloatingText(sf::Vector2f(destroyed.point.x * SqureSize, destroyed.point.y * SqureSize - 24.f),
                                "Raid +12 CMD", sf::Color(255, 224, 99), 12);
            }
        }
        logEvent(std::string(destroyed.team == PLAYER ? "player" : "ai") + " "
            + buildingName(destroyed.type) + " destroyed id=" + std::to_string(destroyed.id));
        it = buildings.erase(it);
    }
}

Building* Game::chooseBuildingTarget(MoveableUnit& unit)
{
    Building* best = nullptr;
    int bestScore = std::numeric_limits<int>::max();
    const Point current(unit.x, unit.y);
    const Point enemyBase = unit.myteam == PLAYER ? Blue_baseP : Red_baseP;
    const int mapW = width / SqureSize;
    const int enemyTeam = unit.myteam == PLAYER ? AI : PLAYER;
    const bool enemyProductionBroken = gameTimeSeconds > 420.f
        && completedBuildingCount(enemyTeam, building::Barracks) == 0
        && completedBuildingCount(enemyTeam, building::DefenseTower) == 0;
    const bool onEnemyHalf = unit.myteam == PLAYER ? current.x > mapW / 2 : current.x < mapW / 2;
    const bool committingToBase = gameTimeSeconds > 330.f && nearPoint(current, enemyBase, 12);
    for (auto& building : buildings) {
        if (building.team == unit.myteam || building.health <= 0 || !building.complete) {
            continue;
        }
        if (building.type == building::Extractor && gameTimeSeconds < 240.f) {
            continue;
        }
        if ((committingToBase || (enemyProductionBroken && onEnemyHalf)) && building.type == building::Extractor) {
            continue;
        }

        int priority = 400;
        if (building.type == building::DefenseTower) {
            priority = 0;
        }
        else if (building.type == building::Extractor) {
            priority = 120;
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
    Point best(-1, -1);
    int bestScore = std::numeric_limits<int>::max();
    const Point current(unit.x, unit.y);
    const Point enemyBase = unit.myteam == PLAYER ? Blue_baseP : Red_baseP;
    const int mapW = width / SqureSize;
    const int enemyTeam = unit.myteam == PLAYER ? AI : PLAYER;
    const bool enemyProductionBroken = gameTimeSeconds > 420.f
        && completedBuildingCount(enemyTeam, building::Barracks) == 0
        && completedBuildingCount(enemyTeam, building::DefenseTower) == 0;
    const bool onEnemyHalf = unit.myteam == PLAYER ? current.x > mapW / 2 : current.x < mapW / 2;
    if (enemyProductionBroken && onEnemyHalf) {
        return Point(-1, -1);
    }

    for (int i = 0; i < static_cast<int>(resources.size()); ++i) {
        const auto& node = resources[i];
        const Building* extractor = findResourceExtractor(i);
        const bool friendlyContested = extractor != nullptr
            && extractor->team == unit.myteam
            && node.contestingTeam != -1
            && node.captureProgress > 0.f;
        const bool enemyOwned = node.owner != -1 && node.owner != -2 && node.owner != unit.myteam && gameTimeSeconds > 210.f;
        const bool valuableNeutral = extractor == nullptr && node.income >= 8 && gameTimeSeconds < 150.f;
        if (!friendlyContested && !enemyOwned && !valuableNeutral) {
            continue;
        }

        const Point goal = extractor != nullptr ? findBuildStandPoint(*extractor) : node.point;
        if (!isCellWalkableForUnit(goal.x, goal.y)) {
            continue;
        }
        if (nearPoint(current, goal, valuableNeutral ? 3 : 2)) {
            continue;
        }

        // Strategic rallying keeps early armies contesting mines before they
        // mindlessly base-race; once a unit reaches the area, auto-combat can
        // resume pushing to the next building or base.
        int score = distanceSquared(current, goal) + distanceSquared(goal, enemyBase) / 4 - node.income * 70;
        if (friendlyContested) {
            score -= 900;
        }
        if (enemyOwned) {
            score -= 500;
        }
        if (valuableNeutral && gameTimeSeconds < 80.f) {
            score -= 180;
        }
        if (score < bestScore) {
            bestScore = score;
            best = goal;
        }
    }

    if (best.x >= 0) {
        return best;
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
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = enemys.erase(u);
        }
        else {
            ++u;
        }
    }
    if (!realtimeMode) {
        updateResourceControl();
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
    helpBtn.setPosition(600.f, 290.f);
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
    if (helpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        helpBtn.setState(NORMAL);
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
    for (int i = 0; i < static_cast<int>(resources.size()); ++i) {
        if (isResourceClick(resources[i], tileX, tileY)) {
            requestBuildExtractor(PLAYER, i);
            return;
        }
    }

    // Barracks and towers are auto-placed from side-panel buttons. Map clicks
    // stay focused on the one meaningful manual build choice: which resource
    // node to claim next.
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
        if (event.key.code == sf::Keyboard::Escape)
        {
            gameSceneState = gameSeceneState::SCENE_START;
        }
        // Reset state and restart.
        else if (event.key.code == sf::Keyboard::C)
        {
            clear();
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
            updateResourceControl();
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
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::H) {
            tutorialVisible = !tutorialVisible;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape && tutorialVisible) {
            tutorialVisible = false;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape)
        {
            gameSceneState = gameSeceneState::SCENE_START;
        }
        // Reset state and restart.
        else if (event.key.code == sf::Keyboard::C)
        {
            clear();
        }
    }
    if (tutorialVisible) {
        return;
    }
    if (endGame.checkMouse(mousePos, event) == RELEASE) {
        gameSceneState = SCENE_START;
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
        upgradeTeam(PLAYER);
        upgradeBtn.setState(NORMAL);
    }

    if (barracksBtn.checkMouse(mousePos, event) == RELEASE) {
        requestAutoBuildBarracks(PLAYER);
        barracksBtn.setState(NORMAL);
        return;
    }

    if (towerBtn.checkMouse(mousePos, event) == RELEASE) {
        requestAutoBuildTower(PLAYER);
        towerBtn.setState(NORMAL);
        return;
    }

    if (inf.checkMouse(mousePos, event) == RELEASE) {
        enqueueUnit(PLAYER, UName::INFANTARY);
        inf.setState(NORMAL);
    }
    if (sho.checkMouse(mousePos, event) == RELEASE) {
        enqueueUnit(PLAYER, UName::SHOOTER);
        sho.setState(NORMAL);
    }
    if (cav.checkMouse(mousePos, event) == RELEASE) {
        enqueueUnit(PLAYER, UName::CAVALRY);
        cav.setState(NORMAL);
    }
    if (siegeBtn.checkMouse(mousePos, event) == RELEASE) {
        enqueueUnit(PLAYER, UName::SIEGE);
        siegeBtn.setState(NORMAL);
    }
    if (guardianBtn.checkMouse(mousePos, event) == RELEASE) {
        enqueueUnit(PLAYER, UName::GUARDIAN);
        guardianBtn.setState(NORMAL);
    }
}

void Game::handleLaneInput(Vector2i mousePos, Event event)
{
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const float y = 148.f;
    const float w = 40.f;
    const float h = 24.f;
    for (int i = 0; i < lane::Count; ++i) {
        const sf::FloatRect rect(config::PanelX + 14.f + static_cast<float>(i) * 47.f, y, w, h);
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
    case SCENE_START:
        back.setPosition(Vector2f(0,0));
        window.draw(back);
        startBtn.setPosition(600, 200);
        helpBtn.setPosition(600, 290);
        window.draw(startBtn);
        window.draw(helpBtn);
        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        break;
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
        drawResourceNodes();
        drawBuildings();
        drawWorkers();

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
    case SCEN_GAMEOVER:
        if (gameWin == false) {
            Globle_text.setString("You Lose");
            window.draw(Globle_text);
        }
        if (gameWin == true) {
            Globle_text.setString("You Win");
            window.draw(Globle_text);
        }
        endGame.setPosition(600,300);
        window.draw(endGame);
        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        break;
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

void Game::drawResourceNodes()
{
    for (const auto& node : resources) {
        const sf::Vector2f center(
            node.point.x * SqureSize + SqureSize / 2.f,
            node.point.y * SqureSize + SqureSize / 2.f);
        const float pulse = 1.f + std::sin(node.pulseClock.getElapsedTime().asSeconds() * 4.f) * 0.08f;
        const sf::Color color = ownerColor(node.owner);
        const sf::Color kindColor = resourceKindColor(node.kind);

        sf::CircleShape aura(11.f * pulse, 32);
        aura.setOrigin(11.f * pulse, 11.f * pulse);
        aura.setPosition(center);
        aura.setFillColor(sf::Color(color.r, color.g, color.b, 48));
        aura.setOutlineColor(sf::Color(color.r, color.g, color.b, 170));
        aura.setOutlineThickness(1.2f);
        window.draw(aura);

        sf::ConvexShape crystal(6);
        crystal.setPoint(0, center + sf::Vector2f(0.f, -9.f));
        crystal.setPoint(1, center + sf::Vector2f(7.f, -3.f));
        crystal.setPoint(2, center + sf::Vector2f(5.f, 6.f));
        crystal.setPoint(3, center + sf::Vector2f(0.f, 10.f));
        crystal.setPoint(4, center + sf::Vector2f(-5.f, 6.f));
        crystal.setPoint(5, center + sf::Vector2f(-7.f, -3.f));
        crystal.setFillColor(kindColor);
        crystal.setOutlineColor(sf::Color(color.r, color.g, color.b, 220));
        crystal.setOutlineThickness(1.2f);
        window.draw(crystal);

        if (node.captureProgress > 0.f) {
            const float pct = std::clamp(node.captureProgress / config::ResourceCaptureSeconds, 0.f, 1.f);
            sf::RectangleShape capBg(sf::Vector2f(18.f, 3.f));
            capBg.setPosition(center + sf::Vector2f(-9.f, 12.f));
            capBg.setFillColor(sf::Color(31, 27, 21, 180));
            window.draw(capBg);
            sf::RectangleShape capBar(sf::Vector2f(18.f * pct, 3.f));
            capBar.setPosition(capBg.getPosition());
            capBar.setFillColor(node.contestingTeam == PLAYER ? sf::Color(255, 220, 93) : sf::Color(145, 196, 255));
            window.draw(capBar);
        }

        sf::Text label(std::string(resourceKindShortName(node.kind)) + "+" + std::to_string(node.income), myfont, 11);
        label.setFillColor(sf::Color(63, 49, 25));
        label.setOutlineColor(sf::Color(255, 245, 190, 180));
        label.setOutlineThickness(0.8f);
        label.setPosition(center.x + 8.f, center.y - 14.f);
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
        else if (building.complete && building.type == building::Extractor) {
            sf::CircleShape lamp(2.6f, 12);
            lamp.setOrigin(2.6f, 2.6f);
            lamp.setPosition(origin + sf::Vector2f(17.f, 4.f));
            lamp.setFillColor(hasActiveHarvester(building) ? sf::Color(255, 224, 76) : sf::Color(88, 82, 67));
            window.draw(lamp);
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
        if (workerUnit.state == worker::Harvesting) {
            sf::CircleShape gatherAura(7.8f, 28);
            gatherAura.setOrigin(7.8f, 7.8f);
            gatherAura.setPosition(center);
            gatherAura.setFillColor(sf::Color(255, 221, 96, 48));
            gatherAura.setOutlineColor(sf::Color(255, 221, 96, 155));
            gatherAura.setOutlineThickness(1.f);
            window.draw(gatherAura);
        }

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
        else if (workerUnit.state == worker::Harvesting) {
            sf::CircleShape cargo(2.2f, 12);
            cargo.setOrigin(2.2f, 2.2f);
            cargo.setPosition(center + sf::Vector2f(4.5f, -4.5f));
            cargo.setFillColor(sf::Color(255, 233, 93));
            cargo.setOutlineColor(sf::Color(93, 69, 29));
            cargo.setOutlineThickness(0.6f);
            window.draw(cargo);
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
        "  Left click a resource node: queue an Extractor. The base auto-sends a drone.",
        "  Click Top / Mid / Bot, then click unit buttons to send new troops to that lane.",
        "  Barracks and Tower buttons auto-place buildings near your base or lane defense.",
        "  Click Upgrade: spend CMD to gain a LEVEL and choose one rogue tactic card.",
        "",
        "Automation",
        "  Drones auto-build first. Extra drones cost CMD and train at the base.",
        "  Extractors only pay income while a drone is harvesting and the mine is not contested.",
        "  Combat units auto-path down their lane, fight enemies, then raid buildings.",
        "  Towers auto-fire at long range and are strong, but cost more and have a cap.",
        "  Main bases have a timed shield; siege units are the clean finisher.",
        "  If all Barracks fall, the base slowly drafts emergency troops.",
        "  Enemy mines can be captured by holding the area with more units than defenders.",
        "  Resource roles: Gold pays CMD, Crystal/Shrine discount LEVEL, Foundry speeds training.",
        "",
        "Tactics and counters",
        "  Every tech upgrade gives 3 tactic cards. Max tech is LEVEL 15.",
        "  Perks stack, so late builds can bend the soft counter rules.",
        "  Shooter > Infantry, Infantry > Cavalry, Cavalry > Shooter/Siege.",
        "  Siege cracks Guardians and buildings; Guardians anchor against Cavalry dives.",
        "",
        "Unlocks",
        "  Infantry: 15 CMD, needs 1 Barracks.",
        "  Shooter: 22 CMD, needs 1 Barracks and 1 Mine or Upgrade 1.",
        "  Cavalry: 38 CMD, needs 2 Barracks and 2 Mines or LEVEL 3.",
        "  Siege: 56 CMD, needs LEVEL 5, 2 Barracks, 2 Mines; shreds buildings.",
        "  Guardian: 74 CMD, needs LEVEL 7, 3 Barracks, 3 Mines; anchors pushes.",
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
        badge.setFillColor(resourceKindColor(i == 0 ? resource::Gold : (i == 1 ? resource::Crystal : resource::Foundry)));
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
    barracksBtn.setPosition(config::ButtonX, config::BuildBarracksY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
    siegeBtn.setPosition(config::ButtonX, config::BuildSiegeY);
    guardianBtn.setPosition(config::ButtonX, config::BuildGuardianY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);

    window.draw(sidePanel);

    sf::RectangleShape accentLine(sf::Vector2f(3.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(219, 166, 75));
    window.draw(accentLine);

    const auto drawPanelCard = [this](float y, float h, sf::Color fill, sf::Color outline) {
        sf::RectangleShape shadow(sf::Vector2f(config::PanelWidth - 20.f, h));
        shadow.setPosition(config::PanelX + 12.f, y + 4.f);
        shadow.setFillColor(sf::Color(8, 11, 10, 70));
        window.draw(shadow);

        sf::RectangleShape card(sf::Vector2f(config::PanelWidth - 24.f, h));
        card.setPosition(config::PanelX + 12.f, y);
        card.setFillColor(fill);
        card.setOutlineColor(outline);
        card.setOutlineThickness(1.4f);
        window.draw(card);
    };

    drawPanelCard(8.f, 72.f, sf::Color(47, 58, 51), sf::Color(93, 103, 81));
    drawPanelCard(86.f, 116.f, sf::Color(41, 50, 45), sf::Color(77, 92, 74));
    drawPanelCard(208.f, 68.f, sf::Color(60, 47, 31), sf::Color(197, 150, 67));
    drawPanelCard(292.f, 392.f, sf::Color(42, 49, 44), sf::Color(81, 91, 73));

    window.draw(panelTitle);
    window.draw(Globle_text);
    if (!realtimeMode) {
        window.draw(UnitText);
        window.draw(UnitAttack);
        window.draw(UnitHP);
    }
    CommandText.setCharacterSize(realtimeMode ? 10 : 14);
    CommandText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), realtimeMode ? 94.f : 176.f);
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
    CommandText.setString("CMD " + std::to_string(playerCommand)
        + "/" + std::to_string(config::MaxCommand)
        + "  Tick +" + std::to_string(resourceIncome(PLAYER))
        + "\nLv " + std::to_string(playerUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "  Up " + std::to_string(upgradeCostForNextLevel(PLAYER))
        + "\nG/C/F/S " + std::to_string(controlledResourceKindCount(PLAYER, resource::Gold))
        + "/" + std::to_string(controlledResourceKindCount(PLAYER, resource::Crystal))
        + "/" + std::to_string(controlledResourceKindCount(PLAYER, resource::Foundry))
        + "/" + std::to_string(controlledResourceKindCount(PLAYER, resource::Shrine))
        + "\nDrone " + std::to_string(workerCount(PLAYER))
        + "/" + std::to_string(realtime::MaxWorkers)
        + " Rax " + std::to_string(completedBuildingCount(PLAYER, building::Barracks))
        + "/" + std::to_string(buildingCap(PLAYER, building::Barracks))
        + "\nTower " + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower))
        + "/" + std::to_string(buildingCap(PLAYER, building::DefenseTower))
        + " Army " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits));
    window.draw(CommandText);

    sf::Text perkText(std::string(inspectingEnemyBase ? "Enemy" : "Yours") + " Lv" + std::to_string(shownLevel)
        + "  " + perkLine(), myfont, 9);
    perkText.setFillColor(sf::Color(255, 226, 142));
    perkText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 172.f);
    window.draw(perkText);

    if (!realtimeMode) {
        window.draw(EndTurnBtn);
    }
    else {
        upgradeBtn.setColor(commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
            && completedBuildingCount(PLAYER, building::Barracks) > 0
            && playerUpgradeLevel < config::MaxTechLevel
            ? sf::Color::White
            : sf::Color(255, 255, 255, 130));
        window.draw(upgradeBtn);
    }

    const float laneY = 148.f;
    for (int i = 0; i < lane::Count; ++i) {
        sf::RectangleShape laneButton(sf::Vector2f(40.f, 24.f));
        laneButton.setPosition(config::PanelX + 14.f + static_cast<float>(i) * 47.f, laneY);
        laneButton.setFillColor(playerSelectedLane == i ? sf::Color(217, 166, 75) : sf::Color(48, 60, 52));
        laneButton.setOutlineColor(sf::Color(231, 221, 168));
        laneButton.setOutlineThickness(playerSelectedLane == i ? 1.6f : 0.8f);
        window.draw(laneButton);

        sf::Text laneText(laneName(i), myfont, 11);
        laneText.setFillColor(playerSelectedLane == i ? sf::Color(41, 31, 20) : sf::Color(224, 232, 203));
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 4.f));
        window.draw(laneText);
    }

    panelHint.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 182.f);
    panelHint.setString("Upgrade = rogue pick\nMine nodes manually");
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

    window.draw(panelHint);
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
    playerWorkerTimer = realtime::WorkerTrainSeconds;
    aiWorkerTimer = realtime::WorkerTrainSeconds;
    playerBaseAttackTimer = 0.f;
    aiBaseAttackTimer = 0.f;
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
        int kind = resource::Gold;
        int income = config::ResourceCommandIncome;
    };
    const std::vector<ResourceTarget> targets = {
        {Point(Red_baseP.x + 8, Red_baseP.y + 4), resource::Gold, 6},
        {Point(Blue_baseP.x - 8, Blue_baseP.y - 4), resource::Gold, 6},
        {Point(mapW / 2, mapH / 2), resource::Shrine, 8},
        {Point(mapW / 2, mapH / 4), resource::Crystal, 6},
        {Point(mapW / 2, mapH * 3 / 4), resource::Crystal, 6},
        {Point(mapW / 3, mapH / 2), resource::Foundry, 7},
        {Point(mapW * 2 / 3, mapH / 2), resource::Foundry, 7}
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

        // Clear a small capture plaza so resource fights and hand-offs are readable.
        for (int y = best.y - 2; y <= best.y + 2; ++y) {
            for (int x = best.x - 2; x <= best.x + 2; ++x) {
                if (isMapCell(x, y)) {
                    setTileID(x, y, tile::Empty);
                }
            }
        }
        ResourceNode node;
        node.point = best;
        node.owner = -1;
        node.kind = target.kind;
        node.income = target.income;
        resources.push_back(node);
        setTileID(best.x, best.y, tile::Resource);
    }
}

void Game::updateResourceControl()
{
    for (std::size_t i = 0; i < resources.size(); ++i) {
        auto& node = resources[i];
        const int previousOwner = node.owner;
        node.owner = -1;
        const Building* extractor = findResourceExtractor(static_cast<int>(i));
        if (extractor != nullptr && extractor->complete) {
            const int enemy = extractor->team == PLAYER ? AI : PLAYER;
            const bool contested = node.contestingTeam == enemy && node.captureProgress > 0.f;
            node.owner = contested ? -2 : (hasActiveHarvester(*extractor) ? extractor->team : -1);
        }

        if (node.owner != previousOwner) {
            node.pulseClock.restart();
            const sf::Vector2f pos(node.point.x * SqureSize + SqureSize / 2.f, node.point.y * SqureSize - 4.f);
            const std::string text = node.owner == PLAYER ? std::string(resourceKindName(node.kind)) + " ONLINE"
                : (node.owner == AI ? std::string("ENEMY ") + resourceKindName(node.kind) : (node.owner == -2 ? "CONTESTED" : std::string(resourceKindName(node.kind)) + " IDLE"));
            const sf::Color color = node.owner == PLAYER
                ? sf::Color(255, 220, 93)
                : (node.owner == AI ? sf::Color(145, 196, 255) : (node.owner == -2 ? sf::Color(255, 126, 84) : sf::Color(205, 196, 158)));
            addFloatingText(pos, text, color, 12);
            startScreenShake(0.10f, 1.5f);
        }
    }
}

int Game::resourceIncome(int team) const
{
    int income = config::BaseCommandIncome;
    int owned = 0;
    for (const auto& node : resources) {
        if (node.owner == team) {
            ++owned;
            income += static_cast<int>(std::round(static_cast<float>(node.income) * miningIncomeMultiplier(team)));
        }
    }
    // A small fallback stipend keeps a losing side from becoming inert; the
    // winner still snowballs through mines, but the defender can rebuild.
    if (gameTimeSeconds > 300.f && owned == 0) {
        income += 3;
    }
    else if (gameTimeSeconds > 600.f && owned <= 1) {
        income += 2;
    }
    return income;
}

int Game::controlledResourceCount(int team) const
{
    return static_cast<int>(std::count_if(resources.begin(), resources.end(), [team](const ResourceNode& node) {
        return node.owner == team;
    }));
}

int Game::controlledResourceKindCount(int team, int kind) const
{
    return static_cast<int>(std::count_if(resources.begin(), resources.end(), [team, kind](const ResourceNode& node) {
        return node.owner == team && node.kind == kind;
    }));
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
        const int bonus = 38 + (team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * 5
            + controlledResourceKindCount(team, resource::Shrine) * 8;
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
    if (gameTimeSeconds > 720.f && perkLevel(AI, perk::SiegeCraft) < maxPerkLevel(perk::SiegeCraft)) {
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
    const int extractors = completedBuildingCount(team, building::Extractor);
    const int barracks = completedBuildingCount(team, building::Barracks);
    const int upgrade = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    switch (name) {
    case UName::INFANTARY:
        return barracks >= 1;
    case UName::SHOOTER:
        return barracks >= 1 && (extractors >= 1 || upgrade >= 1);
    case UName::CAVALRY:
        return barracks >= 2 && (extractors >= 2 || upgrade >= 3);
    case UName::SIEGE:
        return barracks >= 2 && extractors >= 2 && upgrade >= 5;
    case UName::GUARDIAN:
        return barracks >= 3 && extractors >= 3 && upgrade >= 7;
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

    const auto distanceScore = [](Point a, Point b) {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    };
    const auto chooseResource = [this, distanceScore]() {
        int bestIndex = -1;
        int bestScore = std::numeric_limits<int>::max();
        for (int i = 0; i < static_cast<int>(resources.size()); ++i) {
            if (pendingOrCompleteExtractorForResource(i) != 0) {
                continue;
            }
            // Prefer the safe natural first, then let income value pull the AI
            // toward central/flank nodes later in the match.
            int score = distanceScore(resources[i].point, Blue_baseP) - resources[i].income * 45;
            if (gameTimeSeconds > 70.f && resources[i].income >= 8) {
                score -= 160;
            }
            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        return bestIndex;
    };

    const int totalExtractors = totalBuildingCount(AI, building::Extractor);
    const int totalBarracks = totalBuildingCount(AI, building::Barracks);
    const int totalTowers = totalBuildingCount(AI, building::DefenseTower);
    const int completedExtractors = completedBuildingCount(AI, building::Extractor);
    const int completedBarracks = completedBuildingCount(AI, building::Barracks);
    const int currentBarracksCap = buildingCap(AI, building::Barracks);
    const int currentTowerCap = buildingCap(AI, building::DefenseTower);
    const auto allowedTechForAI = [this, completedBarracks, completedExtractors]() {
        int allowedTech = (completedBarracks > 0 || gameTimeSeconds > 360.f)
            ? static_cast<int>(gameTimeSeconds / 66.f)
            : 0;
        if (completedExtractors < 2) {
            allowedTech = std::min(allowedTech, 5);
        }
        if (completedExtractors < 3) {
            allowedTech = std::min(allowedTech, 9);
        }
        if (gameTimeSeconds > 720.f) {
            allowedTech = std::max(allowedTech, 12 + static_cast<int>((gameTimeSeconds - 720.f) / 55.f));
        }
        if (gameTimeSeconds > 870.f) {
            allowedTech = config::MaxTechLevel;
        }
        return std::clamp(allowedTech, 0, config::MaxTechLevel);
    };

    // Late AI tech can continue from the main base even after the player raids
    // its barracks, so a dragged-out game still escalates toward max pressure.
    const int catchupAllowedTech = allowedTechForAI();
    if (gameTimeSeconds > 420.f
        && aiUpgradeLevel < catchupAllowedTech
        && commandForTeam(AI) >= upgradeCostForNextLevel(AI)) {
        if (upgradeTeam(AI)) {
            return;
        }
    }

    if (totalExtractors < 1
        && !(gameTimeSeconds > 420.f && totalBarracks < 1)
        && commandForTeam(AI) >= config::ExtractorCost) {
        const int resourceIndex = chooseResource();
        if (resourceIndex >= 0 && requestBuildExtractor(AI, resourceIndex)) {
            return;
        }
    }

    if (totalBarracks < 1 && commandForTeam(AI) >= config::BarracksCost) {
        if (requestAutoBuildBarracks(AI)) {
            return;
        }
    }
    if (gameTimeSeconds > 420.f && totalBarracks < 1) {
        return;
    }

    int desiredExtractors = 1;
    if (gameTimeSeconds > 50.f) {
        desiredExtractors = 2;
    }
    if (gameTimeSeconds > 120.f) {
        desiredExtractors = 3;
    }
    if (gameTimeSeconds > 300.f && aiUpgradeLevel >= 6) {
        desiredExtractors = 4;
    }
    if (gameTimeSeconds > 650.f && aiUpgradeLevel >= 10) {
        desiredExtractors = 5;
    }
    if (controlledResourceCount(AI) + 1 < controlledResourceCount(PLAYER) && gameTimeSeconds > 130.f) {
        desiredExtractors += 1;
    }
    desiredExtractors = std::min({desiredExtractors, static_cast<int>(resources.size()), realtime::MaxWorkers - 1});
    if (totalExtractors < desiredExtractors && commandForTeam(AI) >= config::ExtractorCost) {
        const int resourceIndex = chooseResource();
        if (resourceIndex >= 0 && requestBuildExtractor(AI, resourceIndex)) {
            return;
        }
    }
    if (totalExtractors < desiredExtractors) {
        return;
    }

    const int allowedTech = allowedTechForAI();

    if (aiUpgradeLevel < std::min(config::MaxTechLevel, allowedTech)
        && commandForTeam(AI) >= upgradeCostForNextLevel(AI)) {
        if (upgradeTeam(AI)) {
            return;
        }
    }
    if (aiUpgradeLevel < std::min(config::MaxTechLevel, allowedTech)) {
        return;
    }

    int desiredBarracks = 1;
    if (gameTimeSeconds > 95.f && completedExtractors >= 2) {
        desiredBarracks = 2;
    }
    if (gameTimeSeconds > 300.f && completedExtractors >= 2 && aiUpgradeLevel >= 4) {
        desiredBarracks = 3;
    }
    if (gameTimeSeconds > 520.f && completedExtractors >= 3 && aiUpgradeLevel >= 9) {
        desiredBarracks = 4;
    }
    if (gameTimeSeconds > 720.f && aiUpgradeLevel >= 12) {
        desiredBarracks = 5;
    }
    if (gameTimeSeconds > 870.f && aiUpgradeLevel >= 14) {
        desiredBarracks = 6;
    }
    desiredBarracks = std::min(desiredBarracks, currentBarracksCap);
    if (totalBarracks < desiredBarracks && commandForTeam(AI) >= config::BarracksCost) {
        if (requestAutoBuildBarracks(AI)) {
            return;
        }
    }
    if (totalBarracks < desiredBarracks) {
        return;
    }

    int desiredTowers = 0;
    if (gameTimeSeconds > 140.f && completedBarracks > 0) {
        desiredTowers = 1;
    }
    if (gameTimeSeconds > 250.f && completedBarracks >= 2) {
        desiredTowers = 2;
    }
    if (gameTimeSeconds > 500.f && aiUpgradeLevel >= 8) {
        desiredTowers = 3;
    }
    if (gameTimeSeconds > 780.f && aiUpgradeLevel >= 12) {
        desiredTowers = 4;
    }
    desiredTowers = std::min(desiredTowers, currentTowerCap);
    if (totalTowers < desiredTowers && commandForTeam(AI) >= config::TowerCost + config::InfantryCost) {
        if (requestAutoBuildTower(AI)) {
            return;
        }
    }

    const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
    const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
    const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
    const int playerSiege = countUnitsNamed(myunits, UName::SIEGE);
    const int playerGuardian = countUnitsNamed(myunits, UName::GUARDIAN);
    const int counterPick = playerShooters > playerInfantry
        ? UName::CAVALRY
        : (playerCavalry + playerGuardian > playerShooters ? UName::INFANTARY : UName::SHOOTER);
    const int pressurePick = enemys.size() + 2 < myunits.size() ? UName::CAVALRY : UName::INFANTARY;
    int techPick = counterPick;
    if (isUnitUnlocked(AI, UName::GUARDIAN)) {
        techPick = UName::GUARDIAN;
    }
    else if (isUnitUnlocked(AI, UName::SIEGE)) {
        techPick = playerSiege > 0 ? UName::CAVALRY : UName::SIEGE;
    }
    const int priorities[] = {
        techPick,
        counterPick,
        pressurePick,
        UName::SIEGE,
        UName::GUARDIAN,
        UName::SHOOTER,
        UName::INFANTARY
    };

    const int orders = std::min(completedBarracks, realtime::AIUnitsPerBurst + aiUpgradeLevel / 4 + (gameTimeSeconds > 840.f ? 2 : 0));
    for (int i = 0; i < orders; ++i) {
        aiSelectedLane = gameTimeSeconds > 780.f
            ? i % lane::Count
            : (static_cast<int>(gameTimeSeconds / 35.f) + i) % lane::Count;
        bool queued = false;
        for (int code : priorities) {
            if (enqueueUnit(AI, code)) {
                queued = true;
                break;
            }
        }
        if (!queued) {
            break;
        }
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

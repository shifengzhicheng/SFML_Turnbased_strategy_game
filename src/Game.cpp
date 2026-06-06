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
    art::makeButtonTexture(tinf, myfont, "Infantry", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfHover, myfont, "Infantry", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfClick, myfont, "Infantry", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tcav, myfont, "Cavalry", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavHover, myfont, "Cavalry", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavClick, myfont, "Cavalry", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tsho, myfont, "Shooter", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoHover, myfont, "Shooter", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoClick, myfont, "Shooter", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tUpgrade, myfont, "UPGRADE", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tUpgradeHover, myfont, "UPGRADE", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tUpgradeClick, myfont, "UPGRADE", art::ButtonState::Pressed, sf::Vector2u(128, 44));
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
    upgradeBtn.setTextures(tUpgrade, tUpgradeHover, tUpgradeClick);
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
    setupText(panelHint, myfont, 13, sf::Color(211, 199, 165), "Select base\nto train", panelTextX, 232.f);
    setupText(infantryLabel, myfont, 11, sf::Color(228, 218, 185), "Rax core unit", panelTextX, config::BuildInfantryY + 57.f);
    setupText(shooterLabel, myfont, 11, sf::Color(228, 218, 185), "Mine unlock", panelTextX, config::BuildShooterY + 57.f);
    setupText(cavalryLabel, myfont, 11, sf::Color(228, 218, 185), "2 rax + 2 mines", panelTextX, config::BuildCavalryY + 57.f);
    setupText(towerLabel, myfont, 11, sf::Color(228, 218, 185), "Base build mode", panelTextX, config::BuildTowerY + 46.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(35, 43, 39));
    sidePanel.setOutlineColor(sf::Color(19, 24, 22));
    sidePanel.setOutlineThickness(2.f);

    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    helpBtn.setPosition(config::ButtonX, config::HelpButtonY);
    upgradeBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);
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
    assignWorkers();
    updateWorkers(dt);
    updateProduction(dt);
    updateDefenseTowers(dt);
    realtime::updateAutoCombat(*this, dt);
    cleanupDestroyedBuildings();
    updateDebugSummary(dt);
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
        << " rax=" << completedBuildingCount(PLAYER, building::Barracks) << "/" << config::BarracksCap
        << " tower=" << totalBuildingCount(PLAYER, building::DefenseTower)
        << " level=" << playerUpgradeLevel
        << " army=" << myunits.size()
        << " | ai cmd=" << aiCommand
        << " mines=" << completedBuildingCount(AI, building::Extractor)
        << " rax=" << completedBuildingCount(AI, building::Barracks) << "/" << config::BarracksCap
        << " tower=" << totalBuildingCount(AI, building::DefenseTower)
        << " level=" << aiUpgradeLevel
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

int Game::buildingCap(int type) const
{
    if (type == building::Barracks) {
        return config::BarracksCap;
    }
    if (type == building::DefenseTower) {
        return config::TowerCap;
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

int Game::upgradeCostForNextLevel(int team) const
{
    const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    return config::UpgradeCost + level * config::UpgradeCostStep;
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
    if (totalBuildingCount(team, building::Barracks) >= buildingCap(building::Barracks)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Rax cap", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y) || commandPool(*this, team) < config::BarracksCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::BarracksCost ? "Need CMD" : "Bad site",
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
    if (totalBuildingCount(team, building::DefenseTower) >= buildingCap(building::DefenseTower)) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            "Tower cap", sf::Color(255, 214, 96), 12);
        }
        return false;
    }
    if (!isBuildableCell(point.x, point.y) || commandPool(*this, team) < config::TowerCost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(point.x * SqureSize, point.y * SqureSize - 8.f),
                            commandPool(*this, team) < config::TowerCost ? "Need CMD" : "Bad site",
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
        if (worker == nullptr) {
            if (workerCount(building.team) < realtime::MaxWorkers) {
                createWorker(building.team, workerSpawnPoint(building.team));
                worker = &workers.back();
            }
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
        if (worker == nullptr && workerCount(building.team) < realtime::MaxWorkers) {
            createWorker(building.team, workerSpawnPoint(building.team));
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
            building.production.activeUnit = building.production.orders.front();
            building.production.orders.pop_front();
            building.production.progress = 0.f;
        }
        if (building.production.activeUnit < 0) {
            continue;
        }

        building.production.progress += dt;
        if (building.production.progress < unitTrainSeconds(building.production.activeUnit)) {
            continue;
        }

        const Point spawn = findSpawnPointAround(building.point);
        if (spawn.x >= 0 && createUnit(building.team, building.production.activeUnit, spawn.x, spawn.y)) {
            building.production.activeUnit = -1;
            building.production.progress = 0.f;
        }
    }
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

        const int enemy = extractor->team == PLAYER ? AI : PLAYER;
        const int defenders = unitsNearPoint(extractor->team, node.point, config::ResourceCaptureRadius);
        const int attackers = unitsNearPoint(enemy, node.point, config::ResourceCaptureRadius);
        if (attackers <= defenders || attackers <= 0) {
            node.captureProgress = std::max(0.f, node.captureProgress - dt * 0.7f);
            if (node.captureProgress <= 0.f) {
                node.contestingTeam = -1;
            }
            continue;
        }

        if (node.contestingTeam != enemy) {
            node.contestingTeam = enemy;
            node.captureProgress = 0.f;
        }
        node.captureProgress += dt * static_cast<float>(attackers - defenders);
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
    const int rangeSquared = config::DefenseTowerRange * config::DefenseTowerRange;

    for (auto& building : buildings) {
        if (!building.complete || building.type != building::DefenseTower) {
            continue;
        }

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
        const int damage = std::max(1, static_cast<int>(std::round(static_cast<float>(config::DefenseTowerDamage) * damageMultiplier(building.team))));
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
        logEvent(std::string(destroyed.team == PLAYER ? "player" : "ai") + " "
            + buildingName(destroyed.type) + " destroyed id=" + std::to_string(destroyed.id));
        it = buildings.erase(it);
    }
}

Building* Game::chooseBuildingTarget(MoveableUnit& unit)
{
    Building* best = nullptr;
    int bestScore = std::numeric_limits<int>::max();
    for (auto& building : buildings) {
        if (building.team == unit.myteam || building.health <= 0 || !building.complete) {
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

        const int score = priority + distanceSquared(Point(unit.x, unit.y), building.point);
        if (score < bestScore) {
            bestScore = score;
            best = &building;
        }
    }
    return best;
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

    const float typeFactor = unit.unitName == UName::CAVALRY ? 1.15f : (unit.unitName == UName::SHOOTER ? 0.82f : 1.f);
    const int damage = std::max(1, static_cast<int>(std::round(static_cast<float>(unit.myattack())
        * damageMultiplier(unit.myteam) * config::BuildingDamageFactor * typeFactor)));
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

    if (realtimeMode) {
        updateRealtime(std::min(realtimeFrameClock.restart().asSeconds(), 0.05f));
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

    if (Base_red && Base_red->UnitState == UState::UNITCLICK && isBuildableCell(tileX, tileY)) {
        if (towerPlacementMode) {
            if (requestBuildTower(PLAYER, Point(tileX, tileY))) {
                towerPlacementMode = false;
            }
        }
        else {
            requestBuildBarracks(PLAYER, Point(tileX, tileY));
        }
    }
}

void Game::GameInput(Vector2i mousePos, Event event) {
    const bool mouseInMap = mousePos.x >= 0 && mousePos.x < width && mousePos.y >= 0 && mousePos.y < height;
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
    return createUnit(team, name, x, y);
}

bool Game::createUnit(int team, int name, int x, int y)
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
    default:
        return false;
    }

    unit->entityId = nextEntityId++;
    if (team == PLAYER) {
        myunits.push_back(std::move(unit));
    }
    else {
        enemys.push_back(std::move(unit));
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " spawned unit=" + std::to_string(name)
        + " at " + std::to_string(x) + "," + std::to_string(y));
    return true;
}

void Game::handleBuildButtons(Vector2i mousePos, Event event)
{
    if (helpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        helpBtn.setState(NORMAL);
        return;
    }

    if (upgradeBtn.checkMouse(mousePos, event) == RELEASE) {
        upgradeTeam(PLAYER);
        upgradeBtn.setState(NORMAL);
    }

    if (towerBtn.checkMouse(mousePos, event) == RELEASE) {
        if (Base_red && Base_red->UnitState == UState::UNITCLICK) {
            towerPlacementMode = !towerPlacementMode;
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildTowerY) - 18.f),
                            towerPlacementMode ? "Click land" : "Tower off", sf::Color(218, 255, 134), 12);
        }
        else {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildTowerY) - 18.f),
                            "Select base", sf::Color(255, 214, 96), 12);
        }
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
        crystal.setFillColor(sf::Color(255, 211, 82));
        crystal.setOutlineColor(sf::Color(100, 74, 30));
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

        sf::Text label("+" + std::to_string(node.income), myfont, 11);
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
            const float pct = std::clamp(building.production.progress / unitTrainSeconds(building.production.activeUnit), 0.f, 1.f);
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
        "  Grow economy, build barracks, then let your army auto-push the enemy base.",
        "",
        "Core controls",
        "  Left click a gold node: queue an Extractor. The base auto-sends a drone.",
        "  Select the red base, then left click open land: queue a Barracks.",
        "  Select the red base, click Tower, then click open land: build defense.",
        "  Click Infantry / Shooter / Cavalry: queue units in the least-busy Barracks.",
        "  Click Upgrade: spend CMD to enter the next LEVEL; all units hit harder.",
        "",
        "Automation",
        "  Drones auto-build first. When work is done, they return to harvesting.",
        "  Extractors only pay income while a drone is harvesting and the mine is not contested.",
        "  Combat units auto-path, attack enemy units, then raid towers, mines, and barracks.",
        "  Towers auto-fire at enemies inside range and also scale with LEVEL.",
        "  Enemy mines can be captured by holding the area with more units than defenders.",
        "",
        "Unlocks",
        "  Infantry: 9 CMD, needs 1 Barracks.",
        "  Shooter: 14 CMD, needs 1 Barracks and 1 Mine or Upgrade 1.",
        "  Cavalry: 22 CMD, needs 2 Barracks and 2 Mines or Upgrade 2.",
        "",
        "Hotkeys",
        "  H: show / hide this guide.  C: restart map.  Esc: back to menu."
    };

    float y = 126.f;
    for (const auto& line : lines) {
        const bool section = !line.empty() && line.front() != ' ';
        sf::Text text(line, myfont, section ? 18 : 14);
        text.setFillColor(section ? sf::Color(255, 218, 112) : sf::Color(224, 232, 203));
        text.setPosition(228.f, y);
        window.draw(text);
        y += line.empty() ? 12.f : (section ? 28.f : 22.f);
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
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);

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
    drawPanelCard(86.f, 126.f, sf::Color(41, 50, 45), sf::Color(77, 92, 74));
    drawPanelCard(208.f, 72.f, sf::Color(60, 47, 31), sf::Color(197, 150, 67));
    drawPanelCard(292.f, 386.f, sf::Color(42, 49, 44), sf::Color(81, 91, 73));

    window.draw(panelTitle);
    window.draw(Globle_text);
    if (!realtimeMode) {
        window.draw(UnitText);
        window.draw(UnitAttack);
        window.draw(UnitHP);
    }
    CommandText.setCharacterSize(realtimeMode ? 12 : 14);
    CommandText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), realtimeMode ? 94.f : 176.f);
    CommandText.setString("CMD: " + std::to_string(playerCommand)
        + "/" + std::to_string(config::MaxCommand)
        + "\nGold: " + std::to_string(controlledResourceCount(PLAYER))
        + "/" + std::to_string(resources.size())
        + "\nTick: +" + std::to_string(resourceIncome(PLAYER))
        + "\nDrone: " + std::to_string(workerCount(PLAYER))
        + "/" + std::to_string(realtime::MaxWorkers)
        + "\nRax: " + std::to_string(completedBuildingCount(PLAYER, building::Barracks))
        + "/" + std::to_string(config::BarracksCap)
        + "  Lv: " + std::to_string(playerUpgradeLevel)
        + "\nTower: " + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower))
        + "/" + std::to_string(config::TowerCap)
        + "  Up: " + std::to_string(upgradeCostForNextLevel(PLAYER))
        + "\nArmy: " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits));
    window.draw(CommandText);

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

    const bool canBuild = Base_red && Base_red->UnitState == UState::UNITCLICK;
    panelHint.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 216.f);
    panelHint.setString(canBuild
        ? (towerPlacementMode ? "Tower mode: click land\nClick gold: mine" : "Click land: rax\nTower btn: defense")
        : "Click gold: mine\nSelect base: build");
    inf.setColor(canQueueUnit(PLAYER, UName::INFANTARY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    sho.setColor(canQueueUnit(PLAYER, UName::SHOOTER) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    cav.setColor(canQueueUnit(PLAYER, UName::CAVALRY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    const bool canQueueTower = canBuild
        && commandForTeam(PLAYER) >= config::TowerCost
        && totalBuildingCount(PLAYER, building::DefenseTower) < config::TowerCap;
    towerBtn.setColor(canQueueTower ? sf::Color::White : sf::Color(255, 255, 255, 130));

    window.draw(panelHint);
    window.draw(helpBtn);
    window.draw(inf);
    window.draw(infantryLabel);
    window.draw(sho);
    window.draw(shooterLabel);
    window.draw(cav);
    window.draw(cavalryLabel);
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
    gameTimeSeconds = 0.f;
    debugSummaryTimer = 0.f;
    towerPlacementMode = false;
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
    int w = width / SqureSize;
    int h = height / SqureSize;
    bool isok=false;
    for (int x = 4, y = 4; x < w; x++) {
        for (y = 4; y < h; y++) {
            if (x - 1 > 0 && y - 1 > 0 && x + 1 < w && y + 1 < h) {
                if (maze[y - 1][x] == 0
                    && maze[y][x + 1] == 0
                    && maze[y + 1][x] == 0
                    && maze[y][x - 1] == 0) {
                    for (int i = y - 3, j = x - 3; i < y + 3; i++) {
                        for (j = x - 3; j < x + 3; j++) {
                            setTileID(j, i, tile::Empty);
                        }
                    }
                    setTileID(x, y, tile::Red_Base);
                    setTileID(x + 1, y, tile::Red_Base);
                    setTileID(x, y + 1, tile::Red_Base);
                    setTileID(x + 1, y + 1, tile::Red_Base);
                    Red_baseP = Point(x, y);
                    Base_red = make_unique<DisMoveableUnit>(x, y, PLAYER, this);
                    art::makeUnitTexture(Base_red->mytexture, art::UnitKind::Base, art::Team::Player);
                    Base_red->setTexture(Base_red->mytexture);
                    isok = true;
                    break;
                }
            }
        }
        if (isok) break;
    }
    isok = false;
    for (int x = w - 5, y = h - 5; x >= 0; x--) {
        for (y = h - 5; y >= 0; y--) {
            if (x - 1 > 0 && y - 1 > 0 && x + 1 < w && y + 1 < h) {
                if (maze[y - 1][x] == 0
                    && maze[y][x + 1] == 0
                    && maze[y + 1][x] == 0
                    && maze[y][x - 1] == 0) {
                    Blue_baseP = Point(x, y);
                    for (int i = y - 3, j = x - 3; i < y + 3; i++) {
                        for (j = x - 3; j < x + 3; j++) {
                            setTileID(j, i, tile::Empty);
                        }
                    }
                    setTileID(x, y, tile::Blue_Base);
                    setTileID(x + 1, y, tile::Blue_Base);
                    setTileID(x, y + 1, tile::Blue_Base);
                    setTileID(x + 1, y + 1, tile::Blue_Base);
                    Base_blue = make_unique<DisMoveableUnit>(x, y, AI, this);
                    art::makeUnitTexture(Base_blue->mytexture, art::UnitKind::Base, art::Team::Enemy);
                    Base_blue->setTexture(Base_blue->mytexture);
                    isok = true;
                    break;
                }
            }
        }
        if (isok) break;
    }
}

void Game::placeResourceNodes()
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    struct ResourceTarget
    {
        Point point;
        int income = config::ResourceCommandIncome;
    };
    const std::vector<ResourceTarget> targets = {
        {Point(Red_baseP.x + 8, Red_baseP.y + 4), 6},
        {Point(Blue_baseP.x - 8, Blue_baseP.y - 4), 6},
        {Point(mapW / 2, mapH / 2), 10},
        {Point(mapW / 2, mapH / 4), 7},
        {Point(mapW / 2, mapH * 3 / 4), 7},
        {Point(mapW / 3, mapH / 2), 8},
        {Point(mapW * 2 / 3, mapH / 2), 8}
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
            const std::string text = node.owner == PLAYER ? "MINE ONLINE"
                : (node.owner == AI ? "ENEMY MINE" : (node.owner == -2 ? "CONTESTED" : "MINE IDLE"));
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
    for (const auto& node : resources) {
        if (node.owner == team) {
            income += node.income;
        }
    }
    return income;
}

int Game::controlledResourceCount(int team) const
{
    return static_cast<int>(std::count_if(resources.begin(), resources.end(), [team](const ResourceNode& node) {
        return node.owner == team;
    }));
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
        return barracks >= 2 && (extractors >= 2 || upgrade >= 2);
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
    best->production.orders.push_back(name);
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(best->point.x * SqureSize, best->point.y * SqureSize - 8.f),
                        "Queued", sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued unit=" + std::to_string(name)
        + " rax=" + std::to_string(best->id) + " load=" + std::to_string(best->production.load()));
    return true;
}

bool Game::upgradeTeam(int team)
{
    int& level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int cost = upgradeCostForNextLevel(team);
    if (level >= config::MaxTechLevel || completedBuildingCount(team, building::Barracks) == 0 || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                            level >= config::MaxTechLevel ? "Max level" : (completedBuildingCount(team, building::Barracks) == 0 ? "Need Barracks" : "Need CMD"),
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    ++level;
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                        "LEVEL " + std::to_string(level), sf::Color(218, 255, 134), 12);
    }
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
            const int score = distanceScore(resources[i].point, Blue_baseP);
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

    if (totalExtractors < 1 && commandForTeam(AI) >= config::ExtractorCost) {
        const int resourceIndex = chooseResource();
        if (resourceIndex >= 0 && requestBuildExtractor(AI, resourceIndex)) {
            return;
        }
    }

    if (totalBarracks < 1 && commandForTeam(AI) >= config::BarracksCost) {
        const Point site = findBuildableNear(Blue_baseP, 9);
        if (site.x >= 0 && requestBuildBarracks(AI, site)) {
            return;
        }
    }

    const int desiredExtractors = std::min(3, std::max(1, (static_cast<int>(resources.size()) + 1) / 2));
    if (totalExtractors < desiredExtractors && commandForTeam(AI) >= config::ExtractorCost + config::InfantryCost) {
        const int resourceIndex = chooseResource();
        if (resourceIndex >= 0 && requestBuildExtractor(AI, resourceIndex)) {
            return;
        }
    }

    const int desiredBarracks = std::min(config::BarracksCap, 1 + totalExtractors + aiUpgradeLevel / 2);
    if (totalBarracks < desiredBarracks && commandForTeam(AI) >= config::BarracksCost + config::InfantryCost) {
        const Point site = findBuildableNear(Blue_baseP, 12);
        if (site.x >= 0 && requestBuildBarracks(AI, site)) {
            return;
        }
    }

    if (totalTowers < 2 && completedBarracks > 0 && commandForTeam(AI) >= config::TowerCost + config::InfantryCost) {
        const Point site = findBuildableNear(Blue_baseP, 8);
        if (site.x >= 0 && requestBuildTower(AI, site)) {
            return;
        }
    }

    if (completedBarracks > 0
        && aiUpgradeLevel < std::min(config::MaxTechLevel, completedExtractors + completedBarracks / 2)
        && commandForTeam(AI) >= upgradeCostForNextLevel(AI) + config::ShooterCost) {
        upgradeTeam(AI);
    }

    const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
    const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
    const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
    const int counterPick = playerShooters > playerInfantry
        ? UName::CAVALRY
        : (playerCavalry > playerShooters ? UName::INFANTARY : UName::SHOOTER);
    const int pressurePick = enemys.size() + 2 < myunits.size() ? UName::CAVALRY : UName::INFANTARY;
    const int priorities[] = {
        counterPick,
        pressurePick,
        UName::SHOOTER,
        UName::INFANTARY
    };

    const int orders = std::min(completedBarracks, realtime::AIUnitsPerBurst);
    for (int i = 0; i < orders; ++i) {
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

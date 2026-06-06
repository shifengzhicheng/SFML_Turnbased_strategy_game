#pragma once
#include <cstddef>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"
#include "AllUnit.h"
#include "Tile.h"
#include "Astar.h"
#include "Button.h"
#include "Map.h"
#include "Effects.h"
#include "Config.h"
#include "AIController.h"
#include "Building.h"
#include "PathfindingService.h"
#include "ResourceNode.h"
#include "Worker.h"

enum gameSeceneState {
    SCENE_START,SCENE_GAME, SCEN_GAMEOVER
};
enum gamePlayer {
    PLAYER,AI
};

class Game
{
public:
    sf::Text Globle_text;
    sf::Text UnitText;
    sf::Text UnitAttack;
    sf::Text UnitHP;
    sf::Text CommandText;
    bool gameWin;

    mapgenerator gm;

    sf::Font myfont;

    std::vector<MapPos> drawPaths;

    sf::Clock clock;
    sf::Clock clock2;
    sf::Clock realtimeFrameClock;

    Point MousePoint;

    Unit* MosOnUnit;

    bool MousePosChanged();

    int horizontalTiles;
    

    std::unique_ptr<DisMoveableUnit> Base_red;

    std::unique_ptr<DisMoveableUnit> Base_blue;

    Point Red_baseP;

    Point Blue_baseP;

    sf::Keyboard::Key keyCode;

    std::list<std::unique_ptr<MoveableUnit>> myunits;

    std::list<std::unique_ptr<MoveableUnit>> enemys;
    std::vector<Worker> workers;
    std::vector<Building> buildings;

    Effects effects;
    std::vector<ResourceNode> resources;
    int playerCommand = config::StartingCommand;
    int aiCommand = config::StartingCommand;
    bool aiProductionDone = false;
    bool realtimeMode = true;
    bool tutorialVisible = false;
    bool towerPlacementMode = false;
    bool debugLogging = false;
    float playerIncomeTimer = 0.f;
    float aiIncomeTimer = 0.f;
    float gameTimeSeconds = 0.f;
    float debugSummaryTimer = 0.f;
    AIController aiController;
    PathfindingService pathfinding;
    int nextEntityId = 1;
    int nextPathRequestId = 1;
    int pathGeneration = 1;
    int playerUpgradeLevel = 0;
    int aiUpgradeLevel = 0;

    std::size_t turn;

    bool playerturn;


    sf::Texture tStartBtnNormal, tStartBtnHover, tStartBtnClick;
    sf::Texture tEndBtnNormal, tEndBtnHover, tEndBtnClick;
    sf::Texture tOverBtnNormal, tOverBtnHover, tOverBtnClick;
    Button startBtn,EndTurnBtn;
    Button inf, cav, sho;
    Button upgradeBtn;
    Button helpBtn;
    Button towerBtn;
    Button endGame;
    sf::Texture tinf, tinfHover, tinfClick;
    sf::Texture tcav, tcavHover, tcavClick;
    sf::Texture tsho, tshoHover, tshoClick;
    sf::Texture tUpgrade, tUpgradeHover, tUpgradeClick;
    sf::Texture tHelp, tHelpHover, tHelpClick;
    sf::Texture tTower, tTowerHover, tTowerClick;
    sf::Texture background;
    sf::Sprite back;
    sf::RectangleShape sidePanel;
    sf::Text panelTitle;
    sf::Text panelHint;
    sf::Text infantryLabel;
    sf::Text shooterLabel;
    sf::Text cavalryLabel;
    sf::Text towerLabel;

    Game();
    ~Game();
    

    sf::RenderWindow window;

    bool running;

    void loadpic();

    std::vector<std::vector<int> > maze;
    std::vector<MapPos> tiles;

    Astar astar;

    int gameSceneState;
    bool gameOver;

    void Initial();

    void loadMediaData();

    void AIlogic();
    
    void logicBeforeDraw();
    void logicAfterDraw();
    void logicBeforeInput();
    void updateRealtime(float dt);
    void updateRealtimeEconomy(float dt);
    void updateDebugSummary(float dt);

    void run();
    void Unitsreset(std::list<std::unique_ptr<MoveableUnit>>& us);
    void AIUnitreset();
    void startInput(sf::Vector2i mousePosition, sf::Event event);

    void GameInput(sf::Vector2i mousePos, sf::Event event);

    void Draw();
    void DrawSidePanel();
    void drawGridOverlay();
    void drawResourceNodes();
    void drawBuildings();
    void drawWorkers();
    void drawTutorialOverlay();
    void addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color);
    void addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size = 15);
    void startScreenShake(float durationSeconds, float intensity);
    sf::Vector2f currentShakeOffset() const;
    void handleBuildButtons(sf::Vector2i mousePos, sf::Event event);
    void clearSelection();
    void selectOnly(Unit* unit);
    void syncMazeFromTiles();
    void setTileID(int x, int y, tile::ID id);
    bool isBlockingTile(tile::ID id) const;
    bool isMapCell(int x, int y) const;
    bool isRealtimeMode() const;
    bool isCellWalkableForUnit(int x, int y) const;
    bool isCellReservedForSpawn(int x, int y) const;
    bool isBuildableCell(int x, int y) const;
    
    void clear();

    void setBase();
    void placeResourceNodes();
    void updateResourceControl();
    void updateWorkers(float dt);
    void updateProduction(float dt);
    void updateDefenseTowers(float dt);
    void handleRealtimeMapClick(sf::Vector2i mousePos, sf::Event event);
    int resourceIncome(int team) const;
    int controlledResourceCount(int team) const;
    int completedBuildingCount(int team, int type) const;
    int pendingOrCompleteExtractorForResource(int resourceIndex) const;
    int unitCost(int name) const;
    int buildingCost(int type) const;
    int totalBuildingCount(int team, int type) const;
    int buildingCap(int type) const;
    int upgradeCostForNextLevel(int team) const;
    int commandForTeam(int team) const;
    float damageMultiplier(int team) const;
    bool hasUnitCapacity(int team) const;
    bool hasSpawnTile(int team) const;
    bool isUnitUnlocked(int team, int name) const;
    bool canQueueUnit(int team, int name) const;
    bool enqueueUnit(int team, int name);
    bool upgradeTeam(int team);
    bool requestBuildExtractor(int team, int resourceIndex);
    bool requestBuildBarracks(int team, Point point);
    bool requestBuildTower(int team, Point point);
    void logEvent(const std::string& message) const;
    void logDebugSummary() const;
    void assignWorkers();
    int workerCount(int team) const;
    int assignedWorkerCount(int buildingId) const;
    bool hasActiveHarvester(const Building& building) const;
    Worker* findAvailableWorker(int team, bool allowHarvesting);
    Worker* findIdleWorker(int team);
    Worker* findWorkerById(int id);
    Building* findBuildingById(int id);
    const Building* findBuildingById(int id) const;
    void assignWorkerToBuilding(Worker& worker, Building& building);
    void assignWorkerToHarvest(Worker& worker, Building& building);
    void updateWorkerTravel(Worker& worker, Point target, float dt);
    Point workerSpawnPoint(int team) const;
    Point findBuildStandPoint(const Building& building) const;
    Point findBuildableNear(Point anchor, int radius) const;
    Point findSpawnPointAround(Point anchor) const;
    void createStartingWorkers();
    void createWorker(int team, Point point);
    bool createUnit(int team, int name, int x, int y);
    std::string spawnBlockReason(int team, int name) const;
    bool canSpawnUnit(int team, int name) const;
    bool spendCommand(int team, int name);
    void addTurnIncome(int team);
    void runAIProduction();
    MoveableUnit* findMoveableUnitById(int id);
    void requestPathForUnit(MoveableUnit& unit, Point goal);
    void requestPathForWorker(Worker& worker, Point goal);
    void applyPathResults();

    int indexAt(sf::Vector2f position);

    void Input();

    void overinput(sf::Vector2i mousePos, sf::Event event);
    
    bool spawnUnit(int team,int name, int x, int y);
    
};

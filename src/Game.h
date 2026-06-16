#pragma once
#include <array>
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
#include "RealtimeConfig.h"
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

namespace perk
{
    enum Type
    {
        Drill,
        Fortitude,
        Volley,
        Charge,
        SiegeCraft,
        TowerCraft,
        Logistics,
        Mining,
        WarChest,
        Count
    };
}

namespace gameop
{
    enum Type
    {
        SelectLane,
        UpgradeEconomy,
        UpgradeTech,
        BuildBarracks,
        BuildTower,
        QueueUnit,
        Count
    };
}

struct GameOperation
{
    int type = gameop::SelectLane;
    int laneIndex = lane::Mid;
    int unitName = UName::INFANTARY;

    GameOperation(int operationType = gameop::SelectLane, int lane = lane::Mid, int unit = UName::INFANTARY)
        : type(operationType), laneIndex(lane), unitName(unit)
    {
    }
};

struct PerkChoice
{
    int type = perk::Drill;
    std::string title;
    std::string description;
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
    bool externalAIControl = false;
    float playerIncomeTimer = 0.f;
    float aiIncomeTimer = 0.f;
    float playerBaseAttackTimer = 0.f;
    float aiBaseAttackTimer = 0.f;
    float playerBaseShieldTimer = 0.f;
    float aiBaseShieldTimer = 0.f;
    float playerEmergencyTrainTimer = 0.f;
    float aiEmergencyTrainTimer = 0.f;
    float gameTimeSeconds = 0.f;
    float debugSummaryTimer = 0.f;
    AIController aiController;
    PathfindingService pathfinding;
    int nextEntityId = 1;
    int nextPathRequestId = 1;
    int pathGeneration = 1;
    int playerUpgradeLevel = 0;
    int aiUpgradeLevel = 0;
    int playerEconomyLevel = 0;
    int aiEconomyLevel = 0;
    bool perkOverlayVisible = false;
    bool autoChooseRewards = false;
    int rewardSequence = 0;
    int playerSelectedLane = lane::Mid;
    int aiSelectedLane = lane::Mid;
    std::array<int, perk::Count> playerPerkLevels{};
    std::array<int, perk::Count> aiPerkLevels{};
    std::array<PerkChoice, 3> perkChoices{};

    std::size_t turn;

    bool playerturn;


    sf::Texture tStartBtnNormal, tStartBtnHover, tStartBtnClick;
    sf::Texture tStartHelpNormal, tStartHelpHover, tStartHelpClick;
    sf::Texture tEndBtnNormal, tEndBtnHover, tEndBtnClick;
    sf::Texture tOverBtnNormal, tOverBtnHover, tOverBtnClick;
    Button startBtn, startHelpBtn, EndTurnBtn;
    Button inf, cav, sho, siegeBtn, guardianBtn;
    Button upgradeBtn;
    Button economyBtn;
    Button barracksBtn;
    Button helpBtn;
    Button towerBtn;
    Button endGame;
    sf::Texture tinf, tinfHover, tinfClick;
    sf::Texture tcav, tcavHover, tcavClick;
    sf::Texture tsho, tshoHover, tshoClick;
    sf::Texture tSiege, tSiegeHover, tSiegeClick;
    sf::Texture tGuardian, tGuardianHover, tGuardianClick;
    sf::Texture tUpgrade, tUpgradeHover, tUpgradeClick;
    sf::Texture tEconomy, tEconomyHover, tEconomyClick;
    sf::Texture tBarracks, tBarracksHover, tBarracksClick;
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
    sf::Text siegeLabel;
    sf::Text guardianLabel;
    sf::Text economyLabel;
    sf::Text barracksLabel;
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
    void updateTimedRewards();

    void run();
    void Unitsreset(std::list<std::unique_ptr<MoveableUnit>>& us);
    void AIUnitreset();
    void startInput(sf::Vector2i mousePosition, sf::Event event);

    void GameInput(sf::Vector2i mousePos, sf::Event event);

    void Draw();
    void DrawSidePanel();
    void drawLaneGuides();
    void drawGridOverlay();
    void drawResourceNodes();
    void drawBuildings();
    void drawWorkers();
    void drawTutorialOverlay();
    void drawRewardOverlay();
    void addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color);
    void addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size = 15);
    void startScreenShake(float durationSeconds, float intensity);
    sf::Vector2f currentShakeOffset() const;
    void handleBuildButtons(sf::Vector2i mousePos, sf::Event event);
    void handleRewardInput(sf::Vector2i mousePos, sf::Event event);
    void handleLaneInput(sf::Vector2i mousePos, sf::Event event);
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
    bool isBuildSiteInInfluence(int team, Point point, int type) const;
    bool isCellOccupiedByUnit(int x, int y, int ignoredEntityId = 0) const;
    bool canUnitStepInto(const MoveableUnit& unit, Point point) const;
    
    void clear();

    void setBase();
    void placeResourceNodes();
    void updateWorkers(float dt);
    void updateProduction(float dt);
    void updateDefenseTowers(float dt);
    void updateBaseDefenses(float dt);
    void updateComebackTimers(float dt);
    void updateEmergencyBaseTraining(float dt);
    void cleanupDestroyedBuildings();
    void handleRealtimeMapClick(sf::Vector2i mousePos, sf::Event event);
    int resourceIncome(int team) const;
    int economyLevelForTeam(int team) const;
    int economyUpgradeCost(int team) const;
    int completedBuildingCount(int team, int type) const;
    int unitCost(int name) const;
    int totalBuildingCount(int team, int type) const;
    int buildingCap(int team, int type) const;
    int upgradeCostForNextLevel(int team) const;
    int unitsNearPoint(int team, Point point, int radius) const;
    int commandForTeam(int team) const;
    float damageMultiplier(int team) const;
    float unitDamageMultiplier(int team, int unitName) const;
    float unitHealthMultiplier(int team, int unitName) const;
    float unitAttackCooldownMultiplier(int team, int unitName) const;
    float baseDamageTakenMultiplier(int attackerUnitName, int defenderTeam) const;
    float baseShieldSecondsForTeam(int team) const;
    float teamTrainTimeMultiplier(int team) const;
    float miningIncomeMultiplier(int team) const;
    int defenseTowerRange(int team) const;
    int selectedLaneForTeam(int team) const;
    Point laneWaypoint(int team, int laneIndex, int stage) const;
    Point laneDefensePoint(int team, int laneIndex) const;
    bool hasUnitCapacity(int team) const;
    bool hasSpawnTile(int team) const;
    bool isUnitUnlocked(int team, int name) const;
    bool canQueueUnit(int team, int name) const;
    bool enqueueUnit(int team, int name);
    bool executeOperation(int team, const GameOperation& operation);
    std::size_t executeOperations(int team, const std::vector<GameOperation>& operations);
    std::string describeOperation(const GameOperation& operation) const;
    bool upgradeTeam(int team);
    bool upgradeEconomy(int team);
    void syncWorkersForEconomy(int team);
    void awardKillBounty(int receiverTeam, int defeatedUnitName, Point point);
    bool requestBuildBarracks(int team, Point point);
    bool requestBuildTower(int team, Point point);
    bool requestAutoBuildBarracks(int team);
    bool requestAutoBuildTower(int team);
    Point findAutoBuildSite(int team, int type, int laneIndex) const;
    Building* chooseBuildingTarget(MoveableUnit& unit);
    Point chooseStrategicRallyPoint(const MoveableUnit& unit) const;
    bool canAttackBuilding(const MoveableUnit& unit, const Building& building) const;
    void autoAttackBuilding(MoveableUnit& unit, Building& building);
    void applyStructureLossRelief(const Building& destroyed);
    void resetWorkersForBuilding(int buildingId);
    void maybeGrantReward(int team, const std::string& reason);
    void buildRewardChoices();
    void applyRewardChoice(int index);
    void applyPerk(int team, int type);
    int perkLevel(int team, int type) const;
    void logEvent(const std::string& message) const;
    void logDebugSummary() const;
    void assignWorkers();
    int workerCount(int team) const;
    int assignedWorkerCount(int buildingId) const;
    Worker* findAvailableWorker(int team);
    Worker* findIdleWorker(int team);
    Worker* findWorkerById(int id);
    Building* findBuildingById(int id);
    const Building* findBuildingById(int id) const;
    void assignWorkerToBuilding(Worker& worker, Building& building);
    void updateWorkerTravel(Worker& worker, Point target, float dt);
    Point workerSpawnPoint(int team) const;
    Point findBuildStandPoint(const Building& building) const;
    Point findAttackStandPoint(const MoveableUnit& unit, const Building& building) const;
    Point findBuildableNear(Point anchor, int radius) const;
    Point findSpawnPointAround(Point anchor) const;
    void createStartingWorkers();
    void createWorker(int team, Point point);
    bool createUnit(int team, int name, int x, int y, int laneIndex = lane::Mid);
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

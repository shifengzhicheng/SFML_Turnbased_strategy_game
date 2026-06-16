#include "Game.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) {
            std::cerr << "game_operation_tests: " << message << '\n';
            std::exit(1);
        }
    }
}

int main()
{
    Game game;
    game.window.setVisible(false);
    game.debugLogging = false;
    game.autoChooseRewards = true;
    game.externalAIControl = true;
    game.gameSceneState = SCENE_GAME;
    game.clear();

    require(!game.canQueueUnit(PLAYER, UName::INFANTARY),
            "infantry should require a completed barracks");

    game.playerCommand = 1000;
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, lane::Top)),
            "player should auto-place first barracks");
    require(game.playerSelectedLane == lane::Top,
            "build operation should update selected lane");
    require(game.totalBuildingCount(PLAYER, building::Barracks) == 1,
            "one barracks should be queued");
    require(game.playerCommand == 1000 - config::BarracksCost,
            "barracks should spend CMD exactly once");

    Building* barracks = game.findBuildingById(game.buildings.front().id);
    require(barracks != nullptr, "queued barracks should be findable by id");
    barracks->complete = true;

    game.playerCommand = 1000;
    require(game.canQueueUnit(PLAYER, UName::INFANTARY),
            "completed barracks should unlock infantry queueing");
    require(game.executeOperation(PLAYER, GameOperation(gameop::QueueUnit, lane::Bot, UName::INFANTARY)),
            "infantry queue operation should succeed");
    require(game.playerSelectedLane == lane::Bot,
            "queue operation should update selected lane");
    require(barracks->production.load() == 1,
            "unit order should land in barracks production queue");
    require(!barracks->production.orders.empty() && barracks->production.orders.front().lane == lane::Bot,
            "queued unit should keep requested lane");
    require(game.playerCommand == 1000 - config::InfantryCost,
            "unit queue should spend unit cost");
    require(!game.canQueueUnit(PLAYER, UName::SIEGE),
            "siege should stay locked before tech requirements are met");

    game.playerCommand = 1000;
    const int techCost = game.upgradeCostForNextLevel(PLAYER);
    require(game.executeOperation(PLAYER, GameOperation(gameop::UpgradeTech)),
            "tech upgrade should succeed with enough CMD");
    require(game.playerUpgradeLevel == 1,
            "tech upgrade should increase player level");
    require(game.playerCommand >= 1000 - techCost,
            "auto reward may refund CMD, but tech cost must be applied");
    require(!game.perkOverlayVisible,
            "auto reward mode should not leave the reward overlay open");

    game.clear();
    game.playerCommand = 1000;
    const int economyCost = game.economyUpgradeCost(PLAYER);
    const int workersBefore = game.workerCount(PLAYER);
    require(game.executeOperation(PLAYER, GameOperation(gameop::UpgradeEconomy)),
            "economy upgrade should succeed with enough CMD");
    require(game.playerEconomyLevel == 1,
            "economy upgrade should increase player economy level");
    require(game.playerCommand == 1000 - economyCost,
            "economy upgrade should spend its current cost");
    require(game.workerCount(PLAYER) > workersBefore,
            "economy upgrade should materialize additional drones");

    game.clear();
    game.playerCommand = 1000;
    require(!game.createUnit(PLAYER, UName::INFANTARY, game.Red_baseP.x, game.Red_baseP.y, lane::Mid),
            "direct unit creation should reject the blocked base tile");
    require(game.myunits.empty(),
            "rejected direct unit creation should not mutate the player roster");
    require(!game.spawnUnit(PLAYER, UName::INFANTARY, game.Red_baseP.x, game.Red_baseP.y),
            "paid unit spawning should reject the blocked base tile");
    require(game.playerCommand == 1000,
            "rejected paid unit spawning should not spend CMD");

    const Point validSpawn = game.findSpawnPointAround(game.Red_baseP);
    require(validSpawn.x >= 0,
            "base should have at least one valid spawn tile after map setup");
    require(game.createUnit(PLAYER, UName::INFANTARY, validSpawn.x, validSpawn.y, lane::Top),
            "direct unit creation should accept a free walkable spawn tile");
    require(game.myunits.size() == 1,
            "successful direct unit creation should add exactly one player unit");
    require(!game.createUnit(PLAYER, UName::INFANTARY, validSpawn.x, validSpawn.y, lane::Top),
            "direct unit creation should reject an already reserved unit tile");
    require(game.myunits.size() == 1,
            "rejected duplicate direct creation should not add another player unit");

    return 0;
}

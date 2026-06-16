#include "Game.h"
#include "AutoCombat.h"
#include "BuildingDefinition.h"

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
    require(game.playerCommand == 1000 - buildingDefinition(building::Barracks).commandCost,
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
    const int baseIncome = game.resourceIncome(PLAYER);
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
    require(game.resourceIncome(PLAYER) - baseIncome >= 3,
            "first economy upgrade should noticeably increase per-tick income");
    game.playerEconomyLevel = 4;
    require(game.resourceIncome(PLAYER) >= 19,
            "mid economy income should scale without running away");
    game.playerEconomyLevel = 8;
    require(game.resourceIncome(PLAYER) >= 39,
            "late economy income should still feel like a production engine");
    game.playerEconomyLevel = 11;
    require(game.economyUpgradeCost(PLAYER) <= 410,
            "late economy cost should stay payable but slow runaway tempo");

    game.clear();
    game.playerEconomyLevel = 8;
    const int incomeBeforeMining = game.resourceIncome(PLAYER);
    game.applyPerk(PLAYER, perk::Mining);
    require(game.resourceIncome(PLAYER) >= incomeBeforeMining + 4,
            "Mining perk should make economy upgrades visibly stronger");
    const float shooterDamageBefore = game.unitDamageMultiplier(PLAYER, UName::SHOOTER);
    const float shooterCooldownBefore = game.unitAttackCooldownMultiplier(PLAYER, UName::SHOOTER);
    game.applyPerk(PLAYER, perk::Volley);
    require(game.unitDamageMultiplier(PLAYER, UName::SHOOTER) >= shooterDamageBefore + 0.12f,
            "Volley should be a chunky shooter damage upgrade");
    require(game.unitAttackCooldownMultiplier(PLAYER, UName::SHOOTER) <= shooterCooldownBefore - 0.05f,
            "Volley should noticeably speed up shooter attacks");
    const float trainBefore = game.teamTrainTimeMultiplier(PLAYER);
    game.applyPerk(PLAYER, perk::Logistics);
    require(game.teamTrainTimeMultiplier(PLAYER) <= trainBefore - 0.08f,
            "Logistics should convert upgrades into production tempo");
    game.playerCommand = 0;
    game.playerUpgradeLevel = 5;
    game.applyPerk(PLAYER, perk::WarChest);
    require(game.playerCommand >= config::WarChestBaseBonus + config::WarChestTechBonus * 5,
            "War Chest should be large enough to create an immediate play");

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

    game.clear();
    const Point rightEdgeAttackCell(game.Blue_baseP.x + config::BaseFootprintSize - 1 + config::InfantryRange,
                                    game.Blue_baseP.y + 1);
    require(game.createUnit(PLAYER, UName::INFANTARY, rightEdgeAttackCell.x, rightEdgeAttackCell.y, lane::Mid),
            "test infantry should fit beside the blue base right edge");
    MoveableUnit* edgeAttacker = game.myunits.back().get();
    require(edgeAttacker->canAutoAttack(game.Base_blue.get()),
            "base range checks should use the whole 2x2 footprint, not only the top-left tile");

    game.clear();
    for (int x = 10; x <= 14; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SHOOTER, 10, 10, lane::Mid),
            "test shooter should spawn for line-of-sight checks");
    require(game.createUnit(AI, UName::INFANTARY, 14, 10, lane::Mid),
            "test infantry target should spawn for line-of-sight checks");
    game.setTileID(12, 10, tile::Tree);
    require(!game.myunits.back()->canAutoAttack(game.enemys.back().get()),
            "tree obstacles between units should block line-of-sight attacks");
    game.setTileID(12, 10, tile::Empty);
    require(game.myunits.back()->canAutoAttack(game.enemys.back().get()),
            "clearing the obstacle should restore line-of-sight attacks");

    game.clear();
    for (int x = 10; x <= 14; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, 11, 10, lane::Mid),
            "test decoy should spawn for aggro checks");
    MoveableUnit* decoyTarget = game.myunits.back().get();
    require(game.createUnit(AI, UName::INFANTARY, 12, 10, lane::Mid),
            "test defender should spawn for aggro checks");
    MoveableUnit* defender = game.enemys.back().get();
    require(game.createUnit(PLAYER, UName::SHOOTER, 14, 10, lane::Mid),
            "test attacker should spawn for aggro checks");
    MoveableUnit* provokingShooter = game.myunits.back().get();
    provokingShooter->autoAttack(defender);
    require(defender->aggroTargetId == provokingShooter->entityId && defender->aggroSeconds > 0.f,
            "damaged units should remember the unit that attacked them");
    const int shooterHealthBeforeAggro = provokingShooter->Health;
    const int decoyHealthBeforeAggro = decoyTarget->Health;
    defender->realtimeAttackTimer = defender->realtimeAttackCooldownSeconds();
    realtime::updateAutoCombat(game, 0.25f);
    require(provokingShooter->Health < shooterHealthBeforeAggro,
            "aggro should make the defender retaliate against its attacker");
    require(decoyTarget->Health == decoyHealthBeforeAggro,
            "aggro should outrank an even closer unrelated target");

    game.clear();
    const Point botEnemyRally = game.laneWaypoint(PLAYER, lane::Bot, 2);
    for (int x = botEnemyRally.x - 6; x <= botEnemyRally.x + 2; ++x) {
        game.setTileID(x, botEnemyRally.y, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, botEnemyRally.x - 2, botEnemyRally.y, lane::Bot),
            "bot-lane test unit should spawn near the enemy-side rally");
    MoveableUnit* botLaneUnit = game.myunits.back().get();
    botLaneUnit->nextRallyStage = 2;
    Point rallyAfterArrival = game.chooseStrategicRallyPoint(*botLaneUnit);
    require(rallyAfterArrival.x < 0 && botLaneUnit->nextRallyStage > 2,
            "reaching the bot enemy-side rally should commit the unit to assault");
    botLaneUnit->x = botEnemyRally.x - 5;
    botLaneUnit->y = botEnemyRally.y;
    Point rallyAfterDetour = game.chooseStrategicRallyPoint(*botLaneUnit);
    require(rallyAfterDetour.x < 0,
            "committed bot-lane units should not re-request old rally points after a detour");

    game.clear();
    for (int y = 9; y <= 11; ++y) {
        for (int x = 9; x <= 12; ++x) {
            game.setTileID(x, y, tile::Empty);
        }
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, 10, 10, lane::Mid),
            "moving test unit should spawn");
    require(game.createUnit(PLAYER, UName::INFANTARY, 11, 10, lane::Mid),
            "blocking test unit should spawn");
    MoveableUnit* crowdedMover = game.myunits.front().get();
    crowdedMover->mypath.clear();
    crowdedMover->mypath.push_back(Point(11, 10));
    realtime::updateAutoCombat(game, 1.0f);
    require(crowdedMover->x == 11 && crowdedMover->y == 10,
            "combat movement should ignore other unit bodies and allow stacking");
    require(game.myunits.back()->x == 11 && game.myunits.back()->y == 10,
            "the blocking unit should remain stacked on the same cell");

    game.gameOver = true;
    game.clear();
    require(!game.gameOver,
            "clear should reset gameOver so simulations and rematches can start");

    game.clear();
    const int aiCommandBeforeBounty = game.aiCommand;
    const Point cleanupSpawn = game.findSpawnPointAround(game.Red_baseP);
    require(game.createUnit(PLAYER, UName::INFANTARY, cleanupSpawn.x, cleanupSpawn.y, lane::Mid),
            "test unit should spawn for realtime cleanup");
    game.myunits.back()->Health = 0;
    game.updateRealtime(0.25f);
    require(game.myunits.empty(),
            "realtime update should remove destroyed player units without relying on rendering");
    require(game.aiCommand > aiCommandBeforeBounty,
            "realtime destroyed-unit cleanup should award kill bounty");

    return 0;
}

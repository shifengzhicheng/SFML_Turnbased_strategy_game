#include "Game.h"
#include "AutoCombat.h"
#include "BuildingDefinition.h"
#include "LaneGeometry.h"
#include "RealtimeConfig.h"
#include "SidebarLayout.h"
#include "UnitDefinition.h"

#include <cmath>
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

    void clearArea(Game& game, Point center, int radius)
    {
        for (int y = center.y - radius; y <= center.y + radius; ++y) {
            for (int x = center.x - radius; x <= center.x + radius; ++x) {
                if (game.isMapCell(x, y)) {
                    game.setTileID(x, y, tile::Empty);
                }
            }
        }
    }

    int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
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

    sf::Event lanePress{};
    lanePress.type = sf::Event::MouseButtonPressed;
    lanePress.mouseButton.button = sf::Mouse::Left;
    for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
        const sf::FloatRect rect = sidebar_layout::laneButtonRect(laneIndex);
        const sf::Vector2i edgeClick(static_cast<int>(rect.left + rect.width - 1.f),
                                     static_cast<int>(rect.top + rect.height - 1.f));
        lanePress.mouseButton.x = edgeClick.x;
        lanePress.mouseButton.y = edgeClick.y;
        require(game.handleLaneInput(edgeClick, lanePress),
                "lane button edge clicks should land inside the shared hitbox");
        require(game.playerSelectedLane == laneIndex,
                "lane button clicks should select the matching lane");
    }
    const sf::FloatRect topRect = sidebar_layout::laneButtonRect(lane::Top);
    const sf::Vector2i gapClick(static_cast<int>(topRect.left + topRect.width + sidebar_layout::LaneButtonGap * 0.5f),
                                static_cast<int>(topRect.top + topRect.height * 0.5f));
    lanePress.mouseButton.x = gapClick.x;
    lanePress.mouseButton.y = gapClick.y;
    require(game.handleLaneInput(gapClick, lanePress),
            "lane strip gap clicks should still be accepted");
    require(game.playerSelectedLane == lane::Mid,
            "clicking the first lane gap should choose the adjacent middle segment");
    const sf::FloatRect laneStrip = sidebar_layout::laneHitStripRect();
    require(laneStrip.top + laneStrip.height <= static_cast<float>(config::EconomyButtonY),
            "lane hitboxes must not overlap the economy action button");

    const int mapW = config::MapTilesX;
    for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
        for (int stage = 0; stage < lane_geometry::RallyStageCount; ++stage) {
            const Point playerRally = game.laneRallyPoint(PLAYER, laneIndex, stage);
            const Point aiRally = game.laneRallyPoint(AI, laneIndex, stage);
            require(playerRally.x + aiRally.x == mapW - 1 && playerRally.y == aiRally.y,
                    "unit rally anchors should be mirrored for both teams");
        }
        for (int stage = 0; stage <= 2; ++stage) {
            const Point playerAnchor = game.laneWaypoint(PLAYER, laneIndex, stage);
            const Point aiAnchor = game.laneWaypoint(AI, laneIndex, stage);
            require(playerAnchor.x + aiAnchor.x == mapW - 1 && playerAnchor.y == aiAnchor.y,
                    "lane objective anchors should be mirrored for both teams");
        }

        const Point playerAnchor = game.laneRallyPoint(PLAYER, laneIndex, 0);
        const Point aiAnchor = game.laneRallyPoint(AI, laneIndex, 0);
        clearArea(game, playerAnchor, 1);
        clearArea(game, aiAnchor, 1);
        const Point playerBarracks = game.findAutoBuildSite(PLAYER, building::Barracks, laneIndex);
        const Point aiBarracks = game.findAutoBuildSite(AI, building::Barracks, laneIndex);
        require(playerBarracks.x >= 0 && aiBarracks.x >= 0,
                "both teams should find an automatic barracks site on each lane");
        require(playerBarracks.x + aiBarracks.x == mapW - 1 && playerBarracks.y == aiBarracks.y,
                "automatic barracks placement should mirror across the map center");
        require(distanceSquared(playerBarracks, game.Red_baseP) <= 36
                    && distanceSquared(aiBarracks, game.Blue_baseP) <= 36,
                "automatic barracks should stay in the protected base pocket");
    }
    const int baseTowerCap = game.buildingCap(PLAYER, building::DefenseTower);
    require(baseTowerCap >= 2,
            "players should have room for multiple early defensive towers");
    game.playerUpgradeLevel = 8;
    game.playerEconomyLevel = 6;
    game.playerPerkLevels[static_cast<std::size_t>(perk::TowerCraft)] = 4;
    require(game.buildingCap(PLAYER, building::DefenseTower) > baseTowerCap
                && game.buildingCap(PLAYER, building::DefenseTower) <= config::TowerCap,
            "tower cap should scale up through tech, economy, and tower perks");
    game.playerUpgradeLevel = 0;
    game.playerEconomyLevel = 0;
    game.playerPerkLevels.fill(0);

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
    require(game.resourceIncome(PLAYER) >= incomeBeforeMining + 6,
            "Mining perk should make economy upgrades visibly stronger");
    game.applyPerk(PLAYER, perk::Volley);
    require(game.additionalAttackTargets(PLAYER, UName::SHOOTER) == 1,
            "Volley should change shooter mechanics by adding a secondary target");
    require(game.unitAttackRange(PLAYER, UName::SHOOTER) == config::ShooterRange,
            "Volley level 1 should not silently inflate shooter range");
    game.applyPerk(PLAYER, perk::Volley);
    require(game.unitAttackRange(PLAYER, UName::SHOOTER) == config::ShooterRange + 1,
            "Volley level 2 should add shooter range");
    game.applyPerk(PLAYER, perk::Volley);
    game.applyPerk(PLAYER, perk::Volley);
    game.applyPerk(PLAYER, perk::Volley);
    require(game.unitAttackRange(PLAYER, UName::SHOOTER) == config::ShooterMaxRange,
            "Shooter range perks should respect the cap");
    require(game.additionalAttackTargets(PLAYER, UName::SHOOTER) == 2,
            "high Volley should upgrade into a second splash target");
    const float trainBefore = game.teamTrainTimeMultiplier(PLAYER);
    game.applyPerk(PLAYER, perk::Logistics);
    require(game.teamTrainTimeMultiplier(PLAYER) <= trainBefore - 0.11f,
            "Logistics should convert upgrades into production tempo");
    game.playerCommand = 0;
    game.playerUpgradeLevel = 5;
    game.applyPerk(PLAYER, perk::WarChest);
    require(game.playerCommand >= config::WarChestBaseBonus + config::WarChestTechBonus * 5,
            "War Chest should be large enough to create an immediate play");

    game.perkOverlayVisible = true;
    game.playerRewardRerolls = 1;
    game.buildRewardChoices();
    const int firstRewardType = game.perkChoices.front().type;
    game.rerollRewardChoices();
    require(game.playerRewardRerolls == 0,
            "reward refresh should consume the free reroll");
    require(game.perkChoices.front().type != firstRewardType,
            "reward refresh should produce a different tactic rotation");

    game.clear();
    game.playerCommand = 1000;
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, lane::Mid)),
            "mastery test should build a barracks to unlock infantry");
    game.buildings.back().complete = true;
    const int infantryMasteryCost = game.unitMasteryUpgradeCost(PLAYER, UName::INFANTARY);
    const float infantryDamageBeforeMastery = game.unitDamageMultiplier(PLAYER, UName::INFANTARY);
    const Point masterySpawn = game.findSpawnPointAround(game.Red_baseP);
    require(game.createUnit(PLAYER, UName::INFANTARY, masterySpawn.x, masterySpawn.y, lane::Mid),
            "mastery test infantry should spawn");
    MoveableUnit* masteryInfantry = game.myunits.back().get();
    const int masteryHealthBefore = masteryInfantry->Health;
    require(game.executeOperation(PLAYER, GameOperation(gameop::UpgradeUnitMastery, lane::Mid, UName::INFANTARY)),
            "unit mastery upgrade should succeed with enough CMD and unlocks");
    require(game.unitMasteryLevel(PLAYER, UName::INFANTARY) == 1,
            "unit mastery should advance independently per unit type");
    require(game.playerCommand == 1000 - buildingDefinition(building::Barracks).commandCost - infantryMasteryCost,
            "unit mastery should spend its displayed cost");
    require(game.unitDamageMultiplier(PLAYER, UName::INFANTARY) >= infantryDamageBeforeMastery + config::MasteryStatBonusPerLevel - 0.001f,
            "unit mastery should add 10 percent baseline damage scaling");
    require(masteryInfantry->Health > masteryHealthBefore,
            "existing units should receive mastery health scaling");

    game.clear();
    for (int x = 10; x <= 14; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    game.applyPerk(PLAYER, perk::Volley);
    require(game.createUnit(PLAYER, UName::SHOOTER, 10, 10, lane::Mid),
            "volley shooter should spawn");
    require(game.createUnit(AI, UName::INFANTARY, 13, 10, lane::Mid),
            "primary volley target should spawn");
    require(game.createUnit(AI, UName::INFANTARY, 12, 10, lane::Mid),
            "secondary volley target should spawn");
    MoveableUnit* volleyShooter = game.myunits.back().get();
    MoveableUnit* primaryVolleyTarget = game.enemys.front().get();
    MoveableUnit* secondaryVolleyTarget = game.enemys.back().get();
    const int secondaryBefore = secondaryVolleyTarget->Health;
    volleyShooter->autoAttack(primaryVolleyTarget);
    require(secondaryVolleyTarget->Health < secondaryBefore,
            "Volley mechanic should damage an additional target in range");

    game.clear();
    for (int x = 10; x <= 15; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SIEGE, 10, 10, lane::Mid),
            "baseline siege should spawn for anti-swarm checks");
    require(game.createUnit(AI, UName::INFANTARY, 14, 10, lane::Mid),
            "primary siege splash target should spawn");
    require(game.createUnit(AI, UName::INFANTARY, 13, 10, lane::Mid),
            "secondary siege splash target should spawn");
    MoveableUnit* baselineSiege = game.myunits.back().get();
    MoveableUnit* primarySiegeTarget = game.enemys.front().get();
    MoveableUnit* secondarySiegeTarget = game.enemys.back().get();
    const int secondaryBeforeSiege = secondarySiegeTarget->Health;
    baselineSiege->autoAttack(primarySiegeTarget);
    require(secondarySiegeTarget->Health < secondaryBeforeSiege,
            "baseline siege should splash nearby units so infantry swarms have a counter");

    game.clear();
    for (int y = 10; y <= 11; ++y) {
        for (int x = 10; x <= 11; ++x) {
            game.setTileID(x, y, tile::Empty);
        }
    }
    require(game.createUnit(PLAYER, UName::GUARDIAN, 10, 10, lane::Mid),
            "baseline guardian should spawn for cleave checks");
    require(game.createUnit(AI, UName::INFANTARY, 11, 10, lane::Mid),
            "primary guardian cleave target should spawn");
    require(game.createUnit(AI, UName::INFANTARY, 10, 11, lane::Mid),
            "secondary guardian cleave target should spawn");
    MoveableUnit* baselineGuardian = game.myunits.back().get();
    MoveableUnit* primaryGuardianTarget = game.enemys.front().get();
    MoveableUnit* secondaryGuardianTarget = game.enemys.back().get();
    const int secondaryBeforeGuardian = secondaryGuardianTarget->Health;
    baselineGuardian->autoAttack(primaryGuardianTarget);
    require(secondaryGuardianTarget->Health < secondaryBeforeGuardian,
            "baseline guardian should cleave adjacent units to anchor against swarms");

    game.clear();
    for (int x = 10; x <= 12; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(AI, UName::INFANTARY, 10, 10, lane::Mid),
            "counter test infantry should spawn");
    require(game.createUnit(PLAYER, UName::CAVALRY, 11, 10, lane::Mid),
            "counter test cavalry should spawn");
    MoveableUnit* counterInfantry = game.enemys.back().get();
    MoveableUnit* counterCavalry = game.myunits.back().get();
    const int cavalryBeforeCounter = counterCavalry->Health;
    counterInfantry->autoAttack(counterCavalry);
    const int counteredDamage = cavalryBeforeCounter - counterCavalry->Health;

    game.clear();
    for (int x = 10; x <= 12; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    game.applyPerk(PLAYER, perk::Charge);
    game.applyPerk(PLAYER, perk::Charge);
    require(game.createUnit(AI, UName::INFANTARY, 10, 10, lane::Mid),
            "immune counter test infantry should spawn");
    require(game.createUnit(PLAYER, UName::CAVALRY, 11, 10, lane::Mid),
            "immune counter test cavalry should spawn");
    counterInfantry = game.enemys.back().get();
    counterCavalry = game.myunits.back().get();
    const int cavalryBeforeImmuneHit = counterCavalry->Health;
    counterInfantry->autoAttack(counterCavalry);
    const int immuneDamage = cavalryBeforeImmuneHit - counterCavalry->Health;
    require(immuneDamage > 0 && immuneDamage < counteredDamage,
            "Charge level 2 should stop Infantry from countering Cavalry");

    game.clear();
    for (int x = 10; x <= 16; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    Building tower;
    tower.id = 999;
    tower.team = PLAYER;
    tower.type = building::DefenseTower;
    tower.point = Point(10, 10);
    tower.complete = true;
    tower.maxHealth = buildingDefinition(building::DefenseTower).maxHealth;
    tower.health = tower.maxHealth;
    tower.attackTimer = realtime::DefenseTowerAttackCooldown;
    game.buildings.push_back(tower);
    require(game.createUnit(AI, UName::GUARDIAN, 15, 10, lane::Mid),
            "tower fixed-splash primary target should spawn");
    MoveableUnit* towerTarget = game.enemys.back().get();
    require(game.createUnit(AI, UName::INFANTARY, 16, 10, lane::Mid),
            "tower fixed-splash secondary target should spawn");
    MoveableUnit* splashTarget = game.enemys.back().get();
    const int towerTargetBefore = towerTarget->Health;
    const int splashTargetBefore = splashTarget->Health;
    game.updateDefenseTowers(0.25f);
    const int towerDamage = towerTargetBefore - towerTarget->Health;
    const int splashDamage = splashTargetBefore - splashTarget->Health;
    require(towerDamage == config::DefenseTowerDamage + config::DefenseTowerSplashDamage,
            "defense tower primary hits should combine flat damage with fixed splash");
    require(splashDamage == config::DefenseTowerSplashDamage,
            "defense tower splash should use a fixed value on nearby units");
    require(towerDamage < config::GuardianHealth / 10,
            "defense tower splash should not scale from tank max health");

    game.clear();
    game.playerCommand = 1000;
    const Point veteranSpawn = game.findSpawnPointAround(game.Red_baseP);
    require(game.createUnit(PLAYER, UName::INFANTARY, veteranSpawn.x, veteranSpawn.y, lane::Mid),
            "test veteran should spawn before a tech upgrade");
    MoveableUnit* veteran = game.myunits.back().get();
    require(game.upgradeTeam(PLAYER),
            "tech upgrade should succeed for existing-unit health consistency");
    const int expectedTechHealth = static_cast<int>(std::round(
        static_cast<float>(unitDefinition(UName::INFANTARY).maxHealth) * game.unitHealthMultiplier(PLAYER, UName::INFANTARY)));
    require(std::abs(veteran->Health - expectedTechHealth) <= 1,
            "existing unit health should match the additive tech multiplier");
    const Point recruitSpawn = game.findSpawnPointAround(game.Red_baseP);
    require(game.createUnit(PLAYER, UName::INFANTARY, recruitSpawn.x, recruitSpawn.y, lane::Mid),
            "test recruit should spawn after a tech upgrade");
    MoveableUnit* recruit = game.myunits.back().get();
    require(std::abs(recruit->Health - veteran->Health) <= 1,
            "fresh and existing infantry should have the same health after tech upgrades");

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
    for (int x = 10; x <= 13; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SHOOTER, 10, 10, lane::Mid),
            "test shooter should spawn for line-of-sight checks");
    require(game.createUnit(AI, UName::INFANTARY, 13, 10, lane::Mid),
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
    require(game.createUnit(PLAYER, UName::SHOOTER, 13, 10, lane::Mid),
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
    for (int x = 10; x <= 15; ++x) {
        game.setTileID(x, 12, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SIEGE, 10, 12, lane::Mid),
            "test siege should spawn for tank resistance checks");
    require(game.createUnit(AI, UName::GUARDIAN, 15, 12, lane::Mid),
            "test guardian should spawn in siege range");
    MoveableUnit* testSiege = game.myunits.back().get();
    MoveableUnit* testGuardian = game.enemys.back().get();
    const int guardianHealthBefore = testGuardian->Health;
    testSiege->autoAttack(testGuardian);
    const int siegeDamageToGuardian = guardianHealthBefore - testGuardian->Health;
    require(siegeDamageToGuardian > 0 && siegeDamageToGuardian < config::SiegeDamage,
            "guardian tanks should resist siege instead of being countered by it");

    game.clear();
    const Point topExit = game.laneRallyPoint(PLAYER, lane::Top, 0);
    const Point topMid = game.laneRallyPoint(PLAYER, lane::Top, 1);
    const Point offLaneSpawn(topExit.x + 4, game.Red_baseP.y + 3);
    clearArea(game, topExit, 2);
    clearArea(game, offLaneSpawn, 1);
    require(game.createUnit(PLAYER, UName::INFANTARY, offLaneSpawn.x, offLaneSpawn.y, lane::Top),
            "top-lane rally test unit should spawn ahead of the exit anchor");
    MoveableUnit* topLaneUnit = game.myunits.back().get();
    require(topLaneUnit->nextRallyStage == 0,
            "newly spawned units should start at the lane-exit rally stage");
    Point firstTopRally = game.chooseStrategicRallyPoint(*topLaneUnit);
    require(firstTopRally.x == topExit.x && firstTopRally.y == topExit.y,
            "off-lane units should still route through the selected lane exit anchor");
    topLaneUnit->x = topExit.x;
    topLaneUnit->y = topExit.y;
    Point secondTopRally = game.chooseStrategicRallyPoint(*topLaneUnit);
    require(secondTopRally.x == topMid.x && secondTopRally.y == topMid.y && topLaneUnit->nextRallyStage == 1,
            "reaching the lane exit should advance to the central lane anchor");

    game.clear();
    const Point botEnemyRally = game.laneRallyPoint(PLAYER, lane::Bot, lane_geometry::RallyStageCount - 1);
    for (int x = botEnemyRally.x - 6; x <= botEnemyRally.x + 2; ++x) {
        game.setTileID(x, botEnemyRally.y, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, botEnemyRally.x - 2, botEnemyRally.y, lane::Bot),
            "bot-lane test unit should spawn near the enemy-side rally");
    MoveableUnit* botLaneUnit = game.myunits.back().get();
    botLaneUnit->nextRallyStage = lane_geometry::RallyStageCount - 1;
    Point rallyAfterArrival = game.chooseStrategicRallyPoint(*botLaneUnit);
    require(rallyAfterArrival.x < 0 && botLaneUnit->nextRallyStage >= lane_geometry::RallyStageCount,
            "reaching the bot enemy-side rally should commit the unit to assault");
    botLaneUnit->x = botEnemyRally.x - 5;
    botLaneUnit->y = botEnemyRally.y;
    Point rallyAfterDetour = game.chooseStrategicRallyPoint(*botLaneUnit);
    require(rallyAfterDetour.x < 0,
            "committed bot-lane units should not re-request old rally points after a detour");

    game.clear();
    for (int y = 14; y <= 16; ++y) {
        for (int x = 9; x <= 12; ++x) {
            game.setTileID(x, y, tile::Empty);
        }
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, 10, 15, lane::Mid),
            "moving test unit should spawn");
    require(game.createUnit(PLAYER, UName::INFANTARY, 11, 15, lane::Mid),
            "blocking test unit should spawn");
    MoveableUnit* crowdedMover = game.myunits.front().get();
    crowdedMover->nextRallyStage = 1;
    crowdedMover->pendingPathGoal = game.laneRallyPoint(PLAYER, lane::Mid, 1);
    crowdedMover->mypath.clear();
    crowdedMover->mypath.push_back(Point(11, 15));
    realtime::updateAutoCombat(game, 1.0f);
    require(crowdedMover->x == 11 && crowdedMover->y == 15,
            "combat movement should ignore other unit bodies and allow stacking");
    require(game.myunits.back()->x == 11 && game.myunits.back()->y == 15,
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

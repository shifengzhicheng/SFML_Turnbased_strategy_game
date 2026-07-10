#include "Game.h"
#include "AutoCombat.h"
#include "BuildingDefinition.h"
#include "LaneGeometry.h"
#include "PolicyModel.h"
#include "PerkMechanics.h"
#include "RealtimeConfig.h"
#include "SidebarLayout.h"
#include "UnitDefinition.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

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

    int openNeighborCount(const Game& game, Point point)
    {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                if (game.isCellWalkableForUnit(point.x + dx, point.y + dy)) {
                    ++count;
                }
            }
        }
        return count;
    }

    tile::ID tileAt(const Game& game, Point point)
    {
        return game.tiles[point.y * game.horizontalTiles + point.x].getID();
    }

    Building makeCompletedBuilding(int id, int team, int type, Point point, int laneIndex = lane::Mid)
    {
        Building building;
        building.id = id;
        building.team = team;
        building.type = type;
        building.laneIndex = laneIndex;
        building.point = point;
        building.complete = true;
        building.maxHealth = buildingDefinition(type).maxHealth;
        building.health = building.maxHealth;
        return building;
    }
}

int main()
{
    Game game;
    game.pathfinding.setExecutionMode(PathfindingService::ExecutionMode::Synchronous);
    game.window.setVisible(false);
    game.debugLogging = false;
    game.autoChooseRewards = true;
    game.externalAIControl = true;
    game.gameSceneState = SCENE_GAME;
    game.matchSeedOverride = 424242u;
    game.clear();

    const auto firstSeededMap = game.maze;
    game.buildRewardChoices();
    std::array<int, 3> firstSeededRewards{};
    for (std::size_t i = 0; i < firstSeededRewards.size(); ++i) {
        firstSeededRewards[i] = game.perkChoices[i].type;
    }
    game.clear();
    require(game.currentMatchSeed == game.matchSeedOverride && game.maze == firstSeededMap,
            "one match seed should reproduce the complete generated map");
    game.buildRewardChoices();
    for (std::size_t i = 0; i < firstSeededRewards.size(); ++i) {
        require(game.perkChoices[i].type == firstSeededRewards[i],
                "one match seed should reproduce the initial Rogue choices");
    }

    game.playerBaseShieldTimer = 0.f;
    game.gameTimeSeconds = 0.f;
    const float earlyBaseDamage = game.baseDamageTakenMultiplier(UName::INFANTARY, PLAYER);
    game.gameTimeSeconds = 240.f;
    const float midBaseDamage = game.baseDamageTakenMultiplier(UName::INFANTARY, PLAYER);
    game.gameTimeSeconds = 420.f;
    const float lateBaseDamage = game.baseDamageTakenMultiplier(UName::INFANTARY, PLAYER);
    game.gameTimeSeconds = 900.f;
    const float overtimeBaseDamage = game.baseDamageTakenMultiplier(UName::INFANTARY, PLAYER);
    require(earlyBaseDamage < midBaseDamage && midBaseDamage < lateBaseDamage
                && lateBaseDamage < overtimeBaseDamage && overtimeBaseDamage > 1.f,
            "base protection should fade into bounded late-game structure escalation");
    require(game.structureDamageEscalation() <= config::EscalationDamageCap,
            "late-game structure escalation should respect its cap");

    game.clear();
    game.gameTimeSeconds = config::EscalationStartSeconds + 60.f;
    const Point pressurePoint(game.Blue_baseP.x - config::CommandZonePressureRadius + 1, game.Blue_baseP.y);
    clearArea(game, pressurePoint, 1);
    require(game.createUnit(PLAYER, UName::INFANTARY, pressurePoint.x, pressurePoint.y, lane::Mid),
            "command-zone pressure test unit should spawn");
    const int baseHealthBeforePressure = game.Base_blue->Health;
    game.applyCommandZonePressure(PLAYER);
    require(game.Base_blue->Health < baseHealthBeforePressure,
            "occupying enemy HQ territory in overtime should create bounded siege pressure");

    game.clear();
    game.Base_red->Health = config::BaseHealth - 600;
    Building lostTower = makeCompletedBuilding(9001, PLAYER, building::DefenseTower, Point(10, 10));
    const int firstRepairStart = game.Base_red->Health;
    game.applyStructureLossRelief(lostTower);
    require(game.playerReliefCharges == config::ComebackReliefCharges - 1
                && game.Base_red->Health == firstRepairStart + config::EmergencyTowerRepair
                && game.playerBaseShieldTimer > 0.f,
            "the first structure loss should consume one emergency relief charge");
    game.playerBaseShieldTimer = 0.f;
    game.applyStructureLossRelief(lostTower);
    require(game.playerReliefCharges == 0 && game.playerBaseShieldTimer > 0.f,
            "the second structure loss should consume the final relief charge");
    game.playerBaseShieldTimer = 0.f;
    const int healthAfterRelief = game.Base_red->Health;
    game.applyStructureLossRelief(lostTower);
    require(game.Base_red->Health == healthAfterRelief && game.playerBaseShieldTimer == 0.f,
            "later structure losses should grant salvage without infinite repair or shields");

    game.clear();
    game.gameTimeSeconds = 100.f;
    game.playerCommand = 1000;
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildTower, lane::Top)),
            "rebuild cooldown test tower should be placed");
    game.buildings.back().complete = true;
    game.buildings.back().health = 0;
    game.cleanupDestroyedBuildings();
    require(!game.canRebuildLane(PLAYER, lane::Top) && game.canRebuildLane(PLAYER, lane::Bot),
            "destroying a structure should contest only its own lane");
    game.gameTimeSeconds += config::LaneRebuildLockSeconds;
    require(game.canRebuildLane(PLAYER, lane::Top),
            "a contested lane should reopen construction after the bounded cooldown");

    game.clear();
    game.gameTimeSeconds = 900.f;
    game.playerEconomyLevel = 6;
    game.aiEconomyLevel = 6;
    game.playerUpgradeLevel = 8;
    game.aiUpgradeLevel = 8;
    game.playerCommand = 700;
    game.aiCommand = 700;
    require(game.resourceIncome(PLAYER) == game.resourceIncome(AI),
            "normal AI economy should use the same income formula as the player");
    require(game.economyUpgradeCost(PLAYER) == game.economyUpgradeCost(AI),
            "normal AI economy upgrades should not receive hidden discounts");
    require(game.upgradeCostForNextLevel(PLAYER) == game.upgradeCostForNextLevel(AI),
            "normal AI technology should not receive hidden discounts");
    const int overtimeTechCost = game.upgradeCostForNextLevel(PLAYER);
    game.gameTimeSeconds = 0.f;
    const int openingTechCost = game.upgradeCostForNextLevel(PLAYER);
    require(overtimeTechCost < openingTechCost
                && overtimeTechCost >= static_cast<int>(std::floor(openingTechCost * config::TechOvertimeDiscountFloor)),
            "visible overtime should accelerate technology without dropping below its shared floor");
    game.gameTimeSeconds = 900.f;
    const auto playerFeatures = policy::extractFeatures(game, PLAYER);
    const auto aiFeatures = policy::extractFeatures(game, AI);
    for (std::size_t i = 0; i < policy::FeatureCount; ++i) {
        require(std::abs(playerFeatures[i] - aiFeatures[i]) < 0.0001f,
                "shared policy features should be side-symmetric in a symmetric state");
    }
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
    const int mapH = config::MapTilesY;
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
        require(openNeighborCount(game, playerBarracks) >= 2
                    && openNeighborCount(game, aiBarracks) >= 2,
                "automatic barracks should leave room for workers and unit spawns");
    }
    for (int x = 15; x < mapW - 15; x += 4) {
        const int midY = static_cast<int>(std::round(lane_geometry::laneYAtX(mapW, mapH, lane::Mid, x)));
        require(game.isCellWalkableForUnit(x, midY - 2)
                    && game.isCellWalkableForUnit(x, midY - 1)
                    && game.isCellWalkableForUnit(x, midY)
                    && game.isCellWalkableForUnit(x, midY + 1)
                    && game.isCellWalkableForUnit(x, midY + 2),
                "middle lane should keep a five-tile playable corridor");
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

    game.clear();
    game.playerCommand = 1000;
    const Point midDefense = game.laneDefensePoint(PLAYER, lane::Mid);
    clearArea(game, midDefense, 5);
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildTower, lane::Mid)),
            "first tower should auto-place near the selected lane");
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildTower, lane::Mid)),
            "second tower should auto-place near the selected lane");
    const Point firstTower = game.buildings[0].point;
    const Point secondTower = game.buildings[1].point;
    require(std::max(std::abs(firstTower.x - secondTower.x), std::abs(firstTower.y - secondTower.y))
                >= config::TowerPlacementMinSpacing,
            "automatic tower placement should keep towers from visually stacking");
    const auto towerCoversLane = [&game](Point tower, int laneIndex) {
        const Point target = game.laneRallyPoint(PLAYER, laneIndex, 1);
        const int range = game.defenseTowerRange(PLAYER);
        return distanceSquared(tower, target) <= range * range
            && game.hasLineOfSightForTower(tower, target, PLAYER);
    };
    require(towerCoversLane(firstTower, lane::Mid) && towerCoversLane(secondTower, lane::Mid),
            "automatic tower placement should cover the selected lane approach");
    for (Point tower : {firstTower, secondTower}) {
        const int laneY = static_cast<int>(std::round(lane_geometry::laneYAtX(mapW, mapH, lane::Mid, tower.x)));
        require(std::abs(tower.y - laneY) >= 1,
                "automatic tower placement should sit on lane shoulders instead of the lane road");
    }
    game.clear();

    game.playerCommand = 1000;
    game.playerUpgradeLevel = 8;
    game.playerEconomyLevel = 6;
    require(game.executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, lane::Top))
                && game.executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, lane::Mid))
                && game.executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, lane::Bot)),
            "multiple automatic barracks should fit into the protected base pocket");
    require(game.totalBuildingCount(PLAYER, building::Barracks) == 3,
            "barracks pocket should support multiple production buildings");
    for (auto a = game.buildings.begin(); a != game.buildings.end(); ++a) {
        if (a->type != building::Barracks) {
            continue;
        }
        require(distanceSquared(a->point, game.Red_baseP) <= 36,
                "every automatic barracks should remain close enough to rebuild after a raid");
        auto b = a;
        for (++b; b != game.buildings.end(); ++b) {
            if (b->type != building::Barracks) {
                continue;
            }
            require(std::max(std::abs(a->point.x - b->point.x), std::abs(a->point.y - b->point.y)) >= 3,
                    "automatic barracks should not stack into a cramped production clump");
        }
    }
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
    require(game.resourceIncome(PLAYER) >= incomeBeforeMining + 6,
            "Mining perk should make economy upgrades visibly stronger");
    const float infantryDamageTakenBefore = game.unitDamageTakenMultiplier(PLAYER, UName::INFANTARY);
    game.applyPerk(PLAYER, perk::Drill);
    require(game.unitDamageTakenMultiplier(PLAYER, UName::INFANTARY) < infantryDamageTakenBefore,
            "Drill level 1 should immediately harden Infantry");
    game.applyPerk(PLAYER, perk::Fortitude);
    require(game.unitTauntsNearbyEnemies(PLAYER, UName::GUARDIAN)
                && game.unitDamageTakenMultiplier(PLAYER, UName::GUARDIAN) < 1.f,
            "Fortitude level 1 should immediately add Guardian taunt and mitigation");
    const float baseChargeMultiplier = game.cavalryChargeDamageMultiplier(PLAYER);
    game.applyPerk(PLAYER, perk::Charge);
    require(game.cavalryChargeDamageMultiplier(PLAYER) > baseChargeMultiplier,
            "Charge level 1 should immediately improve charge damage");
    const int siegeTargetsBefore = game.additionalAttackTargets(PLAYER, UName::SIEGE);
    game.applyPerk(PLAYER, perk::SiegeCraft);
    require(game.additionalAttackTargets(PLAYER, UName::SIEGE) > siegeTargetsBefore,
            "Siege Craft level 1 should immediately widen splash");
    const TowerMechanics towerBefore = towerMechanicsFor(game.playerPerkLevels);
    game.applyPerk(PLAYER, perk::TowerCraft);
    const TowerMechanics towerAfter = towerMechanicsFor(game.playerPerkLevels);
    require(!towerBefore.prioritizesSiege && towerAfter.prioritizesSiege
                && towerAfter.splashDamage > towerBefore.splashDamage,
            "Tower Craft level 1 should immediately improve tower targeting and splash");
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
    const std::array<PerkChoice, 3> firstRewardChoices = game.perkChoices;
    game.rerollRewardChoices();
    require(game.playerRewardRerolls == 0,
            "reward refresh should consume the free reroll");
    for (const auto& refreshed : game.perkChoices) {
        require(std::none_of(firstRewardChoices.begin(), firstRewardChoices.end(), [&refreshed](const PerkChoice& previous) {
                    return previous.type == refreshed.type;
                }),
                "reward refresh should replace every card while the pool permits it");
    }

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
    require(masteryInfantry->deploymentReadyTime > game.gameTimeSeconds
                && masteryInfantry->deploymentReadyTime - game.gameTimeSeconds <= config::ArmyWaveIntervalSeconds,
            "new recruits should join the next bounded army deployment wave");
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
    require(secondaryGuardianTarget->Health == secondaryBeforeGuardian,
            "baseline guardian should not receive a free cleave before Fortitude upgrades");
    game.applyPerk(PLAYER, perk::Fortitude);
    game.applyPerk(PLAYER, perk::Fortitude);
    game.applyPerk(PLAYER, perk::Fortitude);
    baselineGuardian->autoAttack(primaryGuardianTarget);
    require(secondaryGuardianTarget->Health < secondaryBeforeGuardian,
            "Fortitude level 3 should unlock Guardian cleave");

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
    for (int x = 9; x <= 12; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::CAVALRY, 10, 10, lane::Mid)
                && game.createUnit(AI, UName::INFANTARY, 11, 10, lane::Mid),
            "charge damage test units should spawn");
    MoveableUnit* chargingCavalry = game.myunits.back().get();
    MoveableUnit* chargeTarget = game.enemys.back().get();
    const int chargeTargetStartHealth = chargeTarget->Health;
    chargingCavalry->autoAttack(chargeTarget);
    const int normalCavalryDamage = chargeTargetStartHealth - chargeTarget->Health;
    chargingCavalry->tilesMovedSinceAttack = config::CavalryChargeTiles;
    const int healthBeforeCharge = chargeTarget->Health;
    chargingCavalry->autoAttack(chargeTarget);
    const int chargedCavalryDamage = healthBeforeCharge - chargeTarget->Health;
    require(chargedCavalryDamage > normalCavalryDamage,
            "Cavalry should deal a stronger hit after crossing the charge threshold");
    require(chargingCavalry->tilesMovedSinceAttack == 0,
            "a Cavalry attack should consume its accumulated charge distance");

    game.clear();
    clearArea(game, Point(10, 10), 3);
    require(game.createUnit(PLAYER, UName::SHOOTER, 10, 10, lane::Mid)
                && game.createUnit(AI, UName::INFANTARY, 11, 10, lane::Mid),
            "kiting test units should spawn");
    MoveableUnit* kitingShooter = game.myunits.back().get();
    kitingShooter->realtimeMoveTimer = kitingShooter->realtimeMoveStepSeconds();
    realtime::updateAutoCombat(game, 0.01f);
    require(kitingShooter->x == 9 && kitingShooter->y == 10,
            "Shooters should step away from enemies that enter their preferred range");

    game.clear();
    for (int x = 9; x <= 15; ++x) {
        game.setTileID(x, 12, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SIEGE, 10, 12, lane::Mid)
                && game.createUnit(AI, UName::INFANTARY, 14, 12, lane::Mid),
            "deployment test units should spawn");
    MoveableUnit* deployingSiege = game.myunits.back().get();
    MoveableUnit* deploymentTarget = game.enemys.back().get();
    deployingSiege->realtimeAttackTimer = deployingSiege->realtimeAttackCooldownSeconds();
    const int healthBeforeDeployment = deploymentTarget->Health;
    realtime::updateAutoCombat(game, realtime::SiegeDeploymentSeconds * 0.5f);
    require(deploymentTarget->Health == healthBeforeDeployment,
            "Siege should not fire before completing its stationary deployment");
    realtime::updateAutoCombat(game, realtime::SiegeDeploymentSeconds * 0.6f);
    require(deploymentTarget->Health < healthBeforeDeployment,
            "Siege should fire once its deployment delay has elapsed");

    game.clear();
    for (int x = 9; x <= 15; ++x) {
        game.setTileID(x, 14, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SIEGE, 10, 14, lane::Mid)
                && game.createUnit(AI, UName::INFANTARY, 13, 14, lane::Mid),
            "structure-priority test units should spawn");
    MoveableUnit* structureSiege = game.myunits.back().get();
    MoveableUnit* ignoredEscort = game.enemys.back().get();
    game.buildings.push_back(makeCompletedBuilding(1901, AI, building::DefenseTower, Point(14, 14)));
    Building& priorityTower = game.buildings.back();
    const int towerHealthBefore = priorityTower.health;
    const int escortHealthBefore = ignoredEscort->Health;
    structureSiege->stationarySeconds = realtime::SiegeDeploymentSeconds;
    structureSiege->realtimeAttackTimer = structureSiege->realtimeAttackCooldownSeconds();
    realtime::updateAutoCombat(game, 0.01f);
    require(priorityTower.health < towerHealthBefore && ignoredEscort->Health == escortHealthBefore,
            "unprovoked Siege should prioritize a reachable structure over nearby units");

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
    const Point redTopGate(game.Red_baseP.x + 5, game.Red_baseP.y - 3);
    const Point redBotGate(game.Red_baseP.x + 5, game.Red_baseP.y + 4);
    const Point redMiddleGate(game.Red_baseP.x + 5, game.Red_baseP.y + 1);
    const Point blueTopGate(game.Blue_baseP.x - 5, game.Blue_baseP.y - 3);
    const Point blueBotGate(game.Blue_baseP.x - 5, game.Blue_baseP.y + 4);
    const Point blueMiddleGate(game.Blue_baseP.x - 5, game.Blue_baseP.y + 1);
    require(game.isCellWalkableForUnit(redTopGate.x, redTopGate.y)
                && game.isCellWalkableForUnit(redBotGate.x, redBotGate.y)
                && game.isCellWalkableForUnit(redMiddleGate.x, redMiddleGate.y)
                && game.isCellWalkableForUnit(blueTopGate.x, blueTopGate.y)
                && game.isCellWalkableForUnit(blueBotGate.x, blueBotGate.y)
                && game.isCellWalkableForUnit(blueMiddleGate.x, blueMiddleGate.y),
            "base templates should keep top, middle, and bot approach gates walkable");
    require(tileAt(game, Point(game.Red_baseP.x + 5, game.Red_baseP.y - 2)) == tile::Mount
                && tileAt(game, Point(game.Red_baseP.x + 6, game.Red_baseP.y - 1)) == tile::Tree
                && tileAt(game, Point(game.Blue_baseP.x - 5, game.Blue_baseP.y - 2)) == tile::Mount
                && tileAt(game, Point(game.Blue_baseP.x - 6, game.Blue_baseP.y - 1)) == tile::Tree,
            "base templates should place mirrored front blockers that make the direct route harder");

    game.clear();
    for (int x = 10; x <= 16; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    Building losTower = makeCompletedBuilding(2001, PLAYER, building::DefenseTower, Point(10, 10));
    losTower.attackTimer = realtime::DefenseTowerAttackCooldown;
    game.buildings.push_back(losTower);
    require(game.createUnit(AI, UName::INFANTARY, 15, 10, lane::Mid),
            "tower line-of-sight target should spawn");
    MoveableUnit* losTowerTarget = game.enemys.back().get();
    game.setTileID(12, 10, tile::Tree);
    const int targetHealthBeforeBlockedTower = losTowerTarget->Health;
    game.updateDefenseTowers(0.25f);
    require(losTowerTarget->Health == targetHealthBeforeBlockedTower,
            "terrain between a tower and target should block tower fire");
    game.setTileID(12, 10, tile::Empty);
    game.buildings.back().attackTimer = realtime::DefenseTowerAttackCooldown;
    game.updateDefenseTowers(0.25f);
    require(losTowerTarget->Health < targetHealthBeforeBlockedTower,
            "clearing tower line-of-sight should restore tower fire");

    game.clear();
    for (int x = 10; x <= 15; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    game.setTileID(12, 10, tile::Player_Tower);
    require(!game.hasLineOfSight(Point(10, 10), Point(15, 10)),
            "ordinary line-of-sight should still treat towers as blockers");
    require(game.hasLineOfSightForTower(Point(10, 10), Point(15, 10), PLAYER),
            "friendly towers should not block another tower's defensive fire");
    game.setTileID(12, 10, tile::Enemy_Tower);
    require(!game.hasLineOfSightForTower(Point(10, 10), Point(15, 10), PLAYER),
            "enemy towers should remain real line-of-sight blockers");

    game.clear();
    for (int x = 10; x <= 15; ++x) {
        game.setTileID(x, 10, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::SIEGE, 10, 10, lane::Mid),
            "building line-of-sight attacker should spawn");
    MoveableUnit* losSiege = game.myunits.back().get();
    Building losBuilding = makeCompletedBuilding(2002, AI, building::DefenseTower, Point(14, 10));
    game.setTileID(12, 10, tile::Tree);
    require(!game.canAttackBuilding(*losSiege, losBuilding),
            "terrain between a unit and building should block building attacks");
    game.setTileID(12, 10, tile::Empty);
    require(game.canAttackBuilding(*losSiege, losBuilding),
            "clearing building line-of-sight should restore building attacks");

    game.clear();
    clearArea(game, Point(35, 15), 8);
    require(game.createUnit(PLAYER, UName::INFANTARY, 32, 12, lane::Top),
            "top-lane building target test unit should spawn");
    MoveableUnit* topRaider = game.myunits.back().get();
    game.buildings.push_back(makeCompletedBuilding(2101, AI, building::DefenseTower, Point(36, 15), lane::Mid));
    game.buildings.push_back(makeCompletedBuilding(2102, AI, building::Barracks, Point(37, 11), lane::Top));
    Building* topTarget = game.chooseBuildingTarget(*topRaider);
    require(topTarget != nullptr && topTarget->id == 2102,
            "top-lane units should prefer same-side production over collapsing onto the central fort");
    game.myunits.clear();
    require(game.createUnit(PLAYER, UName::INFANTARY, 32, 18, lane::Bot),
            "bot-lane building target test unit should spawn");
    MoveableUnit* botRaider = game.myunits.back().get();
    game.buildings.push_back(makeCompletedBuilding(2103, AI, building::Barracks, Point(37, 19), lane::Bot));
    Building* botTarget = game.chooseBuildingTarget(*botRaider);
    require(botTarget != nullptr && botTarget->id == 2103,
            "bot-lane units should prefer same-side production over off-lane targets");
    game.buildings.erase(std::remove_if(game.buildings.begin(), game.buildings.end(), [](const Building& building) {
        return building.laneIndex == lane::Top;
    }), game.buildings.end());
    game.myunits.clear();
    require(game.createUnit(PLAYER, UName::INFANTARY, 32, 11, lane::Top),
            "breached top-lane unit should spawn in enemy territory");
    MoveableUnit* breachedTopUnit = game.myunits.back().get();
    require(game.chooseBuildingTarget(*breachedTopUnit) == nullptr,
            "a breached lane should expose the HQ instead of pulling units across other lanes");
    game.myunits.clear();
    game.gameTimeSeconds = config::EscalationStartSeconds;
    const Point overtimeAssaultPoint(game.Blue_baseP.x - config::OvertimeHQAssaultRadius + 1, game.Blue_baseP.y);
    clearArea(game, overtimeAssaultPoint, 1);
    require(game.createUnit(PLAYER, UName::SIEGE, overtimeAssaultPoint.x, overtimeAssaultPoint.y, lane::Bot),
            "overtime HQ assault unit should spawn inside the command zone");
    require(game.chooseBuildingTarget(*game.myunits.back()) == nullptr,
            "overtime units inside the HQ zone should stop chasing replacement structures");

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
    crowdedMover->deploymentReadyTime = game.gameTimeSeconds;
    crowdedMover->nextRallyStage = 1;
    crowdedMover->pendingPathGoal = game.laneRallyPoint(PLAYER, lane::Mid, 1);
    crowdedMover->mypath.clear();
    crowdedMover->mypath.push_back(Point(11, 15));
    realtime::updateAutoCombat(game, 1.0f);
    require(crowdedMover->x == 11 && crowdedMover->y == 15,
            "combat movement should ignore other unit bodies and allow stacking");
    require(game.myunits.back()->x == 11 && game.myunits.back()->y == 15,
            "the blocking unit should remain stacked on the same cell");

    game.clear();
    for (int x = 8; x <= 18; ++x) {
        game.setTileID(x, 15, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, 10, 15, lane::Mid),
            "stale path test unit should spawn");
    MoveableUnit* stalePathUnit = game.myunits.back().get();
    game.requestPathForUnit(*stalePathUnit, Point(16, 15));
    stalePathUnit->x = 12;
    stalePathUnit->y = 15;
    for (int i = 0; i < 60 && stalePathUnit->pendingPathRequest != 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        game.applyPathResults();
    }
    require(stalePathUnit->pendingPathRequest == 0,
            "pathfinding worker should finish the stale path request");
    require(!stalePathUnit->mypath.empty()
                && stalePathUnit->mypath.front().x == 13
                && stalePathUnit->mypath.front().y == 15,
            "async path results should be trimmed to the unit's current tile");

    game.clear();
    for (int x = 8; x <= 18; ++x) {
        game.setTileID(x, 15, tile::Empty);
    }
    require(game.createUnit(PLAYER, UName::INFANTARY, 10, 15, lane::Mid),
            "corrupt path test unit should spawn");
    MoveableUnit* corruptPathUnit = game.myunits.back().get();
    corruptPathUnit->deploymentReadyTime = game.gameTimeSeconds;
    corruptPathUnit->nextRallyStage = 1;
    corruptPathUnit->pendingPathGoal = game.laneRallyPoint(PLAYER, lane::Mid, 1);
    corruptPathUnit->mypath.push_back(Point(15, 15));
    realtime::updateAutoCombat(game, 1.0f);
    require(corruptPathUnit->x == 10 && corruptPathUnit->y == 15,
            "combat movement should reject non-adjacent path steps instead of teleporting");
    require(corruptPathUnit->mypath.empty(),
            "rejecting a corrupt path should force a fresh path request later");

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

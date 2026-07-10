#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "UnitDefinition.h"
#include "PerkMechanics.h"
#include "UnitStatsResolver.h"
#include "UnitUpgradeDefinition.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    const std::array<int, perk::Count>& perkLevelsForTeam(const Game& game, int team)
    {
        return team == PLAYER ? game.playerPerkLevels : game.aiPerkLevels;
    }

    const UnitMasteryState& masteryForTeam(const Game& game, int team)
    {
        return team == PLAYER ? game.playerMastery : game.aiMastery;
    }

    TeamStatContext statContextForTeam(const Game& game, int team)
    {
        return TeamStatContext{
            team == PLAYER ? game.playerUpgradeLevel : game.aiUpgradeLevel,
            &masteryForTeam(game, team),
            &perkLevelsForTeam(game, team)
        };
    }

    UnitComputedStats resolvedStatsForTeam(const Game& game, int team, int unitName)
    {
        return resolveUnitStats(unitDefinition(unitName), statContextForTeam(game, team));
    }

    bool baseCounter(int attacker, int defender)
    {
        return (attacker == UName::SHOOTER && defender == UName::INFANTARY)
            || (attacker == UName::INFANTARY && defender == UName::CAVALRY)
            || (attacker == UName::CAVALRY && (defender == UName::SHOOTER || defender == UName::SIEGE));
    }
}

int Game::buildingCap(int team, int type) const
{
    const int tech = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int economy = economyLevelForTeam(team);
    if (type == building::Barracks) {
        return std::min(config::BarracksCap, config::BarracksBaseCap + tech / 4 + economy / 2);
    }
    if (type == building::DefenseTower) {
        return std::min(config::TowerCap,
                        config::TowerBaseCap + tech / 4 + economy / 3 + perkLevel(team, perk::TowerCraft) / 2);
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
    if (!isTrainableUnit(unitName)) {
        return damageMultiplier(team);
    }
    return resolvedStatsForTeam(*this, team, unitName).damageMultiplier;
}

float Game::unitHealthMultiplier(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 1.f;
    }
    return resolvedStatsForTeam(*this, team, unitName).healthMultiplier;
}

float Game::unitAttackCooldownMultiplier(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 1.f;
    }
    return std::clamp(resolvedStatsForTeam(*this, team, unitName).attackCooldownMultiplier,
                      config::AttackCooldownFloor, 1.f);
}

float Game::unitBuildingDamageMultiplier(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 1.f;
    }
    return resolvedStatsForTeam(*this, team, unitName).buildingDamageMultiplier;
}

float Game::unitDamageTakenMultiplier(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 1.f;
    }
    return unitMechanicsFor(unitName, perkLevelsForTeam(*this, team)).damageTakenMultiplier;
}

float Game::cavalryChargeDamageMultiplier(int team) const
{
    return unitMechanicsFor(UName::CAVALRY, perkLevelsForTeam(*this, team)).chargeDamageMultiplier;
}

int Game::unitAttackRange(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 1;
    }
    return resolvedStatsForTeam(*this, team, unitName).attackRange;
}

int Game::unitMasteryLevel(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 0;
    }
    return ::unitMasteryLevel(masteryForTeam(*this, team), unitName);
}

int Game::unitMasteryUpgradeCost(int team, int unitName) const
{
    if (!isTrainableUnit(unitName)) {
        return 0;
    }
    return ::unitMasteryUpgradeCost(unitName, unitMasteryLevel(team, unitName));
}

bool Game::canUpgradeUnitMastery(int team, int unitName) const
{
    const int cost = unitMasteryUpgradeCost(team, unitName);
    return cost > 0 && isUnitUnlocked(team, unitName) && commandForTeam(team) >= cost;
}

bool Game::counterApplies(int attackerTeam, int attackerUnitName, int defenderTeam, int defenderUnitName) const
{
    if (!baseCounter(attackerUnitName, defenderUnitName)) {
        return false;
    }
    if (defenderUnitName == UName::CAVALRY) {
        const UnitMechanics cavalry = unitMechanicsFor(UName::CAVALRY, perkLevelsForTeam(*this, defenderTeam));
        if (cavalry.ignoresInfantryCounter && attackerUnitName == UName::INFANTARY) {
            return false;
        }
    }
    if (defenderUnitName == UName::INFANTARY) {
        const UnitMechanics infantry = unitMechanicsFor(UName::INFANTARY, perkLevelsForTeam(*this, defenderTeam));
        if (infantry.ignoresShooterCounter && attackerUnitName == UName::SHOOTER) {
            return false;
        }
    }
    (void)attackerTeam;
    return true;
}

int Game::additionalAttackTargets(int team, int unitName) const
{
    return unitMechanicsFor(unitName, perkLevelsForTeam(*this, team)).additionalAttackTargets;
}

float Game::additionalTargetDamageMultiplier(int team, int unitName) const
{
    return unitMechanicsFor(unitName, perkLevelsForTeam(*this, team)).additionalTargetDamageMultiplier;
}

bool Game::unitTauntsNearbyEnemies(int team, int unitName) const
{
    return unitMechanicsFor(unitName, perkLevelsForTeam(*this, team)).tauntsNearbyEnemies;
}

float Game::siegeDamageTakenMultiplier(int team, int unitName) const
{
    return unitMechanicsFor(unitName, perkLevelsForTeam(*this, team)).siegeDamageTakenMultiplier;
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
    const float logistics = static_cast<float>(perkLevel(team, perk::Logistics)) * config::LogisticsTrainTimeReduction;
    return std::clamp(1.f - logistics, config::LogisticsTrainTimeFloor, 1.f);
}

float Game::miningIncomeMultiplier(int team) const
{
    return 1.f + static_cast<float>(perkLevel(team, perk::Mining)) * config::MiningIncomeBonus;
}

int Game::defenseTowerRange(int team) const
{
    (void)team;
    return config::DefenseTowerRange;
}

int Game::unitCost(int name) const
{
    if (const UnitDefinition* definition = findUnitDefinition(name)) {
        return definition->commandCost;
    }
    return 0;
}

bool Game::isUnitUnlocked(int team, int name) const
{
    const UnitDefinition* definition = findUnitDefinition(name);
    if (!definition) {
        return false;
    }

    const int economy = economyLevelForTeam(team);
    const int barracks = completedBuildingCount(team, building::Barracks);
    const int upgrade = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    if (barracks < definition->requiredBarracks) {
        return false;
    }
    if (definition->unlockByEconomyOrTech) {
        return economy >= definition->requiredEconomyLevel || upgrade >= definition->requiredTechLevel;
    }
    return economy >= definition->requiredEconomyLevel && upgrade >= definition->requiredTechLevel;
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

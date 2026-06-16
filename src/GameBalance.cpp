#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "UnitDefinition.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

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

int Game::unitCost(int name) const
{
    if (const UnitDefinition* definition = findUnitDefinition(name)) {
        return definition->commandCost;
    }
    return 0;
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

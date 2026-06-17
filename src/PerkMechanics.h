#pragma once

#include "GameTypes.h"

#include <array>

struct UnitMechanics
{
    int attackRangeBonus = 0;
    int attackRangeCap = 0;
    int additionalAttackTargets = 0;
    float additionalTargetDamageMultiplier = 0.f;
    bool ignoresShooterCounter = false;
    bool ignoresInfantryCounter = false;
    bool tauntsNearbyEnemies = false;
    float siegeDamageTakenMultiplier = 1.f;
    float buildingDamageMultiplier = 1.f;
};

struct TowerMechanics
{
    bool prioritizesSiege = false;
    int splashDamage = 0;
    int splashRadius = 0;
};

UnitMechanics unitMechanicsFor(int unitName, const std::array<int, perk::Count>& perkLevels);
TowerMechanics towerMechanicsFor(const std::array<int, perk::Count>& perkLevels);

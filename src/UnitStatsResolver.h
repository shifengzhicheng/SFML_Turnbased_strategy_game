#pragma once

#include "GameTypes.h"
#include "UnitDefinition.h"
#include "UnitUpgradeState.h"

#include <array>

struct TeamStatContext
{
    int techLevel = 0;
    const UnitMasteryState* mastery = nullptr;
    const std::array<int, perk::Count>* perks = nullptr;
};

struct UnitComputedStats
{
    int maxHealth = 0;
    int attackDamage = 0;
    int attackRange = 0;
    float damageMultiplier = 1.f;
    float healthMultiplier = 1.f;
    float attackCooldownMultiplier = 1.f;
    float buildingDamageMultiplier = 1.f;
};

UnitComputedStats resolveUnitStats(const UnitDefinition& definition, const TeamStatContext& context);


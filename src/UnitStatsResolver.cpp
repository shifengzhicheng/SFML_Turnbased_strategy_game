#include "UnitStatsResolver.h"

#include "Config.h"
#include "PerkMechanics.h"
#include "UnitUpgradeDefinition.h"

#include <algorithm>
#include <cmath>

UnitComputedStats resolveUnitStats(const UnitDefinition& definition, const TeamStatContext& context)
{
    const int masteryLevel = context.mastery != nullptr
        ? unitMasteryLevel(*context.mastery, definition.unitName)
        : 0;
    const float masteryBonus = unitMasteryStatMultiplier(definition.unitName, masteryLevel) - 1.f;
    const float techDamageBonus = static_cast<float>(std::max(0, context.techLevel)) * config::TechDamageBonus;
    const float techHealthBonus = static_cast<float>(std::max(0, context.techLevel)) * config::TechHealthBonus;
    const UnitMechanics mechanics = context.perks != nullptr
        ? unitMechanicsFor(definition.unitName, *context.perks)
        : UnitMechanics{};

    UnitComputedStats stats;
    // All numeric growth is additive against the original unit definition. This
    // keeps infinite mastery readable and avoids compounding already-mutated HP.
    stats.damageMultiplier = 1.f + techDamageBonus + masteryBonus;
    stats.healthMultiplier = 1.f + techHealthBonus + masteryBonus;
    stats.maxHealth = std::max(1, static_cast<int>(std::round(static_cast<float>(definition.maxHealth) * stats.healthMultiplier)));
    stats.attackDamage = std::max(1, static_cast<int>(std::round(static_cast<float>(definition.attackDamage) * stats.damageMultiplier)));
    stats.attackRange = std::max(1, definition.attackRange + mechanics.attackRangeBonus);
    if (mechanics.attackRangeCap > 0) {
        stats.attackRange = std::min(stats.attackRange, mechanics.attackRangeCap);
    }
    stats.attackCooldownMultiplier = 1.f;
    stats.buildingDamageMultiplier = mechanics.buildingDamageMultiplier;
    return stats;
}


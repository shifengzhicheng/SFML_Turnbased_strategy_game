#include "PerkMechanics.h"

#include "AllUnit.h"
#include "Config.h"

#include <algorithm>

UnitMechanics unitMechanicsFor(int unitName, const std::array<int, perk::Count>& perkLevels)
{
    UnitMechanics mechanics;
    switch (unitName) {
    case UName::INFANTARY: {
        const int drill = perkLevels[static_cast<std::size_t>(perk::Drill)];
        mechanics.ignoresShooterCounter = drill >= 2;
        mechanics.additionalAttackTargets = drill >= 3 ? 1 : 0;
        mechanics.additionalTargetDamageMultiplier = drill >= 5 ? 0.48f : 0.32f;
        break;
    }
    case UName::SHOOTER: {
        const int volley = perkLevels[static_cast<std::size_t>(perk::Volley)];
        mechanics.additionalAttackTargets = volley >= 5 ? 2 : (volley >= 1 ? 1 : 0);
        mechanics.additionalTargetDamageMultiplier = volley >= 3 ? 0.65f : 0.45f;
        mechanics.attackRangeBonus = std::min(2, volley / 2);
        mechanics.attackRangeCap = config::ShooterMaxRange;
        break;
    }
    case UName::CAVALRY: {
        const int charge = perkLevels[static_cast<std::size_t>(perk::Charge)];
        mechanics.ignoresInfantryCounter = charge >= 2;
        break;
    }
    case UName::GUARDIAN: {
        const int fortitude = perkLevels[static_cast<std::size_t>(perk::Fortitude)];
        mechanics.tauntsNearbyEnemies = fortitude >= 1;
        mechanics.siegeDamageTakenMultiplier = fortitude >= 3 ? 0.62f : (fortitude >= 2 ? 0.75f : 1.f);
        mechanics.additionalAttackTargets = fortitude >= 3 ? 2 : 1;
        mechanics.additionalTargetDamageMultiplier = fortitude >= 4 ? 0.42f : 0.28f;
        break;
    }
    case UName::SIEGE: {
        const int siegeCraft = perkLevels[static_cast<std::size_t>(perk::SiegeCraft)];
        mechanics.buildingDamageMultiplier = siegeCraft >= 2 ? 1.20f : 1.f;
        mechanics.additionalAttackTargets = siegeCraft >= 1 ? 2 : 1;
        mechanics.additionalTargetDamageMultiplier = siegeCraft >= 4 ? 0.50f : (siegeCraft >= 1 ? 0.38f : 0.24f);
        break;
    }
    default:
        break;
    }
    return mechanics;
}

TowerMechanics towerMechanicsFor(const std::array<int, perk::Count>& perkLevels)
{
    TowerMechanics mechanics;
    const int towerCraft = perkLevels[static_cast<std::size_t>(perk::TowerCraft)];
    mechanics.prioritizesSiege = towerCraft >= 1;
    mechanics.splashDamage = config::DefenseTowerSplashDamage + towerCraft * 2;
    mechanics.splashRadius = config::DefenseTowerSplashRadius;
    return mechanics;
}

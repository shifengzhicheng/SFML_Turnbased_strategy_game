#include "UnitDefinition.h"

#include "AllUnit.h"
#include "Config.h"
#include "RealtimeConfig.h"

#include <array>
#include <stdexcept>

namespace
{
    const std::array<UnitDefinition, 5> Definitions = {{
        {UName::INFANTARY, "Infantry", config::InfantryHealth, config::InfantryDamage, config::InfantryRange,
         config::InfantryCost, realtime::InfantryStepSeconds, realtime::InfantryAttackCooldown,
         realtime::InfantryTrainSeconds, 1, 0, 0, false, art::UnitKind::Infantry},
        {UName::SHOOTER, "Shooter", config::ShooterHealth, config::ShooterDamage, config::ShooterRange,
         config::ShooterCost, realtime::ShooterStepSeconds, realtime::ShooterAttackCooldown,
         realtime::ShooterTrainSeconds, 1, 1, 1, true, art::UnitKind::Shooter},
        {UName::CAVALRY, "Cavalry", config::CavalryHealth, config::CavalryDamage, config::CavalryRange,
         config::CavalryCost, realtime::CavalryStepSeconds, realtime::CavalryAttackCooldown,
         realtime::CavalryTrainSeconds, 2, 2, 3, true, art::UnitKind::Cavalry},
        {UName::SIEGE, "Siege", config::SiegeHealth, config::SiegeDamage, config::SiegeRange,
         config::SiegeCost, realtime::SiegeStepSeconds, realtime::SiegeAttackCooldown,
         realtime::SiegeTrainSeconds, 2, 3, 5, false, art::UnitKind::Siege},
        {UName::GUARDIAN, "Guardian", config::GuardianHealth, config::GuardianDamage, config::GuardianRange,
         config::GuardianCost, realtime::GuardianStepSeconds, realtime::GuardianAttackCooldown,
         realtime::GuardianTrainSeconds, 3, 4, 7, false, art::UnitKind::Guardian},
    }};
}

const std::array<UnitDefinition, 5>& unitDefinitions()
{
    return Definitions;
}

const UnitDefinition* findUnitDefinition(int unitName)
{
    for (const auto& definition : Definitions) {
        if (definition.unitName == unitName) {
            return &definition;
        }
    }
    return nullptr;
}

const UnitDefinition& unitDefinition(int unitName)
{
    if (const UnitDefinition* definition = findUnitDefinition(unitName)) {
        return *definition;
    }
    throw std::out_of_range("unknown unit definition");
}

#include "CombatBehavior.h"

#include "AllUnit.h"
#include "RealtimeConfig.h"

#include <array>

namespace
{
    const std::array<CombatBehaviorDefinition, 5> Definitions = {{
        {UName::INFANTARY, CombatRole::Frontline, 1, false, false, 0.f},
        {UName::SHOOTER, CombatRole::Marksman, 3, true, false, 0.f},
        {UName::CAVALRY, CombatRole::Raider, 1, false, false, 0.f},
        {UName::SIEGE, CombatRole::Artillery, 5, false, true, realtime::SiegeDeploymentSeconds},
        {UName::GUARDIAN, CombatRole::Guardian, 1, false, false, 0.f},
    }};
}

const CombatBehaviorDefinition& combatBehavior(int unitName)
{
    for (const auto& definition : Definitions) {
        if (definition.unitName == unitName) {
            return definition;
        }
    }
    return Definitions.front();
}

int combatTargetBias(int attackerUnitName, int targetUnitName)
{
    if (targetUnitName == UName::BASE) {
        return attackerUnitName == UName::SIEGE ? -20 : 90;
    }

    switch (attackerUnitName) {
    case UName::INFANTARY:
        return targetUnitName == UName::CAVALRY ? -55 : 0;
    case UName::SHOOTER:
        return targetUnitName == UName::INFANTARY ? -45 : (targetUnitName == UName::GUARDIAN ? 35 : 0);
    case UName::CAVALRY:
        if (targetUnitName == UName::SIEGE) return -95;
        if (targetUnitName == UName::SHOOTER) return -85;
        if (targetUnitName == UName::GUARDIAN) return 65;
        if (targetUnitName == UName::INFANTARY) return 25;
        return 0;
    case UName::SIEGE:
        return targetUnitName == UName::GUARDIAN ? 70 : 20;
    case UName::GUARDIAN:
        return targetUnitName == UName::CAVALRY ? -60 : 0;
    default:
        return 0;
    }
}

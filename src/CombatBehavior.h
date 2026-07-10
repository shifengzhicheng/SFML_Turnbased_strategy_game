#pragma once

enum class CombatRole
{
    Frontline,
    Marksman,
    Raider,
    Artillery,
    Guardian
};

struct CombatBehaviorDefinition
{
    int unitName = 0;
    CombatRole role = CombatRole::Frontline;
    int preferredRange = 1;
    bool kitesAtCloseRange = false;
    bool prioritizesStructures = false;
    float deploymentSeconds = 0.f;
};

const CombatBehaviorDefinition& combatBehavior(int unitName);
int combatTargetBias(int attackerUnitName, int targetUnitName);

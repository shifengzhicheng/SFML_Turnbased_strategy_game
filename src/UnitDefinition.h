#pragma once

#include "ArtAssets.h"

#include <array>

struct UnitDefinition
{
    int unitName;
    const char* debugName;
    int maxHealth;
    int attackDamage;
    int attackRange;
    int commandCost;
    float moveStepSeconds;
    float attackCooldownSeconds;
    float trainSeconds;
    int requiredBarracks;
    int requiredEconomyLevel;
    int requiredTechLevel;
    bool unlockByEconomyOrTech;
    art::UnitKind artKind;
};

const std::array<UnitDefinition, 5>& unitDefinitions();
const UnitDefinition* findUnitDefinition(int unitName);
const UnitDefinition& unitDefinition(int unitName);

#pragma once

#include "UnitUpgradeState.h"

struct UnitMasteryDefinition
{
    int unitName;
    float statBonusPerLevel;
    float baseCostMultiplier;
    float costGrowth;
};

const UnitMasteryDefinition& unitMasteryDefinition(int unitName);
int unitMasteryLevel(const UnitMasteryState& state, int unitName);
void setUnitMasteryLevel(UnitMasteryState& state, int unitName, int level);
int unitMasteryUpgradeCost(int unitName, int currentLevel);
float unitMasteryStatMultiplier(int unitName, int level);

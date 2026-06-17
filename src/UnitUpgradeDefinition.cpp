#include "UnitUpgradeDefinition.h"

#include "AllUnit.h"
#include "Config.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace
{
    const std::array<UnitMasteryDefinition, TrainableUnitCount> Definitions = {{
        {UName::INFANTARY, config::MasteryStatBonusPerLevel, 3.4f, 1.140f},
        {UName::SHOOTER, config::MasteryStatBonusPerLevel, 3.5f, 1.145f},
        {UName::CAVALRY, config::MasteryStatBonusPerLevel, 3.8f, 1.150f},
        {UName::SIEGE, config::MasteryStatBonusPerLevel, 4.0f, 1.155f},
        {UName::GUARDIAN, config::MasteryStatBonusPerLevel, 4.1f, 1.155f},
    }};
}

const UnitMasteryDefinition& unitMasteryDefinition(int unitName)
{
    for (const auto& definition : Definitions) {
        if (definition.unitName == unitName) {
            return definition;
        }
    }
    throw std::out_of_range("unknown unit mastery definition");
}

int unitMasteryLevel(const UnitMasteryState& state, int unitName)
{
    return state.levels[trainableUnitIndex(unitName)];
}

void setUnitMasteryLevel(UnitMasteryState& state, int unitName, int level)
{
    state.levels[trainableUnitIndex(unitName)] = std::max(0, level);
}

int unitMasteryUpgradeCost(int unitName, int currentLevel)
{
    const UnitDefinition& unit = unitDefinition(unitName);
    const UnitMasteryDefinition& mastery = unitMasteryDefinition(unitName);
    const int safeLevel = std::max(0, currentLevel);
    const float base = std::max(55.f, static_cast<float>(unit.commandCost) * mastery.baseCostMultiplier);
    const float scaling = std::pow(mastery.costGrowth, static_cast<float>(safeLevel));
    const float levelTax = static_cast<float>(safeLevel * unit.commandCost) * 0.75f;
    return std::max(35, static_cast<int>(std::round(base * scaling + levelTax)));
}

float unitMasteryStatMultiplier(int unitName, int level)
{
    return 1.f + static_cast<float>(std::max(0, level)) * unitMasteryDefinition(unitName).statBonusPerLevel;
}

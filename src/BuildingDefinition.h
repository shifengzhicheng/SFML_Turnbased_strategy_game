#pragma once

#include "Building.h"

#include <array>

struct BuildingDefinition
{
    int type;
    const char* debugName;
    int commandCost;
    int maxHealth;
    float buildSeconds;
};

const std::array<BuildingDefinition, 2>& buildingDefinitions();
const BuildingDefinition* findBuildingDefinition(int type);
const BuildingDefinition& buildingDefinition(int type);

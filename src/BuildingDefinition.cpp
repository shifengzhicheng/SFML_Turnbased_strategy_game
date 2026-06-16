#include "BuildingDefinition.h"

#include "Config.h"
#include "RealtimeConfig.h"

#include <array>
#include <stdexcept>

namespace
{
    const std::array<BuildingDefinition, 2> Definitions = {{
        {building::Barracks, "Barracks", config::BarracksCost, config::BarracksHealth,
         realtime::BarracksBuildSeconds},
        {building::DefenseTower, "Tower", config::TowerCost, config::DefenseTowerHealth,
         realtime::DefenseTowerBuildSeconds},
    }};
}

const std::array<BuildingDefinition, 2>& buildingDefinitions()
{
    return Definitions;
}

const BuildingDefinition* findBuildingDefinition(int type)
{
    for (const auto& definition : Definitions) {
        if (definition.type == type) {
            return &definition;
        }
    }
    return nullptr;
}

const BuildingDefinition& buildingDefinition(int type)
{
    if (const BuildingDefinition* definition = findBuildingDefinition(type)) {
        return *definition;
    }
    throw std::out_of_range("unknown building definition");
}

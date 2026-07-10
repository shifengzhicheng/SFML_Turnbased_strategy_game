#pragma once

#include "BuildingDefinition.h"
#include "Game.h"
#include "RealtimeConfig.h"
#include "Tile.h"
#include "UnitDefinition.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <memory>

namespace game_internal
{
    inline bool isKnownTeam(int team)
    {
        return team == PLAYER || team == AI;
    }

    inline int& commandPool(Game& game, int team)
    {
        assert(isKnownTeam(team));
        return team == PLAYER ? game.playerCommand : game.aiCommand;
    }

    inline int countUnitsNamed(const std::list<std::unique_ptr<MoveableUnit>>& units, int name)
    {
        return static_cast<int>(std::count_if(units.begin(), units.end(), [name](const std::unique_ptr<MoveableUnit>& unit) {
            return unit->unitName == name;
        }));
    }

    inline tile::ID buildingTileId(int team, int type)
    {
        assert(isKnownTeam(team));
        if (type == building::DefenseTower) {
            return team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower;
        }
        return team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks;
    }

    inline float buildingSeconds(int type)
    {
        if (const BuildingDefinition* definition = findBuildingDefinition(type)) {
            return definition->buildSeconds;
        }
        return realtime::BarracksBuildSeconds;
    }

    inline int buildingMaxHealth(int type)
    {
        if (const BuildingDefinition* definition = findBuildingDefinition(type)) {
            return definition->maxHealth;
        }
        return config::BarracksHealth;
    }

    inline int buildingCommandCost(int type)
    {
        if (const BuildingDefinition* definition = findBuildingDefinition(type)) {
            return definition->commandCost;
        }
        return 0;
    }

    inline float unitTrainSeconds(int name)
    {
        if (const UnitDefinition* definition = findUnitDefinition(name)) {
            return definition->trainSeconds;
        }
        return realtime::InfantryTrainSeconds;
    }

}

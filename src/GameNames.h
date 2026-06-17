#pragma once

#include "AllUnit.h"
#include "BuildingDefinition.h"
#include "GameTypes.h"
#include "RealtimeConfig.h"
#include "UnitDefinition.h"

namespace game_internal
{
    inline const char* perkTitle(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Iron Wall";
        case perk::Volley:
            return "Volley Drill";
        case perk::Charge:
            return "Shock Charge";
        case perk::SiegeCraft:
            return "Siege Craft";
        case perk::TowerCraft:
            return "Watchtowers";
        case perk::Logistics:
            return "Logistics";
        case perk::Mining:
            return "Supply Crew";
        case perk::WarChest:
            return "War Chest";
        case perk::Drill:
        default:
            return "Blade Drill";
        }
    }

    inline const char* perkDescription(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Guardians taunt nearby enemies.\nHigher levels harden them versus siege.";
        case perk::Volley:
            return "Shooters chain shots.\nLv2/Lv4 extend range, capped at 5.";
        case perk::Charge:
            return "Cavalry gains shock tactics.\nLv2 ignores Infantry counter.";
        case perk::SiegeCraft:
            return "Siege splash chips escorts.\nHigher levels improve breach pressure.";
        case perk::TowerCraft:
            return "Towers focus siege threats.\nShots deal fixed splash damage.";
        case perk::Logistics:
            return "Barracks train 12% faster.\nTurns economy into tempo.";
        case perk::Mining:
            return "Natural CMD +18%.\nEconomy upgrades compound hard.";
        case perk::WarChest:
            return "Instant 80+12/LEVEL CMD.\nBuild or queue now.";
        case perk::Drill:
        default:
            return "Infantry gains shield drill.\nLv2 resists shooters, Lv3 cleaves.";
        }
    }

    inline int maxPerkLevel(int type)
    {
        if (type == perk::WarChest) {
            return 99;
        }
        if (type == perk::TowerCraft || type == perk::SiegeCraft) {
            return 4;
        }
        return 5;
    }

    inline const char* laneName(int laneIndex)
    {
        switch (laneIndex) {
        case lane::Top:
            return "Top";
        case lane::Bot:
            return "Bot";
        case lane::Mid:
        default:
            return "Mid";
        }
    }

    inline const char* operationTypeName(int type)
    {
        switch (type) {
        case gameop::UpgradeEconomy:
            return "UpgradeEconomy";
        case gameop::UpgradeTech:
            return "UpgradeTech";
        case gameop::BuildBarracks:
            return "BuildBarracks";
        case gameop::BuildTower:
            return "BuildTower";
        case gameop::QueueUnit:
            return "QueueUnit";
        case gameop::UpgradeUnitMastery:
            return "UpgradeUnitMastery";
        case gameop::SelectLane:
        default:
            return "SelectLane";
        }
    }

    inline const char* unitDebugName(int name)
    {
        if (const UnitDefinition* definition = findUnitDefinition(name)) {
            return definition->debugName;
        }
        return name == UName::BASE ? "Base" : "Unknown";
    }

    inline const char* perkShortName(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Wall";
        case perk::Volley:
            return "Volley";
        case perk::Charge:
            return "Charge";
        case perk::SiegeCraft:
            return "Siege";
        case perk::TowerCraft:
            return "Tower";
        case perk::Logistics:
            return "Logi";
        case perk::Mining:
            return "Eco";
        case perk::WarChest:
            return "Chest";
        case perk::Drill:
        default:
            return "Blade";
        }
    }

    inline const char* buildingName(int type)
    {
        if (const BuildingDefinition* definition = findBuildingDefinition(type)) {
            return definition->debugName;
        }
        return "Unknown";
    }
}

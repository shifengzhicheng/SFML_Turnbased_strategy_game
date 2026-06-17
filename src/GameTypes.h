#pragma once

#include "AllUnit.h"
#include "Config.h"

#include <string>

enum gameSeceneState
{
    SCENE_START,
    SCENE_GAME,
    SCEN_GAMEOVER
};

enum gamePlayer
{
    PLAYER,
    AI
};

namespace perk
{
    enum Type
    {
        Drill,
        Fortitude,
        Volley,
        Charge,
        SiegeCraft,
        TowerCraft,
        Logistics,
        Mining,
        WarChest,
        Count
    };
}

namespace gameop
{
    enum Type
    {
        SelectLane,
        UpgradeEconomy,
        UpgradeTech,
        BuildBarracks,
        BuildTower,
        QueueUnit,
        UpgradeUnitMastery,
        Count
    };
}

struct GameOperation
{
    int type = gameop::SelectLane;
    int laneIndex = lane::Mid;
    int unitName = UName::INFANTARY;

    GameOperation(int operationType = gameop::SelectLane, int lane = lane::Mid, int unit = UName::INFANTARY)
        : type(operationType), laneIndex(lane), unitName(unit)
    {
    }
};

struct PerkChoice
{
    int type = perk::Drill;
    std::string title;
    std::string description;
};

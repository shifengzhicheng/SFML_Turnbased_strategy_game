#pragma once

namespace config
{
    inline constexpr int TileSize = 20;
    inline constexpr int MapWidth = 1120;
    inline constexpr int MapHeight = 720;
    inline constexpr int PanelWidth = 160;
    inline constexpr int WindowWidth = MapWidth + PanelWidth;
    inline constexpr int WindowHeight = MapHeight;
    inline constexpr int MaxUnits = 96;

    inline constexpr int PanelX = MapWidth;
    inline constexpr int PanelPadding = 12;
    inline constexpr int ButtonX = PanelX + 16;
    inline constexpr int HelpButtonY = 210;
    inline constexpr int EndTurnButtonY = 254;
    inline constexpr int BuildBarracksY = 306;
    inline constexpr int BuildInfantryY = 358;
    inline constexpr int BuildShooterY = 410;
    inline constexpr int BuildCavalryY = 462;
    inline constexpr int BuildSiegeY = 514;
    inline constexpr int BuildGuardianY = 566;
    inline constexpr int BuildTowerY = 626;
    inline constexpr int UnitTextureSize = 44;
    inline constexpr float UnitSpriteScale = 0.66f;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 72;
    inline constexpr int MaxCommand = 1600;
    inline constexpr int BaseCommandIncome = 4;
    inline constexpr int ResourceCommandIncome = 7;
    inline constexpr int ExtractorCost = 18;
    inline constexpr int BarracksCost = 56;
    inline constexpr int TowerCost = 48;
    inline constexpr int WorkerCost = 10;
    inline constexpr int BarracksBaseCap = 1;
    inline constexpr int BarracksCap = 7;
    inline constexpr int TowerBaseCap = 2;
    inline constexpr int TowerCap = 7;
    inline constexpr int MaxTechLevel = 15;
    inline constexpr int UpgradeCost = 60;
    inline constexpr int UpgradeCostStep = 42;
    inline constexpr float TechDamageBonus = 0.025f;
    inline constexpr float TechHealthBonus = 0.015f;
    inline constexpr float BuildingDamageFactor = 0.62f;
    inline constexpr int ExtractorHealth = 460;
    inline constexpr int BarracksHealth = 1220;
    inline constexpr int DefenseTowerHealth = 980;
    inline constexpr int DefenseTowerDamage = 50;
    inline constexpr int DefenseTowerRange = 8;
    inline constexpr int BaseDefenseDamage = 34;
    inline constexpr int BaseDefenseRange = 8;
    inline constexpr int ResourceCaptureRadius = 4;
    inline constexpr float ResourceCaptureSeconds = 13.5f;
    inline constexpr int BuildInfluenceRadius = 13;
    inline constexpr int InfantryCost = 15;
    inline constexpr int ShooterCost = 22;
    inline constexpr int CavalryCost = 38;
    inline constexpr int SiegeCost = 56;
    inline constexpr int GuardianCost = 74;
    inline constexpr double Pi = 3.14159265358979323846;
}

namespace lane
{
    enum Type
    {
        Top,
        Mid,
        Bot,
        Count
    };
}

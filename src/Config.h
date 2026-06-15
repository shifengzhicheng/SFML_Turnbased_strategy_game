#pragma once

namespace config
{
    inline constexpr int TileSize = 20;
    inline constexpr int MapWidth = 1120;
    inline constexpr int MapHeight = 720;
    inline constexpr int PanelWidth = 160;
    inline constexpr int WindowWidth = MapWidth + PanelWidth;
    inline constexpr int WindowHeight = MapHeight;
    inline constexpr int MaxUnits = 72;

    inline constexpr int PanelX = MapWidth;
    inline constexpr int PanelPadding = 12;
    inline constexpr int ButtonX = PanelX + 16;
    inline constexpr int HelpButtonY = 246;
    inline constexpr int EndTurnButtonY = 292;
    inline constexpr int BuildInfantryY = 334;
    inline constexpr int BuildShooterY = 386;
    inline constexpr int BuildCavalryY = 438;
    inline constexpr int BuildSiegeY = 490;
    inline constexpr int BuildGuardianY = 542;
    inline constexpr int BuildTowerY = 616;
    inline constexpr int UnitTextureSize = 44;
    inline constexpr float UnitSpriteScale = 0.66f;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 60;
    inline constexpr int MaxCommand = 300;
    inline constexpr int BaseCommandIncome = 3;
    inline constexpr int ResourceCommandIncome = 7;
    inline constexpr int ExtractorCost = 18;
    inline constexpr int BarracksCost = 42;
    inline constexpr int TowerCost = 30;
    inline constexpr int WorkerCost = 10;
    inline constexpr int BarracksBaseCap = 1;
    inline constexpr int BarracksCap = 4;
    inline constexpr int TowerBaseCap = 2;
    inline constexpr int TowerCap = 5;
    inline constexpr int MaxTechLevel = 4;
    inline constexpr int UpgradeCost = 60;
    inline constexpr int UpgradeCostStep = 42;
    inline constexpr float TechDamageBonus = 0.16f;
    inline constexpr float BuildingDamageFactor = 0.62f;
    inline constexpr int ExtractorHealth = 460;
    inline constexpr int BarracksHealth = 1020;
    inline constexpr int DefenseTowerHealth = 820;
    inline constexpr int DefenseTowerDamage = 30;
    inline constexpr int DefenseTowerRange = 7;
    inline constexpr int ResourceCaptureRadius = 4;
    inline constexpr float ResourceCaptureSeconds = 8.5f;
    inline constexpr int BuildInfluenceRadius = 10;
    inline constexpr int InfantryCost = 12;
    inline constexpr int ShooterCost = 18;
    inline constexpr int CavalryCost = 30;
    inline constexpr int SiegeCost = 44;
    inline constexpr int GuardianCost = 58;
    inline constexpr double Pi = 3.14159265358979323846;
}

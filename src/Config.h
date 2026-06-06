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
    inline constexpr int HelpButtonY = 252;
    inline constexpr int EndTurnButtonY = 300;
    inline constexpr int BuildInfantryY = 346;
    inline constexpr int BuildShooterY = 428;
    inline constexpr int BuildCavalryY = 510;
    inline constexpr int BuildTowerY = 606;
    inline constexpr int UnitTextureSize = 44;
    inline constexpr float UnitSpriteScale = 0.66f;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 52;
    inline constexpr int MaxCommand = 260;
    inline constexpr int BaseCommandIncome = 3;
    inline constexpr int ResourceCommandIncome = 7;
    inline constexpr int ExtractorCost = 16;
    inline constexpr int BarracksCost = 34;
    inline constexpr int TowerCost = 24;
    inline constexpr int BarracksCap = 4;
    inline constexpr int TowerCap = 5;
    inline constexpr int MaxTechLevel = 4;
    inline constexpr int UpgradeCost = 45;
    inline constexpr int UpgradeCostStep = 30;
    inline constexpr float TechDamageBonus = 0.16f;
    inline constexpr float BuildingDamageFactor = 0.70f;
    inline constexpr int ExtractorHealth = 420;
    inline constexpr int BarracksHealth = 900;
    inline constexpr int DefenseTowerHealth = 720;
    inline constexpr int DefenseTowerDamage = 34;
    inline constexpr int DefenseTowerRange = 7;
    inline constexpr int ResourceCaptureRadius = 4;
    inline constexpr float ResourceCaptureSeconds = 7.0f;
    inline constexpr int InfantryCost = 9;
    inline constexpr int ShooterCost = 14;
    inline constexpr int CavalryCost = 22;
    inline constexpr double Pi = 3.14159265358979323846;
}

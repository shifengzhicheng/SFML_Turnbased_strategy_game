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
    inline constexpr int EndTurnButtonY = 300;
    inline constexpr int BuildInfantryY = 346;
    inline constexpr int BuildShooterY = 428;
    inline constexpr int BuildCavalryY = 510;
    inline constexpr int UnitTextureSize = 44;
    inline constexpr float UnitSpriteScale = 0.66f;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 26;
    inline constexpr int MaxCommand = 160;
    inline constexpr int BaseCommandIncome = 2;
    inline constexpr int ResourceCommandIncome = 5;
    inline constexpr int ExtractorCost = 6;
    inline constexpr int BarracksCost = 10;
    inline constexpr int UpgradeCost = 18;
    inline constexpr int InfantryCost = 3;
    inline constexpr int ShooterCost = 4;
    inline constexpr int CavalryCost = 6;
    inline constexpr double Pi = 3.14159265358979323846;
}

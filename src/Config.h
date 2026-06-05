#pragma once

namespace config
{
    inline constexpr int TileSize = 20;
    inline constexpr int MapWidth = 1120;
    inline constexpr int MapHeight = 720;
    inline constexpr int PanelWidth = 160;
    inline constexpr int WindowWidth = MapWidth + PanelWidth;
    inline constexpr int WindowHeight = MapHeight;
    inline constexpr int MaxUnits = 10;

    inline constexpr int PanelX = MapWidth;
    inline constexpr int PanelPadding = 12;
    inline constexpr int ButtonX = PanelX + 30;
    inline constexpr int EndTurnButtonY = 300;
    inline constexpr int BuildInfantryY = 350;
    inline constexpr int BuildShooterY = 420;
    inline constexpr int BuildCavalryY = 490;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr double Pi = 3.14159265358979323846;
}

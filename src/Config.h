#pragma once

namespace config
{
    inline constexpr int TileSize = 20;
    inline constexpr int MapWidth = 1120;
    inline constexpr int MapHeight = 720;
    inline constexpr int PanelWidth = 220;
    inline constexpr int WindowWidth = MapWidth + PanelWidth;
    inline constexpr int WindowHeight = MapHeight;
    inline constexpr int MaxUnits = 96;

    inline constexpr int PanelX = MapWidth;
    inline constexpr int PanelPadding = 14;
    inline constexpr int ButtonX = PanelX + 18;
    inline constexpr int EconomyButtonY = 226;
    inline constexpr int EndTurnButtonY = 276;
    inline constexpr int HelpButtonY = 676;
    inline constexpr int BuildBarracksY = 334;
    inline constexpr int BuildInfantryY = 380;
    inline constexpr int BuildShooterY = 426;
    inline constexpr int BuildCavalryY = 472;
    inline constexpr int BuildSiegeY = 518;
    inline constexpr int BuildGuardianY = 564;
    inline constexpr int BuildTowerY = 610;
    inline constexpr int SideButtonWidth = PanelWidth - 36;
    inline constexpr int SideButtonHeight = 38;
    inline constexpr int UnitTextureSize = 44;
    inline constexpr float UnitSpriteScale = 0.66f;
    inline constexpr int BaseFootprintSize = 2;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 72;
    inline constexpr int MaxCommand = 1600;
    inline constexpr int BaseCommandIncome = 5;
    inline constexpr int EconomyIncomeStep = 3;
    inline constexpr int EconomyIncomeQuadraticDivisor = 4;
    inline constexpr int MaxEconomyLevel = 12;
    inline constexpr int EconomyUpgradeCost = 42;
    inline constexpr int EconomyUpgradeCostStep = 18;
    inline constexpr int KillBountyPercent = 45;
    inline constexpr int KillBountyMin = 4;
    inline constexpr int BarracksCost = 56;
    inline constexpr int TowerCost = 48;
    inline constexpr int StructureSalvagePercent = 42;
    inline constexpr int LastBarracksReliefBonus = 18;
    inline constexpr float EmergencyShieldSeconds = 18.0f;
    inline constexpr float EmergencyShieldDamageMultiplier = 0.58f;
    inline constexpr int EmergencyBarracksRepair = 260;
    inline constexpr int EmergencyTowerRepair = 170;
    inline constexpr int BarracksBaseCap = 1;
    inline constexpr int BarracksCap = 7;
    inline constexpr int TowerBaseCap = 1;
    inline constexpr int TowerCap = 4;
    inline constexpr int MaxTechLevel = 15;
    inline constexpr float TechDamageBonus = 0.035f;
    inline constexpr float TechHealthBonus = 0.025f;
    inline constexpr float DrillInfantryDamageBonus = 0.14f;
    inline constexpr float DrillGuardianDamageBonus = 0.10f;
    inline constexpr float FortitudeHealthBonus = 0.14f;
    inline constexpr float VolleyDamageBonus = 0.13f;
    inline constexpr float VolleyCooldownReduction = 0.055f;
    inline constexpr float ChargeDamageBonus = 0.14f;
    inline constexpr float ChargeHealthBonus = 0.08f;
    inline constexpr float ChargeCooldownReduction = 0.035f;
    inline constexpr float SiegeDamageBonus = 0.15f;
    inline constexpr float SiegeHealthBonus = 0.08f;
    inline constexpr float SiegeBuildingDamageBonus = 0.15f;
    inline constexpr float TowerDamageBonus = 0.12f;
    inline constexpr float LogisticsTrainTimeReduction = 0.09f;
    inline constexpr float LogisticsTrainTimeFloor = 0.60f;
    inline constexpr float MiningIncomeBonus = 0.12f;
    inline constexpr float AttackCooldownFloor = 0.66f;
    inline constexpr int WarChestBaseBonus = 60;
    inline constexpr int WarChestTechBonus = 8;
    inline constexpr float BuildingDamageFactor = 0.62f;
    inline constexpr int BarracksHealth = 1220;
    inline constexpr int DefenseTowerHealth = 980;
    inline constexpr int DefenseTowerDamage = 44;
    inline constexpr int DefenseTowerRange = 7;
    inline constexpr int BaseDefenseDamage = 34;
    inline constexpr int BaseDefenseRange = 8;
    inline constexpr int BuildInfluenceRadius = 13;
    inline constexpr int InfantryCost = 15;
    inline constexpr int ShooterCost = 22;
    inline constexpr int CavalryCost = 38;
    inline constexpr int SiegeCost = 56;
    inline constexpr int GuardianCost = 74;
    inline constexpr int InfantryHealth = 220;
    inline constexpr int ShooterHealth = 100;
    inline constexpr int CavalryHealth = 420;
    inline constexpr int SiegeHealth = 270;
    inline constexpr int GuardianHealth = 720;
    inline constexpr int InfantryDamage = 48;
    inline constexpr int ShooterDamage = 28;
    inline constexpr int CavalryDamage = 68;
    inline constexpr int SiegeDamage = 56;
    inline constexpr int GuardianDamage = 88;
    inline constexpr int InfantryRange = 2;
    inline constexpr int ShooterRange = 5;
    inline constexpr int CavalryRange = 2;
    inline constexpr int SiegeRange = 9;
    inline constexpr int GuardianRange = 2;
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

#pragma once

namespace config
{
    inline constexpr int TileSize = 24;
    inline constexpr int MapTilesX = 47;
    inline constexpr int MapTilesY = 30;
    inline constexpr int MapWidth = TileSize * MapTilesX;
    inline constexpr int MapHeight = TileSize * MapTilesY;
    inline constexpr int PanelWidth = 260;
    inline constexpr int WindowWidth = MapWidth + PanelWidth;
    inline constexpr int WindowHeight = MapHeight;
    inline constexpr int MaxUnits = 96;

    inline constexpr int PanelX = MapWidth;
    inline constexpr int PanelPadding = 14;
    inline constexpr int ButtonX = PanelX + 18;
    inline constexpr int EconomyButtonY = 250;
    inline constexpr int EndTurnButtonY = 294;
    inline constexpr int HelpButtonY = 680;
    inline constexpr int BuildBarracksY = 374;
    inline constexpr int BuildInfantryY = 416;
    inline constexpr int BuildShooterY = 458;
    inline constexpr int BuildCavalryY = 500;
    inline constexpr int BuildSiegeY = 542;
    inline constexpr int BuildGuardianY = 584;
    inline constexpr int BuildTowerY = 626;
    inline constexpr int SideButtonWidth = PanelWidth - 36;
    inline constexpr int SideButtonHeight = 38;
    inline constexpr int MasteryButtonWidth = 74;
    inline constexpr int MasteryButtonHeight = 32;
    inline constexpr int MasteryButtonInsetY = 3;
    inline constexpr int UnitButtonWidth = SideButtonWidth - MasteryButtonWidth - 8;
    inline constexpr int MasteryButtonX = ButtonX + UnitButtonWidth + 8;
    inline constexpr int UnitTextureSize = 96;
    inline constexpr float UnitSpriteScale = 0.86f;
    inline constexpr int BaseFootprintSize = 2;

    inline constexpr int PathStraightCost = 10;
    inline constexpr int PathDiagonalCost = 14;
    inline constexpr int StartingCommand = 72;
    inline constexpr int MaxCommand = 50000;
    inline constexpr int CommandFeatureScale = 1600;
    inline constexpr int BaseCommandIncome = 5;
    inline constexpr int EconomyIncomeStep = 3;
    inline constexpr int EconomyIncomeQuadraticDivisor = 6;
    inline constexpr int MaxEconomyLevel = 12;
    inline constexpr int EconomyUpgradeCost = 46;
    inline constexpr int EconomyUpgradeCostStep = 21;
    inline constexpr int KillBountyPercent = 45;
    inline constexpr int KillBountyMin = 4;
    inline constexpr int BarracksCost = 56;
    inline constexpr int TowerCost = 48;
    inline constexpr int StructureSalvagePercent = 42;
    inline constexpr int LastBarracksReliefBonus = 18;
    inline constexpr float LaneRebuildLockSeconds = 30.0f;
    inline constexpr int ComebackReliefCharges = 2;
    inline constexpr float EmergencyShieldSeconds = 12.0f;
    inline constexpr float EmergencyShieldDamageMultiplier = 0.62f;
    inline constexpr int EmergencyBarracksRepair = 160;
    inline constexpr int EmergencyTowerRepair = 80;
    inline constexpr int BarracksBaseCap = 1;
    inline constexpr int BarracksCap = 7;
    inline constexpr int TowerBaseCap = 2;
    inline constexpr int TowerCap = 8;
    inline constexpr int MaxTechLevel = 15;
    inline constexpr float TechDamageBonus = 0.06f;
    inline constexpr float TechHealthBonus = 0.045f;
    inline constexpr float MasteryStatBonusPerLevel = 0.10f;
    inline constexpr float LogisticsTrainTimeReduction = 0.12f;
    inline constexpr float LogisticsTrainTimeFloor = 0.48f;
    inline constexpr float MiningIncomeBonus = 0.18f;
    inline constexpr float AttackCooldownFloor = 0.50f;
    inline constexpr float AggroMemorySeconds = 5.0f;
    inline constexpr float ArmyWaveIntervalSeconds = 18.0f;
    inline constexpr int RewardRerollsPerChoice = 1;
    // Tiles scanned for nearby enemies while units are still following lane
    // rally points. Keep these above attack range so armies peel before melee.
    inline constexpr int InfantrySeekRange = 7;
    inline constexpr int ShooterSeekRange = 9;
    inline constexpr int CavalrySeekRange = 10;
    inline constexpr int SiegeSeekRange = 12;
    inline constexpr int GuardianSeekRange = 8;
    inline constexpr int WarChestBaseBonus = 80;
    inline constexpr int WarChestTechBonus = 12;
    inline constexpr float BuildingDamageFactor = 0.62f;
    inline constexpr int BarracksHealth = 1220;
    inline constexpr int DefenseTowerHealth = 980;
    inline constexpr int DefenseTowerDamage = 24;
    inline constexpr int DefenseTowerRange = 7;
    inline constexpr int DefenseTowerSplashDamage = 10;
    inline constexpr int DefenseTowerSplashRadius = 2;
    inline constexpr int TowerPlacementMinSpacing = 2;
    inline constexpr int TowerPlacementPreferredSpacing = 3;
    inline constexpr int BaseDefenseDamage = 34;
    inline constexpr int BaseDefenseRange = 8;
    inline constexpr int BaseHealth = 3400;
    inline constexpr float BaseProtectionEarlyEnd = 150.f;
    inline constexpr float BaseProtectionMidEnd = 300.f;
    inline constexpr float BaseProtectionLateEnd = 450.f;
    inline constexpr float BaseProtectionEarlyMultiplier = 0.45f;
    inline constexpr float BaseProtectionMidMultiplier = 0.68f;
    inline constexpr float BaseProtectionLateMultiplier = 0.86f;
    inline constexpr float EscalationStartSeconds = 450.f;
    inline constexpr float EscalationDamagePerMinute = 0.25f;
    inline constexpr float EscalationDamageCap = 3.00f;
    inline constexpr int OvertimeHQAssaultRadius = 9;
    inline constexpr int CommandZonePressureRadius = 11;
    inline constexpr int CommandZoneDamagePerUnit = 6;
    inline constexpr int CommandZoneDamageCap = 150;
    inline constexpr float TechOvertimeDiscountStart = 540.f;
    inline constexpr float TechOvertimeDiscountPerMinute = 0.10f;
    inline constexpr float TechOvertimeDiscountFloor = 0.55f;
    inline constexpr int BuildInfluenceRadius = 13;
    inline constexpr int InfantryCost = 30;
    inline constexpr int ShooterCost = 31;
    inline constexpr int CavalryCost = 58;
    inline constexpr int SiegeCost = 96;
    inline constexpr int GuardianCost = 120;
    inline constexpr int InfantryHealth = 145;
    inline constexpr int ShooterHealth = 105;
    inline constexpr int CavalryHealth = 350;
    inline constexpr int SiegeHealth = 250;
    inline constexpr int GuardianHealth = 760;
    inline constexpr int InfantryDamage = 29;
    inline constexpr int ShooterDamage = 26;
    inline constexpr int CavalryDamage = 58;
    inline constexpr int SiegeDamage = 50;
    inline constexpr int GuardianDamage = 78;
    inline constexpr int InfantryRange = 1;
    inline constexpr int ShooterRange = 3;
    inline constexpr int ShooterMaxRange = 5;
    inline constexpr int CavalryRange = 1;
    inline constexpr int SiegeRange = 5;
    inline constexpr int GuardianRange = 1;
    inline constexpr float CounterDamageMultiplier = 1.55f;
    inline constexpr float CounteredDamageMultiplier = 0.78f;
    inline constexpr float GuardianSiegeDamageMultiplier = 0.85f;
    inline constexpr int CavalryChargeTiles = 4;
    inline constexpr float CavalryBaseChargeDamageMultiplier = 1.15f;
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

#pragma once

namespace realtime
{
    inline constexpr bool Enabled = true;
    inline constexpr float EconomyTickSeconds = 3.1f;
    inline constexpr float AIThinkSeconds = 4.2f;
    inline constexpr int AIUnitsPerBurst = 1;
    inline constexpr int StartingWorkers = 2;
    inline constexpr int MaxWorkers = 7;
    inline constexpr float WorkerTrainSeconds = 5.5f;

    inline constexpr float PathRefreshSeconds = 0.90f;
    inline constexpr float WorkerPathRefreshSeconds = 0.60f;
    inline constexpr float WorkerStepSeconds = 0.28f;
    inline constexpr float InfantryStepSeconds = 0.38f;
    inline constexpr float ShooterStepSeconds = 0.44f;
    inline constexpr float CavalryStepSeconds = 0.30f;
    inline constexpr float SiegeStepSeconds = 0.52f;
    inline constexpr float GuardianStepSeconds = 0.48f;

    inline constexpr float InfantryAttackCooldown = 1.45f;
    inline constexpr float ShooterAttackCooldown = 1.70f;
    inline constexpr float CavalryAttackCooldown = 1.62f;
    inline constexpr float SiegeAttackCooldown = 2.35f;
    inline constexpr float GuardianAttackCooldown = 2.05f;
    inline constexpr float DefenseTowerAttackCooldown = 1.55f;

    inline constexpr float ExtractorBuildSeconds = 7.0f;
    inline constexpr float BarracksBuildSeconds = 12.0f;
    inline constexpr float DefenseTowerBuildSeconds = 8.5f;
    inline constexpr float InfantryTrainSeconds = 6.2f;
    inline constexpr float ShooterTrainSeconds = 7.5f;
    inline constexpr float CavalryTrainSeconds = 10.2f;
    inline constexpr float SiegeTrainSeconds = 13.5f;
    inline constexpr float GuardianTrainSeconds = 16.0f;
}

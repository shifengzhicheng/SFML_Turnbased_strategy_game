#pragma once

namespace realtime
{
    inline constexpr bool Enabled = true;
    inline constexpr float EconomyTickSeconds = 2.0f;
    inline constexpr float AIThinkSeconds = 1.2f;
    inline constexpr int AIUnitsPerBurst = 2;
    inline constexpr int StartingWorkers = 2;
    inline constexpr int MaxWorkers = 6;

    inline constexpr float PathRefreshSeconds = 0.55f;
    inline constexpr float WorkerPathRefreshSeconds = 0.45f;
    inline constexpr float WorkerStepSeconds = 0.18f;
    inline constexpr float InfantryStepSeconds = 0.22f;
    inline constexpr float ShooterStepSeconds = 0.28f;
    inline constexpr float CavalryStepSeconds = 0.16f;

    inline constexpr float InfantryAttackCooldown = 0.95f;
    inline constexpr float ShooterAttackCooldown = 1.15f;
    inline constexpr float CavalryAttackCooldown = 1.05f;

    inline constexpr float ExtractorBuildSeconds = 4.0f;
    inline constexpr float BarracksBuildSeconds = 7.0f;
    inline constexpr float InfantryTrainSeconds = 2.3f;
    inline constexpr float ShooterTrainSeconds = 3.0f;
    inline constexpr float CavalryTrainSeconds = 4.2f;
}

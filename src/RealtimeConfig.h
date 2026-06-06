#pragma once

namespace realtime
{
    inline constexpr bool Enabled = true;
    inline constexpr float EconomyTickSeconds = 2.6f;
    inline constexpr float AIThinkSeconds = 2.8f;
    inline constexpr int AIUnitsPerBurst = 1;
    inline constexpr int StartingWorkers = 2;
    inline constexpr int MaxWorkers = 6;

    inline constexpr float PathRefreshSeconds = 0.75f;
    inline constexpr float WorkerPathRefreshSeconds = 0.60f;
    inline constexpr float WorkerStepSeconds = 0.24f;
    inline constexpr float InfantryStepSeconds = 0.32f;
    inline constexpr float ShooterStepSeconds = 0.38f;
    inline constexpr float CavalryStepSeconds = 0.24f;

    inline constexpr float InfantryAttackCooldown = 1.30f;
    inline constexpr float ShooterAttackCooldown = 1.55f;
    inline constexpr float CavalryAttackCooldown = 1.45f;

    inline constexpr float ExtractorBuildSeconds = 5.5f;
    inline constexpr float BarracksBuildSeconds = 9.0f;
    inline constexpr float InfantryTrainSeconds = 3.6f;
    inline constexpr float ShooterTrainSeconds = 4.6f;
    inline constexpr float CavalryTrainSeconds = 6.2f;
}

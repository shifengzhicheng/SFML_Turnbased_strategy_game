#pragma once

namespace realtime
{
    inline constexpr bool Enabled = true;
    inline constexpr float SimulationStepSeconds = 1.f / 60.f;
    inline constexpr float MaxFrameAdvanceSeconds = 0.25f;
    inline constexpr int MaxSimulationStepsPerFrame = 20;
    inline constexpr float EconomyTickSeconds = 3.1f;
    inline constexpr float AIThinkSeconds = 5.0f;
    inline constexpr int AIUnitsPerBurst = 1;
    inline constexpr int StartingWorkers = 2;
    inline constexpr int MaxWorkers = 14;
    inline constexpr float PathRefreshSeconds = 0.90f;
    inline constexpr float WorkerPathRefreshSeconds = 0.60f;
    inline constexpr float WorkerStepSeconds = 0.34f;
    // Movement is measured as seconds per tile; larger values are slower.
    // Siege must need escorts, while cavalry should visibly rotate faster.
    inline constexpr float InfantryStepSeconds = 0.64f;
    inline constexpr float ShooterStepSeconds = 0.82f;
    inline constexpr float CavalryStepSeconds = 0.34f;
    inline constexpr float SiegeStepSeconds = 1.58f;
    inline constexpr float GuardianStepSeconds = 0.96f;

    static_assert(CavalryStepSeconds < InfantryStepSeconds);
    static_assert(InfantryStepSeconds < ShooterStepSeconds);
    static_assert(ShooterStepSeconds < GuardianStepSeconds);
    static_assert(GuardianStepSeconds < SiegeStepSeconds);

    inline constexpr float InfantryAttackCooldown = 1.50f;
    inline constexpr float ShooterAttackCooldown = 1.82f;
    inline constexpr float CavalryAttackCooldown = 1.82f;
    inline constexpr float SiegeAttackCooldown = 2.62f;
    inline constexpr float GuardianAttackCooldown = 2.20f;
    inline constexpr float DefenseTowerAttackCooldown = 1.55f;
    inline constexpr float SiegeDeploymentSeconds = 0.85f;

    inline constexpr float BarracksBuildSeconds = 12.0f;
    inline constexpr float DefenseTowerBuildSeconds = 8.5f;
    inline constexpr float InfantryTrainSeconds = 8.0f;
    inline constexpr float ShooterTrainSeconds = 9.6f;
    inline constexpr float CavalryTrainSeconds = 17.5f;
    inline constexpr float SiegeTrainSeconds = 24.5f;
    inline constexpr float GuardianTrainSeconds = 28.0f;
}

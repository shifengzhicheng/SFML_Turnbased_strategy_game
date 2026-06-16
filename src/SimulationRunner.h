#pragma once

enum class ScriptedPlan
{
    Balanced,
    Rush,
    Greedy
};

struct CliOptions
{
    bool simulate = false;
    bool simulatePlayer = false;
    bool simulateIgnoreGameOver = false;
    bool trainPolicies = false;
    ScriptedPlan scriptedPlan = ScriptedPlan::Balanced;
    float simulateSeconds = 120.f;
    float simulateDt = 0.10f;
    int trainEpisodes = 40;
    float trainSeconds = 420.f;
    unsigned int trainSeed = 20260616u;
};

CliOptions parseCliOptions(int argc, char* argv[]);
int runPolicyTrainingCommand(const CliOptions& options);
int runSimulationCommand(const CliOptions& options);

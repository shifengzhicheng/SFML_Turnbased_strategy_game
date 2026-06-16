#pragma once

#include <string>

class Game;

enum class ScriptedPlan
{
    Balanced,
    Rush,
    Greedy
};

ScriptedPlan parseScriptedPlan(const std::string& value);
const char* scriptedPlanName(ScriptedPlan plan);
int runScriptedSimulation(Game& game, ScriptedPlan scriptedPlan, bool simulatePlayer,
                         bool simulateIgnoreGameOver, float simulateSeconds, float simulateDt);

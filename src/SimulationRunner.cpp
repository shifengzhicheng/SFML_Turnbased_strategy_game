#include "SimulationRunner.h"

#include "Game.h"
#include "PolicyTrainer.h"
#include "ScriptedPlan.h"

#include <algorithm>
#include <cstdlib>
#include <string>

namespace
{
    bool looksLikeNumber(const std::string& value)
    {
        if (value.empty()) {
            return false;
        }
        char* end = nullptr;
        std::strtof(value.c_str(), &end);
        return end != value.c_str() && *end == '\0';
    }
}

CliOptions parseCliOptions(int argc, char* argv[])
{
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--simulate") {
            options.simulate = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-player") {
            options.simulate = true;
            options.simulatePlayer = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                options.scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-plan") {
            options.simulate = true;
            options.simulatePlayer = true;
            if (i + 1 < argc) {
                options.scriptedPlan = parseScriptedPlan(argv[++i]);
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-ignore-gameover") {
            options.simulate = true;
            options.simulateIgnoreGameOver = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-dt" && i + 1 < argc && looksLikeNumber(argv[i + 1])) {
            options.simulateDt = std::clamp(std::stof(argv[++i]), 0.02f, 0.25f);
        }
        else if (arg == "--train-policies") {
            options.trainPolicies = true;
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainEpisodes = std::max(1, std::stoi(argv[++i]));
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainSeconds = std::max(60.f, std::stof(argv[++i]));
            }
            if (i + 1 < argc && looksLikeNumber(argv[i + 1])) {
                options.trainSeed = static_cast<unsigned int>(std::stoul(argv[++i]));
            }
        }
    }
    return options;
}

int runPolicyTrainingCommand(const CliOptions& options)
{
    runPolicyTraining(options.trainEpisodes, options.trainSeconds, options.simulateDt, options.trainSeed);
    return 0;
}

int runSimulationCommand(const CliOptions& options)
{
    Game game;
    game.debugLogging = true;
    game.autoChooseRewards = true;
    game.window.setVisible(false);
    game.gameSceneState = SCENE_GAME;
    game.clear();

    return runScriptedSimulation(game, options.scriptedPlan, options.simulatePlayer,
                                options.simulateIgnoreGameOver, options.simulateSeconds, options.simulateDt);
}

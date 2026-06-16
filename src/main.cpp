#include "Game.h"
#include "SimulationRunner.h"

#include <filesystem>

namespace
{
    void switchToExecutableDirectory(int argc, char* argv[])
    {
        if (argc <= 0) {
            return;
        }

        const auto executablePath = std::filesystem::weakly_canonical(argv[0]);
        const auto executableDir = executablePath.parent_path();
        if (!executableDir.empty()) {
            std::filesystem::current_path(executableDir);
        }
    }
}

int main(int argc, char* argv[])
{
    const CliOptions options = parseCliOptions(argc, argv);
    switchToExecutableDirectory(argc, argv);

    if (options.trainPolicies) {
        return runPolicyTrainingCommand(options);
    }
    if (options.simulate) {
        return runSimulationCommand(options);
    }

    Game game;
    game.run();
    return 0;
}

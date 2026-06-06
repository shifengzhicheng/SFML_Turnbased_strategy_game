#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "Game.h"

int main(int argc, char* argv[])
{
    bool simulate = false;
    float simulateSeconds = 120.f;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--simulate") {
            simulate = true;
            if (i + 1 < argc) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
    }

    if (argc > 0) {
        const auto executablePath = std::filesystem::weakly_canonical(argv[0]);
        const auto executableDir = executablePath.parent_path();
        if (!executableDir.empty()) {
            std::filesystem::current_path(executableDir);
        }
    }

    Game game;
    if (simulate) {
        game.debugLogging = true;
        game.window.setVisible(false);
        game.gameSceneState = SCENE_GAME;
        game.clear();

        constexpr float dt = 0.10f;
        const int ticks = static_cast<int>(simulateSeconds / dt);
        for (int i = 0; i < ticks && !game.gameOver; ++i) {
            game.updateRealtime(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        game.logDebugSummary();
        return 0;
    }

    game.run();
    return 0;
}

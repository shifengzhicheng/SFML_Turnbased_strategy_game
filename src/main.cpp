#include <filesystem>
#include <iostream>

#include "Game.h"

int main(int argc, char* argv[])
{
    if (argc > 0) {
        const auto executablePath = std::filesystem::weakly_canonical(argv[0]);
        const auto executableDir = executablePath.parent_path();
        if (!executableDir.empty()) {
            std::filesystem::current_path(executableDir);
        }
    }

    Game game;
    game.run();
    return 0;
}

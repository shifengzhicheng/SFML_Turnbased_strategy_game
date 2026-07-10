#include "Config.h"
#include "Game.h"

#include <SFML/Graphics.hpp>

#include <iostream>
#include <string>

namespace
{
    bool render(Game& game, const std::string& path)
    {
        sf::RenderTexture canvas;
        if (!canvas.create(config::WindowWidth, config::WindowHeight)) {
            return false;
        }
        canvas.clear(sf::Color(8, 11, 10));
        game.Draw(canvas);
        canvas.display();
        return canvas.getTexture().copyToImage().saveToFile(path);
    }
}

int main(int argc, char** argv)
{
    const std::string prefix = argc > 1 ? argv[1] : "build";
    Game game;
    game.window.setVisible(false);
    game.debugLogging = false;

    game.gameSceneState = SCENE_START;
    game.tutorialVisible = false;
    if (!render(game, prefix + "/frontend_start.png")) {
        return 1;
    }

    game.tutorialVisible = true;
    if (!render(game, prefix + "/frontend_help.png")) {
        return 1;
    }

    game.tutorialVisible = false;
    game.gameSceneState = SCEN_GAMEOVER;
    game.gameWin = true;
    game.gameTimeSeconds = 612.f;
    game.playerUpgradeLevel = 12;
    game.aiUpgradeLevel = 11;
    if (!render(game, prefix + "/frontend_victory.png")) {
        return 1;
    }

    game.gameSceneState = SCENE_GAME;
    game.clear();
    game.externalAIControl = true;
    game.playerUpgradeLevel = 7;
    game.playerRewardRerolls = config::RewardRerollsPerChoice;
    game.buildRewardChoices();
    game.perkOverlayVisible = true;
    if (!render(game, prefix + "/frontend_reward.png")) {
        return 1;
    }

    std::cout << prefix + "/frontend_start.png\n"
              << prefix + "/frontend_help.png\n"
              << prefix + "/frontend_victory.png\n"
              << prefix + "/frontend_reward.png\n";
    return 0;
}

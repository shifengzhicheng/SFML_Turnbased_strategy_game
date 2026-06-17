#include "Config.h"
#include "Game.h"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : "build/map_preview.png";

    Game game;
    game.window.setVisible(false);
    game.debugLogging = false;
    game.externalAIControl = true;
    game.gameSceneState = SCENE_GAME;
    game.clear();

    sf::RenderTexture canvas;
    if (!canvas.create(config::MapWidth, config::MapHeight)) {
        return 1;
    }
    canvas.clear(sf::Color(26, 34, 28));

    std::vector<const MapPos*> raised;
    raised.reserve(game.tiles.size());
    for (const auto& tile : game.tiles) {
        tile.drawGround(canvas, sf::RenderStates::Default);
        // Resource nodes are rendered by GameWorldRender at runtime; include
        // their tile object here so the standalone terrain preview is faithful.
        if (tile.hasRaisedObject() || tile.getID() == tile::Resource) {
            raised.push_back(&tile);
        }
    }
    std::sort(raised.begin(), raised.end(), [](const MapPos* a, const MapPos* b) {
        return a->renderSortY() < b->renderSortY();
    });
    for (const MapPos* tile : raised) {
        tile->drawObject(canvas, sf::RenderStates::Default);
    }

    canvas.display();
    if (!canvas.getTexture().copyToImage().saveToFile(out)) {
        std::cerr << "Failed to save map preview to " << out << '\n';
        return 1;
    }

    std::cout << out << '\n';
    return 0;
}

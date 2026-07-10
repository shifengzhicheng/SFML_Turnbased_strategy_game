#include "Game.h"
#include "AllUnit.h"
#include "Config.h"
#include "Map.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "GameInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    constexpr sf::Uint32 WindowStyle = sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
}

Game::Game() :
    gameWin(false),
    MosOnUnit(nullptr),
    horizontalTiles(width / SqureSize),
    debugLogging(std::getenv("TBS_LOG") != nullptr)
{
    // Rendering and hit testing share a fixed logical canvas. The OS window can
    // resize freely; letterboxing preserves square tiles and exact UI hitboxes.
    window.create(sf::VideoMode{ config::WindowWidth, config::WindowHeight }, "Project War", WindowStyle);
    window.setView(logicalView());
    window.setFramerateLimit(60);
    Initial();
}

Game::~Game() = default;

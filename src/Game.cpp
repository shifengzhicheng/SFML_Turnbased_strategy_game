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
    constexpr sf::Uint32 FixedWindowStyle = sf::Style::Titlebar | sf::Style::Close;
}

Game::Game() :
    gameWin(false),
    MosOnUnit(nullptr),
    horizontalTiles(width / SqureSize),
    debugLogging(std::getenv("TBS_LOG") != nullptr)
{
    // Gameplay, pixel art, and hit testing all use one fixed logical canvas.
    // Keep the OS window non-resizable so SFML never stretches tiles or UI.
    window.create(sf::VideoMode{ config::WindowWidth, config::WindowHeight }, "Project_War", FixedWindowStyle);
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight)));
    window.setFramerateLimit(60);
    Initial();
}

Game::~Game() = default;

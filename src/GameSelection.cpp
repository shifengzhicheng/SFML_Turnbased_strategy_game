#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::clearSelection()
{
    // Keep selection exclusive so UI hints, build buttons, and path previews
    // cannot be overwritten by multiple selected actors in one frame.
    drawPaths.clear();
    if (Base_red) {
        Base_red->setState(UState::UNITNORMAL);
    }
    if (Base_blue) {
        Base_blue->setState(UState::UNITNORMAL);
    }
    for (auto& unit : myunits) {
        unit->setState(UState::UNITNORMAL);
    }
    for (auto& unit : enemys) {
        unit->setState(UState::UNITNORMAL);
    }
}

void Game::selectOnly(Unit* unit)
{
    const bool wasSelected = unit != nullptr && unit->UnitState == UState::UNITCLICK;
    clearSelection();
    if (unit != nullptr && !wasSelected) {
        unit->setState(UState::UNITCLICK);
    }
}

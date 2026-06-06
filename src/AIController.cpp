#include "AIController.h"

#include "Game.h"
#include "RealtimeConfig.h"

void AIController::reset()
{
    thinkTimer = 0.f;
}

void AIController::update(Game& game, float dt)
{
    thinkTimer += dt;
    if (thinkTimer < realtime::AIThinkSeconds) {
        return;
    }
    thinkTimer = 0.f;

    game.runAIProduction();
}

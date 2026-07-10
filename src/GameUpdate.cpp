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

void Game::logicBeforeInput()
{
    syncMazeFromTiles();
    astar.setMaze(maze);
}

void Game::updateRealtime(float dt)
{
    gameTimeSeconds += dt;
    syncMazeFromTiles();
    astar.setMaze(maze);
    applyPathResults();
    cleanupDestroyedBuildings();
    updateComebackTimers(dt);
    updateRealtimeEconomy(dt);
    if (!externalAIControl) {
        aiController.update(*this, dt);
    }
    assignWorkers();
    updateWorkers(dt);
    updateProduction(dt);
    updateEmergencyBaseTraining(dt);
    updateDefenseTowers(dt);
    updateBaseDefenses(dt);
    realtime::updateAutoCombat(*this, dt);
    cleanupDestroyedUnits();
    if ((Base_red && Base_red->Health <= 0) || (Base_blue && Base_blue->Health <= 0)) {
        gameOver = true;
    }
    cleanupDestroyedBuildings();
    updateDebugSummary(dt);
}

void Game::advanceRealtime(float elapsedSeconds)
{
    const double fixedStep = static_cast<double>(realtime::SimulationStepSeconds);
    realtimeAccumulator += static_cast<double>(std::clamp(elapsedSeconds, 0.f, realtime::MaxFrameAdvanceSeconds));

    int steps = 0;
    while (realtimeAccumulator + 1e-9 >= fixedStep && steps < realtime::MaxSimulationStepsPerFrame) {
        updateRealtime(realtime::SimulationStepSeconds);
        realtimeAccumulator -= fixedStep;
        ++steps;
    }

    if (steps == realtime::MaxSimulationStepsPerFrame && realtimeAccumulator >= fixedStep) {
        realtimeAccumulator = std::fmod(realtimeAccumulator, fixedStep);
    }
}

void Game::updateComebackTimers(float dt)
{
    playerBaseShieldTimer = std::max(0.f, playerBaseShieldTimer - dt);
    aiBaseShieldTimer = std::max(0.f, aiBaseShieldTimer - dt);
}

void Game::updateTimedRewards()
{
    // Rogue choices are intentionally tied to tech upgrades, not timers.
    // The method stays as a hook for future event-driven rewards.
}

void Game::updateRealtimeEconomy(float dt)
{
    playerIncomeTimer += dt;
    aiIncomeTimer += dt;

    if (playerIncomeTimer >= realtime::EconomyTickSeconds) {
        playerIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(PLAYER);
    }
    if (aiIncomeTimer >= realtime::EconomyTickSeconds) {
        aiIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(AI);
    }
}

void Game::logEvent(const std::string& message) const
{
    if (!debugLogging) {
        return;
    }
    std::clog << "[tbs " << static_cast<int>(std::round(gameTimeSeconds)) << "s] " << message << '\n';
}

void Game::logDebugSummary() const
{
    if (!debugLogging) {
        return;
    }
    std::clog << "[tbs " << static_cast<int>(std::round(gameTimeSeconds)) << "s] "
        << "player cmd=" << playerCommand
        << " eco=" << playerEconomyLevel
        << " rax=" << completedBuildingCount(PLAYER, building::Barracks) << "/" << buildingCap(PLAYER, building::Barracks)
        << " tower=" << totalBuildingCount(PLAYER, building::DefenseTower)
        << " level=" << playerUpgradeLevel
        << " base=" << (Base_red ? Base_red->Health : 0)
        << " shield=" << static_cast<int>(std::ceil(playerBaseShieldTimer))
        << " army=" << myunits.size()
        << " | ai cmd=" << aiCommand
        << " eco=" << aiEconomyLevel
        << " rax=" << completedBuildingCount(AI, building::Barracks) << "/" << buildingCap(AI, building::Barracks)
        << " tower=" << totalBuildingCount(AI, building::DefenseTower)
        << " level=" << aiUpgradeLevel
        << " base=" << (Base_blue ? Base_blue->Health : 0)
        << " shield=" << static_cast<int>(std::ceil(aiBaseShieldTimer))
        << " army=" << enemys.size()
        << '\n';
}

void Game::updateDebugSummary(float dt)
{
    if (!debugLogging) {
        return;
    }
    debugSummaryTimer += dt;
    if (debugSummaryTimer >= 10.f) {
        debugSummaryTimer -= 10.f;
        logDebugSummary();
    }
}

void Game::logicBeforeDraw()
{
    if (!perkOverlayVisible) {
        advanceRealtime(realtimeFrameClock.restart().asSeconds());
    }
    else {
        realtimeFrameClock.restart();
    }

    if (Base_blue) {
        Base_blue->updatemystate();
    }
    if (Base_red) {
        Base_red->updatemystate();
    }
    for (auto& unit : myunits) {
        unit->updatemystate();
    }
    for (auto& unit : enemys) {
        unit->updatemystate();
    }
}

void Game::cleanupDestroyedUnits()
{
    const auto removeDead = [this](std::list<std::unique_ptr<MoveableUnit>>& units, int bountyReceiver) {
        for (auto it = units.begin(); it != units.end(); ) {
            MoveableUnit& unit = **it;
            if (!unit.isdead()) {
                ++it;
                continue;
            }

            awardKillBounty(bountyReceiver, unit.unitName, Point(unit.x, unit.y));
            if (MosOnUnit == &unit) {
                MosOnUnit = nullptr;
            }
            it = units.erase(it);
        }
    };

    // Unit death is simulation state, not rendering state. Keeping cleanup here
    // makes CLI simulations, AI training, and the visible game follow one path.
    removeDead(myunits, AI);
    removeDead(enemys, PLAYER);
}

void Game::logicAfterDraw()
{
}

void Game::run()
{

    while (window.isOpen())
    {

        window.clear();
        if(gameSceneState==SCENE_GAME)
            logicBeforeInput();


        Input();

        if (gameSceneState == SCENE_GAME)
            logicBeforeDraw();

        Draw();

        logicAfterDraw();


        window.display();


    }
}

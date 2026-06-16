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
    if ((Base_red && Base_red->Health <= 0) || (Base_blue && Base_blue->Health <= 0)) {
        gameOver = true;
    }
    cleanupDestroyedBuildings();
    updateDebugSummary(dt);
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
        updateRealtime(std::min(realtimeFrameClock.restart().asSeconds(), 0.05f));
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
    for (auto u = myunits.begin(); u != myunits.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            awardKillBounty(AI, (*u)->unitName, Point((*u)->x, (*u)->y));
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = myunits.erase(u);
        }
        else {
            ++u;
        }
    }
    for (auto u = enemys.begin(); u != enemys.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            awardKillBounty(PLAYER, (*u)->unitName, Point((*u)->x, (*u)->y));
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = enemys.erase(u);
        }
        else {
            ++u;
        }
    }
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

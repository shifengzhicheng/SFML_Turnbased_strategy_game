#include <filesystem>
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "Game.h"
#include "RealtimeConfig.h"

namespace
{
    int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    int chooseScriptedResource(Game& game, int team)
    {
        const Point base = team == PLAYER ? game.Red_baseP : game.Blue_baseP;
        int bestIndex = -1;
        int bestScore = 1'000'000;
        for (int i = 0; i < static_cast<int>(game.resources.size()); ++i) {
            if (game.pendingOrCompleteExtractorForResource(i) != 0) {
                continue;
            }
            int score = distanceSquared(base, game.resources[i].point) - game.resources[i].income * 45;
            if (game.gameTimeSeconds > 70.f && game.resources[i].income >= 8) {
                score -= 160;
            }
            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    void runScriptedPlayer(Game& game)
    {
        const int totalExtractors = game.totalBuildingCount(PLAYER, building::Extractor);
        const int totalBarracks = game.totalBuildingCount(PLAYER, building::Barracks);
        const int completedExtractors = game.completedBuildingCount(PLAYER, building::Extractor);
        const int completedBarracks = game.completedBuildingCount(PLAYER, building::Barracks);

        if (totalExtractors < 1 && game.commandForTeam(PLAYER) >= config::ExtractorCost) {
            const int resource = chooseScriptedResource(game, PLAYER);
            if (resource >= 0 && game.requestBuildExtractor(PLAYER, resource)) {
                return;
            }
        }

        if (totalBarracks < 1 && game.commandForTeam(PLAYER) >= config::BarracksCost) {
            const Point site = game.findBuildableNear(game.Red_baseP, 9);
            if (site.x >= 0 && game.requestBuildBarracks(PLAYER, site)) {
                return;
            }
        }

        int desiredExtractors = 1;
        if (game.gameTimeSeconds > 50.f) {
            desiredExtractors = 2;
        }
        if (game.gameTimeSeconds > 120.f) {
            desiredExtractors = 3;
        }
        desiredExtractors = std::min(desiredExtractors, realtime::MaxWorkers - 1);
        if (totalExtractors < desiredExtractors && game.commandForTeam(PLAYER) >= config::ExtractorCost) {
            const int resource = chooseScriptedResource(game, PLAYER);
            if (resource >= 0 && game.requestBuildExtractor(PLAYER, resource)) {
                return;
            }
        }
        if (totalExtractors < desiredExtractors) {
            return;
        }

        int allowedTech = 0;
        if (game.gameTimeSeconds > 70.f && completedExtractors >= 1 && completedBarracks >= 1) {
            allowedTech = 1;
        }
        if (game.gameTimeSeconds > 145.f && completedExtractors >= 2) {
            allowedTech = 2;
        }
        if (game.playerUpgradeLevel < allowedTech
            && game.commandForTeam(PLAYER) >= game.upgradeCostForNextLevel(PLAYER)) {
            if (game.upgradeTeam(PLAYER)) {
                return;
            }
        }
        if (game.playerUpgradeLevel < allowedTech) {
            return;
        }

        int desiredBarracks = 1;
        if (game.gameTimeSeconds > 95.f && completedExtractors >= 2) {
            desiredBarracks = 2;
        }
        if (game.gameTimeSeconds > 175.f && game.playerUpgradeLevel >= 2) {
            desiredBarracks = 3;
        }
        desiredBarracks = std::min(desiredBarracks, game.buildingCap(PLAYER, building::Barracks));
        if (totalBarracks < desiredBarracks && game.commandForTeam(PLAYER) >= config::BarracksCost) {
            const Point site = game.findBuildableNear(game.Red_baseP, 12);
            if (site.x >= 0 && game.requestBuildBarracks(PLAYER, site)) {
                return;
            }
        }
        if (totalBarracks < desiredBarracks) {
            return;
        }

        const int priorities[] = {
            UName::SHOOTER,
            UName::INFANTARY,
            UName::CAVALRY
        };
        const int orders = std::max(1, game.playerUpgradeLevel / 2 + 1);
        for (int i = 0; i < orders; ++i) {
            bool queued = false;
            for (int unit : priorities) {
                if (game.enqueueUnit(PLAYER, unit)) {
                    queued = true;
                    break;
                }
            }
            if (!queued) {
                break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    bool simulate = false;
    bool simulatePlayer = false;
    float simulateSeconds = 120.f;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--simulate") {
            simulate = true;
            if (i + 1 < argc) {
                simulateSeconds = std::stof(argv[++i]);
            }
        }
        else if (arg == "--simulate-player") {
            simulate = true;
            simulatePlayer = true;
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
        float playerScriptTimer = 0.f;
        for (int i = 0; i < ticks && !game.gameOver; ++i) {
            if (simulatePlayer) {
                playerScriptTimer += dt;
                if (playerScriptTimer >= realtime::AIThinkSeconds) {
                    playerScriptTimer = 0.f;
                    runScriptedPlayer(game);
                }
            }
            game.updateRealtime(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        game.logDebugSummary();
        return 0;
    }

    game.run();
    return 0;
}

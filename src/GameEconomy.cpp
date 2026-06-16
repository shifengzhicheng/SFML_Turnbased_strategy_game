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

namespace
{
    int economyIncomeBonusForLevel(int level)
    {
        const int safeLevel = std::max(0, level);
        return safeLevel * config::EconomyIncomeStep
            + (safeLevel * safeLevel) / config::EconomyIncomeQuadraticDivisor;
    }

    float perkHealthBonusForUnit(const Game& game, int team, int unitName)
    {
        float bonus = 0.f;
        if (unitName == UName::INFANTARY || unitName == UName::GUARDIAN) {
            bonus += static_cast<float>(game.perkLevel(team, perk::Fortitude)) * config::FortitudeHealthBonus;
        }
        if (unitName == UName::CAVALRY) {
            bonus += static_cast<float>(game.perkLevel(team, perk::Charge)) * config::ChargeHealthBonus;
        }
        if (unitName == UName::SIEGE) {
            bonus += static_cast<float>(game.perkLevel(team, perk::SiegeCraft)) * config::SiegeHealthBonus;
        }
        return bonus;
    }
}

int Game::upgradeCostForNextLevel(int team) const
{
    const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    if (level >= config::MaxTechLevel) {
        return 0;
    }

    const int costs[config::MaxTechLevel] = {
        65, 85, 110, 140, 175,
        215, 260, 310, 370, 440,
        520, 610, 710, 820, 950
    };
    int rawCost = costs[level];
    if (team == AI && gameTimeSeconds > 420.f && economyLevelForTeam(AI) <= 1) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.62f));
    }
    if (team == AI && gameTimeSeconds > 840.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.42f));
    }
    else if (team == AI && gameTimeSeconds > 780.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.58f));
    }
    else if (team == AI && gameTimeSeconds > 660.f) {
        rawCost = static_cast<int>(std::round(static_cast<float>(rawCost) * 0.74f));
    }
    return std::max(25, rawCost);
}

int Game::resourceIncome(int team) const
{
    const int level = economyLevelForTeam(team);
    const float multiplier = miningIncomeMultiplier(team);
    int income = config::BaseCommandIncome
        + static_cast<int>(std::round(static_cast<float>(economyIncomeBonusForLevel(level)) * multiplier));
    if (team == AI) {
        // A mild difficulty stipend keeps heuristic AI competitive without
        // adding hidden resource types for the player to understand.
        income += 1 + static_cast<int>(gameTimeSeconds / 300.f);
    }
    return income;
}

int Game::economyLevelForTeam(int team) const
{
    return team == PLAYER ? playerEconomyLevel : aiEconomyLevel;
}

int Game::economyUpgradeCost(int team) const
{
    const int level = economyLevelForTeam(team);
    if (level >= config::MaxEconomyLevel) {
        return 0;
    }
    // Keep payback near the length of one or two pushes: late economy should
    // accelerate the army, not feel like a dead sink before the match ends.
    int cost = config::EconomyUpgradeCost
        + level * config::EconomyUpgradeCostStep
        + level * level;
    if (team == AI && gameTimeSeconds > 840.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.52f));
    }
    else if (team == AI && gameTimeSeconds > 720.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.65f));
    }
    else if (team == AI && gameTimeSeconds > 420.f) {
        cost = static_cast<int>(std::round(static_cast<float>(cost) * 0.78f));
    }
    return std::max(20, cost);
}

bool Game::upgradeTeam(int team)
{
    int& level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int cost = upgradeCostForNextLevel(team);
    if (level >= config::MaxTechLevel || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                            level >= config::MaxTechLevel ? "Max level" : "Need CMD",
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    const int previousLevel = level;
    ++level;
    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        const float perkBonus = perkHealthBonusForUnit(*this, team, unit->unitName);
        const float oldMultiplier = 1.f + static_cast<float>(previousLevel) * config::TechHealthBonus + perkBonus;
        const float newMultiplier = 1.f + static_cast<float>(level) * config::TechHealthBonus + perkBonus;
        // Use the same additive tech formula as freshly spawned units, so old
        // and new soldiers share identical max health at a given LEVEL.
        unit->scaleMaxHealth(newMultiplier / oldMultiplier);
    }
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EndTurnButtonY) - 18.f),
                        "LEVEL " + std::to_string(level), sf::Color(218, 255, 134), 12);
    }
    maybeGrantReward(team, "Tech " + std::to_string(level));
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " upgraded to level " + std::to_string(level));
    return true;
}

void Game::syncWorkersForEconomy(int team)
{
    const int targetWorkers = std::min(realtime::MaxWorkers, realtime::StartingWorkers + economyLevelForTeam(team));
    while (workerCount(team) < targetWorkers) {
        createWorker(team, workerSpawnPoint(team));
    }
}

bool Game::upgradeEconomy(int team)
{
    int& level = team == PLAYER ? playerEconomyLevel : aiEconomyLevel;
    const int cost = economyUpgradeCost(team);
    const int previousIncome = resourceIncome(team);
    if (level >= config::MaxEconomyLevel || commandPool(*this, team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::EconomyButtonY) - 18.f),
                            level >= config::MaxEconomyLevel ? "Max economy" : "Need CMD",
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    commandPool(*this, team) -= cost;
    ++level;
    syncWorkersForEconomy(team);
    const int incomeGain = resourceIncome(team) - previousIncome;
    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 42.f),
                        "Eco Lv" + std::to_string(level) + " +" + std::to_string(incomeGain) + "/tick",
                        team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 13);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " economy level " + std::to_string(level));
    return true;
}

void Game::awardKillBounty(int receiverTeam, int defeatedUnitName, Point point)
{
    const int cost = unitCost(defeatedUnitName);
    if (cost <= 0) {
        return;
    }
    const int bounty = std::max(config::KillBountyMin,
        static_cast<int>(std::round(static_cast<float>(cost * config::KillBountyPercent) / 100.f)));
    commandPool(*this, receiverTeam) = std::min(config::MaxCommand, commandPool(*this, receiverTeam) + bounty);
    addFloatingText(sf::Vector2f(point.x * SqureSize + SqureSize / 2.f, point.y * SqureSize - 22.f),
                    "+" + std::to_string(bounty) + " CMD",
                    receiverTeam == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 12);
    logEvent(std::string(receiverTeam == PLAYER ? "player" : "ai")
        + " bounty +" + std::to_string(bounty) + " unit=" + std::to_string(defeatedUnitName));
}

void Game::addTurnIncome(int team)
{
    const int income = resourceIncome(team);
    int& command = commandPool(*this, team);
    command = std::min(config::MaxCommand, command + income);

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 8.f),
                        "+" + std::to_string(income) + " CMD",
                        team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 13);
    }
}

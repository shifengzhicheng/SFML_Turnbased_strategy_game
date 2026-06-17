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
    float perkHealthBonusForUnit(int type, int unitName, int level)
    {
        if (type == perk::Fortitude && (unitName == UName::INFANTARY || unitName == UName::GUARDIAN)) {
            return static_cast<float>(level) * config::FortitudeHealthBonus;
        }
        if (type == perk::Charge && unitName == UName::CAVALRY) {
            return static_cast<float>(level) * config::ChargeHealthBonus;
        }
        if (type == perk::SiegeCraft && unitName == UName::SIEGE) {
            return static_cast<float>(level) * config::SiegeHealthBonus;
        }
        return 0.f;
    }
}

int Game::perkLevel(int team, int type) const
{
    if (type < 0 || type >= perk::Count) {
        return 0;
    }
    return team == PLAYER ? playerPerkLevels[static_cast<std::size_t>(type)]
        : aiPerkLevels[static_cast<std::size_t>(type)];
}

void Game::buildRewardChoices()
{
    const int rotation[] = {
        perk::Drill,
        perk::Fortitude,
        perk::Volley,
        perk::Logistics,
        perk::Mining,
        perk::WarChest,
        perk::Charge,
        perk::SiegeCraft,
        perk::TowerCraft
    };
    const int rotationSize = static_cast<int>(sizeof(rotation) / sizeof(rotation[0]));
    bool used[perk::Count] = {};
    int cursor = rewardSequence % rotationSize;

    for (auto& choice : perkChoices) {
        int selected = perk::WarChest;
        for (int attempts = 0; attempts < rotationSize * 2; ++attempts) {
            const int candidate = rotation[(cursor + attempts) % rotationSize];
            if (!used[candidate] && perkLevel(PLAYER, candidate) < maxPerkLevel(candidate)) {
                selected = candidate;
                cursor = (cursor + attempts + 1) % rotationSize;
                break;
            }
        }
        used[selected] = true;
        choice.type = selected;
        choice.title = perkTitle(selected);
        choice.description = perkDescription(selected);
    }

    ++rewardSequence;
}

void Game::rerollRewardChoices()
{
    if (!perkOverlayVisible) {
        return;
    }
    if (playerRewardRerolls <= 0) {
        addFloatingText(sf::Vector2f(236.f, 486.f), "No reroll left", sf::Color(255, 184, 116), 12);
        return;
    }

    --playerRewardRerolls;
    buildRewardChoices();
    // Rerolling advances the deterministic reward sequence, so the player can
    // fish for a build-defining spike without adding hidden randomness.
    addFloatingText(sf::Vector2f(236.f, 486.f), "Tactics refreshed", sf::Color(218, 255, 134), 12);
}

void Game::applyPerk(int team, int type)
{
    if (type < 0 || type >= perk::Count) {
        return;
    }

    if (type == perk::WarChest) {
        auto& levels = team == PLAYER ? playerPerkLevels : aiPerkLevels;
        levels[static_cast<std::size_t>(type)] += 1;
        const int bonus = config::WarChestBaseBonus
            + (team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * config::WarChestTechBonus;
        commandPool(*this, team) = std::min(config::MaxCommand, commandPool(*this, team) + bonus);
        Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
        if (base != nullptr) {
            addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 30.f),
                            "War Chest +" + std::to_string(bonus), team == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 13);
        }
        return;
    }

    auto& levels = team == PLAYER ? playerPerkLevels : aiPerkLevels;
    int& level = levels[static_cast<std::size_t>(type)];
    if (level >= maxPerkLevel(type)) {
        return;
    }
    const int previousLevel = level;
    ++level;

    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        const float oldBonus = perkHealthBonusForUnit(type, unit->unitName, previousLevel);
        const float newBonus = perkHealthBonusForUnit(type, unit->unitName, level);
        if (newBonus > oldBonus) {
            const int techLevel = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
            const float techMultiplier = 1.f + static_cast<float>(techLevel) * config::TechHealthBonus;
            unit->scaleMaxHealth((techMultiplier + newBonus) / (techMultiplier + oldBonus));
        }
    }

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 30.f),
                        std::string(perkTitle(type)) + " Lv" + std::to_string(level),
                        team == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 13);
    }
}

void Game::applyRewardChoice(int index)
{
    if (index < 0 || index >= static_cast<int>(perkChoices.size())) {
        return;
    }
    applyPerk(PLAYER, perkChoices[static_cast<std::size_t>(index)].type);
    playerRewardRerolls = 0;
    perkOverlayVisible = false;
}

void Game::maybeGrantReward(int team, const std::string& reason)
{
    if (team == PLAYER) {
        if (perkOverlayVisible) {
            return;
        }
        playerRewardRerolls = config::RewardRerollsPerChoice;
        buildRewardChoices();
        if (autoChooseRewards) {
            applyRewardChoice(0);
        }
        else {
            perkOverlayVisible = true;
            addFloatingText(sf::Vector2f(config::PanelX + 16.f, 42.f),
                            "Tactic ready", sf::Color(255, 226, 112), 13);
        }
        logEvent("player reward: " + reason);
        return;
    }

    int choice = perk::WarChest;
    const bool playerTurtling = totalBuildingCount(PLAYER, building::DefenseTower) > 0
        || totalBuildingCount(PLAYER, building::Barracks) >= 3;
    const bool aiMacroBehind = aiEconomyLevel < std::min(config::MaxEconomyLevel, 3 + static_cast<int>(gameTimeSeconds / 140.f));
    if (aiMacroBehind && perkLevel(AI, perk::Mining) < maxPerkLevel(perk::Mining)) {
        choice = perk::Mining;
    }
    else if ((playerTurtling || gameTimeSeconds > 720.f) && perkLevel(AI, perk::SiegeCraft) < maxPerkLevel(perk::SiegeCraft)) {
        choice = perk::SiegeCraft;
    }
    else if (completedBuildingCount(AI, building::Barracks) >= 2 && perkLevel(AI, perk::Logistics) < maxPerkLevel(perk::Logistics)) {
        choice = perk::Logistics;
    }
    else if (countUnitsNamed(myunits, UName::SHOOTER) > countUnitsNamed(myunits, UName::INFANTARY)
        && perkLevel(AI, perk::Charge) < maxPerkLevel(perk::Charge)) {
        choice = perk::Charge;
    }
    else if (isUnitUnlocked(AI, UName::SIEGE) && perkLevel(AI, perk::SiegeCraft) < maxPerkLevel(perk::SiegeCraft)) {
        choice = perk::SiegeCraft;
    }
    else if (isUnitUnlocked(AI, UName::GUARDIAN) && perkLevel(AI, perk::Fortitude) < maxPerkLevel(perk::Fortitude)) {
        choice = perk::Fortitude;
    }
    else if (isUnitUnlocked(AI, UName::GUARDIAN) && perkLevel(AI, perk::Drill) < maxPerkLevel(perk::Drill)) {
        choice = perk::Drill;
    }
    else if (completedBuildingCount(AI, building::DefenseTower) > 0 && perkLevel(AI, perk::TowerCraft) < maxPerkLevel(perk::TowerCraft)) {
        choice = perk::TowerCraft;
    }
    else if (perkLevel(AI, perk::Volley) < maxPerkLevel(perk::Volley)) {
        choice = perk::Volley;
    }

    applyPerk(AI, choice);
    logEvent("ai reward: " + reason + " perk=" + std::to_string(choice));
}

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
#include <vector>

using namespace sf;
using namespace std;
using namespace game_internal;

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
    const int rewardPool[] = {
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
    std::vector<int> candidates;
    for (int type : rewardPool) {
        if (perkLevel(PLAYER, type) < maxPerkLevel(type)) {
            candidates.push_back(type);
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), rewardRng);

    if (rewardChoicesGenerated && candidates.size() > perkChoices.size()) {
        std::array<bool, perk::Count> previous{};
        for (const auto& choice : perkChoices) {
            previous[static_cast<std::size_t>(choice.type)] = true;
        }
        std::stable_partition(candidates.begin(), candidates.end(), [&previous](int type) {
            return !previous[static_cast<std::size_t>(type)];
        });
    }

    for (std::size_t i = 0; i < perkChoices.size(); ++i) {
        const int selected = i < candidates.size() ? candidates[i] : perk::WarChest;
        auto& choice = perkChoices[i];
        choice.type = selected;
        choice.title = perkTitle(selected);
        choice.description = perkDescription(selected);
    }

    rewardChoicesGenerated = true;
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
    // A reroll excludes the previous cards when the remaining pool permits it.
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
    ++level;

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

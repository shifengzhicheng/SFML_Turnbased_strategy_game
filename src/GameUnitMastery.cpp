#include "Game.h"
#include "GameInternal.h"
#include "UnitUpgradeDefinition.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace game_internal;

namespace
{
    UnitMasteryState& masteryForTeam(Game& game, int team)
    {
        return team == PLAYER ? game.playerMastery : game.aiMastery;
    }

    int masteryButtonYForUnit(int unitName)
    {
        switch (unitName) {
        case UName::SHOOTER:
            return config::BuildShooterY;
        case UName::CAVALRY:
            return config::BuildCavalryY;
        case UName::SIEGE:
            return config::BuildSiegeY;
        case UName::GUARDIAN:
            return config::BuildGuardianY;
        case UName::INFANTARY:
        default:
            return config::BuildInfantryY;
        }
    }
}

bool Game::upgradeUnitMastery(int team, int unitName)
{
    if (!isTrainableUnit(unitName)) {
        return false;
    }

    const int cost = unitMasteryUpgradeCost(team, unitName);
    if (!isUnitUnlocked(team, unitName) || commandForTeam(team) < cost) {
        if (team == PLAYER) {
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(masteryButtonYForUnit(unitName)) - 18.f),
                            !isUnitUnlocked(team, unitName) ? "Locked" : "Need CMD",
                            sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    auto& roster = team == PLAYER ? myunits : enemys;
    std::vector<std::pair<MoveableUnit*, float>> oldHealthMultipliers;
    oldHealthMultipliers.reserve(roster.size());
    for (auto& unit : roster) {
        if (unit->unitName == unitName && unit->Health > 0) {
            oldHealthMultipliers.push_back({unit.get(), unitHealthMultiplier(team, unitName)});
        }
    }

    commandPool(*this, team) -= cost;
    UnitMasteryState& state = masteryForTeam(*this, team);
    const int nextLevel = ::unitMasteryLevel(state, unitName) + 1;
    setUnitMasteryLevel(state, unitName, nextLevel);

    const float newMultiplier = unitHealthMultiplier(team, unitName);
    for (auto& entry : oldHealthMultipliers) {
        if (entry.second > 0.f) {
            entry.first->scaleMaxHealth(newMultiplier / entry.second);
        }
    }

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 54.f),
                        std::string(unitDebugName(unitName)) + " M" + std::to_string(nextLevel) + " +"
                            + std::to_string(static_cast<int>(std::round(nextLevel * config::MasteryStatBonusPerLevel * 100.f))) + "%",
                        team == PLAYER ? sf::Color(255, 226, 112) : sf::Color(145, 196, 255), 13);
    }
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(masteryButtonYForUnit(unitName)) - 18.f),
                        std::string(unitDebugName(unitName)) + " +10% DMG/HP",
                        sf::Color(151, 235, 154), 13);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai")
        + " mastery unit=" + std::to_string(unitName)
        + " level=" + std::to_string(nextLevel)
        + " cost=" + std::to_string(cost));
    return true;
}

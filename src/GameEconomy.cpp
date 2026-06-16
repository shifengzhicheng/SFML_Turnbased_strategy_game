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

int Game::buildingCap(int team, int type) const
{
    const int tech = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const int economy = economyLevelForTeam(team);
    if (type == building::Barracks) {
        return std::min(config::BarracksCap, config::BarracksBaseCap + tech / 4 + economy / 2);
    }
    if (type == building::DefenseTower) {
        return std::min(config::TowerCap, config::TowerBaseCap + tech / 5 + perkLevel(team, perk::TowerCraft) / 3);
    }
    return static_cast<int>(resources.size());
}

float Game::damageMultiplier(int team) const
{
    const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    return 1.f + static_cast<float>(level) * config::TechDamageBonus;
}

float Game::unitDamageMultiplier(int team, int unitName) const
{
    float multiplier = damageMultiplier(team);
    switch (unitName) {
    case UName::INFANTARY:
        multiplier += static_cast<float>(perkLevel(team, perk::Drill)) * 0.08f;
        break;
    case UName::GUARDIAN:
        multiplier += static_cast<float>(perkLevel(team, perk::Drill)) * 0.06f;
        break;
    case UName::SHOOTER:
        multiplier += static_cast<float>(perkLevel(team, perk::Volley)) * 0.08f;
        break;
    case UName::CAVALRY:
        multiplier += static_cast<float>(perkLevel(team, perk::Charge)) * 0.08f;
        break;
    case UName::SIEGE:
        multiplier += static_cast<float>(perkLevel(team, perk::SiegeCraft)) * 0.08f;
        break;
    default:
        break;
    }
    return multiplier;
}

float Game::unitHealthMultiplier(int team, int unitName) const
{
    float multiplier = 1.f + static_cast<float>(team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * config::TechHealthBonus;
    if (unitName == UName::INFANTARY || unitName == UName::GUARDIAN) {
        multiplier += static_cast<float>(perkLevel(team, perk::Fortitude)) * 0.09f;
    }
    if (unitName == UName::CAVALRY) {
        multiplier += static_cast<float>(perkLevel(team, perk::Charge)) * 0.04f;
    }
    if (unitName == UName::SIEGE) {
        multiplier += static_cast<float>(perkLevel(team, perk::SiegeCraft)) * 0.04f;
    }
    return multiplier;
}

float Game::unitAttackCooldownMultiplier(int team, int unitName) const
{
    float multiplier = 1.f;
    if (unitName == UName::SHOOTER) {
        multiplier -= static_cast<float>(perkLevel(team, perk::Volley)) * 0.035f;
    }
    if (unitName == UName::CAVALRY) {
        multiplier -= static_cast<float>(perkLevel(team, perk::Charge)) * 0.018f;
    }
    return std::clamp(multiplier, 0.76f, 1.f);
}

float Game::baseDamageTakenMultiplier(int attackerUnitName, int defenderTeam) const
{
    float shield = 1.f;
    if (gameTimeSeconds < 480.f) {
        shield = 0.25f;
    }
    else if (gameTimeSeconds < 600.f) {
        shield = 0.45f;
    }
    else if (gameTimeSeconds < 720.f) {
        shield = 0.70f;
    }

    // Siege should feel like the correct finisher, but not fully erase the
    // pacing shield that keeps normal wins near the 10-minute target.
    if (attackerUnitName == UName::SIEGE) {
        shield += 0.18f;
    }
    else if (attackerUnitName == UName::GUARDIAN) {
        shield += 0.08f;
    }
    if (baseShieldSecondsForTeam(defenderTeam) > 0.f) {
        shield *= config::EmergencyShieldDamageMultiplier;
    }
    return std::clamp(shield, 0.25f, 1.f);
}

float Game::baseShieldSecondsForTeam(int team) const
{
    return team == PLAYER ? playerBaseShieldTimer : aiBaseShieldTimer;
}

float Game::teamTrainTimeMultiplier(int team) const
{
    const float logistics = static_cast<float>(perkLevel(team, perk::Logistics)) * 0.06f;
    return std::clamp(1.f - logistics, 0.74f, 1.f);
}

float Game::miningIncomeMultiplier(int team) const
{
    return 1.f + static_cast<float>(perkLevel(team, perk::Mining)) * 0.07f;
}

int Game::defenseTowerRange(int team) const
{
    return config::DefenseTowerRange + std::min(1, perkLevel(team, perk::TowerCraft) / 3);
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
        + static_cast<int>(std::round(static_cast<float>(level * config::EconomyIncomeStep) * multiplier));
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
    int cost = config::EconomyUpgradeCost
        + level * config::EconomyUpgradeCostStep
        + level * level * 3;
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

void Game::applyPerk(int team, int type)
{
    if (type < 0 || type >= perk::Count) {
        return;
    }

    if (type == perk::WarChest) {
        auto& levels = team == PLAYER ? playerPerkLevels : aiPerkLevels;
        levels[static_cast<std::size_t>(type)] += 1;
        const int bonus = 38 + (team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel) * 5;
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

    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        if (type == perk::Fortitude && (unit->unitName == UName::INFANTARY || unit->unitName == UName::GUARDIAN)) {
            unit->scaleMaxHealth(1.09f);
        }
        else if (type == perk::Charge && unit->unitName == UName::CAVALRY) {
            unit->scaleMaxHealth(1.04f);
        }
        else if (type == perk::SiegeCraft && unit->unitName == UName::SIEGE) {
            unit->scaleMaxHealth(1.04f);
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
    perkOverlayVisible = false;
}

void Game::maybeGrantReward(int team, const std::string& reason)
{
    if (team == PLAYER) {
        if (perkOverlayVisible) {
            return;
        }
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

int Game::unitCost(int name) const
{
    switch (name) {
    case UName::INFANTARY:
        return config::InfantryCost;
    case UName::SHOOTER:
        return config::ShooterCost;
    case UName::CAVALRY:
        return config::CavalryCost;
    case UName::SIEGE:
        return config::SiegeCost;
    case UName::GUARDIAN:
        return config::GuardianCost;
    default:
        return 0;
    }
}

bool Game::isUnitUnlocked(int team, int name) const
{
    const int economy = economyLevelForTeam(team);
    const int barracks = completedBuildingCount(team, building::Barracks);
    const int upgrade = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    switch (name) {
    case UName::INFANTARY:
        return barracks >= 1;
    case UName::SHOOTER:
        return barracks >= 1 && (economy >= 1 || upgrade >= 1);
    case UName::CAVALRY:
        return barracks >= 2 && (economy >= 2 || upgrade >= 3);
    case UName::SIEGE:
        return barracks >= 2 && economy >= 3 && upgrade >= 5;
    case UName::GUARDIAN:
        return barracks >= 3 && economy >= 4 && upgrade >= 7;
    default:
        return false;
    }
}

bool Game::canQueueUnit(int team, int name) const
{
    return unitCost(name) > 0
        && hasUnitCapacity(team)
        && isUnitUnlocked(team, name)
        && commandForTeam(team) >= unitCost(name)
        && completedBuildingCount(team, building::Barracks) > 0;
}

bool Game::enqueueUnit(int team, int name)
{
    if (!canQueueUnit(team, name)) {
        if (team == PLAYER) {
            std::string reason = "Need Barracks";
            if (completedBuildingCount(team, building::Barracks) > 0 && !isUnitUnlocked(team, name)) {
                reason = "Locked";
            }
            else if (commandForTeam(team) < unitCost(name)) {
                reason = "Need CMD";
            }
            addFloatingText(sf::Vector2f(config::PanelX + 18.f, static_cast<float>(config::BuildInfantryY) - 18.f),
                            reason, sf::Color(255, 214, 96), 12);
        }
        return false;
    }

    Building* best = nullptr;
    for (auto& building : buildings) {
        if (building.team != team || building.type != building::Barracks || !building.complete) {
            continue;
        }
        if (best == nullptr || building.production.load() < best->production.load()) {
            best = &building;
        }
    }
    if (best == nullptr) {
        return false;
    }

    commandPool(*this, team) -= unitCost(name);
    const int orderLane = selectedLaneForTeam(team);
    best->production.orders.push_back(ProductionOrder{name, orderLane});
    if (team == PLAYER) {
        addFloatingText(sf::Vector2f(best->point.x * SqureSize, best->point.y * SqureSize - 8.f),
                        std::string("Queued ") + laneName(orderLane), sf::Color(218, 255, 134), 12);
    }
    logEvent(std::string(team == PLAYER ? "player" : "ai") + " queued unit=" + std::to_string(name)
        + " lane=" + laneName(orderLane) + " rax=" + std::to_string(best->id)
        + " load=" + std::to_string(best->production.load()));
    return true;
}

bool Game::executeOperation(int team, const GameOperation& operation)
{
    // Central command dispatcher: UI clicks, scripted playtests, and AI plans
    // all go through this path so command validation stays identical.
    int& selectedLane = team == PLAYER ? playerSelectedLane : aiSelectedLane;
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);

    switch (operation.type) {
    case gameop::SelectLane:
        selectedLane = safeLane;
        return true;
    case gameop::UpgradeEconomy:
        return upgradeEconomy(team);
    case gameop::UpgradeTech:
        return upgradeTeam(team);
    case gameop::BuildBarracks:
        selectedLane = safeLane;
        return requestAutoBuildBarracks(team);
    case gameop::BuildTower:
        selectedLane = safeLane;
        return requestAutoBuildTower(team);
    case gameop::QueueUnit:
        selectedLane = safeLane;
        return enqueueUnit(team, operation.unitName);
    default:
        return false;
    }
}

std::size_t Game::executeOperations(int team, const std::vector<GameOperation>& operations)
{
    std::size_t successes = 0;
    for (const auto& operation : operations) {
        const bool ok = executeOperation(team, operation);
        if (ok) {
            ++successes;
        }
        logEvent(std::string(team == PLAYER ? "player" : "ai")
            + " op " + describeOperation(operation) + (ok ? " ok" : " blocked"));
    }
    return successes;
}

std::string Game::describeOperation(const GameOperation& operation) const
{
    std::string label(operationTypeName(operation.type));
    const int safeLane = std::clamp(operation.laneIndex, 0, lane::Count - 1);
    if (operation.type == gameop::BuildBarracks
        || operation.type == gameop::BuildTower
        || operation.type == gameop::QueueUnit
        || operation.type == gameop::SelectLane) {
        label += "[";
        label += laneName(safeLane);
        label += "]";
    }
    if (operation.type == gameop::QueueUnit) {
        label += ":";
        label += unitDebugName(operation.unitName);
    }
    return label;
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
    ++level;
    auto& units = team == PLAYER ? myunits : enemys;
    for (auto& unit : units) {
        unit->scaleMaxHealth(1.f + config::TechHealthBonus);
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
    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 42.f),
                        "Economy Lv" + std::to_string(level),
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

int Game::commandForTeam(int team) const
{
    return team == PLAYER ? playerCommand : aiCommand;
}

bool Game::hasUnitCapacity(int team) const
{
    return team == PLAYER ? myunits.size() < MaxUnit : enemys.size() < MaxUnit;
}

bool Game::hasSpawnTile(int team) const
{
    const DisMoveableUnit* base = team == PLAYER ? Base_red.get() : Base_blue.get();
    if (base == nullptr) {
        return false;
    }

    for (int i = base->x - 1; i < base->x + 3; ++i) {
        for (int j = base->y - 1; j < base->y + 3; ++j) {
            if (!isMapCell(i, j)) {
                continue;
            }
            if (tiles[i + horizontalTiles * j].getID() == tile::Empty && !isCellReservedForSpawn(i, j)) {
                return true;
            }
        }
    }
    return false;
}

std::string Game::spawnBlockReason(int team, int name) const
{
    const int cost = unitCost(name);
    if (cost <= 0) {
        return "Bad unit";
    }
    if (!hasUnitCapacity(team)) {
        return "Unit cap";
    }
    if (commandForTeam(team) < cost) {
        return "Need CMD";
    }
    if (!hasSpawnTile(team)) {
        return "No room";
    }
    return "";
}

bool Game::canSpawnUnit(int team, int name) const
{
    return spawnBlockReason(team, name).empty();
}

bool Game::spendCommand(int team, int name)
{
    if (!canSpawnUnit(team, name)) {
        return false;
    }
    commandPool(*this, team) -= unitCost(name);
    return true;
}

void Game::runAIProduction()
{
    if (!Base_blue) {
        return;
    }

    const int playerPressure = unitsNearPoint(PLAYER, Blue_baseP, 13);
    const int aiPressure = unitsNearPoint(AI, Red_baseP, 13);
    const int playerTowers = totalBuildingCount(PLAYER, building::DefenseTower);
    const int playerBarracks = totalBuildingCount(PLAYER, building::Barracks);
    const bool playerTurtling = playerTowers > 0 || playerBarracks >= 3;
    const bool defenseMode = playerPressure >= 4 || (Base_blue && Base_blue->Health < 2800);
    const bool armyBehind = static_cast<int>(enemys.size()) + 4 < static_cast<int>(myunits.size());
    const bool siegeMode = !defenseMode && (playerTurtling || (gameTimeSeconds > 540.f && isUnitUnlocked(AI, UName::SIEGE)));
    const bool macroMode = !defenseMode && !armyBehind && enemys.size() >= 14;

    const auto nearestLaneForY = [this](int y) {
        int bestLane = lane::Mid;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < lane::Count; ++i) {
            const int laneY = laneWaypoint(AI, i, 1).y;
            const int distance = std::abs(y - laneY);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestLane = i;
            }
        }
        return bestLane;
    };

    const auto laneWithMostPlayerUnits = [this]() {
        int counts[lane::Count] = {};
        for (const auto& unit : myunits) {
            if (unit->Health > 0) {
                ++counts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
            }
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (counts[i] > counts[bestLane]) {
                bestLane = i;
            }
        }
        return bestLane;
    };

    const auto laneWithMostPlayerStructures = [&nearestLaneForY, &laneWithMostPlayerUnits, this]() {
        int scores[lane::Count] = {};
        for (const auto& building : buildings) {
            if (building.team != PLAYER || !building.complete) {
                continue;
            }
            const int laneIndex = nearestLaneForY(building.point.y);
            scores[laneIndex] += building.type == building::DefenseTower ? 4 : 2;
        }
        int bestLane = lane::Mid;
        for (int i = 0; i < lane::Count; ++i) {
            if (scores[i] > scores[bestLane]) {
                bestLane = i;
            }
        }
        return scores[bestLane] > 0 ? bestLane : laneWithMostPlayerUnits();
    };

    const auto chooseLane = [&](int orderIndex) {
        if (defenseMode) {
            return laneWithMostPlayerUnits();
        }
        if (siegeMode) {
            return laneWithMostPlayerStructures();
        }
        return (static_cast<int>(gameTimeSeconds / 34.f) + orderIndex) % lane::Count;
    };

    int desiredEconomy = 1;
    if (gameTimeSeconds > 25.f) desiredEconomy = 2;
    if (gameTimeSeconds > 70.f) desiredEconomy = 3;
    if (gameTimeSeconds > 135.f) desiredEconomy = 4;
    if (gameTimeSeconds > 230.f) desiredEconomy = 5;
    if (gameTimeSeconds > 360.f) desiredEconomy = 7;
    if (gameTimeSeconds > 520.f) desiredEconomy = 9;
    if (gameTimeSeconds > 700.f) desiredEconomy = 11;
    if (gameTimeSeconds > 840.f) desiredEconomy = config::MaxEconomyLevel;
    if (playerEconomyLevel > aiEconomyLevel + 1 || macroMode) {
        ++desiredEconomy;
    }
    desiredEconomy = std::clamp(desiredEconomy, 0, config::MaxEconomyLevel);

    int allowedTech = static_cast<int>(gameTimeSeconds / 46.f);
    allowedTech = std::min(allowedTech, aiEconomyLevel + (gameTimeSeconds > 600.f ? 7 : 5));
    if (playerUpgradeLevel > aiUpgradeLevel) {
        allowedTech = std::max(allowedTech, playerUpgradeLevel + 1);
    }
    if (gameTimeSeconds > 620.f) {
        allowedTech = std::max(allowedTech, 10 + static_cast<int>((gameTimeSeconds - 620.f) / 46.f));
    }
    if (gameTimeSeconds > 840.f) {
        allowedTech = config::MaxTechLevel;
    }
    allowedTech = std::clamp(allowedTech, 0, config::MaxTechLevel);

    int desiredBarracks = 1;
    if (gameTimeSeconds > 80.f && aiEconomyLevel >= 1) desiredBarracks = 2;
    if (gameTimeSeconds > 210.f && aiUpgradeLevel >= 2) desiredBarracks = 3;
    if (gameTimeSeconds > 360.f && aiUpgradeLevel >= 5) desiredBarracks = 4;
    if (gameTimeSeconds > 560.f && aiUpgradeLevel >= 8) desiredBarracks = 5;
    if (gameTimeSeconds > 760.f) desiredBarracks = 6;
    if (armyBehind || siegeMode) ++desiredBarracks;
    desiredBarracks = std::min(desiredBarracks, buildingCap(AI, building::Barracks));

    int desiredTowers = 0;
    if (playerPressure >= 4) desiredTowers = 1;
    if (playerPressure >= 8 || (Base_blue && Base_blue->Health < 2200)) desiredTowers = 2;
    if (gameTimeSeconds > 700.f && playerPressure >= 6 && aiUpgradeLevel >= 10) desiredTowers = 3;
    desiredTowers = std::min(desiredTowers, buildingCap(AI, building::DefenseTower));

    if (totalBuildingCount(AI, building::Barracks) < 1) {
        if (commandForTeam(AI) >= config::BarracksCost) {
            executeOperations(AI, {GameOperation(gameop::BuildBarracks, lane::Mid)});
        }
        return;
    }
    if (completedBuildingCount(AI, building::Barracks) < 1) {
        return;
    }

    bool majorActionTaken = false;
    const auto tryMajorAction = [&](bool condition, const GameOperation& operation) {
        if (majorActionTaken || !condition) {
            return;
        }
        if (executeOperations(AI, {operation}) > 0) {
            majorActionTaken = true;
        }
    };

    const bool openingNeedsUnits = enemys.size() < 4 && gameTimeSeconds < 110.f && playerPressure < 4;
    if (!openingNeedsUnits) {
        tryMajorAction(aiEconomyLevel < desiredEconomy && commandForTeam(AI) >= economyUpgradeCost(AI),
                       GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < allowedTech && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
                       GameOperation(gameop::UpgradeTech));
    }

    tryMajorAction(totalBuildingCount(AI, building::Barracks) < desiredBarracks
        && commandForTeam(AI) >= config::BarracksCost + (defenseMode ? 0 : config::InfantryCost),
        GameOperation(gameop::BuildBarracks, chooseLane(0)));

    tryMajorAction(defenseMode
        && totalBuildingCount(AI, building::DefenseTower) < desiredTowers
        && commandForTeam(AI) >= config::TowerCost + config::InfantryCost,
        GameOperation(gameop::BuildTower, laneWithMostPlayerUnits()));

    if (openingNeedsUnits && !majorActionTaken) {
        tryMajorAction(aiEconomyLevel < std::min(2, desiredEconomy)
            && enemys.size() >= 3
            && commandForTeam(AI) >= economyUpgradeCost(AI),
            GameOperation(gameop::UpgradeEconomy));
        tryMajorAction(aiUpgradeLevel < std::min(1, allowedTech)
            && enemys.size() >= 3
            && commandForTeam(AI) >= upgradeCostForNextLevel(AI),
            GameOperation(gameop::UpgradeTech));
    }

    int reserve = 0;
    const bool armyEmergency = defenseMode || armyBehind || enemys.size() < 10;
    if (!armyEmergency) {
        if (aiEconomyLevel < desiredEconomy) {
            reserve = std::max(reserve, std::min(economyUpgradeCost(AI), macroMode ? 230 : 170));
        }
        if (aiUpgradeLevel < allowedTech) {
            reserve = std::max(reserve, std::min(upgradeCostForNextLevel(AI), macroMode ? 270 : 190));
        }
        if (totalBuildingCount(AI, building::Barracks) < desiredBarracks) {
            reserve = std::max(reserve, config::BarracksCost);
        }
    }
    if (majorActionTaken) {
        reserve = std::min(reserve, 55);
    }
    if (!hasUnitCapacity(AI)) {
        if (majorActionTaken) {
            logEvent(std::string("ai plan=")
                + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
                + " ecoTarget=" + std::to_string(desiredEconomy)
                + " techTarget=" + std::to_string(allowedTech)
                + " raxTarget=" + std::to_string(desiredBarracks));
        }
        return;
    }

    const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
    const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
    const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
    const int playerSiege = countUnitsNamed(myunits, UName::SIEGE);
    const int playerGuardian = countUnitsNamed(myunits, UName::GUARDIAN);

    int counterPick = UName::SHOOTER;
    if (playerShooters > playerInfantry + 1 || playerSiege > 0) {
        counterPick = UName::CAVALRY;
    }
    else if (playerCavalry + playerGuardian > playerShooters) {
        counterPick = UName::INFANTARY;
    }

    std::vector<int> priorities;
    priorities.reserve(8);
    if (defenseMode) {
        if (gameTimeSeconds < 420.f) {
            priorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, counterPick, UName::GUARDIAN};
        }
        else {
            priorities = {UName::GUARDIAN, UName::INFANTARY, UName::CAVALRY, counterPick, UName::SHOOTER};
        }
    }
    else if (siegeMode) {
        priorities = {UName::SIEGE, UName::GUARDIAN, UName::CAVALRY, counterPick, UName::SHOOTER, UName::INFANTARY};
    }
    else if (gameTimeSeconds < 300.f) {
        // Rotate a small opening roster so the AI does not look like it only
        // understands infantry spam before siege/guardian tech comes online.
        priorities = {UName::SHOOTER, UName::INFANTARY, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    else {
        priorities = {counterPick, UName::SHOOTER, UName::CAVALRY, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
    }
    if (aiPressure > 0 || playerTurtling) {
        priorities.insert(priorities.begin(), UName::SIEGE);
    }

    const auto leastLoadedBarracks = [this]() {
        int bestLoad = std::numeric_limits<int>::max();
        for (const auto& building : buildings) {
            if (building.team == AI && building.type == building::Barracks && building.complete) {
                bestLoad = std::min(bestLoad, building.production.load());
            }
        }
        return bestLoad == std::numeric_limits<int>::max() ? 0 : bestLoad;
    };
    const auto aiPlannedUnitCount = [this](int unitName) {
        int total = countUnitsNamed(enemys, unitName);
        for (const auto& building : buildings) {
            if (building.team != AI || building.type != building::Barracks) {
                continue;
            }
            if (building.production.activeUnit == unitName) {
                ++total;
            }
            total += static_cast<int>(std::count_if(
                building.production.orders.begin(),
                building.production.orders.end(),
                [unitName](const ProductionOrder& order) { return order.unit == unitName; }));
        }
        return total;
    };

    if (gameTimeSeconds < 120.f && !defenseMode && leastLoadedBarracks() >= 2) {
        const int openingMacroCost = aiEconomyLevel < 1
            ? economyUpgradeCost(AI)
            : (aiUpgradeLevel < 1 ? upgradeCostForNextLevel(AI) : 0);
        if (openingMacroCost > 0 && commandForTeam(AI) < openingMacroCost) {
            logEvent("ai opening banks CMD for early economy/tech");
            return;
        }
    }

    int orders = realtime::AIUnitsPerBurst + aiUpgradeLevel / 5;
    if (aiEconomyLevel >= 4) ++orders;
    if (aiEconomyLevel >= 8) ++orders;
    if (armyBehind || defenseMode) ++orders;
    if (siegeMode) ++orders;
    if (gameTimeSeconds > 760.f) ++orders;
    orders = std::min({orders, completedBuildingCount(AI, building::Barracks), 5});

    const int queueLoadLimit = macroMode ? 3 : (defenseMode ? 6 : 4);
    for (int i = 0; i < orders; ++i) {
        if (leastLoadedBarracks() >= queueLoadLimit && commandForTeam(AI) < reserve + 160 && !armyEmergency) {
            break;
        }
        if (gameTimeSeconds > 75.f && gameTimeSeconds < 155.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::SHOOTER)
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::ShooterCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first shooter");
            break;
        }
        const bool wantsOpeningCavalry = gameTimeSeconds > 185.f && gameTimeSeconds < 380.f
            && !defenseMode
            && isUnitUnlocked(AI, UName::CAVALRY)
            && aiPlannedUnitCount(UName::CAVALRY) < 1;
        if (wantsOpeningCavalry
            && commandForTeam(AI) >= config::InfantryCost
            && commandForTeam(AI) < config::CavalryCost
            && leastLoadedBarracks() <= 2) {
            logEvent("ai opening banks CMD for first cavalry");
            break;
        }

        std::vector<int> orderPriorities = priorities;
        if (wantsOpeningCavalry && commandForTeam(AI) >= config::CavalryCost) {
            orderPriorities = {UName::CAVALRY, UName::SHOOTER, UName::INFANTARY, UName::SIEGE, UName::GUARDIAN};
        }
        if (!wantsOpeningCavalry && gameTimeSeconds < 300.f && !defenseMode && !siegeMode && !orderPriorities.empty()) {
            const auto shift = static_cast<std::ptrdiff_t>(
                (static_cast<int>(gameTimeSeconds / realtime::AIThinkSeconds) + i)
                % static_cast<int>(orderPriorities.size()));
            std::rotate(orderPriorities.begin(), orderPriorities.begin() + shift, orderPriorities.end());
        }

        bool queued = false;
        for (int code : orderPriorities) {
            const int cost = unitCost(code);
            if (cost <= 0 || !canQueueUnit(AI, code)) {
                continue;
            }
            if (!armyEmergency && commandForTeam(AI) < cost + reserve) {
                continue;
            }
            if (executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), code)}) > 0) {
                queued = true;
                break;
            }
        }
        if (!queued && armyEmergency && commandForTeam(AI) >= config::InfantryCost) {
            queued = executeOperations(AI, {GameOperation(gameop::QueueUnit, chooseLane(i), UName::INFANTARY)}) > 0;
        }
        if (!queued) {
            break;
        }
    }

    if (majorActionTaken) {
        logEvent(std::string("ai plan=")
            + (defenseMode ? "defense" : (siegeMode ? "siege" : (macroMode ? "macro" : "tempo")))
            + " ecoTarget=" + std::to_string(desiredEconomy)
            + " techTarget=" + std::to_string(allowedTech)
            + " raxTarget=" + std::to_string(desiredBarracks));
    }
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

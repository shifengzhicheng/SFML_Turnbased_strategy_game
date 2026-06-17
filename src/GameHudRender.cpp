#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "SidebarLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::DrawSidePanel()
{
    DrawSidePanel(window);
}

void Game::DrawSidePanel(sf::RenderTarget& target)
{
    helpBtn.setPosition(config::ButtonX, config::HelpButtonY);
    upgradeBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    economyBtn.setPosition(config::ButtonX, config::EconomyButtonY);
    barracksBtn.setPosition(config::ButtonX, config::BuildBarracksY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
    siegeBtn.setPosition(config::ButtonX, config::BuildSiegeY);
    guardianBtn.setPosition(config::ButtonX, config::BuildGuardianY);
    towerBtn.setPosition(config::ButtonX, config::BuildTowerY);

    target.draw(sidePanel);

    sf::RectangleShape accentLine(sf::Vector2f(3.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(226, 172, 81));
    target.draw(accentLine);

    sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(config::PanelWidth), 120.f));
    topGlow.setPosition(static_cast<float>(config::PanelX), 0.f);
    topGlow.setFillColor(sf::Color(255, 222, 138, 10));
    target.draw(topGlow);

    const float panelLeft = sidebar_layout::CardLeft;
    const float cardWidth = sidebar_layout::CardWidth;
    const auto drawPanelCard = [this, panelLeft, cardWidth, &target](float y, float h, sf::Color fill, sf::Color outline, const std::string& title) {
        sf::RectangleShape shadow(sf::Vector2f(cardWidth, h));
        shadow.setPosition(panelLeft + 2.f, y + 4.f);
        shadow.setFillColor(sf::Color(8, 11, 10, 86));
        target.draw(shadow);

        sf::RectangleShape card(sf::Vector2f(cardWidth, h));
        card.setPosition(panelLeft, y);
        card.setFillColor(fill);
        card.setOutlineColor(outline);
        card.setOutlineThickness(1.2f);
        target.draw(card);

        sf::RectangleShape inner(sf::Vector2f(cardWidth - 10.f, h - 10.f));
        inner.setPosition(panelLeft + 5.f, y + 5.f);
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineColor(sf::Color(255, 246, 200, 18));
        inner.setOutlineThickness(1.f);
        target.draw(inner);

        if (!title.empty()) {
            sf::RectangleShape titleBar(sf::Vector2f(cardWidth - 16.f, 1.4f));
            titleBar.setPosition(panelLeft + 8.f, y + 21.f);
            titleBar.setFillColor(sf::Color(outline.r, outline.g, outline.b, 120));
            target.draw(titleBar);

            sf::Text titleText(title, myfont, 10);
            titleText.setFillColor(sf::Color(255, 232, 156));
            titleText.setLetterSpacing(1.18f);
            titleText.setPosition(panelLeft + 10.f, y + 5.f);
            target.draw(titleText);
        }
    };

    drawPanelCard(sidebar_layout::HeaderCardY, sidebar_layout::HeaderCardH, sf::Color(42, 51, 45), sf::Color(104, 120, 89), "COMMAND");
    drawPanelCard(sidebar_layout::StatusCardY, sidebar_layout::StatusCardH, sf::Color(33, 42, 38), sf::Color(80, 96, 73), "STATUS");
    drawPanelCard(sidebar_layout::ActionCardY, sidebar_layout::ActionCardH, sf::Color(55, 44, 29), sf::Color(211, 158, 69), "");
    drawPanelCard(sidebar_layout::ProduceCardY, sidebar_layout::ProduceCardH, sf::Color(35, 42, 38), sf::Color(82, 95, 70), "PRODUCE / MASTERY");
    drawPanelCard(sidebar_layout::HelpCardY, sidebar_layout::HelpCardH, sf::Color(32, 39, 36), sf::Color(80, 96, 73), "");

    const auto guideText = [this]() {
        if (playerEconomyLevel == 0) {
            return std::string("Next: ECONOMY first");
        }
        if (completedBuildingCount(PLAYER, building::Barracks) == 0) {
            return std::string("Next: build Barracks");
        }
        if (playerUpgradeLevel < 1 && commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)) {
            return std::string("Next: Upgrade for cards");
        }
        if (totalBuildingCount(AI, building::DefenseTower) > 0 && !isUnitUnlocked(PLAYER, UName::SIEGE)) {
            return std::string("Enemy tower: tech Siege");
        }
        if (myunits.size() + 4 < enemys.size()) {
            return std::string("Under pressure: queue units");
        }
        if (playerEconomyLevel < config::MaxEconomyLevel && commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)) {
            return std::string("Float CMD: buy ECONOMY");
        }
        return std::string("Pick lane, keep queues busy");
    };

    panelTitle.setString("WAR ROOM");
    panelTitle.setCharacterSize(20);
    panelTitle.setFillColor(sf::Color(255, 242, 188));
    panelTitle.setOutlineColor(sf::Color(12, 15, 13, 220));
    panelTitle.setOutlineThickness(0.8f);
    panelTitle.setLetterSpacing(1.18f);
    panelTitle.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 27.f);
    Globle_text.setCharacterSize(10);
    Globle_text.setFillColor(sf::Color(221, 211, 177));
    Globle_text.setOutlineColor(sf::Color(12, 15, 13, 190));
    Globle_text.setOutlineThickness(0.8f);
    Globle_text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 55.f);
    Globle_text.setString(guideText());
    target.draw(panelTitle);
    target.draw(Globle_text);

    const bool inspectingEnemyBase = MosOnUnit == Base_blue.get();
    const int shownTeam = inspectingEnemyBase ? AI : PLAYER;
    const int shownLevel = shownTeam == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
    const auto perkLine = [this, shownTeam]() {
        std::string text;
        for (int type = 0; type < perk::Count; ++type) {
            const int level = perkLevel(shownTeam, type);
            if (level <= 0) {
                continue;
            }
            if (!text.empty()) {
                text += " ";
            }
            text += perkShortName(type);
            text += std::to_string(level);
        }
        return text.empty() ? std::string("none") : text;
    };
    const auto clampText = [](std::string text, std::size_t maxChars) {
        if (text.size() <= maxChars) {
            return text;
        }
        if (maxChars <= 2) {
            return text.substr(0, maxChars);
        }
        text.resize(maxChars - 2);
        text += "..";
        return text;
    };

    CommandText.setCharacterSize(10);
    CommandText.setFillColor(sf::Color(255, 226, 128));
    CommandText.setOutlineColor(sf::Color(11, 14, 12, 200));
    CommandText.setOutlineThickness(0.7f);
    CommandText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 101.f);
    CommandText.setString("CMD " + std::to_string(playerCommand)
        + "   +" + std::to_string(resourceIncome(PLAYER)) + "/tick"
        + "\nTECH " + std::to_string(playerUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "   AI " + std::to_string(aiUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "\nECON " + std::to_string(playerEconomyLevel)
        + "/" + std::to_string(config::MaxEconomyLevel)
        + "   DRN " + std::to_string(workerCount(PLAYER))
        + "\nRAX " + std::to_string(completedBuildingCount(PLAYER, building::Barracks))
        + "/" + std::to_string(buildingCap(PLAYER, building::Barracks))
        + "  TWR " + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower))
        + "/" + std::to_string(buildingCap(PLAYER, building::DefenseTower))
        + "  ARMY " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits));
    target.draw(CommandText);

    sf::Text perkText(std::string(inspectingEnemyBase ? "Enemy" : "Your") + " Lv" + std::to_string(shownLevel)
        + " buffs: " + clampText(perkLine(), 24), myfont, 9);
    perkText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(255, 226, 142));
    perkText.setOutlineColor(sf::Color(11, 14, 12, 200));
    perkText.setOutlineThickness(0.7f);
    perkText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 153.f);
    target.draw(perkText);

    const auto masteryBonus = [this, shownTeam](int unitName) {
        return static_cast<int>(std::round(static_cast<float>(unitMasteryLevel(shownTeam, unitName))
            * config::MasteryStatBonusPerLevel * 100.f));
    };
    sf::Text masteryText("MST I+" + std::to_string(masteryBonus(UName::INFANTARY))
        + " Sh+" + std::to_string(masteryBonus(UName::SHOOTER))
        + " Cv+" + std::to_string(masteryBonus(UName::CAVALRY))
        + " Sg+" + std::to_string(masteryBonus(UName::SIEGE))
        + " Gd+" + std::to_string(masteryBonus(UName::GUARDIAN))
        + "%", myfont, 8);
    masteryText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(151, 235, 154));
    masteryText.setOutlineColor(sf::Color(11, 14, 12, 200));
    masteryText.setOutlineThickness(0.7f);
    masteryText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 164.f);
    target.draw(masteryText);

    int playerLaneCounts[lane::Count] = {};
    int aiLaneCounts[lane::Count] = {};
    for (const auto& unit : myunits) {
        if (unit->Health > 0) {
            ++playerLaneCounts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
        }
    }
    for (const auto& unit : enemys) {
        if (unit->Health > 0) {
            ++aiLaneCounts[std::clamp(unit->laneIndex, 0, lane::Count - 1)];
        }
    }
    for (int i = 0; i < lane::Count; ++i) {
        const sf::FloatRect laneRect = sidebar_layout::laneButtonRect(i);
        sf::RectangleShape laneButton(sf::Vector2f(laneRect.width, laneRect.height));
        laneButton.setPosition(laneRect.left, laneRect.top);
        laneButton.setFillColor(playerSelectedLane == i ? sf::Color(222, 169, 77) : sf::Color(39, 50, 45));
        laneButton.setOutlineColor(playerSelectedLane == i ? sf::Color(255, 239, 158) : sf::Color(92, 109, 82));
        laneButton.setOutlineThickness(playerSelectedLane == i ? 1.8f : 1.f);
        target.draw(laneButton);

        sf::RectangleShape laneTop(sf::Vector2f(laneRect.width - 8.f, 3.f));
        laneTop.setPosition(laneRect.left + 4.f, laneRect.top + 4.f);
        laneTop.setFillColor(playerSelectedLane == i ? sf::Color(255, 239, 166, 135) : sf::Color(255, 255, 255, 22));
        target.draw(laneTop);

        sf::Text laneText(laneName(i), myfont, 10);
        laneText.setFillColor(playerSelectedLane == i ? sf::Color(41, 31, 20) : sf::Color(224, 232, 203));
        laneText.setLetterSpacing(1.12f);
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 7.f));
        target.draw(laneText);

        sf::Text laneCount(std::to_string(playerLaneCounts[i]) + "/" + std::to_string(aiLaneCounts[i]), myfont, 8);
        laneCount.setFillColor(playerSelectedLane == i ? sf::Color(64, 45, 23) : sf::Color(205, 214, 188));
        laneCount.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 22.f));
        target.draw(laneCount);
    }

    upgradeBtn.setColor(commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
        && playerUpgradeLevel < config::MaxTechLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));
    target.draw(upgradeBtn);

    economyBtn.setColor(commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));
    target.draw(economyBtn);

    economyLabel.setCharacterSize(9);
    economyLabel.setFillColor(sf::Color(244, 221, 150));
    economyLabel.setOutlineColor(sf::Color(18, 15, 10, 190));
    economyLabel.setOutlineThickness(0.7f);
    economyLabel.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EconomyButtonY + config::SideButtonHeight - 2.f);
    const auto projectedIncome = [this](int level) {
        const int bonus = level * config::EconomyIncomeStep
            + (level * level) / config::EconomyIncomeQuadraticDivisor;
        return config::BaseCommandIncome
            + static_cast<int>(std::round(static_cast<float>(bonus) * miningIncomeMultiplier(PLAYER)));
    };
    const int nextEconomyGain = playerEconomyLevel < config::MaxEconomyLevel
        ? projectedIncome(playerEconomyLevel + 1) - projectedIncome(playerEconomyLevel)
        : 0;
    economyLabel.setString(playerEconomyLevel < config::MaxEconomyLevel
        ? ("Cost " + std::to_string(economyUpgradeCost(PLAYER))
            + " | +" + std::to_string(nextEconomyGain) + "/tick +drone")
        : ("Max | " + std::to_string(resourceIncome(PLAYER)) + "/tick"));
    target.draw(economyLabel);

    sf::Text upgradeCost("Cost " + std::to_string(upgradeCostForNextLevel(PLAYER)) + " | 3 mechanic cards", myfont, 9);
    upgradeCost.setFillColor(sf::Color(244, 221, 150));
    upgradeCost.setOutlineColor(sf::Color(18, 15, 10, 190));
    upgradeCost.setOutlineThickness(0.7f);
    upgradeCost.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EndTurnButtonY + config::SideButtonHeight - 2.f);
    target.draw(upgradeCost);

    const bool canBuildBarracks = commandForTeam(PLAYER) >= buildingCommandCost(building::Barracks)
        && totalBuildingCount(PLAYER, building::Barracks) < buildingCap(PLAYER, building::Barracks);
    inf.setColor(canQueueUnit(PLAYER, UName::INFANTARY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    sho.setColor(canQueueUnit(PLAYER, UName::SHOOTER) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    cav.setColor(canQueueUnit(PLAYER, UName::CAVALRY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    siegeBtn.setColor(canQueueUnit(PLAYER, UName::SIEGE) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    guardianBtn.setColor(canQueueUnit(PLAYER, UName::GUARDIAN) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    barracksBtn.setColor(canBuildBarracks ? sf::Color::White : sf::Color(255, 255, 255, 130));
    const bool canQueueTower = commandForTeam(PLAYER) >= buildingCommandCost(building::DefenseTower)
        && totalBuildingCount(PLAYER, building::DefenseTower) < buildingCap(PLAYER, building::DefenseTower);
    towerBtn.setColor(canQueueTower ? sf::Color::White : sf::Color(255, 255, 255, 130));

    const auto setLabel = [this](sf::Text& text, const std::string& value, int buttonY) {
        text.setCharacterSize(8);
        text.setFillColor(sf::Color(235, 225, 190));
        text.setOutlineColor(sf::Color(12, 15, 13, 210));
        text.setOutlineThickness(0.7f);
        text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), static_cast<float>(buttonY + config::SideButtonHeight - 1));
        text.setString(value);
    };
    setLabel(barracksLabel, "cap "
        + std::to_string(totalBuildingCount(PLAYER, building::Barracks)) + "/"
        + std::to_string(buildingCap(PLAYER, building::Barracks)) + " | auto base", config::BuildBarracksY);
    const auto masteryLabel = [this](int unitName, const std::string& role) {
        const int level = unitMasteryLevel(PLAYER, unitName);
        const int bonus = static_cast<int>(std::round(static_cast<float>(level) * config::MasteryStatBonusPerLevel * 100.f));
        if (!isUnitUnlocked(PLAYER, unitName)) {
            return "locked | " + role;
        }
        return "M" + std::to_string(level)
            + " +" + std::to_string(bonus) + "%"
            + " | next " + std::to_string(unitMasteryUpgradeCost(PLAYER, unitName));
    };
    setLabel(infantryLabel, masteryLabel(UName::INFANTARY, "front"), config::BuildInfantryY);
    setLabel(shooterLabel, masteryLabel(UName::SHOOTER, "multi"), config::BuildShooterY);
    setLabel(cavalryLabel, masteryLabel(UName::CAVALRY, "dive"), config::BuildCavalryY);
    setLabel(siegeLabel, masteryLabel(UName::SIEGE, "breach"), config::BuildSiegeY);
    setLabel(guardianLabel, masteryLabel(UName::GUARDIAN, "tank"), config::BuildGuardianY);
    setLabel(towerLabel, "cap "
        + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower)) + "/"
        + std::to_string(buildingCap(PLAYER, building::DefenseTower)) + " | anti-rush", config::BuildTowerY);

    target.draw(helpBtn);
    target.draw(barracksBtn);
    target.draw(inf);
    target.draw(sho);
    target.draw(cav);
    target.draw(siegeBtn);
    target.draw(guardianBtn);
    target.draw(towerBtn);

    const auto drawMasteryButton = [this, &target](int unitName, int buttonY) {
        const sf::FloatRect rect = sidebar_layout::masteryButtonRect(buttonY);
        const bool enabled = canUpgradeUnitMastery(PLAYER, unitName);
        const bool locked = !isUnitUnlocked(PLAYER, unitName);
        const int level = unitMasteryLevel(PLAYER, unitName);
        const int bonus = static_cast<int>(std::round(static_cast<float>(level)
            * config::MasteryStatBonusPerLevel * 100.f));

        sf::RectangleShape shadow(sf::Vector2f(rect.width, rect.height));
        shadow.setPosition(rect.left + 2.f, rect.top + 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 105));
        target.draw(shadow);

        sf::RectangleShape pill(sf::Vector2f(rect.width, rect.height));
        pill.setPosition(rect.left, rect.top);
        pill.setFillColor(enabled ? sf::Color(210, 147, 44, 246)
            : (locked ? sf::Color(46, 50, 46, 232) : sf::Color(71, 62, 43, 232)));
        pill.setOutlineColor(enabled ? sf::Color(255, 240, 158)
            : (locked ? sf::Color(86, 96, 84) : sf::Color(161, 124, 61)));
        pill.setOutlineThickness(enabled ? 2.f : 1.2f);
        target.draw(pill);

        sf::RectangleShape shine(sf::Vector2f(rect.width - 10.f, 4.f));
        shine.setPosition(rect.left + 5.f, rect.top + 5.f);
        shine.setFillColor(enabled ? sf::Color(255, 250, 200, 86) : sf::Color(255, 255, 255, 22));
        target.draw(shine);

        sf::Text upText(locked ? "LOCK" : ("M" + std::to_string(level)), myfont, locked ? 9 : 11);
        upText.setFillColor(enabled ? sf::Color(45, 29, 12) : sf::Color(212, 199, 158));
        upText.setPosition(rect.left + (locked ? 14.f : 22.f), rect.top + 2.f);
        target.draw(upText);

        sf::Text bonusText("+" + std::to_string(bonus) + "%", myfont, 9);
        bonusText.setFillColor(enabled ? sf::Color(46, 31, 17) : sf::Color(190, 184, 154));
        bonusText.setPosition(rect.left + 15.f, rect.top + 17.f);
        target.draw(bonusText);

        if (enabled) {
            sf::Text plusText("+", myfont, 11);
            plusText.setFillColor(sf::Color(63, 40, 18));
            plusText.setPosition(rect.left + 5.f, rect.top + 2.f);
            target.draw(plusText);
        }
    };
    drawMasteryButton(UName::INFANTARY, config::BuildInfantryY);
    drawMasteryButton(UName::SHOOTER, config::BuildShooterY);
    drawMasteryButton(UName::CAVALRY, config::BuildCavalryY);
    drawMasteryButton(UName::SIEGE, config::BuildSiegeY);
    drawMasteryButton(UName::GUARDIAN, config::BuildGuardianY);
}

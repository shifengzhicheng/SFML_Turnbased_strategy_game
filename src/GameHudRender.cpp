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
#include <string>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    constexpr sf::Uint8 SubtlePanelAlpha = 34;
}

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

    sf::RectangleShape leftShade(sf::Vector2f(16.f, static_cast<float>(config::WindowHeight)));
    leftShade.setPosition(static_cast<float>(config::PanelX), 0.f);
    leftShade.setFillColor(sf::Color(0, 0, 0, 82));
    target.draw(leftShade);

    sf::RectangleShape accentLine(sf::Vector2f(4.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(231, 173, 74));
    target.draw(accentLine);

    sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(config::PanelWidth), 120.f));
    topGlow.setPosition(static_cast<float>(config::PanelX), 0.f);
    topGlow.setFillColor(sf::Color(255, 222, 138, 13));
    target.draw(topGlow);

    for (int y = 0; y < config::WindowHeight; y += 20) {
        sf::RectangleShape dash(sf::Vector2f(static_cast<float>(config::PanelWidth - 42), 1.f));
        dash.setPosition(static_cast<float>(config::PanelX + 22), static_cast<float>(y + 9));
        dash.setFillColor(sf::Color(255, 245, 190, y % 40 == 0 ? 12 : 5));
        target.draw(dash);
    }

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
        card.setOutlineThickness(1.4f);
        target.draw(card);

        sf::RectangleShape topBand(sf::Vector2f(cardWidth - 4.f, 18.f));
        topBand.setPosition(panelLeft + 2.f, y + 2.f);
        topBand.setFillColor(sf::Color(outline.r, outline.g, outline.b, title.empty() ? 18 : SubtlePanelAlpha));
        target.draw(topBand);

        sf::RectangleShape inner(sf::Vector2f(cardWidth - 10.f, h - 10.f));
        inner.setPosition(panelLeft + 5.f, y + 5.f);
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineColor(sf::Color(255, 246, 200, 18));
        inner.setOutlineThickness(1.f);
        target.draw(inner);

        const sf::Color corner(outline.r, outline.g, outline.b, 145);
        const sf::Vector2f cornerSize(12.f, 2.f);
        sf::RectangleShape c(cornerSize);
        c.setFillColor(corner);
        c.setPosition(panelLeft + 7.f, y + 7.f);
        target.draw(c);
        c.setPosition(panelLeft + cardWidth - 19.f, y + 7.f);
        target.draw(c);
        c.setPosition(panelLeft + 7.f, y + h - 9.f);
        target.draw(c);
        c.setPosition(panelLeft + cardWidth - 19.f, y + h - 9.f);
        target.draw(c);

        if (!title.empty()) {
            sf::RectangleShape titleBar(sf::Vector2f(cardWidth - 16.f, 1.4f));
            titleBar.setPosition(panelLeft + 8.f, y + 21.f);
            titleBar.setFillColor(sf::Color(outline.r, outline.g, outline.b, 120));
            target.draw(titleBar);

            sf::RectangleShape titleDot(sf::Vector2f(4.f, 8.f));
            titleDot.setPosition(panelLeft + 9.f, y + 7.f);
            titleDot.setFillColor(sf::Color(255, 218, 112, 190));
            target.draw(titleDot);

            sf::Text titleText(title, myfont, 10);
            titleText.setFillColor(sf::Color(255, 232, 156));
            titleText.setLetterSpacing(1.18f);
            titleText.setPosition(panelLeft + 18.f, y + 5.f);
            target.draw(titleText);
        }
    };

    drawPanelCard(sidebar_layout::HeaderCardY, sidebar_layout::HeaderCardH, sf::Color(38, 50, 44), sf::Color(108, 128, 91), "COMMAND");
    drawPanelCard(sidebar_layout::StatusCardY, sidebar_layout::StatusCardH, sf::Color(29, 39, 35), sf::Color(83, 102, 76), "STATUS");
    drawPanelCard(sidebar_layout::ActionCardY, sidebar_layout::ActionCardH, sf::Color(58, 45, 28), sf::Color(225, 168, 69), "");
    drawPanelCard(sidebar_layout::ProduceCardY, sidebar_layout::ProduceCardH, sf::Color(31, 41, 37), sf::Color(87, 105, 75), "PRODUCE / MASTERY");
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

    sf::RectangleShape liveBadge(sf::Vector2f(70.f, 16.f));
    liveBadge.setPosition(static_cast<float>(config::PanelX + config::PanelWidth - 88), 30.f);
    liveBadge.setFillColor(sf::Color(27, 35, 31, 235));
    liveBadge.setOutlineColor(sf::Color(225, 169, 74, 155));
    liveBadge.setOutlineThickness(1.f);
    target.draw(liveBadge);
    sf::Text liveText(std::string("LIVE ") + laneName(playerSelectedLane), myfont, 8);
    liveText.setFillColor(sf::Color(255, 226, 142));
    liveText.setPosition(liveBadge.getPosition() + sf::Vector2f(7.f, 3.f));
    target.draw(liveText);

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

    const auto drawStatChip = [this, &target](float x, float y, float w,
                                               const std::string& label, const std::string& value,
                                               sf::Color accent, bool strong = false) {
        sf::RectangleShape shadow(sf::Vector2f(w, 17.f));
        shadow.setPosition(x + 1.f, y + 2.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 82));
        target.draw(shadow);

        sf::RectangleShape chip(sf::Vector2f(w, 17.f));
        chip.setPosition(x, y);
        chip.setFillColor(strong ? sf::Color(62, 48, 27, 238) : sf::Color(24, 33, 30, 234));
        chip.setOutlineColor(strong ? sf::Color(242, 184, 82, 190) : sf::Color(91, 105, 79, 160));
        chip.setOutlineThickness(1.f);
        target.draw(chip);

        sf::RectangleShape rail(sf::Vector2f(4.f, 13.f));
        rail.setPosition(x + 3.f, y + 2.f);
        rail.setFillColor(accent);
        target.draw(rail);

        sf::Text labelText(label, myfont, 8);
        labelText.setFillColor(sf::Color(187, 197, 166));
        labelText.setPosition(x + 10.f, y + 2.f);
        target.draw(labelText);

        sf::Text valueText(value, myfont, strong ? 12 : 10);
        valueText.setFillColor(strong ? sf::Color(255, 231, 133) : sf::Color(239, 232, 201));
        valueText.setOutlineColor(sf::Color(10, 13, 11, 190));
        valueText.setOutlineThickness(0.5f);
        const auto bounds = valueText.getLocalBounds();
        valueText.setPosition(x + w - bounds.width - bounds.left - 7.f, y + (strong ? 0.f : 2.f));
        target.draw(valueText);
    };

    const float statusX = static_cast<float>(config::PanelX + config::PanelPadding);
    drawStatChip(statusX, 108.f, 112.f, "CMD", std::to_string(playerCommand), sf::Color(255, 210, 91), true);
    drawStatChip(statusX + 118.f, 108.f, 102.f, "FLOW", "+" + std::to_string(resourceIncome(PLAYER)) + "/t", sf::Color(118, 209, 134), true);
    drawStatChip(statusX, 130.f, 68.f, "TECH", std::to_string(playerUpgradeLevel) + "/" + std::to_string(config::MaxTechLevel), sf::Color(255, 193, 86));
    drawStatChip(statusX + 74.f, 130.f, 68.f, "AI", std::to_string(aiUpgradeLevel) + "/" + std::to_string(config::MaxTechLevel), sf::Color(116, 181, 255));
    drawStatChip(statusX + 148.f, 130.f, 72.f, "ECON", std::to_string(playerEconomyLevel) + "/" + std::to_string(config::MaxEconomyLevel), sf::Color(136, 214, 116));
    drawStatChip(statusX, 148.f, 68.f, "RAX", std::to_string(completedBuildingCount(PLAYER, building::Barracks)) + "/" + std::to_string(buildingCap(PLAYER, building::Barracks)), sf::Color(224, 150, 72));
    drawStatChip(statusX + 74.f, 148.f, 68.f, "TWR", std::to_string(totalBuildingCount(PLAYER, building::DefenseTower)) + "/" + std::to_string(buildingCap(PLAYER, building::DefenseTower)), sf::Color(239, 202, 107));
    drawStatChip(statusX + 148.f, 148.f, 72.f, "ARMY", std::to_string(myunits.size()) + "/" + std::to_string(config::MaxUnits), sf::Color(222, 86, 68));

    sf::RectangleShape buffStrip(sf::Vector2f(220.f, 12.f));
    buffStrip.setPosition(statusX, 167.f);
    buffStrip.setFillColor(sf::Color(17, 24, 21, 180));
    buffStrip.setOutlineColor(inspectingEnemyBase ? sf::Color(80, 140, 206, 120) : sf::Color(212, 152, 60, 120));
    buffStrip.setOutlineThickness(1.f);
    target.draw(buffStrip);

    sf::Text perkText(std::string(inspectingEnemyBase ? "ENEMY" : "YOUR") + " Lv" + std::to_string(shownLevel)
        + " BUFFS " + clampText(perkLine(), 24), myfont, 8);
    perkText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(255, 226, 142));
    perkText.setOutlineColor(sf::Color(11, 14, 12, 200));
    perkText.setOutlineThickness(0.6f);
    perkText.setPosition(statusX + 5.f, 166.5f);
    target.draw(perkText);

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
        laneButton.setFillColor(playerSelectedLane == i ? sf::Color(211, 154, 62) : sf::Color(33, 45, 40));
        laneButton.setOutlineColor(playerSelectedLane == i ? sf::Color(255, 239, 158) : sf::Color(88, 108, 80));
        laneButton.setOutlineThickness(playerSelectedLane == i ? 1.8f : 1.f);
        target.draw(laneButton);

        sf::RectangleShape laneTop(sf::Vector2f(laneRect.width - 8.f, 3.f));
        laneTop.setPosition(laneRect.left + 4.f, laneRect.top + 4.f);
        laneTop.setFillColor(playerSelectedLane == i ? sf::Color(255, 239, 166, 135) : sf::Color(255, 255, 255, 22));
        target.draw(laneTop);

        const int laneTotal = std::max(1, playerLaneCounts[i] + aiLaneCounts[i]);
        const float meterWidth = laneRect.width - 12.f;
        const float playerWidth = meterWidth * static_cast<float>(playerLaneCounts[i]) / static_cast<float>(laneTotal);
        sf::RectangleShape laneMeterBack(sf::Vector2f(meterWidth, 4.f));
        laneMeterBack.setPosition(laneRect.left + 6.f, laneRect.top + laneRect.height - 7.f);
        laneMeterBack.setFillColor(sf::Color(11, 16, 14, 155));
        target.draw(laneMeterBack);
        sf::RectangleShape lanePlayer(sf::Vector2f(std::max(2.f, playerWidth), 4.f));
        lanePlayer.setPosition(laneMeterBack.getPosition());
        lanePlayer.setFillColor(sf::Color(218, 76, 60, playerLaneCounts[i] > 0 ? 210 : 70));
        target.draw(lanePlayer);
        sf::RectangleShape laneEnemy(sf::Vector2f(std::max(2.f, meterWidth - playerWidth), 4.f));
        laneEnemy.setPosition(laneMeterBack.getPosition() + sf::Vector2f(meterWidth - laneEnemy.getSize().x, 0.f));
        laneEnemy.setFillColor(sf::Color(83, 147, 226, aiLaneCounts[i] > 0 ? 210 : 70));
        target.draw(laneEnemy);

        if (playerSelectedLane == i) {
            sf::ConvexShape marker(3);
            marker.setPoint(0, sf::Vector2f(laneRect.left + laneRect.width - 10.f, laneRect.top + 9.f));
            marker.setPoint(1, sf::Vector2f(laneRect.left + laneRect.width - 4.f, laneRect.top + 15.f));
            marker.setPoint(2, sf::Vector2f(laneRect.left + laneRect.width - 10.f, laneRect.top + 21.f));
            marker.setFillColor(sf::Color(46, 33, 18));
            target.draw(marker);
        }

        sf::Text laneText(laneName(i), myfont, 10);
        laneText.setFillColor(playerSelectedLane == i ? sf::Color(41, 31, 20) : sf::Color(224, 232, 203));
        laneText.setLetterSpacing(1.12f);
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 7.f));
        target.draw(laneText);

        sf::Text laneCount("P" + std::to_string(playerLaneCounts[i]) + " E" + std::to_string(aiLaneCounts[i]), myfont, 8);
        laneCount.setFillColor(playerSelectedLane == i ? sf::Color(64, 45, 23) : sf::Color(205, 214, 188));
        laneCount.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 21.f));
        target.draw(laneCount);
    }

    upgradeBtn.setColor(commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
        && playerUpgradeLevel < config::MaxTechLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));

    economyBtn.setColor(commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));

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

    sf::Text upgradeCost("Cost " + std::to_string(upgradeCostForNextLevel(PLAYER)) + " | 3 mechanic cards", myfont, 9);
    upgradeCost.setFillColor(sf::Color(244, 221, 150));
    upgradeCost.setOutlineColor(sf::Color(18, 15, 10, 190));
    upgradeCost.setOutlineThickness(0.7f);
    upgradeCost.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EndTurnButtonY + config::SideButtonHeight - 2.f);

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

    const auto drawButtonBackplate = [&target](int buttonY, bool available) {
        const float h = static_cast<float>(config::SideButtonHeight);
        sf::RectangleShape plate(sf::Vector2f(static_cast<float>(config::SideButtonWidth) + 8.f, h));
        plate.setPosition(static_cast<float>(config::ButtonX - 4), static_cast<float>(buttonY));
        plate.setFillColor(available ? sf::Color(70, 53, 31, 70) : sf::Color(10, 14, 12, 56));
        plate.setOutlineColor(available ? sf::Color(233, 179, 80, 70) : sf::Color(88, 100, 82, 45));
        plate.setOutlineThickness(1.f);
        target.draw(plate);
    };
    drawButtonBackplate(config::EconomyButtonY, commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel);
    drawButtonBackplate(config::EndTurnButtonY, commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
        && playerUpgradeLevel < config::MaxTechLevel);
    drawButtonBackplate(config::BuildBarracksY, canBuildBarracks);
    drawButtonBackplate(config::BuildInfantryY, canQueueUnit(PLAYER, UName::INFANTARY));
    drawButtonBackplate(config::BuildShooterY, canQueueUnit(PLAYER, UName::SHOOTER));
    drawButtonBackplate(config::BuildCavalryY, canQueueUnit(PLAYER, UName::CAVALRY));
    drawButtonBackplate(config::BuildSiegeY, canQueueUnit(PLAYER, UName::SIEGE));
    drawButtonBackplate(config::BuildGuardianY, canQueueUnit(PLAYER, UName::GUARDIAN));
    drawButtonBackplate(config::BuildTowerY, canQueueTower);

    target.draw(helpBtn);
    target.draw(economyBtn);
    target.draw(upgradeBtn);
    target.draw(economyLabel);
    target.draw(upgradeCost);
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

        if (enabled) {
            sf::RectangleShape leftPulse(sf::Vector2f(3.f, rect.height - 10.f));
            leftPulse.setPosition(rect.left + 5.f, rect.top + 5.f);
            leftPulse.setFillColor(sf::Color(255, 239, 147, 140));
            target.draw(leftPulse);
        }

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

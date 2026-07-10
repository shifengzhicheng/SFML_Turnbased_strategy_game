#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "SidebarLayout.h"
#include "UnitDefinition.h"

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
    DrawSidePanel(renderTarget());
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
    constexpr float actionButtonScaleY = 32.f / static_cast<float>(config::SideButtonHeight);
    economyBtn.setScale(1.f, actionButtonScaleY);
    upgradeBtn.setScale(1.f, actionButtonScaleY);

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
        const int incoming = unitsNearPoint(AI, Red_baseP, config::CommandZonePressureRadius);
        const int outgoing = unitsNearPoint(PLAYER, Blue_baseP, config::CommandZonePressureRadius);
        if (incoming > 0) {
            return std::string("HQ under pressure: ") + std::to_string(incoming) + " attackers";
        }
        if (outgoing > 0) {
            return std::string("Enemy HQ pressured by ") + std::to_string(outgoing) + " units";
        }
        if (finalAssaultActive()) {
            return std::string("Final assault: all lanes commit to HQ");
        }
        if (gameTimeSeconds >= config::EscalationStartSeconds) {
            const int pressurePercent = static_cast<int>(std::round(structureDamageEscalation() * 100.f));
            return std::string("Overtime structure damage ") + std::to_string(pressurePercent) + "%";
        }
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

    const float waveRemainingRaw = config::ArmyWaveIntervalSeconds
        - std::fmod(std::max(0.f, gameTimeSeconds), config::ArmyWaveIntervalSeconds);
    const int waveRemaining = static_cast<int>(std::ceil(waveRemainingRaw));
    const bool overtime = gameTimeSeconds >= config::EscalationStartSeconds;
    const std::string liveStatus = finalAssaultActive()
        ? "FINAL"
        : (overtime
            ? ("OT " + std::to_string(static_cast<int>(std::round(structureDamageEscalation() * 100.f))) + "%")
            : ("WAVE " + std::to_string(waveRemaining) + "s"));
    sf::RectangleShape liveBadge(sf::Vector2f(100.f, 17.f));
    liveBadge.setPosition(static_cast<float>(config::PanelX + config::PanelWidth - 116), 29.f);
    liveBadge.setFillColor(sf::Color(27, 35, 31, 235));
    liveBadge.setOutlineColor(sf::Color(225, 169, 74, 155));
    liveBadge.setOutlineThickness(1.f);
    target.draw(liveBadge);
    sf::Text liveText(liveStatus + " " + laneName(playerSelectedLane), myfont, 8);
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
        const float height = strong ? 22.f : 16.f;
        sf::RectangleShape shadow(sf::Vector2f(w, height));
        shadow.setPosition(x + 1.f, y + 2.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 82));
        target.draw(shadow);

        sf::RectangleShape chip(sf::Vector2f(w, height));
        chip.setPosition(x, y);
        chip.setFillColor(strong ? sf::Color(62, 48, 27, 238) : sf::Color(24, 33, 30, 234));
        chip.setOutlineColor(strong ? sf::Color(242, 184, 82, 190) : sf::Color(91, 105, 79, 160));
        chip.setOutlineThickness(1.f);
        target.draw(chip);

        sf::RectangleShape rail(sf::Vector2f(4.f, height - 4.f));
        rail.setPosition(x + 3.f, y + 2.f);
        rail.setFillColor(accent);
        target.draw(rail);

        sf::Text labelText(label, myfont, strong ? 9 : 8);
        labelText.setFillColor(sf::Color(187, 197, 166));
        labelText.setPosition(x + 10.f, y + 2.f);
        target.draw(labelText);

        sf::Text valueText(value, myfont, strong ? 15 : 10);
        valueText.setFillColor(strong ? sf::Color(255, 231, 133) : sf::Color(239, 232, 201));
        valueText.setOutlineColor(sf::Color(10, 13, 11, 190));
        valueText.setOutlineThickness(0.5f);
        const auto bounds = valueText.getLocalBounds();
        valueText.setPosition(x + w - bounds.width - bounds.left - 7.f, y + (strong ? 1.f : 1.f));
        target.draw(valueText);
    };

    const float statusX = static_cast<float>(config::PanelX + config::PanelPadding);
    const auto drawBaseBar = [this, &target, statusX](float y, const std::string& label,
                                                      const DisMoveableUnit* base, sf::Color accent,
                                                      const std::string& rightText) {
        constexpr float barWidth = 220.f;
        constexpr float barHeight = 14.f;
        const int health = base != nullptr ? std::max(0, base->Health) : 0;
        const float ratio = std::clamp(static_cast<float>(health) / static_cast<float>(config::BaseHealth), 0.f, 1.f);
        sf::RectangleShape back(sf::Vector2f(barWidth, barHeight));
        back.setPosition(statusX, y);
        back.setFillColor(sf::Color(14, 20, 18, 232));
        back.setOutlineColor(sf::Color(accent.r, accent.g, accent.b, 125));
        back.setOutlineThickness(1.f);
        target.draw(back);
        sf::RectangleShape fill(sf::Vector2f((barWidth - 2.f) * ratio, barHeight - 2.f));
        fill.setPosition(statusX + 1.f, y + 1.f);
        fill.setFillColor(sf::Color(accent.r, accent.g, accent.b, 125));
        target.draw(fill);
        sf::Text left(label + " " + std::to_string(health) + "/" + std::to_string(config::BaseHealth), myfont, 8);
        left.setFillColor(sf::Color(248, 239, 202));
        left.setOutlineColor(sf::Color(10, 13, 11, 220));
        left.setOutlineThickness(0.7f);
        left.setPosition(statusX + 5.f, y + 1.f);
        target.draw(left);
        sf::Text right(rightText, myfont, 8);
        right.setFillColor(sf::Color(248, 239, 202));
        right.setOutlineColor(sf::Color(10, 13, 11, 220));
        right.setOutlineThickness(0.7f);
        const auto bounds = right.getLocalBounds();
        right.setPosition(statusX + barWidth - bounds.width - bounds.left - 5.f, y + 1.f);
        target.draw(right);
    };
    const std::string playerRelief = "R" + std::to_string(playerReliefCharges)
        + (playerBaseShieldTimer > 0.f ? " S" + std::to_string(static_cast<int>(std::ceil(playerBaseShieldTimer))) : "");
    drawBaseBar(108.f, "HQ", Base_red.get(), sf::Color(222, 83, 61), playerRelief);
    drawBaseBar(126.f, "ENEMY", Base_blue.get(), sf::Color(73, 143, 224), "T" + std::to_string(aiUpgradeLevel));

    drawStatChip(statusX, 146.f, 108.f, "CMD", std::to_string(playerCommand), sf::Color(255, 210, 91), true);
    drawStatChip(statusX + 114.f, 146.f, 106.f, "FLOW", "+" + std::to_string(resourceIncome(PLAYER)), sf::Color(118, 209, 134), true);
    drawStatChip(statusX, 172.f, 70.f, "TECH", std::to_string(playerUpgradeLevel) + "/" + std::to_string(config::MaxTechLevel), sf::Color(255, 193, 86));
    drawStatChip(statusX + 75.f, 172.f, 70.f, "ECON", std::to_string(playerEconomyLevel) + "/" + std::to_string(config::MaxEconomyLevel), sf::Color(136, 214, 116));
    drawStatChip(statusX + 150.f, 172.f, 70.f, "ARMY", std::to_string(myunits.size()), sf::Color(222, 86, 68));

    sf::RectangleShape buffStrip(sf::Vector2f(220.f, 8.f));
    buffStrip.setPosition(statusX, 190.f);
    buffStrip.setFillColor(sf::Color(17, 24, 21, 180));
    buffStrip.setOutlineColor(inspectingEnemyBase ? sf::Color(80, 140, 206, 120) : sf::Color(212, 152, 60, 120));
    buffStrip.setOutlineThickness(1.f);
    target.draw(buffStrip);

    sf::Text perkText(std::string(inspectingEnemyBase ? "ENEMY" : "YOUR") + " Lv" + std::to_string(shownLevel)
        + " BUFFS " + clampText(perkLine(), 27), myfont, 7);
    perkText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(255, 226, 142));
    perkText.setOutlineColor(sf::Color(11, 14, 12, 200));
    perkText.setOutlineThickness(0.6f);
    perkText.setPosition(statusX + 5.f, 189.f);
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
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 5.f));
        target.draw(laneText);

        sf::Text laneCount("P" + std::to_string(playerLaneCounts[i]) + " E" + std::to_string(aiLaneCounts[i]), myfont, 8);
        laneCount.setFillColor(playerSelectedLane == i ? sf::Color(64, 45, 23) : sf::Color(205, 214, 188));
        laneCount.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 19.f));
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

    economyLabel.setCharacterSize(8);
    economyLabel.setFillColor(sf::Color(244, 221, 150));
    economyLabel.setOutlineColor(sf::Color(18, 15, 10, 190));
    economyLabel.setOutlineThickness(0.7f);
    economyLabel.setPosition(static_cast<float>(config::PanelX + config::PanelPadding + 5), config::EconomyButtonY + 32.f);
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
        ? (std::to_string(economyUpgradeCost(PLAYER)) + " CMD | +"
            + std::to_string(nextEconomyGain) + " FLOW | +1 drone")
        : ("MAX ECONOMY | " + std::to_string(resourceIncome(PLAYER)) + " FLOW"));

    sf::Text upgradeCost(std::to_string(upgradeCostForNextLevel(PLAYER)) + " CMD | choose 1 of 3 tactics", myfont, 8);
    upgradeCost.setFillColor(sf::Color(244, 221, 150));
    upgradeCost.setOutlineColor(sf::Color(18, 15, 10, 190));
    upgradeCost.setOutlineThickness(0.7f);
    upgradeCost.setPosition(static_cast<float>(config::PanelX + config::PanelPadding + 5), config::EndTurnButtonY + 32.f);

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

    const auto drawButtonBackplate = [&target](int buttonY, bool available, float h = static_cast<float>(config::SideButtonHeight)) {
        sf::RectangleShape plate(sf::Vector2f(static_cast<float>(config::SideButtonWidth) + 8.f, h));
        plate.setPosition(static_cast<float>(config::ButtonX - 4), static_cast<float>(buttonY));
        plate.setFillColor(available ? sf::Color(70, 53, 31, 70) : sf::Color(10, 14, 12, 56));
        plate.setOutlineColor(available ? sf::Color(233, 179, 80, 70) : sf::Color(88, 100, 82, 45));
        plate.setOutlineThickness(1.f);
        target.draw(plate);
    };
    drawButtonBackplate(config::EconomyButtonY, commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel, 32.f);
    drawButtonBackplate(config::EndTurnButtonY, commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
        && playerUpgradeLevel < config::MaxTechLevel, 32.f);
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
        const int nextCost = unitMasteryUpgradeCost(PLAYER, unitName);
        const int bonus = static_cast<int>(std::round(static_cast<float>(level)
            * config::MasteryStatBonusPerLevel * 100.f));

        sf::RectangleShape shadow(sf::Vector2f(rect.width, rect.height));
        shadow.setPosition(rect.left + 2.f, rect.top + 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 105));
        target.draw(shadow);

        sf::RectangleShape pill(sf::Vector2f(rect.width, rect.height));
        pill.setPosition(rect.left, rect.top);
        pill.setFillColor(enabled ? sf::Color(77, 61, 35, 246)
            : (locked ? sf::Color(46, 50, 46, 232) : sf::Color(71, 62, 43, 232)));
        pill.setOutlineColor(enabled ? sf::Color(222, 174, 78)
            : (locked ? sf::Color(86, 96, 84) : sf::Color(161, 124, 61)));
        pill.setOutlineThickness(enabled ? 1.5f : 1.2f);
        target.draw(pill);

        sf::RectangleShape shine(sf::Vector2f(rect.width - 10.f, 4.f));
        shine.setPosition(rect.left + 5.f, rect.top + 5.f);
        shine.setFillColor(enabled ? sf::Color(255, 229, 139, 44) : sf::Color(255, 255, 255, 22));
        target.draw(shine);

        if (enabled) {
            sf::RectangleShape leftPulse(sf::Vector2f(3.f, rect.height - 10.f));
            leftPulse.setPosition(rect.left + 5.f, rect.top + 5.f);
            leftPulse.setFillColor(sf::Color(244, 189, 79, 205));
            target.draw(leftPulse);
        }

        sf::Text upText(locked
            ? "LOCKED"
            : ("M" + std::to_string(level) + " > M" + std::to_string(level + 1)), myfont, locked ? 8 : 9);
        upText.setFillColor(enabled ? sf::Color(255, 226, 143) : sf::Color(212, 199, 158));
        upText.setPosition(rect.left + (locked ? 17.f : 10.f), rect.top + 1.f);
        target.draw(upText);

        const std::string detail = locked
            ? ("TECH " + std::to_string(unitDefinition(unitName).requiredTechLevel))
            : ("+" + std::to_string(bonus) + "% | " + std::to_string(nextCost));
        sf::Text bonusText(detail, myfont, 8);
        bonusText.setFillColor(enabled ? sf::Color(235, 218, 170) : sf::Color(190, 184, 154));
        bonusText.setPosition(rect.left + (locked ? 17.f : 9.f), rect.top + 16.f);
        target.draw(bonusText);
    };
    drawMasteryButton(UName::INFANTARY, config::BuildInfantryY);
    drawMasteryButton(UName::SHOOTER, config::BuildShooterY);
    drawMasteryButton(UName::CAVALRY, config::BuildCavalryY);
    drawMasteryButton(UName::SIEGE, config::BuildSiegeY);
    drawMasteryButton(UName::GUARDIAN, config::BuildGuardianY);
}

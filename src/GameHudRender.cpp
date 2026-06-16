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

void Game::DrawSidePanel()
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

    window.draw(sidePanel);

    sf::RectangleShape accentLine(sf::Vector2f(4.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(219, 166, 75));
    window.draw(accentLine);

    sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(config::PanelWidth), 120.f));
    topGlow.setPosition(static_cast<float>(config::PanelX), 0.f);
    topGlow.setFillColor(sf::Color(255, 222, 138, 14));
    window.draw(topGlow);

    const float panelLeft = static_cast<float>(config::PanelX + 12);
    const float cardWidth = static_cast<float>(config::PanelWidth - 24);
    const auto drawPanelCard = [this, panelLeft, cardWidth](float y, float h, sf::Color fill, sf::Color outline, const std::string& title) {
        sf::RectangleShape shadow(sf::Vector2f(cardWidth, h));
        shadow.setPosition(panelLeft + 2.f, y + 4.f);
        shadow.setFillColor(sf::Color(8, 11, 10, 86));
        window.draw(shadow);

        sf::RectangleShape card(sf::Vector2f(cardWidth, h));
        card.setPosition(panelLeft, y);
        card.setFillColor(fill);
        card.setOutlineColor(outline);
        card.setOutlineThickness(1.4f);
        window.draw(card);

        if (!title.empty()) {
            sf::RectangleShape titleBar(sf::Vector2f(cardWidth - 16.f, 1.4f));
            titleBar.setPosition(panelLeft + 8.f, y + 21.f);
            titleBar.setFillColor(sf::Color(outline.r, outline.g, outline.b, 120));
            window.draw(titleBar);

            sf::Text titleText(title, myfont, 10);
            titleText.setFillColor(sf::Color(255, 232, 156));
            titleText.setLetterSpacing(1.18f);
            titleText.setPosition(panelLeft + 10.f, y + 5.f);
            window.draw(titleText);
        }
    };

    drawPanelCard(8.f, 70.f, sf::Color(47, 58, 51), sf::Color(107, 118, 91), "COMMAND");
    drawPanelCard(84.f, 136.f, sf::Color(40, 50, 45), sf::Color(84, 99, 78), "STATUS");
    drawPanelCard(222.f, 96.f, sf::Color(61, 48, 31), sf::Color(205, 156, 70), "");
    drawPanelCard(322.f, 340.f, sf::Color(42, 49, 44), sf::Color(86, 98, 75), "");
    drawPanelCard(668.f, 44.f, sf::Color(37, 45, 41), sf::Color(86, 98, 75), "");

    panelTitle.setCharacterSize(20);
    panelTitle.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 28.f);
    Globle_text.setCharacterSize(13);
    Globle_text.setFillColor(sf::Color(221, 211, 177));
    Globle_text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 54.f);
    window.draw(panelTitle);
    window.draw(Globle_text);

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

    CommandText.setCharacterSize(11);
    CommandText.setFillColor(sf::Color(255, 226, 128));
    CommandText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 106.f);
    CommandText.setString("CMD " + std::to_string(playerCommand)
        + "/" + std::to_string(config::MaxCommand)
        + "   +" + std::to_string(resourceIncome(PLAYER)) + "/tick"
        + "\nTech P " + std::to_string(playerUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "  AI " + std::to_string(aiUpgradeLevel)
        + "/" + std::to_string(config::MaxTechLevel)
        + "\nEco  P " + std::to_string(playerEconomyLevel)
        + "/" + std::to_string(config::MaxEconomyLevel)
        + "  AI " + std::to_string(aiEconomyLevel)
        + "/" + std::to_string(config::MaxEconomyLevel)
        + "\nDrone " + std::to_string(workerCount(PLAYER))
        + "/" + std::to_string(realtime::MaxWorkers)
        + "  Army " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits)
        + "\nRax " + std::to_string(completedBuildingCount(PLAYER, building::Barracks))
        + "/" + std::to_string(buildingCap(PLAYER, building::Barracks))
        + "  Tower " + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower))
        + "/" + std::to_string(buildingCap(PLAYER, building::DefenseTower)));
    window.draw(CommandText);

    sf::Text perkText(std::string(inspectingEnemyBase ? "Enemy" : "Your") + " Lv" + std::to_string(shownLevel)
        + " buffs: " + clampText(perkLine(), 24), myfont, 10);
    perkText.setFillColor(inspectingEnemyBase ? sf::Color(149, 203, 255) : sf::Color(255, 226, 142));
    perkText.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 169.f);
    window.draw(perkText);

    const float laneY = 190.f;
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
        sf::RectangleShape laneButton(sf::Vector2f(56.f, 24.f));
        laneButton.setPosition(config::PanelX + 17.f + static_cast<float>(i) * 64.f, laneY);
        laneButton.setFillColor(playerSelectedLane == i ? sf::Color(217, 166, 75) : sf::Color(48, 60, 52));
        laneButton.setOutlineColor(playerSelectedLane == i ? sf::Color(255, 236, 164) : sf::Color(111, 128, 99));
        laneButton.setOutlineThickness(playerSelectedLane == i ? 1.8f : 0.9f);
        window.draw(laneButton);

        sf::Text laneText(laneName(i), myfont, 10);
        laneText.setFillColor(playerSelectedLane == i ? sf::Color(41, 31, 20) : sf::Color(224, 232, 203));
        laneText.setPosition(laneButton.getPosition() + sf::Vector2f(7.f, 1.f));
        window.draw(laneText);

        sf::Text laneCount(std::to_string(playerLaneCounts[i]) + "/" + std::to_string(aiLaneCounts[i]), myfont, 8);
        laneCount.setFillColor(playerSelectedLane == i ? sf::Color(64, 45, 23) : sf::Color(205, 214, 188));
        laneCount.setPosition(laneButton.getPosition() + sf::Vector2f(8.f, 14.f));
        window.draw(laneCount);
    }

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
    panelHint.setCharacterSize(10);
    panelHint.setFillColor(sf::Color(219, 209, 174));
    panelHint.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), 203.f);
    panelHint.setString(guideText());
    window.draw(panelHint);

    upgradeBtn.setColor(commandForTeam(PLAYER) >= upgradeCostForNextLevel(PLAYER)
        && playerUpgradeLevel < config::MaxTechLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));
    window.draw(upgradeBtn);

    economyBtn.setColor(commandForTeam(PLAYER) >= economyUpgradeCost(PLAYER)
        && playerEconomyLevel < config::MaxEconomyLevel
        ? sf::Color::White
        : sf::Color(255, 255, 255, 130));
    window.draw(economyBtn);

    economyLabel.setCharacterSize(9);
    economyLabel.setFillColor(sf::Color(244, 221, 150));
    economyLabel.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EconomyButtonY + config::SideButtonHeight - 2.f);
    economyLabel.setString("Cost " + std::to_string(economyUpgradeCost(PLAYER))
        + " | +" + std::to_string(config::EconomyIncomeStep) + "/tick +drone");
    window.draw(economyLabel);

    sf::Text upgradeCost("Cost " + std::to_string(upgradeCostForNextLevel(PLAYER)) + " | Level gives 3 cards", myfont, 9);
    upgradeCost.setFillColor(sf::Color(244, 221, 150));
    upgradeCost.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), config::EndTurnButtonY + config::SideButtonHeight - 2.f);
    window.draw(upgradeCost);

    const bool canBuildBarracks = commandForTeam(PLAYER) >= config::BarracksCost
        && totalBuildingCount(PLAYER, building::Barracks) < buildingCap(PLAYER, building::Barracks);
    inf.setColor(canQueueUnit(PLAYER, UName::INFANTARY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    sho.setColor(canQueueUnit(PLAYER, UName::SHOOTER) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    cav.setColor(canQueueUnit(PLAYER, UName::CAVALRY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    siegeBtn.setColor(canQueueUnit(PLAYER, UName::SIEGE) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    guardianBtn.setColor(canQueueUnit(PLAYER, UName::GUARDIAN) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    barracksBtn.setColor(canBuildBarracks ? sf::Color::White : sf::Color(255, 255, 255, 130));
    const bool canQueueTower = commandForTeam(PLAYER) >= config::TowerCost
        && totalBuildingCount(PLAYER, building::DefenseTower) < buildingCap(PLAYER, building::DefenseTower);
    towerBtn.setColor(canQueueTower ? sf::Color::White : sf::Color(255, 255, 255, 130));

    const auto setLabel = [this](sf::Text& text, const std::string& value, int buttonY) {
        text.setCharacterSize(8);
        text.setFillColor(sf::Color(221, 211, 177));
        text.setPosition(static_cast<float>(config::PanelX + config::PanelPadding), static_cast<float>(buttonY + config::SideButtonHeight - 1));
        text.setString(value);
    };
    setLabel(barracksLabel, std::to_string(config::BarracksCost) + " | cap "
        + std::to_string(totalBuildingCount(PLAYER, building::Barracks)) + "/"
        + std::to_string(buildingCap(PLAYER, building::Barracks)) + " auto near base", config::BuildBarracksY);
    setLabel(infantryLabel, std::to_string(config::InfantryCost) + " | steady frontline", config::BuildInfantryY);
    setLabel(shooterLabel, std::to_string(config::ShooterCost) + " | ranged, slower", config::BuildShooterY);
    setLabel(cavalryLabel, std::to_string(config::CavalryCost) + " | fastest dive", config::BuildCavalryY);
    setLabel(siegeLabel, std::to_string(config::SiegeCost) + " | very slow tower-breaker", config::BuildSiegeY);
    setLabel(guardianLabel, std::to_string(config::GuardianCost) + " | slow heavy tank", config::BuildGuardianY);
    setLabel(towerLabel, std::to_string(config::TowerCost) + " | cap "
        + std::to_string(totalBuildingCount(PLAYER, building::DefenseTower)) + "/"
        + std::to_string(buildingCap(PLAYER, building::DefenseTower)) + " anti-rush", config::BuildTowerY);

    window.draw(helpBtn);
    window.draw(barracksBtn);
    window.draw(barracksLabel);
    window.draw(inf);
    window.draw(infantryLabel);
    window.draw(sho);
    window.draw(shooterLabel);
    window.draw(cav);
    window.draw(cavalryLabel);
    window.draw(siegeBtn);
    window.draw(siegeLabel);
    window.draw(guardianBtn);
    window.draw(guardianLabel);
    window.draw(towerBtn);
    window.draw(towerLabel);
}


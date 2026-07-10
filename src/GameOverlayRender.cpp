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

void Game::drawTutorialOverlay()
{
    const sf::View defaultView = logicalView();
    renderTarget().setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(8, 11, 10, 176));
    renderTarget().draw(veil);

    sf::RectangleShape shadow(sf::Vector2f(820.f, 620.f));
    shadow.setPosition(196.f, 56.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 92));
    renderTarget().draw(shadow);

    sf::RectangleShape card(sf::Vector2f(820.f, 620.f));
    card.setPosition(184.f, 44.f);
    card.setFillColor(sf::Color(39, 49, 43, 245));
    card.setOutlineColor(sf::Color(224, 170, 76));
    card.setOutlineThickness(3.f);
    renderTarget().draw(card);

    sf::RectangleShape inner(sf::Vector2f(792.f, 592.f));
    inner.setPosition(198.f, 58.f);
    inner.setFillColor(sf::Color::Transparent);
    inner.setOutlineColor(sf::Color(255, 241, 184, 34));
    inner.setOutlineThickness(1.f);
    renderTarget().draw(inner);

    sf::RectangleShape header(sf::Vector2f(820.f, 68.f));
    header.setPosition(184.f, 44.f);
    header.setFillColor(sf::Color(86, 61, 35, 238));
    renderTarget().draw(header);

    for (int x = 214; x < 958; x += 24) {
        sf::RectangleShape tick(sf::Vector2f(10.f, 2.f));
        tick.setPosition(static_cast<float>(x), 101.f);
        tick.setFillColor(sf::Color(255, 218, 112, 74));
        renderTarget().draw(tick);
    }

    sf::Text title("HOW TO PLAY", myfont, 31);
    title.setFillColor(sf::Color(255, 243, 201));
    title.setPosition(222.f, 59.f);
    renderTarget().draw(title);

    sf::Text closeHint("Press H or Esc to close", myfont, 14);
    closeHint.setFillColor(sf::Color(255, 226, 142));
    closeHint.setPosition(790.f, 75.f);
    renderTarget().draw(closeHint);

    const auto drawHelpCard = [this](sf::Vector2f pos, sf::Vector2f size, const std::string& title,
                                     const std::vector<std::string>& bullets, sf::Color accent) {
        sf::RectangleShape shadow(size);
        shadow.setPosition(pos + sf::Vector2f(4.f, 5.f));
        shadow.setFillColor(sf::Color(0, 0, 0, 66));
        renderTarget().draw(shadow);

        sf::RectangleShape card(size);
        card.setPosition(pos);
        card.setFillColor(sf::Color(29, 40, 36, 238));
        card.setOutlineColor(sf::Color(accent.r, accent.g, accent.b, 176));
        card.setOutlineThickness(1.6f);
        renderTarget().draw(card);

        sf::RectangleShape headerBar(sf::Vector2f(size.x - 12.f, 3.f));
        headerBar.setPosition(pos + sf::Vector2f(6.f, 8.f));
        headerBar.setFillColor(sf::Color(accent.r, accent.g, accent.b, 138));
        renderTarget().draw(headerBar);

        sf::CircleShape badge(13.f, 6);
        badge.setOrigin(13.f, 13.f);
        badge.setPosition(pos + sf::Vector2f(24.f, 31.f));
        badge.setFillColor(accent);
        badge.setOutlineColor(sf::Color(19, 24, 21));
        badge.setOutlineThickness(1.2f);
        renderTarget().draw(badge);

        sf::Text titleText(title, myfont, 17);
        titleText.setFillColor(sf::Color(255, 239, 190));
        titleText.setPosition(pos + sf::Vector2f(48.f, 20.f));
        renderTarget().draw(titleText);

        float lineY = pos.y + 54.f;
        for (const auto& bullet : bullets) {
            sf::Text line(bullet, myfont, 11);
            line.setFillColor(sf::Color(220, 231, 204));
            line.setPosition(pos.x + 22.f, lineY);
            renderTarget().draw(line);
            lineY += 18.f;
        }
    };

    drawHelpCard({220.f, 132.f}, {360.f, 146.f}, "Quick Start", {
        "ECONOMY grows CMD and visible drones.",
        "Barracks and Towers auto-place near HQ.",
        "Pick TOP / MID / BOT, then queue units.",
        "UPGRADE tech to draft 3 rogue tactics."
    }, sf::Color(255, 210, 91));

    drawHelpCard({608.f, 132.f}, {360.f, 146.f}, "Auto Battle", {
        "Units follow lane anchors and fight alone.",
        "Kills refund CMD by enemy unit value.",
        "Lost structures grant salvage and HQ shield.",
        "Siege ends games, but needs escorts."
    }, sf::Color(116, 184, 255));

    drawHelpCard({220.f, 306.f}, {360.f, 146.f}, "Power Spikes", {
        "Tech tactics change mechanics, not just stats.",
        "Press R once to refresh bad tactic cards.",
        "Mastery adds +10% baseline HP and damage.",
        "Buff icons show on the selected base."
    }, sf::Color(126, 206, 142));

    drawHelpCard({608.f, 306.f}, {360.f, 146.f}, "Counters", {
        "Shooter > Infantry, Infantry > Cavalry.",
        "Cavalry dives Shooter and Siege lines.",
        "Guardians tank late pushes.",
        "Towers splash escorts and punish rushes."
    }, sf::Color(255, 146, 92));

    drawHelpCard({220.f, 480.f}, {360.f, 132.f}, "Unlocks", {
        "Inf " + std::to_string(unitCost(UName::INFANTARY)) + " | Bow " + std::to_string(unitCost(UName::SHOOTER)) + " | Cav " + std::to_string(unitCost(UName::CAVALRY)) + " CMD.",
        "Siege needs Lv5, 2 Barracks, Econ 3.",
        "Guardian needs Lv7, 3 Barracks, Econ 4."
    }, sf::Color(238, 188, 92));

    drawHelpCard({608.f, 480.f}, {360.f, 132.f}, "Hotkeys", {
        "1-5: queue units on selected lane.",
        "Shift+1-5: buy unit Mastery.",
        "H: guide | C: restart | Esc: close."
    }, sf::Color(180, 205, 255));
}

void Game::drawRewardOverlay()
{
    const sf::View defaultView = logicalView();
    renderTarget().setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(7, 9, 8, 142));
    renderTarget().draw(veil);

    sf::RectangleShape panelShadow(sf::Vector2f(890.f, 330.f));
    panelShadow.setPosition(205.f, 194.f);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 105));
    renderTarget().draw(panelShadow);

    sf::RectangleShape panel(sf::Vector2f(890.f, 330.f));
    panel.setPosition(195.f, 184.f);
    panel.setFillColor(sf::Color(37, 48, 41, 248));
    panel.setOutlineColor(sf::Color(232, 177, 77));
    panel.setOutlineThickness(2.4f);
    renderTarget().draw(panel);

    sf::RectangleShape innerPanel(sf::Vector2f(862.f, 302.f));
    innerPanel.setPosition(209.f, 198.f);
    innerPanel.setFillColor(sf::Color::Transparent);
    innerPanel.setOutlineColor(sf::Color(255, 242, 188, 34));
    innerPanel.setOutlineThickness(1.f);
    renderTarget().draw(innerPanel);

    sf::Text title("Choose A Battle Tactic", myfont, 29);
    title.setFillColor(sf::Color(255, 239, 190));
    title.setPosition(236.f, 210.f);
    renderTarget().draw(title);

    sf::Text hint("Each tactic is a huge power spike. Press 1/2/3, click a card, or press R to refresh once.", myfont, 14);
    hint.setFillColor(sf::Color(222, 230, 204));
    hint.setPosition(236.f, 250.f);
    renderTarget().draw(hint);

    sf::RectangleShape reroll(sf::Vector2f(210.f, 42.f));
    reroll.setPosition(760.f, 202.f);
    reroll.setFillColor(playerRewardRerolls > 0 ? sf::Color(82, 69, 39, 242) : sf::Color(52, 55, 50, 226));
    reroll.setOutlineColor(playerRewardRerolls > 0 ? sf::Color(255, 218, 112) : sf::Color(116, 124, 105));
    reroll.setOutlineThickness(1.8f);
    renderTarget().draw(reroll);

    sf::RectangleShape rerollShine(sf::Vector2f(188.f, 3.f));
    rerollShine.setPosition(771.f, 210.f);
    rerollShine.setFillColor(playerRewardRerolls > 0 ? sf::Color(255, 246, 190, 72) : sf::Color(255, 255, 255, 20));
    renderTarget().draw(rerollShine);

    const std::string rerollText = playerRewardRerolls > 0
        ? ("Refresh cards (R) x" + std::to_string(playerRewardRerolls))
        : "Refresh used";
    sf::Text rerollLabel(rerollText, myfont, 14);
    rerollLabel.setFillColor(playerRewardRerolls > 0 ? sf::Color(255, 239, 190) : sf::Color(185, 192, 170));
    rerollLabel.setPosition(782.f, 214.f);
    renderTarget().draw(rerollLabel);

    for (int i = 0; i < static_cast<int>(perkChoices.size()); ++i) {
        const sf::Vector2f pos(235.f + static_cast<float>(i) * 278.f, 295.f);
        const bool hovered = i == hoveredRewardChoice;
        const sf::Color cardAccent = i == 0 ? sf::Color(239, 194, 84)
            : (i == 1 ? sf::Color(116, 184, 255) : sf::Color(255, 142, 91));
        sf::RectangleShape card(sf::Vector2f(250.f, 170.f));
        card.setPosition(pos);
        card.setFillColor(hovered ? sf::Color(65, 58, 39, 248) : sf::Color(47, 58, 51, 246));
        card.setOutlineColor(hovered ? sf::Color(255, 231, 143) : sf::Color(cardAccent.r, cardAccent.g, cardAccent.b, 150));
        card.setOutlineThickness(hovered ? 2.5f : 1.6f);
        renderTarget().draw(card);

        sf::RectangleShape headerBand(sf::Vector2f(246.f, 42.f));
        headerBand.setPosition(pos + sf::Vector2f(2.f, 2.f));
        headerBand.setFillColor(hovered
            ? sf::Color(cardAccent.r, cardAccent.g, cardAccent.b, 82)
            : sf::Color(35, 48, 43, 190));
        renderTarget().draw(headerBand);

        sf::RectangleShape topLine(sf::Vector2f(218.f, 3.f));
        topLine.setPosition(pos + sf::Vector2f(16.f, 10.f));
        topLine.setFillColor(sf::Color(cardAccent.r, cardAccent.g, cardAccent.b, hovered ? 150 : 68));
        renderTarget().draw(topLine);

        sf::RectangleShape bottomRail(sf::Vector2f(218.f, 2.f));
        bottomRail.setPosition(pos + sf::Vector2f(16.f, 154.f));
        bottomRail.setFillColor(i == 1 ? sf::Color(255, 218, 112, 138) : sf::Color(120, 137, 104, 110));
        renderTarget().draw(bottomRail);

        sf::CircleShape badge(17.f, 24);
        badge.setOrigin(17.f, 17.f);
        badge.setPosition(pos + sf::Vector2f(30.f, 30.f));
        const sf::Color badgeColor[] = {
            sf::Color(255, 211, 82),
            sf::Color(136, 207, 255),
            sf::Color(255, 146, 92)
        };
        badge.setFillColor(badgeColor[i]);
        badge.setOutlineColor(sf::Color(39, 35, 26));
        badge.setOutlineThickness(1.4f);
        renderTarget().draw(badge);

        sf::Text number(std::to_string(i + 1), myfont, 15);
        number.setFillColor(sf::Color(38, 32, 22));
        number.setPosition(pos.x + 25.f, pos.y + 19.f);
        renderTarget().draw(number);

        const auto& choice = perkChoices[static_cast<std::size_t>(i)];
        sf::Text name(choice.title, myfont, 20);
        name.setFillColor(sf::Color(255, 239, 190));
        name.setPosition(pos + sf::Vector2f(56.f, 20.f));
        renderTarget().draw(name);

        sf::Text desc(choice.description, myfont, 14);
        desc.setFillColor(sf::Color(224, 232, 203));
        desc.setPosition(pos + sf::Vector2f(22.f, 68.f));
        desc.setLineSpacing(1.2f);
        renderTarget().draw(desc);

        const int level = perkLevel(PLAYER, choice.type);
        const std::string levelText = choice.type == perk::WarChest
            ? "Instant tempo"
            : ("Level " + std::to_string(level) + "/" + std::to_string(maxPerkLevel(choice.type)));
        sf::Text meta(levelText, myfont, 12);
        meta.setFillColor(sf::Color(255, 218, 112));
        meta.setPosition(pos + sf::Vector2f(22.f, 138.f));
        renderTarget().draw(meta);
    }
}

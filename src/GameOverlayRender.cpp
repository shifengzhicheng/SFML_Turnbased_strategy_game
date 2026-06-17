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

void Game::drawTutorialOverlay()
{
    const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
    window.setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(8, 11, 10, 176));
    window.draw(veil);

    sf::RectangleShape shadow(sf::Vector2f(820.f, 620.f));
    shadow.setPosition(196.f, 56.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 92));
    window.draw(shadow);

    sf::RectangleShape card(sf::Vector2f(820.f, 620.f));
    card.setPosition(184.f, 44.f);
    card.setFillColor(sf::Color(39, 49, 43, 245));
    card.setOutlineColor(sf::Color(224, 170, 76));
    card.setOutlineThickness(3.f);
    window.draw(card);

    sf::RectangleShape header(sf::Vector2f(820.f, 68.f));
    header.setPosition(184.f, 44.f);
    header.setFillColor(sf::Color(86, 61, 35, 238));
    window.draw(header);

    sf::Text title("HOW TO PLAY", myfont, 31);
    title.setFillColor(sf::Color(255, 243, 201));
    title.setPosition(222.f, 59.f);
    window.draw(title);

    sf::Text closeHint("Press H or Esc to close", myfont, 14);
    closeHint.setFillColor(sf::Color(255, 226, 142));
    closeHint.setPosition(790.f, 75.f);
    window.draw(closeHint);

    const std::vector<std::string> lines = {
        "Goal",
        "  Build an economy, pick a lane, draft rogue tactics, then auto-push the enemy base.",
        "",
        "Core controls",
        "  Click ECONOMY: improve accelerating CMD income and add one visible drone.",
        "  Click Top / Mid / Bot, then click unit buttons to send new troops to that lane.",
        "  Click the small + on a unit row, or Shift+1..5, to buy infinite CMD Mastery.",
        "  Barracks and Tower buttons auto-place buildings near your base or lane defense.",
        "  Click Upgrade: spend CMD to gain a LEVEL and choose one rogue tactic card.",
        "",
        "Automation",
        "  CMD comes from natural income and kill bounties based on enemy unit cost.",
        "  Drones are your economy/readability meter and auto-build nearby structures.",
        "  Combat units auto-path down highlighted lanes, fight enemies, then raid buildings.",
        "  Towers prioritize siege and splash nearby escorts; send units to break tower lines.",
        "  Main bases have a timed shield; siege pushes are the clean finisher.",
        "  If all Barracks fall, the base slowly drafts emergency troops.",
        "  Lost structures refund CMD and trigger a short HQ shield so you can rebuild.",
        "",
        "Tactics and counters",
        "  Every tech upgrade gives 3 mechanism tactic cards. Max tech is LEVEL 15.",
        "  Press R on the tactic screen to refresh once if the 3 cards miss your build.",
        "  Unit Mastery is numeric: each level adds +10% baseline damage and HP forever.",
        "  Perks are build-changing: multi-shot, range caps, taunt, counter immunity.",
        "  Shooter > Infantry, Infantry > Cavalry, Cavalry > Shooter/Siege.",
        "  Cavalry rotates almost twice as fast as infantry; Siege crawls and needs escorts.",
        "  Siege cracks buildings; Guardians are heavy tanks that resist siege splash.",
        "",
        "Unlocks",
        "  Infantry: " + std::to_string(unitCost(UName::INFANTARY)) + " CMD, needs 1 Barracks.",
        "  Shooter: " + std::to_string(unitCost(UName::SHOOTER)) + " CMD, needs 1 Barracks and Economy 1 or LEVEL 1.",
        "  Cavalry: " + std::to_string(unitCost(UName::CAVALRY)) + " CMD, needs 2 Barracks and Economy 2 or LEVEL 3.",
        "  Siege: " + std::to_string(unitCost(UName::SIEGE)) + " CMD, needs LEVEL 5, 2 Barracks, Economy 3; outranges units.",
        "  Guardian: " + std::to_string(unitCost(UName::GUARDIAN)) + " CMD, needs LEVEL 7, 3 Barracks, Economy 4; anchors pushes.",
        "",
        "Hotkeys",
        "  1-5: queue units.  Shift+1-5: buy Mastery.  H: guide.  C: restart."
    };

    float y = 126.f;
    for (const auto& line : lines) {
        const bool section = !line.empty() && line.front() != ' ';
        sf::Text text(line, myfont, section ? 16 : 12);
        text.setFillColor(section ? sf::Color(255, 218, 112) : sf::Color(224, 232, 203));
        text.setPosition(228.f, y);
        window.draw(text);
        y += line.empty() ? 7.f : (section ? 22.f : 17.f);
    }
}

void Game::drawRewardOverlay()
{
    const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
    window.setView(defaultView);

    sf::RectangleShape veil(sf::Vector2f(config::WindowWidth, config::WindowHeight));
    veil.setFillColor(sf::Color(7, 9, 8, 142));
    window.draw(veil);

    sf::RectangleShape panelShadow(sf::Vector2f(890.f, 330.f));
    panelShadow.setPosition(205.f, 194.f);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 105));
    window.draw(panelShadow);

    sf::RectangleShape panel(sf::Vector2f(890.f, 330.f));
    panel.setPosition(195.f, 184.f);
    panel.setFillColor(sf::Color(37, 48, 41, 248));
    panel.setOutlineColor(sf::Color(232, 177, 77));
    panel.setOutlineThickness(2.4f);
    window.draw(panel);

    sf::Text title("Choose A Battle Tactic", myfont, 29);
    title.setFillColor(sf::Color(255, 239, 190));
    title.setPosition(236.f, 210.f);
    window.draw(title);

    sf::Text hint("Each tactic is a huge power spike. Press 1/2/3, click a card, or press R to refresh once.", myfont, 14);
    hint.setFillColor(sf::Color(222, 230, 204));
    hint.setPosition(236.f, 250.f);
    window.draw(hint);

    sf::RectangleShape reroll(sf::Vector2f(210.f, 42.f));
    reroll.setPosition(760.f, 216.f);
    reroll.setFillColor(playerRewardRerolls > 0 ? sf::Color(82, 69, 39, 242) : sf::Color(52, 55, 50, 226));
    reroll.setOutlineColor(playerRewardRerolls > 0 ? sf::Color(255, 218, 112) : sf::Color(116, 124, 105));
    reroll.setOutlineThickness(1.8f);
    window.draw(reroll);

    const std::string rerollText = playerRewardRerolls > 0
        ? ("Refresh cards (R) x" + std::to_string(playerRewardRerolls))
        : "Refresh used";
    sf::Text rerollLabel(rerollText, myfont, 14);
    rerollLabel.setFillColor(playerRewardRerolls > 0 ? sf::Color(255, 239, 190) : sf::Color(185, 192, 170));
    rerollLabel.setPosition(782.f, 228.f);
    window.draw(rerollLabel);

    for (int i = 0; i < static_cast<int>(perkChoices.size()); ++i) {
        const sf::Vector2f pos(235.f + static_cast<float>(i) * 278.f, 295.f);
        sf::RectangleShape card(sf::Vector2f(250.f, 170.f));
        card.setPosition(pos);
        card.setFillColor(i == 1 ? sf::Color(63, 55, 35, 246) : sf::Color(47, 58, 51, 246));
        card.setOutlineColor(i == 1 ? sf::Color(255, 218, 112) : sf::Color(120, 137, 104));
        card.setOutlineThickness(2.f);
        window.draw(card);

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
        window.draw(badge);

        sf::Text number(std::to_string(i + 1), myfont, 15);
        number.setFillColor(sf::Color(38, 32, 22));
        number.setPosition(pos.x + 25.f, pos.y + 19.f);
        window.draw(number);

        const auto& choice = perkChoices[static_cast<std::size_t>(i)];
        sf::Text name(choice.title, myfont, 20);
        name.setFillColor(sf::Color(255, 239, 190));
        name.setPosition(pos + sf::Vector2f(56.f, 20.f));
        window.draw(name);

        sf::Text desc(choice.description, myfont, 14);
        desc.setFillColor(sf::Color(224, 232, 203));
        desc.setPosition(pos + sf::Vector2f(22.f, 68.f));
        desc.setLineSpacing(1.2f);
        window.draw(desc);

        const int level = perkLevel(PLAYER, choice.type);
        const std::string levelText = choice.type == perk::WarChest
            ? "Instant tempo"
            : ("Level " + std::to_string(level) + "/" + std::to_string(maxPerkLevel(choice.type)));
        sf::Text meta(levelText, myfont, 12);
        meta.setFillColor(sf::Color(255, 218, 112));
        meta.setPosition(pos + sf::Vector2f(22.f, 138.f));
        window.draw(meta);
    }
}

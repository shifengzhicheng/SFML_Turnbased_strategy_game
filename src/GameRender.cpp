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

void Game::Draw()
{
    switch (gameSceneState)
    {
    case SCENE_START: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        window.setView(defaultView);

        sf::RectangleShape sky(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        sky.setFillColor(sf::Color(24, 35, 31));
        window.draw(sky);

        for (int i = 0; i < 9; ++i) {
            sf::CircleShape haze(130.f + static_cast<float>(i % 3) * 34.f, 64);
            haze.setOrigin(haze.getRadius(), haze.getRadius());
            haze.setPosition(120.f + static_cast<float>(i) * 156.f, 90.f + static_cast<float>((i * 73) % 420));
            haze.setScale(1.45f, 0.48f);
            haze.setRotation(static_cast<float>((i * 19) % 360));
            haze.setFillColor(i % 2 == 0 ? sf::Color(219, 166, 75, 24) : sf::Color(78, 135, 115, 22));
            window.draw(haze);
        }

        sf::RectangleShape horizon(sf::Vector2f(static_cast<float>(config::WindowWidth), 210.f));
        horizon.setPosition(0.f, 510.f);
        horizon.setFillColor(sf::Color(34, 45, 36, 210));
        window.draw(horizon);

        sf::RectangleShape heroShadow(sf::Vector2f(720.f, 360.f));
        heroShadow.setPosition(310.f, 126.f);
        heroShadow.setFillColor(sf::Color(8, 12, 10, 118));
        window.draw(heroShadow);

        sf::RectangleShape hero(sf::Vector2f(720.f, 360.f));
        hero.setPosition(300.f, 112.f);
        hero.setFillColor(sf::Color(45, 58, 50, 232));
        hero.setOutlineColor(sf::Color(224, 171, 82, 210));
        hero.setOutlineThickness(2.2f);
        window.draw(hero);

        sf::Text title("COMMAND LINES", myfont, 48);
        title.setFillColor(sf::Color(255, 236, 176));
        title.setOutlineColor(sf::Color(30, 22, 14, 220));
        title.setOutlineThickness(2.f);
        title.setLetterSpacing(1.25f);
        title.setPosition(365.f, 146.f);
        window.draw(title);

        sf::Text subtitle("A 10-minute rogue RTS auto-battler", myfont, 18);
        subtitle.setFillColor(sf::Color(206, 224, 190));
        subtitle.setPosition(415.f, 214.f);
        window.draw(subtitle);

        const char* cards[] = {"1. Grow CMD", "2. Pick a lane", "3. Draft tactics"};
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape card(sf::Vector2f(188.f, 76.f));
            card.setPosition(368.f + static_cast<float>(i) * 204.f, 274.f);
            card.setFillColor(sf::Color(36, 46, 40, 230));
            card.setOutlineColor(i == 1 ? sf::Color(126, 190, 139, 210) : sf::Color(192, 141, 66, 190));
            card.setOutlineThickness(1.4f);
            window.draw(card);

            sf::CircleShape gem(12.f, 6);
            gem.setOrigin(12.f, 12.f);
            gem.setPosition(card.getPosition() + sf::Vector2f(24.f, 22.f));
            gem.setFillColor(i == 0 ? sf::Color(244, 199, 90) : (i == 1 ? sf::Color(109, 184, 138) : sf::Color(140, 184, 238)));
            window.draw(gem);

            sf::Text cardText(cards[i], myfont, 14);
            cardText.setFillColor(sf::Color(236, 232, 202));
            cardText.setPosition(card.getPosition() + sf::Vector2f(46.f, 15.f));
            window.draw(cardText);
        }

        startBtn.setPosition(470.f, 410.f);
        startHelpBtn.setPosition(710.f, 410.f);
        window.draw(startBtn);
        window.draw(startHelpBtn);

        sf::Text hint("Simple loop: Economy -> Barracks -> Pick a lane -> Draft upgrades", myfont, 13);
        hint.setFillColor(sf::Color(229, 214, 160));
        hint.setPosition(430.f, 516.f);
        window.draw(hint);

        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        break;
    }
    case SCENE_GAME: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        sf::View gameView(defaultView);
        gameView.move(currentShakeOffset());
        window.setView(gameView);

        for (const auto& tile : tiles)
            window.draw(tile);

        if (Base_red && Base_red->UnitState == UState::UNITCLICK) {
            RectangleShape rect;
            rect.setPosition(Base_red->getPosition());
            rect.setSize(sf::Vector2f(2 * SqureSize, 2 * SqureSize));
            rect.setFillColor(sf::Color::Transparent);
            rect.setOutlineColor(sf::Color::Red);
            rect.setOutlineThickness(2.f);
            window.draw(rect);
        }

        for (const auto& pathTile : drawPaths)
            window.draw(pathTile);
        drawGridOverlay();
        drawLaneGuides();
        drawResourceNodes();
        drawBuildings();
        drawWorkers();

        const auto drawBaseShield = [this](const DisMoveableUnit* base, int team) {
            const float seconds = baseShieldSecondsForTeam(team);
            if (base == nullptr || seconds <= 0.f) {
                return;
            }
            const float pulse = 0.5f + 0.5f * std::sin(gameTimeSeconds * 8.f);
            const sf::Color color = team == PLAYER ? sf::Color(255, 224, 112) : sf::Color(116, 184, 255);
            sf::CircleShape shield(31.f, 56);
            shield.setOrigin(31.f, 31.f);
            shield.setScale(1.12f + pulse * 0.05f, 0.88f + pulse * 0.04f);
            shield.setPosition(base->x * SqureSize + SqureSize, base->y * SqureSize + SqureSize);
            shield.setFillColor(sf::Color(color.r, color.g, color.b, 28));
            shield.setOutlineColor(sf::Color(color.r, color.g, color.b, static_cast<sf::Uint8>(150 + pulse * 75.f)));
            shield.setOutlineThickness(2.2f);
            window.draw(shield);
        };
        drawBaseShield(Base_red.get(), PLAYER);
        drawBaseShield(Base_blue.get(), AI);

        if (Base_red) {
            window.draw(*Base_red);
            window.draw(Base_red->UnitText);
        }
        if (Base_blue) {
            window.draw(*Base_blue);
            window.draw(Base_blue->UnitText);
        }
        const auto drawBasePerks = [this](const DisMoveableUnit* base, int team) {
            if (base == nullptr) {
                return;
            }
            const bool show = (team == PLAYER && base->UnitState == UState::UNITCLICK)
                || (team == AI && MosOnUnit == base);
            if (!show) {
                return;
            }
            std::string perks;
            for (int type = 0; type < perk::Count; ++type) {
                const int level = perkLevel(team, type);
                if (level <= 0) {
                    continue;
                }
                if (!perks.empty()) {
                    perks += " ";
                }
                perks += perkShortName(type);
                perks += std::to_string(level);
            }
            if (perks.empty()) {
                perks = "no perks";
            }
            const int shieldSeconds = static_cast<int>(std::ceil(baseShieldSecondsForTeam(team)));
            if (shieldSeconds > 0) {
                perks += " Shield";
                perks += std::to_string(shieldSeconds);
                perks += "s";
            }
            const int level = team == PLAYER ? playerUpgradeLevel : aiUpgradeLevel;
            sf::Text status("Lv" + std::to_string(level) + " " + perks, myfont, 10);
            status.setFillColor(team == PLAYER ? sf::Color(255, 226, 142) : sf::Color(155, 203, 255));
            status.setOutlineColor(sf::Color(31, 24, 18, 220));
            status.setOutlineThickness(1.f);
            status.setPosition(base->x * SqureSize - 10.f, base->y * SqureSize - 28.f);
            window.draw(status);
        };
        drawBasePerks(Base_red.get(), PLAYER);
        drawBasePerks(Base_blue.get(), AI);

        UnitText.setString("");
        UnitAttack.setString("");
        UnitHP.setString("");

        for (const auto& u : enemys) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(61, 128, 206));
            window.draw(*u);
            window.draw(u->UnitText);
        }
        for (const auto& u : myunits) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(218, 76, 60));
            if (u->UnitState == UState::UNITCLICK) {
                MapPos selected(Point(u->x, u->y), tile::Choosen);
                window.draw(selected);
                string temp = "Action: "+to_string(u->myActionPoint);
                UnitText.setString(temp);
                temp = "Attack: " + to_string(u->myattack());
                UnitAttack.setString(temp);
                temp = "HP: " + to_string(u->Health);
                UnitHP.setString(temp);
            }
            window.draw(*u);
            window.draw(u->UnitText);
        }
        effects.draw(window);

        window.setView(defaultView);
        DrawSidePanel();
        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        if (perkOverlayVisible) {
            drawRewardOverlay();
        }
        
        break;
    }
    case SCEN_GAMEOVER: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        window.setView(defaultView);

        sf::RectangleShape backdrop(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        backdrop.setFillColor(gameWin ? sf::Color(30, 54, 43) : sf::Color(55, 39, 34));
        window.draw(backdrop);

        for (int i = 0; i < 7; ++i) {
            sf::CircleShape flare(115.f + static_cast<float>(i % 2) * 42.f, 60);
            flare.setOrigin(flare.getRadius(), flare.getRadius());
            flare.setPosition(168.f + static_cast<float>(i) * 176.f, 110.f + static_cast<float>((i * 91) % 430));
            flare.setScale(1.35f, 0.52f);
            flare.setFillColor(gameWin ? sf::Color(219, 184, 92, 28) : sf::Color(124, 88, 72, 34));
            window.draw(flare);
        }

        sf::RectangleShape panelShadow(sf::Vector2f(620.f, 344.f));
        panelShadow.setPosition(372.f, 154.f);
        panelShadow.setFillColor(sf::Color(8, 10, 9, 120));
        window.draw(panelShadow);

        sf::RectangleShape panel(sf::Vector2f(620.f, 344.f));
        panel.setPosition(360.f, 140.f);
        panel.setFillColor(sf::Color(39, 49, 43, 238));
        panel.setOutlineColor(gameWin ? sf::Color(239, 196, 102) : sf::Color(210, 113, 86));
        panel.setOutlineThickness(2.4f);
        window.draw(panel);

        sf::Text result(gameWin ? "VICTORY" : "DEFEAT", myfont, 50);
        result.setFillColor(gameWin ? sf::Color(255, 236, 168) : sf::Color(255, 184, 142));
        result.setOutlineColor(sf::Color(22, 18, 14, 230));
        result.setOutlineThickness(2.f);
        result.setLetterSpacing(1.25f);
        result.setPosition(gameWin ? 545.f : 560.f, 178.f);
        window.draw(result);

        sf::Text summary(gameWin ? "Your build order broke the enemy core." : "The enemy policy found a stronger timing.", myfont, 18);
        summary.setFillColor(sf::Color(226, 232, 202));
        summary.setPosition(gameWin ? 478.f : 475.f, 255.f);
        window.draw(summary);

        sf::Text stats("Time " + std::to_string(static_cast<int>(gameTimeSeconds)) + "s   Your Lv " + std::to_string(playerUpgradeLevel)
            + " / AI Lv " + std::to_string(aiUpgradeLevel), myfont, 15);
        stats.setFillColor(sf::Color(212, 204, 166));
        stats.setPosition(506.f, 306.f);
        window.draw(stats);

        endGame.setPosition(550.f, 372.f);
        window.draw(endGame);
        break;
    }
    default:
        break;
    }
    
}

void Game::drawGridOverlay()
{
    sf::VertexArray lines(sf::Lines);
    const sf::Color lineColor(108, 119, 115, 175);

    for (int x = 0; x <= width; x += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(height)), lineColor));
    }
    for (int y = 0; y <= height; y += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(width), static_cast<float>(y)), lineColor));
    }

    window.draw(lines);
}

void Game::drawLaneGuides()
{
    const int mapH = height / SqureSize;
    const int laneYs[] = {
        std::max(4, mapH / 4),
        mapH / 2,
        std::min(mapH - 5, mapH * 3 / 4)
    };
    const char* labels[] = {"TOP", "MID", "BOT"};

    for (int i = 0; i < lane::Count; ++i) {
        const float y = static_cast<float>(laneYs[i] * SqureSize + SqureSize / 2);
        sf::RectangleShape ribbon(sf::Vector2f(static_cast<float>(width), 5.f));
        ribbon.setOrigin(0.f, 2.5f);
        ribbon.setPosition(0.f, y);
        ribbon.setFillColor(i == playerSelectedLane ? sf::Color(255, 218, 112, 72) : sf::Color(205, 220, 190, 34));
        window.draw(ribbon);

        sf::CircleShape arrow(7.f, 3);
        arrow.setOrigin(7.f, 7.f);
        arrow.setPosition(static_cast<float>(Red_baseP.x * SqureSize + 95), y);
        arrow.setRotation(90.f);
        arrow.setFillColor(i == playerSelectedLane ? sf::Color(255, 218, 112, 150) : sf::Color(220, 230, 196, 90));
        window.draw(arrow);

        sf::Text laneText(labels[i], myfont, 11);
        laneText.setFillColor(i == playerSelectedLane ? sf::Color(255, 236, 168, 190) : sf::Color(213, 225, 198, 110));
        laneText.setOutlineColor(sf::Color(24, 29, 23, 140));
        laneText.setOutlineThickness(1.f);
        laneText.setPosition(static_cast<float>(Red_baseP.x * SqureSize + 18), y - 18.f);
        window.draw(laneText);
    }
}

void Game::drawResourceNodes()
{
    for (const auto& node : resources) {
        const sf::Vector2f center(
            node.point.x * SqureSize + SqureSize / 2.f,
            node.point.y * SqureSize + SqureSize / 2.f);
        const float t = node.pulseClock.getElapsedTime().asSeconds();
        const float pulse = 1.f + std::sin(t * 4.f) * 0.08f;
        const sf::Color brass = sf::Color(226, 180, 63);
        const sf::Color gold = sf::Color(255, 211, 82);

        sf::CircleShape shadow(11.f, 30);
        shadow.setOrigin(11.f, 11.f);
        shadow.setScale(1.35f, 0.42f);
        shadow.setPosition(center + sf::Vector2f(0.f, 7.f));
        shadow.setFillColor(sf::Color(22, 18, 10, 95));
        window.draw(shadow);

        sf::CircleShape aura(13.f * pulse, 40);
        aura.setOrigin(13.f * pulse, 13.f * pulse);
        aura.setPosition(center);
        aura.setFillColor(sf::Color(brass.r, brass.g, brass.b, 42));
        aura.setOutlineColor(sf::Color(brass.r, brass.g, brass.b, 178));
        aura.setOutlineThickness(1.2f);
        window.draw(aura);

        sf::RectangleShape beam(sf::Vector2f(4.f, 22.f));
        beam.setOrigin(2.f, 20.f);
        beam.setPosition(center + sf::Vector2f(0.f, -5.f));
        beam.setFillColor(sf::Color(255, 230, 122, 48));
        window.draw(beam);

        sf::ConvexShape crystal(8);
        crystal.setPoint(0, center + sf::Vector2f(0.f, -12.f));
        crystal.setPoint(1, center + sf::Vector2f(8.f, -7.f));
        crystal.setPoint(2, center + sf::Vector2f(10.f, 1.f));
        crystal.setPoint(3, center + sf::Vector2f(5.f, 9.f));
        crystal.setPoint(4, center + sf::Vector2f(0.f, 12.f));
        crystal.setPoint(5, center + sf::Vector2f(-5.f, 9.f));
        crystal.setPoint(6, center + sf::Vector2f(-10.f, 1.f));
        crystal.setPoint(7, center + sf::Vector2f(-8.f, -7.f));
        crystal.setFillColor(gold);
        crystal.setOutlineColor(sf::Color(brass.r, brass.g, brass.b, 235));
        crystal.setOutlineThickness(1.4f);
        window.draw(crystal);

        sf::CircleShape core(3.2f, 18);
        core.setOrigin(3.2f, 3.2f);
        core.setPosition(center + sf::Vector2f(0.f, -1.f));
        core.setFillColor(sf::Color(255, 250, 180, 210));
        window.draw(core);

        sf::RectangleShape labelBg(sf::Vector2f(29.f, 11.f));
        labelBg.setPosition(center.x + 9.f, center.y - 17.f);
        labelBg.setFillColor(sf::Color(40, 32, 19, 185));
        labelBg.setOutlineColor(sf::Color(255, 225, 128, 170));
        labelBg.setOutlineThickness(0.7f);
        window.draw(labelBg);

        sf::Text label("CMD", myfont, 8);
        label.setFillColor(sf::Color(255, 235, 145));
        label.setPosition(labelBg.getPosition() + sf::Vector2f(4.f, 0.f));
        window.draw(label);
    }
}

void Game::drawBuildings()
{
    for (const auto& building : buildings) {
        const sf::Vector2f origin(building.point.x * SqureSize, building.point.y * SqureSize);
        if (!building.complete) {
            const float pct = std::clamp(building.buildProgress / std::max(0.1f, building.buildSeconds), 0.f, 1.f);
            sf::RectangleShape barBg(sf::Vector2f(18.f, 3.f));
            barBg.setPosition(origin + sf::Vector2f(1.f, -5.f));
            barBg.setFillColor(sf::Color(25, 25, 22, 170));
            window.draw(barBg);
            sf::RectangleShape bar(sf::Vector2f(18.f * pct, 3.f));
            bar.setPosition(barBg.getPosition());
            bar.setFillColor(building.team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255));
            window.draw(bar);
        }
        else if (building.type == building::Barracks && building.production.activeUnit >= 0) {
            const float trainSeconds = unitTrainSeconds(building.production.activeUnit) * teamTrainTimeMultiplier(building.team);
            const float pct = std::clamp(building.production.progress / trainSeconds, 0.f, 1.f);
            sf::RectangleShape bar(sf::Vector2f(18.f * pct, 2.f));
            bar.setPosition(origin + sf::Vector2f(1.f, -4.f));
            bar.setFillColor(sf::Color(255, 218, 112));
            window.draw(bar);
        }
        if (building.complete && building.type == building::Barracks && building.production.load() > 0) {
            sf::Text queueText("Q" + std::to_string(building.production.load()), myfont, 9);
            queueText.setFillColor(sf::Color(255, 241, 177));
            queueText.setOutlineColor(sf::Color(44, 32, 24, 210));
            queueText.setOutlineThickness(0.8f);
            queueText.setPosition(origin + sf::Vector2f(2.f, 10.f));
            window.draw(queueText);
        }

        if (building.maxHealth > 0 && building.health < building.maxHealth) {
            const float pct = std::clamp(static_cast<float>(building.health) / static_cast<float>(building.maxHealth), 0.f, 1.f);
            sf::RectangleShape hpBg(sf::Vector2f(18.f, 2.f));
            hpBg.setPosition(origin + sf::Vector2f(1.f, 18.f));
            hpBg.setFillColor(sf::Color(42, 23, 20, 190));
            window.draw(hpBg);
            sf::RectangleShape hpBar(sf::Vector2f(18.f * pct, 2.f));
            hpBar.setPosition(hpBg.getPosition());
            hpBar.setFillColor(sf::Color(112, 230, 118));
            window.draw(hpBar);
        }
    }
}

void Game::drawWorkers()
{
    for (const auto& workerUnit : workers) {
        const sf::Vector2f center(workerUnit.point.x * SqureSize + SqureSize / 2.f, workerUnit.point.y * SqureSize + SqureSize / 2.f);
        sf::CircleShape body(5.f, 18);
        body.setOrigin(5.f, 5.f);
        body.setPosition(center);
        body.setFillColor(workerUnit.team == PLAYER ? sf::Color(255, 187, 87) : sf::Color(118, 178, 255));
        body.setOutlineColor(sf::Color(39, 35, 26));
        body.setOutlineThickness(1.f);
        window.draw(body);

        if (workerUnit.state == worker::Building) {
            sf::RectangleShape tool(sf::Vector2f(8.f, 1.6f));
            tool.setOrigin(4.f, 0.8f);
            tool.setPosition(center + sf::Vector2f(3.f, -5.f));
            tool.setRotation(35.f);
            tool.setFillColor(sf::Color(255, 241, 177));
            window.draw(tool);
        }
    }
}

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
        "  Click ECONOMY: improve natural CMD income and add one visible drone.",
        "  Click Top / Mid / Bot, then click unit buttons to send new troops to that lane.",
        "  Barracks and Tower buttons auto-place buildings near your base or lane defense.",
        "  Click Upgrade: spend CMD to gain a LEVEL and choose one rogue tactic card.",
        "",
        "Automation",
        "  CMD comes from natural income and kill bounties based on enemy unit cost.",
        "  Drones are your economy/readability meter and auto-build nearby structures.",
        "  Combat units auto-path down highlighted lanes, fight enemies, then raid buildings.",
        "  Towers beat basic attacks, but Siege outranges towers and forces a response.",
        "  Main bases have a timed shield; siege pushes are the clean finisher.",
        "  If all Barracks fall, the base slowly drafts emergency troops.",
        "  Lost structures refund CMD and trigger a short HQ shield so you can rebuild.",
        "",
        "Tactics and counters",
        "  Every tech upgrade gives 3 tactic cards. Max tech is LEVEL 15.",
        "  Perks stack, so late builds can bend the soft counter rules.",
        "  Shooter > Infantry, Infantry > Cavalry, Cavalry > Shooter/Siege.",
        "  Cavalry rotates fastest; Siege is very slow and needs escorts.",
        "  Siege cracks Guardians and buildings; Guardians anchor against Cavalry dives.",
        "",
        "Unlocks",
        "  Infantry: 15 CMD, needs 1 Barracks.",
        "  Shooter: 22 CMD, needs 1 Barracks and Economy 1 or LEVEL 1.",
        "  Cavalry: 38 CMD, needs 2 Barracks and Economy 2 or LEVEL 3.",
        "  Siege: 56 CMD, needs LEVEL 5, 2 Barracks, Economy 3; outranges towers.",
        "  Guardian: 74 CMD, needs LEVEL 7, 3 Barracks, Economy 4; anchors pushes.",
        "",
        "Hotkeys",
        "  H: show / hide this guide.  C: restart map.  Esc: back to menu."
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

    sf::Text hint("Rewards are small and capped, so every unit keeps a role. Press 1/2/3 or click a card.", myfont, 14);
    hint.setFillColor(sf::Color(222, 230, 204));
    hint.setPosition(236.f, 250.f);
    window.draw(hint);

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

void Game::addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    effects.addAttack(start, end, color);
}

void Game::addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size)
{
    effects.addFloatingText(myfont, position, value, color, size);
}

void Game::startScreenShake(float durationSeconds, float intensity)
{
    effects.startShake(durationSeconds, intensity);
}

sf::Vector2f Game::currentShakeOffset() const
{
    return effects.shakeOffset();
}

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

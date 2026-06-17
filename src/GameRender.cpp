#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

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
        hero.setFillColor(sf::Color(37, 48, 43, 236));
        hero.setOutlineColor(sf::Color(224, 171, 82, 226));
        hero.setOutlineThickness(2.2f);
        window.draw(hero);

        sf::VertexArray heroGrid(sf::Lines);
        for (int x = 324; x <= 996; x += 48) {
            heroGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 250.f), sf::Color(164, 184, 132, 34)));
            heroGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 446.f), sf::Color(164, 184, 132, 14)));
        }
        for (int y = 250; y <= 446; y += 32) {
            heroGrid.append(sf::Vertex(sf::Vector2f(324.f, static_cast<float>(y)), sf::Color(164, 184, 132, 22)));
            heroGrid.append(sf::Vertex(sf::Vector2f(996.f, static_cast<float>(y)), sf::Color(164, 184, 132, 10)));
        }
        window.draw(heroGrid);

        const sf::Color laneGold(255, 218, 112, 92);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape laneMark(sf::Vector2f(520.f, 5.f));
            laneMark.setOrigin(260.f, 2.5f);
            laneMark.setPosition(660.f, 308.f + static_cast<float>(i) * 45.f);
            laneMark.setRotation(i == 0 ? -7.f : (i == 2 ? 7.f : 0.f));
            laneMark.setFillColor(i == 1 ? laneGold : sf::Color(122, 184, 111, 70));
            window.draw(laneMark);
        }

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

        for (const auto& tile : tiles) {
            tile.drawGround(window, sf::RenderStates::Default);
        }

        drawLaneGuides();
        drawGridOverlay();

        struct RenderItem
        {
            float sortY = 0.f;
            int sortX = 0;
            std::function<void()> draw;
        };
        std::vector<RenderItem> renderItems;

        for (const auto& tile : tiles) {
            if (!tile.hasRaisedObject()) {
                continue;
            }
            const auto index = tile.getIndex();
            const MapPos* tilePtr = &tile;
            renderItems.push_back({tile.renderSortY(), index.x, [this, tilePtr]() {
                tilePtr->drawObject(window, sf::RenderStates::Default);
            }});
        }

        for (const auto& node : resources) {
            const ResourceNode* nodePtr = &node;
            renderItems.push_back({static_cast<float>((node.point.y + 1) * SqureSize), node.point.x, [this, nodePtr]() {
                drawResourceNode(*nodePtr);
            }});
        }

        for (const auto& workerUnit : workers) {
            const Worker* workerPtr = &workerUnit;
            renderItems.push_back({static_cast<float>((workerUnit.point.y + 1) * SqureSize), workerUnit.point.x, [this, workerPtr]() {
                drawWorkerSprite(*workerPtr);
            }});
        }

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
        const auto pushBase = [&](DisMoveableUnit* base, int team) {
            if (base == nullptr) {
                return;
            }
            renderItems.push_back({static_cast<float>((base->y + config::BaseFootprintSize) * SqureSize), base->x, [this, base, team, drawBaseShield, drawBasePerks]() {
                drawBaseShield(base, team);
                if (team == PLAYER && base->UnitState == UState::UNITCLICK) {
                    MapPos selected(Point(base->x, base->y), true, true);
                    window.draw(selected);
                }
                window.draw(*base);
                window.draw(base->UnitText);
                drawBasePerks(base, team);
            }});
        };
        pushBase(Base_red.get(), PLAYER);
        pushBase(Base_blue.get(), AI);

        const auto pushUnit = [&](MoveableUnit* unit, sf::Color color, bool playerUnit) {
            if (unit == nullptr) {
                return;
            }
            renderItems.push_back({static_cast<float>((unit->y + 1) * SqureSize), unit->x, [this, unit, color, playerUnit]() {
                drawUnitBase(window, Point(unit->x, unit->y), color);
                if (playerUnit && unit->UnitState == UState::UNITCLICK) {
                    MapPos selected(Point(unit->x, unit->y), tile::Choosen);
                    window.draw(selected);
                }
                window.draw(*unit);
                window.draw(unit->UnitText);
            }});
        };
        for (const auto& u : enemys) {
            pushUnit(u.get(), sf::Color(61, 128, 206), false);
        }
        for (const auto& u : myunits) {
            pushUnit(u.get(), sf::Color(218, 76, 60), true);
        }

        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            if (a.sortY != b.sortY) {
                return a.sortY < b.sortY;
            }
            return a.sortX < b.sortX;
        });
        for (const auto& item : renderItems) {
            item.draw();
        }

        drawBuildings();

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

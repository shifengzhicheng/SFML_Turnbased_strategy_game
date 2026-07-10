#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "CombatBehavior.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace sf;
using namespace std;
using namespace game_internal;

namespace
{
    void drawUnitCombatCue(sf::RenderTarget& target, const MoveableUnit& unit, float gameTimeSeconds)
    {
        const sf::Vector2f foot((unit.x + 0.5f) * config::TileSize,
                                (unit.y + 0.88f) * config::TileSize);
        if (gameTimeSeconds + 0.001f < unit.deploymentReadyTime) {
            const float pulse = 0.5f + std::sin(gameTimeSeconds * 6.f + static_cast<float>(unit.entityId)) * 0.5f;
            sf::CircleShape rally(14.f, 16);
            rally.setOrigin(14.f, 14.f);
            rally.setScale(1.f, 0.34f);
            rally.setPosition(foot);
            rally.setFillColor(sf::Color(255, 211, 84, static_cast<sf::Uint8>(18.f + pulse * 18.f)));
            rally.setOutlineColor(sf::Color(255, 227, 128, static_cast<sf::Uint8>(115.f + pulse * 95.f)));
            rally.setOutlineThickness(2.f);
            target.draw(rally);
        }

        if (unit.unitName == UName::CAVALRY && unit.UnitState == UState::MOVING
            && unit.tilesMovedSinceAttack > 0) {
            const float intensity = std::min(1.f, static_cast<float>(unit.tilesMovedSinceAttack)
                / static_cast<float>(config::CavalryChargeTiles));
            sf::RectangleShape trail(sf::Vector2f(28.f + intensity * 12.f, 4.f));
            trail.setOrigin(trail.getSize().x * 0.5f, 2.f);
            trail.setPosition(foot + sf::Vector2f(0.f, 2.f));
            trail.setFillColor(unit.myteam == PLAYER
                ? sf::Color(255, 154, 70, static_cast<sf::Uint8>(40.f + intensity * 70.f))
                : sf::Color(103, 176, 255, static_cast<sf::Uint8>(40.f + intensity * 70.f)));
            target.draw(trail);
        }

        const CombatBehaviorDefinition& behavior = combatBehavior(unit.unitName);
        if (behavior.deploymentSeconds > 0.f && unit.UnitState != UState::MOVING
            && unit.stationarySeconds < behavior.deploymentSeconds
            && gameTimeSeconds + 0.001f >= unit.deploymentReadyTime) {
            const float progress = std::clamp(unit.stationarySeconds / behavior.deploymentSeconds, 0.f, 1.f);
            const sf::Color braceColor = unit.myteam == PLAYER
                ? sf::Color(255, 201, 91, 180)
                : sf::Color(118, 181, 255, 180);
            sf::RectangleShape left(sf::Vector2f(8.f + progress * 8.f, 2.f));
            left.setOrigin(left.getSize().x, 1.f);
            left.setPosition(foot + sf::Vector2f(-6.f, 1.f));
            left.setRotation(18.f);
            left.setFillColor(braceColor);
            target.draw(left);
            sf::RectangleShape right(left);
            right.setOrigin(0.f, 1.f);
            right.setPosition(foot + sf::Vector2f(6.f, 1.f));
            right.setRotation(-18.f);
            target.draw(right);
        }
    }

    void drawUnitHealthBar(sf::RenderTarget& target, const MoveableUnit& unit, bool showNumber)
    {
        const int maximum = std::max(1, unit.maxHealth());
        const float ratio = std::clamp(static_cast<float>(unit.Health) / static_cast<float>(maximum), 0.f, 1.f);
        if (ratio >= 0.999f && !showNumber) {
            return;
        }

        const sf::Vector2f position((unit.x + 0.5f) * config::TileSize - 14.f,
                                    unit.UnitText.getPosition().y + 6.f);
        sf::RectangleShape back(sf::Vector2f(28.f, 5.f));
        back.setPosition(position);
        back.setFillColor(sf::Color(20, 24, 21, 220));
        back.setOutlineColor(sf::Color(240, 230, 190, 120));
        back.setOutlineThickness(1.f);
        target.draw(back);

        const sf::Color fill = ratio > 0.55f
            ? sf::Color(102, 207, 105)
            : (ratio > 0.25f ? sf::Color(239, 188, 73) : sf::Color(225, 77, 58));
        sf::RectangleShape health(sf::Vector2f(26.f * ratio, 3.f));
        health.setPosition(position + sf::Vector2f(1.f, 1.f));
        health.setFillColor(fill);
        target.draw(health);

        if (showNumber) {
            target.draw(unit.UnitText);
        }
    }
}

void Game::Draw(sf::RenderTarget& target)
{
    sf::RenderTarget* previous = renderTargetOverride;
    renderTargetOverride = &target;
    Draw();
    renderTargetOverride = previous;
}

void Game::Draw()
{
    switch (gameSceneState)
    {
    case SCENE_START: {
        const sf::View defaultView = logicalView();
        renderTarget().setView(defaultView);

        sf::RectangleShape sky(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        sky.setFillColor(sf::Color(24, 35, 31));
        renderTarget().draw(sky);

        sf::VertexArray backdropGrid(sf::Lines);
        for (int x = 0; x <= config::WindowWidth; x += 48) {
            backdropGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), sf::Color(132, 166, 119, 16)));
            backdropGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(config::WindowHeight)), sf::Color(132, 166, 119, 5)));
        }
        for (int y = 0; y <= config::WindowHeight; y += 48) {
            backdropGrid.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), sf::Color(132, 166, 119, 12)));
            backdropGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(y)), sf::Color(132, 166, 119, 4)));
        }
        renderTarget().draw(backdropGrid);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape laneBand(sf::Vector2f(1540.f, 54.f));
            laneBand.setOrigin(770.f, 27.f);
            laneBand.setPosition(694.f, 145.f + static_cast<float>(i) * 208.f);
            laneBand.setRotation(i == 0 ? -8.f : (i == 2 ? 8.f : 0.f));
            laneBand.setFillColor(i == 1 ? sf::Color(204, 157, 70, 18) : sf::Color(75, 135, 105, 18));
            renderTarget().draw(laneBand);
        }
        for (int i = 0; i < 26; ++i) {
            sf::RectangleShape pixel(sf::Vector2f(8.f + static_cast<float>(i % 3) * 4.f, 8.f));
            pixel.setPosition(static_cast<float>((i * 173) % config::WindowWidth),
                              static_cast<float>(42 + (i * 97) % 610));
            pixel.setFillColor(i % 2 == 0 ? sf::Color(223, 178, 82, 30) : sf::Color(104, 171, 128, 26));
            renderTarget().draw(pixel);
        }

        sf::RectangleShape horizon(sf::Vector2f(static_cast<float>(config::WindowWidth), 210.f));
        horizon.setPosition(0.f, 510.f);
        horizon.setFillColor(sf::Color(34, 45, 36, 210));
        renderTarget().draw(horizon);

        sf::RectangleShape heroShadow(sf::Vector2f(720.f, 360.f));
        heroShadow.setPosition(310.f, 126.f);
        heroShadow.setFillColor(sf::Color(8, 12, 10, 118));
        renderTarget().draw(heroShadow);

        sf::RectangleShape hero(sf::Vector2f(720.f, 360.f));
        hero.setPosition(300.f, 112.f);
        hero.setFillColor(sf::Color(37, 48, 43, 236));
        hero.setOutlineColor(sf::Color(224, 171, 82, 226));
        hero.setOutlineThickness(2.2f);
        renderTarget().draw(hero);

        sf::RectangleShape heroTop(sf::Vector2f(696.f, 4.f));
        heroTop.setPosition(312.f, 124.f);
        heroTop.setFillColor(sf::Color(255, 226, 142, 128));
        renderTarget().draw(heroTop);

        sf::VertexArray heroGrid(sf::Lines);
        for (int x = 324; x <= 996; x += 48) {
            heroGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 250.f), sf::Color(164, 184, 132, 34)));
            heroGrid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 446.f), sf::Color(164, 184, 132, 14)));
        }
        for (int y = 250; y <= 446; y += 32) {
            heroGrid.append(sf::Vertex(sf::Vector2f(324.f, static_cast<float>(y)), sf::Color(164, 184, 132, 22)));
            heroGrid.append(sf::Vertex(sf::Vector2f(996.f, static_cast<float>(y)), sf::Color(164, 184, 132, 10)));
        }
        renderTarget().draw(heroGrid);

        const sf::Color laneGold(255, 218, 112, 92);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape laneMark(sf::Vector2f(520.f, 5.f));
            laneMark.setOrigin(260.f, 2.5f);
            laneMark.setPosition(660.f, 308.f + static_cast<float>(i) * 45.f);
            laneMark.setRotation(i == 0 ? -7.f : (i == 2 ? 7.f : 0.f));
            laneMark.setFillColor(i == 1 ? laneGold : sf::Color(122, 184, 111, 70));
            renderTarget().draw(laneMark);

            for (int step = 0; step < 6; ++step) {
                sf::RectangleShape pip(sf::Vector2f(9.f, 9.f));
                pip.setOrigin(4.5f, 4.5f);
                pip.setRotation(45.f);
                pip.setPosition(430.f + static_cast<float>(step) * 92.f,
                                308.f + static_cast<float>(i) * 45.f
                                    + (i == 0 ? -static_cast<float>(step) * 2.2f
                                              : (i == 2 ? static_cast<float>(step) * 2.2f : 0.f)));
                pip.setFillColor(i == 1 ? sf::Color(255, 231, 137, 130) : sf::Color(144, 207, 130, 82));
                renderTarget().draw(pip);
            }
        }

        const auto drawBaseMark = [this](sf::Vector2f pos, sf::Color color, bool rightFacing) {
            sf::RectangleShape base(sf::Vector2f(52.f, 42.f));
            base.setOrigin(26.f, 21.f);
            base.setPosition(pos);
            base.setFillColor(sf::Color(color.r, color.g, color.b, 158));
            base.setOutlineColor(sf::Color(255, 239, 180, 148));
            base.setOutlineThickness(1.4f);
            renderTarget().draw(base);

            sf::RectangleShape roof(sf::Vector2f(40.f, 12.f));
            roof.setOrigin(20.f, 6.f);
            roof.setPosition(pos + sf::Vector2f(0.f, -18.f));
            roof.setFillColor(sf::Color(22, 28, 25, 230));
            roof.setOutlineColor(sf::Color(color.r, color.g, color.b, 230));
            roof.setOutlineThickness(1.2f);
            renderTarget().draw(roof);

            sf::ConvexShape flag(3);
            const float dir = rightFacing ? 1.f : -1.f;
            flag.setPoint(0, pos + sf::Vector2f(dir * 19.f, -34.f));
            flag.setPoint(1, pos + sf::Vector2f(dir * 19.f, -17.f));
            flag.setPoint(2, pos + sf::Vector2f(dir * 38.f, -26.f));
            flag.setFillColor(color);
            renderTarget().draw(flag);
        };
        drawBaseMark({370.f, 386.f}, sf::Color(218, 76, 60), true);
        drawBaseMark({950.f, 386.f}, sf::Color(68, 132, 218), false);

        sf::Text title("COMMAND LINES", myfont, 48);
        title.setFillColor(sf::Color(255, 236, 176));
        title.setOutlineColor(sf::Color(30, 22, 14, 220));
        title.setOutlineThickness(2.f);
        title.setLetterSpacing(1.25f);
        title.setPosition(365.f, 146.f);
        renderTarget().draw(title);

        sf::Text subtitle("A 10-minute rogue RTS auto-battler", myfont, 18);
        subtitle.setFillColor(sf::Color(206, 224, 190));
        subtitle.setPosition(415.f, 214.f);
        renderTarget().draw(subtitle);

        const char* cards[] = {"1. Grow CMD", "2. Pick a lane", "3. Draft tactics"};
        const char* details[] = {"income + drones", "TOP / MID / BOT", "big power spikes"};
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape card(sf::Vector2f(188.f, 76.f));
            card.setPosition(368.f + static_cast<float>(i) * 204.f, 274.f);
            card.setFillColor(sf::Color(36, 46, 40, 230));
            card.setOutlineColor(i == 1 ? sf::Color(126, 190, 139, 210) : sf::Color(192, 141, 66, 190));
            card.setOutlineThickness(1.4f);
            renderTarget().draw(card);

            sf::CircleShape gem(12.f, 6);
            gem.setOrigin(12.f, 12.f);
            gem.setPosition(card.getPosition() + sf::Vector2f(24.f, 22.f));
            gem.setFillColor(i == 0 ? sf::Color(244, 199, 90) : (i == 1 ? sf::Color(109, 184, 138) : sf::Color(140, 184, 238)));
            renderTarget().draw(gem);

            sf::Text cardText(cards[i], myfont, 14);
            cardText.setFillColor(sf::Color(236, 232, 202));
            cardText.setPosition(card.getPosition() + sf::Vector2f(46.f, 15.f));
            renderTarget().draw(cardText);

            sf::Text detail(details[i], myfont, 11);
            detail.setFillColor(sf::Color(190, 207, 174));
            detail.setPosition(card.getPosition() + sf::Vector2f(46.f, 42.f));
            renderTarget().draw(detail);
        }

        startBtn.setPosition(470.f, 410.f);
        startHelpBtn.setPosition(710.f, 410.f);
        renderTarget().draw(startBtn);
        renderTarget().draw(startHelpBtn);

        sf::Text hint("Simple loop: Economy -> Barracks -> Pick a lane -> Draft upgrades", myfont, 13);
        hint.setFillColor(sf::Color(229, 214, 160));
        hint.setPosition(430.f, 516.f);
        renderTarget().draw(hint);

        sf::Text noMicro("No micro-control: you build the engine, armies execute the fight.", myfont, 12);
        noMicro.setFillColor(sf::Color(178, 207, 180));
        noMicro.setPosition(466.f, 542.f);
        renderTarget().draw(noMicro);

        if (tutorialVisible) {
            drawTutorialOverlay();
        }
        break;
    }
    case SCENE_GAME: {
        const sf::View defaultView = logicalView();
        const sf::View gameView = logicalView(currentShakeOffset());
        renderTarget().setView(gameView);

        for (const auto& tile : tiles) {
            tile.drawGround(renderTarget(), sf::RenderStates::Default);
        }

        drawLaneGuides();
        drawGridOverlay();

        if (gameTimeSeconds >= config::EscalationStartSeconds) {
            const auto drawCommandZone = [this](Point basePoint, sf::Color color, int pressure) {
                const float radius = static_cast<float>(config::CommandZonePressureRadius * SqureSize);
                const float pulse = 0.5f + 0.5f * std::sin(gameTimeSeconds * 3.2f);
                sf::CircleShape zone(radius, 72);
                zone.setOrigin(radius, radius);
                zone.setPosition((basePoint.x + 1.f) * SqureSize, (basePoint.y + 1.f) * SqureSize);
                zone.setFillColor(sf::Color(color.r, color.g, color.b, pressure > 0 ? 8 : 3));
                zone.setOutlineColor(sf::Color(color.r, color.g, color.b,
                    static_cast<sf::Uint8>((pressure > 0 ? 75.f : 30.f) + pulse * (pressure > 0 ? 55.f : 18.f))));
                zone.setOutlineThickness(pressure > 0 ? 2.f : 1.f);
                renderTarget().draw(zone);
            };
            drawCommandZone(Red_baseP, sf::Color(222, 83, 61),
                            unitsNearPoint(AI, Red_baseP, config::CommandZonePressureRadius));
            drawCommandZone(Blue_baseP, sf::Color(73, 143, 224),
                            unitsNearPoint(PLAYER, Blue_baseP, config::CommandZonePressureRadius));
        }

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
                tilePtr->drawObject(renderTarget(), sf::RenderStates::Default);
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
            renderTarget().draw(shield);
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
            renderTarget().draw(status);
        };
        const auto pushBase = [&](DisMoveableUnit* base, int team) {
            if (base == nullptr) {
                return;
            }
            renderItems.push_back({static_cast<float>((base->y + config::BaseFootprintSize) * SqureSize), base->x, [this, base, team, drawBaseShield, drawBasePerks]() {
                drawBaseShield(base, team);
                if (team == PLAYER && base->UnitState == UState::UNITCLICK) {
                    MapPos selected(Point(base->x, base->y), true, true);
                    renderTarget().draw(selected);
                }
                renderTarget().draw(*base);
                renderTarget().draw(base->UnitText);
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
                drawUnitCombatCue(renderTarget(), *unit, gameTimeSeconds);
                drawUnitBase(renderTarget(), Point(unit->x, unit->y), color);
                if (playerUnit && unit->UnitState == UState::UNITCLICK) {
                    MapPos selected(Point(unit->x, unit->y), tile::Choosen);
                    renderTarget().draw(selected);
                }
                renderTarget().draw(*unit);
                drawUnitHealthBar(renderTarget(), *unit,
                                  unit->UnitState == UState::UNITCLICK || MosOnUnit == unit);
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

        effects.draw(renderTarget());

        renderTarget().setView(defaultView);
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
        const sf::View defaultView = logicalView();
        renderTarget().setView(defaultView);

        sf::RectangleShape backdrop(sf::Vector2f(static_cast<float>(config::WindowWidth), static_cast<float>(config::WindowHeight)));
        backdrop.setFillColor(gameWin ? sf::Color(30, 54, 43) : sf::Color(55, 39, 34));
        renderTarget().draw(backdrop);

        for (int laneIndex = 0; laneIndex < lane::Count; ++laneIndex) {
            sf::RectangleShape laneBand(sf::Vector2f(1120.f, 8.f));
            laneBand.setOrigin(560.f, 4.f);
            laneBand.setPosition(694.f, 160.f + static_cast<float>(laneIndex) * 190.f);
            laneBand.setRotation(laneIndex == lane::Top ? -7.f : (laneIndex == lane::Bot ? 7.f : 0.f));
            laneBand.setFillColor(gameWin ? sf::Color(220, 183, 88, 32) : sf::Color(169, 103, 78, 36));
            renderTarget().draw(laneBand);
        }
        for (int i = 0; i < 30; ++i) {
            sf::RectangleShape marker(sf::Vector2f(8.f + static_cast<float>(i % 3) * 3.f, 8.f));
            marker.setPosition(static_cast<float>(48 + (i * 181) % 1280),
                               static_cast<float>(34 + (i * 103) % 640));
            marker.setFillColor(gameWin ? sf::Color(239, 196, 102, 34) : sf::Color(210, 113, 86, 38));
            renderTarget().draw(marker);
        }

        sf::RectangleShape panelShadow(sf::Vector2f(620.f, 344.f));
        panelShadow.setPosition(372.f, 154.f);
        panelShadow.setFillColor(sf::Color(8, 10, 9, 120));
        renderTarget().draw(panelShadow);

        sf::RectangleShape panel(sf::Vector2f(620.f, 344.f));
        panel.setPosition(360.f, 140.f);
        panel.setFillColor(sf::Color(39, 49, 43, 238));
        panel.setOutlineColor(gameWin ? sf::Color(239, 196, 102) : sf::Color(210, 113, 86));
        panel.setOutlineThickness(2.4f);
        renderTarget().draw(panel);

        sf::RectangleShape panelTop(sf::Vector2f(596.f, 4.f));
        panelTop.setPosition(372.f, 154.f);
        panelTop.setFillColor(gameWin ? sf::Color(255, 230, 142, 132) : sf::Color(255, 152, 116, 122));
        renderTarget().draw(panelTop);

        sf::Text result(gameWin ? "VICTORY" : "DEFEAT", myfont, 50);
        result.setFillColor(gameWin ? sf::Color(255, 236, 168) : sf::Color(255, 184, 142));
        result.setOutlineColor(sf::Color(22, 18, 14, 230));
        result.setOutlineThickness(2.f);
        result.setLetterSpacing(1.25f);
        result.setPosition(gameWin ? 545.f : 560.f, 178.f);
        renderTarget().draw(result);

        const std::string summaryText = gameWin
            ? "You destroyed the enemy base."
            : "Your base fell. Regroup and try again.";
        sf::Text summary(summaryText, myfont, 18);
        summary.setFillColor(sf::Color(226, 232, 202));
        const sf::FloatRect summaryBounds = summary.getLocalBounds();
        summary.setOrigin(summaryBounds.left + summaryBounds.width * 0.5f, 0.f);
        summary.setPosition(670.f, 255.f);
        renderTarget().draw(summary);

        const auto drawResultChip = [this](sf::Vector2f pos, const std::string& label, const std::string& value, sf::Color accent) {
            sf::RectangleShape chip(sf::Vector2f(160.f, 52.f));
            chip.setPosition(pos);
            chip.setFillColor(sf::Color(27, 36, 32, 235));
            chip.setOutlineColor(sf::Color(accent.r, accent.g, accent.b, 170));
            chip.setOutlineThickness(1.3f);
            renderTarget().draw(chip);

            sf::Text labelText(label, myfont, 10);
            labelText.setFillColor(sf::Color(183, 196, 166));
            labelText.setPosition(pos + sf::Vector2f(12.f, 8.f));
            renderTarget().draw(labelText);

            sf::Text valueText(value, myfont, 18);
            valueText.setFillColor(sf::Color(255, 236, 168));
            valueText.setPosition(pos + sf::Vector2f(12.f, 24.f));
            renderTarget().draw(valueText);
        };
        drawResultChip({410.f, 304.f}, "TIME", std::to_string(static_cast<int>(gameTimeSeconds)) + "s", sf::Color(239, 196, 102));
        drawResultChip({590.f, 304.f}, "YOUR TECH", "Lv " + std::to_string(playerUpgradeLevel), sf::Color(126, 206, 142));
        drawResultChip({770.f, 304.f}, "AI TECH", "Lv " + std::to_string(aiUpgradeLevel), sf::Color(116, 184, 255));

        endGame.setPosition(550.f, 372.f);
        renderTarget().draw(endGame);
        break;
    }
    default:
        break;
    }
    
}

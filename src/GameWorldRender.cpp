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


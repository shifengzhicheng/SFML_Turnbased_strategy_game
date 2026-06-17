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
    const sf::Color lineColor(53, 74, 58, 42);
    const sf::Color majorColor(40, 58, 45, 62);

    for (int x = 0; x <= width; x += SqureSize) {
        const sf::Color color = (x / SqureSize) % 4 == 0 ? majorColor : lineColor;
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), color));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(height)), color));
    }
    for (int y = 0; y <= height; y += SqureSize) {
        const sf::Color color = (y / SqureSize) % 4 == 0 ? majorColor : lineColor;
        lines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), color));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(width), static_cast<float>(y)), color));
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

void Game::drawResourceNode(const ResourceNode& node)
{
    const sf::Vector2f origin(
        node.point.x * SqureSize,
        node.point.y * SqureSize);
    const float t = node.pulseClock.getElapsedTime().asSeconds();
    const int glow = static_cast<int>(28.f + (0.5f + 0.5f * std::sin(t * 4.f)) * 34.f);

    sf::RectangleShape shadow(sf::Vector2f(24.f, 5.f));
    shadow.setPosition(origin + sf::Vector2f(-2.f, 17.f));
    shadow.setFillColor(sf::Color(31, 22, 9, 95));
    window.draw(shadow);

    sf::RectangleShape aura(sf::Vector2f(28.f, 24.f));
    aura.setPosition(origin + sf::Vector2f(-4.f, -4.f));
    aura.setFillColor(sf::Color(226, 180, 63, static_cast<sf::Uint8>(glow)));
    aura.setOutlineColor(sf::Color(255, 226, 117, 118));
    aura.setOutlineThickness(1.f);
    window.draw(aura);

    sf::RectangleShape core(sf::Vector2f(6.f, 17.f));
    core.setPosition(origin + sf::Vector2f(8.f, 1.f));
    core.setFillColor(sf::Color(248, 181, 51));
    window.draw(core);
    sf::RectangleShape top(sf::Vector2f(4.f, 5.f));
    top.setPosition(origin + sf::Vector2f(9.f, -4.f));
    top.setFillColor(sf::Color(255, 243, 134));
    window.draw(top);
    sf::RectangleShape left(sf::Vector2f(5.f, 9.f));
    left.setPosition(origin + sf::Vector2f(4.f, 8.f));
    left.setFillColor(sf::Color(210, 139, 42));
    window.draw(left);
    sf::RectangleShape right(sf::Vector2f(5.f, 10.f));
    right.setPosition(origin + sf::Vector2f(13.f, 7.f));
    right.setFillColor(sf::Color(255, 210, 78));
    window.draw(right);
    sf::RectangleShape shine(sf::Vector2f(2.f, 12.f));
    shine.setPosition(origin + sf::Vector2f(10.f, 3.f));
    shine.setFillColor(sf::Color(255, 252, 183, 210));
    window.draw(shine);

    sf::RectangleShape labelBg(sf::Vector2f(30.f, 12.f));
    labelBg.setPosition(origin + sf::Vector2f(9.f, -20.f));
    labelBg.setFillColor(sf::Color(40, 32, 19, 188));
    labelBg.setOutlineColor(sf::Color(255, 225, 128, 150));
    labelBg.setOutlineThickness(1.f);
    window.draw(labelBg);

    sf::Text label("CMD", myfont, 8);
    label.setFillColor(sf::Color(255, 235, 145));
    label.setPosition(labelBg.getPosition() + sf::Vector2f(4.f, 1.f));
    window.draw(label);
}

void Game::drawResourceNodes()
{
    for (const auto& node : resources) {
        drawResourceNode(node);
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

void Game::drawWorkerSprite(const Worker& workerUnit)
{
    const sf::Vector2f origin(workerUnit.point.x * SqureSize, workerUnit.point.y * SqureSize);
    const sf::Color cloth = workerUnit.team == PLAYER ? sf::Color(236, 145, 61) : sf::Color(83, 144, 218);
    const sf::Color outline(39, 35, 26);
    const float swing = workerUnit.state == worker::Building ? std::sin(gameTimeSeconds * 11.f) * 3.f : 0.f;

    sf::RectangleShape shadow(sf::Vector2f(16.f, 3.f));
    shadow.setPosition(origin + sf::Vector2f(2.f, 17.f));
    shadow.setFillColor(sf::Color(12, 17, 13, 78));
    window.draw(shadow);

    sf::RectangleShape body(sf::Vector2f(8.f, 10.f));
    body.setPosition(origin + sf::Vector2f(6.f, 8.f));
    body.setFillColor(outline);
    window.draw(body);
    body.setSize(sf::Vector2f(6.f, 8.f));
    body.setPosition(origin + sf::Vector2f(7.f, 9.f));
    body.setFillColor(cloth);
    window.draw(body);

    sf::RectangleShape head(sf::Vector2f(6.f, 5.f));
    head.setPosition(origin + sf::Vector2f(7.f, 4.f));
    head.setFillColor(sf::Color(226, 178, 123));
    window.draw(head);
    sf::RectangleShape helm(sf::Vector2f(8.f, 3.f));
    helm.setPosition(origin + sf::Vector2f(6.f, 2.f));
    helm.setFillColor(sf::Color(242, 202, 87));
    window.draw(helm);

    if (workerUnit.state == worker::Building) {
        sf::RectangleShape handle(sf::Vector2f(11.f, 2.f));
        handle.setOrigin(1.f, 1.f);
        handle.setPosition(origin + sf::Vector2f(12.f, 7.f + swing));
        handle.setRotation(32.f + swing * 5.f);
        handle.setFillColor(sf::Color(102, 68, 40));
        window.draw(handle);
        sf::RectangleShape headTool(sf::Vector2f(5.f, 2.f));
        headTool.setOrigin(1.f, 1.f);
        headTool.setPosition(origin + sf::Vector2f(19.f, 11.f + swing));
        headTool.setRotation(32.f + swing * 5.f);
        headTool.setFillColor(sf::Color(232, 224, 189));
        window.draw(headTool);
    }
}

void Game::drawWorkers()
{
    for (const auto& workerUnit : workers) {
        drawWorkerSprite(workerUnit);
    }
}

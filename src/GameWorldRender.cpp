#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "LaneGeometry.h"
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
    const sf::Color lineColor(38, 56, 43, 24);
    const sf::Color majorColor(36, 52, 42, 40);

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
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const char* labels[] = {"TOP", "MID", "BOT"};

    const auto drawSegment = [this](Point a, Point b, sf::Color fill, sf::Color outline, bool active) {
        const int steps = std::max(std::abs(b.x - a.x), std::abs(b.y - a.y)) * 2 + 1;
        const float markerSize = active ? SqureSize * 0.48f : SqureSize * 0.34f;
        for (int i = 0; i <= steps; ++i) {
            if (!active && i % 2 != 0) {
                continue;
            }
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const float gx = static_cast<float>(a.x) + static_cast<float>(b.x - a.x) * t;
            const float gy = static_cast<float>(a.y) + static_cast<float>(b.y - a.y) * t;
            const sf::Vector2f center((gx + 0.5f) * SqureSize, (gy + 0.5f) * SqureSize);

            sf::RectangleShape marker(sf::Vector2f(markerSize, markerSize * 0.28f));
            marker.setOrigin(marker.getSize() * 0.5f);
            marker.setPosition(center);
            marker.setRotation(static_cast<float>((b.y - a.y) == 0 ? 0 : (b.y > a.y ? 12 : -12)));
            marker.setFillColor(fill);
            marker.setOutlineColor(outline);
            marker.setOutlineThickness(active ? 1.f : 0.f);
            window.draw(marker);
        }
    };

    for (int i = 0; i < lane::Count; ++i) {
        const bool active = i == playerSelectedLane;
        const auto route = lane_geometry::laneRoute(mapW, mapH, i);
        const sf::Color fill = active
            ? sf::Color(255, 219, 105, 132)
            : sf::Color(125, 184, 111, 48);
        const sf::Color outline = active
            ? sf::Color(255, 244, 175, 170)
            : sf::Color(56, 96, 60, 70);

        for (std::size_t p = 1; p < route.size(); ++p) {
            drawSegment(route[p - 1], route[p], fill, outline, active);
        }

        for (std::size_t p = 1; p + 1 < route.size(); ++p) {
            const sf::Vector2f center(
                (static_cast<float>(route[p].x) + 0.5f) * SqureSize,
                (static_cast<float>(route[p].y) + 0.5f) * SqureSize);
            sf::RectangleShape flag(sf::Vector2f(active ? 14.f : 10.f, active ? 12.f : 9.f));
            flag.setOrigin(flag.getSize() * 0.5f);
            flag.setPosition(center + sf::Vector2f(0.f, active ? -2.f : 0.f));
            flag.setFillColor(active ? sf::Color(255, 224, 105, 215) : sf::Color(151, 198, 126, 118));
            flag.setOutlineColor(sf::Color(46, 60, 42, 160));
            flag.setOutlineThickness(1.f);
            window.draw(flag);
            sf::RectangleShape flagTail(sf::Vector2f(active ? 4.f : 3.f, active ? 8.f : 6.f));
            flagTail.setPosition(center + sf::Vector2f(-1.5f, 3.f));
            flagTail.setFillColor(sf::Color(46, 60, 42, active ? 180 : 112));
            window.draw(flagTail);
        }

        sf::Text laneText(labels[i], myfont, 11);
        laneText.setFillColor(active ? sf::Color(255, 238, 169, 225) : sf::Color(213, 225, 198, 128));
        laneText.setOutlineColor(sf::Color(24, 29, 23, 170));
        laneText.setOutlineThickness(1.f);
        const Point labelPoint = route[1];
        laneText.setPosition((labelPoint.x - 1) * SqureSize, (labelPoint.y - 1.25f) * SqureSize);
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

    sf::CircleShape shadow(14.f, 28);
    shadow.setScale(1.15f, 0.26f);
    shadow.setPosition(origin + sf::Vector2f(-2.f, 16.f));
    shadow.setFillColor(sf::Color(31, 22, 9, 95));
    window.draw(shadow);

    sf::CircleShape aura(16.f, 32);
    aura.setOrigin(16.f, 16.f);
    aura.setPosition(origin + sf::Vector2f(12.f, 9.f));
    aura.setScale(1.12f, 0.86f);
    aura.setFillColor(sf::Color(226, 180, 63, static_cast<sf::Uint8>(glow)));
    aura.setOutlineColor(sf::Color(255, 226, 117, 118));
    aura.setOutlineThickness(1.f);
    window.draw(aura);

    const auto drawCrystal = [this, origin](sf::Vector2f tip, sf::Vector2f left, sf::Vector2f right,
                                            sf::Vector2f foot, sf::Color body, sf::Color light) {
        sf::ConvexShape crystal(4);
        crystal.setPoint(0, origin + tip);
        crystal.setPoint(1, origin + right);
        crystal.setPoint(2, origin + foot);
        crystal.setPoint(3, origin + left);
        crystal.setFillColor(body);
        crystal.setOutlineColor(sf::Color(89, 61, 24, 210));
        crystal.setOutlineThickness(1.f);
        window.draw(crystal);

        sf::Vertex shine[] = {
            sf::Vertex(origin + tip + sf::Vector2f(1.f, 3.f), light),
            sf::Vertex(origin + foot + sf::Vector2f(-1.f, -3.f), sf::Color(light.r, light.g, light.b, 60))
        };
        window.draw(shine, 2, sf::Lines);
    };

    drawCrystal(sf::Vector2f(12.f, -7.f), sf::Vector2f(6.f, 7.f), sf::Vector2f(16.f, 6.f), sf::Vector2f(11.f, 20.f),
                sf::Color(241, 173, 46), sf::Color(255, 253, 190, 230));
    drawCrystal(sf::Vector2f(6.f, 2.f), sf::Vector2f(2.f, 11.f), sf::Vector2f(9.f, 10.f), sf::Vector2f(6.f, 19.f),
                sf::Color(205, 131, 39), sf::Color(255, 228, 128, 180));
    drawCrystal(sf::Vector2f(18.f, 3.f), sf::Vector2f(14.f, 10.f), sf::Vector2f(22.f, 11.f), sf::Vector2f(18.f, 19.f),
                sf::Color(255, 207, 80), sf::Color(255, 249, 166, 190));

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

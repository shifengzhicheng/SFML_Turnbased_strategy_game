#include "ArtAssets.h"
#include "Config.h"
#include "LaneGeometry.h"
#include "Tile.h"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    sf::Vector2f footAnchoredSpritePosition(const sf::Texture& texture, sf::Vector2f tileTopLeft, float scale)
    {
        const sf::Vector2u size = texture.getSize();
        const sf::Vector2f foot(tileTopLeft.x + config::TileSize * 0.5f,
                                tileTopLeft.y + config::TileSize + 1.f);
        return sf::Vector2f(
            foot.x - static_cast<float>(size.x) * scale * 0.5f,
            foot.y - static_cast<float>(size.y) * scale);
    }

    void drawLabel(sf::RenderTexture& canvas, const sf::Font& font, const std::string& text, float x, float y)
    {
        sf::Text label(text, font, 14);
        label.setFillColor(sf::Color(233, 224, 194));
        label.setPosition(x, y);
        canvas.draw(label);
    }

    void drawPreviewLane(sf::RenderTexture& canvas, sf::Vector2f boardOrigin, int laneIndex, bool active)
    {
        constexpr int previewCols = 23;
        constexpr int previewRows = 15;
        const auto route = lane_geometry::laneRoute(previewCols, previewRows, laneIndex);
        const sf::Color fill = active ? sf::Color(255, 218, 112, 116) : sf::Color(128, 190, 124, 58);
        for (std::size_t p = 1; p < route.size(); ++p) {
            const Point a = route[p - 1];
            const Point b = route[p];
            const int steps = std::max(std::abs(b.x - a.x), std::abs(b.y - a.y)) * 2 + 1;
            for (int i = 0; i <= steps; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                const float gx = static_cast<float>(a.x) + static_cast<float>(b.x - a.x) * t;
                const float gy = static_cast<float>(a.y) + static_cast<float>(b.y - a.y) * t;
                sf::RectangleShape marker(sf::Vector2f(active ? 14.f : 10.f, active ? 5.f : 4.f));
                marker.setOrigin(marker.getSize() * 0.5f);
                marker.setPosition(boardOrigin + sf::Vector2f((gx + 0.5f) * config::TileSize,
                                                              (gy + 0.5f) * config::TileSize));
                marker.setFillColor(fill);
                canvas.draw(marker);
            }
        }
    }
}

int main(int argc, char** argv)
{
    sf::Font font;
    if (!font.loadFromFile("data/ttf/arial.ttf")) {
        std::cerr << "Failed to load data/ttf/arial.ttf\n";
        return 1;
    }

    sf::RenderTexture canvas;
    if (!canvas.create(920, 560)) {
        return 1;
    }
    canvas.clear(sf::Color(28, 35, 32));

    drawLabel(canvas, font, "Art board: in-game scale plus close-up checks", 24.f, 18.f);

    sf::RectangleShape board(sf::Vector2f(560.f, 360.f));
    board.setPosition(24.f, 72.f);
    board.setFillColor(sf::Color(212, 224, 170));
    board.setOutlineColor(sf::Color(219, 166, 75));
    board.setOutlineThickness(2.f);
    canvas.draw(board);

    for (int y = 82; y < 422; y += config::TileSize) {
        for (int x = 34; x < 574; x += config::TileSize) {
            sf::RectangleShape dash(sf::Vector2f(5.f, 1.f));
            dash.setFillColor(sf::Color(162, 184, 118, 70));
            dash.setPosition(static_cast<float>(x + ((x + y) % 7)), static_cast<float>(y + ((x * 3 + y) % 8)));
            canvas.draw(dash);
        }
    }

    sf::VertexArray grid(sf::Lines);
    for (int x = 24; x <= 584; x += config::TileSize) {
        grid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 72.f), sf::Color(53, 74, 58, 62)));
        grid.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 432.f), sf::Color(53, 74, 58, 62)));
    }
    for (int y = 72; y <= 432; y += config::TileSize) {
        grid.append(sf::Vertex(sf::Vector2f(24.f, static_cast<float>(y)), sf::Color(53, 74, 58, 62)));
        grid.append(sf::Vertex(sf::Vector2f(584.f, static_cast<float>(y)), sf::Color(53, 74, 58, 62)));
    }
    canvas.draw(grid);

    drawPreviewLane(canvas, sf::Vector2f(24.f, 72.f), lane::Top, false);
    drawPreviewLane(canvas, sf::Vector2f(24.f, 72.f), lane::Mid, true);
    drawPreviewLane(canvas, sf::Vector2f(24.f, 72.f), lane::Bot, false);

    const tile::ID samples[] = {
        tile::Empty,
        tile::Tree,
        tile::Mount,
        tile::River,
        tile::Resource,
        tile::Player_Barracks,
        tile::Player_Tower
    };
    for (int i = 0; i < 7; ++i) {
        MapPos tileSample(sf::IntRect(42 + i * 34, 386, config::TileSize, config::TileSize), samples[i]);
        tileSample.drawGround(canvas, sf::RenderStates::Default);
        tileSample.drawObject(canvas, sf::RenderStates::Default);
    }

    sf::Texture infantry, shooter, cavalry, siege, guardian, enemy, endTurn, infButton, shoButton, cavButton, siegeButton, guardianButton;
    art::makeUnitTexture(infantry, art::UnitKind::Infantry, art::Team::Player);
    art::makeUnitTexture(shooter, art::UnitKind::Shooter, art::Team::Player);
    art::makeUnitTexture(cavalry, art::UnitKind::Cavalry, art::Team::Player);
    art::makeUnitTexture(siege, art::UnitKind::Siege, art::Team::Player);
    art::makeUnitTexture(guardian, art::UnitKind::Guardian, art::Team::Player);
    art::makeUnitTexture(enemy, art::UnitKind::Shooter, art::Team::Enemy);
    art::makeButtonTexture(endTurn, font, "END TURN", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(infButton, font, "INF", art::ButtonState::Normal, sf::Vector2u(128, 42), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(shoButton, font, "BOW", art::ButtonState::Hover, sf::Vector2u(128, 42), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(cavButton, font, "CAV", art::ButtonState::Pressed, sf::Vector2u(128, 42), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(siegeButton, font, "SGE", art::ButtonState::Normal, sf::Vector2u(128, 42), art::UnitKind::Siege, art::Team::Player);
    art::makeButtonTexture(guardianButton, font, "GRD", art::ButtonState::Hover, sf::Vector2u(128, 42), art::UnitKind::Guardian, art::Team::Player);

    const sf::Vector2f unitPositions[] = {{104.f, 124.f}, {164.f, 164.f}, {244.f, 244.f}, {324.f, 204.f}, {404.f, 244.f}, {464.f, 164.f}};
    const sf::Texture* unitTextures[] = {&infantry, &shooter, &cavalry, &siege, &guardian, &enemy};
    for (int i = 0; i < 6; ++i) {
        sf::RectangleShape base(sf::Vector2f(18.f, 4.f));
        base.setPosition(unitPositions[i].x + 1.f, unitPositions[i].y + 16.f);
        base.setFillColor(i == 5 ? sf::Color(61, 128, 206, 110) : sf::Color(218, 76, 60, 110));
        canvas.draw(base);

        sf::Sprite sprite(*unitTextures[i]);
        sprite.setScale(config::UnitSpriteScale, config::UnitSpriteScale);
        sprite.setPosition(footAnchoredSpritePosition(*unitTextures[i], unitPositions[i], config::UnitSpriteScale));
        canvas.draw(sprite);
    }

    const sf::Vector2f closePositions[] = {{96.f, 448.f}, {176.f, 448.f}, {256.f, 448.f}, {336.f, 448.f}, {416.f, 448.f}, {496.f, 448.f}};
    const char* names[] = {"Inf", "Bow", "Cav", "Siege", "Guard", "Enemy"};
    for (int i = 0; i < 6; ++i) {
        sf::Sprite sprite(*unitTextures[i]);
        sprite.setOrigin(unitTextures[i]->getSize().x / 2.f, unitTextures[i]->getSize().y / 2.f);
        sprite.setPosition(closePositions[i]);
        sprite.setScale(1.05f, 1.05f);
        canvas.draw(sprite);
        drawLabel(canvas, font, names[i], closePositions[i].x - 14.f, closePositions[i].y + 24.f);
    }

    sf::CircleShape resourceAura(18.f, 36);
    resourceAura.setOrigin(18.f, 18.f);
    resourceAura.setPosition(494.f, 292.f);
    resourceAura.setFillColor(sf::Color(226, 180, 63, 50));
    resourceAura.setOutlineColor(sf::Color(226, 180, 63, 180));
    resourceAura.setOutlineThickness(2.f);
    canvas.draw(resourceAura);
    sf::ConvexShape crystal(6);
    crystal.setPoint(0, sf::Vector2f(494.f, 278.f));
    crystal.setPoint(1, sf::Vector2f(506.f, 287.f));
    crystal.setPoint(2, sf::Vector2f(502.f, 300.f));
    crystal.setPoint(3, sf::Vector2f(494.f, 308.f));
    crystal.setPoint(4, sf::Vector2f(486.f, 300.f));
    crystal.setPoint(5, sf::Vector2f(482.f, 287.f));
    crystal.setFillColor(sf::Color(255, 211, 82));
    crystal.setOutlineColor(sf::Color(100, 74, 30));
    crystal.setOutlineThickness(2.f);
    canvas.draw(crystal);

    sf::RectangleShape panel(sf::Vector2f(176.f, 500.f));
    panel.setPosition(642.f, 40.f);
    panel.setFillColor(sf::Color(24, 31, 28));
    panel.setOutlineColor(sf::Color(219, 166, 75));
    panel.setOutlineThickness(2.f);
    canvas.draw(panel);
    drawLabel(canvas, font, "WAR ROOM", 660.f, 58.f);
    drawLabel(canvas, font, "CMD 760  +38/t", 660.f, 96.f);
    drawLabel(canvas, font, "TECH 8/15", 660.f, 116.f);
    drawLabel(canvas, font, "LANE: BOT", 660.f, 136.f);
    drawLabel(canvas, font, "Queue units,", 660.f, 170.f);
    drawLabel(canvas, font, "mastery spikes", 660.f, 190.f);

    sf::Sprite b1(endTurn), b2(infButton), b3(shoButton), b4(cavButton), b5(siegeButton), b6(guardianButton);
    b1.setPosition(666.f, 226.f);
    b2.setPosition(666.f, 278.f);
    b3.setPosition(666.f, 326.f);
    b4.setPosition(666.f, 374.f);
    b5.setPosition(666.f, 422.f);
    b6.setPosition(666.f, 470.f);
    canvas.draw(b1);
    canvas.draw(b2);
    canvas.draw(b3);
    canvas.draw(b4);
    canvas.draw(b5);
    canvas.draw(b6);

    canvas.display();
    const std::string out = argc > 1 ? argv[1] : "build/art_preview.png";
    if (!canvas.getTexture().copyToImage().saveToFile(out)) {
        return 1;
    }
    std::cout << out << "\n";
    return 0;
}

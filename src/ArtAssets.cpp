#include "ArtAssets.h"
#include "ArtInternal.h"
#include "Config.h"

#include <algorithm>
#include <cmath>

namespace art
{
    using namespace art_internal;
    sf::Color teamColor(Team team)
    {
        switch (team) {
        case Team::Player:
            return sf::Color(222, 75, 59);
        case Team::Enemy:
            return sf::Color(63, 122, 201);
        case Team::Neutral:
        default:
            return sf::Color(222, 179, 91);
        }
    }

    sf::Color teamAccent(Team team)
    {
        switch (team) {
        case Team::Player:
            return sf::Color(110, 32, 30);
        case Team::Enemy:
            return sf::Color(26, 58, 111);
        case Team::Neutral:
        default:
            return sf::Color(88, 68, 38);
        }
    }

    void makeIntroTexture(sf::Texture& texture, const sf::Font& font)
    {
        sf::RenderTexture canvas;
        const sf::Vector2u size(config::WindowWidth, config::WindowHeight);
        if (!createCanvas(canvas, size)) {
            return;
        }

        for (unsigned int y = 0; y < size.y; ++y) {
            const float t = static_cast<float>(y) / static_cast<float>(size.y - 1);
            sf::RectangleShape line(sf::Vector2f(static_cast<float>(size.x), 1.f));
            line.setPosition(0.f, static_cast<float>(y));
            line.setFillColor(mix(sf::Color(29, 49, 44), sf::Color(227, 195, 126), t));
            canvas.draw(line);
        }

        sf::VertexArray sun(sf::TriangleFan, 42);
        sun[0].position = sf::Vector2f(950.f, 155.f);
        sun[0].color = sf::Color(255, 221, 125, 210);
        for (std::size_t i = 1; i < sun.getVertexCount(); ++i) {
            const float a = static_cast<float>(i - 1) / 40.f * 2.f * static_cast<float>(config::Pi);
            sun[i].position = sf::Vector2f(950.f + std::cos(a) * 100.f, 155.f + std::sin(a) * 100.f);
            sun[i].color = sf::Color(255, 195, 85, 0);
        }
        canvas.draw(sun);

        for (int i = 0; i < 26; ++i) {
            sf::RectangleShape stripe(sf::Vector2f(static_cast<float>(size.x), 2.f));
            stripe.setPosition(0.f, 165.f + i * 18.f);
            stripe.setFillColor(sf::Color(255, 248, 207, static_cast<sf::Uint8>(50 - i)));
            canvas.draw(stripe);
        }

        for (int i = 0; i < 16; ++i) {
            sf::CircleShape hill(260.f - i * 4.f, 64);
            hill.setOrigin(260.f - i * 4.f, 260.f - i * 4.f);
            hill.setScale(1.7f, 0.42f);
            hill.setPosition(80.f + i * 92.f, 610.f + (i % 3) * 18.f);
            hill.setFillColor(i % 2 == 0 ? sf::Color(41, 74, 56, 180) : sf::Color(67, 91, 59, 150));
            canvas.draw(hill);
        }

        for (int x = 0; x < config::MapWidth; x += config::TileSize * 2) {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), sf::Color(255, 255, 255, 18)),
                sf::Vertex(sf::Vector2f(static_cast<float>(x + 190), static_cast<float>(size.y)), sf::Color(255, 255, 255, 3))
            };
            canvas.draw(line, 2, sf::Lines);
        }

        drawTextCentered(canvas, font, "PROJECT WAR", 56, sf::Color(255, 246, 208), sf::FloatRect(95.f, 82.f, 520.f, 80.f));
        drawTextCentered(canvas, font, "fast turns / clean tactics / hard hits", 20, sf::Color(220, 233, 201), sf::FloatRect(120.f, 158.f, 470.f, 36.f));
        canvas.display();
        commitTexture(texture, canvas);
    }

    void makeUnitTexture(sf::Texture& texture, UnitKind kind, Team team)
    {
        const sf::Vector2u size = kind == UnitKind::Base
            ? sf::Vector2u(config::TileSize * 2, config::TileSize * 2)
            : sf::Vector2u(config::UnitTextureSize, config::UnitTextureSize);
        sf::RenderTexture canvas;
        if (!createCanvas(canvas, size)) {
            return;
        }
        canvas.clear(sf::Color::Transparent);
        const float scale = kind == UnitKind::Base ? 1.f : 1.50f;
        drawUnitIcon(canvas, kind, team, sf::Vector2f(size.x / 2.f, size.y / 2.f), scale);
        canvas.display();
        commitTexture(texture, canvas);
    }

    void makeButtonTexture(sf::Texture& texture, const sf::Font& font, const std::string& label,
                           ButtonState state, sf::Vector2u size, UnitKind icon, Team team)
    {
        sf::RenderTexture canvas;
        if (!createCanvas(canvas, size)) {
            return;
        }
        canvas.clear(sf::Color::Transparent);

        const bool pressed = state == ButtonState::Pressed;
        const bool hover = state == ButtonState::Hover;
        const float inset = pressed ? 4.f : 2.f;
        const sf::Color deep = sf::Color(39, 45, 39);
        const sf::Color leather = pressed ? sf::Color(104, 74, 48) : (hover ? sf::Color(143, 102, 61) : sf::Color(112, 82, 52));
        const sf::Color brass = hover ? sf::Color(255, 220, 116) : sf::Color(219, 166, 75);
        const sf::Color paper = pressed ? sf::Color(214, 177, 107) : sf::Color(240, 203, 126);

        drawPill(canvas, sf::Vector2f(inset + 3.f, inset + 7.f),
                 sf::Vector2f(static_cast<float>(size.x) - inset * 2.f - 6.f, static_cast<float>(size.y) - inset * 2.f - 9.f),
                 13.f, sf::Color(10, 13, 12, 92), sf::Color::Transparent, 0.f);
        drawPill(canvas, sf::Vector2f(inset, inset),
                 sf::Vector2f(static_cast<float>(size.x) - inset * 2.f, static_cast<float>(size.y) - inset * 2.f - 4.f),
                 13.f, leather, deep, 2.f);

        sf::RectangleShape face(sf::Vector2f(static_cast<float>(size.x) - inset * 2.f - 10.f, static_cast<float>(size.y) - inset * 2.f - 14.f));
        face.setPosition(inset + 5.f, inset + 5.f);
        face.setFillColor(paper);
        canvas.draw(face);

        sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(size.x) - inset * 2.f - 24.f, 4.f));
        topGlow.setPosition(inset + 12.f, inset + 10.f);
        topGlow.setFillColor(sf::Color(255, 255, 235, hover ? 105 : 58));
        canvas.draw(topGlow);

        sf::VertexArray slash(sf::Triangles, 3);
        slash[0].position = sf::Vector2f(static_cast<float>(size.x) - 40.f, inset + 5.f);
        slash[1].position = sf::Vector2f(static_cast<float>(size.x) - 5.f, inset + 5.f);
        slash[2].position = sf::Vector2f(static_cast<float>(size.x) - 5.f, static_cast<float>(size.y) - 12.f);
        slash[0].color = sf::Color(255, 248, 205, 45);
        slash[1].color = sf::Color(255, 248, 205, 16);
        slash[2].color = sf::Color(255, 248, 205, 0);
        canvas.draw(slash);

        float textLeft = 12.f;
        if (icon != UnitKind::None) {
            sf::CircleShape medallion(18.f, 36);
            medallion.setOrigin(18.f, 18.f);
            medallion.setPosition(28.f, static_cast<float>(size.y) / 2.f - 2.f + inset * 0.45f);
            medallion.setFillColor(sf::Color(45, 56, 48));
            medallion.setOutlineColor(brass);
            medallion.setOutlineThickness(2.f);
            canvas.draw(medallion);
            drawUnitIcon(canvas, icon, team, medallion.getPosition(), 0.82f);
            textLeft = 52.f;

            const std::string cost = costForIcon(icon);
            sf::CircleShape costChip(10.f, 24);
            costChip.setOrigin(10.f, 10.f);
            costChip.setPosition(static_cast<float>(size.x) - 17.f, 16.f + inset * 0.35f);
            costChip.setFillColor(sf::Color(62, 48, 28));
            costChip.setOutlineColor(sf::Color(255, 219, 93));
            costChip.setOutlineThickness(1.5f);
            canvas.draw(costChip);
            drawTextCentered(canvas, font, cost, 13, sf::Color(255, 232, 124),
                             sf::FloatRect(static_cast<float>(size.x) - 27.f, 6.f + inset * 0.35f, 20.f, 20.f));
        }

        const unsigned int textSize = size.y >= 64 ? 24 : (icon == UnitKind::None ? 15 : 14);
        drawTextCentered(canvas, font, label, textSize, sf::Color(39, 33, 25),
                         sf::FloatRect(textLeft, 8.f + inset * 0.4f, static_cast<float>(size.x) - textLeft - 14.f, static_cast<float>(size.y) - 18.f));

        if (pressed) {
            sf::RectangleShape veil(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
            veil.setFillColor(sf::Color(30, 18, 10, 38));
            canvas.draw(veil);
        }

        canvas.display();
        commitTexture(texture, canvas);
    }
}

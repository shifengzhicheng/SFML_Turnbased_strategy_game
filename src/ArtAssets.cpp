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
            return sf::Color(214, 54, 43);
        case Team::Enemy:
            return sf::Color(49, 105, 190);
        case Team::Neutral:
        default:
            return sf::Color(222, 179, 91);
        }
    }

    sf::Color teamAccent(Team team)
    {
        switch (team) {
        case Team::Player:
            return sf::Color(111, 27, 24);
        case Team::Enemy:
            return sf::Color(23, 49, 101);
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
        const float scale = static_cast<float>(size.x) / 64.f;
        drawUnitIcon(canvas, kind, team, sf::Vector2f(size.x / 2.f, size.y / 2.f), scale);
        canvas.display();
        if (team == Team::Enemy && kind != UnitKind::Base) {
            sf::Image mirrored = canvas.getTexture().copyToImage();
            mirrored.flipHorizontally();
            texture.loadFromImage(mirrored);
            texture.setSmooth(false);
            return;
        }
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
        const float shift = pressed ? 2.f : 0.f;
        const sf::Color ink(8, 10, 9);
        const sf::Color brass = hover ? sf::Color(255, 226, 120) : sf::Color(224, 170, 75);
        const sf::Color brassDark = pressed ? sf::Color(118, 78, 37) : sf::Color(151, 101, 44);
        const sf::Color face = pressed ? sf::Color(27, 33, 30) : (hover ? sf::Color(47, 61, 53) : sf::Color(34, 43, 39));
        const sf::Color faceTop = hover ? sf::Color(70, 88, 72) : sf::Color(48, 59, 51);
        const sf::Color textColor = hover ? sf::Color(255, 243, 187) : sf::Color(238, 231, 201);

        sf::RectangleShape shadow(sf::Vector2f(static_cast<float>(size.x) - 8.f, static_cast<float>(size.y) - 8.f));
        shadow.setPosition(6.f + shift, 8.f + shift);
        shadow.setFillColor(sf::Color(0, 0, 0, 104));
        canvas.draw(shadow);

        sf::RectangleShape outer(sf::Vector2f(static_cast<float>(size.x) - 6.f, static_cast<float>(size.y) - 6.f));
        outer.setPosition(3.f + shift, 2.f + shift);
        outer.setFillColor(ink);
        canvas.draw(outer);

        sf::RectangleShape rail(sf::Vector2f(outer.getSize().x - 4.f, outer.getSize().y - 4.f));
        rail.setPosition(outer.getPosition() + sf::Vector2f(2.f, 2.f));
        rail.setFillColor(brass);
        canvas.draw(rail);

        // Pixel-cut corners keep every button in the same hard-edged UI language
        // as the tiles and units without requiring external image assets.
        const sf::Vector2f o = outer.getPosition();
        const float ox = o.x;
        const float oy = o.y;
        const float ow = outer.getSize().x;
        const float oh = outer.getSize().y;
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + 2.f, oy + 2.f, 5.f, 2.f, ink);
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + ow - 7.f, oy + 2.f, 5.f, 2.f, ink);
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + 2.f, oy + oh - 4.f, 5.f, 2.f, ink);
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + ow - 7.f, oy + oh - 4.f, 5.f, 2.f, ink);
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + 7.f, oy + 4.f, ow - 14.f, 1.f, sf::Color(255, 247, 183, hover ? 145 : 74));
        drawPixelRect(canvas, sf::Vector2f(0.f, 0.f), 1.f, ox + 5.f, oy + oh - 5.f, ow - 10.f, 2.f, brassDark);

        sf::RectangleShape faceRect(sf::Vector2f(rail.getSize().x - 8.f, rail.getSize().y - 8.f));
        faceRect.setPosition(rail.getPosition() + sf::Vector2f(4.f, 4.f));
        faceRect.setFillColor(face);
        canvas.draw(faceRect);

        sf::RectangleShape topBand(sf::Vector2f(faceRect.getSize().x, 8.f));
        topBand.setPosition(faceRect.getPosition());
        topBand.setFillColor(faceTop);
        canvas.draw(topBand);

        sf::RectangleShape lowerRail(sf::Vector2f(faceRect.getSize().x, 3.f));
        lowerRail.setPosition(faceRect.getPosition() + sf::Vector2f(0.f, faceRect.getSize().y - 3.f));
        lowerRail.setFillColor(brassDark);
        canvas.draw(lowerRail);

        sf::RectangleShape highlight(sf::Vector2f(std::max(10.f, faceRect.getSize().x - 18.f), 2.f));
        highlight.setPosition(faceRect.getPosition() + sf::Vector2f(9.f, 5.f));
        highlight.setFillColor(sf::Color(255, 246, 191, hover ? 96 : 40));
        canvas.draw(highlight);

        sf::RectangleShape innerVignette(sf::Vector2f(faceRect.getSize().x, 5.f));
        innerVignette.setPosition(faceRect.getPosition() + sf::Vector2f(0.f, faceRect.getSize().y - 7.f));
        innerVignette.setFillColor(sf::Color(0, 0, 0, pressed ? 70 : 38));
        canvas.draw(innerVignette);

        float textLeft = 13.f;
        float textRightPad = 12.f;
        if (icon != UnitKind::None) {
            sf::RectangleShape iconWell(sf::Vector2f(38.f, 30.f));
            iconWell.setPosition(8.f + shift, static_cast<float>(size.y) / 2.f - 15.f + shift);
            iconWell.setFillColor(sf::Color(22, 28, 25));
            iconWell.setOutlineColor(brassDark);
            iconWell.setOutlineThickness(1.4f);
            canvas.draw(iconWell);

            sf::RectangleShape iconLight(sf::Vector2f(30.f, 2.f));
            iconLight.setPosition(iconWell.getPosition() + sf::Vector2f(4.f, 4.f));
            iconLight.setFillColor(sf::Color(255, 239, 178, 38));
            canvas.draw(iconLight);
            sf::RectangleShape iconFloor(sf::Vector2f(26.f, 3.f));
            iconFloor.setPosition(iconWell.getPosition() + sf::Vector2f(6.f, 23.f));
            iconFloor.setFillColor(sf::Color(0, 0, 0, 62));
            canvas.draw(iconFloor);
            drawUnitIcon(canvas, icon, team, iconWell.getPosition() + sf::Vector2f(19.f, 16.f), 0.52f);
            textLeft = 52.f;

            const std::string cost = costForIcon(icon);
            sf::RectangleShape costChip(sf::Vector2f(25.f, 19.f));
            costChip.setPosition(static_cast<float>(size.x) - 31.f + shift, static_cast<float>(size.y) / 2.f - 9.5f + shift);
            costChip.setFillColor(sf::Color(72, 48, 23));
            costChip.setOutlineColor(sf::Color(255, 219, 93));
            costChip.setOutlineThickness(1.3f);
            canvas.draw(costChip);
            drawTextCentered(canvas, font, cost, cost.size() > 2 ? 11 : 12, sf::Color(255, 233, 127),
                             sf::FloatRect(costChip.getPosition().x, costChip.getPosition().y - 1.f, costChip.getSize().x, costChip.getSize().y));
            textRightPad = 34.f;
        }

        const unsigned int textSize = size.y >= 64 ? 25 : (icon == UnitKind::None ? 15 : 14);
        sf::Text text(label, font, textSize);
        text.setFillColor(textColor);
        text.setOutlineColor(sf::Color(8, 10, 9, 130));
        text.setOutlineThickness(size.y >= 64 ? 0.8f : 0.5f);
        const auto bounds = text.getLocalBounds();
        const float boxWidth = static_cast<float>(size.x) - textLeft - textRightPad;
        text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        text.setPosition(textLeft + boxWidth * 0.5f + shift, static_cast<float>(size.y) * 0.52f + shift);
        canvas.draw(text);

        if (pressed) {
            sf::RectangleShape veil(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
            veil.setFillColor(sf::Color(15, 12, 8, 42));
            canvas.draw(veil);
        }

        canvas.display();
        commitTexture(texture, canvas);
    }
}

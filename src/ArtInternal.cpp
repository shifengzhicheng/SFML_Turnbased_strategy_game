#include "ArtAssets.h"
#include "Config.h"
#include "ArtInternal.h"

#include <algorithm>
#include <cmath>

namespace art_internal
{
    sf::Color mix(sf::Color a, sf::Color b, float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        const auto blend = [t](sf::Uint8 x, sf::Uint8 y) {
            return static_cast<sf::Uint8>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
        };
        return sf::Color(blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a));
    }

    void commitTexture(sf::Texture& texture, const sf::RenderTexture& renderTexture)
    {
        texture.loadFromImage(renderTexture.getTexture().copyToImage());
        texture.setSmooth(false);
    }

    bool createCanvas(sf::RenderTexture& canvas, sf::Vector2u size)
    {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 0;
        return canvas.create(size.x, size.y, settings);
    }

    void drawPill(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size,
                  float radius, sf::Color fill, sf::Color outline, float outlineThickness)
    {
        sf::RectangleShape center(sf::Vector2f(std::max(0.f, size.x - 2.f * radius), size.y));
        center.setPosition(pos.x + radius, pos.y);
        center.setFillColor(fill);
        target.draw(center);

        sf::RectangleShape middle(sf::Vector2f(size.x, std::max(0.f, size.y - 2.f * radius)));
        middle.setPosition(pos.x, pos.y + radius);
        middle.setFillColor(fill);
        target.draw(middle);

        sf::CircleShape corner(radius, 32);
        corner.setFillColor(fill);
        corner.setPosition(pos);
        target.draw(corner);
        corner.setPosition(pos.x + size.x - 2.f * radius, pos.y);
        target.draw(corner);
        corner.setPosition(pos.x, pos.y + size.y - 2.f * radius);
        target.draw(corner);
        corner.setPosition(pos.x + size.x - 2.f * radius, pos.y + size.y - 2.f * radius);
        target.draw(corner);

        if (outlineThickness > 0.f) {
            sf::RectangleShape top(sf::Vector2f(size.x - 2.f * radius, outlineThickness));
            top.setFillColor(outline);
            top.setPosition(pos.x + radius, pos.y);
            target.draw(top);

            sf::RectangleShape bottom(top);
            bottom.setPosition(pos.x + radius, pos.y + size.y - outlineThickness);
            target.draw(bottom);

            sf::RectangleShape left(sf::Vector2f(outlineThickness, size.y - 2.f * radius));
            left.setFillColor(outline);
            left.setPosition(pos.x, pos.y + radius);
            target.draw(left);

            sf::RectangleShape right(left);
            right.setPosition(pos.x + size.x - outlineThickness, pos.y + radius);
            target.draw(right);

            sf::CircleShape ring(radius, 32);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(outline);
            ring.setOutlineThickness(outlineThickness);
            ring.setPosition(pos);
            target.draw(ring);
            ring.setPosition(pos.x + size.x - 2.f * radius, pos.y);
            target.draw(ring);
            ring.setPosition(pos.x, pos.y + size.y - 2.f * radius);
            target.draw(ring);
            ring.setPosition(pos.x + size.x - 2.f * radius, pos.y + size.y - 2.f * radius);
            target.draw(ring);
        }
    }

    void drawSword(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color metal, sf::Color accent)
    {
        sf::RectangleShape blade(sf::Vector2f(2.f * scale, 13.f * scale));
        blade.setOrigin(scale, 12.f * scale);
        blade.setPosition(center.x + 1.f * scale, center.y + 3.f * scale);
        blade.setRotation(-35.f);
        blade.setFillColor(metal);
        target.draw(blade);

        sf::RectangleShape guard(sf::Vector2f(9.f * scale, 2.f * scale));
        guard.setOrigin(4.5f * scale, scale);
        guard.setPosition(center.x - 2.f * scale, center.y + 5.f * scale);
        guard.setRotation(-35.f);
        guard.setFillColor(accent);
        target.draw(guard);
    }

    void drawBow(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color accent, sf::Color stringColor)
    {
        sf::CircleShape arc(7.f * scale, 24);
        arc.setOrigin(7.f * scale, 7.f * scale);
        arc.setPosition(center.x - 2.f * scale, center.y);
        arc.setFillColor(sf::Color::Transparent);
        arc.setOutlineColor(accent);
        arc.setOutlineThickness(2.f * scale);
        target.draw(arc);

        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(center.x + 4.f * scale, center.y - 8.f * scale), stringColor),
            sf::Vertex(sf::Vector2f(center.x + 4.f * scale, center.y + 8.f * scale), stringColor)
        };
        target.draw(line, 2, sf::Lines);

        sf::RectangleShape arrow(sf::Vector2f(11.f * scale, 1.5f * scale));
        arrow.setOrigin(1.f * scale, 0.75f * scale);
        arrow.setPosition(center.x - 5.f * scale, center.y);
        arrow.setFillColor(stringColor);
        target.draw(arrow);
    }

    void drawBase(sf::RenderTarget& target, sf::Vector2f pos, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);

        sf::RectangleShape shadow(sf::Vector2f(32.f * scale, 6.f * scale));
        shadow.setOrigin(16.f * scale, 3.f * scale);
        shadow.setPosition(pos.x + 20.f * scale, pos.y + 35.f * scale);
        shadow.setFillColor(sf::Color(23, 30, 28, 70));
        target.draw(shadow);

        sf::RectangleShape keep(sf::Vector2f(24.f * scale, 23.f * scale));
        keep.setPosition(pos.x + 8.f * scale, pos.y + 11.f * scale);
        keep.setFillColor(mix(main, sf::Color::White, 0.18f));
        keep.setOutlineColor(accent);
        keep.setOutlineThickness(2.f * scale);
        target.draw(keep);

        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape crenel(sf::Vector2f(6.f * scale, 6.f * scale));
            crenel.setPosition(pos.x + (8.f + i * 9.f) * scale, pos.y + 6.f * scale);
            crenel.setFillColor(accent);
            target.draw(crenel);
        }

        sf::ConvexShape roof(3);
        roof.setPoint(0, sf::Vector2f(pos.x + 6.f * scale, pos.y + 12.f * scale));
        roof.setPoint(1, sf::Vector2f(pos.x + 20.f * scale, pos.y + 1.f * scale));
        roof.setPoint(2, sf::Vector2f(pos.x + 34.f * scale, pos.y + 12.f * scale));
        roof.setFillColor(mix(accent, sf::Color::Black, 0.12f));
        target.draw(roof);

        sf::RectangleShape gate(sf::Vector2f(8.f * scale, 12.f * scale));
        gate.setPosition(pos.x + 16.f * scale, pos.y + 22.f * scale);
        gate.setFillColor(sf::Color(36, 34, 31));
        target.draw(gate);
    }

    void drawPixelRect(sf::RenderTarget& target, sf::Vector2f origin, float scale,
                       float x, float y, float w, float h, sf::Color color)
    {
        sf::RectangleShape rect(sf::Vector2f(std::max(1.f, std::round(w * scale)),
                                             std::max(1.f, std::round(h * scale))));
        rect.setPosition(std::round(origin.x + x * scale), std::round(origin.y + y * scale));
        rect.setFillColor(color);
        target.draw(rect);
    }

    void drawPixelOutline(sf::RenderTarget& target, sf::Vector2f origin, float scale,
                          float x, float y, float w, float h, sf::Color fill, sf::Color outline)
    {
        drawPixelRect(target, origin, scale, x - 1.f, y - 1.f, w + 2.f, h + 2.f, outline);
        drawPixelRect(target, origin, scale, x, y, w, h, fill);
    }

    void drawPixelShadow(sf::RenderTarget& target, sf::Vector2f origin, float scale,
                         float x, float y, float w, sf::Color color)
    {
        drawPixelRect(target, origin, scale, x + 4.f, y - 2.f, w - 8.f, 2.f, sf::Color(color.r, color.g, color.b, 42));
        drawPixelRect(target, origin, scale, x, y, w, 3.f, color);
        drawPixelRect(target, origin, scale, x + 5.f, y + 3.f, w - 10.f, 2.f, sf::Color(color.r, color.g, color.b, 52));
    }

    void drawPixelBaseIcon(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto wall = sf::Color(190, 176, 132);
        const auto wallDark = sf::Color(79, 68, 50);
        const auto roof = mix(accent, sf::Color(188, 132, 58), 0.35f);

        drawPixelShadow(target, origin, scale, 10.f, 54.f, 44.f, sf::Color(10, 15, 13, 95));
        drawPixelOutline(target, origin, scale, 17.f, 30.f, 31.f, 23.f, wall, wallDark);
        drawPixelRect(target, origin, scale, 20.f, 34.f, 25.f, 4.f, mix(wall, sf::Color::White, 0.22f));
        drawPixelRect(target, origin, scale, 23.f, 41.f, 5.f, 12.f, sf::Color(57, 45, 35));
        drawPixelRect(target, origin, scale, 36.f, 41.f, 5.f, 12.f, sf::Color(57, 45, 35));
        drawPixelRect(target, origin, scale, 15.f, 24.f, 35.f, 7.f, roof);
        drawPixelRect(target, origin, scale, 20.f, 17.f, 25.f, 7.f, roof);
        drawPixelRect(target, origin, scale, 25.f, 11.f, 15.f, 6.f, roof);
        drawPixelRect(target, origin, scale, 18.f, 26.f, 6.f, 4.f, main);
        drawPixelRect(target, origin, scale, 30.f, 19.f, 5.f, 4.f, main);
        drawPixelRect(target, origin, scale, 42.f, 26.f, 6.f, 4.f, main);
        drawPixelRect(target, origin, scale, 29.f, 42.f, 6.f, 11.f, sf::Color(37, 30, 24));
        drawPixelRect(target, origin, scale, 30.f, 43.f, 4.f, 2.f, sf::Color(255, 219, 104));
    }

    void drawInfantryPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(37, 31, 27);
        const auto skin = sf::Color(239, 196, 143);
        const auto metal = sf::Color(206, 213, 205);
        const auto gold = sf::Color(235, 186, 71);

        drawPixelShadow(target, origin, scale, 20.f, 57.f, 26.f, sf::Color(9, 15, 13, 96));
        drawPixelRect(target, origin, scale, 27.f, 47.f, 4.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 36.f, 47.f, 4.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 27.f, 53.f, 7.f, 3.f, sf::Color(58, 43, 34));
        drawPixelRect(target, origin, scale, 34.f, 53.f, 7.f, 3.f, sf::Color(58, 43, 34));
        drawPixelOutline(target, origin, scale, 25.f, 32.f, 16.f, 17.f, main, outline);
        drawPixelRect(target, origin, scale, 28.f, 35.f, 10.f, 4.f, mix(main, sf::Color::White, 0.35f));
        drawPixelRect(target, origin, scale, 31.f, 40.f, 4.f, 9.f, accent);
        drawPixelOutline(target, origin, scale, 27.f, 23.f, 11.f, 9.f, skin, outline);
        drawPixelRect(target, origin, scale, 25.f, 19.f, 15.f, 5.f, metal);
        drawPixelRect(target, origin, scale, 28.f, 16.f, 9.f, 4.f, metal);
        drawPixelRect(target, origin, scale, 23.f, 34.f, 10.f, 16.f, gold);
        drawPixelRect(target, origin, scale, 25.f, 37.f, 6.f, 10.f, main);
        drawPixelRect(target, origin, scale, 43.f, 18.f, 6.f, 34.f, outline);
        drawPixelRect(target, origin, scale, 45.f, 16.f, 2.f, 4.f, sf::Color(255, 248, 214));
        drawPixelRect(target, origin, scale, 45.f, 20.f, 2.f, 27.f, sf::Color(236, 239, 218));
        drawPixelRect(target, origin, scale, 47.f, 23.f, 1.f, 21.f, sf::Color(154, 163, 162));
        drawPixelRect(target, origin, scale, 40.f, 25.f, 11.f, 4.f, gold);
        drawPixelRect(target, origin, scale, 42.f, 47.f, 7.f, 3.f, sf::Color(89, 58, 34));
        drawPixelRect(target, origin, scale, 29.f, 27.f, 2.f, 2.f, outline);
        drawPixelRect(target, origin, scale, 35.f, 27.f, 2.f, 2.f, outline);
    }

    void drawShooterPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(36, 28, 25);
        const auto skin = sf::Color(231, 185, 133);
        const auto leather = sf::Color(94, 61, 38);
        const auto bow = sf::Color(204, 145, 65);

        drawPixelShadow(target, origin, scale, 21.f, 57.f, 25.f, sf::Color(9, 15, 13, 92));
        drawPixelRect(target, origin, scale, 27.f, 47.f, 4.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 35.f, 47.f, 4.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 24.f, 32.f, 18.f, 18.f, mix(main, sf::Color::Black, 0.18f));
        drawPixelRect(target, origin, scale, 22.f, 38.f, 5.f, 13.f, accent);
        drawPixelRect(target, origin, scale, 39.f, 37.f, 5.f, 13.f, accent);
        drawPixelRect(target, origin, scale, 25.f, 24.f, 16.f, 10.f, mix(main, sf::Color::Black, 0.10f));
        drawPixelRect(target, origin, scale, 28.f, 20.f, 10.f, 5.f, mix(main, sf::Color::Black, 0.05f));
        drawPixelRect(target, origin, scale, 29.f, 27.f, 8.f, 6.f, skin);
        drawPixelRect(target, origin, scale, 31.f, 30.f, 2.f, 2.f, outline);
        drawPixelRect(target, origin, scale, 43.f, 21.f, 5.f, 34.f, outline);
        drawPixelRect(target, origin, scale, 45.f, 23.f, 3.f, 30.f, bow);
        drawPixelRect(target, origin, scale, 47.f, 27.f, 1.f, 23.f, sf::Color(246, 236, 191));
        drawPixelRect(target, origin, scale, 34.f, 35.f, 20.f, 3.f, sf::Color(246, 236, 191));
        drawPixelRect(target, origin, scale, 52.f, 33.f, 6.f, 7.f, sf::Color(246, 236, 191));
        drawPixelRect(target, origin, scale, 53.f, 35.f, 8.f, 2.f, sf::Color(255, 249, 211));
        drawPixelRect(target, origin, scale, 18.f, 34.f, 5.f, 16.f, leather);
        drawPixelRect(target, origin, scale, 17.f, 31.f, 7.f, 3.f, sf::Color(232, 220, 165));
        drawPixelRect(target, origin, scale, 30.f, 28.f, 2.f, 2.f, outline);
    }

    void drawCavalryPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(34, 28, 24);
        const auto horse = team == art::Team::Enemy ? sf::Color(83, 104, 140) : sf::Color(126, 76, 42);
        const auto horseLight = mix(horse, sf::Color::White, 0.18f);
        const auto metal = sf::Color(218, 222, 210);

        drawPixelShadow(target, origin, scale, 12.f, 57.f, 41.f, sf::Color(9, 15, 13, 105));
        drawPixelRect(target, origin, scale, 17.f, 47.f, 5.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 27.f, 47.f, 4.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 41.f, 46.f, 5.f, 11.f, outline);
        drawPixelOutline(target, origin, scale, 15.f, 37.f, 31.f, 12.f, horse, outline);
        drawPixelRect(target, origin, scale, 20.f, 35.f, 18.f, 4.f, horseLight);
        drawPixelOutline(target, origin, scale, 43.f, 31.f, 10.f, 10.f, horse, outline);
        drawPixelRect(target, origin, scale, 51.f, 35.f, 5.f, 3.f, outline);
        drawPixelRect(target, origin, scale, 14.f, 35.f, 5.f, 7.f, sf::Color(48, 36, 28));
        drawPixelOutline(target, origin, scale, 28.f, 25.f, 11.f, 13.f, main, outline);
        drawPixelRect(target, origin, scale, 30.f, 20.f, 8.f, 7.f, metal);
        drawPixelRect(target, origin, scale, 33.f, 16.f, 4.f, 4.f, accent);
        drawPixelRect(target, origin, scale, 36.f, 19.f, 26.f, 6.f, outline);
        drawPixelRect(target, origin, scale, 38.f, 20.f, 22.f, 3.f, sf::Color(239, 229, 187));
        drawPixelRect(target, origin, scale, 58.f, 17.f, 4.f, 8.f, sf::Color(239, 229, 187));
        drawPixelRect(target, origin, scale, 60.f, 19.f, 3.f, 4.f, sf::Color(255, 246, 205));
        drawPixelRect(target, origin, scale, 24.f, 39.f, 13.f, 3.f, sf::Color(227, 178, 75));
        drawPixelRect(target, origin, scale, 47.f, 34.f, 2.f, 2.f, outline);
    }

    void drawSiegePixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(31, 29, 26);
        const auto wood = sf::Color(104, 75, 47);
        const auto woodLight = sf::Color(146, 103, 60);
        const auto metal = sf::Color(181, 184, 170);
        const auto fire = sf::Color(255, 188, 76);

        drawPixelShadow(target, origin, scale, 9.f, 57.f, 48.f, sf::Color(9, 15, 13, 112));
        drawPixelOutline(target, origin, scale, 17.f, 40.f, 32.f, 12.f, wood, outline);
        drawPixelRect(target, origin, scale, 21.f, 43.f, 24.f, 3.f, woodLight);
        drawPixelRect(target, origin, scale, 19.f, 32.f, 35.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 21.f, 34.f, 29.f, 6.f, sf::Color(82, 72, 60));
        drawPixelRect(target, origin, scale, 44.f, 27.f, 19.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 46.f, 29.f, 14.f, 5.f, metal);
        drawPixelRect(target, origin, scale, 57.f, 27.f, 5.f, 9.f, sf::Color(70, 70, 66));
        drawPixelRect(target, origin, scale, 12.f, 51.f, 11.f, 8.f, outline);
        drawPixelRect(target, origin, scale, 16.f, 53.f, 3.f, 3.f, sf::Color(220, 176, 76));
        drawPixelRect(target, origin, scale, 42.f, 50.f, 11.f, 8.f, outline);
        drawPixelRect(target, origin, scale, 46.f, 52.f, 3.f, 3.f, sf::Color(220, 176, 76));
        drawPixelRect(target, origin, scale, 24.f, 31.f, 11.f, 5.f, accent);
        drawPixelRect(target, origin, scale, 59.f, 24.f, 4.f, 5.f, fire);
        drawPixelRect(target, origin, scale, 61.f, 22.f, 2.f, 2.f, sf::Color(255, 230, 131));
        drawPixelRect(target, origin, scale, 53.f, 30.f, 5.f, 2.f, sf::Color(231, 228, 190));
    }

    void drawGuardianPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(28, 31, 29);
        const auto armor = mix(main, sf::Color(91, 102, 96), 0.62f);
        const auto armorLight = mix(armor, sf::Color::White, 0.25f);
        const auto gold = sf::Color(237, 190, 76);

        drawPixelShadow(target, origin, scale, 9.f, 57.f, 49.f, sf::Color(9, 15, 13, 120));
        drawPixelRect(target, origin, scale, 24.f, 48.f, 6.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 36.f, 48.f, 6.f, 9.f, outline);
        drawPixelOutline(target, origin, scale, 21.f, 26.f, 24.f, 25.f, armor, outline);
        drawPixelRect(target, origin, scale, 16.f, 31.f, 9.f, 18.f, armorLight);
        drawPixelRect(target, origin, scale, 42.f, 31.f, 9.f, 18.f, armorLight);
        drawPixelRect(target, origin, scale, 26.f, 22.f, 14.f, 6.f, armorLight);
        drawPixelRect(target, origin, scale, 29.f, 15.f, 8.f, 8.f, gold);
        drawPixelRect(target, origin, scale, 28.f, 33.f, 11.f, 12.f, accent);
        drawPixelRect(target, origin, scale, 31.f, 36.f, 5.f, 6.f, sf::Color(255, 230, 119));
        drawPixelRect(target, origin, scale, 48.f, 17.f, 7.f, 35.f, outline);
        drawPixelRect(target, origin, scale, 50.f, 18.f, 3.f, 31.f, sf::Color(232, 226, 196));
        drawPixelRect(target, origin, scale, 45.f, 25.f, 13.f, 4.f, gold);
        drawPixelRect(target, origin, scale, 49.f, 13.f, 5.f, 7.f, sf::Color(255, 246, 204));
        drawPixelRect(target, origin, scale, 14.f, 34.f, 9.f, 15.f, outline);
        drawPixelRect(target, origin, scale, 16.f, 36.f, 5.f, 11.f, gold);
    }

    void drawUnitIcon(sf::RenderTarget& target, art::UnitKind kind, art::Team team, sf::Vector2f center, float scale)
    {
        const sf::Vector2f origin(std::round(center.x - 32.f * scale), std::round(center.y - 32.f * scale));

        switch (kind) {
        case art::UnitKind::Base:
            drawPixelBaseIcon(target, origin, scale, team);
            break;
        case art::UnitKind::Infantry:
            drawInfantryPixel(target, origin, scale, team);
            break;
        case art::UnitKind::Shooter:
            drawShooterPixel(target, origin, scale, team);
            break;
        case art::UnitKind::Cavalry:
            drawCavalryPixel(target, origin, scale, team);
            break;
        case art::UnitKind::Siege:
            drawSiegePixel(target, origin, scale, team);
            break;
        case art::UnitKind::Guardian:
            drawGuardianPixel(target, origin, scale, team);
            break;
        default:
            break;
        }
    }

    std::string costForIcon(art::UnitKind icon)
    {
        switch (icon) {
        case art::UnitKind::Infantry:
            return std::to_string(config::InfantryCost);
        case art::UnitKind::Shooter:
            return std::to_string(config::ShooterCost);
        case art::UnitKind::Cavalry:
            return std::to_string(config::CavalryCost);
        case art::UnitKind::Siege:
            return std::to_string(config::SiegeCost);
        case art::UnitKind::Guardian:
            return std::to_string(config::GuardianCost);
        default:
            return "";
        }
    }

    void drawTextCentered(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                          unsigned int size, sf::Color color, sf::FloatRect box)
    {
        sf::Text text(label, font, size);
        text.setFillColor(color);
        const auto bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(box.left + box.width / 2.f, box.top + box.height / 2.f - 1.f);
        target.draw(text);
    }
}

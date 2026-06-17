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

    void drawPixelHilt(sf::RenderTarget& target, sf::Vector2f origin, float scale,
                       float x, float y, sf::Color metal, sf::Color gold, sf::Color outline)
    {
        drawPixelRect(target, origin, scale, x, y, 3.f, 30.f, outline);
        drawPixelRect(target, origin, scale, x + 1.f, y + 1.f, 1.f, 26.f, metal);
        drawPixelRect(target, origin, scale, x - 4.f, y + 10.f, 11.f, 4.f, gold);
    }

    void drawPixelTeamTab(sf::RenderTarget& target, sf::Vector2f origin, float scale,
                          float x, float y, art::Team team, sf::Color outline)
    {
        const auto main = art::teamColor(team);
        const auto light = mix(main, sf::Color::White, 0.28f);
        drawPixelRect(target, origin, scale, x - 1.f, y - 1.f, 10.f, 8.f, outline);
        drawPixelRect(target, origin, scale, x, y, 8.f, 6.f, main);
        drawPixelRect(target, origin, scale, x + 1.f, y + 1.f, 5.f, 2.f, light);
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
        const auto outline = sf::Color(31, 27, 23);
        const auto skin = sf::Color(238, 190, 134);
        const auto metal = sf::Color(219, 224, 213);
        const auto metalDark = sf::Color(121, 132, 130);
        const auto gold = sf::Color(235, 181, 67);
        const auto boot = sf::Color(58, 43, 33);

        drawPixelShadow(target, origin, scale, 17.f, 58.f, 31.f, sf::Color(8, 13, 11, 118));
        drawPixelRect(target, origin, scale, 27.f, 47.f, 5.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 36.f, 47.f, 5.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 26.f, 54.f, 9.f, 3.f, boot);
        drawPixelRect(target, origin, scale, 35.f, 54.f, 9.f, 3.f, boot);

        drawPixelOutline(target, origin, scale, 25.f, 31.f, 18.f, 18.f, main, outline);
        drawPixelRect(target, origin, scale, 28.f, 34.f, 12.f, 4.f, mix(main, sf::Color::White, 0.34f));
        drawPixelRect(target, origin, scale, 32.f, 39.f, 5.f, 10.f, accent);
        drawPixelRect(target, origin, scale, 25.f, 32.f, 5.f, 17.f, mix(main, sf::Color::Black, 0.16f));

        drawPixelOutline(target, origin, scale, 27.f, 22.f, 12.f, 10.f, skin, outline);
        drawPixelRect(target, origin, scale, 29.f, 26.f, 2.f, 2.f, outline);
        drawPixelRect(target, origin, scale, 36.f, 26.f, 2.f, 2.f, outline);
        drawPixelRect(target, origin, scale, 29.f, 19.f, 12.f, 4.f, metal);
        drawPixelRect(target, origin, scale, 26.f, 21.f, 17.f, 5.f, metalDark);
        drawPixelRect(target, origin, scale, 30.f, 16.f, 8.f, 4.f, metal);
        drawPixelTeamTab(target, origin, scale, 23.f, 34.f, team, outline);

        drawPixelOutline(target, origin, scale, 17.f, 33.f, 12.f, 18.f, gold, outline);
        drawPixelRect(target, origin, scale, 20.f, 36.f, 7.f, 11.f, main);
        drawPixelRect(target, origin, scale, 22.f, 38.f, 3.f, 7.f, mix(main, sf::Color::White, 0.22f));
        drawPixelRect(target, origin, scale, 18.f, 32.f, 10.f, 3.f, sf::Color(255, 226, 105));

        drawPixelHilt(target, origin, scale, 48.f, 16.f, metal, gold, outline);
        drawPixelRect(target, origin, scale, 47.f, 14.f, 3.f, 4.f, sf::Color(255, 250, 219));
        drawPixelRect(target, origin, scale, 49.f, 18.f, 2.f, 27.f, sf::Color(245, 248, 226));
        drawPixelRect(target, origin, scale, 51.f, 21.f, 1.f, 22.f, metalDark);
        drawPixelRect(target, origin, scale, 40.f, 47.f, 10.f, 3.f, boot);
    }

    void drawShooterPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(31, 25, 22);
        const auto skin = sf::Color(229, 180, 126);
        const auto leather = sf::Color(94, 60, 37);
        const auto bow = sf::Color(213, 146, 59);
        const auto stringColor = sf::Color(248, 239, 201);

        drawPixelShadow(target, origin, scale, 18.f, 58.f, 30.f, sf::Color(8, 13, 11, 112));
        drawPixelRect(target, origin, scale, 26.f, 48.f, 5.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 35.f, 48.f, 5.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 25.f, 54.f, 9.f, 3.f, sf::Color(58, 43, 34));
        drawPixelRect(target, origin, scale, 34.f, 54.f, 9.f, 3.f, sf::Color(58, 43, 34));

        drawPixelOutline(target, origin, scale, 23.f, 31.f, 20.f, 18.f, mix(main, sf::Color::Black, 0.14f), outline);
        drawPixelRect(target, origin, scale, 26.f, 34.f, 14.f, 4.f, mix(main, sf::Color::White, 0.23f));
        drawPixelRect(target, origin, scale, 30.f, 39.f, 6.f, 10.f, accent);
        drawPixelOutline(target, origin, scale, 25.f, 22.f, 17.f, 12.f, mix(main, sf::Color::Black, 0.06f), outline);
        drawPixelRect(target, origin, scale, 28.f, 19.f, 11.f, 5.f, mix(main, sf::Color::White, 0.16f));
        drawPixelRect(target, origin, scale, 30.f, 26.f, 8.f, 6.f, skin);
        drawPixelRect(target, origin, scale, 31.f, 28.f, 2.f, 2.f, outline);
        drawPixelTeamTab(target, origin, scale, 17.f, 35.f, team, outline);

        drawPixelRect(target, origin, scale, 18.f, 35.f, 6.f, 15.f, leather);
        drawPixelRect(target, origin, scale, 17.f, 32.f, 8.f, 3.f, sf::Color(236, 220, 162));
        drawPixelRect(target, origin, scale, 36.f, 36.f, 19.f, 3.f, stringColor);
        drawPixelRect(target, origin, scale, 52.f, 33.f, 7.f, 8.f, stringColor);
        drawPixelRect(target, origin, scale, 55.f, 36.f, 6.f, 2.f, sf::Color(255, 250, 216));

        drawPixelRect(target, origin, scale, 47.f, 17.f, 4.f, 40.f, outline);
        drawPixelRect(target, origin, scale, 49.f, 19.f, 3.f, 36.f, bow);
        drawPixelRect(target, origin, scale, 51.f, 22.f, 2.f, 31.f, mix(bow, sf::Color::White, 0.24f));
        drawPixelRect(target, origin, scale, 53.f, 24.f, 1.f, 27.f, stringColor);
        drawPixelRect(target, origin, scale, 43.f, 20.f, 7.f, 4.f, bow);
        drawPixelRect(target, origin, scale, 43.f, 50.f, 7.f, 4.f, bow);
    }

    void drawCavalryPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(29, 24, 21);
        const auto horse = team == art::Team::Enemy ? sf::Color(76, 98, 141) : sf::Color(129, 75, 39);
        const auto horseLight = mix(horse, sf::Color::White, 0.20f);
        const auto horseDark = mix(horse, sf::Color::Black, 0.32f);
        const auto metal = sf::Color(222, 226, 214);
        const auto gold = sf::Color(232, 177, 66);

        drawPixelShadow(target, origin, scale, 10.f, 58.f, 47.f, sf::Color(8, 13, 11, 126));
        drawPixelRect(target, origin, scale, 17.f, 47.f, 5.f, 11.f, outline);
        drawPixelRect(target, origin, scale, 28.f, 47.f, 5.f, 11.f, outline);
        drawPixelRect(target, origin, scale, 42.f, 46.f, 5.f, 12.f, outline);
        drawPixelRect(target, origin, scale, 16.f, 54.f, 9.f, 3.f, horseDark);
        drawPixelRect(target, origin, scale, 27.f, 54.f, 9.f, 3.f, horseDark);
        drawPixelRect(target, origin, scale, 41.f, 54.f, 9.f, 3.f, horseDark);

        drawPixelOutline(target, origin, scale, 14.f, 37.f, 33.f, 12.f, horse, outline);
        drawPixelRect(target, origin, scale, 18.f, 35.f, 22.f, 4.f, horseLight);
        drawPixelRect(target, origin, scale, 14.f, 34.f, 7.f, 8.f, horseDark);
        drawPixelOutline(target, origin, scale, 43.f, 30.f, 11.f, 11.f, horse, outline);
        drawPixelRect(target, origin, scale, 51.f, 34.f, 6.f, 3.f, outline);
        drawPixelRect(target, origin, scale, 47.f, 33.f, 2.f, 2.f, outline);
        drawPixelRect(target, origin, scale, 22.f, 40.f, 16.f, 3.f, gold);

        drawPixelOutline(target, origin, scale, 28.f, 24.f, 12.f, 14.f, main, outline);
        drawPixelRect(target, origin, scale, 30.f, 27.f, 8.f, 4.f, mix(main, sf::Color::White, 0.26f));
        drawPixelRect(target, origin, scale, 31.f, 18.f, 9.f, 7.f, metal);
        drawPixelRect(target, origin, scale, 34.f, 15.f, 4.f, 4.f, accent);
        drawPixelTeamTab(target, origin, scale, 25.f, 28.f, team, outline);

        drawPixelRect(target, origin, scale, 38.f, 18.f, 25.f, 5.f, outline);
        drawPixelRect(target, origin, scale, 40.f, 19.f, 21.f, 2.f, sf::Color(248, 237, 190));
        drawPixelRect(target, origin, scale, 60.f, 16.f, 4.f, 8.f, sf::Color(255, 250, 214));
        drawPixelRect(target, origin, scale, 61.f, 18.f, 3.f, 4.f, sf::Color(255, 255, 236));
        drawPixelRect(target, origin, scale, 39.f, 23.f, 8.f, 3.f, gold);
    }

    void drawSiegePixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(27, 25, 22);
        const auto wood = sf::Color(103, 73, 45);
        const auto woodLight = sf::Color(150, 104, 58);
        const auto metal = sf::Color(184, 188, 175);
        const auto steelLight = sf::Color(225, 224, 197);
        const auto fire = sf::Color(255, 185, 70);

        drawPixelShadow(target, origin, scale, 8.f, 58.f, 50.f, sf::Color(8, 13, 11, 132));
        drawPixelOutline(target, origin, scale, 16.f, 41.f, 34.f, 11.f, wood, outline);
        drawPixelRect(target, origin, scale, 20.f, 43.f, 26.f, 3.f, woodLight);
        drawPixelRect(target, origin, scale, 19.f, 50.f, 7.f, 3.f, mix(wood, sf::Color::Black, 0.18f));
        drawPixelRect(target, origin, scale, 40.f, 50.f, 7.f, 3.f, mix(wood, sf::Color::Black, 0.18f));

        drawPixelRect(target, origin, scale, 18.f, 31.f, 38.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 21.f, 33.f, 30.f, 6.f, sf::Color(79, 70, 59));
        drawPixelRect(target, origin, scale, 23.f, 34.f, 22.f, 2.f, woodLight);
        drawPixelRect(target, origin, scale, 45.f, 26.f, 19.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 47.f, 28.f, 15.f, 5.f, metal);
        drawPixelRect(target, origin, scale, 54.f, 29.f, 7.f, 2.f, steelLight);
        drawPixelRect(target, origin, scale, 62.f, 26.f, 4.f, 10.f, sf::Color(66, 66, 62));

        drawPixelRect(target, origin, scale, 12.f, 50.f, 12.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 15.f, 52.f, 5.f, 5.f, sf::Color(225, 176, 74));
        drawPixelRect(target, origin, scale, 40.f, 50.f, 13.f, 9.f, outline);
        drawPixelRect(target, origin, scale, 44.f, 52.f, 5.f, 5.f, sf::Color(225, 176, 74));
        drawPixelRect(target, origin, scale, 23.f, 29.f, 14.f, 5.f, accent);
        drawPixelTeamTab(target, origin, scale, 25.f, 24.f, team, outline);
        drawPixelRect(target, origin, scale, 58.f, 22.f, 5.f, 5.f, fire);
        drawPixelRect(target, origin, scale, 61.f, 20.f, 2.f, 2.f, sf::Color(255, 232, 132));
        drawPixelRect(target, origin, scale, 30.f, 38.f, 15.f, 3.f, main);
    }

    void drawGuardianPixel(sf::RenderTarget& target, sf::Vector2f origin, float scale, art::Team team)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto outline = sf::Color(25, 28, 26);
        const auto armor = mix(main, sf::Color(92, 104, 98), 0.64f);
        const auto armorLight = mix(armor, sf::Color::White, 0.28f);
        const auto armorDark = mix(armor, sf::Color::Black, 0.25f);
        const auto gold = sf::Color(238, 188, 73);
        const auto metal = sf::Color(232, 228, 198);

        drawPixelShadow(target, origin, scale, 8.f, 58.f, 50.f, sf::Color(8, 13, 11, 136));
        drawPixelRect(target, origin, scale, 22.f, 48.f, 7.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 37.f, 48.f, 7.f, 10.f, outline);
        drawPixelRect(target, origin, scale, 20.f, 54.f, 12.f, 4.f, armorDark);
        drawPixelRect(target, origin, scale, 35.f, 54.f, 12.f, 4.f, armorDark);

        drawPixelOutline(target, origin, scale, 21.f, 25.f, 25.f, 26.f, armor, outline);
        drawPixelRect(target, origin, scale, 25.f, 29.f, 17.f, 5.f, armorLight);
        drawPixelRect(target, origin, scale, 29.f, 35.f, 9.f, 10.f, accent);
        drawPixelRect(target, origin, scale, 32.f, 37.f, 4.f, 6.f, sf::Color(255, 230, 119));
        drawPixelRect(target, origin, scale, 25.f, 21.f, 17.f, 6.f, armorLight);
        drawPixelRect(target, origin, scale, 29.f, 14.f, 9.f, 8.f, gold);
        drawPixelTeamTab(target, origin, scale, 24.f, 31.f, team, outline);

        drawPixelOutline(target, origin, scale, 13.f, 33.f, 13.f, 18.f, gold, outline);
        drawPixelRect(target, origin, scale, 16.f, 36.f, 7.f, 11.f, main);
        drawPixelRect(target, origin, scale, 18.f, 38.f, 3.f, 7.f, mix(main, sf::Color::White, 0.20f));

        drawPixelHilt(target, origin, scale, 51.f, 15.f, metal, gold, outline);
        drawPixelRect(target, origin, scale, 50.f, 12.f, 5.f, 7.f, sf::Color(255, 248, 206));
        drawPixelRect(target, origin, scale, 52.f, 20.f, 2.f, 28.f, metal);
        drawPixelRect(target, origin, scale, 54.f, 23.f, 1.f, 21.f, sf::Color(151, 151, 141));
        drawPixelRect(target, origin, scale, 45.f, 25.f, 13.f, 4.f, gold);
        drawPixelRect(target, origin, scale, 43.f, 31.f, 8.f, 18.f, armorLight);
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

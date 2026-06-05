#include "ArtAssets.h"
#include "Config.h"

#include <algorithm>
#include <cmath>

namespace
{
    sf::Color withAlpha(sf::Color color, sf::Uint8 alpha)
    {
        color.a = alpha;
        return color;
    }

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
        texture.setSmooth(true);
    }

    bool createCanvas(sf::RenderTexture& canvas, sf::Vector2u size)
    {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 8;
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

    void drawDiamond(sf::RenderTarget& target, sf::Vector2f center, float radius, sf::Color fill, sf::Color outline)
    {
        sf::ConvexShape diamond(4);
        diamond.setPoint(0, sf::Vector2f(center.x, center.y - radius));
        diamond.setPoint(1, sf::Vector2f(center.x + radius, center.y));
        diamond.setPoint(2, sf::Vector2f(center.x, center.y + radius));
        diamond.setPoint(3, sf::Vector2f(center.x - radius, center.y));
        diamond.setFillColor(fill);
        diamond.setOutlineColor(outline);
        diamond.setOutlineThickness(std::max(1.f, radius * 0.12f));
        target.draw(diamond);
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

    void drawHorse(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color body, sf::Color accent)
    {
        sf::CircleShape bodyShape(6.f * scale, 24);
        bodyShape.setOrigin(6.f * scale, 6.f * scale);
        bodyShape.setScale(1.35f, 0.82f);
        bodyShape.setPosition(center.x, center.y + 2.f * scale);
        bodyShape.setFillColor(body);
        bodyShape.setOutlineColor(accent);
        bodyShape.setOutlineThickness(1.2f * scale);
        target.draw(bodyShape);

        sf::CircleShape head(3.7f * scale, 18);
        head.setOrigin(3.7f * scale, 3.7f * scale);
        head.setPosition(center.x + 7.f * scale, center.y - 2.f * scale);
        head.setFillColor(body);
        head.setOutlineColor(accent);
        head.setOutlineThickness(scale);
        target.draw(head);

        for (float dx : {-4.f, 3.f}) {
            sf::RectangleShape leg(sf::Vector2f(1.8f * scale, 7.f * scale));
            leg.setOrigin(0.9f * scale, 0.f);
            leg.setPosition(center.x + dx * scale, center.y + 6.f * scale);
            leg.setFillColor(accent);
            target.draw(leg);
        }
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

    void drawUnitIcon(sf::RenderTarget& target, art::UnitKind kind, art::Team team, sf::Vector2f center, float scale)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto pale = mix(main, sf::Color::White, 0.42f);

        sf::CircleShape shadow(8.f * scale, 28);
        shadow.setOrigin(8.f * scale, 4.f * scale);
        shadow.setScale(1.25f, 0.42f);
        shadow.setPosition(center.x, center.y + 9.f * scale);
        shadow.setFillColor(sf::Color(13, 23, 20, 85));
        target.draw(shadow);

        if (kind == art::UnitKind::Base) {
            drawBase(target, sf::Vector2f(center.x - 20.f * scale, center.y - 20.f * scale), scale, team);
            return;
        }

        drawDiamond(target, center, 8.f * scale, pale, accent);

        sf::CircleShape head(3.5f * scale, 18);
        head.setOrigin(3.5f * scale, 3.5f * scale);
        head.setPosition(center.x, center.y - 4.f * scale);
        head.setFillColor(sf::Color(245, 220, 170));
        head.setOutlineColor(sf::Color(68, 49, 40));
        head.setOutlineThickness(0.7f * scale);
        target.draw(head);

        switch (kind) {
        case art::UnitKind::Infantry:
            drawSword(target, center, scale, sf::Color(238, 242, 230), accent);
            break;
        case art::UnitKind::Shooter:
            drawBow(target, center, scale, accent, sf::Color(41, 37, 32));
            break;
        case art::UnitKind::Cavalry:
            drawHorse(target, center, scale * 0.86f, main, accent);
            break;
        default:
            break;
        }

        sf::CircleShape badge(2.5f * scale, 16);
        badge.setOrigin(2.5f * scale, 2.5f * scale);
        badge.setPosition(center.x + 6.f * scale, center.y + 6.f * scale);
        badge.setFillColor(accent);
        target.draw(badge);
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

namespace art
{
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
        const sf::Vector2u size = kind == UnitKind::Base ? sf::Vector2u(40, 40) : sf::Vector2u(20, 20);
        sf::RenderTexture canvas;
        if (!createCanvas(canvas, size)) {
            return;
        }
        canvas.clear(sf::Color::Transparent);
        const float scale = kind == UnitKind::Base ? 1.f : 1.f;
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
        const sf::Color base = hover ? sf::Color(255, 231, 145) : sf::Color(242, 204, 116);
        const sf::Color fill = pressed ? sf::Color(206, 137, 70) : base;
        const sf::Color outline = pressed ? sf::Color(88, 57, 41) : sf::Color(75, 67, 47);

        drawPill(canvas, sf::Vector2f(inset + 2.f, inset + 5.f),
                 sf::Vector2f(static_cast<float>(size.x) - inset * 2.f - 4.f, static_cast<float>(size.y) - inset * 2.f - 8.f),
                 12.f, sf::Color(17, 23, 22, 70), sf::Color::Transparent, 0.f);
        drawPill(canvas, sf::Vector2f(inset, inset),
                 sf::Vector2f(static_cast<float>(size.x) - inset * 2.f, static_cast<float>(size.y) - inset * 2.f - 3.f),
                 12.f, fill, outline, 2.f);

        sf::RectangleShape shine(sf::Vector2f(static_cast<float>(size.x) - 24.f, 4.f));
        shine.setPosition(12.f, 9.f + inset);
        shine.setFillColor(sf::Color(255, 255, 255, hover ? 95 : 55));
        canvas.draw(shine);

        float textLeft = 8.f;
        if (icon != UnitKind::None) {
            drawUnitIcon(canvas, icon, team, sf::Vector2f(21.f, static_cast<float>(size.y) / 2.f - 2.f + inset * 0.5f), 0.82f);
            textLeft = 38.f;
        }
        drawTextCentered(canvas, font, label, size.y >= 64 ? 24 : 14, sf::Color(37, 35, 29),
                         sf::FloatRect(textLeft, 0.f, static_cast<float>(size.x) - textLeft - 8.f, static_cast<float>(size.y) - 3.f));

        canvas.display();
        commitTexture(texture, canvas);
    }
}

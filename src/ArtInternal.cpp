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
        texture.setSmooth(true);
    }

    bool createCanvas(sf::RenderTexture& canvas, sf::Vector2u size)
    {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 4;
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

    void drawUnitIcon(sf::RenderTarget& target, art::UnitKind kind, art::Team team, sf::Vector2f center, float scale)
    {
        const auto main = art::teamColor(team);
        const auto accent = art::teamAccent(team);
        const auto pale = mix(main, sf::Color::White, 0.48f);
        const auto dark = mix(accent, sf::Color::Black, 0.28f);
        const auto brass = sf::Color(239, 193, 88);

        sf::CircleShape shadow(12.f * scale, 36);
        shadow.setOrigin(12.f * scale, 4.f * scale);
        shadow.setScale(1.35f, 0.42f);
        shadow.setPosition(center.x, center.y + 11.f * scale);
        shadow.setFillColor(sf::Color(13, 23, 20, 85));
        target.draw(shadow);

        if (kind == art::UnitKind::Base) {
            drawBase(target, sf::Vector2f(center.x - 20.f * scale, center.y - 20.f * scale), scale, team);
            return;
        }

        sf::CircleShape rim(14.f * scale, 40);
        rim.setOrigin(14.f * scale, 14.f * scale);
        rim.setPosition(center);
        rim.setFillColor(dark);
        rim.setOutlineColor(sf::Color(255, 230, 139, 210));
        rim.setOutlineThickness(1.35f * scale);
        target.draw(rim);

        sf::CircleShape token(11.4f * scale, 40);
        token.setOrigin(11.4f * scale, 11.4f * scale);
        token.setPosition(center.x - 0.2f * scale, center.y - 0.8f * scale);
        token.setFillColor(sf::Color(pale.r, pale.g, pale.b, 238));
        target.draw(token);

        sf::CircleShape shine(5.2f * scale, 24);
        shine.setOrigin(5.2f * scale, 5.2f * scale);
        shine.setPosition(center.x - 4.5f * scale, center.y - 6.f * scale);
        shine.setFillColor(sf::Color(255, 255, 244, 72));
        target.draw(shine);

        switch (kind) {
        case art::UnitKind::Infantry: {
            drawSword(target, center + sf::Vector2f(5.f * scale, 1.f * scale), scale * 1.13f, sf::Color(244, 247, 232), dark);

            sf::ConvexShape shield(5);
            shield.setPoint(0, sf::Vector2f(center.x - 8.f * scale, center.y - 7.f * scale));
            shield.setPoint(1, sf::Vector2f(center.x + 2.f * scale, center.y - 10.f * scale));
            shield.setPoint(2, sf::Vector2f(center.x + 9.f * scale, center.y - 3.f * scale));
            shield.setPoint(3, sf::Vector2f(center.x + 5.f * scale, center.y + 10.f * scale));
            shield.setPoint(4, sf::Vector2f(center.x - 8.f * scale, center.y + 7.f * scale));
            shield.setFillColor(sf::Color(242, 215, 121));
            shield.setOutlineColor(dark);
            shield.setOutlineThickness(1.1f * scale);
            target.draw(shield);

            sf::RectangleShape stripe(sf::Vector2f(2.8f * scale, 16.f * scale));
            stripe.setOrigin(1.4f * scale, 8.f * scale);
            stripe.setPosition(center.x + 0.5f * scale, center.y);
            stripe.setFillColor(main);
            stripe.setRotation(-14.f);
            target.draw(stripe);
            break;
        }
        case art::UnitKind::Shooter: {
            sf::ConvexShape cape(4);
            cape.setPoint(0, sf::Vector2f(center.x - 9.f * scale, center.y - 3.f * scale));
            cape.setPoint(1, sf::Vector2f(center.x - 1.f * scale, center.y - 12.f * scale));
            cape.setPoint(2, sf::Vector2f(center.x + 8.f * scale, center.y - 1.f * scale));
            cape.setPoint(3, sf::Vector2f(center.x + 2.f * scale, center.y + 10.f * scale));
            cape.setFillColor(mix(main, sf::Color::Black, 0.10f));
            cape.setOutlineColor(dark);
            cape.setOutlineThickness(0.9f * scale);
            target.draw(cape);

            sf::ConvexShape hood(3);
            hood.setPoint(0, sf::Vector2f(center.x - 7.f * scale, center.y - 5.f * scale));
            hood.setPoint(1, sf::Vector2f(center.x - 1.f * scale, center.y - 13.f * scale));
            hood.setPoint(2, sf::Vector2f(center.x + 7.f * scale, center.y - 5.f * scale));
            hood.setFillColor(dark);
            target.draw(hood);

            sf::CircleShape face(3.2f * scale, 18);
            face.setOrigin(3.2f * scale, 3.2f * scale);
            face.setPosition(center.x - 0.5f * scale, center.y - 4.f * scale);
            face.setFillColor(sf::Color(246, 219, 166));
            target.draw(face);

            drawBow(target, center + sf::Vector2f(1.5f * scale, 1.5f * scale), scale * 1.20f, brass, sf::Color(39, 34, 28));
            sf::RectangleShape quiver(sf::Vector2f(3.f * scale, 10.f * scale));
            quiver.setOrigin(1.5f * scale, 5.f * scale);
            quiver.setPosition(center.x + 8.f * scale, center.y + 2.f * scale);
            quiver.setRotation(-20.f);
            quiver.setFillColor(sf::Color(82, 53, 35));
            target.draw(quiver);
            break;
        }
        case art::UnitKind::Cavalry: {
            sf::RectangleShape lance(sf::Vector2f(20.f * scale, 1.8f * scale));
            lance.setOrigin(2.f * scale, 0.8f * scale);
            lance.setPosition(center.x - 5.f * scale, center.y - 7.f * scale);
            lance.setRotation(-25.f);
            lance.setFillColor(sf::Color(247, 238, 197));
            target.draw(lance);

            const sf::Color horseBody = team == art::Team::Enemy ? sf::Color(89, 104, 132) : sf::Color(121, 77, 43);

            sf::ConvexShape knight(7);
            knight.setPoint(0, sf::Vector2f(center.x - 9.f * scale, center.y + 8.f * scale));
            knight.setPoint(1, sf::Vector2f(center.x - 7.f * scale, center.y - 3.f * scale));
            knight.setPoint(2, sf::Vector2f(center.x - 1.f * scale, center.y - 10.f * scale));
            knight.setPoint(3, sf::Vector2f(center.x + 8.f * scale, center.y - 7.f * scale));
            knight.setPoint(4, sf::Vector2f(center.x + 10.f * scale, center.y - 1.f * scale));
            knight.setPoint(5, sf::Vector2f(center.x + 3.f * scale, center.y + 5.f * scale));
            knight.setPoint(6, sf::Vector2f(center.x + 6.f * scale, center.y + 11.f * scale));
            knight.setFillColor(horseBody);
            knight.setOutlineColor(dark);
            knight.setOutlineThickness(1.1f * scale);
            target.draw(knight);

            sf::ConvexShape mane(4);
            mane.setPoint(0, sf::Vector2f(center.x - 5.f * scale, center.y - 5.f * scale));
            mane.setPoint(1, sf::Vector2f(center.x + 1.f * scale, center.y - 10.f * scale));
            mane.setPoint(2, sf::Vector2f(center.x + 4.f * scale, center.y - 6.f * scale));
            mane.setPoint(3, sf::Vector2f(center.x - 2.f * scale, center.y - 2.f * scale));
            mane.setFillColor(dark);
            target.draw(mane);

            sf::CircleShape eye(1.1f * scale, 10);
            eye.setOrigin(1.1f * scale, 1.1f * scale);
            eye.setPosition(center.x + 5.6f * scale, center.y - 4.1f * scale);
            eye.setFillColor(sf::Color(255, 236, 165));
            target.draw(eye);

            sf::RectangleShape reins(sf::Vector2f(8.f * scale, 1.4f * scale));
            reins.setOrigin(0.f, 0.7f * scale);
            reins.setPosition(center.x + 0.5f * scale, center.y + 1.f * scale);
            reins.setRotation(-18.f);
            reins.setFillColor(brass);
            target.draw(reins);
            break;
        }
        case art::UnitKind::Siege: {
            sf::ConvexShape carriage(6);
            carriage.setPoint(0, sf::Vector2f(center.x - 10.f * scale, center.y + 5.f * scale));
            carriage.setPoint(1, sf::Vector2f(center.x - 7.f * scale, center.y - 4.f * scale));
            carriage.setPoint(2, sf::Vector2f(center.x + 3.f * scale, center.y - 8.f * scale));
            carriage.setPoint(3, sf::Vector2f(center.x + 11.f * scale, center.y - 1.f * scale));
            carriage.setPoint(4, sf::Vector2f(center.x + 8.f * scale, center.y + 8.f * scale));
            carriage.setPoint(5, sf::Vector2f(center.x - 4.f * scale, center.y + 10.f * scale));
            carriage.setFillColor(sf::Color(87, 73, 56));
            carriage.setOutlineColor(dark);
            carriage.setOutlineThickness(1.f * scale);
            target.draw(carriage);

            sf::RectangleShape barrel(sf::Vector2f(21.f * scale, 4.4f * scale));
            barrel.setOrigin(3.f * scale, 2.2f * scale);
            barrel.setPosition(center.x - 3.f * scale, center.y - 5.f * scale);
            barrel.setRotation(-18.f);
            barrel.setFillColor(sf::Color(221, 218, 196));
            barrel.setOutlineColor(sf::Color(38, 35, 31));
            barrel.setOutlineThickness(0.8f * scale);
            target.draw(barrel);

            sf::CircleShape wheelL(3.7f * scale, 18);
            wheelL.setOrigin(3.7f * scale, 3.7f * scale);
            wheelL.setPosition(center.x - 7.2f * scale, center.y + 8.f * scale);
            wheelL.setFillColor(sf::Color(48, 42, 36));
            wheelL.setOutlineColor(brass);
            wheelL.setOutlineThickness(0.9f * scale);
            target.draw(wheelL);

            sf::CircleShape wheelR = wheelL;
            wheelR.setPosition(center.x + 7.2f * scale, center.y + 5.8f * scale);
            target.draw(wheelR);

            sf::CircleShape spark(2.3f * scale, 12);
            spark.setOrigin(2.3f * scale, 2.3f * scale);
            spark.setPosition(center.x + 11.f * scale, center.y - 9.f * scale);
            spark.setFillColor(sf::Color(255, 226, 112));
            target.draw(spark);
            break;
        }
        case art::UnitKind::Guardian: {
            sf::ConvexShape armor(8);
            armor.setPoint(0, sf::Vector2f(center.x - 8.f * scale, center.y + 10.f * scale));
            armor.setPoint(1, sf::Vector2f(center.x - 11.f * scale, center.y + 1.f * scale));
            armor.setPoint(2, sf::Vector2f(center.x - 8.f * scale, center.y - 8.f * scale));
            armor.setPoint(3, sf::Vector2f(center.x - 1.f * scale, center.y - 12.f * scale));
            armor.setPoint(4, sf::Vector2f(center.x + 8.f * scale, center.y - 8.f * scale));
            armor.setPoint(5, sf::Vector2f(center.x + 11.f * scale, center.y + 1.f * scale));
            armor.setPoint(6, sf::Vector2f(center.x + 7.f * scale, center.y + 10.f * scale));
            armor.setPoint(7, sf::Vector2f(center.x - 1.f * scale, center.y + 13.f * scale));
            armor.setFillColor(mix(main, sf::Color(65, 73, 70), 0.55f));
            armor.setOutlineColor(sf::Color(31, 35, 33));
            armor.setOutlineThickness(1.2f * scale);
            target.draw(armor);

            sf::RectangleShape crest(sf::Vector2f(4.f * scale, 14.f * scale));
            crest.setOrigin(2.f * scale, 7.f * scale);
            crest.setPosition(center.x, center.y - 1.f * scale);
            crest.setFillColor(brass);
            crest.setRotation(-12.f);
            target.draw(crest);

            sf::CircleShape core(4.8f * scale, 22);
            core.setOrigin(4.8f * scale, 4.8f * scale);
            core.setPosition(center.x + 1.f * scale, center.y - 2.f * scale);
            core.setFillColor(sf::Color(255, 229, 120));
            core.setOutlineColor(sf::Color(71, 53, 28));
            core.setOutlineThickness(0.8f * scale);
            target.draw(core);

            drawSword(target, center + sf::Vector2f(8.f * scale, 2.f * scale), scale * 1.1f, sf::Color(250, 244, 211), dark);
            break;
        }
        default:
            break;
        }

        sf::CircleShape badge(3.3f * scale, 18);
        badge.setOrigin(3.3f * scale, 3.3f * scale);
        badge.setPosition(center.x + 8.7f * scale, center.y + 8.5f * scale);
        badge.setFillColor(accent);
        badge.setOutlineColor(sf::Color(255, 245, 207, 210));
        badge.setOutlineThickness(0.8f * scale);
        target.draw(badge);
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

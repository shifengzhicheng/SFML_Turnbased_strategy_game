#include "ArtAssets.h"
#include "ArtInternal.h"
#include "Config.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

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

    const sf::Texture& unitTexture(UnitKind kind, Team team)
    {
        constexpr std::size_t kindCount = 6;
        constexpr std::size_t teamCount = 3;
        const std::size_t kindIndex = static_cast<std::size_t>(kind);
        const std::size_t teamIndex = static_cast<std::size_t>(team);
        if (kindIndex >= kindCount || teamIndex >= teamCount) {
            throw std::out_of_range("invalid cached unit texture key");
        }

        struct Cache
        {
            std::array<sf::Texture, kindCount * teamCount> textures;
            std::array<bool, kindCount * teamCount> ready{};
        };
        static Cache cache;

        // Unit creation and rendering remain on the SFML context thread. Lazy
        // creation avoids allocating all variants in headless simulations.
        const std::size_t index = kindIndex * teamCount + teamIndex;
        if (!cache.ready[index]) {
            makeUnitTexture(cache.textures[index], kind, team);
            cache.ready[index] = true;
        }
        return cache.textures[index];
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
        const sf::Color brass = hover ? sf::Color(255, 226, 120) : sf::Color(225, 169, 72);
        const sf::Color brassDark = pressed ? sf::Color(112, 72, 34) : sf::Color(145, 96, 43);
        const sf::Color face = pressed ? sf::Color(25, 31, 28) : (hover ? sf::Color(48, 64, 55) : sf::Color(32, 42, 38));
        const sf::Color faceTop = hover ? sf::Color(77, 97, 78) : sf::Color(49, 61, 52);
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

        for (float y = 11.f; y < faceRect.getSize().y - 6.f; y += 6.f) {
            sf::RectangleShape scan(sf::Vector2f(faceRect.getSize().x - 12.f, 1.f));
            scan.setPosition(faceRect.getPosition() + sf::Vector2f(6.f, y));
            scan.setFillColor(sf::Color(255, 246, 190, hover ? 18 : 11));
            canvas.draw(scan);
        }

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

        const float boltXs[] = {8.f, static_cast<float>(size.x) - 13.f};
        for (float boltX : boltXs) {
            sf::RectangleShape bolt(sf::Vector2f(4.f, 4.f));
            bolt.setPosition(boltX + shift, 8.f + shift);
            bolt.setFillColor(sf::Color(255, 235, 143, hover ? 180 : 118));
            canvas.draw(bolt);
            bolt.setPosition(boltX + shift, static_cast<float>(size.y) - 13.f + shift);
            bolt.setFillColor(sf::Color(98, 64, 32, 190));
            canvas.draw(bolt);
        }

        float textLeft = 13.f;
        float textRightPad = 12.f;
        if (icon != UnitKind::None) {
            sf::RectangleShape iconWell(sf::Vector2f(40.f, 30.f));
            iconWell.setPosition(8.f + shift, static_cast<float>(size.y) / 2.f - 15.f + shift);
            iconWell.setFillColor(sf::Color(18, 25, 22));
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
            sf::RectangleShape teamTag(sf::Vector2f(4.f, 22.f));
            teamTag.setPosition(iconWell.getPosition() + sf::Vector2f(4.f, 5.f));
            teamTag.setFillColor(teamColor(team));
            canvas.draw(teamTag);
            drawUnitIcon(canvas, icon, team, iconWell.getPosition() + sf::Vector2f(21.f, 16.f), 0.54f);
            textLeft = 55.f;

            const std::string cost = costForIcon(icon);
            sf::RectangleShape costChip(sf::Vector2f(27.f, 20.f));
            costChip.setPosition(static_cast<float>(size.x) - 33.f + shift, static_cast<float>(size.y) / 2.f - 10.f + shift);
            costChip.setFillColor(sf::Color(79, 51, 23));
            costChip.setOutlineColor(sf::Color(255, 219, 93));
            costChip.setOutlineThickness(1.3f);
            canvas.draw(costChip);
            sf::RectangleShape costShine(sf::Vector2f(21.f, 2.f));
            costShine.setPosition(costChip.getPosition() + sf::Vector2f(3.f, 3.f));
            costShine.setFillColor(sf::Color(255, 247, 190, 64));
            canvas.draw(costShine);
            drawTextCentered(canvas, font, cost, cost.size() > 2 ? 11 : 12, sf::Color(255, 233, 127),
                             sf::FloatRect(costChip.getPosition().x, costChip.getPosition().y - 1.f, costChip.getSize().x, costChip.getSize().y));
            textRightPad = 37.f;
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

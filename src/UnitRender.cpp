#include "UnitRender.h"

#include "AllUnit.h"
#include "CombatBehavior.h"
#include "Config.h"
#include "GameTypes.h"

#include <algorithm>
#include <cmath>

namespace unit_render
{
    void drawCombatCue(sf::RenderTarget& target, const MoveableUnit& unit, float gameTimeSeconds)
    {
        const sf::Vector2f foot((unit.x + 0.5f) * config::TileSize,
                                (unit.y + 0.88f) * config::TileSize);
        if (gameTimeSeconds + 0.001f < unit.deploymentReadyTime) {
            const float pulse = 0.5f + std::sin(gameTimeSeconds * 6.f + static_cast<float>(unit.entityId)) * 0.5f;
            sf::CircleShape rally(14.f, 16);
            rally.setOrigin(14.f, 14.f);
            rally.setScale(1.f, 0.34f);
            rally.setPosition(foot);
            rally.setFillColor(sf::Color(255, 211, 84, static_cast<sf::Uint8>(18.f + pulse * 18.f)));
            rally.setOutlineColor(sf::Color(255, 227, 128, static_cast<sf::Uint8>(115.f + pulse * 95.f)));
            rally.setOutlineThickness(2.f);
            target.draw(rally);
        }

        if (unit.unitName == UName::CAVALRY && unit.UnitState == UState::MOVING
            && unit.tilesMovedSinceAttack > 0) {
            const float intensity = std::min(1.f, static_cast<float>(unit.tilesMovedSinceAttack)
                / static_cast<float>(config::CavalryChargeTiles));
            sf::RectangleShape trail(sf::Vector2f(28.f + intensity * 12.f, 4.f));
            trail.setOrigin(trail.getSize().x * 0.5f, 2.f);
            trail.setPosition(foot + sf::Vector2f(0.f, 2.f));
            trail.setFillColor(unit.myteam == PLAYER
                ? sf::Color(255, 154, 70, static_cast<sf::Uint8>(40.f + intensity * 70.f))
                : sf::Color(103, 176, 255, static_cast<sf::Uint8>(40.f + intensity * 70.f)));
            target.draw(trail);
        }

        const CombatBehaviorDefinition& behavior = combatBehavior(unit.unitName);
        if (behavior.deploymentSeconds > 0.f && unit.UnitState != UState::MOVING
            && unit.stationarySeconds < behavior.deploymentSeconds
            && gameTimeSeconds + 0.001f >= unit.deploymentReadyTime) {
            const float progress = std::clamp(unit.stationarySeconds / behavior.deploymentSeconds, 0.f, 1.f);
            const sf::Color braceColor = unit.myteam == PLAYER
                ? sf::Color(255, 201, 91, 180)
                : sf::Color(118, 181, 255, 180);
            sf::RectangleShape left(sf::Vector2f(8.f + progress * 8.f, 2.f));
            left.setOrigin(left.getSize().x, 1.f);
            left.setPosition(foot + sf::Vector2f(-6.f, 1.f));
            left.setRotation(18.f);
            left.setFillColor(braceColor);
            target.draw(left);
            sf::RectangleShape right(left);
            right.setOrigin(0.f, 1.f);
            right.setPosition(foot + sf::Vector2f(6.f, 1.f));
            right.setRotation(-18.f);
            target.draw(right);
        }
    }

    void drawHealthBar(sf::RenderTarget& target, const MoveableUnit& unit, bool showNumber)
    {
        const int maximum = std::max(1, unit.maxHealth());
        const float ratio = std::clamp(static_cast<float>(unit.Health) / static_cast<float>(maximum), 0.f, 1.f);
        if (ratio >= 0.999f && !showNumber) {
            return;
        }

        const sf::Vector2f position((unit.x + 0.5f) * config::TileSize - 14.f,
                                    unit.UnitText.getPosition().y + 6.f);
        sf::RectangleShape back(sf::Vector2f(28.f, 5.f));
        back.setPosition(position);
        back.setFillColor(sf::Color(20, 24, 21, 220));
        back.setOutlineColor(sf::Color(240, 230, 190, 120));
        back.setOutlineThickness(1.f);
        target.draw(back);

        const sf::Color fill = ratio > 0.55f
            ? sf::Color(102, 207, 105)
            : (ratio > 0.25f ? sf::Color(239, 188, 73) : sf::Color(225, 77, 58));
        sf::RectangleShape health(sf::Vector2f(26.f * ratio, 3.f));
        health.setPosition(position + sf::Vector2f(1.f, 1.f));
        health.setFillColor(fill);
        target.draw(health);

        if (showNumber) {
            target.draw(unit.UnitText);
        }
    }
}

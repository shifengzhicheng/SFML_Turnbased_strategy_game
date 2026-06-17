#include "Effects.h"
#include "Config.h"

#include <algorithm>
#include <cmath>
#include <utility>

void Effects::clear()
{
    attacks.clear();
    floatingTexts.clear();
    shakeDurationSeconds = 0.f;
    shakeIntensity = 0.f;
}

void Effects::addAttack(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    const auto delta = end - start;
    const auto length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length <= 0.f) {
        return;
    }

    // SFML window/context work stays on the main thread; effects are lightweight
    // time-driven drawables, so splitting rendering into another thread is riskier.
    AttackEffect effect;
    effect.durationSeconds = 0.38f;
    effect.beam.setSize(sf::Vector2f(length, 3.f));
    effect.beam.setOrigin(0.f, 1.5f);
    effect.beam.setPosition(start);
    effect.color = color;
    effect.beam.setFillColor(color);
    effect.beam.setRotation(static_cast<float>(std::atan2(delta.y, delta.x) * 180.0 / config::Pi));

    effect.impact.setSize(sf::Vector2f(12.f, 12.f));
    effect.impact.setOrigin(6.f, 6.f);
    effect.impact.setPosition(end);
    effect.impact.setFillColor(sf::Color(255, 232, 112, 150));
    effect.impact.setOutlineColor(sf::Color(255, 86, 43, 220));
    effect.impact.setOutlineThickness(2.f);
    attacks.push_back(std::move(effect));
}

void Effects::addFloatingText(const sf::Font& font, sf::Vector2f position, const std::string& value,
                              sf::Color color, unsigned int size)
{
    FloatingText effect;
    effect.text.setFont(font);
    effect.text.setString(value);
    effect.text.setCharacterSize(size);
    effect.text.setFillColor(color);
    effect.text.setOutlineColor(sf::Color(39, 32, 24, 210));
    effect.text.setOutlineThickness(1.2f);
    const auto bounds = effect.text.getLocalBounds();
    effect.text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    effect.startPosition = position;
    effect.velocity = sf::Vector2f(0.f, -34.f);
    effect.durationSeconds = 0.82f;
    effect.text.setPosition(effect.startPosition);
    floatingTexts.push_back(std::move(effect));
}

void Effects::startShake(float durationSeconds, float intensity)
{
    shakeDurationSeconds = std::max(shakeDurationSeconds, durationSeconds);
    shakeIntensity = std::max(shakeIntensity, intensity);
    shakeClock.restart();
}

sf::Vector2f Effects::shakeOffset() const
{
    if (shakeDurationSeconds <= 0.f) {
        return sf::Vector2f(0.f, 0.f);
    }

    const float elapsed = shakeClock.getElapsedTime().asSeconds();
    if (elapsed >= shakeDurationSeconds) {
        return sf::Vector2f(0.f, 0.f);
    }

    const float decay = 1.f - elapsed / shakeDurationSeconds;
    return sf::Vector2f(
        std::sin(elapsed * 91.f) * shakeIntensity * decay,
        std::cos(elapsed * 73.f) * shakeIntensity * decay);
}

void Effects::draw(sf::RenderWindow& window)
{
    drawAttacks(window);
    drawFloatingTexts(window);
}

void Effects::drawAttacks(sf::RenderWindow& window)
{
    for (auto it = attacks.begin(); it != attacks.end(); ) {
        const float elapsed = it->lifetime.getElapsedTime().asSeconds();
        const float progress = elapsed / it->durationSeconds;
        if (progress >= 1.f) {
            it = attacks.erase(it);
            continue;
        }

        const auto alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
        auto beamColor = it->color;
        beamColor.a = alpha;
        it->beam.setFillColor(beamColor);
        it->beam.setScale(1.f, 1.f + progress * 1.25f);

        const float size = 12.f + 22.f * progress;
        it->impact.setSize(sf::Vector2f(size, size));
        it->impact.setOrigin(size * 0.5f, size * 0.5f);
        it->impact.setFillColor(sf::Color(255, 214, 82, static_cast<sf::Uint8>(120.f * (1.f - progress))));
        it->impact.setOutlineColor(sf::Color(it->color.r, it->color.g, it->color.b, alpha));

        window.draw(it->beam);
        window.draw(it->impact);
        ++it;
    }
}

void Effects::drawFloatingTexts(sf::RenderWindow& window)
{
    for (auto it = floatingTexts.begin(); it != floatingTexts.end(); ) {
        const float elapsed = it->lifetime.getElapsedTime().asSeconds();
        const float progress = elapsed / it->durationSeconds;
        if (progress >= 1.f) {
            it = floatingTexts.erase(it);
            continue;
        }

        const float eased = 1.f - (1.f - progress) * (1.f - progress);
        it->text.setPosition(it->startPosition + it->velocity * eased);
        const auto alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
        auto fill = it->text.getFillColor();
        fill.a = alpha;
        it->text.setFillColor(fill);
        auto outline = it->text.getOutlineColor();
        outline.a = static_cast<sf::Uint8>(210.f * (1.f - progress));
        it->text.setOutlineColor(outline);
        window.draw(it->text);
        ++it;
    }
}

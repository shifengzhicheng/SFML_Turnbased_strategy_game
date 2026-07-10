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

void Effects::addAttack(sf::Vector2f start, sf::Vector2f end, sf::Color color, AttackEffectStyle style)
{
    const auto delta = end - start;
    const auto length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length <= 0.f) {
        return;
    }

    // SFML window/context work stays on the main thread; effects are lightweight
    // time-driven drawables, so splitting rendering into another thread is riskier.
    AttackEffect effect;
    effect.start = start;
    effect.end = end;
    effect.style = style;
    switch (style) {
    case AttackEffectStyle::Arrow:
        effect.durationSeconds = 0.24f;
        break;
    case AttackEffectStyle::Shell:
        effect.durationSeconds = 0.42f;
        break;
    case AttackEffectStyle::Charge:
        effect.durationSeconds = 0.30f;
        break;
    case AttackEffectStyle::Slash:
    case AttackEffectStyle::Heavy:
        effect.durationSeconds = 0.22f;
        break;
    case AttackEffectStyle::Beam:
    default:
        effect.durationSeconds = 0.34f;
        break;
    }
    effect.color = color;
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

void Effects::draw(sf::RenderTarget& target)
{
    drawAttacks(target);
    drawFloatingTexts(target);
}

void Effects::drawAttacks(sf::RenderTarget& target)
{
    for (auto it = attacks.begin(); it != attacks.end(); ) {
        const float elapsed = it->lifetime.getElapsedTime().asSeconds();
        const float progress = elapsed / it->durationSeconds;
        if (progress >= 1.f) {
            it = attacks.erase(it);
            continue;
        }

        const sf::Vector2f delta = it->end - it->start;
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        const float angle = static_cast<float>(std::atan2(delta.y, delta.x) * 180.0 / config::Pi);
        const auto alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
        const auto faded = [alpha](sf::Color color) {
            color.a = static_cast<sf::Uint8>(static_cast<unsigned int>(color.a) * alpha / 255u);
            return color;
        };
        const auto drawImpact = [&target, &effect = *it, progress, faded](float startProgress, float maxRadius) {
            if (progress < startProgress) {
                return;
            }
            const float local = (progress - startProgress) / std::max(0.01f, 1.f - startProgress);
            sf::CircleShape ring(3.f + maxRadius * local, 12);
            ring.setOrigin(ring.getRadius(), ring.getRadius());
            ring.setPosition(effect.end);
            ring.setFillColor(sf::Color(255, 205, 86, static_cast<sf::Uint8>(85.f * (1.f - local))));
            ring.setOutlineColor(faded(effect.color));
            ring.setOutlineThickness(2.f);
            target.draw(ring);
        };

        switch (it->style) {
        case AttackEffectStyle::Arrow: {
            const float travel = std::min(1.f, progress / 0.78f);
            const sf::Vector2f position = it->start + delta * travel;
            sf::RectangleShape shaft(sf::Vector2f(15.f, 2.f));
            shaft.setOrigin(12.f, 1.f);
            shaft.setPosition(position);
            shaft.setRotation(angle);
            shaft.setFillColor(faded(it->color));
            target.draw(shaft);
            sf::ConvexShape head(3);
            head.setPoint(0, sf::Vector2f(5.f, 0.f));
            head.setPoint(1, sf::Vector2f(-3.f, -3.f));
            head.setPoint(2, sf::Vector2f(-3.f, 3.f));
            head.setPosition(position);
            head.setRotation(angle);
            head.setFillColor(faded(sf::Color(255, 244, 196)));
            target.draw(head);
            drawImpact(0.72f, 7.f);
            break;
        }
        case AttackEffectStyle::Shell: {
            const float travel = std::min(1.f, progress / 0.68f);
            sf::Vector2f position = it->start + delta * travel;
            position.y -= std::sin(travel * static_cast<float>(config::Pi)) * std::min(22.f, length * 0.22f);
            sf::CircleShape shell(4.f, 8);
            shell.setOrigin(4.f, 4.f);
            shell.setPosition(position);
            shell.setFillColor(faded(sf::Color(65, 59, 48)));
            shell.setOutlineColor(faded(it->color));
            shell.setOutlineThickness(2.f);
            target.draw(shell);
            drawImpact(0.60f, 20.f);
            break;
        }
        case AttackEffectStyle::Slash:
        case AttackEffectStyle::Heavy: {
            const bool heavy = it->style == AttackEffectStyle::Heavy;
            const float slashLength = heavy ? 26.f : 19.f;
            sf::RectangleShape slash(sf::Vector2f(slashLength, heavy ? 6.f : 3.f));
            slash.setOrigin(slashLength * 0.5f, heavy ? 3.f : 1.5f);
            slash.setPosition(it->end - delta * (length > 0.f ? 3.f / length : 0.f));
            slash.setRotation(angle + (heavy ? -28.f : 42.f) + progress * (heavy ? 32.f : -58.f));
            slash.setScale(1.f + progress * 0.45f, 1.f);
            slash.setFillColor(faded(it->color));
            target.draw(slash);
            drawImpact(heavy ? 0.30f : 0.42f, heavy ? 13.f : 8.f);
            break;
        }
        case AttackEffectStyle::Charge: {
            const float streakLength = length * std::min(1.f, progress * 2.4f);
            sf::RectangleShape streak(sf::Vector2f(streakLength, 5.f));
            streak.setOrigin(0.f, 2.5f);
            streak.setPosition(it->start);
            streak.setRotation(angle);
            streak.setFillColor(faded(it->color));
            target.draw(streak);
            sf::RectangleShape core(sf::Vector2f(streakLength, 2.f));
            core.setOrigin(0.f, 1.f);
            core.setPosition(it->start);
            core.setRotation(angle);
            core.setFillColor(faded(sf::Color(255, 239, 171)));
            target.draw(core);
            drawImpact(0.32f, 15.f);
            break;
        }
        case AttackEffectStyle::Beam:
        default: {
            sf::RectangleShape beam(sf::Vector2f(length, 3.f));
            beam.setOrigin(0.f, 1.5f);
            beam.setPosition(it->start);
            beam.setRotation(angle);
            beam.setScale(1.f, 1.f + progress * 1.25f);
            beam.setFillColor(faded(it->color));
            target.draw(beam);
            drawImpact(0.f, 16.f);
            break;
        }
        }
        ++it;
    }
}

void Effects::drawFloatingTexts(sf::RenderTarget& target)
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
        target.draw(it->text);
        ++it;
    }
}

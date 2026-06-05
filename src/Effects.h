#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct AttackEffect
{
    sf::RectangleShape beam;
    sf::CircleShape impact;
    sf::Color color = sf::Color::White;
    sf::Clock lifetime;
    float durationSeconds = 0.35f;
};

struct FloatingText
{
    sf::Text text;
    sf::Vector2f startPosition;
    sf::Vector2f velocity;
    sf::Clock lifetime;
    float durationSeconds = 0.8f;
};

class Effects
{
public:
    void clear();
    void addAttack(sf::Vector2f start, sf::Vector2f end, sf::Color color);
    void addFloatingText(const sf::Font& font, sf::Vector2f position, const std::string& value,
                         sf::Color color, unsigned int size);
    void startShake(float durationSeconds, float intensity);
    sf::Vector2f shakeOffset() const;
    void draw(sf::RenderWindow& window);

private:
    std::vector<AttackEffect> attacks;
    std::vector<FloatingText> floatingTexts;
    sf::Clock shakeClock;
    float shakeDurationSeconds = 0.f;
    float shakeIntensity = 0.f;

    void drawAttacks(sf::RenderWindow& window);
    void drawFloatingTexts(sf::RenderWindow& window);
};

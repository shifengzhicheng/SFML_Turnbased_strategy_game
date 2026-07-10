#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color, AttackEffectStyle style)
{
    effects.addAttack(start, end, color, style);
}

void Game::addUnitAttackEffect(int unitName, sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    AttackEffectStyle style = AttackEffectStyle::Slash;
    switch (unitName) {
    case UName::SHOOTER:
        style = AttackEffectStyle::Arrow;
        break;
    case UName::CAVALRY:
        style = AttackEffectStyle::Charge;
        break;
    case UName::SIEGE:
        style = AttackEffectStyle::Shell;
        break;
    case UName::GUARDIAN:
        style = AttackEffectStyle::Heavy;
        break;
    case UName::INFANTARY:
    default:
        style = AttackEffectStyle::Slash;
        break;
    }
    addAttackEffect(start, end, color, style);
}

void Game::addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size)
{
    effects.addFloatingText(myfont, position, value, color, size);
}

void Game::startScreenShake(float durationSeconds, float intensity)
{
    effects.startShake(durationSeconds, intensity);
}

sf::Vector2f Game::currentShakeOffset() const
{
    return effects.shakeOffset();
}

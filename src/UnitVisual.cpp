#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Tile.h"
#include "Game.h"
#include "ArtAssets.h"
#include "RealtimeConfig.h"
#include "UnitVisualHelpers.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace sf;
using namespace std;
using namespace unit_visual;

void Unit::setState(int state)
{
	switch (state)
	{
	case UState::UNITNORMAL:
		UnitState = UState::UNITNORMAL;
		break;
	case UState::UNITCLICK:
		UnitState = UState::UNITCLICK;
		break;
	case UState::MOVING:
		UnitState = UState::MOVING;
		break;
	default:
		break;
	}
}

void Unit::playFlash(sf::Color color, float seconds)
{
	flashColor = color;
	flashSeconds = seconds;
	flashing = seconds > 0.f;
	flashClock.restart();
	setColor(color);
}

void Unit::playAction(sf::Vector2f direction, float seconds)
{
	const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
	if (length <= 0.f || seconds <= 0.f) {
		return;
	}
	actionDirection = sf::Vector2f(direction.x / length, direction.y / length);
	actionSeconds = seconds;
	actionAnimating = true;
	actionClock.restart();
}

sf::Vector2f Unit::actionOffset(float distance)
{
	if (!actionAnimating) {
		return sf::Vector2f(0.f, 0.f);
	}

	const float progress = actionClock.getElapsedTime().asSeconds() / actionSeconds;
	if (progress >= 1.f) {
		actionAnimating = false;
		return sf::Vector2f(0.f, 0.f);
	}

	const float wave = progress < 0.5f ? progress * 2.f : (1.f - progress) * 2.f;
	return actionDirection * (distance * wave);
}

void Unit::updateFlash()
{
	if (!flashing) {
		return;
	}

	const float progress = flashClock.getElapsedTime().asSeconds() / flashSeconds;
	if (progress >= 1.f) {
		flashing = false;
		setColor(sf::Color::White);
		return;
	}

	const auto fade = [progress](sf::Uint8 value) {
		return static_cast<sf::Uint8>(static_cast<float>(value) + (255.f - static_cast<float>(value)) * progress);
	};
	setColor(sf::Color(fade(flashColor.r), fade(flashColor.g), fade(flashColor.b), flashColor.a));
}

void Unit::checkHover(Vector2i mousePos, Event)
{
	if (mousePos.x > 0 && mousePos.x < MapWidth && mousePos.y > 0 && mousePos.y < MapHeight) {
		if (mousePosinMyRange(mousePos)) {
			mygame->MosOnUnit = this;
		}
		else if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Unit
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Blue_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Red_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Choosen)
			mygame->MosOnUnit = nullptr;
	}
	else mygame->MosOnUnit = nullptr;
}

bool Unit::mousePosinMyRange(Vector2i mousePos)
{
	return getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
}

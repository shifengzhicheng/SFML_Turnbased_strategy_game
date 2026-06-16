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

MoveableUnit::MoveableUnit(int _team, int _x, int _y, Game* _mygame)
{
	mygame = _mygame;
	UnitState = UState::UNITNORMAL;
	x = _x;
	y = _y;
	myteam = _team;
	UnitText.setFont(mygame->myfont);
	UnitText.setCharacterSize(12);
	UnitText.setFillColor(sf::Color(41, 35, 28));
	UnitText.setOutlineColor(sf::Color(255, 246, 205, 210));
	UnitText.setOutlineThickness(1.f);
	placeHealthLabel(UnitText, x, y);
}

bool MoveableUnit::isBlocked(Point p)
{
	astar.setMaze(Astar::makeEmptyMaze());
	myattackpath=astar.GetPath(Point(x, y), p,true);
	if(!myattackpath.empty())
		myattackpath.pop_front();
	for (const auto& it : myattackpath) {
		tile::ID id = mygame->tiles[it.y * mygame->horizontalTiles + it.x].getID();
		if (id == tile::Mount || id == tile::Tree) {
			return true;
		}
	}
	return false;
}

bool MoveableUnit::isdead()
{
	return Health <= 0;
}

void MoveableUnit::move(Point p)
{
	if (mygame->canUnitStepInto(*this, p)) {
		y = p.y;
		x = p.x;
		placeUnitSprite(*this, x, y, config::UnitSpriteScale);
		placeHealthLabel(UnitText, x, y);
	}
	else {
		mypath.clear();
		mygame->running = false;
	}
}

void MoveableUnit::updatemystate()
{
	updateFlash();
	const float t = visualClock.getElapsedTime().asSeconds();
	const float moveBob = UnitState == UState::MOVING ? std::sin(t * 13.f + static_cast<float>(x + y)) * 2.2f : 0.f;
	const float selectedPulse = UnitState == UState::UNITCLICK ? 1.f + std::sin(t * 6.f) * 0.045f : 1.f;
	const sf::Vector2f offset = actionOffset(4.4f) + sf::Vector2f(0.f, moveBob);
	placeUnitSprite(*this, x, y, config::UnitSpriteScale * selectedPulse, offset);
	string temp = std::to_string(Health) + "/" + std::to_string(myinfo.Health);
	UnitText.setString(temp);
	placeHealthLabel(UnitText, x, y, offset);
}

int MoveableUnit::myattack()
{
	if (!attackmethod) {
		return 0;
	}
	return attackmethod->damage;
}

int MoveableUnit::myAttackRange() const
{
	return attackmethod ? attackmethod->range : 0;
}

void MoveableUnit::scaleMaxHealth(float multiplier)
{
	if (multiplier <= 0.f) {
		return;
	}
	myinfo.Health = std::max(1, static_cast<int>(std::round(static_cast<float>(myinfo.Health) * multiplier)));
	Health = std::max(1, static_cast<int>(std::round(static_cast<float>(Health) * multiplier)));
	UnitText.setString(std::to_string(Health) + "/" + std::to_string(myinfo.Health));
}

float MoveableUnit::realtimeMoveStepSeconds() const
{
	switch (unitName) {
	case UName::SHOOTER:
		return realtime::ShooterStepSeconds;
	case UName::CAVALRY:
		return realtime::CavalryStepSeconds;
	case UName::SIEGE:
		return realtime::SiegeStepSeconds;
	case UName::GUARDIAN:
		return realtime::GuardianStepSeconds;
	case UName::INFANTARY:
	default:
		return realtime::InfantryStepSeconds;
	}
}

float MoveableUnit::realtimeAttackCooldownSeconds() const
{
	float baseSeconds = realtime::InfantryAttackCooldown;
	switch (unitName) {
	case UName::SHOOTER:
		baseSeconds = realtime::ShooterAttackCooldown;
		break;
	case UName::CAVALRY:
		baseSeconds = realtime::CavalryAttackCooldown;
		break;
	case UName::SIEGE:
		baseSeconds = realtime::SiegeAttackCooldown;
		break;
	case UName::GUARDIAN:
		baseSeconds = realtime::GuardianAttackCooldown;
		break;
	case UName::INFANTARY:
	default:
		baseSeconds = realtime::InfantryAttackCooldown;
		break;
	}
	return baseSeconds * mygame->unitAttackCooldownMultiplier(myteam, unitName);
}

bool MoveableUnit::canAutoAttack(Unit* target)
{
	return attackmethod && target != nullptr && attackmethod->isInMyAttackRange(this, target);
}

void MoveableUnit::autoAttack(Unit* target)
{
	if (attackmethod && target != nullptr) {
		attackmethod->Attack(this, target);
	}
}

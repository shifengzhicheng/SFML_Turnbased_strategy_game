#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Game.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	constexpr int SqureSize = config::TileSize;

	bool isCombatUnit(int unitName)
	{
		return unitName == UName::SHOOTER || unitName == UName::INFANTARY || unitName == UName::CAVALRY;
	}

	bool counters(int attacker, int defender)
	{
		return (attacker == UName::SHOOTER && defender == UName::INFANTARY)
			|| (attacker == UName::INFANTARY && defender == UName::CAVALRY)
			|| (attacker == UName::CAVALRY && defender == UName::SHOOTER);
	}

	sf::Vector2f unitCenter(Unit* unit)
	{
		const auto bounds = unit->getGlobalBounds();
		if (bounds.width > 0.f && bounds.height > 0.f) {
			return sf::Vector2f(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
		}
		const float baseOffset = unit->unitName == UName::BASE ? SqureSize : SqureSize / 2.f;
		return sf::Vector2f(unit->x * SqureSize + baseOffset, unit->y * SqureSize + baseOffset);
	}

	struct DamageResult
	{
		int amount = 0;
		bool counter = false;
		bool resisted = false;
	};

	DamageResult calculateDamage(MoveableUnit* attacker, Unit* defender, int baseDamage)
	{
		DamageResult result;
		result.amount = baseDamage;
		if (!isCombatUnit(attacker->unitName) || !isCombatUnit(defender->unitName)) {
			return result;
		}

		// The triangle is intentionally small and readable: shooter > infantry,
		// infantry > cavalry, cavalry > shooter.
		if (counters(attacker->unitName, defender->unitName)) {
			result.amount = static_cast<int>(std::round(static_cast<float>(baseDamage) * 1.45f));
			result.counter = true;
		}
		else if (counters(defender->unitName, attacker->unitName)) {
			result.amount = static_cast<int>(std::round(static_cast<float>(baseDamage) * 0.8f));
			result.resisted = true;
		}
		result.amount = std::max(1, result.amount);
		return result;
	}
}

bool Attacker::isInMyAttackRange(MoveableUnit* me, Unit* u) {
	if (me == nullptr || u == nullptr) {
		return false;
	}
	int l = sqrt((me->x - u->x) * (me->x - u->x) + (me->y - u->y) * (me->y - u->y));
	if (l > range) return false;
	else {
		if(me->isBlocked(Point(u->x,u->y)))
			return false;
		else return true;
	}
}

void Attacker::drawAttackline(MoveableUnit* me, Unit* u, sf::Color color) {
	mygame->addAttackEffect(unitCenter(me), unitCenter(u), color);
}

void Attacker::Attack(MoveableUnit* me, Unit* u)
{
	if (me == nullptr || u == nullptr) {
		return;
	}
	if (isInMyAttackRange(me, u) && me->isOktoAttackAndAttackconsume()) {
		const DamageResult damageResult = calculateDamage(me, u, damage);
		u->Health -= damageResult.amount;

		const sf::Vector2f targetCenter = unitCenter(u);
		const sf::Color beamColor = damageResult.counter
			? sf::Color(255, 206, 65, 255)
			: (damageResult.resisted ? sf::Color(113, 176, 255, 255) : sf::Color(255, 74, 39, 255));

		me->playFlash(sf::Color(255, 245, 180, 255), 0.16f);
		u->playFlash(sf::Color(255, 146, 112, 255), 0.22f);
		drawAttackline(me, u, beamColor);

		mygame->addFloatingText(targetCenter + sf::Vector2f(0.f, -16.f), "-" + std::to_string(damageResult.amount),
			damageResult.counter ? sf::Color(255, 222, 90) : sf::Color(255, 106, 82), damageResult.counter ? 18 : 15);
		if (damageResult.counter) {
			me->gainActionPoint(1);
			mygame->addFloatingText(targetCenter + sf::Vector2f(0.f, -34.f), "COUNTER!", sf::Color(255, 238, 126), 13);
			mygame->addFloatingText(unitCenter(me) + sf::Vector2f(0.f, -20.f), "AP+1", sf::Color(214, 255, 142), 12);
		}
		else if (damageResult.resisted) {
			mygame->addFloatingText(targetCenter + sf::Vector2f(0.f, -32.f), "RESIST", sf::Color(156, 205, 255), 12);
		}
		if (u->Health <= 0) {
			me->gainActionPoint(3);
			mygame->addFloatingText(targetCenter + sf::Vector2f(0.f, -48.f), "KO!", sf::Color(255, 245, 206), 17);
			mygame->addFloatingText(unitCenter(me) + sf::Vector2f(0.f, -36.f), "FINISH AP+3", sf::Color(214, 255, 142), 12);
		}

		mygame->startScreenShake(damageResult.counter ? 0.20f : 0.13f, damageResult.counter ? 5.2f : 3.2f);
	}
}

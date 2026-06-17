#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Game.h"
#include "UnitGeometry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
	constexpr int SqureSize = config::TileSize;

	bool isCombatUnit(int unitName)
	{
		return unitName == UName::SHOOTER
			|| unitName == UName::INFANTARY
			|| unitName == UName::CAVALRY
			|| unitName == UName::SIEGE
			|| unitName == UName::GUARDIAN;
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

	DamageResult calculateDamage(Game& game, MoveableUnit* attacker, Unit* defender, int baseDamage)
	{
		DamageResult result;
		result.amount = baseDamage;
		if (!isCombatUnit(attacker->unitName) || !isCombatUnit(defender->unitName)) {
			return result;
		}

		// The readable counter web stays small: core units form a triangle,
		// cavalry dives siege, and guardians remain the late-game tank rather
		// than being invalidated by siege splash.
		if (attacker->unitName == UName::SIEGE && defender->unitName == UName::GUARDIAN) {
			result.amount = static_cast<int>(std::round(static_cast<float>(baseDamage)
				* 0.85f
				* game.siegeDamageTakenMultiplier(defender->myteam, defender->unitName)));
			result.resisted = true;
		}
		else if (game.counterApplies(attacker->myteam, attacker->unitName, defender->myteam, defender->unitName)) {
			result.amount = static_cast<int>(std::round(static_cast<float>(baseDamage) * 1.60f));
			result.counter = true;
		}
		else if (game.counterApplies(defender->myteam, defender->unitName, attacker->myteam, attacker->unitName)) {
			result.amount = static_cast<int>(std::round(static_cast<float>(baseDamage) * 0.74f));
			result.resisted = true;
		}
		result.amount = std::max(1, result.amount);
		return result;
	}

	sf::Color attackEffectColor(int unitName, const DamageResult& damageResult, bool secondary)
	{
		if (secondary) {
			return sf::Color(255, 166, 94, 230);
		}
		if (damageResult.counter) {
			return sf::Color(255, 206, 65, 255);
		}
		if (damageResult.resisted) {
			return sf::Color(113, 176, 255, 255);
		}
		switch (unitName) {
		case UName::SHOOTER:
			return sf::Color(255, 236, 152, 255);
		case UName::CAVALRY:
			return sf::Color(255, 121, 72, 255);
		case UName::SIEGE:
			return sf::Color(255, 138, 48, 255);
		case UName::GUARDIAN:
			return sf::Color(255, 218, 95, 255);
		case UName::INFANTARY:
		default:
			return sf::Color(255, 92, 58, 255);
		}
	}

	float screenShakeForUnit(int unitName)
	{
		switch (unitName) {
		case UName::SIEGE:
			return 5.2f;
		case UName::GUARDIAN:
			return 4.2f;
		case UName::CAVALRY:
			return 3.8f;
		default:
			return 2.7f;
		}
	}

	int dealDamage(Game& game, MoveableUnit* attacker, Unit* target, int baseDamage, float finalScale, bool secondary)
	{
		const DamageResult damageResult = calculateDamage(game, attacker, target, baseDamage);
		int finalDamage = std::max(1, static_cast<int>(std::round(
			static_cast<float>(damageResult.amount) * game.unitDamageMultiplier(attacker->myteam, attacker->unitName) * finalScale)));
		if (target->unitName == UName::BASE) {
			finalDamage = std::max(1, static_cast<int>(std::round(
				static_cast<float>(finalDamage) * game.baseDamageTakenMultiplier(attacker->unitName, target->myteam))));
		}

		target->Health -= finalDamage;
		if (auto* defender = dynamic_cast<MoveableUnit*>(target); defender != nullptr && defender->Health > 0) {
			defender->rememberAttacker(*attacker);
		}

		const sf::Vector2f targetCenter = unitCenter(target);
		const sf::Color beamColor = attackEffectColor(attacker->unitName, damageResult, secondary);
		const sf::Vector2f attackVector = unitCenter(target) - unitCenter(attacker);
		attacker->playFlash(sf::Color(255, 245, 180, 255), secondary ? 0.10f : 0.16f);
		if (!secondary) {
			attacker->playAction(attackVector, 0.18f);
		}
		target->playFlash(secondary ? sf::Color(255, 183, 116, 255) : sf::Color(255, 146, 112, 255), secondary ? 0.15f : 0.22f);
		target->playAction(sf::Vector2f(-attackVector.x, -attackVector.y), secondary ? 0.15f : 0.22f);
		game.addAttackEffect(unitCenter(attacker), targetCenter, beamColor);
		game.addFloatingText(targetCenter + sf::Vector2f(0.f, secondary ? -10.f : -16.f),
			(secondary ? "~" : "-") + std::to_string(finalDamage),
			damageResult.counter ? sf::Color(255, 222, 90) : sf::Color(255, 106, 82),
			secondary ? 12 : (damageResult.counter ? 18 : 15));
		if (!secondary) {
			if (damageResult.counter) {
				game.addFloatingText(targetCenter + sf::Vector2f(0.f, -34.f), "COUNTER!", sf::Color(255, 238, 126), 13);
			}
			else if (damageResult.resisted) {
				game.addFloatingText(targetCenter + sf::Vector2f(0.f, -32.f), "RESIST", sf::Color(156, 205, 255), 12);
			}
		}
		if (target->Health <= 0) {
			if (auto* defeated = dynamic_cast<MoveableUnit*>(target)) {
				defeated->mypath.clear();
				defeated->pendingPathRequest = 0;
				defeated->realtimeMoveTimer = 0.f;
				defeated->UnitState = UState::UNITNORMAL;
			}
			game.addFloatingText(targetCenter + sf::Vector2f(0.f, secondary ? -28.f : -48.f), "KO!", sf::Color(255, 245, 206), secondary ? 13 : 17);
		}
		return finalDamage;
	}

	std::vector<Unit*> pickAdditionalTargets(Game& game, MoveableUnit* attacker, Unit* primary, int count)
	{
		std::vector<Unit*> picked;
		if (count <= 0) {
			return picked;
		}
		std::vector<MoveableUnit*> candidates;
		auto collect = [&](auto& roster) {
			for (auto& unit : roster) {
				if (unit.get() != primary && unit->Health > 0 && unit->myteam != attacker->myteam && attacker->canAutoAttack(unit.get())) {
					candidates.push_back(unit.get());
				}
			}
		};
		collect(game.myunits);
		collect(game.enemys);
		std::sort(candidates.begin(), candidates.end(), [primary](const MoveableUnit* a, const MoveableUnit* b) {
			const int da = (a->x - primary->x) * (a->x - primary->x) + (a->y - primary->y) * (a->y - primary->y);
			const int db = (b->x - primary->x) * (b->x - primary->x) + (b->y - primary->y) * (b->y - primary->y);
			return da < db;
		});
		for (MoveableUnit* candidate : candidates) {
			picked.push_back(candidate);
			if (static_cast<int>(picked.size()) >= count) {
				break;
			}
		}
		return picked;
	}
}

bool Attacker::isInMyAttackRange(MoveableUnit* me, Unit* u) {
	if (me == nullptr || u == nullptr) {
		return false;
	}
	if (!unit_geometry::isInAttackRange(Point(me->x, me->y), *u, me->myAttackRange())) {
		return false;
	}
	return !me->isBlocked(unit_geometry::closestFootprintCell(Point(me->x, me->y), *u));
}

void Attacker::drawAttackline(MoveableUnit* me, Unit* u, sf::Color color) {
	mygame->addAttackEffect(unitCenter(me), unitCenter(u), color);
}

void Attacker::Attack(MoveableUnit* me, Unit* u)
{
	if (me == nullptr || u == nullptr) {
		return;
	}
	if (isInMyAttackRange(me, u)) {
		dealDamage(*mygame, me, u, damage, 1.f, false);
		const int extraTargets = mygame->additionalAttackTargets(me->myteam, me->unitName);
		const float splashScale = mygame->additionalTargetDamageMultiplier(me->myteam, me->unitName);
		if (extraTargets > 0 && splashScale > 0.f) {
			for (Unit* target : pickAdditionalTargets(*mygame, me, u, extraTargets)) {
				if (target->Health > 0) {
					dealDamage(*mygame, me, target, damage, splashScale, true);
				}
			}
		}

		mygame->startScreenShake(0.13f, screenShakeForUnit(me->unitName));
	}
}

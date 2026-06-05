#include "AllUnit.h"
#include <cmath>
#include "Action.h"
#include "Config.h"
#include "Game.h"

namespace
{
	constexpr int SqureSize = config::TileSize;
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

void Attacker::drawAttackline(MoveableUnit* me, Unit* u) {
	sf::Vector2f p1(me->x * SqureSize+10, me->y * SqureSize+10);
	sf::Vector2f p2(u->x * SqureSize+10, u->y * SqureSize+10);
	mygame->addAttackEffect(p1, p2, sf::Color(255, 74, 39, 255));
}

void Attacker::Attack(MoveableUnit* me, Unit* u)
{
	if (me == nullptr || u == nullptr) {
		return;
	}
	if ( isInMyAttackRange(me, u)&&me->isOktoAttackAndAttackconsume()) {
		u->Health = u->Health - damage;
		drawAttackline(me, u);
	}
}

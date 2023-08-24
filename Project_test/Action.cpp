#include "AllUnit.h"
#include <cmath>
#include "Action.h"
#include "Game.h"
#define SqureSize 20
#define PI 3.1415
bool Attacker::isInMyAttackRange(MoveableUnit* me, Unit* u) {
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
	p2=p1 - p2;
	double length = sqrt(p2.x * p2.x + p2.y * p2.y);
	sf::RectangleShape* pset=new sf::RectangleShape(sf::Vector2f(length,1.f));
	pset->setPosition(p1);
	pset->setFillColor(sf::Color::Red);
	if (p2.x >= 0) {
		pset->rotate(180+float(180*atan((double)p2.y / (double)p2.x)/PI));
	}
	else pset->rotate(float(180*atan((double)p2.y / (double)p2.x)/PI));
	mygame->Attackdraws=shared_ptr<sf::RectangleShape>(pset);
}

void Attacker::Attack(MoveableUnit* me, Unit* u)
{
	if ( isInMyAttackRange(me, u)&&me->isOktoAttackAndAttackconsume()) {
		u->Health = u->Health - damage;
		drawAttackline(me, u);
	}
}
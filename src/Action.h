#pragma once
#include <SFML/Graphics/Color.hpp>

class Unit;
class MoveableUnit;
class Game;

// Base class for all attack behaviors.
class Attacker
{
protected:
	Game* mygame;

public:
	int damage;
	int range;
	Attacker(Game* _game) {
		mygame = _game;
	}
	virtual ~Attacker() = default;
	virtual void Attack(MoveableUnit* me, Unit* u);
	virtual bool isInMyAttackRange(MoveableUnit*, Unit*);
	virtual void drawAttackline(MoveableUnit*, Unit*, sf::Color);
};
class shot :public Attacker {
public:
	shot(Game* _game): Attacker(_game) {
		damage = 28;
		range = 5;
	}
};
class fight :public Attacker {
public:
	fight(Game* _game) : Attacker(_game) {
		damage = 48;
		range = 2;
	}
};
class roll : public Attacker {
public:
	roll(Game* _game) : Attacker(_game) {
		damage = 68;
		range = 2;
	}
};
class bombard : public Attacker {
public:
	bombard(Game* _game) : Attacker(_game) {
		damage = 56;
		range = 9;
	}
};
class crush : public Attacker {
public:
	crush(Game* _game) : Attacker(_game) {
		damage = 88;
		range = 2;
	}
};

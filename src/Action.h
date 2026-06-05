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
		damage = 30;
		range = 12;
	}
};
class fight :public Attacker {
public:
	fight(Game* _game) : Attacker(_game) {
		damage = 50;
		range = 2;
	}
};
class roll : public Attacker {
public:
	roll(Game* _game) : Attacker(_game) {
		damage = 80;
		range = 2;
	}
};

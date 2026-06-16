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
	int damage = 0;
	int range = 0;
	Attacker(Game* _game, int baseDamage, int attackRange)
		: mygame(_game), damage(baseDamage), range(attackRange)
	{
	}
	virtual ~Attacker() = default;
	virtual void Attack(MoveableUnit* me, Unit* u);
	virtual bool isInMyAttackRange(MoveableUnit*, Unit*);
	virtual void drawAttackline(MoveableUnit*, Unit*, sf::Color);
};

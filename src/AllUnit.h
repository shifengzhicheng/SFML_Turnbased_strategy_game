#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <iostream>
#include <memory>
#include "Action.h"
#include "Point.h"
#include "Astar.h"

class Game;
class Attacker;
namespace UState {
	enum UnitState {
		UNITNORMAL, UNITCLICK, MOVING, FIGHTBACK, ATTACKING
	};
}
namespace UName {
	enum UnitName {
		SHOOTER, INFANTARY, CAVALRY, SIEGE, GUARDIAN, BASE
	};
}
struct defaultInfo
{
	int Health = 0;
};
class Unit :public sf::Sprite
{
protected:
	Game* mygame;
	bool mousePosinMyRange(sf::Vector2i mousePos);
	bool flashing = false;
	float flashSeconds = 0.f;
	sf::Color flashColor = sf::Color::White;
	sf::Clock flashClock;
	bool actionAnimating = false;
	float actionSeconds = 0.f;
	sf::Vector2f actionDirection;
	sf::Clock actionClock;
	sf::Clock visualClock;
	sf::Vector2f actionOffset(float distance);
public:
	
	int UnitState;
	int x;
	int y;
	int myteam;
	int Health;
	int entityId = 0;
	sf::Texture mytexture;
	sf::Text UnitText;
	int unitName = UName::BASE;
	virtual ~Unit() = default;
	virtual void updatemystate()=0;
	void playFlash(sf::Color color, float seconds);
	void playAction(sf::Vector2f direction, float seconds);
	void updateFlash();
	void setState(int state);
	void checkHover(sf::Vector2i, sf::Event);

};
class MoveableUnit : public Unit
{

protected:

	std::deque<Point> myattackpath;
	defaultInfo myinfo;
	std::unique_ptr<Attacker> attackmethod;

public:
	bool isBlocked(Point p);
	Astar astar;

	int laneIndex = 1;
	float realtimeMoveTimer = 0.f;
	float realtimeAttackTimer = 0.f;
	float realtimePathTimer = 0.f;
	int pendingPathRequest = 0;
	Point pendingPathGoal;

	std::deque<Point> mypath;

	MoveableUnit(int _team, int _x, int _y, Game* _mygame);
	bool isdead();
	virtual void move(Point p);
	virtual void updatemystate();
	int myattack();
	int myAttackRange() const;
	void scaleMaxHealth(float multiplier);
	float realtimeMoveStepSeconds() const;
	float realtimeAttackCooldownSeconds() const;
	bool canAutoAttack(Unit* target);
	void autoAttack(Unit* target);
};
class DisMoveableUnit : public Unit
{
	bool indanger;

public:
	DisMoveableUnit(int _x, int _y, int _team, Game* _game);

	void checkMouse(sf::Vector2i mousePos, sf::Event event);

	void updatemystate();
};
class Shooter :public MoveableUnit
{
public:
	Shooter(int _team, int _x, int _y, Game* _mygame);
};
class Cavalry : public MoveableUnit
{
public:
	Cavalry(int _team, int _x, int _y, Game* _mygame);
};
class Infantry : public MoveableUnit
{
public:
	Infantry(int _team, int _x, int _y, Game* _mygame);
};
class Siege : public MoveableUnit
{
public:
	Siege(int _team, int _x, int _y, Game* _mygame);
};
class Guardian : public MoveableUnit
{
public:
	Guardian(int _team, int _x, int _y, Game* _mygame);
};

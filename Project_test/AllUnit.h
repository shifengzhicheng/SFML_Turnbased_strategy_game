#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <list>
#include "Action.h"
#include "Point.h"
#include "Astar.h"

using namespace sf;

class Game;//Game������
class Attacker;
namespace UState {
	enum UnitState {
		UNITNORMAL, UNITCLICK, MOVING, FIGHTBACK, ATTACKING
	};
}
namespace UName {
	enum UnitName {
		SHOOTER,INFANTARY,CAVALRY
	};
}
struct defaultInfo
{
	int UnitState;
	int actionPoint;
	int Health;
	int attackconsume;
};
class Unit :public sf::Sprite
{
protected:
	Game* mygame;
	bool mousePosinMyRange(Vector2i mousePos);
public:
	
	int UnitState;
	int x;
	int y;
	int myteam;//������
	int Health;//����ֵ
	sf::Texture mytexture;
	sf::Text UnitText;
	virtual void updatemystate()=0;
	void setState(int state);
	void checkHover(Vector2i,Event);

};
class MoveableUnit : public Unit
{

protected:

	std::list<shared_ptr<Point>> myattackpath;
	defaultInfo myinfo;
	Attacker* attackmethod;//������ʽ

public:
	
	void setdefalut();

	bool InmyRange();
	bool isBlocked(Point p);
	bool actionPointCanAttack();
	Astar astar;//����ʵ��Ѱ·�㷨

	int myActionPoint;

	std::list<shared_ptr<Point>> mypath;

	void checkMouse(Vector2i, Event);//ÿ����λ������״̬

	MoveableUnit(int _team, int _x, int _y, Game* _mygame);
	void generatepath(Point from, Point to);
	
	void Showpath(Vector2f mousePos);
	bool isOktoAttackAndAttackconsume();
	bool isdead();
	virtual void move(Point p);
	virtual bool guard();
	void generateLongestpath(Point p);//Ѱ��ȥ�з����ص����·��
	virtual void decide();
	virtual void updatemystate();
	int myattack();
};
class DisMoveableUnit : public Unit
{
	bool indanger;
	bool cangenerate;

public:
	DisMoveableUnit(int _x, int _y, int _team, Game* _game);

	void checkMouse(Vector2i mousePos, Event event);

	void generateUnit(int code);

	void reset();

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
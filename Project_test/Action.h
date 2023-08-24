#pragma once
#include "AllUnit.h"
//����ļ�����ʵ�����еĶ����࣬����ʵ�����еĹ���
using namespace std;
class Unit;
class MoveableUnit;
class Game;
class Attacker
	//ʵ�ֹ���
{
protected:
	Game* mygame;

public:
	int damage;
	int range;
	Attacker(Game* _game) {
		mygame = _game;
	}
	virtual void Attack(MoveableUnit* me, Unit* u);
	virtual bool isInMyAttackRange(MoveableUnit*, Unit*);
	virtual void drawAttackline(MoveableUnit*, Unit*);
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

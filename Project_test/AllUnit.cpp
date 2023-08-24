#include "AllUnit.h"
#include "Action.h"
#include "Tile.h"
#include "Game.h"
#include <vector>
#include <iostream>
#include <iterator>
#define SqureSize 20
#define width 1280
#define height 720
class Point;
using namespace std;

Shooter::Shooter(int _team, int _x, int _y, Game* _mygame):MoveableUnit(_team, _x, _y, _mygame)
{
	myinfo.actionPoint = 15;
	UnitState = 0;
	myinfo.Health = 100;
	Health = 100;
	if(myteam==PLAYER)
		mytexture.loadFromFile("data/unit/unit_archers.png");
	else
		mytexture.loadFromFile("data/unit/unit_riflemen.png");
	setTexture(mytexture);
	UnitText.setString("100/100");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 15;
	attackmethod = new shot(mygame);
	myinfo.attackconsume = 1;
}

Infantry::Infantry(int _team, int _x, int _y, Game* _mygame) :MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	Health = 200;
	if (myteam == PLAYER)
	mytexture.loadFromFile("data/unit/unit_swordsmen.png");
	else
		mytexture.loadFromFile("data/unit/unit_peasants.png");
	setTexture(mytexture);
	UnitText.setString("200/200");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 20;
	myinfo.actionPoint = 20;
	myinfo.Health = 200;
	attackmethod = new fight(mygame);
	myinfo.attackconsume = 2;
}

Cavalry::Cavalry(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	Health = 500;
	if (myteam == PLAYER)
	mytexture.loadFromFile("data/unit/unit_horsemen.png");
		else
		mytexture.loadFromFile("data/unit/unit_chariots.png");
	setTexture(mytexture);
	UnitText.setString("500/500");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 40;
	myinfo.actionPoint = 40;
	myinfo.Health = 500;
	attackmethod = new roll(mygame);
	myinfo.attackconsume = 10;
}

void MoveableUnit::Showpath(sf::Vector2f mousePos)
{
	//Show Path of Unit
	
	shared_ptr<Node> head = NULL;
	head = shared_ptr<Node>(new Node(MapPos(Point(x,y), tile::Path)));
	mygame->drawPaths = head;
	bool reachable=true;
	int temp = myActionPoint;
	for (auto& it : mypath) {
		mygame->drawPaths->next = shared_ptr<Node>(new Node(MapPos(*it, tile::Path)));
		mygame->drawPaths = mygame->drawPaths->next;
		temp--;
		if (temp < 0) {
			reachable = false;
			break;
		}
	}
	for (shared_ptr<Node> t = head->next; t != NULL; t = t->next) {
		if (mygame->tiles[mygame->indexAt(sf::Vector2f(t->t.getIndex().x*SqureSize,t->t.getIndex().y*SqureSize))].getID() != tile::Empty) {
			reachable = false;
			break;
		}
	}
	if (mygame->tiles[mygame->indexAt(mousePos)].getID() != tile::Empty) reachable = false;
	if (!reachable) {
		/*head->del();*/
		mygame->drawPaths = shared_ptr<Node>(new Node(MapPos(sf::Mouse::getPosition(mygame->window), tile::UnableToReach)));	
	}
	else mygame->drawPaths = head;
}

bool MoveableUnit::isOktoAttackAndAttackconsume()
{
	if (myinfo.attackconsume <= myActionPoint) {
		myActionPoint -= myinfo.attackconsume;
		return true;
	}
	else return false;
}

void MoveableUnit::setdefalut()
{
	myActionPoint = myinfo.actionPoint;
	UnitState = UState::UNITNORMAL;
}

bool MoveableUnit::InmyRange()
{
	return attackmethod->isInMyAttackRange(this, mygame->MosOnUnit);
}

bool MoveableUnit::isBlocked(Point p)
{
	astar.setMaze(astar.emptymaze);
	myattackpath=astar.GetPath(Point(x, y), p,true);
	if(!myattackpath.empty())
		myattackpath.pop_front();
	for (auto& it : myattackpath) {
		tile::ID id = mygame->tiles[it->y * mygame->horizontalTiles + it->x].getID();
		if (id == tile::Mount || id == tile::Tree) {
			return true;
		}
	}
	return false;
}

bool MoveableUnit::actionPointCanAttack()
{
	if (myActionPoint >= myinfo.attackconsume)
		return true;
	else return false;
}

void MoveableUnit::checkMouse(Vector2i mousePos, Event event)
{
	//�ж�����ǲ����ڰ�ť��
	if (mousePos.x > 0 && mousePos.x < width && mousePos.y > 0 && mousePos.y < height) {
		//����ڵ�λ��Χ�ڵĲ���
		if (myteam == PLAYER && mousePosinMyRange(mousePos)) {
			//mygame->drawPaths->del();
			mygame->drawPaths = shared_ptr<Node>(new Node(MapPos(Point(x, y), tile::Path)));
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				//����ڷ�Χ�ﰴ�������һ����CLICK״̬
				//���������ʱ
				if (UnitState == UState::UNITNORMAL) {
					setState(UState::UNITCLICK);
					//cout << "UNITCLICK" << endl;
				}
				//�����λ�Ѿ�ѡ�У���ôȡ��ѡ��
				else if (UnitState == UState::UNITCLICK) {
					setState(UState::UNITNORMAL);
					//cout << "UNITNORMAL" << endl;
				}
			}
			//���������Hover,�˲�����CheckHover��ʵ��
		}
		else {
			//����ڰ�ť��Χ��
			//����ڵ�λ����������
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {			//�ڷ�Χ���������
				if (UnitState == UState::UNITCLICK) {
					setState(UState::UNITNORMAL);	//�ع�NORMAL״̬	
				}
			}
			//��λѡ�к��Ѱ··����ʾ
			else if (UnitState == UState::UNITCLICK&& mygame->MosOnUnit == NULL &&event.type == Event::EventType::MouseMoved) {
				const int x = static_cast<int>(mousePos.x - (mousePos.x % SqureSize));
				const int y = static_cast<int>(mousePos.y - (mousePos.y % SqureSize));
				if (myActionPoint > 0) {
					int py = y / SqureSize;
					int px = x / SqureSize;
					if (mygame->MosOnUnit == NULL && mygame->MousePosChanged()) {
						if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() == tile::Empty) {
							generatepath(Point(this->x, this->y), Point(px, py));
						}
					}
					Showpath(sf::Vector2f(mousePos));//չʾѰ··��
				}
			}
			//��λ������Ҽ�����
			else if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Right) {
				if (UnitState == UState::UNITCLICK) {
					if (mygame->MosOnUnit == NULL && mygame->drawPaths != NULL && mygame->drawPaths->t.getID() != tile::UnableToReach) {
						const int x = static_cast<int>(mousePos.x - (mousePos.x % SqureSize));
						const int y = static_cast<int>(mousePos.y - (mousePos.y % SqureSize));
						mygame->running = true;
						//mygame->drawPaths->del();
						mygame->drawPaths = NULL;
						UnitState = UState::MOVING;
					}
					else if (mygame->MosOnUnit != NULL && mygame->MosOnUnit->myteam != myteam) {
						attackmethod->Attack(this, mygame->MosOnUnit);
					}
					else if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() == tile::Mount) {
						int px = mousePos.x / SqureSize;
						int py = mousePos.y / SqureSize;
						if (myActionPoint > 0 && (x - px <= 1 && x - px >= -1) || (y - py <= 1 && y - py >= -1)) {
							mygame->maze[py][px] = 0;
							mygame->tiles[mygame->indexAt(Vector2f(mousePos))].setID(tile::Empty);
							myActionPoint--;
						}
					}
				}
			}
			//��λ��������λ�Ľ���
			if (UnitState == UState::UNITCLICK&&mygame->MosOnUnit != NULL && myActionPoint > 0) {
				if(InmyRange()&&actionPointCanAttack())
					if(mygame->MosOnUnit!=mygame->Base_blue.get())
						mygame->drawPaths= shared_ptr<Node>(new Node(MapPos(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y),true,false)));
					else
						mygame->drawPaths = shared_ptr<Node>(new Node(MapPos(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), true,true)));
				else {
					if(mygame->MosOnUnit != mygame->Base_blue.get())
						mygame->drawPaths = shared_ptr<Node>(new Node(MapPos(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), false,false)));
					else
						mygame->drawPaths = shared_ptr<Node>(new Node(MapPos(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), false, true)));
				}
			}
		}
	}
}

void Unit::setState(int state)
{
	switch (state)
	{
	case UState::UNITNORMAL:
		UnitState = UState::UNITNORMAL;
		break;
	case UState::UNITCLICK:
		UnitState = UState::UNITCLICK;
		mygame->drawPaths = NULL;
		break;
	case UState::MOVING:
		UnitState = UState::MOVING;
		break;
	default:
		break;
	}
}

void Unit::checkHover(Vector2i mousePos, Event event)
{
	if (mousePos.x > 0 && mousePos.x < width && mousePos.y > 0 && mousePos.y < height) {
		//�������Χ�ڽ���ֻ��hover����Ҫ�������ѡ�е�λ�ı���Ա��ں����Ĺ�������
		if (mousePosinMyRange(mousePos)) {
			mygame->MosOnUnit = this;
			//cout << "Team" << myteam << endl;
			//cout << "Health" << this->Health << endl;
			//cout << x << "\t" << y << endl;
		}
		else if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Unit
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Blue_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Red_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Choosen)
			mygame->MosOnUnit = NULL;
	}
	else mygame->MosOnUnit = NULL;
}

MoveableUnit::MoveableUnit(int _team, int _x, int _y, Game* _mygame)
{
	mygame = _mygame;
	UnitState = UState::UNITNORMAL;
	x = _x;
	y = _y;
	mygame->tiles[y*mygame->horizontalTiles+x].setID(tile::Unit);
	myteam = _team;
	attackmethod = NULL;
	myinfo.UnitState = UState::UNITNORMAL;
	UnitText.setFont(mygame->myfont);
	UnitText.setCharacterSize(12);
	UnitText.setFillColor(sf::Color::Black);
	UnitText.setPosition(sf::Transformable::getPosition().x, sf::Transformable::getPosition().y - 10);
}

void MoveableUnit::generatepath(Point from, Point to)
{
	astar.setMaze(mygame->maze);
	mypath = astar.GetPath(from, to,false);
	if (!mypath.empty()) mypath.pop_front();
}

bool MoveableUnit::isdead()
{

	if (Health <= 0) {
		mygame->maze[y][x] = 0;
		mygame->tiles[mygame->indexAt(getPosition())].setID(tile::Empty);
		return true;
	}
	else return false;
	//��λ����
}

void MoveableUnit::move(Point p)
{
	if (mygame->tiles[p.y*mygame->horizontalTiles+p.x].getID()==tile::Empty) {
		mygame->tiles[y * mygame->horizontalTiles + x].setID(tile::Empty);
		mygame->maze[y][x] = 0;
		y = p.y;
		x = p.x;
		mygame->maze[y][x] = 1;
		mygame->tiles[y * mygame->horizontalTiles + x].setID(tile::Unit);
		sf::Transformable::setPosition(Vector2f(Vector2i(p.x * SqureSize, p.y * SqureSize)));
		UnitText.setPosition(sf::Transformable::getPosition().x, sf::Transformable::getPosition().y - 10);
		myActionPoint--;
	}
	else {
		mypath.clear();
		mygame->running = false;
	}
}

bool MoveableUnit::guard()
{
	if (myteam == AI) {
		for (auto& u : mygame->myunits) {
			if (attackmethod->isInMyAttackRange(this, u.get())) {
				if (actionPointCanAttack()) {
					attackmethod->Attack(this, u.get());
				}
				else myActionPoint = 0;
				return true;
			}
		}
		if (attackmethod->isInMyAttackRange(this, mygame->Base_red.get())) {
			if (actionPointCanAttack()) {
				attackmethod->Attack(this, mygame->Base_red.get());
			}
			else myActionPoint = 0;
			return true;
		}
	}
	else if (myteam == PLAYER) {
		for (auto& u : mygame->enemys) {
			if (attackmethod->isInMyAttackRange(this, u.get())) {
				attackmethod->Attack(this, u.get());
				return true;
			}
		}
		if (attackmethod->isInMyAttackRange(this, mygame->Base_blue.get())) {
			attackmethod->Attack(this, mygame->Base_blue.get());
			return true;
		}
	}
	return false;
}

void MoveableUnit::generateLongestpath(Point p) {
	astar.setMaze(mygame->maze);
	mypath=astar.GetPath(Point(x, y), Point(p.x-rand()%3-1,p.y-rand()%3-1), false);
	if (!mypath.empty()) mypath.pop_front();
}

void MoveableUnit::decide()
{
	if (myActionPoint > 0) {
		if (myteam == AI) {
			if (UnitState == UState::MOVING||!mygame->running) {
				if (!mygame->running) {
					generateLongestpath(mygame->Red_baseP);
					if (!mypath.empty()) {
						UnitState = UState::MOVING;
						mygame->running = true;
					}
					else {
						myActionPoint = 0;
					}
				}
				if (UnitState == UState::MOVING&&!mypath.empty() && myActionPoint > 0) {
					if(!guard())
						move(*(mypath.front()));
					if(!mypath.empty())
						mypath.pop_front();
					if (myActionPoint <= 0) {
						mypath.clear();
					}
				}
				else {
					mypath.clear();
					UnitState = UState::UNITNORMAL;
					mygame->running = false;
				}
			}
		}
	}
	else if (this->UnitState == UState::MOVING) {
		UnitState = UState::UNITNORMAL;
		mygame->running = false;
	}
}

void MoveableUnit::updatemystate()
{
	string temp = std::to_string(Health) + "/" + std::to_string(myinfo.Health);
	UnitText.setString(temp);
	UnitText.setPosition(sf::Transformable::getPosition().x, sf::Transformable::getPosition().y - 10);
}

int MoveableUnit::myattack()
{
	return attackmethod->damage;
}

DisMoveableUnit::DisMoveableUnit(int _x, int _y, int _team, Game* _game)
{

	mygame = _game;
	x = _x;
	y = _y;
	Health = 4000;
	myteam = _team;
	indanger = false;
	cangenerate = true;
	UnitText.setFont(mygame->myfont);
	UnitText.setCharacterSize(14);
	UnitText.setString("4000/4000");
	UnitText.setFillColor(sf::Color::Green);
	UnitText.setPosition(sf::Transformable::getPosition().x+5, sf::Transformable::getPosition().y - 15);	
	setPosition(sf::Vector2f(x * SqureSize, y * SqureSize));

}

void DisMoveableUnit::checkMouse(Vector2i mousePos, Event event)
{
	//�ж�����ǲ����ڰ�ť��
	if (mousePos.x > 0 && mousePos.x < width+100 && mousePos.y > 0 && mousePos.y < height) {
		if (myteam==PLAYER&&mousePosinMyRange(mousePos)) {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				//����ڷ�Χ�ﰴ�������һ����CLICK״̬
				//���������ʱ
				if (UnitState == UState::UNITNORMAL) {
					setState(UState::UNITCLICK);
					//cout << "UNITCLICK" << endl;
				}
				//�����λ�Ѿ�ѡ�У���ôȡ��ѡ��
				else if (UnitState == UState::UNITCLICK) {
					setState(UState::UNITNORMAL);
					//cout << "UNITNORMAL" << endl;
				}
			}
			//�����������Χ�ڽ���ֻ��hover����Ҫ�������ѡ�е�λ�ı���Ա��ں����Ĺ�������
			else {
				//cout << "HOVER" << endl;
			}
		}
		else {																								//����ڰ�ť��Χ��
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				//�ڷ�Χ����������
				if (UnitState == UState::UNITCLICK) {
					if (width < mousePos.x && mousePos.x < width + 100 && mousePos.y < 400 && mousePos.y > 350) {
						generateUnit(UName::INFANTARY);
						//cout<<"INFANTARY" << endl;
					}
					else if (width < mousePos.x && mousePos.x < width + 100 && mousePos.y < 470 && mousePos.y > 420) {
						generateUnit(UName::SHOOTER);
						//cout<<"SHOOTER" << endl;
					}
					else if(width < mousePos.x && mousePos.x < width + 100 && mousePos.y < 540 && mousePos.y > 490) {
						generateUnit(UName::CAVALRY);
						//cout<<"CAVALRY" << endl;
					}
					else setState(UState::UNITNORMAL);	//�ع�NORMAL״̬	
				}
			}
		}
	}
}

void DisMoveableUnit::generateUnit(int code)
{
	if (cangenerate) {
		bool success = false;
		for (int i = x - 1; i < x + 3; i++) {
			for (int j = y - 1; j < y + 3; j++) {
				if (mygame->tiles[i + mygame->horizontalTiles * j].getID() != tile::Empty)
					continue;
				else {
					mygame->spawnUnit(myteam, code, i, j);
					success = true;
					break;
				}
			}
			if (success) break;
		}
		cangenerate = false;
	}
}

void DisMoveableUnit::reset()
{
	cangenerate = true;
}

void DisMoveableUnit::updatemystate()
{

	string temp = std::to_string(Health) + "/4000";
	UnitText.setString(temp);
	UnitText.setPosition(sf::Transformable::getPosition().x + 5, sf::Transformable::getPosition().y - 15);
	if (Health < 500) indanger = true;
	if (Health <= 0) {
		mygame->gameOver = true;
		if (myteam == PLAYER) {
			//cout << "FAIL" << endl;
			mygame->gameSceneState=SCEN_GAMEOVER;
			mygame->gameWin = false;
		}
		if (myteam == AI) {
			//cout << "SUCCESS" << endl;
			mygame->gameSceneState = SCEN_GAMEOVER;
			mygame->gameWin = true;
		}
	}
}

bool Unit::mousePosinMyRange(Vector2i mousePos)
{
	if ((mousePos.x > getPosition().x && mousePos.x < getPosition().x + getTexture()->getSize().x) &&
		(mousePos.y > getPosition().y && mousePos.y < getPosition().y + getTexture()->getSize().y))
		return true;
	else return false;
}

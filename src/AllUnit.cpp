#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Tile.h"
#include "Game.h"
#include "ArtAssets.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <iterator>

using namespace sf;
using namespace std;

namespace
{
	constexpr int SqureSize = config::TileSize;
	constexpr int width = config::MapWidth;
	constexpr int height = config::MapHeight;

}
Shooter::Shooter(int _team, int _x, int _y, Game* _mygame):MoveableUnit(_team, _x, _y, _mygame)
{
	myinfo.actionPoint = 15;
	UnitState = 0;
	unitName = UName::SHOOTER;
	myinfo.Health = 100;
	Health = 100;
	art::makeUnitTexture(mytexture, art::UnitKind::Shooter, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	setScale(0.5f, 0.5f);
	UnitText.setString("100/100");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 15;
	attackmethod = make_unique<shot>(mygame);
	myinfo.attackconsume = 1;
}

Infantry::Infantry(int _team, int _x, int _y, Game* _mygame) :MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::INFANTARY;
	Health = 200;
	art::makeUnitTexture(mytexture, art::UnitKind::Infantry, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	setScale(0.5f, 0.5f);
	UnitText.setString("200/200");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 20;
	myinfo.actionPoint = 20;
	myinfo.Health = 200;
	attackmethod = make_unique<fight>(mygame);
	myinfo.attackconsume = 2;
}

Cavalry::Cavalry(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::CAVALRY;
	Health = 500;
	art::makeUnitTexture(mytexture, art::UnitKind::Cavalry, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	setScale(0.5f, 0.5f);
	UnitText.setString("500/500");
	sf::Transformable::setPosition(_x * SqureSize, _y * SqureSize);
	myActionPoint = 40;
	myinfo.actionPoint = 40;
	myinfo.Health = 500;
	attackmethod = make_unique<roll>(mygame);
	myinfo.attackconsume = 10;
}

void MoveableUnit::Showpath(sf::Vector2f mousePos)
{
	mygame->drawPaths.clear();
	mygame->drawPaths.emplace_back(Point(x, y), tile::Path);

	bool reachable=true;
	int temp = myActionPoint;
	for (const auto& it : mypath) {
		mygame->drawPaths.emplace_back(it, tile::Path);
		temp--;
		if (temp < 0) {
			reachable = false;
			break;
		}
	}
	for (auto t = next(mygame->drawPaths.begin()); t != mygame->drawPaths.end(); ++t) {
		if (mygame->tiles[mygame->indexAt(sf::Vector2f(t->getIndex().x*SqureSize,t->getIndex().y*SqureSize))].getID() != tile::Empty) {
			reachable = false;
			break;
		}
	}
	if (mygame->tiles[mygame->indexAt(mousePos)].getID() != tile::Empty) reachable = false;
	if (!reachable) {
		mygame->drawPaths.clear();
		mygame->drawPaths.emplace_back(sf::Mouse::getPosition(mygame->window), tile::UnableToReach);
	}
}

bool MoveableUnit::isOktoAttackAndAttackconsume()
{
	if (myinfo.attackconsume <= myActionPoint) {
		myActionPoint -= myinfo.attackconsume;
		return true;
	}
	else return false;
}

void MoveableUnit::gainActionPoint(int amount)
{
	myActionPoint = std::min(myinfo.actionPoint, myActionPoint + amount);
}

void MoveableUnit::setdefalut()
{
	myActionPoint = myinfo.actionPoint;
	UnitState = UState::UNITNORMAL;
}

bool MoveableUnit::InmyRange()
{
	if (!attackmethod || mygame->MosOnUnit == nullptr) {
		return false;
	}
	return attackmethod->isInMyAttackRange(this, mygame->MosOnUnit);
}

bool MoveableUnit::isBlocked(Point p)
{
	astar.setMaze(Astar::makeEmptyMaze());
	myattackpath=astar.GetPath(Point(x, y), p,true);
	if(!myattackpath.empty())
		myattackpath.pop_front();
	for (const auto& it : myattackpath) {
		tile::ID id = mygame->tiles[it.y * mygame->horizontalTiles + it.x].getID();
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
	if (mousePos.x > 0 && mousePos.x < width && mousePos.y > 0 && mousePos.y < height) {
		if (myteam == PLAYER && mousePosinMyRange(mousePos)) {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				mygame->selectOnly(this);
			}
		}
		else {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				if (UnitState == UState::UNITCLICK) {
					mygame->clearSelection();
				}
			}
			else if (UnitState == UState::UNITCLICK&& mygame->MosOnUnit == nullptr &&event.type == Event::EventType::MouseMoved) {
				const int x = static_cast<int>(mousePos.x - (mousePos.x % SqureSize));
				const int y = static_cast<int>(mousePos.y - (mousePos.y % SqureSize));
				if (myActionPoint > 0) {
					int py = y / SqureSize;
					int px = x / SqureSize;
					if (mygame->MosOnUnit == nullptr && mygame->MousePosChanged()) {
						if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() == tile::Empty) {
							generatepath(Point(this->x, this->y), Point(px, py));
						}
					}
					Showpath(sf::Vector2f(mousePos));
				}
			}
			else if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Right) {
				if (UnitState == UState::UNITCLICK) {
					if (mygame->MosOnUnit == nullptr && mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() == tile::Empty) {
						const int px = mousePos.x / SqureSize;
						const int py = mousePos.y / SqureSize;
						// Recompute on click so movement never uses a stale hover path.
						generatepath(Point(this->x, this->y), Point(px, py));
						Showpath(sf::Vector2f(mousePos));
						if (!mypath.empty() && !mygame->drawPaths.empty() && mygame->drawPaths.front().getID() != tile::UnableToReach) {
							mygame->running = true;
							mygame->drawPaths.clear();
							UnitState = UState::MOVING;
						}
					}
					else if (mygame->MosOnUnit != nullptr && mygame->MosOnUnit->myteam != myteam) {
						attackmethod->Attack(this, mygame->MosOnUnit);
					}
					else if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() == tile::Mount) {
							int px = mousePos.x / SqureSize;
							int py = mousePos.y / SqureSize;
							if (myActionPoint > 0 && ((x - px <= 1 && x - px >= -1) || (y - py <= 1 && y - py >= -1))) {
								mygame->setTileID(px, py, tile::Empty);
								myActionPoint--;
							}
					}
				}
			}
			if (UnitState == UState::UNITCLICK&&mygame->MosOnUnit != nullptr && myActionPoint > 0) {
				mygame->drawPaths.clear();
				if(InmyRange()&&actionPointCanAttack())
					if(mygame->MosOnUnit!=mygame->Base_blue.get())
						mygame->drawPaths.emplace_back(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y),true,false);
					else
						mygame->drawPaths.emplace_back(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), true,true);
				else {
					if(mygame->MosOnUnit != mygame->Base_blue.get())
						mygame->drawPaths.emplace_back(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), false,false);
					else
						mygame->drawPaths.emplace_back(Point(mygame->MosOnUnit->x, mygame->MosOnUnit->y), false, true);
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
		mygame->drawPaths.clear();
		break;
	case UState::MOVING:
		UnitState = UState::MOVING;
		break;
	default:
		break;
	}
}

void Unit::playFlash(sf::Color color, float seconds)
{
	flashColor = color;
	flashSeconds = seconds;
	flashing = seconds > 0.f;
	flashClock.restart();
	setColor(color);
}

void Unit::playAction(sf::Vector2f direction, float seconds)
{
	const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
	if (length <= 0.f || seconds <= 0.f) {
		return;
	}
	actionDirection = sf::Vector2f(direction.x / length, direction.y / length);
	actionSeconds = seconds;
	actionAnimating = true;
	actionClock.restart();
}

sf::Vector2f Unit::actionOffset(float distance)
{
	if (!actionAnimating) {
		return sf::Vector2f(0.f, 0.f);
	}

	const float progress = actionClock.getElapsedTime().asSeconds() / actionSeconds;
	if (progress >= 1.f) {
		actionAnimating = false;
		return sf::Vector2f(0.f, 0.f);
	}

	const float wave = progress < 0.5f ? progress * 2.f : (1.f - progress) * 2.f;
	return actionDirection * (distance * wave);
}

void Unit::updateFlash()
{
	if (!flashing) {
		return;
	}

	const float progress = flashClock.getElapsedTime().asSeconds() / flashSeconds;
	if (progress >= 1.f) {
		flashing = false;
		setColor(sf::Color::White);
		return;
	}

	const auto fade = [progress](sf::Uint8 value) {
		return static_cast<sf::Uint8>(static_cast<float>(value) + (255.f - static_cast<float>(value)) * progress);
	};
	setColor(sf::Color(fade(flashColor.r), fade(flashColor.g), fade(flashColor.b), flashColor.a));
}

void Unit::checkHover(Vector2i mousePos, Event)
{
	if (mousePos.x > 0 && mousePos.x < width && mousePos.y > 0 && mousePos.y < height) {
		if (mousePosinMyRange(mousePos)) {
			mygame->MosOnUnit = this;
		}
		else if (mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Unit
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Blue_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Red_Base
			&& mygame->tiles[mygame->indexAt(Vector2f(mousePos))].getID() != tile::Choosen)
			mygame->MosOnUnit = nullptr;
	}
	else mygame->MosOnUnit = nullptr;
}

MoveableUnit::MoveableUnit(int _team, int _x, int _y, Game* _mygame)
{
	mygame = _mygame;
	UnitState = UState::UNITNORMAL;
	x = _x;
	y = _y;
	mygame->setTileID(x, y, tile::Unit);
	myteam = _team;
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
		mygame->setTileID(x, y, tile::Empty);
		return true;
	}
	else return false;
}

void MoveableUnit::move(Point p)
{
	if (mygame->tiles[p.y*mygame->horizontalTiles+p.x].getID()==tile::Empty) {
		mygame->setTileID(x, y, tile::Empty);
		y = p.y;
		x = p.x;
		mygame->setTileID(x, y, tile::Unit);
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
		if (mygame->Base_red && attackmethod->isInMyAttackRange(this, mygame->Base_red.get())) {
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
		if (mygame->Base_blue && attackmethod->isInMyAttackRange(this, mygame->Base_blue.get())) {
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
						move(mypath.front());
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
	updateFlash();
	const float t = visualClock.getElapsedTime().asSeconds();
	const float moveBob = UnitState == UState::MOVING ? std::sin(t * 13.f + static_cast<float>(x + y)) * 2.2f : 0.f;
	const float selectedPulse = UnitState == UState::UNITCLICK ? 1.f + std::sin(t * 6.f) * 0.045f : 1.f;
	const sf::Vector2f offset = actionOffset(3.5f);
	setScale(0.5f * selectedPulse, 0.5f * selectedPulse);
	sf::Transformable::setPosition(x * SqureSize + offset.x, y * SqureSize + moveBob + offset.y);
	string temp = std::to_string(Health) + "/" + std::to_string(myinfo.Health);
	UnitText.setString(temp);
	UnitText.setPosition(sf::Transformable::getPosition().x, sf::Transformable::getPosition().y - 10);
}

int MoveableUnit::myattack()
{
	if (!attackmethod) {
		return 0;
	}
	return attackmethod->damage;
}

DisMoveableUnit::DisMoveableUnit(int _x, int _y, int _team, Game* _game)
{

	mygame = _game;
	x = _x;
	y = _y;
	Health = 4000;
	myteam = _team;
	unitName = UName::BASE;
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
	if (mousePos.x > 0 && mousePos.x < config::WindowWidth && mousePos.y > 0 && mousePos.y < height) {
		if (myteam==PLAYER&&mousePosinMyRange(mousePos)) {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				mygame->selectOnly(this);
			}
		}
		else {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
					if (UnitState == UState::UNITCLICK) {
						if (mousePos.x < config::PanelX)
							mygame->clearSelection();
					}
			}
		}
	}
}

bool DisMoveableUnit::generateUnit(int code)
{
	if (cangenerate) {
		if (!mygame->canSpawnUnit(myteam, code)) {
			if (myteam == PLAYER) {
				mygame->addFloatingText(sf::Vector2f(x * SqureSize + SqureSize, y * SqureSize - 8.f),
					"Need CMD", sf::Color(255, 214, 96), 12);
			}
			return false;
		}
		bool success = false;
		for (int i = x - 1; i < x + 3; i++) {
			for (int j = y - 1; j < y + 3; j++) {
				if (!mygame->isMapCell(i, j))
					continue;
				if (mygame->tiles[i + mygame->horizontalTiles * j].getID() != tile::Empty)
					continue;
				else {
					success = mygame->spawnUnit(myteam, code, i, j);
					break;
				}
			}
			if (success) break;
		}
		if (success) {
			cangenerate = false;
		}
		return success;
	}
	return false;
}

void DisMoveableUnit::reset()
{
	cangenerate = true;
}

void DisMoveableUnit::updatemystate()
{
	updateFlash();
	const sf::Vector2f offset = actionOffset(2.f);
	setPosition(sf::Vector2f(x * SqureSize + offset.x, y * SqureSize + offset.y));

	string temp = std::to_string(Health) + "/4000";
	UnitText.setString(temp);
	UnitText.setPosition(sf::Transformable::getPosition().x + 5, sf::Transformable::getPosition().y - 15);
	if (Health < 500) indanger = true;
	if (Health <= 0) {
		mygame->gameOver = true;
		if (myteam == PLAYER) {
			mygame->gameSceneState=SCEN_GAMEOVER;
			mygame->gameWin = false;
		}
		if (myteam == AI) {
			mygame->gameSceneState = SCEN_GAMEOVER;
			mygame->gameWin = true;
		}
	}
}

bool Unit::mousePosinMyRange(Vector2i mousePos)
{
	return getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
}

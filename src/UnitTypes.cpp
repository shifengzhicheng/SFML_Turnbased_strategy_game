#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Tile.h"
#include "Game.h"
#include "UnitDefinition.h"
#include "UnitVisualHelpers.h"

#include <string>

using namespace sf;
using namespace std;
using namespace unit_visual;

Shooter::Shooter(int _team, int _x, int _y, Game* _mygame):MoveableUnit(_team, _x, _y, _mygame)
{
	configureFromDefinition(unitDefinition(UName::SHOOTER));
}

Infantry::Infantry(int _team, int _x, int _y, Game* _mygame) :MoveableUnit(_team, _x, _y, _mygame)
{
	configureFromDefinition(unitDefinition(UName::INFANTARY));
}

Cavalry::Cavalry(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	configureFromDefinition(unitDefinition(UName::CAVALRY));
}

Siege::Siege(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	configureFromDefinition(unitDefinition(UName::SIEGE));
}

Guardian::Guardian(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	configureFromDefinition(unitDefinition(UName::GUARDIAN));
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
	UnitText.setFont(mygame->myfont);
	UnitText.setCharacterSize(14);
	UnitText.setString("4000/4000");
	UnitText.setFillColor(sf::Color::Green);
	UnitText.setPosition(sf::Transformable::getPosition().x+5, sf::Transformable::getPosition().y - 15);	
	setPosition(sf::Vector2f(x * TileSize, y * TileSize));

}

void DisMoveableUnit::checkMouse(Vector2i mousePos, Event event)
{
	if (mousePos.x > 0 && mousePos.x < config::WindowWidth && mousePos.y > 0 && mousePos.y < MapHeight) {
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

void DisMoveableUnit::updatemystate()
{
	updateFlash();
	const sf::Vector2f offset = actionOffset(2.f);
	setPosition(sf::Vector2f(x * TileSize + offset.x, y * TileSize + offset.y));

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

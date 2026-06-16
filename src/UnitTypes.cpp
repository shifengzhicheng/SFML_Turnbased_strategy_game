#include "AllUnit.h"
#include "Action.h"
#include "Config.h"
#include "Tile.h"
#include "Game.h"
#include "ArtAssets.h"
#include "RealtimeConfig.h"
#include "UnitVisualHelpers.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace sf;
using namespace std;
using namespace unit_visual;

Shooter::Shooter(int _team, int _x, int _y, Game* _mygame):MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::SHOOTER;
	myinfo.Health = 100;
	Health = 100;
	art::makeUnitTexture(mytexture, art::UnitKind::Shooter, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	UnitText.setString("100/100");
	placeUnitSprite(*this, _x, _y, config::UnitSpriteScale);
	placeHealthLabel(UnitText, _x, _y);
	attackmethod = make_unique<shot>(mygame);
}

Infantry::Infantry(int _team, int _x, int _y, Game* _mygame) :MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::INFANTARY;
	Health = 220;
	art::makeUnitTexture(mytexture, art::UnitKind::Infantry, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	UnitText.setString("220/220");
	placeUnitSprite(*this, _x, _y, config::UnitSpriteScale);
	placeHealthLabel(UnitText, _x, _y);
	myinfo.Health = 220;
	attackmethod = make_unique<fight>(mygame);
}

Cavalry::Cavalry(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::CAVALRY;
	Health = 420;
	art::makeUnitTexture(mytexture, art::UnitKind::Cavalry, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	UnitText.setString("420/420");
	placeUnitSprite(*this, _x, _y, config::UnitSpriteScale);
	placeHealthLabel(UnitText, _x, _y);
	myinfo.Health = 420;
	attackmethod = make_unique<roll>(mygame);
}

Siege::Siege(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::SIEGE;
	Health = 270;
	art::makeUnitTexture(mytexture, art::UnitKind::Siege, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	UnitText.setString("270/270");
	placeUnitSprite(*this, _x, _y, config::UnitSpriteScale);
	placeHealthLabel(UnitText, _x, _y);
	myinfo.Health = 270;
	attackmethod = make_unique<bombard>(mygame);
}

Guardian::Guardian(int _team, int _x, int _y, Game* _mygame) : MoveableUnit(_team, _x, _y, _mygame)
{
	UnitState = 0;
	unitName = UName::GUARDIAN;
	Health = 720;
	art::makeUnitTexture(mytexture, art::UnitKind::Guardian, myteam == PLAYER ? art::Team::Player : art::Team::Enemy);
	setTexture(mytexture);
	UnitText.setString("720/720");
	placeUnitSprite(*this, _x, _y, config::UnitSpriteScale);
	placeHealthLabel(UnitText, _x, _y);
	myinfo.Health = 720;
	attackmethod = make_unique<crush>(mygame);
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

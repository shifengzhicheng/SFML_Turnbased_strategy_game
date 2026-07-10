#pragma once
#include <SFML/Graphics.hpp>
#include "Config.h"

enum BtnState {
	NORMAL, HOVER, CLICK, RELEASE
};

class Button :public sf::Sprite {
public:

	int btnState = NORMAL;

	sf::Texture tNormal;
	sf::Texture tHover;
	sf::Texture tClick;
	int checkMouse(sf::Vector2i, sf::Event);
	void setTextures(const sf::Texture&, const sf::Texture&);
	void setTextures(const sf::Texture&, const sf::Texture&, const sf::Texture&);
	void setState(int state);
	void offset(double _x, double _y) {
			setPosition(getPosition().x + _x, getPosition().y + _y);
		}

private:
	bool pressedInside = false;
};

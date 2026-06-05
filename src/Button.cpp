#include "Button.h"
#include <iostream>
using namespace sf;
using namespace std;

void Button::setTextures(const Texture& _tNormal, const Texture& _tClick) {
	tNormal = _tNormal;
	tClick = _tClick;
	setTexture(tNormal, true);
	btnState = NORMAL;
}
void Button::setTextures(const Texture& _tNormal, const Texture& _tHover, const Texture& _tClick) {
	tNormal = _tNormal;
	tHover = _tHover;
	tClick = _tClick;
	setTexture(tNormal, true);
	btnState = NORMAL;
}
void Button::setState(int state)
{
	btnState = state;
	switch (btnState) {
	case NORMAL:
		setTexture(tNormal, true); break;
	case HOVER:
		setTexture(tHover, true); break;
	case CLICK:
		setTexture(tClick, true); break;
	case RELEASE:
		setTexture(tNormal, true); break;
	default:
		break;
	}
}

int Button::checkMouse(Vector2i mouse, Event event) {
	if (getTexture() == nullptr) {
		return btnState;
	}
	if (mouse.x >= 0 && mouse.x <= config::WindowWidth && mouse.y >= 0 && mouse.y <= config::WindowHeight) {
		if (getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y))) {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				if (btnState == NORMAL) {
					setState(CLICK);
				}
				else if (btnState == RELEASE) {
					setState(NORMAL);
				}
			}
			else if(event.type == Event::EventType::MouseButtonReleased && event.mouseButton.button == Mouse::Left)
			{
				setState(RELEASE);
			}
			else {
				if (btnState != CLICK) {
					setState(HOVER);
				}
			}
		}
		else {
			if (event.type == Event::EventType::MouseButtonReleased && event.mouseButton.button == Mouse::Left) {
				setState(NORMAL);
			}
			else if (btnState == HOVER) {
				setState(NORMAL);
			}
		}
	}
	return btnState;
}

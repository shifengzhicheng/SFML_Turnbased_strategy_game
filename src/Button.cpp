#include "Button.h"
#include <iostream>
using namespace sf;
using namespace std;

void Button::setTextures(const Texture& _tNormal, const Texture& _tClick) {
	tNormal = _tNormal;
	tClick = _tClick;
	setTexture(tNormal, true);
	btnState = NORMAL;
	pressedInside = false;
}
void Button::setTextures(const Texture& _tNormal, const Texture& _tHover, const Texture& _tClick) {
	tNormal = _tNormal;
	tHover = _tHover;
	tClick = _tClick;
	setTexture(tNormal, true);
	btnState = NORMAL;
	pressedInside = false;
}
void Button::setState(int state)
{
	if (btnState == state) {
		return;
	}
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
	const bool inCanvas = mouse.x >= 0 && mouse.x <= config::WindowWidth
		&& mouse.y >= 0 && mouse.y <= config::WindowHeight;
	const bool inside = inCanvas
		&& getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

	if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
		pressedInside = inside;
		setState(inside ? CLICK : NORMAL);
	}
	else if (event.type == Event::MouseButtonReleased && event.mouseButton.button == Mouse::Left) {
		const bool activate = pressedInside && inside;
		pressedInside = false;
		setState(activate ? RELEASE : (inside ? HOVER : NORMAL));
	}
	else if (!pressedInside) {
		setState(inside ? HOVER : NORMAL);
	}
	return btnState;
}

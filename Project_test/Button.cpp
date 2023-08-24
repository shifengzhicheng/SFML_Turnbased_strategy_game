#include "Button.h"
#include <iostream>
using namespace std;

void Button::setTextures(Texture _tNormal, Texture _tClick) {
	tNormal = _tNormal;
	tClick = _tClick;
	setTexture(tNormal);		//Ĭ�ϼ�����ͨ����
}
void Button::setTextures(Texture _tNormal, Texture _tHover, Texture _tClick) {
	tNormal = _tNormal;
	tHover = _tHover;
	tClick = _tClick;
	setTexture(tNormal);		//Ĭ�ϼ�����ͨ����	
}
void Button::setState(int state)
{
	btnState = state;
	switch (btnState) {
	case NORMAL:
		setTexture(tNormal); break;
	case HOVER:
		setTexture(tHover); break;
	case CLICK:
		setTexture(tClick); break;
	case RELEASE:
		setTexture(tNormal); break;
	default:
		break;
	}
}

int Button::checkMouse(Vector2i mouse, Event event) {
	//�ж�����ǲ����ڰ�ť��
	if (mouse.x >= 0 && mouse.x <= width+100 && mouse.y >= 0 && mouse.y <= height) {
		if ((mouse.x > getPosition().x && mouse.x < getPosition().x + getTexture()->getSize().x) &&
			(mouse.y > getPosition().y && mouse.y < getPosition().y + getTexture()->getSize().y)) {
			if (event.type == Event::EventType::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
				//����ڷ�Χ�ﰴ�������һ����CLICK״̬
				//���������ʱ
				if (btnState == NORMAL) {
					setState(CLICK);
					//cout << "CLICK" << endl;
				}
				else if (btnState == RELEASE) {
					setState(NORMAL);
				}
			}
			else if(event.type == Event::EventType::MouseButtonReleased && event.mouseButton.button == Mouse::Left)
			{
				setState(RELEASE);
				//cout << "RELEASE" << endl;
			}
			else {																										//�������ƶ��Ļ�������ǲ��ǰ��ŵģ���ʾHOVER״̬
				if (btnState != CLICK) {
					setState(HOVER);
					//cout << "HOVER" << endl;
				}
			}
		}
		else {																											//����ڰ�ť��Χ��
			if (event.type == Event::EventType::MouseButtonReleased && event.mouseButton.button == Mouse::Left) {			//�ڷ�Χ���ͷ�������
				setState(NORMAL);																							//�ع�NORMAL״̬
				//cout << "NOMRAL" << endl;
			}
			else if (btnState == HOVER) {																					//�����HOVER��Ҳ����Ҳû�а��¹����ع�NORMAL״̬
				setState(NORMAL);																							//�����ͱ���ԭ�� ���簴ס���ŵ�ʱ��
				//cout << "HOVER to NOMRAL" << endl;
			}
		}
	}
	return btnState;
}

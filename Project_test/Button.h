#pragma once
#include <SFML/Graphics.hpp>
using namespace sf;

enum BtnState {
	NORMAL, HOVER, CLICK, RELEASE
};
class Button :public Sprite { 	
	//�̳�SFML��Sprite��
	int width = 1280;
	int height = 720;
public:

	int btnState;//��ť״̬

	Texture tNormal;			//���ֲ�ͬ״̬������
	Texture tHover;
	Texture tClick;
	int checkMouse(Vector2i, Event);					//������״̬
	void setTextures(Texture, Texture);					//�������������״̬
	void setTextures(Texture, Texture, Texture);		//�������������״̬
	void setState(int state);//��ť״̬
	void offset(double _x, double _y) {
		setPosition(getPosition().x + _x, getPosition().y + _y);
	}
};

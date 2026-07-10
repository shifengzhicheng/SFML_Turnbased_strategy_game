#include "Button.h"

#include <SFML/Graphics.hpp>

#include <cstdlib>
#include <iostream>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    sf::Event mouseEvent(sf::Event::EventType type, int x, int y)
    {
        sf::Event event{};
        event.type = type;
        event.mouseButton.button = sf::Mouse::Left;
        event.mouseButton.x = x;
        event.mouseButton.y = y;
        return event;
    }
}

int main()
{
    sf::Image image;
    image.create(40, 20, sf::Color::White);
    sf::Texture normal;
    sf::Texture hover;
    sf::Texture pressed;
    require(normal.loadFromImage(image) && hover.loadFromImage(image) && pressed.loadFromImage(image),
            "button test textures should load");

    Button button;
    button.setTextures(normal, hover, pressed);
    button.setPosition(100.f, 100.f);

    sf::Event moved{};
    moved.type = sf::Event::MouseMoved;
    require(button.checkMouse({110, 110}, moved) == HOVER,
            "moving inside should hover the button");
    require(button.checkMouse({110, 110}, mouseEvent(sf::Event::MouseButtonPressed, 110, 110)) == CLICK,
            "pressing a hovered button should enter pressed state");
    require(button.checkMouse({20, 20}, mouseEvent(sf::Event::MouseButtonReleased, 20, 20)) != RELEASE,
            "releasing outside should cancel activation");

    button.checkMouse({20, 20}, mouseEvent(sf::Event::MouseButtonPressed, 20, 20));
    require(button.checkMouse({110, 110}, mouseEvent(sf::Event::MouseButtonReleased, 110, 110)) != RELEASE,
            "a press that started outside must not activate after moving inside");

    button.checkMouse({110, 110}, mouseEvent(sf::Event::MouseButtonPressed, 110, 110));
    require(button.checkMouse({110, 110}, mouseEvent(sf::Event::MouseButtonReleased, 110, 110)) == RELEASE,
            "pressing and releasing inside should activate exactly once");
    return 0;
}

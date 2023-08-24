#pragma once
#include "Point.h"
#include <SFML/Graphics.hpp>

namespace tile
{
    enum ID
    {
        Red_Base, Blue_Base, Path, Empty, Tree, Choosen, River, Mount,Unit,UnableToReach
    };
}

class MapPos: public sf::Sprite
{
private:
    int x;
    int y;

    tile::ID id;
    // 不同id对应不同颜色
    static sf::Color IDtoColor(tile::ID id);

public:

    MapPos() {}

    sf::RectangleShape rect;
    explicit MapPos(sf::IntRect intrect, tile::ID id = tile::Empty);
    MapPos(Point p, bool code,bool isbase);
    MapPos(Point, tile::ID id = tile::Empty);
    MapPos(sf::Vector2i mousePos, tile::ID id);
    sf::Vector2i getIndex();
    void setID(tile::ID id);
    tile::ID getID() const;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

#pragma once
#include "Point.h"
#include <SFML/Graphics.hpp>

namespace tile
{
    enum ID
    {
        Red_Base, Blue_Base, Path, Empty, Tree, Choosen, River, Mount, Unit, UnableToReach,
        Resource, Player_Extractor, Enemy_Extractor, Player_Barracks, Enemy_Barracks
    };
}

class MapPos: public sf::Sprite
{
private:
    int x = 0;
    int y = 0;

    tile::ID id = tile::Empty;
    // Each tile id maps to a display color.
    static sf::Color IDtoColor(tile::ID id);

public:

    MapPos() = default;

    sf::RectangleShape rect;
    explicit MapPos(sf::IntRect intrect, tile::ID id = tile::Empty);
    MapPos(Point p, bool code,bool isbase);
    MapPos(Point, tile::ID id = tile::Empty);
    MapPos(sf::Vector2i mousePos, tile::ID id);
    sf::Vector2i getIndex() const;
    void setID(tile::ID id);
    tile::ID getID() const;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

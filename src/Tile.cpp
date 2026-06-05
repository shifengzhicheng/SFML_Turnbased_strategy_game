#include "Tile.h"
#include "Config.h"

namespace
{
    constexpr int SqureSize = config::TileSize;
}
// Initialize a tile from a rectangle and optional tile id.
MapPos::MapPos(sf::IntRect intrect, tile::ID ID):
                id(ID),
                rect(sf::Vector2f(intrect.width, intrect.height))
{
    rect.setFillColor(IDtoColor(id));
    rect.setPosition(intrect.left, intrect.top);

    rect.setOutlineColor(sf::Color(160, 160, 160));
    rect.setOutlineThickness(0.6f);

    x = intrect.left / intrect.width;
    y = intrect.top / intrect.height;
}
MapPos::MapPos(Point p, bool code, bool isBase):
    id(code ? tile::Choosen : tile::UnableToReach)
{
    x = p.x;
    y = p.y;
    if (isBase)
        rect.setSize(sf::Vector2f(2 * SqureSize, 2 * SqureSize));
    else
        rect.setSize(sf::Vector2f(SqureSize, SqureSize));
    rect.setPosition(x * SqureSize, y * SqureSize);
    if (code)
        rect.setOutlineColor(sf::Color(127, 255, 0));
    else
        rect.setOutlineColor(sf::Color(210, 105, 30));
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(3.f);
    
}
MapPos::MapPos(Point p, tile::ID id)
{
    x = p.x;
    y = p.y;
    this->id = id; 
    rect.setSize(sf::Vector2f(SqureSize, SqureSize));
    rect.setFillColor(IDtoColor(id));
    rect.setPosition(x*SqureSize, y*SqureSize);
    rect.setOutlineColor(sf::Color(160, 160, 160));
    rect.setOutlineThickness(0.6f);
}

MapPos::MapPos(sf::Vector2i mousePos, tile::ID id)
{
    const int x = static_cast<int>(mousePos.x - (mousePos.x % SqureSize));
    const int y = static_cast<int>(mousePos.y - (mousePos.y % SqureSize));
    this->x = x / SqureSize;
    this->y = y / SqureSize;
    this->id = id;
    rect.setSize(sf::Vector2f(SqureSize, SqureSize));
    rect.setFillColor(IDtoColor(id));
    rect.setPosition(x , y);
    rect.setOutlineColor(sf::Color(160, 160, 160));
    rect.setOutlineThickness(0.6f);
}
// Draw the tile rectangle.
void MapPos::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(rect, states);
}

// Return the tile index in the map grid.
sf::Vector2i MapPos::getIndex() const
{
    return sf::Vector2i(x, y);
}

void MapPos::setID(tile::ID id)
{
    this->id = id;
    rect.setFillColor(IDtoColor(id));
}

tile::ID MapPos::getID() const
{
    return id;
}

// Map each tile id to a display color.
sf::Color MapPos::IDtoColor(tile::ID id)
{
    switch (id)
    {
    case tile::Unit:
        return sf::Color::Yellow;
        break;
    case tile::Empty:
        return sf::Color::White;
        break;
    case tile::Path:
        return sf::Color(152,251,152);
        break;
    case tile::Red_Base:
        return sf::Color(64 ,224 ,208);
        break;
    case tile::Blue_Base:
        return sf::Color(0, 191, 255);
        break;
    case tile::Choosen:
        return sf::Color::Green;
        break;
    case tile::River:
        return sf::Color(30, 144, 255);
        break;
    case tile::Mount:
        return sf::Color(119, 136, 153);
        break;
    case tile::Tree:
        return sf::Color(0, 100, 0);
        break;
    case tile::UnableToReach:
        return sf::Color(255,106,106);
    default:
        return sf::Color::Black;
        break;
    }
}

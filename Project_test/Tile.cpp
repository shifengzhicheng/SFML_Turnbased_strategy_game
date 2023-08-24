#include "Tile.h"
#define SqureSize 20
const int width = 1280;
const int height = 720;
// 初始化，默认ID：empty，需传入IntRect变量
MapPos::MapPos(sf::IntRect intrect, tile::ID ID):
                rect(sf::Vector2f(intrect.width, intrect.height)),
                id(ID)
{
    rect.setFillColor(IDtoColor(id));
    rect.setPosition(intrect.left, intrect.top);

    rect.setOutlineColor(sf::Color(160, 160, 160));
    rect.setOutlineThickness(0.6f);

    x = intrect.left / intrect.width;
    y = intrect.top / intrect.height;
}
MapPos::MapPos(Point p, bool code, bool isBase) {
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
// 绘制图形
void MapPos::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(rect, states);
}

// 返回 tile 在二维矩阵中的索引，
sf::Vector2i MapPos::getIndex()
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

// 将标签转为对应的颜色
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
        return sf::Color::Color(152,251,152);
        break;
    case tile::Red_Base:
        return sf::Color::Color(64 ,224 ,208);
        break;
    case tile::Blue_Base:
        return sf::Color::Color(0, 191, 255);
        break;
    case tile::Choosen:
        return sf::Color::Green;
        break;
    case tile::River:
        return sf::Color::Color(30, 144, 255);
        break;
    case tile::Mount:
        return sf::Color::Color(119, 136, 153);
        break;
    case tile::Tree:
        return sf::Color::Color(0, 100, 0);
        break;
    case tile::UnableToReach:
        return sf::Color::Color(255,106,106);
    default:
        return sf::Color::Black;
        break;
    }
}

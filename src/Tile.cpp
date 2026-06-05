#include "Tile.h"
#include "Config.h"

namespace
{
    constexpr int SqureSize = config::TileSize;

    sf::Color withAlpha(sf::Color color, sf::Uint8 alpha)
    {
        color.a = alpha;
        return color;
    }

    void drawMount(sf::RenderTarget& target, sf::Vector2f origin)
    {
        sf::ConvexShape peak(3);
        peak.setPoint(0, origin + sf::Vector2f(4.f, 17.f));
        peak.setPoint(1, origin + sf::Vector2f(11.f, 4.f));
        peak.setPoint(2, origin + sf::Vector2f(18.f, 17.f));
        peak.setFillColor(sf::Color(117, 122, 122));
        peak.setOutlineColor(sf::Color(72, 76, 76));
        peak.setOutlineThickness(0.8f);
        target.draw(peak);

        sf::ConvexShape snow(3);
        snow.setPoint(0, origin + sf::Vector2f(8.f, 10.f));
        snow.setPoint(1, origin + sf::Vector2f(11.f, 4.f));
        snow.setPoint(2, origin + sf::Vector2f(14.f, 10.f));
        snow.setFillColor(sf::Color(232, 234, 223));
        target.draw(snow);
    }

    void drawTree(sf::RenderTarget& target, sf::Vector2f origin)
    {
        sf::RectangleShape trunk(sf::Vector2f(3.f, 7.f));
        trunk.setPosition(origin + sf::Vector2f(9.f, 12.f));
        trunk.setFillColor(sf::Color(91, 60, 35));
        target.draw(trunk);

        for (sf::Vector2f offset : {sf::Vector2f(5.f, 8.f), sf::Vector2f(9.f, 5.f), sf::Vector2f(12.f, 9.f)}) {
            sf::CircleShape leaf(4.5f, 18);
            leaf.setPosition(origin + offset);
            leaf.setFillColor(sf::Color(47, 117, 67));
            leaf.setOutlineColor(sf::Color(28, 80, 46));
            leaf.setOutlineThickness(0.5f);
            target.draw(leaf);
        }
    }

    void drawRiver(sf::RenderTarget& target, sf::Vector2f origin)
    {
        sf::RectangleShape band(sf::Vector2f(SqureSize, 7.f));
        band.setPosition(origin + sf::Vector2f(0.f, 7.f));
        band.setFillColor(sf::Color(69, 157, 198, 190));
        target.draw(band);

        sf::Vertex wave[] = {
            sf::Vertex(origin + sf::Vector2f(2.f, 10.f), sf::Color(217, 245, 255, 170)),
            sf::Vertex(origin + sf::Vector2f(8.f, 8.f), sf::Color(217, 245, 255, 170)),
            sf::Vertex(origin + sf::Vector2f(14.f, 12.f), sf::Color(217, 245, 255, 170)),
            sf::Vertex(origin + sf::Vector2f(20.f, 9.f), sf::Color(217, 245, 255, 170))
        };
        target.draw(wave, 4, sf::LineStrip);
    }

    void drawBaseTile(sf::RenderTarget& target, sf::Vector2f origin, sf::Color color)
    {
        sf::RectangleShape plate(sf::Vector2f(SqureSize - 4.f, SqureSize - 4.f));
        plate.setPosition(origin + sf::Vector2f(2.f, 2.f));
        plate.setFillColor(withAlpha(color, 85));
        plate.setOutlineColor(withAlpha(color, 190));
        plate.setOutlineThickness(1.2f);
        target.draw(plate);
    }
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

    const sf::Vector2f origin = rect.getPosition();
    switch (id) {
    case tile::Mount:
        drawMount(target, origin);
        break;
    case tile::Tree:
        drawTree(target, origin);
        break;
    case tile::River:
        drawRiver(target, origin);
        break;
    case tile::Red_Base:
        drawBaseTile(target, origin, sf::Color(218, 76, 60));
        break;
    case tile::Blue_Base:
        drawBaseTile(target, origin, sf::Color(61, 128, 206));
        break;
    case tile::Path: {
        sf::CircleShape dot(4.f, 18);
        dot.setOrigin(4.f, 4.f);
        dot.setPosition(origin + sf::Vector2f(10.f, 10.f));
        dot.setFillColor(sf::Color(95, 180, 95, 170));
        target.draw(dot);
        break;
    }
    case tile::UnableToReach: {
        sf::RectangleShape slash(sf::Vector2f(18.f, 2.f));
        slash.setOrigin(9.f, 1.f);
        slash.setPosition(origin + sf::Vector2f(10.f, 10.f));
        slash.setRotation(45.f);
        slash.setFillColor(sf::Color(207, 68, 48, 210));
        target.draw(slash);
        slash.setRotation(-45.f);
        target.draw(slash);
        break;
    }
    default:
        break;
    }
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
        return sf::Color(255, 228, 111, 120);
    case tile::Empty:
        return sf::Color(212, 224, 170);
    case tile::Path:
        return sf::Color(159, 222, 139, 95);
    case tile::Red_Base:
        return sf::Color(232, 176, 138);
    case tile::Blue_Base:
        return sf::Color(159, 190, 225);
    case tile::Choosen:
        return sf::Color(95, 202, 100, 95);
    case tile::River:
        return sf::Color(121, 187, 208);
    case tile::Mount:
        return sf::Color(171, 169, 151);
    case tile::Tree:
        return sf::Color(115, 170, 98);
    case tile::UnableToReach:
        return sf::Color(255,106,106);
    default:
        return sf::Color::Black;
        break;
    }
}

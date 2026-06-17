#include "Tile.h"
#include "Config.h"

namespace
{
    constexpr int SqureSize = config::TileSize;

    void drawPixelRect(sf::RenderTarget& target, sf::Vector2f origin,
                       float x, float y, float w, float h, sf::Color color)
    {
        sf::RectangleShape rect(sf::Vector2f(w, h));
        rect.setPosition(origin + sf::Vector2f(x, y));
        rect.setFillColor(color);
        target.draw(rect);
    }

    void drawPixelFrame(sf::RenderTarget& target, sf::Vector2f origin, sf::Color light, sf::Color dark)
    {
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, 1.f, light);
        drawPixelRect(target, origin, 0.f, 0.f, 1.f, SqureSize, light);
        drawPixelRect(target, origin, 0.f, SqureSize - 1.f, SqureSize, 1.f, dark);
        drawPixelRect(target, origin, SqureSize - 1.f, 0.f, 1.f, SqureSize, dark);
    }

    void drawGrassDetail(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const bool warm = (x * 19 + y * 7) % 3 == 0;
        const sf::Color blade = warm ? sf::Color(230, 228, 156, 82) : sf::Color(92, 142, 78, 72);
        const int px = 3 + (x * 5 + y * 3) % 12;
        const int py = 5 + (x * 7 + y * 2) % 10;
        drawPixelRect(target, origin, static_cast<float>(px), static_cast<float>(py), 5.f, 1.f, blade);
        drawPixelRect(target, origin, static_cast<float>(px + 1), static_cast<float>(py - 1), 1.f, 1.f, blade);
        if ((x * 11 + y * 17) % 5 == 0) {
            drawPixelRect(target, origin, 14.f, 14.f, 2.f, 2.f, sf::Color(91, 117, 75, 64));
        }
    }

    void drawPathGround(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(169, 191, 126));
        drawPixelRect(target, origin, 2.f, 8.f, 16.f, 5.f, sf::Color(132, 165, 104, 132));
        drawPixelRect(target, origin, 5.f + static_cast<float>((x + y) % 5), 9.f, 4.f, 2.f, sf::Color(220, 221, 156, 90));
        drawPixelFrame(target, origin, sf::Color(223, 232, 169, 52), sf::Color(78, 105, 70, 64));
    }

    void drawRiverGround(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(57, 132, 178));
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, 4.f, sf::Color(47, 108, 158));
        drawPixelRect(target, origin, 0.f, 14.f, SqureSize, 6.f, sf::Color(41, 96, 145));
        const int wave = (x * 3 + y * 5) % 8;
        drawPixelRect(target, origin, static_cast<float>(wave), 7.f, 8.f, 2.f, sf::Color(173, 226, 238, 118));
        drawPixelRect(target, origin, static_cast<float>((wave + 9) % 16), 12.f, 6.f, 1.f, sf::Color(206, 245, 249, 100));
    }

    void drawTree(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const sf::Color outline(27, 53, 35);
        const sf::Color trunk(102, 67, 39);
        const sf::Color leaf = (x + y) % 2 == 0 ? sf::Color(44, 119, 62) : sf::Color(54, 137, 71);
        const sf::Color leafLight(94, 171, 82);

        drawPixelRect(target, origin, 4.f, 17.f, 13.f, 3.f, sf::Color(10, 18, 13, 64));
        drawPixelRect(target, origin, 8.f, 8.f, 4.f, 12.f, outline);
        drawPixelRect(target, origin, 9.f, 9.f, 2.f, 11.f, trunk);
        drawPixelRect(target, origin, 4.f, 2.f, 13.f, 7.f, outline);
        drawPixelRect(target, origin, 2.f, 7.f, 17.f, 8.f, outline);
        drawPixelRect(target, origin, 5.f, -3.f, 10.f, 7.f, outline);
        drawPixelRect(target, origin, 5.f, 3.f, 11.f, 5.f, leaf);
        drawPixelRect(target, origin, 3.f, 8.f, 15.f, 6.f, leaf);
        drawPixelRect(target, origin, 6.f, -2.f, 8.f, 5.f, leaf);
        drawPixelRect(target, origin, 8.f, 1.f, 5.f, 2.f, leafLight);
        drawPixelRect(target, origin, 5.f, 9.f, 4.f, 2.f, leafLight);
    }

    void drawMount(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const bool warm = (x + y) % 2 == 0;
        const sf::Color rock = warm ? sf::Color(124, 127, 118) : sf::Color(112, 119, 119);
        const sf::Color rockDark(65, 70, 68);
        const sf::Color rockLight(170, 169, 148);
        const sf::Color snow(230, 233, 218);

        drawPixelRect(target, origin, 2.f, 17.f, 16.f, 3.f, sf::Color(12, 16, 15, 68));
        sf::ConvexShape peak(5);
        peak.setPoint(0, origin + sf::Vector2f(1.f, 18.f));
        peak.setPoint(1, origin + sf::Vector2f(6.f, 7.f));
        peak.setPoint(2, origin + sf::Vector2f(10.f, -3.f));
        peak.setPoint(3, origin + sf::Vector2f(15.f, 8.f));
        peak.setPoint(4, origin + sf::Vector2f(19.f, 18.f));
        peak.setFillColor(rockDark);
        target.draw(peak);
        sf::ConvexShape face(4);
        face.setPoint(0, origin + sf::Vector2f(3.f, 17.f));
        face.setPoint(1, origin + sf::Vector2f(10.f, -1.f));
        face.setPoint(2, origin + sf::Vector2f(11.f, 17.f));
        face.setPoint(3, origin + sf::Vector2f(8.f, 19.f));
        face.setFillColor(rock);
        target.draw(face);
        sf::ConvexShape lit(4);
        lit.setPoint(0, origin + sf::Vector2f(10.f, -1.f));
        lit.setPoint(1, origin + sf::Vector2f(17.f, 17.f));
        lit.setPoint(2, origin + sf::Vector2f(11.f, 17.f));
        lit.setPoint(3, origin + sf::Vector2f(10.f, 8.f));
        lit.setFillColor(rockLight);
        target.draw(lit);
        drawPixelRect(target, origin, 8.f, 1.f, 5.f, 4.f, snow);
        drawPixelRect(target, origin, 9.f, 5.f, 3.f, 2.f, snow);
    }

    void drawBuildingTile(sf::RenderTarget& target, sf::Vector2f origin, sf::Color color, bool barracks, bool tower)
    {
        const sf::Color outline(45, 38, 31);
        const sf::Color stone(172, 158, 119);
        const sf::Color roof = barracks ? sf::Color(210, 136, 65) : sf::Color(205, 193, 145);

        drawPixelRect(target, origin, 2.f, 17.f, 17.f, 3.f, sf::Color(10, 14, 12, 76));
        if (tower) {
            drawPixelRect(target, origin, 6.f, 2.f, 9.f, 17.f, outline);
            drawPixelRect(target, origin, 7.f, 3.f, 7.f, 15.f, stone);
            drawPixelRect(target, origin, 5.f, -3.f, 11.f, 6.f, outline);
            drawPixelRect(target, origin, 6.f, -2.f, 9.f, 4.f, roof);
            drawPixelRect(target, origin, 8.f, 6.f, 3.f, 4.f, sf::Color(255, 229, 101));
            drawPixelRect(target, origin, 11.f, 6.f, 2.f, 4.f, color);
        }
        else if (barracks) {
            drawPixelRect(target, origin, 3.f, 7.f, 15.f, 12.f, outline);
            drawPixelRect(target, origin, 4.f, 8.f, 13.f, 10.f, stone);
            drawPixelRect(target, origin, 2.f, 3.f, 17.f, 5.f, outline);
            drawPixelRect(target, origin, 3.f, 2.f, 15.f, 5.f, roof);
            drawPixelRect(target, origin, 8.f, 11.f, 5.f, 7.f, sf::Color(58, 43, 33));
            drawPixelRect(target, origin, 5.f, 9.f, 4.f, 2.f, color);
            drawPixelRect(target, origin, 13.f, 9.f, 3.f, 2.f, color);
        }
    }

    void drawResourceCrystal(sf::RenderTarget& target, sf::Vector2f origin)
    {
        drawPixelRect(target, origin, 4.f, 17.f, 13.f, 3.f, sf::Color(38, 28, 12, 82));
        drawPixelRect(target, origin, 8.f, 3.f, 5.f, 15.f, sf::Color(184, 124, 38));
        drawPixelRect(target, origin, 9.f, -1.f, 3.f, 5.f, sf::Color(255, 238, 127));
        drawPixelRect(target, origin, 5.f, 8.f, 4.f, 8.f, sf::Color(238, 184, 58));
        drawPixelRect(target, origin, 12.f, 7.f, 4.f, 9.f, sf::Color(249, 205, 76));
        drawPixelRect(target, origin, 10.f, 4.f, 2.f, 12.f, sf::Color(255, 249, 178));
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
    rect.setOutlineThickness(0.f);

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
    rect.setOutlineThickness(0.f);
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
    rect.setOutlineThickness(0.f);
}

bool MapPos::hasRaisedObject() const
{
    switch (id) {
    case tile::Tree:
    case tile::Mount:
    case tile::Player_Barracks:
    case tile::Enemy_Barracks:
    case tile::Player_Tower:
    case tile::Enemy_Tower:
    case tile::Choosen:
    case tile::UnableToReach:
        return true;
    default:
        return false;
    }
}

float MapPos::renderSortY() const
{
    return static_cast<float>((y + 1) * SqureSize);
}

void MapPos::drawGround(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(rect, states);

    const sf::Vector2f origin = rect.getPosition();
    switch (id) {
    case tile::River:
        drawRiverGround(target, origin, x, y);
        break;
    case tile::Path:
        drawPathGround(target, origin, x, y);
        break;
    case tile::Red_Base:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(156, 90, 70));
        drawPixelFrame(target, origin, sf::Color(238, 164, 128, 105), sf::Color(83, 44, 36, 92));
        break;
    case tile::Blue_Base:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(77, 105, 151));
        drawPixelFrame(target, origin, sf::Color(143, 184, 228, 105), sf::Color(36, 55, 94, 92));
        break;
    case tile::Player_Barracks:
    case tile::Enemy_Barracks:
    case tile::Player_Tower:
    case tile::Enemy_Tower:
        drawPixelRect(target, origin, 1.f, 1.f, 18.f, 18.f, sf::Color(156, 147, 110));
        drawPixelFrame(target, origin, sf::Color(224, 211, 159, 80), sf::Color(75, 68, 51, 86));
        break;
    case tile::Mount:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(147, 157, 124));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Tree:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(137, 177, 104));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Resource:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(174, 181, 113));
        drawPixelRect(target, origin, 3.f, 6.f, 14.f, 8.f, sf::Color(195, 168, 78, 72));
        drawPixelFrame(target, origin, sf::Color(250, 231, 128, 70), sf::Color(107, 85, 39, 70));
        break;
    case tile::Choosen:
    case tile::UnableToReach:
        break;
    case tile::Unit:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(213, 210, 118, 80));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Empty:
    default:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, IDtoColor(id));
        drawGrassDetail(target, origin, x, y);
        drawPixelFrame(target, origin, sf::Color(241, 246, 187, 32), sf::Color(83, 111, 70, 36));
        break;
    }
}

void MapPos::drawObject(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    const sf::Vector2f origin = rect.getPosition();
    switch (id) {
    case tile::Mount:
        drawMount(target, origin, x, y);
        break;
    case tile::Tree:
        drawTree(target, origin, x, y);
        break;
    case tile::Resource:
        drawResourceCrystal(target, origin);
        break;
    case tile::Player_Barracks:
        drawBuildingTile(target, origin, sf::Color(218, 76, 60), true, false);
        break;
    case tile::Enemy_Barracks:
        drawBuildingTile(target, origin, sf::Color(61, 128, 206), true, false);
        break;
    case tile::Player_Tower:
        drawBuildingTile(target, origin, sf::Color(218, 76, 60), false, true);
        break;
    case tile::Enemy_Tower:
        drawBuildingTile(target, origin, sf::Color(61, 128, 206), false, true);
        break;
    case tile::Choosen:
        drawPixelRect(target, origin, 1.f, 1.f, rect.getSize().x - 2.f, 2.f, sf::Color(218, 255, 134, 210));
        drawPixelRect(target, origin, 1.f, rect.getSize().y - 3.f, rect.getSize().x - 2.f, 2.f, sf::Color(218, 255, 134, 210));
        drawPixelRect(target, origin, 1.f, 1.f, 2.f, rect.getSize().y - 2.f, sf::Color(218, 255, 134, 210));
        drawPixelRect(target, origin, rect.getSize().x - 3.f, 1.f, 2.f, rect.getSize().y - 2.f, sf::Color(218, 255, 134, 210));
        break;
    case tile::UnableToReach:
        drawPixelRect(target, origin, 2.f, 8.f, rect.getSize().x - 4.f, 4.f, sf::Color(207, 68, 48, 210));
        drawPixelRect(target, origin, 8.f, 2.f, 4.f, rect.getSize().y - 4.f, sf::Color(207, 68, 48, 210));
        break;
    default:
        break;
    }
}

// Draw the tile rectangle.
void MapPos::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    drawGround(target, states);
    drawObject(target, states);
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
        return sf::Color(177, 186, 115, 120);
    case tile::Empty:
        return sf::Color(154, 188, 112);
    case tile::Path:
        return sf::Color(159, 184, 116);
    case tile::Red_Base:
        return sf::Color(156, 90, 70);
    case tile::Blue_Base:
        return sf::Color(77, 105, 151);
    case tile::Choosen:
        return sf::Color::Transparent;
    case tile::River:
        return sf::Color(57, 132, 178);
    case tile::Mount:
        return sf::Color(147, 157, 124);
    case tile::Tree:
        return sf::Color(137, 177, 104);
    case tile::Resource:
        return sf::Color(174, 181, 113);
    case tile::Player_Barracks:
        return sf::Color(156, 147, 110);
    case tile::Enemy_Barracks:
        return sf::Color(156, 147, 110);
    case tile::Player_Tower:
        return sf::Color(156, 147, 110);
    case tile::Enemy_Tower:
        return sf::Color(156, 147, 110);
    case tile::UnableToReach:
        return sf::Color::Transparent;
    default:
        return sf::Color::Black;
    }
}

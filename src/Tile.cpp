#include "Tile.h"
#include "Config.h"

#include <algorithm>
#include <cmath>

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

    int terrainHash(int x, int y, int salt)
    {
        int value = x * 7349 + y * 9157 + salt * 2971;
        value ^= value << 7;
        value ^= value >> 9;
        value ^= value << 8;
        return std::abs(value);
    }

    sf::Color mixColor(sf::Color a, sf::Color b, float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        const auto blend = [t](sf::Uint8 x, sf::Uint8 y) {
            return static_cast<sf::Uint8>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
        };
        return sf::Color(blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a));
    }

    void drawPixelDiamond(sf::RenderTarget& target, sf::Vector2f origin, float cx, float cy,
                          float rx, float ry, sf::Color color)
    {
        sf::ConvexShape diamond(4);
        diamond.setPoint(0, origin + sf::Vector2f(cx, cy - ry));
        diamond.setPoint(1, origin + sf::Vector2f(cx + rx, cy));
        diamond.setPoint(2, origin + sf::Vector2f(cx, cy + ry));
        diamond.setPoint(3, origin + sf::Vector2f(cx - rx, cy));
        diamond.setFillColor(color);
        target.draw(diamond);
    }

    void drawGrassBase(sf::RenderTarget& target, sf::Vector2f origin, int x, int y,
                       sf::Color base = sf::Color(139, 176, 98))
    {
        const int shade = terrainHash(x, y, 1) % 5;
        const sf::Color ground = mixColor(base, shade < 2 ? sf::Color(111, 148, 85) : sf::Color(201, 202, 124), shade < 2 ? 0.20f : 0.12f);
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, ground);

        // A few deterministic patches keep repeated tiles from looking like a
        // flat fill while still preserving crisp pixel readability.
        const sf::Color patchA(92, 140, 76, 54);
        const sf::Color patchB(220, 211, 127, 46);
        drawPixelRect(target, origin,
                      static_cast<float>(2 + terrainHash(x, y, 2) % 12),
                      static_cast<float>(3 + terrainHash(x, y, 3) % 8),
                      7.f, 2.f, shade % 2 == 0 ? patchA : patchB);
        drawPixelRect(target, origin,
                      static_cast<float>(5 + terrainHash(x, y, 4) % 12),
                      static_cast<float>(13 + terrainHash(x, y, 5) % 7),
                      6.f, 2.f, shade % 2 == 0 ? patchB : patchA);

        drawPixelFrame(target, origin, sf::Color(236, 245, 176, 24), sf::Color(65, 96, 63, 30));
    }

    void drawGrassDetail(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const bool flower = terrainHash(x, y, 6) % 9 == 0;
        const sf::Color blade = terrainHash(x, y, 7) % 3 == 0
            ? sf::Color(224, 219, 136, 76)
            : sf::Color(65, 125, 68, 82);
        for (int i = 0; i < 2; ++i) {
            const int px = 3 + terrainHash(x, y, 10 + i) % 16;
            const int py = 5 + terrainHash(x, y, 20 + i) % 13;
            drawPixelRect(target, origin, static_cast<float>(px), static_cast<float>(py), 4.f, 1.f, blade);
            drawPixelRect(target, origin, static_cast<float>(px + 1), static_cast<float>(py - 1), 1.f, 1.f, blade);
        }
        if (flower) {
            const float fx = static_cast<float>(5 + terrainHash(x, y, 31) % 12);
            const float fy = static_cast<float>(7 + terrainHash(x, y, 32) % 10);
            drawPixelRect(target, origin, fx, fy, 2.f, 2.f, sf::Color(255, 215, 109, 95));
            drawPixelRect(target, origin, fx + 2.f, fy + 1.f, 1.f, 1.f, sf::Color(244, 120, 94, 90));
        }
    }

    void drawPathGround(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        drawGrassBase(target, origin, x, y, sf::Color(150, 177, 103));
        drawPixelRect(target, origin, 0.f, 7.f, SqureSize, 10.f, sf::Color(118, 98, 66, 112));
        drawPixelRect(target, origin, 0.f, 8.f, SqureSize, 7.f, sf::Color(176, 145, 87));
        drawPixelRect(target, origin, 0.f, 15.f, SqureSize, 2.f, sf::Color(91, 80, 57, 102));
        drawPixelRect(target, origin, 0.f, 6.f, SqureSize, 2.f, sf::Color(231, 210, 128, 84));

        const float pebbleX = static_cast<float>(3 + terrainHash(x, y, 40) % 15);
        drawPixelRect(target, origin, pebbleX, 10.f, 3.f, 2.f, sf::Color(225, 198, 122, 120));
        drawPixelRect(target, origin, static_cast<float>(8 + terrainHash(x, y, 41) % 11), 14.f, 2.f, 1.f, sf::Color(95, 80, 55, 82));
    }

    void drawRiverGround(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const sf::Color base = terrainHash(x, y, 50) % 2 == 0
            ? sf::Color(45, 112, 166)
            : sf::Color(42, 101, 155);
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, base);
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(68, 153, 190, 28));
        drawPixelFrame(target, origin, sf::Color(132, 203, 218, 42), sf::Color(18, 55, 94, 56));

        const int wave = terrainHash(x, y, 51) % 10;
        drawPixelRect(target, origin, static_cast<float>(wave), 6.f, 8.f, 1.f, sf::Color(172, 230, 238, 142));
        drawPixelRect(target, origin, static_cast<float>((wave + 8) % 16), 12.f, 7.f, 1.f, sf::Color(217, 249, 251, 118));
        drawPixelRect(target, origin, static_cast<float>((wave + 4) % 14), 18.f, 5.f, 1.f, sf::Color(98, 184, 215, 105));
        drawPixelRect(target, origin, 2.f, 3.f, 3.f, 2.f, sf::Color(26, 77, 127, 70));
        drawPixelRect(target, origin, 18.f, 19.f, 4.f, 2.f, sf::Color(23, 66, 114, 72));
    }

    void drawTree(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const sf::Color outline(20, 43, 29);
        const sf::Color trunk(117, 74, 39);
        const sf::Color trunkLight(162, 103, 55);
        const sf::Color leaf = terrainHash(x, y, 61) % 2 == 0 ? sf::Color(42, 116, 61) : sf::Color(51, 134, 69);
        const sf::Color leafDark(29, 82, 50);
        const sf::Color leafLight(112, 184, 86);

        drawPixelDiamond(target, origin, 11.f, 20.f, 11.f, 3.5f, sf::Color(7, 15, 11, 82));
        drawPixelRect(target, origin, 8.f, 8.f, 6.f, 14.f, outline);
        drawPixelRect(target, origin, 9.f, 9.f, 4.f, 13.f, trunk);
        drawPixelRect(target, origin, 11.f, 9.f, 1.f, 9.f, trunkLight);

        drawPixelRect(target, origin, 3.f, 6.f, 17.f, 10.f, outline);
        drawPixelRect(target, origin, 1.f, 10.f, 21.f, 8.f, outline);
        drawPixelRect(target, origin, 5.f, -3.f, 13.f, 10.f, outline);
        drawPixelRect(target, origin, 6.f, -8.f, 10.f, 7.f, outline);

        drawPixelRect(target, origin, 4.f, 7.f, 15.f, 8.f, leaf);
        drawPixelRect(target, origin, 2.f, 11.f, 19.f, 6.f, leafDark);
        drawPixelRect(target, origin, 6.f, -2.f, 11.f, 8.f, leaf);
        drawPixelRect(target, origin, 7.f, -7.f, 8.f, 6.f, mixColor(leaf, sf::Color::White, 0.10f));
        drawPixelRect(target, origin, 8.f, -4.f, 5.f, 2.f, leafLight);
        drawPixelRect(target, origin, 5.f, 8.f, 5.f, 2.f, leafLight);
        drawPixelRect(target, origin, 15.f, 12.f, 3.f, 2.f, leafLight);
    }

    void drawMount(sf::RenderTarget& target, sf::Vector2f origin, int x, int y)
    {
        const bool warm = terrainHash(x, y, 70) % 2 == 0;
        const sf::Color rock = warm ? sf::Color(119, 124, 113) : sf::Color(101, 116, 119);
        const sf::Color rockDark(55, 61, 58);
        const sf::Color rockLight(174, 171, 141);
        const sf::Color snow(235, 236, 218);

        drawPixelDiamond(target, origin, 11.f, 21.f, 12.f, 4.f, sf::Color(8, 12, 12, 86));
        sf::ConvexShape peak(5);
        peak.setPoint(0, origin + sf::Vector2f(0.f, 20.f));
        peak.setPoint(1, origin + sf::Vector2f(6.f, 7.f));
        peak.setPoint(2, origin + sf::Vector2f(11.f, -6.f));
        peak.setPoint(3, origin + sf::Vector2f(18.f, 8.f));
        peak.setPoint(4, origin + sf::Vector2f(23.f, 20.f));
        peak.setFillColor(rockDark);
        target.draw(peak);
        sf::ConvexShape face(4);
        face.setPoint(0, origin + sf::Vector2f(2.f, 19.f));
        face.setPoint(1, origin + sf::Vector2f(11.f, -4.f));
        face.setPoint(2, origin + sf::Vector2f(12.f, 19.f));
        face.setPoint(3, origin + sf::Vector2f(8.f, 21.f));
        face.setFillColor(rock);
        target.draw(face);
        sf::ConvexShape lit(4);
        lit.setPoint(0, origin + sf::Vector2f(11.f, -4.f));
        lit.setPoint(1, origin + sf::Vector2f(21.f, 19.f));
        lit.setPoint(2, origin + sf::Vector2f(12.f, 19.f));
        lit.setPoint(3, origin + sf::Vector2f(11.f, 8.f));
        lit.setFillColor(rockLight);
        target.draw(lit);
        drawPixelRect(target, origin, 8.f, -2.f, 6.f, 4.f, snow);
        drawPixelRect(target, origin, 9.f, 2.f, 4.f, 2.f, snow);
        drawPixelRect(target, origin, 5.f, 13.f, 5.f, 2.f, sf::Color(73, 79, 70, 132));
        drawPixelRect(target, origin, 14.f, 12.f, 4.f, 2.f, sf::Color(92, 100, 91, 126));
    }

    void drawBuildingTile(sf::RenderTarget& target, sf::Vector2f origin, sf::Color color, bool barracks, bool tower)
    {
        const sf::Color outline(42, 34, 28);
        const sf::Color stone(178, 164, 124);
        const sf::Color stoneLight(225, 208, 154);
        const sf::Color roof = barracks ? sf::Color(213, 121, 56) : sf::Color(213, 195, 128);

        drawPixelDiamond(target, origin, 12.f, 21.f, 13.f, 4.f, sf::Color(10, 14, 12, 86));
        if (tower) {
            drawPixelRect(target, origin, 5.f, 1.f, 12.f, 19.f, outline);
            drawPixelRect(target, origin, 6.f, 2.f, 10.f, 17.f, stone);
            drawPixelRect(target, origin, 7.f, 4.f, 8.f, 3.f, stoneLight);
            drawPixelRect(target, origin, 3.f, -5.f, 16.f, 7.f, outline);
            drawPixelRect(target, origin, 4.f, -4.f, 14.f, 5.f, roof);
            drawPixelRect(target, origin, 6.f, -6.f, 3.f, 3.f, outline);
            drawPixelRect(target, origin, 13.f, -6.f, 3.f, 3.f, outline);
            drawPixelRect(target, origin, 8.f, 8.f, 3.f, 5.f, sf::Color(255, 230, 105));
            drawPixelRect(target, origin, 12.f, 8.f, 2.f, 5.f, color);
            drawPixelRect(target, origin, 7.f, 15.f, 8.f, 2.f, sf::Color(112, 96, 72));
        }
        else if (barracks) {
            drawPixelRect(target, origin, 2.f, 8.f, 19.f, 12.f, outline);
            drawPixelRect(target, origin, 3.f, 9.f, 17.f, 10.f, stone);
            drawPixelRect(target, origin, 5.f, 11.f, 13.f, 2.f, stoneLight);
            drawPixelRect(target, origin, 1.f, 3.f, 21.f, 6.f, outline);
            drawPixelRect(target, origin, 2.f, 2.f, 19.f, 6.f, roof);
            drawPixelRect(target, origin, 5.f, 0.f, 13.f, 3.f, mixColor(roof, sf::Color::White, 0.12f));
            drawPixelRect(target, origin, 8.f, 12.f, 6.f, 8.f, sf::Color(55, 40, 31));
            drawPixelRect(target, origin, 5.f, 10.f, 4.f, 2.f, color);
            drawPixelRect(target, origin, 15.f, 10.f, 3.f, 2.f, color);
            drawPixelRect(target, origin, 3.f, 18.f, 17.f, 2.f, sf::Color(91, 78, 58));
        }
    }

    void drawResourceCrystal(sf::RenderTarget& target, sf::Vector2f origin)
    {
        drawPixelDiamond(target, origin, 12.f, 20.f, 13.f, 4.f, sf::Color(38, 28, 12, 86));
        drawPixelRect(target, origin, 5.f, 4.f, 5.f, 14.f, sf::Color(204, 137, 40));
        drawPixelRect(target, origin, 7.f, -2.f, 3.f, 7.f, sf::Color(255, 235, 112));
        drawPixelRect(target, origin, 10.f, 1.f, 7.f, 18.f, sf::Color(242, 177, 48));
        drawPixelRect(target, origin, 12.f, -5.f, 3.f, 7.f, sf::Color(255, 249, 162));
        drawPixelRect(target, origin, 16.f, 7.f, 4.f, 10.f, sf::Color(255, 211, 84));
        drawPixelRect(target, origin, 11.f, 4.f, 2.f, 13.f, sf::Color(255, 253, 194));
        drawPixelRect(target, origin, 18.f, 9.f, 1.f, 6.f, sf::Color(255, 244, 150));
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
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(136, 75, 60));
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, 5.f, sf::Color(176, 102, 78));
        drawPixelRect(target, origin, 2.f, 8.f, 20.f, 9.f, sf::Color(104, 60, 52, 122));
        drawPixelFrame(target, origin, sf::Color(243, 172, 128, 118), sf::Color(77, 38, 34, 110));
        break;
    case tile::Blue_Base:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(68, 94, 141));
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, 5.f, sf::Color(101, 143, 194));
        drawPixelRect(target, origin, 2.f, 8.f, 20.f, 9.f, sf::Color(47, 71, 111, 122));
        drawPixelFrame(target, origin, sf::Color(153, 200, 238, 118), sf::Color(31, 51, 90, 110));
        break;
    case tile::Player_Barracks:
    case tile::Enemy_Barracks:
    case tile::Player_Tower:
    case tile::Enemy_Tower:
        drawPixelRect(target, origin, 0.f, 0.f, SqureSize, SqureSize, sf::Color(119, 119, 88));
        drawPixelRect(target, origin, 2.f, 3.f, 20.f, 16.f, sf::Color(161, 149, 108));
        drawPixelRect(target, origin, 3.f, 4.f, 18.f, 3.f, sf::Color(213, 199, 143, 78));
        drawPixelRect(target, origin, 3.f, 18.f, 18.f, 2.f, sf::Color(68, 61, 45, 86));
        drawPixelFrame(target, origin, sf::Color(231, 214, 154, 84), sf::Color(68, 59, 42, 92));
        break;
    case tile::Mount:
        drawGrassBase(target, origin, x, y, sf::Color(134, 154, 111));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Tree:
        drawGrassBase(target, origin, x, y, sf::Color(126, 169, 95));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Resource:
        drawGrassBase(target, origin, x, y, sf::Color(151, 166, 94));
        drawPixelDiamond(target, origin, 12.f, 13.f, 11.f, 6.f, sf::Color(202, 157, 62, 78));
        drawPixelRect(target, origin, 4.f, 6.f, 16.f, 9.f, sf::Color(214, 181, 76, 60));
        drawPixelFrame(target, origin, sf::Color(255, 232, 126, 74), sf::Color(105, 80, 36, 78));
        break;
    case tile::Choosen:
    case tile::UnableToReach:
        break;
    case tile::Unit:
        drawGrassBase(target, origin, x, y, sf::Color(158, 181, 102));
        drawGrassDetail(target, origin, x, y);
        break;
    case tile::Empty:
    default:
        drawGrassBase(target, origin, x, y);
        drawGrassDetail(target, origin, x, y);
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

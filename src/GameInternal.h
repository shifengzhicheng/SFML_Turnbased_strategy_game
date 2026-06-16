#pragma once

#include "Game.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <list>
#include <memory>
#include <string>

namespace game_internal
{
    inline constexpr int SqureSize = config::TileSize;
    inline constexpr int width = config::MapWidth;
    inline constexpr int height = config::MapHeight;
    inline constexpr int MaxUnit = config::MaxUnits;

    inline bool loadFont(sf::Font& font, const std::string& path)
    {
        if (!font.loadFromFile(path)) {
            std::cerr << "Failed to load font: " << path << std::endl;
            return false;
        }
        return true;
    }

    inline void setupText(sf::Text& text, const sf::Font& font, unsigned int size, sf::Color color,
                   const std::string& value, float x, float y)
    {
        text.setFont(font);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setString(value);
        text.setPosition(x, y);
    }

    inline void drawUnitBase(sf::RenderWindow& window, Point point, sf::Color color)
    {
        sf::CircleShape marker(9.f, 28);
        marker.setOrigin(9.f, 9.f);
        marker.setScale(1.15f, 0.62f);
        marker.setPosition(point.x * SqureSize + SqureSize / 2.f, point.y * SqureSize + SqureSize / 2.f + 7.f);
        marker.setFillColor(sf::Color(color.r, color.g, color.b, 96));
        marker.setOutlineColor(sf::Color(color.r, color.g, color.b, 210));
        marker.setOutlineThickness(1.4f);
        window.draw(marker);
    }

    inline sf::Color ownerColor(int owner)
    {
        if (owner == PLAYER) {
            return sf::Color(218, 76, 60);
        }
        if (owner == AI) {
            return sf::Color(61, 128, 206);
        }
        if (owner == -2) {
            return sf::Color(236, 111, 72);
        }
        return sf::Color(226, 180, 63);
    }

    inline const char* perkTitle(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Iron Wall";
        case perk::Volley:
            return "Volley Drill";
        case perk::Charge:
            return "Shock Charge";
        case perk::SiegeCraft:
            return "Siege Craft";
        case perk::TowerCraft:
            return "Watchtowers";
        case perk::Logistics:
            return "Logistics";
        case perk::Mining:
            return "Supply Crew";
        case perk::WarChest:
            return "War Chest";
        case perk::Drill:
        default:
            return "Blade Drill";
        }
    }

    inline const char* perkDescription(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Infantry and Guard +9% HP.\nStronger front line.";
        case perk::Volley:
            return "Shooters +8% damage.\nAlso shoot slightly faster.";
        case perk::Charge:
            return "Cavalry +8% damage.\nBetter dives.";
        case perk::SiegeCraft:
            return "Siege +8% damage.\nBuildings take extra pain.";
        case perk::TowerCraft:
            return "Towers +9% damage.\nRange scales carefully.";
        case perk::Logistics:
            return "Barracks train 6% faster.\nTurns economy into tempo.";
        case perk::Mining:
            return "Natural CMD +7%.\nSimple scaling.";
        case perk::WarChest:
            return "Instant CMD burst.\nBuild or queue now.";
        case perk::Drill:
        default:
            return "Infantry +8%, Guard +6%.\nFrontline stays useful.";
        }
    }

    inline int maxPerkLevel(int type)
    {
        if (type == perk::WarChest) {
            return 99;
        }
        if (type == perk::TowerCraft || type == perk::SiegeCraft) {
            return 4;
        }
        return 5;
    }

    inline const char* laneName(int laneIndex)
    {
        switch (laneIndex) {
        case lane::Top:
            return "Top";
        case lane::Bot:
            return "Bot";
        case lane::Mid:
        default:
            return "Mid";
        }
    }

    inline const char* operationTypeName(int type)
    {
        switch (type) {
        case gameop::UpgradeEconomy:
            return "UpgradeEconomy";
        case gameop::UpgradeTech:
            return "UpgradeTech";
        case gameop::BuildBarracks:
            return "BuildBarracks";
        case gameop::BuildTower:
            return "BuildTower";
        case gameop::QueueUnit:
            return "QueueUnit";
        case gameop::SelectLane:
        default:
            return "SelectLane";
        }
    }

    inline const char* unitDebugName(int name)
    {
        switch (name) {
        case UName::SHOOTER:
            return "Shooter";
        case UName::CAVALRY:
            return "Cavalry";
        case UName::SIEGE:
            return "Siege";
        case UName::GUARDIAN:
            return "Guardian";
        case UName::INFANTARY:
        default:
            return "Infantry";
        }
    }

    inline const char* perkShortName(int type)
    {
        switch (type) {
        case perk::Fortitude:
            return "Wall";
        case perk::Volley:
            return "Volley";
        case perk::Charge:
            return "Charge";
        case perk::SiegeCraft:
            return "Siege";
        case perk::TowerCraft:
            return "Tower";
        case perk::Logistics:
            return "Logi";
        case perk::Mining:
            return "Eco";
        case perk::WarChest:
            return "Chest";
        case perk::Drill:
        default:
            return "Blade";
        }
    }

    inline bool nearPoint(Point a, Point b, int radius)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy <= radius * radius;
    }

    inline int& commandPool(Game& game, int team)
    {
        return team == PLAYER ? game.playerCommand : game.aiCommand;
    }

    inline int countUnitsNamed(const std::list<std::unique_ptr<MoveableUnit>>& units, int name)
    {
        return static_cast<int>(std::count_if(units.begin(), units.end(), [name](const std::unique_ptr<MoveableUnit>& unit) {
            return unit->unitName == name;
        }));
    }

    inline tile::ID buildingTileId(int team, int type)
    {
        if (type == building::DefenseTower) {
            return team == PLAYER ? tile::Player_Tower : tile::Enemy_Tower;
        }
        return team == PLAYER ? tile::Player_Barracks : tile::Enemy_Barracks;
    }

    inline float buildingSeconds(int type)
    {
        if (type == building::DefenseTower) {
            return realtime::DefenseTowerBuildSeconds;
        }
        return realtime::BarracksBuildSeconds;
    }

    inline const char* buildingName(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return "Tower";
        case building::Barracks:
        default:
            return "Barracks";
        }
    }

    inline int buildingMaxHealth(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return config::DefenseTowerHealth;
        case building::Barracks:
        default:
            return config::BarracksHealth;
        }
    }

    inline int buildingCommandCost(int type)
    {
        switch (type) {
        case building::DefenseTower:
            return config::TowerCost;
        case building::Barracks:
        default:
            return config::BarracksCost;
        }
    }

    inline int distanceSquared(Point a, Point b)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    inline sf::Vector2f unitCenter(const Unit& unit)
    {
        const auto bounds = unit.getGlobalBounds();
        if (bounds.width > 0.f && bounds.height > 0.f) {
            return sf::Vector2f(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        }
        const float baseOffset = unit.unitName == UName::BASE ? SqureSize : SqureSize / 2.f;
        return sf::Vector2f(unit.x * SqureSize + baseOffset, unit.y * SqureSize + baseOffset);
    }

    inline float unitTrainSeconds(int name)
    {
        switch (name) {
        case UName::SHOOTER:
            return realtime::ShooterTrainSeconds;
        case UName::CAVALRY:
            return realtime::CavalryTrainSeconds;
        case UName::SIEGE:
            return realtime::SiegeTrainSeconds;
        case UName::GUARDIAN:
            return realtime::GuardianTrainSeconds;
        case UName::INFANTARY:
        default:
            return realtime::InfantryTrainSeconds;
        }
    }

    inline bool isResourceClick(const ResourceNode& node, int tileX, int tileY)
    {
        return node.point.x == tileX && node.point.y == tileY;
    }
}


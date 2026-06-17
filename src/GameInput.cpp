#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace sf;
using namespace std;
using namespace game_internal;

void Game::startInput(Vector2i mousePos, Event event) {
    startBtn.setPosition(470.f, 410.f);
    startHelpBtn.setPosition(710.f, 410.f);
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::H) {
            tutorialVisible = !tutorialVisible;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape && tutorialVisible) {
            tutorialVisible = false;
            return;
        }
    }
    if (startHelpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        startHelpBtn.setState(NORMAL);
        return;
    }
    if (tutorialVisible) {
        return;
    }

    if (startBtn.checkMouse(mousePos, event) == RELEASE) {
        gameSceneState = SCENE_GAME;
        startBtn.setState(NORMAL);
        clear();
    }
}

void Game::handleRealtimeMapClick(Vector2i mousePos, Event event)
{
    if (event.type != Event::EventType::MouseButtonPressed || event.mouseButton.button != Mouse::Left) {
        return;
    }
    if (mousePos.x < 0 || mousePos.x >= width || mousePos.y < 0 || mousePos.y >= height) {
        return;
    }

    const int tileX = mousePos.x / SqureSize;
    const int tileY = mousePos.y / SqureSize;
    for (const auto& node : resources) {
        if (isResourceClick(node, tileX, tileY)) {
            addFloatingText(sf::Vector2f(node.point.x * SqureSize, node.point.y * SqureSize - 14.f),
                            "Income: ECONOMY button", sf::Color(255, 226, 112), 12);
            return;
        }
    }

    // Economy is now a single natural-income track upgraded from the side
    // panel. Map clicks are reserved for selection and lane reading.
}

void Game::GameInput(Vector2i mousePos, Event event) {
    const bool mouseInMap = mousePos.x >= 0 && mousePos.x < width && mousePos.y >= 0 && mousePos.y < height;
    if (perkOverlayVisible) {
        handleRewardInput(mousePos, event);
        return;
    }
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::H) {
            tutorialVisible = !tutorialVisible;
            return;
        }
        if (event.key.code == sf::Keyboard::Escape && tutorialVisible) {
            tutorialVisible = false;
            return;
        }
        if (event.key.code == sf::Keyboard::C)
        {
            clear();
            gameSceneState = SCENE_GAME;
        }
    }
    if (tutorialVisible) {
        return;
    }
    handleBuildButtons(mousePos, event);
    if (tutorialVisible) {
        return;
    }
    handleRealtimeMapClick(mousePos, event);
    if (Base_red) {
        Base_red->checkMouse(mousePos, event);
    }
    if (Base_blue) {
        Base_blue->checkHover(mousePos, event);
    }
    for (auto& u : enemys) {
        u->checkHover(mousePos, event);
    }
    if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T))
    {
        setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Tree);
    }
    if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M))
    {
        setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Mount);
    }
    if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
    {
        setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::River);
    }
    if (mouseInMap && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
    {
        setTileID(mousePos.x / SqureSize, mousePos.y / SqureSize, tile::Empty);
    }
}

void Game::Input()
{
    sf::Event event;
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    while (window.pollEvent(event))
    {
        if (event.type == Event::Closed) {
            window.close();
        }
        switch (gameSceneState) {
        case SCENE_START:
            startInput(mousePos, event); break;
        case SCENE_GAME:
            GameInput(mousePos, event); break;
        case SCEN_GAMEOVER:
            overinput(mousePos, event); break;
        default:
            break;
        }
    }
}

void Game::overinput(sf::Vector2i mousePos, sf::Event event) {
    endGame.setPosition(550.f, 372.f);
    tutorialVisible = false;
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::C)
        {
            clear();
            gameSceneState = SCENE_GAME;
        }
    }
    if (endGame.checkMouse(mousePos, event) == RELEASE) {
        clear();
        gameSceneState = SCENE_GAME;
        endGame.setState(NORMAL);
    }
}

void Game::handleBuildButtons(Vector2i mousePos, Event event)
{
    handleLaneInput(mousePos, event);

    if (helpBtn.checkMouse(mousePos, event) == RELEASE) {
        tutorialVisible = !tutorialVisible;
        helpBtn.setState(NORMAL);
        return;
    }

    if (upgradeBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::UpgradeTech));
        upgradeBtn.setState(NORMAL);
    }

    if (economyBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::UpgradeEconomy));
        economyBtn.setState(NORMAL);
        return;
    }

    if (barracksBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::BuildBarracks, playerSelectedLane));
        barracksBtn.setState(NORMAL);
        return;
    }

    if (towerBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::BuildTower, playerSelectedLane));
        towerBtn.setState(NORMAL);
        return;
    }

    if (inf.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::INFANTARY));
        inf.setState(NORMAL);
    }
    if (sho.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::SHOOTER));
        sho.setState(NORMAL);
    }
    if (cav.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::CAVALRY));
        cav.setState(NORMAL);
    }
    if (siegeBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::SIEGE));
        siegeBtn.setState(NORMAL);
    }
    if (guardianBtn.checkMouse(mousePos, event) == RELEASE) {
        executeOperation(PLAYER, GameOperation(gameop::QueueUnit, playerSelectedLane, UName::GUARDIAN));
        guardianBtn.setState(NORMAL);
    }
}

void Game::handleLaneInput(Vector2i mousePos, Event event)
{
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const float y = 190.f;
    const float w = 56.f;
    const float h = 24.f;
    for (int i = 0; i < lane::Count; ++i) {
        const sf::FloatRect rect(config::PanelX + 17.f + static_cast<float>(i) * 64.f, y, w, h);
        if (rect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
            playerSelectedLane = i;
            addFloatingText(sf::Vector2f(config::PanelX + 20.f, 146.f),
                            std::string("Lane: ") + laneName(i), sf::Color(218, 255, 134), 12);
            return;
        }
    }
}

void Game::handleRewardInput(sf::Vector2i mousePos, sf::Event event)
{
    if (!perkOverlayVisible) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1 || event.key.code == sf::Keyboard::Enter) {
            applyRewardChoice(0);
        }
        else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
            applyRewardChoice(1);
        }
        else if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            applyRewardChoice(2);
        }
        else if (event.key.code == sf::Keyboard::R || event.key.code == sf::Keyboard::Space) {
            rerollRewardChoices();
        }
        return;
    }

    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const sf::Vector2f point(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    for (int i = 0; i < static_cast<int>(perkChoices.size()); ++i) {
        const sf::FloatRect card(235.f + static_cast<float>(i) * 278.f, 295.f, 250.f, 170.f);
        if (card.contains(point)) {
            applyRewardChoice(i);
            return;
        }
    }

    const sf::FloatRect rerollButton(760.f, 216.f, 210.f, 42.f);
    if (rerollButton.contains(point)) {
        rerollRewardChoices();
    }
}

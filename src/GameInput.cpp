#include "Game.h"
#include "GameInternal.h"
#include "AllUnit.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"
#include "SidebarLayout.h"

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
        if (!tutorialVisible) {
            const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
            const auto unitHotkey = [this, shift](int unitName) {
                executeOperation(PLAYER, GameOperation(shift ? gameop::UpgradeUnitMastery : gameop::QueueUnit,
                                                       playerSelectedLane, unitName));
            };
            if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
                unitHotkey(UName::INFANTARY);
                return;
            }
            if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
                unitHotkey(UName::SHOOTER);
                return;
            }
            if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
                unitHotkey(UName::CAVALRY);
                return;
            }
            if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
                unitHotkey(UName::SIEGE);
                return;
            }
            if (event.key.code == sf::Keyboard::Num5 || event.key.code == sf::Keyboard::Numpad5) {
                unitHotkey(UName::GUARDIAN);
                return;
            }
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
    while (window.pollEvent(event))
    {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (event.type == Event::MouseButtonPressed || event.type == Event::MouseButtonReleased) {
            mousePos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        }
        else if (event.type == Event::MouseMoved) {
            mousePos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
        }
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
    if (handleLaneInput(mousePos, event)) {
        return;
    }

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

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const std::pair<int, int> masteryButtons[] = {
            {UName::INFANTARY, config::BuildInfantryY},
            {UName::SHOOTER, config::BuildShooterY},
            {UName::CAVALRY, config::BuildCavalryY},
            {UName::SIEGE, config::BuildSiegeY},
            {UName::GUARDIAN, config::BuildGuardianY},
        };
        for (const auto& entry : masteryButtons) {
            if (sidebar_layout::masteryButtonRect(entry.second).contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                executeOperation(PLAYER, GameOperation(gameop::UpgradeUnitMastery, playerSelectedLane, entry.first));
                inf.setState(NORMAL);
                sho.setState(NORMAL);
                cav.setState(NORMAL);
                siegeBtn.setState(NORMAL);
                guardianBtn.setState(NORMAL);
                return;
            }
        }
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

bool Game::handleLaneInput(Vector2i mousePos, Event event)
{
    const bool isLeftPress = event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left;
    const bool isLeftRelease = event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left;
    if (!isLeftPress && !isLeftRelease) {
        return false;
    }

    const sf::Vector2f point(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    // The visible lane tabs have small gaps, so treat the whole strip as a
    // three-way segmented control. A click in a gap selects the nearest lane
    // instead of feeling like the button is broken.
    const sf::FloatRect strip = sidebar_layout::laneHitStripRect();
    if (strip.contains(point)) {
        const float segment = strip.width / static_cast<float>(lane::Count);
        const int laneIndex = std::clamp(static_cast<int>((point.x - strip.left) / segment), 0, lane::Count - 1);
        if (isLeftPress || playerSelectedLane != laneIndex) {
            playerSelectedLane = laneIndex;
            addFloatingText(sf::Vector2f(config::PanelX + 20.f, 146.f),
                            std::string("Lane: ") + laneName(laneIndex), sf::Color(218, 255, 134), 12);
        }
        return true;
    }
    return false;
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

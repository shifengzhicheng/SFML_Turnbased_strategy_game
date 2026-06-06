#include "Game.h"
#include "AllUnit.h"
#include "Config.h"
#include "Map.h"
#include "ArtAssets.h"
#include "AutoCombat.h"
#include "RealtimeConfig.h"

#include <algorithm>
#include <cmath>

using namespace sf;
using namespace std;

namespace
{
    constexpr int SqureSize = config::TileSize;
    constexpr int width = config::MapWidth;
    constexpr int height = config::MapHeight;
    constexpr int MaxUnit = config::MaxUnits;

    bool loadFont(sf::Font& font, const std::string& path)
    {
        if (!font.loadFromFile(path)) {
            std::cerr << "Failed to load font: " << path << std::endl;
            return false;
        }
        return true;
    }

    void setupText(sf::Text& text, const sf::Font& font, unsigned int size, sf::Color color,
                   const std::string& value, float x, float y)
    {
        text.setFont(font);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setString(value);
        text.setPosition(x, y);
    }

    void drawUnitBase(sf::RenderWindow& window, Point point, sf::Color color)
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

    sf::Color ownerColor(int owner)
    {
        if (owner == PLAYER) {
            return sf::Color(218, 76, 60);
        }
        if (owner == AI) {
            return sf::Color(61, 128, 206);
        }
        return sf::Color(226, 180, 63);
    }

    bool nearPoint(Point a, Point b, int radius)
    {
        const int dx = a.x - b.x;
        const int dy = a.y - b.y;
        return dx * dx + dy * dy <= radius * radius;
    }

    int& commandPool(Game& game, int team)
    {
        return team == PLAYER ? game.playerCommand : game.aiCommand;
    }

    int countUnitsNamed(const std::list<std::unique_ptr<MoveableUnit>>& units, int name)
    {
        return static_cast<int>(std::count_if(units.begin(), units.end(), [name](const std::unique_ptr<MoveableUnit>& unit) {
            return unit->unitName == name;
        }));
    }
}

bool Game::MousePosChanged()
{
    Vector2i mouse = sf::Mouse::getPosition(window);
    int x = mouse.x / SqureSize;
    int y = mouse.y / SqureSize;
    if (MousePoint.x != x || MousePoint.y != y) {
        MousePoint.x = x;
        MousePoint.y = y;
        return true;
    }
    return false;
}

Game::Game() :
    gameWin(false),
    MosOnUnit(nullptr),
    horizontalTiles(width / SqureSize),
    playerturn(true),
    running(false)
{
    window.create(sf::VideoMode{ config::WindowWidth, config::WindowHeight }, "Project_War");
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight)));
    window.setFramerateLimit(60);
    Initial();
}

Game::~Game() = default;
void Game::loadpic()
{
    art::makeIntroTexture(background, myfont);
    art::makeButtonTexture(tStartBtnNormal, myfont, "START", art::ButtonState::Normal, sf::Vector2u(180, 72));
    art::makeButtonTexture(tStartBtnHover, myfont, "START", art::ButtonState::Hover, sf::Vector2u(190, 76));
    art::makeButtonTexture(tStartBtnClick, myfont, "START", art::ButtonState::Pressed, sf::Vector2u(170, 68));
    art::makeButtonTexture(tEndBtnNormal, myfont, "END TURN", art::ButtonState::Normal, sf::Vector2u(128, 44));
    art::makeButtonTexture(tEndBtnHover, myfont, "END TURN", art::ButtonState::Hover, sf::Vector2u(128, 44));
    art::makeButtonTexture(tEndBtnClick, myfont, "END TURN", art::ButtonState::Pressed, sf::Vector2u(128, 44));
    art::makeButtonTexture(tOverBtnNormal, myfont, "PLAY AGAIN", art::ButtonState::Normal, sf::Vector2u(240, 96));
    art::makeButtonTexture(tOverBtnHover, myfont, "PLAY AGAIN", art::ButtonState::Hover, sf::Vector2u(250, 100));
    art::makeButtonTexture(tOverBtnClick, myfont, "PLAY AGAIN", art::ButtonState::Pressed, sf::Vector2u(232, 92));
    art::makeButtonTexture(tinf, myfont, "Infantry", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfHover, myfont, "Infantry", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfClick, myfont, "Infantry", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tcav, myfont, "Cavalry", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavHover, myfont, "Cavalry", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavClick, myfont, "Cavalry", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tsho, myfont, "Shooter", art::ButtonState::Normal, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoHover, myfont, "Shooter", art::ButtonState::Hover, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoClick, myfont, "Shooter", art::ButtonState::Pressed, sf::Vector2u(128, 54), art::UnitKind::Shooter, art::Team::Player);

    startBtn.setTextures(tStartBtnNormal, tStartBtnHover, tStartBtnClick);
    EndTurnBtn.setTextures(tEndBtnNormal, tEndBtnHover, tEndBtnClick);
    endGame.setTextures(tOverBtnNormal, tOverBtnHover, tOverBtnClick);
    inf.setTextures(tinf, tinfHover, tinfClick);
    cav.setTextures(tcav, tcavHover, tcavClick);
    sho.setTextures(tsho, tshoHover, tshoClick);
    back.setTexture(background);
}
void Game::Initial()
{
    loadFont(myfont, "data/ttf/arial.ttf");
    loadMediaData();
    gameSceneState = SCENE_START;
    gameOver = false;
    const float panelTextX = static_cast<float>(config::PanelX + config::PanelPadding);
    setupText(Globle_text, myfont, 17, sf::Color(229, 221, 189), "GameStart", panelTextX, 48.f);
    setupText(UnitText, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 96.f);
    setupText(UnitAttack, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 118.f);
    setupText(UnitHP, myfont, 14, sf::Color(201, 215, 186), "", panelTextX, 140.f);
    setupText(CommandText, myfont, 14, sf::Color(255, 218, 112), "", panelTextX, 176.f);
    setupText(panelTitle, myfont, 19, sf::Color(255, 246, 208), "AUTO WAR", panelTextX, 16.f);
    setupText(panelHint, myfont, 13, sf::Color(211, 199, 165), "Select base\nto train", panelTextX, 232.f);
    setupText(infantryLabel, myfont, 11, sf::Color(228, 218, 185), "Auto front line", panelTextX, config::BuildInfantryY + 57.f);
    setupText(shooterLabel, myfont, 11, sf::Color(228, 218, 185), "Auto ranged DPS", panelTextX, config::BuildShooterY + 57.f);
    setupText(cavalryLabel, myfont, 11, sf::Color(228, 218, 185), "Auto fast raid", panelTextX, config::BuildCavalryY + 57.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(35, 43, 39));
    sidePanel.setOutlineColor(sf::Color(19, 24, 22));
    sidePanel.setOutlineThickness(2.f);

    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);
}

void Game::loadMediaData()
{
    loadpic();
}

void Game::logicBeforeInput()
{
    syncMazeFromTiles();
    astar.setMaze(maze);
    if (!realtimeMode && playerturn == false) {
        AIlogic();
    }
}

void Game::AIUnitreset()
{
    Unitsreset(enemys);
    if (Base_blue) {
        Base_blue->reset();
    }
}

void Game::clearSelection()
{
    // Keep selection exclusive so UI hints, build buttons, and path previews
    // cannot be overwritten by multiple selected actors in one frame.
    drawPaths.clear();
    if (Base_red) {
        Base_red->setState(UState::UNITNORMAL);
    }
    if (Base_blue) {
        Base_blue->setState(UState::UNITNORMAL);
    }
    for (auto& unit : myunits) {
        unit->setState(UState::UNITNORMAL);
    }
    for (auto& unit : enemys) {
        unit->setState(UState::UNITNORMAL);
    }
}

void Game::selectOnly(Unit* unit)
{
    const bool wasSelected = unit != nullptr && unit->UnitState == UState::UNITCLICK;
    clearSelection();
    if (unit != nullptr && !wasSelected) {
        unit->setState(UState::UNITCLICK);
    }
}

bool Game::isBlockingTile(tile::ID id) const
{
    return id == tile::Mount
        || id == tile::River
        || id == tile::Tree
        || (!realtimeMode && id == tile::Unit)
        || id == tile::Red_Base
        || id == tile::Blue_Base;
}

bool Game::isMapCell(int x, int y) const
{
    return y >= 0
        && y < static_cast<int>(maze.size())
        && x >= 0
        && x < static_cast<int>(maze[y].size());
}

bool Game::isRealtimeMode() const
{
    return realtimeMode;
}

bool Game::isCellWalkableForUnit(int x, int y) const
{
    if (!isMapCell(x, y)) {
        return false;
    }
    const tile::ID id = tiles[y * horizontalTiles + x].getID();
    return id == tile::Empty || id == tile::Path || id == tile::Choosen || id == tile::Unit;
}

bool Game::isCellReservedForSpawn(int x, int y) const
{
    const auto matchesCell = [x, y](const std::unique_ptr<MoveableUnit>& unit) {
        return unit->x == x && unit->y == y;
    };
    return std::any_of(myunits.begin(), myunits.end(), matchesCell)
        || std::any_of(enemys.begin(), enemys.end(), matchesCell);
}

void Game::setTileID(int x, int y, tile::ID id)
{
    const auto index = y * horizontalTiles + x;
    if (!isMapCell(x, y) || index < 0 || index >= static_cast<int>(tiles.size())) {
        return;
    }
    // Tiles are the render source and maze is the pathfinding source; update
    // them together to avoid one-frame desyncs.
    tiles[index].setID(id);
    maze[y][x] = isBlockingTile(id) ? 1 : 0;
}

void Game::syncMazeFromTiles()
{
    // Rebuild pathing from visible map state before input/AI mutates gameplay.
    for (const auto& tile : tiles) {
        const auto p = tile.getIndex();
        if (isMapCell(p.x, p.y)) {
            maze[p.y][p.x] = isBlockingTile(tile.getID()) ? 1 : 0;
        }
    }
}

void Game::AIlogic() {
    bool timepass = clock2.getElapsedTime().asMilliseconds() > 30.f;
    if (!aiProductionDone) {
        runAIProduction();
        aiProductionDone = true;
    }
    if (enemys.empty()) {
        playerturn = true;
        Globle_text.setString("YourTurn");
        running = false;
        AIUnitreset();
        addTurnIncome(PLAYER);
        aiProductionDone = false;
        return;
    }
    if (timepass) {
        for (auto& u : enemys) {
            u->decide();
        }
        clock2.restart();
    }
    std::size_t n = 0;
    for (auto& u : enemys) { 
        if (u->myActionPoint <= 0) {
            n++;
            if (n == enemys.size()) {
                playerturn = true;
                Globle_text.setString("YourTurn");
                running = false;
                AIUnitreset();
                addTurnIncome(PLAYER);
                aiProductionDone = false;
            }
        }
    }
}

void Game::updateRealtime(float dt)
{
    syncMazeFromTiles();
    astar.setMaze(maze);
    updateResourceControl();
    updateRealtimeEconomy(dt);
    aiController.update(*this, dt);
    realtime::updateAutoCombat(*this, dt);
}

void Game::updateRealtimeEconomy(float dt)
{
    playerIncomeTimer += dt;
    aiIncomeTimer += dt;

    if (playerIncomeTimer >= realtime::EconomyTickSeconds) {
        playerIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(PLAYER);
    }
    if (aiIncomeTimer >= realtime::EconomyTickSeconds) {
        aiIncomeTimer -= realtime::EconomyTickSeconds;
        addTurnIncome(AI);
    }
}

void Game::logicBeforeDraw()
{

    // Update all units before drawing.

    if (realtimeMode) {
        updateRealtime(std::min(realtimeFrameClock.restart().asSeconds(), 0.05f));
    }

    bool timepassed = !realtimeMode && clock.getElapsedTime().asMilliseconds() > 30.f;
    if (timepassed) {
        for (auto& test : myunits) {
            if (test->UnitState == UState::MOVING)
            {

                if (!test->mypath.empty()) {
                    test->move(test->mypath.front());
                        if(!test->mypath.empty())
                    test->mypath.pop_front();
                }
                else {
                    test->setState(UState::UNITNORMAL);
                    running = false;
                }
            }
        }
        clock.restart();
    }
    if (Base_blue) {
        Base_blue->updatemystate();
    }
    if (Base_red) {
        Base_red->updatemystate();
    }
    for (auto u = myunits.begin(); u != myunits.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = myunits.erase(u);
        }
        else {
            ++u;
        }
    }
    for (auto u = enemys.begin(); u != enemys.end(); ) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            if (MosOnUnit == u->get()) {
                MosOnUnit = nullptr;
            }
            u = enemys.erase(u);
        }
        else {
            ++u;
        }
    }
    if (!realtimeMode) {
        updateResourceControl();
    }

}

void Game::logicAfterDraw()
{
}

void Game::run()
{

    while (window.isOpen())
    {

        window.clear();
        if(gameSceneState==SCENE_GAME)
            logicBeforeInput();


        Input();

        if (gameSceneState == SCENE_GAME)
            logicBeforeDraw();

        Draw();

        logicAfterDraw();


        window.display();


    }
}

void Game::Unitsreset(list<unique_ptr<MoveableUnit>>& us) {
    for (auto& u:us)
    {
        u->setdefalut();
    }
}

void Game::startInput(Vector2i mousePos, Event event) {
    
    if (startBtn.checkMouse(mousePos, event) == RELEASE) {
        gameSceneState = SCENE_GAME;
        startBtn.setState(NORMAL);
        clear();
    }
}

void Game::GameInput(Vector2i mousePos, Event event) {
    const bool mouseInMap = mousePos.x >= 0 && mousePos.x < width && mousePos.y >= 0 && mousePos.y < height;
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape)
        {
            gameSceneState = gameSeceneState::SCENE_START;
        }
        // Reset state and restart.
        else if (event.key.code == sf::Keyboard::C)
        {
            clear();
        }
    }
    if (realtimeMode) {
        handleBuildButtons(mousePos, event);
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
        return;
    }
    if (!running && playerturn) {
        EndTurnBtn.checkMouse(mousePos, event);
        if (EndTurnBtn.btnState == RELEASE) {
            playerturn = false;
            Globle_text.setString("EnemyTurn");
            clearSelection();
            updateResourceControl();
            addTurnIncome(AI);
            aiProductionDone = false;
            Unitsreset(myunits);
            if (Base_red) {
                Base_red->reset();
            }
            EndTurnBtn.setState(NORMAL);
        }
        else {
            handleBuildButtons(mousePos, event);
            if (Base_red) {
                Base_red->checkMouse(mousePos, event);
            }
            if (Base_blue) {
                Base_blue->checkHover(mousePos,event);
            }
            for (auto& u : enemys) {
                u->checkHover(mousePos, event);
            }
            if (!myunits.empty()) {
                for (auto &u:myunits)
                {
                   u->checkMouse(mousePos, event);
                }
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
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape)
        {
            gameSceneState = gameSeceneState::SCENE_START;
        }
        // Reset state and restart.
        else if (event.key.code == sf::Keyboard::C)
        {
            clear();
        }
    }
    if (endGame.checkMouse(mousePos, event) == RELEASE) {
        gameSceneState = SCENE_START;
        endGame.setState(NORMAL);
    }
}

bool Game::spawnUnit(int team, int name, int x, int y)
{
    if (!spendCommand(team, name)) {
        return false;
    }

    switch (name)
    {
    case UName::SHOOTER:
        if (team == PLAYER) myunits.push_back(make_unique<Shooter>(team, x, y, this));
        if (team == AI) enemys.push_back(make_unique<Shooter>(team, x, y, this));
        return true;
    case UName::INFANTARY:
        if (team == PLAYER) myunits.push_back(make_unique<Infantry>(team, x, y, this));
        if (team == AI) enemys.push_back(make_unique<Infantry>(team, x, y, this));
        return true;
    case UName::CAVALRY:
        if (team == PLAYER) myunits.push_back(make_unique<Cavalry>(team, x, y, this));
        if (team == AI) enemys.push_back(make_unique<Cavalry>(team, x, y, this));
        return true;
    default:
        return false;
    }
}

void Game::handleBuildButtons(Vector2i mousePos, Event event)
{
    if (!Base_red || Base_red->UnitState != UState::UNITCLICK) {
        inf.setState(NORMAL);
        sho.setState(NORMAL);
        cav.setState(NORMAL);
        return;
    }

    if (inf.checkMouse(mousePos, event) == RELEASE) {
        Base_red->generateUnit(UName::INFANTARY);
        inf.setState(NORMAL);
    }
    if (sho.checkMouse(mousePos, event) == RELEASE) {
        Base_red->generateUnit(UName::SHOOTER);
        sho.setState(NORMAL);
    }
    if (cav.checkMouse(mousePos, event) == RELEASE) {
        Base_red->generateUnit(UName::CAVALRY);
        cav.setState(NORMAL);
    }
}

void Game::Draw()
{
    switch (gameSceneState)
    {
    case SCENE_START:
        back.setPosition(Vector2f(0,0));
        window.draw(back);
        startBtn.setPosition(600, 200);
        window.draw(startBtn);
        break;
    case SCENE_GAME: {
        const sf::View defaultView(sf::FloatRect(0.f, 0.f, config::WindowWidth, config::WindowHeight));
        sf::View gameView(defaultView);
        gameView.move(currentShakeOffset());
        window.setView(gameView);

        for (const auto& tile : tiles)
            window.draw(tile);

        if (Base_red && Base_red->UnitState == UState::UNITCLICK) {
            RectangleShape rect;
            rect.setPosition(Base_red->getPosition());
            rect.setSize(sf::Vector2f(2 * SqureSize, 2 * SqureSize));
            rect.setFillColor(sf::Color::Transparent);
            rect.setOutlineColor(sf::Color::Red);
            rect.setOutlineThickness(2.f);
            window.draw(rect);
        }

        for (const auto& pathTile : drawPaths)
            window.draw(pathTile);
        drawGridOverlay();
        drawResourceNodes();

        if (Base_red) {
            window.draw(*Base_red);
            window.draw(Base_red->UnitText);
        }
        if (Base_blue) {
            window.draw(*Base_blue);
            window.draw(Base_blue->UnitText);
        }

        UnitText.setString("");
        UnitAttack.setString("");
        UnitHP.setString("");

        for (const auto& u : enemys) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(61, 128, 206));
            window.draw(*u);
            window.draw(u->UnitText);
        }
        for (const auto& u : myunits) {
            drawUnitBase(window, Point(u->x, u->y), sf::Color(218, 76, 60));
            if (u->UnitState == UState::UNITCLICK) {
                MapPos selected(Point(u->x, u->y), tile::Choosen);
                window.draw(selected);
                string temp = "Action: "+to_string(u->myActionPoint);
                UnitText.setString(temp);
                temp = "Attack: " + to_string(u->myattack());
                UnitAttack.setString(temp);
                temp = "HP: " + to_string(u->Health);
                UnitHP.setString(temp);
            }
            window.draw(*u);
            window.draw(u->UnitText);
        }
        effects.draw(window);

        window.setView(defaultView);
        DrawSidePanel();
        
        break;
    }
    case SCEN_GAMEOVER:
        if (gameWin == false) {
            Globle_text.setString("You Lose");
            window.draw(Globle_text);
        }
        if (gameWin == true) {
            Globle_text.setString("You Win");
            window.draw(Globle_text);
        }
        endGame.setPosition(600,300);
        window.draw(endGame);
        break;
    default:
        break;
    }
    
}

void Game::drawGridOverlay()
{
    sf::VertexArray lines(sf::Lines);
    const sf::Color lineColor(108, 119, 115, 175);

    for (int x = 0; x <= width; x += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(height)), lineColor));
    }
    for (int y = 0; y <= height; y += SqureSize) {
        lines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), lineColor));
        lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(width), static_cast<float>(y)), lineColor));
    }

    window.draw(lines);
}

void Game::drawResourceNodes()
{
    for (const auto& node : resources) {
        const sf::Vector2f center(
            node.point.x * SqureSize + SqureSize / 2.f,
            node.point.y * SqureSize + SqureSize / 2.f);
        const float pulse = 1.f + std::sin(node.pulseClock.getElapsedTime().asSeconds() * 4.f) * 0.08f;
        const sf::Color color = ownerColor(node.owner);

        sf::CircleShape aura(11.f * pulse, 32);
        aura.setOrigin(11.f * pulse, 11.f * pulse);
        aura.setPosition(center);
        aura.setFillColor(sf::Color(color.r, color.g, color.b, 48));
        aura.setOutlineColor(sf::Color(color.r, color.g, color.b, 170));
        aura.setOutlineThickness(1.2f);
        window.draw(aura);

        sf::ConvexShape crystal(6);
        crystal.setPoint(0, center + sf::Vector2f(0.f, -9.f));
        crystal.setPoint(1, center + sf::Vector2f(7.f, -3.f));
        crystal.setPoint(2, center + sf::Vector2f(5.f, 6.f));
        crystal.setPoint(3, center + sf::Vector2f(0.f, 10.f));
        crystal.setPoint(4, center + sf::Vector2f(-5.f, 6.f));
        crystal.setPoint(5, center + sf::Vector2f(-7.f, -3.f));
        crystal.setFillColor(sf::Color(255, 211, 82));
        crystal.setOutlineColor(sf::Color(100, 74, 30));
        crystal.setOutlineThickness(1.2f);
        window.draw(crystal);

        sf::Text label("+2", myfont, 11);
        label.setFillColor(sf::Color(63, 49, 25));
        label.setOutlineColor(sf::Color(255, 245, 190, 180));
        label.setOutlineThickness(0.8f);
        label.setPosition(center.x + 8.f, center.y - 14.f);
        window.draw(label);
    }
}

void Game::addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    effects.addAttack(start, end, color);
}

void Game::addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size)
{
    effects.addFloatingText(myfont, position, value, color, size);
}

void Game::startScreenShake(float durationSeconds, float intensity)
{
    effects.startShake(durationSeconds, intensity);
}

sf::Vector2f Game::currentShakeOffset() const
{
    return effects.shakeOffset();
}

void Game::DrawSidePanel()
{
    EndTurnBtn.setPosition(config::ButtonX, config::EndTurnButtonY);
    inf.setPosition(config::ButtonX, config::BuildInfantryY);
    sho.setPosition(config::ButtonX, config::BuildShooterY);
    cav.setPosition(config::ButtonX, config::BuildCavalryY);

    window.draw(sidePanel);

    sf::RectangleShape accentLine(sf::Vector2f(3.f, static_cast<float>(config::WindowHeight)));
    accentLine.setPosition(static_cast<float>(config::PanelX), 0.f);
    accentLine.setFillColor(sf::Color(219, 166, 75));
    window.draw(accentLine);

    const auto drawPanelCard = [this](float y, float h, sf::Color fill, sf::Color outline) {
        sf::RectangleShape shadow(sf::Vector2f(config::PanelWidth - 20.f, h));
        shadow.setPosition(config::PanelX + 12.f, y + 4.f);
        shadow.setFillColor(sf::Color(8, 11, 10, 70));
        window.draw(shadow);

        sf::RectangleShape card(sf::Vector2f(config::PanelWidth - 24.f, h));
        card.setPosition(config::PanelX + 12.f, y);
        card.setFillColor(fill);
        card.setOutlineColor(outline);
        card.setOutlineThickness(1.4f);
        window.draw(card);
    };

    drawPanelCard(8.f, 72.f, sf::Color(47, 58, 51), sf::Color(93, 103, 81));
    drawPanelCard(86.f, 126.f, sf::Color(41, 50, 45), sf::Color(77, 92, 74));
    drawPanelCard(208.f, 72.f, sf::Color(60, 47, 31), sf::Color(197, 150, 67));
    drawPanelCard(292.f, 300.f, sf::Color(42, 49, 44), sf::Color(81, 91, 73));

    window.draw(panelTitle);
    window.draw(Globle_text);
    window.draw(UnitText);
    window.draw(UnitAttack);
    window.draw(UnitHP);
    CommandText.setString("CMD: " + std::to_string(playerCommand)
        + "/" + std::to_string(config::MaxCommand)
        + "\nGold: " + std::to_string(controlledResourceCount(PLAYER))
        + "/" + std::to_string(resources.size())
        + "\nTick: +" + std::to_string(resourceIncome(PLAYER))
        + "\nArmy: " + std::to_string(myunits.size())
        + "/" + std::to_string(config::MaxUnits));
    window.draw(CommandText);

    if (!realtimeMode) {
        window.draw(EndTurnBtn);
    }

    const bool canBuild = Base_red && Base_red->UnitState == UState::UNITCLICK;
    panelHint.setString(canBuild ? "Train squads:\nauto attack" : "Select base\nto train");
    inf.setColor(canBuild && canSpawnUnit(PLAYER, UName::INFANTARY) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    sho.setColor(canBuild && canSpawnUnit(PLAYER, UName::SHOOTER) ? sf::Color::White : sf::Color(255, 255, 255, 130));
    cav.setColor(canBuild && canSpawnUnit(PLAYER, UName::CAVALRY) ? sf::Color::White : sf::Color(255, 255, 255, 130));

    window.draw(panelHint);
    window.draw(inf);
    window.draw(infantryLabel);
    window.draw(sho);
    window.draw(shooterLabel);
    window.draw(cav);
    window.draw(cavalryLabel);
}

void Game::clear()
{
    effects.clear();
    drawPaths.clear();
    running = false;
    playerturn = true;
    gameWin = false;
    MosOnUnit = nullptr;
    playerCommand = config::StartingCommand;
    aiCommand = config::StartingCommand;
    aiProductionDone = false;
    playerIncomeTimer = 0.f;
    aiIncomeTimer = 0.f;
    aiController.reset();
    realtimeFrameClock.restart();
    resources.clear();
    Globle_text.setString(realtimeMode ? "RealTime" : "YourTurn");
    Base_red.reset();
    Base_blue.reset();
    myunits.clear();
    enemys.clear();
    tiles.clear();
    maze = vector<vector<int>>(height / SqureSize, vector<int>(width / SqureSize));
    gm.gmap(maze, width / SqureSize, height / SqureSize);
    int yp = 0;
    int xp = 0;
    for (int y = yp = 0; y < height; y += SqureSize, yp++)
    {
        for (int x = xp = 0; x < width; x += SqureSize, xp++)
        {
            tile::ID id;
            switch (maze[yp][xp])
            {
            case 1:
                id = tile::Mount;
                break;
            case 2:
                id = tile::River;
                break;
            case 3:
                id = tile::Tree;
                break;
            default:
                id = tile::Empty;
                break;
            }
            MapPos t(sf::IntRect(x, y, SqureSize, SqureSize), id);
            tiles.push_back(t);
        }
    }
    setBase();
    placeResourceNodes();

    astar = Astar(maze);

}

void Game::setBase()
{
    int w = width / SqureSize;
    int h = height / SqureSize;
    bool isok=false;
    for (int x = 4, y = 4; x < w; x++) {
        for (y = 4; y < h; y++) {
            if (x - 1 > 0 && y - 1 > 0 && x + 1 < w && y + 1 < h) {
                if (maze[y - 1][x] == 0
                    && maze[y][x + 1] == 0
                    && maze[y + 1][x] == 0
                    && maze[y][x - 1] == 0) {
                    for (int i = y - 3, j = x - 3; i < y + 3; i++) {
                        for (j = x - 3; j < x + 3; j++) {
                            setTileID(j, i, tile::Empty);
                        }
                    }
                    setTileID(x, y, tile::Red_Base);
                    setTileID(x + 1, y, tile::Red_Base);
                    setTileID(x, y + 1, tile::Red_Base);
                    setTileID(x + 1, y + 1, tile::Red_Base);
                    Red_baseP = Point(x, y);
                    Base_red = make_unique<DisMoveableUnit>(x, y, PLAYER, this);
                    art::makeUnitTexture(Base_red->mytexture, art::UnitKind::Base, art::Team::Player);
                    Base_red->setTexture(Base_red->mytexture);
                    isok = true;
                    break;
                }
            }
        }
        if (isok) break;
    }
    isok = false;
    for (int x = w - 5, y = h - 5; x >= 0; x--) {
        for (y = h - 5; y >= 0; y--) {
            if (x - 1 > 0 && y - 1 > 0 && x + 1 < w && y + 1 < h) {
                if (maze[y - 1][x] == 0
                    && maze[y][x + 1] == 0
                    && maze[y + 1][x] == 0
                    && maze[y][x - 1] == 0) {
                    Blue_baseP = Point(x, y);
                    for (int i = y - 3, j = x - 3; i < y + 3; i++) {
                        for (j = x - 3; j < x + 3; j++) {
                            setTileID(j, i, tile::Empty);
                        }
                    }
                    setTileID(x, y, tile::Blue_Base);
                    setTileID(x + 1, y, tile::Blue_Base);
                    setTileID(x, y + 1, tile::Blue_Base);
                    setTileID(x + 1, y + 1, tile::Blue_Base);
                    Base_blue = make_unique<DisMoveableUnit>(x, y, AI, this);
                    art::makeUnitTexture(Base_blue->mytexture, art::UnitKind::Base, art::Team::Enemy);
                    Base_blue->setTexture(Base_blue->mytexture);
                    isok = true;
                    break;
                }
            }
        }
        if (isok) break;
    }
}

void Game::placeResourceNodes()
{
    const int mapW = width / SqureSize;
    const int mapH = height / SqureSize;
    const std::vector<Point> targets = {
        Point(mapW / 2, mapH / 2),
        Point(mapW / 2, mapH / 3),
        Point(mapW / 2, mapH * 2 / 3),
        Point(mapW / 3, mapH / 2),
        Point(mapW * 2 / 3, mapH / 2)
    };

    for (const auto& target : targets) {
        Point best = target;
        bool found = false;
        for (int radius = 0; radius < 10 && !found; ++radius) {
            for (int y = target.y - radius; y <= target.y + radius && !found; ++y) {
                for (int x = target.x - radius; x <= target.x + radius; ++x) {
                    if (!isMapCell(x, y)) {
                        continue;
                    }
                    const auto id = tiles[y * horizontalTiles + x].getID();
                    const bool duplicate = std::any_of(resources.begin(), resources.end(), [x, y](const ResourceNode& node) {
                        return nearPoint(node.point, Point(x, y), 4);
                    });
                    if (!duplicate && id == tile::Empty && !nearPoint(Point(x, y), Red_baseP, 7) && !nearPoint(Point(x, y), Blue_baseP, 7)) {
                        best = Point(x, y);
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            continue;
        }

        // Clear a tiny capture plaza so resource fights are readable.
        for (int y = best.y - 1; y <= best.y + 1; ++y) {
            for (int x = best.x - 1; x <= best.x + 1; ++x) {
                if (isMapCell(x, y)) {
                    setTileID(x, y, tile::Empty);
                }
            }
        }
        ResourceNode node;
        node.point = best;
        node.owner = -1;
        resources.push_back(node);
    }
}

void Game::updateResourceControl()
{
    for (auto& node : resources) {
        bool playerPresent = false;
        bool enemyPresent = false;

        for (const auto& unit : myunits) {
            playerPresent = playerPresent || nearPoint(Point(unit->x, unit->y), node.point, 1);
        }
        for (const auto& unit : enemys) {
            enemyPresent = enemyPresent || nearPoint(Point(unit->x, unit->y), node.point, 1);
        }

        const int previousOwner = node.owner;
        if (playerPresent && !enemyPresent) {
            node.owner = PLAYER;
        }
        else if (enemyPresent && !playerPresent) {
            node.owner = AI;
        }

        if (node.owner != previousOwner) {
            node.pulseClock.restart();
            const sf::Vector2f pos(node.point.x * SqureSize + SqureSize / 2.f, node.point.y * SqureSize - 4.f);
            addFloatingText(pos, node.owner == PLAYER ? "CAPTURED +2" : "ENEMY +2",
                            node.owner == PLAYER ? sf::Color(255, 220, 93) : sf::Color(145, 196, 255), 12);
            startScreenShake(0.10f, 1.5f);
        }
    }
}

int Game::resourceIncome(int team) const
{
    int income = config::BaseCommandIncome;
    for (const auto& node : resources) {
        if (node.owner == team) {
            income += config::ResourceCommandIncome;
        }
    }
    return income;
}

int Game::controlledResourceCount(int team) const
{
    return static_cast<int>(std::count_if(resources.begin(), resources.end(), [team](const ResourceNode& node) {
        return node.owner == team;
    }));
}

int Game::unitCost(int name) const
{
    switch (name) {
    case UName::INFANTARY:
        return config::InfantryCost;
    case UName::SHOOTER:
        return config::ShooterCost;
    case UName::CAVALRY:
        return config::CavalryCost;
    default:
        return 0;
    }
}

int Game::commandForTeam(int team) const
{
    return team == PLAYER ? playerCommand : aiCommand;
}

bool Game::hasUnitCapacity(int team) const
{
    return team == PLAYER ? myunits.size() < MaxUnit : enemys.size() < MaxUnit;
}

bool Game::hasSpawnTile(int team) const
{
    const DisMoveableUnit* base = team == PLAYER ? Base_red.get() : Base_blue.get();
    if (base == nullptr) {
        return false;
    }

    for (int i = base->x - 1; i < base->x + 3; ++i) {
        for (int j = base->y - 1; j < base->y + 3; ++j) {
            if (!isMapCell(i, j)) {
                continue;
            }
            if (tiles[i + horizontalTiles * j].getID() == tile::Empty && !isCellReservedForSpawn(i, j)) {
                return true;
            }
        }
    }
    return false;
}

std::string Game::spawnBlockReason(int team, int name) const
{
    const int cost = unitCost(name);
    if (cost <= 0) {
        return "Bad unit";
    }
    if (!hasUnitCapacity(team)) {
        return "Unit cap";
    }
    if (commandForTeam(team) < cost) {
        return "Need CMD";
    }
    if (!hasSpawnTile(team)) {
        return "No room";
    }
    return "";
}

bool Game::canSpawnUnit(int team, int name) const
{
    return spawnBlockReason(team, name).empty();
}

bool Game::spendCommand(int team, int name)
{
    if (!canSpawnUnit(team, name)) {
        return false;
    }
    commandPool(*this, team) -= unitCost(name);
    return true;
}

void Game::runAIProduction()
{
    if (!Base_blue) {
        return;
    }

    // Real-time AI uses the same economy as the player and spends CMD in small
    // bursts instead of receiving direct units from a turn script.
    for (int i = 0; i < realtime::AIUnitsPerBurst; ++i) {
        const int playerInfantry = countUnitsNamed(myunits, UName::INFANTARY);
        const int playerShooters = countUnitsNamed(myunits, UName::SHOOTER);
        const int playerCavalry = countUnitsNamed(myunits, UName::CAVALRY);
        const int counterPick = playerShooters > playerInfantry && aiCommand >= config::CavalryCost
            ? UName::CAVALRY
            : (playerCavalry > playerShooters ? UName::INFANTARY : UName::SHOOTER);
        const int priorities[] = {
            counterPick,
            aiCommand >= config::CavalryCost && enemys.size() < myunits.size() + 2 ? UName::CAVALRY : UName::INFANTARY,
            UName::SHOOTER,
            UName::INFANTARY
        };

        bool spawned = false;
        for (int code : priorities) {
            if (Base_blue->generateUnit(code)) {
                spawned = true;
                break;
            }
        }
        if (!spawned) {
            return;
        }
    }
}

void Game::addTurnIncome(int team)
{
    const int income = resourceIncome(team);
    int& command = commandPool(*this, team);
    command = std::min(config::MaxCommand, command + income);

    Unit* base = team == PLAYER ? static_cast<Unit*>(Base_red.get()) : static_cast<Unit*>(Base_blue.get());
    if (base != nullptr) {
        addFloatingText(sf::Vector2f(base->x * SqureSize + SqureSize, base->y * SqureSize - 8.f),
                        "+" + std::to_string(income) + " CMD",
                        team == PLAYER ? sf::Color(218, 255, 134) : sf::Color(149, 203, 255), 13);
    }
}

int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}

#include "Game.h"
#include "AllUnit.h"
#include "Config.h"
#include "Map.h"
#include "ArtAssets.h"

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
    art::makeButtonTexture(tEndBtnNormal, myfont, "END TURN", art::ButtonState::Normal, sf::Vector2u(112, 40));
    art::makeButtonTexture(tEndBtnHover, myfont, "END TURN", art::ButtonState::Hover, sf::Vector2u(112, 40));
    art::makeButtonTexture(tEndBtnClick, myfont, "END TURN", art::ButtonState::Pressed, sf::Vector2u(112, 40));
    art::makeButtonTexture(tOverBtnNormal, myfont, "PLAY AGAIN", art::ButtonState::Normal, sf::Vector2u(240, 96));
    art::makeButtonTexture(tOverBtnHover, myfont, "PLAY AGAIN", art::ButtonState::Hover, sf::Vector2u(250, 100));
    art::makeButtonTexture(tOverBtnClick, myfont, "PLAY AGAIN", art::ButtonState::Pressed, sf::Vector2u(232, 92));
    art::makeButtonTexture(tinf, myfont, "Infantry", art::ButtonState::Normal, sf::Vector2u(100, 50), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfHover, myfont, "Infantry", art::ButtonState::Hover, sf::Vector2u(100, 50), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tinfClick, myfont, "Infantry", art::ButtonState::Pressed, sf::Vector2u(100, 50), art::UnitKind::Infantry, art::Team::Player);
    art::makeButtonTexture(tcav, myfont, "Cavalry", art::ButtonState::Normal, sf::Vector2u(100, 50), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavHover, myfont, "Cavalry", art::ButtonState::Hover, sf::Vector2u(100, 50), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tcavClick, myfont, "Cavalry", art::ButtonState::Pressed, sf::Vector2u(100, 50), art::UnitKind::Cavalry, art::Team::Player);
    art::makeButtonTexture(tsho, myfont, "Shooter", art::ButtonState::Normal, sf::Vector2u(100, 50), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoHover, myfont, "Shooter", art::ButtonState::Hover, sf::Vector2u(100, 50), art::UnitKind::Shooter, art::Team::Player);
    art::makeButtonTexture(tshoClick, myfont, "Shooter", art::ButtonState::Pressed, sf::Vector2u(100, 50), art::UnitKind::Shooter, art::Team::Player);

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
    setupText(Globle_text, myfont, 18, sf::Color(38, 92, 68), "GameStart", panelTextX, 44.f);
    setupText(UnitText, myfont, 16, sf::Color(38, 92, 68), "", panelTextX, 92.f);
    setupText(UnitAttack, myfont, 16, sf::Color(38, 92, 68), "", panelTextX, 118.f);
    setupText(UnitHP, myfont, 16, sf::Color(38, 92, 68), "", panelTextX, 144.f);
    setupText(panelTitle, myfont, 18, sf::Color(27, 47, 42), "Command", panelTextX, 16.f);
    setupText(panelHint, myfont, 14, sf::Color(96, 91, 78), "Select base\nto build", panelTextX, 244.f);
    setupText(infantryLabel, myfont, 13, sf::Color(27, 47, 42), "Infantry", panelTextX, config::BuildInfantryY + 52.f);
    setupText(shooterLabel, myfont, 13, sf::Color(27, 47, 42), "Shooter", panelTextX, config::BuildShooterY + 52.f);
    setupText(cavalryLabel, myfont, 13, sf::Color(27, 47, 42), "Cavalry", panelTextX, config::BuildCavalryY + 52.f);

    sidePanel.setSize(sf::Vector2f(config::PanelWidth, config::WindowHeight));
    sidePanel.setPosition(config::PanelX, 0.f);
    sidePanel.setFillColor(sf::Color(238, 232, 213));
    sidePanel.setOutlineColor(sf::Color(94, 89, 74));
    sidePanel.setOutlineThickness(1.f);

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
    if (playerturn == false) {
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
        || id == tile::Unit
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
    if (Base_blue) {
        Base_blue->generateUnit(rand()%3);
    }
    if (enemys.empty()) {
        playerturn = true;
        Globle_text.setString("YourTurn");
        running = false;
        AIUnitreset();
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
            }
        }
    }
}

void Game::logicBeforeDraw()
{

    // Update all units before drawing.

    bool timepassed = clock.getElapsedTime().asMilliseconds() > 30.f;
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
    if (!running && playerturn) {
        EndTurnBtn.checkMouse(mousePos, event);
        if (EndTurnBtn.btnState == RELEASE) {
            playerturn = false;
            Globle_text.setString("EnemyTurn");
            clearSelection();
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

void Game::spawnUnit(int team, int name, int x, int y)
{
    switch (name)
    {
    case UName::SHOOTER:
        if (team == PLAYER&&myunits.size()< MaxUnit) myunits.push_back(make_unique<Shooter>(team, x, y, this));
        if (team == AI&&enemys.size()< MaxUnit) enemys.push_back(make_unique<Shooter>(team, x, y, this));
        break;
    case UName::INFANTARY:
        if (team == PLAYER && myunits.size() < MaxUnit) myunits.push_back(make_unique<Infantry>(team, x, y, this));
        if (team == AI && enemys.size() < MaxUnit) enemys.push_back(make_unique<Infantry>(team, x, y, this));
        break;
    case UName::CAVALRY:
        if (team == PLAYER && myunits.size() < MaxUnit) myunits.push_back(make_unique<Cavalry>(team, x, y, this));
        if (team == AI && enemys.size() < MaxUnit) enemys.push_back(make_unique<Cavalry>(team, x, y, this));
        break;
    default:
        break;
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
            MapPos marker(Point(u->x, u->y), tile::Blue_Base);
            window.draw(marker);
            window.draw(*u);
            window.draw(u->UnitText);
        }
        for (const auto& u : myunits) {
            MapPos marker(Point(u->x, u->y), tile::Red_Base);
            window.draw(marker);
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
    window.draw(panelTitle);
    window.draw(Globle_text);
    window.draw(UnitText);
    window.draw(UnitAttack);
    window.draw(UnitHP);

    window.draw(EndTurnBtn);

    const bool canBuild = Base_red && Base_red->UnitState == UState::UNITCLICK;
    panelHint.setString(canBuild ? "Build once\nper turn" : "Select base\nto build");
    inf.setColor(canBuild ? sf::Color::White : sf::Color(255, 255, 255, 160));
    sho.setColor(canBuild ? sf::Color::White : sf::Color(255, 255, 255, 160));
    cav.setColor(canBuild ? sf::Color::White : sf::Color(255, 255, 255, 160));

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
    Globle_text.setString("YourTurn");
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

int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}

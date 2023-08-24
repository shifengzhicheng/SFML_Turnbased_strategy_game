#include "Game.h"
#include "AllUnit.h"
#include "Map.h"
#define SqureSize 20
#define width 1280
#define height 720
#define MaxUnit 10
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
    horizontalTiles(width / SqureSize),
    running(false),
    playerturn(true),
    gameWin(false)
{
    window.create(sf::VideoMode{ width + 120, height }, "Project_War");
    window.setFramerateLimit(60);
    Initial();
}
void Game::loadpic()
{
    //������ͼ����
    if (!tStartBtnNormal.loadFromFile("data/button/start.png")) {
        //cout << "�Ҳ���data/button/start.png" << endl;
    }//��ʼ�İ�ť
    if (!tStartBtnHover.loadFromFile("data/button/startHover.png")) {
        //cout << "�Ҳ���data/button/startHover.png" << endl;
    }//��ʼ�İ�ť����ȥ
    if (!tStartBtnClick.loadFromFile("data/button/startClick.png")) {
        //cout << "�Ҳ���data/button/startClick.png" << endl;
    }//��ʼ�İ�ť���
        //������ͼ����
    if (!tEndBtnNormal.loadFromFile("data/button/endNormal.png")) {
        //cout << "�Ҳ���data/button/start.png" << endl;
    }//��ʼ�İ�ť
    if (!tEndBtnHover.loadFromFile("data/button/endHover.png")) {
        //cout << "�Ҳ���data/button/startHover.png" << endl;
    }//��ʼ�İ�ť����ȥ
    if (!tEndBtnClick.loadFromFile("data/button/endClick.png")) {
        //cout << "�Ҳ���data/button/startClick.png" << endl;
    }//��ʼ�İ�ť���
    //������
    if (!tOverBtnNormal.loadFromFile("data/button/endGame.png")) {
        //cout << "�Ҳ���" << endl;
    }//��ʼ�İ�ť
    if (!tOverBtnHover.loadFromFile("data/button/endGame2.png")) {
        //cout << "�Ҳ���" << endl;
    }//��ʼ�İ�ť����ȥ
    if (!tOverBtnClick.loadFromFile("data/button/endGame.png")) {
        //cout << "�Ҳ���" << endl;
    }//��ʼ�İ�ť���
    if (!tinf.loadFromFile("data/button/inf.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tinfHover.loadFromFile("data/button/tinfHover.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tinfClick.loadFromFile("data/button/tinfClick.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tcav.loadFromFile("data/button/cav.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tcavHover.loadFromFile("data/button/tcavHover.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tcavClick.loadFromFile("data/button/tcavClick.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tsho.loadFromFile("data/button/sho.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tshoHover.loadFromFile("data/button/tshoHover.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!tshoClick.loadFromFile("data/button/tshoClick.png")) {
        //cout << "�Ҳ���" << endl;
    }
    if (!background.loadFromFile("data/bg/intro.jpg")) {

    }
    startBtn.setTextures(tStartBtnNormal, tStartBtnHover, tStartBtnClick);
    EndTurnBtn.setTextures(tEndBtnNormal, tEndBtnHover, tEndBtnClick);
    endGame.setTextures(tOverBtnNormal, tOverBtnHover, tOverBtnClick);
    //�����ť����
    inf.setTextures(tinf, tinfHover, tinfClick);
    cav.setTextures(tcav, tcavHover, tcavClick);
    sho.setTextures(tsho, tshoHover, tshoClick);
    back.setTexture(background);
}
void Game::Initial()
{
    loadMediaData();
    myfont.loadFromFile("data/ttf/arial.ttf");
    gameSceneState = SCENE_START;
    gameOver = false;
    Globle_text.setFont(myfont);
    Globle_text.setCharacterSize(20);
    Globle_text.setFillColor(sf::Color::Green);
    Globle_text.setString("GameStart");
    Globle_text.setPosition(1280,50);
    UnitText.setFont(myfont);
    UnitText.setCharacterSize(20);
    UnitText.setFillColor(sf::Color::Green);
    UnitText.setPosition(1280, 100);
    UnitAttack.setFont(myfont);
    UnitAttack.setCharacterSize(20);
    UnitAttack.setPosition(1280, 150);
    UnitAttack.setFillColor(sf::Color::Green);
    UnitHP.setFillColor(sf::Color::Green);
    UnitHP.setFont(myfont);
    UnitHP.setCharacterSize(20);
    UnitHP.setPosition(1280, 200);
}

void Game::loadMediaData()
{
    loadpic();
}

void Game::logicBeforeInput()
{
    //Update map information
    for (auto& tile : tiles)
    {
        if (tile.getID() == tile::Mount|| tile.getID() == tile::River|| tile.getID() == tile::Tree)
        {
            sf::Vector2i p = tile.getIndex();
            maze[p.y][p.x] = 1;
        }
    }
    astar.setMaze(maze);
    if (playerturn == false) {
        AIlogic();
    }
}

void Game::AIUnitreset()
{
    Unitsreset(enemys);
    Base_blue->reset();
}

void Game::AIlogic() {
    bool timepass = clock2.getElapsedTime().asMilliseconds() > 30.f;
    Base_blue->generateUnit(rand()%3);
    if (timepass) {
        for (auto& u : enemys) {
            u->decide();
        }
        clock2.restart();
    }
    int n = 0;
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

    //EveryUnit should complete its update before draw
    //for all unit

    bool timepassed = clock.getElapsedTime().asMilliseconds() > 30.f;
    if (timepassed) {
        for (auto& test : myunits) {
            if (test->UnitState == UState::MOVING)
            {

                if (!test->mypath.empty()) {
                    test->move(**test->mypath.begin());
                        if(!test->mypath.empty())
                    test->mypath.pop_front();
                }
                else {
                    test->setState(UState::UNITNORMAL);
                    running = false;
                }
            }
            clock.restart();
        }
    }
    Base_blue->updatemystate();
    Base_red->updatemystate();
    for (list<shared_ptr<MoveableUnit>>::iterator u = myunits.begin(); u != myunits.end(); u++) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            MosOnUnit = NULL;
            u = myunits.erase(u);
            if (u != myunits.end()) (*u)->updatemystate();
            else break;
        }
    }
    for (list<shared_ptr<MoveableUnit>>::iterator u = enemys.begin(); u != enemys.end(); u++) {
        (*u)->updatemystate();
        if ((*u)->isdead()) {
            MosOnUnit = NULL;
            u = enemys.erase(u);
            if (u != enemys.end()) (*u)->updatemystate();
            else break;
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

        window.setSize(sf::Vector2u(width+120, height));
        //�Ӵ����
        window.clear();
        if(gameSceneState==SCENE_GAME)
            logicBeforeInput();

        //��ȡ����

        Input();

        if (gameSceneState == SCENE_GAME)
            logicBeforeDraw();

        //����
        Draw();

        logicAfterDraw();


        //��ʾ
        window.display();


    }
}

void Game::Unitsreset(list<shared_ptr<MoveableUnit>> us) {
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

void Game::GameInput(Vector2i mousePos, Vector2f mousePosition, Event event) {
    if (event.key.code == sf::Keyboard::Escape)
    {
        gameSceneState = gameSeceneState::SCENE_START;
    }
    //clear all thing and restart
    else if (event.key.code == sf::Keyboard::C)
    {
        clear();
    }
    if (!running && playerturn) {
        EndTurnBtn.checkMouse(mousePos, event);
        if (EndTurnBtn.btnState == RELEASE) {
            playerturn = false;
            Globle_text.setString("EnemyTurn");
            Unitsreset(myunits);
            Base_red->reset();
            EndTurnBtn.setState(NORMAL);
        }
        else {
            Base_red->checkMouse(mousePos, event);
            Base_blue->checkHover(mousePos,event);
            for (auto& u : enemys) {
                u->checkHover(mousePos, event);
            }
            if (!myunits.empty()) {
                for (auto &u:myunits)
                {
                   u->checkMouse(mousePos, event);
                }
            }
            switch (event.type)
            {
            case sf::Event::KeyPressed:
                break;
            default:
                break;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T))
            {
                tiles[indexAt(mousePosition)].setID(tile::Tree);
                maze[mousePos.y/SqureSize][mousePos.x / SqureSize] = 0;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M))
            {
                tiles[indexAt(mousePosition)].setID(tile::Mount);
                maze[mousePos.y / SqureSize][mousePos.x / SqureSize] = 0;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            {
                tiles[indexAt(mousePosition)].setID(tile::River);
                maze[mousePos.y / SqureSize][mousePos.x / SqureSize] = 0;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            {
                tiles[indexAt(mousePosition)].setID(tile::Empty);
                maze[mousePos.y / SqureSize][mousePos.x / SqureSize] = 0;
            }
        }
    }
}


void Game::Input()
{
    sf::Event event;
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosition(mousePos.x, mousePos.y);
    while (window.pollEvent(event))
    {
        if (event.type == Event::Closed) {
            window.close();                         //�رռ��رմ���
        }
        switch (gameSceneState) {
        case SCENE_START:
            startInput(mousePos, event); break;
        case SCENE_GAME:
            GameInput(mousePos, mousePosition, event); break;
        case SCEN_GAMEOVER:
            overinput(mousePos, event); break;
        default:
            break;
        }
    }
}

void Game::overinput(sf::Vector2i mousePos, sf::Event event) {
    if (event.key.code == sf::Keyboard::Escape)
    {
        gameSceneState = gameSeceneState::SCENE_START;
    }
    //clear all thing and restart
    else if (event.key.code == sf::Keyboard::C)
    {
        clear();
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
        if (team == PLAYER&&myunits.size()< MaxUnit) myunits.push_back(shared_ptr<Shooter>(new Shooter(team, x, y, this)));
        if (team == AI&&enemys.size()< MaxUnit) enemys.push_back(shared_ptr<Shooter>(new Shooter(team, x, y, this)));
        break;
    case UName::INFANTARY:
        if (team == PLAYER && myunits.size() < MaxUnit) myunits.push_back(shared_ptr<Infantry>(new Infantry(team, x, y, this)));
        if (team == AI && enemys.size() < MaxUnit) enemys.push_back(shared_ptr<Infantry>(new Infantry(team, x, y, this)));
        break;
    case UName::CAVALRY:
        if (team == PLAYER && myunits.size() < MaxUnit) myunits.push_back(shared_ptr<Cavalry>(new Cavalry(team, x, y, this)));
        if (team == AI && enemys.size() < MaxUnit) enemys.push_back(shared_ptr<Cavalry>(new Cavalry(team, x, y, this)));
        break;
    default:
        break;
    }
}

inline void Game::Draw()
{
    switch (gameSceneState)
    {
    case SCENE_START:
        back.setPosition(Vector2f(0,0));
        window.draw(back);
        startBtn.setPosition(600, 200);
        window.draw(startBtn);
        break;
    case SCENE_GAME:
        //���ư�ť
        EndTurnBtn.setPosition(1280, 300);
        inf.setPosition(1280, 350);
        sho.setPosition(1280, 420);
        cav.setPosition(1280, 490);

        //���ư�ť
        window.draw(EndTurnBtn);
        window.draw(inf);
        window.draw(sho);
        window.draw(cav);

        //���Ʒ���
        for (const auto& tile : tiles)
            window.draw(tile);

        //���ƻ���
        if (Base_red->UnitState == UState::UNITCLICK) {
            RectangleShape *rect=new RectangleShape();
            rect->setPosition(Base_red->getPosition());
            rect->setSize(sf::Vector2f(2 * SqureSize, 2 * SqureSize));
            rect->setOutlineColor(sf::Color::Red);
            rect->setOutlineThickness(2.f);
            window.draw(*shared_ptr<RectangleShape>(rect));
        }

        for (shared_ptr<Node> te = drawPaths; te != NULL; te = te->next)
            window.draw(te->t);
        window.draw(*Base_red);
        window.draw(Base_red->UnitText);
        window.draw(*Base_blue);
        window.draw(Base_blue->UnitText);

        //���Ƶз���λ
        for (const auto& u : enemys) {
            window.draw(*shared_ptr<MapPos>(new MapPos(Point(u->x, u->y), tile::Blue_Base)));
            window.draw(*u);
            window.draw(u->UnitText);
        }
        //�����ҷ���λ
        for (const auto& u : myunits) {
            window.draw(*shared_ptr<MapPos>(new MapPos(Point(u->x, u->y), tile::Red_Base)));
            if (u->UnitState == UState::UNITCLICK) {
                window.draw(*shared_ptr<MapPos>(new MapPos(Point(u->x, u->y), tile::Choosen)));
                string temp = "Action: "+to_string(u->myActionPoint);
                UnitText.setString(temp);
                window.draw(UnitText);
                temp = "Attack: " + to_string(u->myattack());
                UnitAttack.setString(temp);
                window.draw(UnitAttack);
                temp = "HP: " + to_string(u->Health);
                UnitHP.setString(temp);
                window.draw(UnitHP);
            }
            window.draw(*u);
            window.draw(u->UnitText);
        }
        //���ƹ�������
        if (Attackdraws != NULL) {
            window.draw(*Attackdraws);
            bool timepass = clock3.getElapsedTime().asMilliseconds() > 50.f;
            if (timepass) Attackdraws = NULL;
        }
        //����Global_text
        window.draw(Globle_text);
        
        //���Ʋ��Ե�λ
        //���Ƶ�λ���ַ�
        break;
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

void Game::clear()
{
    //����״̬,���¿�ʼ
    Attackdraws = NULL;
    drawPaths = NULL;
    running = false;
    playerturn = true;
    gameWin = false;
    Globle_text.setString("GameStart");
    myunits.clear();
    enemys.clear();
    tiles.clear();
    maze = vector<vector<int>>(height / SqureSize, vector<int>(width / SqureSize));
    //�յ�ͼ����
    gm.gmap(maze, width / SqureSize, height / SqureSize);
    //MapPos����
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
    //����˫�����صĵ�ͼ����

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
                            maze[i][j] = 0;
                            tiles[i * horizontalTiles + j].setID(tile::Empty);
                        }
                    }
                    maze[y][x] = 1;
                    maze[y][x+1] = 1;
                    maze[y+1][x] = 1;
                    maze[y+1][x+1] = 1;
                    tiles[y * horizontalTiles + x].setID(tile::Red_Base);
                    tiles[y * horizontalTiles + x+1].setID(tile::Red_Base);
                    tiles[(y+1) * horizontalTiles + x].setID(tile::Red_Base);
                    tiles[(y+1) * horizontalTiles + x].setID(tile::Red_Base);
                    Red_baseP = Point(x, y);
                    Base_red = shared_ptr<DisMoveableUnit>(new DisMoveableUnit(x, y, PLAYER, this));
                    Base_red->mytexture.loadFromFile("data/unit/city_modern.png");
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
                            maze[i][j] = 0;
                            tiles[i * horizontalTiles + j].setID(tile::Empty);
                        }
                    }
                    maze[y][x] = 1;
                    maze[y][x + 1] = 1;
                    maze[y + 1][x] = 1;
                    maze[y + 1][x + 1] = 1;
                    tiles[y * horizontalTiles + x].setID(tile::Blue_Base);
                    tiles[y * horizontalTiles + x + 1].setID(tile::Blue_Base);
                    tiles[(y + 1) * horizontalTiles + x].setID(tile::Blue_Base);
                    tiles[(y + 1) * horizontalTiles + x].setID(tile::Blue_Base);
                    Base_blue = shared_ptr<DisMoveableUnit>(new DisMoveableUnit(x, y, AI, this));
                    Base_blue->mytexture.loadFromFile("data/unit/city.png");
                    Base_blue->setTexture(Base_blue->mytexture);
                    isok = true;
                    break;
                }
            }
        }
        if (isok) break;
    }
}

inline int Game::indexAt(sf::Vector2f position)
{
    auto positionX = static_cast<int>(position.x);
    auto positionY = static_cast<int>(position.y);
    positionX = positionX / SqureSize;
    positionY = positionY / SqureSize;
    return (positionY * (horizontalTiles)+positionX);
}


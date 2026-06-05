#pragma once
#include <cstddef>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"
#include "AllUnit.h"
#include "Tile.h"
#include "Astar.h"
#include "Button.h"
#include "Map.h"
#include "Effects.h"

enum gameSeceneState {
    SCENE_START,SCENE_GAME, SCEN_GAMEOVER
};
enum gamePlayer {
    PLAYER,AI
};

class Game
{
public:
    sf::Text Globle_text;
    sf::Text UnitText;
    sf::Text UnitAttack;
    sf::Text UnitHP;
    bool gameWin;

    mapgenerator gm;

    sf::Font myfont;

    std::vector<MapPos> drawPaths;

    sf::Clock clock;
    sf::Clock clock2;

    Point MousePoint;

    Unit* MosOnUnit;

    bool MousePosChanged();

    int horizontalTiles;
    

    std::unique_ptr<DisMoveableUnit> Base_red;

    std::unique_ptr<DisMoveableUnit> Base_blue;

    Point Red_baseP;

    Point Blue_baseP;

    sf::Keyboard::Key keyCode;

    std::list<std::unique_ptr<MoveableUnit>> myunits;

    std::list<std::unique_ptr<MoveableUnit>> enemys;

    Effects effects;

    std::size_t turn;

    bool playerturn;


    sf::Texture tStartBtnNormal, tStartBtnHover, tStartBtnClick;
    sf::Texture tEndBtnNormal, tEndBtnHover, tEndBtnClick;
    sf::Texture tOverBtnNormal, tOverBtnHover, tOverBtnClick;
    Button startBtn,EndTurnBtn;
    Button inf, cav, sho;
    Button endGame;
    sf::Texture tinf, tinfHover, tinfClick;
    sf::Texture tcav, tcavHover, tcavClick;
    sf::Texture tsho, tshoHover, tshoClick;
    sf::Texture background;
    sf::Sprite back;
    sf::RectangleShape sidePanel;
    sf::Text panelTitle;
    sf::Text panelHint;
    sf::Text infantryLabel;
    sf::Text shooterLabel;
    sf::Text cavalryLabel;

    Game();
    ~Game();
    

    sf::RenderWindow window;

    bool running;

    void loadpic();

    std::vector<std::vector<int> > maze;
    std::vector<MapPos> tiles;

    Astar astar;

    int gameSceneState;
    bool gameOver;

    void Initial();

    void loadMediaData();

    void AIlogic();
    
    void logicBeforeDraw();
    void logicAfterDraw();
    void logicBeforeInput();

    void run();
    void Unitsreset(std::list<std::unique_ptr<MoveableUnit>>& us);
    void AIUnitreset();
    void startInput(sf::Vector2i mousePosition, sf::Event event);

    void GameInput(sf::Vector2i mousePos, sf::Event event);

    void Draw();
    void DrawSidePanel();
    void addAttackEffect(sf::Vector2f start, sf::Vector2f end, sf::Color color);
    void addFloatingText(sf::Vector2f position, const std::string& value, sf::Color color, unsigned int size = 15);
    void startScreenShake(float durationSeconds, float intensity);
    sf::Vector2f currentShakeOffset() const;
    void handleBuildButtons(sf::Vector2i mousePos, sf::Event event);
    void clearSelection();
    void selectOnly(Unit* unit);
    void syncMazeFromTiles();
    void setTileID(int x, int y, tile::ID id);
    bool isBlockingTile(tile::ID id) const;
    bool isMapCell(int x, int y) const;
    
    void clear();

    void setBase();

    int indexAt(sf::Vector2f position);

    void Input();

    void overinput(sf::Vector2i mousePos, sf::Event event);
    
    void spawnUnit(int team,int name, int x, int y);
    
};

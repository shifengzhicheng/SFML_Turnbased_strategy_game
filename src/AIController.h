#pragma once

class Game;

class AIController
{
public:
    void reset();
    void update(Game& game, float dt);

private:
    float thinkTimer = 0.f;
};

#pragma once

#include <array>
#include <random>

class Game;

class AIController
{
public:
    void reset();
    void update(Game& game, float dt);

private:
    static constexpr int FeatureCount = 12;
    static constexpr int ActionCount = 10;

    float thinkTimer = 0.f;
    int decisionStep = 0;
    std::mt19937 rng{0xA11CEu};
    std::array<int, ActionCount> recentActions{};
};

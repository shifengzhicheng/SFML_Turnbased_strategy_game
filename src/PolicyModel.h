#pragma once

#include <array>
#include <cstddef>

class Game;

namespace policy
{
    enum class Action
    {
        Economy,
        Tech,
        Barracks,
        Tower,
        Infantry,
        Shooter,
        Cavalry,
        Siege,
        Guardian,
        Mastery,
        Wait,
        Count
    };

    inline constexpr int ActionCount = static_cast<int>(Action::Count);
    inline constexpr std::size_t ActionCountSize = static_cast<std::size_t>(Action::Count);
    inline constexpr std::size_t FeatureCount = 13;

    using FeatureVector = std::array<float, FeatureCount>;
    using WeightMatrix = std::array<FeatureVector, ActionCountSize>;

    int actionIndex(Action action);
    const char* actionName(Action action);
    int unitForAction(Action action);
    bool isUnitAction(Action action);
    bool isMacroAction(Action action);
    FeatureVector extractFeatures(const Game& game, int team);
    const WeightMatrix& baselineWeights();
    float scoreAction(const WeightMatrix& weights, Action action, const FeatureVector& features);
}

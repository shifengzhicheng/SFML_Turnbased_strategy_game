#pragma once

#include <cstddef>

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
        Wait,
        Count
    };

    inline constexpr int ActionCount = static_cast<int>(Action::Count);
    inline constexpr std::size_t ActionCountSize = static_cast<std::size_t>(Action::Count);

    int actionIndex(Action action);
    const char* actionName(Action action);
    int unitForAction(Action action);
    bool isUnitAction(Action action);
    bool isMacroAction(Action action);
}

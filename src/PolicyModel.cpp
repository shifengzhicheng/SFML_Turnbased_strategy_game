#include "PolicyModel.h"

#include "AllUnit.h"

namespace policy
{
    int actionIndex(Action action)
    {
        return static_cast<int>(action);
    }

    const char* actionName(Action action)
    {
        switch (action) {
        case Action::Economy:
            return "eco";
        case Action::Tech:
            return "tech";
        case Action::Barracks:
            return "rax";
        case Action::Tower:
            return "tower";
        case Action::Infantry:
            return "inf";
        case Action::Shooter:
            return "shoot";
        case Action::Cavalry:
            return "cav";
        case Action::Siege:
            return "siege";
        case Action::Guardian:
            return "guard";
        case Action::Wait:
            return "wait";
        case Action::Count:
        default:
            return "unknown";
        }
    }

    int unitForAction(Action action)
    {
        switch (action) {
        case Action::Shooter:
            return UName::SHOOTER;
        case Action::Cavalry:
            return UName::CAVALRY;
        case Action::Siege:
            return UName::SIEGE;
        case Action::Guardian:
            return UName::GUARDIAN;
        case Action::Infantry:
        default:
            return UName::INFANTARY;
        }
    }

    bool isUnitAction(Action action)
    {
        return action == Action::Infantry
            || action == Action::Shooter
            || action == Action::Cavalry
            || action == Action::Siege
            || action == Action::Guardian;
    }

    bool isMacroAction(Action action)
    {
        return action == Action::Economy
            || action == Action::Tech
            || action == Action::Barracks
            || action == Action::Tower;
    }
}

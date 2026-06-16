#include "UnitFactory.h"

#include "AllUnit.h"

std::unique_ptr<MoveableUnit> createMoveableUnit(int team, int name, int x, int y, Game* game)
{
    switch (name) {
    case UName::SHOOTER:
        return std::make_unique<Shooter>(team, x, y, game);
    case UName::INFANTARY:
        return std::make_unique<Infantry>(team, x, y, game);
    case UName::CAVALRY:
        return std::make_unique<Cavalry>(team, x, y, game);
    case UName::SIEGE:
        return std::make_unique<Siege>(team, x, y, game);
    case UName::GUARDIAN:
        return std::make_unique<Guardian>(team, x, y, game);
    default:
        return nullptr;
    }
}

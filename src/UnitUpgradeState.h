#pragma once

#include "UnitDefinition.h"

#include <array>

struct UnitMasteryState
{
    std::array<int, TrainableUnitCount> levels{};
};


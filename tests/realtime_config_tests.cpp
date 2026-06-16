#include "RealtimeConfig.h"

#include <cassert>

int main()
{
    // Larger step time means lower speed. Keep enough separation that the
    // player can feel cavalry rotations and punish unsupported siege.
    assert(realtime::CavalryStepSeconds < realtime::InfantryStepSeconds);
    assert(realtime::InfantryStepSeconds < realtime::ShooterStepSeconds);
    assert(realtime::ShooterStepSeconds < realtime::GuardianStepSeconds);
    assert(realtime::GuardianStepSeconds < realtime::SiegeStepSeconds);
    assert(realtime::SiegeStepSeconds >= realtime::CavalryStepSeconds * 2.5f);
    assert(realtime::SiegeStepSeconds >= realtime::InfantryStepSeconds * 1.8f);

    return 0;
}

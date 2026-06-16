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
    assert(realtime::CavalryStepSeconds <= realtime::InfantryStepSeconds * 0.60f);
    assert(realtime::GuardianStepSeconds >= realtime::InfantryStepSeconds * 1.55f);
    assert(realtime::SiegeStepSeconds >= realtime::CavalryStepSeconds * 4.0f);
    assert(realtime::SiegeStepSeconds >= realtime::InfantryStepSeconds * 2.2f);

    return 0;
}

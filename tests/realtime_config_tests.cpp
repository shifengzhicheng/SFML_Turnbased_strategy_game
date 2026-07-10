#include "RealtimeConfig.h"

#include <cstdlib>
#include <iostream>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }
}

int main()
{
    // Larger step time means lower speed. Keep enough separation that the
    // player can feel cavalry rotations and punish unsupported siege.
    require(realtime::CavalryStepSeconds < realtime::InfantryStepSeconds,
            "cavalry should move faster than infantry");
    require(realtime::InfantryStepSeconds < realtime::ShooterStepSeconds,
            "infantry should move faster than shooters");
    require(realtime::ShooterStepSeconds < realtime::GuardianStepSeconds,
            "shooters should move faster than guardians");
    require(realtime::GuardianStepSeconds < realtime::SiegeStepSeconds,
            "guardians should move faster than siege");
    require(realtime::CavalryStepSeconds <= realtime::InfantryStepSeconds * 0.60f,
            "cavalry speed advantage should be visible");
    require(realtime::GuardianStepSeconds >= realtime::InfantryStepSeconds * 1.55f,
            "guardians should feel substantially heavier than infantry");
    require(realtime::SiegeStepSeconds >= realtime::CavalryStepSeconds * 4.0f,
            "siege should not rotate as quickly as cavalry");
    require(realtime::SiegeStepSeconds >= realtime::InfantryStepSeconds * 2.2f,
            "siege should require escorts");
    return 0;
}

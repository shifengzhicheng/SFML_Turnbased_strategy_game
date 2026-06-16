#include "AllUnit.h"
#include "Config.h"
#include "RealtimeConfig.h"
#include "UnitDefinition.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) {
            std::cerr << "unit_definition_tests: " << message << '\n';
            std::exit(1);
        }
    }
}

int main()
{
    const auto& definitions = unitDefinitions();
    require(definitions.size() == 5, "the roster should expose five trainable unit definitions");

    std::set<int> seenNames;
    for (const UnitDefinition& definition : definitions) {
        require(seenNames.insert(definition.unitName).second, "unit definitions should not duplicate ids");
        require(definition.debugName != nullptr && std::string(definition.debugName).size() > 0,
                "unit definitions should have readable debug names");
        require(definition.maxHealth > 0, "unit health should be positive");
        require(definition.attackDamage > 0, "unit damage should be positive");
        require(definition.attackRange > 0, "unit range should be positive");
        require(definition.commandCost > 0, "unit cost should be positive");
        require(definition.moveStepSeconds > 0.f, "unit move cadence should be positive");
        require(definition.attackCooldownSeconds > 0.f, "unit attack cooldown should be positive");
        require(definition.trainSeconds > 0.f, "unit training time should be positive");
        require(definition.artKind != art::UnitKind::None, "unit art kind should be explicit");
    }

    require(findUnitDefinition(UName::BASE) == nullptr, "base is not a trainable unit definition");
    require(unitDefinition(UName::INFANTARY).commandCost == config::InfantryCost,
            "infantry cost should stay sourced from Config");
    require(unitDefinition(UName::SHOOTER).trainSeconds == realtime::ShooterTrainSeconds,
            "shooter train time should stay sourced from RealtimeConfig");
    require(unitDefinition(UName::CAVALRY).artKind == art::UnitKind::Cavalry,
            "cavalry should point to cavalry art");
    require(unitDefinition(UName::SIEGE).attackRange == config::SiegeRange,
            "siege range should stay sourced from Config");
    require(unitDefinition(UName::GUARDIAN).maxHealth == config::GuardianHealth,
            "guardian health should stay sourced from Config");

    bool threw = false;
    try {
        (void)unitDefinition(-999);
    }
    catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "unknown unitDefinition lookups should fail loudly");

    return 0;
}

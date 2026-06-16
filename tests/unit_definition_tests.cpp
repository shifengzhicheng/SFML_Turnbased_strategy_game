#include "AllUnit.h"
#include "BuildingDefinition.h"
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
        require(definition.requiredBarracks >= 1, "trainable units should require at least one barracks");
        require(definition.requiredEconomyLevel >= 0, "economy unlock requirement should be non-negative");
        require(definition.requiredTechLevel >= 0, "tech unlock requirement should be non-negative");
        require(definition.artKind != art::UnitKind::None, "unit art kind should be explicit");
    }

    require(findUnitDefinition(UName::BASE) == nullptr, "base is not a trainable unit definition");
    require(unitDefinition(UName::INFANTARY).commandCost == config::InfantryCost,
            "infantry cost should stay sourced from Config");
    require(unitDefinition(UName::SHOOTER).trainSeconds == realtime::ShooterTrainSeconds,
            "shooter train time should stay sourced from RealtimeConfig");
    require(unitDefinition(UName::SHOOTER).unlockByEconomyOrTech,
            "shooter should unlock through either economy or tech");
    require(unitDefinition(UName::CAVALRY).artKind == art::UnitKind::Cavalry,
            "cavalry should point to cavalry art");
    require(unitDefinition(UName::CAVALRY).requiredBarracks == 2,
            "cavalry should require two barracks");
    require(unitDefinition(UName::CAVALRY).commandCost >= unitDefinition(UName::SHOOTER).commandCost * 2,
            "cavalry should be a meaningful mid-tier investment");
    require(unitDefinition(UName::SIEGE).attackRange == config::SiegeRange,
            "siege range should stay sourced from Config");
    require(unitDefinition(UName::SIEGE).commandCost >= unitDefinition(UName::SHOOTER).commandCost * 3,
            "siege should not be cheap enough to spam behind towers");
    require(!unitDefinition(UName::SIEGE).unlockByEconomyOrTech,
            "siege should require both economy and tech pacing gates");
    require(unitDefinition(UName::GUARDIAN).maxHealth == config::GuardianHealth,
            "guardian health should stay sourced from Config");
    require(unitDefinition(UName::GUARDIAN).commandCost >= unitDefinition(UName::CAVALRY).commandCost * 2,
            "guardian should remain a late-game commitment");
    require(unitDefinition(UName::GUARDIAN).requiredTechLevel == 7,
            "guardian should stay a late-game unlock");

    bool threw = false;
    try {
        (void)unitDefinition(-999);
    }
    catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "unknown unitDefinition lookups should fail loudly");

    const auto& buildings = buildingDefinitions();
    require(buildings.size() == 2, "the building table should expose two buildable structures");
    require(buildingDefinition(building::Barracks).commandCost == config::BarracksCost,
            "barracks cost should stay sourced from Config");
    require(buildingDefinition(building::Barracks).buildSeconds == realtime::BarracksBuildSeconds,
            "barracks build time should stay sourced from RealtimeConfig");
    require(buildingDefinition(building::DefenseTower).maxHealth == config::DefenseTowerHealth,
            "tower health should stay sourced from Config");

    threw = false;
    try {
        (void)buildingDefinition(-999);
    }
    catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "unknown buildingDefinition lookups should fail loudly");

    return 0;
}

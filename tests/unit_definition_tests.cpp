#include "AllUnit.h"
#include "BuildingDefinition.h"
#include "Config.h"
#include "RealtimeConfig.h"
#include "UnitDefinition.h"
#include "UnitUpgradeDefinition.h"

#include <algorithm>
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
    require(config::InfantryCost >= 24 && config::InfantryHealth <= 170 && config::InfantryDamage <= 36,
            "infantry should not be cheap enough to dominate no-collision swarm fights");
    require(unitDefinition(UName::SHOOTER).trainSeconds == realtime::ShooterTrainSeconds,
            "shooter train time should stay sourced from RealtimeConfig");
    require(unitDefinition(UName::SHOOTER).unlockByEconomyOrTech,
            "shooter should unlock through either economy or tech");
    require(unitDefinition(UName::CAVALRY).artKind == art::UnitKind::Cavalry,
            "cavalry should point to cavalry art");
    require(unitDefinition(UName::CAVALRY).requiredBarracks == 2,
            "cavalry should require two barracks");
    require(unitDefinition(UName::CAVALRY).maxHealth >= 450,
            "cavalry should have enough health to survive diving ahead");
    require(unitDefinition(UName::CAVALRY).commandCost >= unitDefinition(UName::SHOOTER).commandCost * 2,
            "cavalry should be a meaningful mid-tier investment");
    require(unitDefinition(UName::SIEGE).attackRange == config::SiegeRange,
            "siege range should stay sourced from Config");
    require(config::InfantryRange == 1
            && config::CavalryRange == 1
            && config::GuardianRange == 1
            && config::ShooterRange == 3
            && config::SiegeRange == 5,
            "baseline ranges should match the readable melee/shooter/siege contract");
    require(config::ShooterMaxRange == 5,
            "shooter range perks should have a clear hard cap");
    require(unitDefinition(UName::SIEGE).commandCost >= unitDefinition(UName::SHOOTER).commandCost * 3,
            "siege should not be cheap enough to spam behind towers");
    require(!unitDefinition(UName::SIEGE).unlockByEconomyOrTech,
            "siege should require both economy and tech pacing gates");
    require(unitDefinition(UName::GUARDIAN).maxHealth == config::GuardianHealth,
            "guardian health should stay sourced from Config");
    require(unitDefinition(UName::GUARDIAN).commandCost >= unitDefinition(UName::CAVALRY).commandCost * 2,
            "guardian should remain a late-game commitment");
    require(unitDefinition(UName::GUARDIAN).maxHealth >= 900,
            "guardian should be sturdy enough to anchor a mixed army");
    require(unitDefinition(UName::GUARDIAN).requiredTechLevel == 7,
            "guardian should stay a late-game unlock");
    require(config::InfantrySeekRange > config::InfantryRange
            && config::ShooterSeekRange > config::ShooterRange
            && config::CavalrySeekRange > config::CavalryRange
            && config::SiegeSeekRange > config::SiegeRange
            && config::GuardianSeekRange > config::GuardianRange,
            "seek ranges should always be wider than attack ranges");
    require(config::CavalrySeekRange >= config::InfantrySeekRange + 2,
            "cavalry should peel toward nearby fights before over-diving alone");
    require(config::SiegeSeekRange >= config::SiegeRange + 3,
            "siege should look far enough ahead to escort itself near towers");
    require(unitMasteryUpgradeCost(UName::INFANTARY, 1) > unitMasteryUpgradeCost(UName::INFANTARY, 0),
            "unit mastery costs should grow forever");
    for (const UnitDefinition& definition : definitions) {
        require(unitMasteryUpgradeCost(definition.unitName, 0) <= std::max(60, definition.commandCost * 5),
                "first mastery purchase should be reachable soon after a unit unlocks");
        require(unitMasteryUpgradeCost(definition.unitName, 10) <= definition.commandCost * 28,
                "level 10 mastery should be a strategic purchase, not an economy wall");
        require(unitMasteryUpgradeCost(definition.unitName, 20) <= definition.commandCost * 90,
                "level 20 mastery should remain reachable in long games");
    }
    require(unitMasteryStatMultiplier(UName::SIEGE, 7) >= 1.f + 7.f * config::MasteryStatBonusPerLevel - 0.001f,
            "unit mastery should expose the expected +10 percent per level multiplier");
    require(config::MaxCommand > unitMasteryUpgradeCost(UName::INFANTARY, 20),
            "CMD bank should not create a practical hard cap for late mastery");

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

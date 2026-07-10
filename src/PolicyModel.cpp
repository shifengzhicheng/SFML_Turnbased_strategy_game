#include "PolicyModel.h"

#include "AllUnit.h"
#include "Building.h"
#include "Config.h"
#include "Game.h"

#include <algorithm>

namespace policy
{
    namespace
    {
        const WeightMatrix BaselineWeights = {{
            {{ 0.10f,  0.70f,  1.45f, -1.35f, -0.28f,  0.08f, -0.10f,  0.18f, -0.12f,  0.18f,  0.00f, -0.65f,  0.16f}},
            {{-0.05f,  0.92f,  1.18f,  0.10f, -1.18f,  0.12f, -0.05f,  0.10f,  0.03f,  0.10f, -0.18f, -0.30f,  0.20f}},
            {{ 0.18f,  0.62f,  0.95f,  0.38f,  0.20f, -1.25f, -0.10f, -0.15f,  0.28f,  0.16f, -0.06f, -0.22f,  0.24f}},
            {{-0.70f,  0.35f,  0.70f,  0.02f,  0.18f,  0.05f, -1.00f, -0.08f,  0.15f, -0.35f,  0.00f,  1.35f, -0.18f}},
            {{ 0.20f, -0.35f,  0.62f, -0.10f, -0.08f,  0.50f,  0.00f, -0.70f,  0.70f, -0.08f, -0.04f,  0.32f, -0.10f}},
            {{ 0.08f, -0.20f,  0.82f,  0.20f,  0.04f,  0.45f,  0.00f, -0.58f,  0.58f, -0.05f, -0.04f,  0.12f,  0.24f}},
            {{-0.08f,  0.12f,  1.02f,  0.35f,  0.24f,  0.55f, -0.03f, -0.48f,  0.45f,  0.02f, -0.08f,  0.18f,  0.35f}},
            {{-0.42f,  1.10f,  1.05f,  0.50f,  0.80f,  0.78f, -0.10f, -0.35f,  0.35f,  0.00f, -0.18f, -0.05f,  1.10f}},
            {{-0.58f,  0.95f,  1.02f,  0.55f,  0.82f,  0.80f, -0.05f, -0.42f,  0.52f, -0.02f, -0.10f,  0.60f,  0.25f}},
            {{-0.30f,  0.98f,  1.20f,  0.42f,  0.72f,  0.42f, -0.02f, -0.20f,  0.16f,  0.08f, -0.08f, -0.12f,  0.36f}},
            {{-0.48f,  0.05f, -0.95f,  0.06f,  0.02f, -0.18f,  0.00f,  0.82f, -0.70f,  0.22f,  0.10f, -0.50f,  0.12f}}
        }};
    }

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
        case Action::Mastery:
            return "mastery";
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
            || action == Action::Tower
            || action == Action::Mastery;
    }

    FeatureVector extractFeatures(const Game& game, int team)
    {
        const int enemy = team == PLAYER ? AI : PLAYER;
        const auto& ownUnits = team == PLAYER ? game.myunits : game.enemys;
        const auto& enemyUnits = team == PLAYER ? game.enemys : game.myunits;
        const DisMoveableUnit* ownBase = team == PLAYER ? game.Base_red.get() : game.Base_blue.get();
        const DisMoveableUnit* enemyBase = team == PLAYER ? game.Base_blue.get() : game.Base_red.get();
        const Point ownBasePoint = team == PLAYER ? game.Red_baseP : game.Blue_baseP;
        const Point enemyBasePoint = team == PLAYER ? game.Blue_baseP : game.Red_baseP;
        const int ownBarracksCap = std::max(1, game.buildingCap(team, building::Barracks));
        const int ownTowerCap = std::max(1, game.buildingCap(team, building::DefenseTower));

        return {{
            1.f,
            std::clamp(game.gameTimeSeconds / 900.f, 0.f, 1.5f),
            static_cast<float>(std::min(game.commandForTeam(team), config::CommandFeatureScale)) / static_cast<float>(config::CommandFeatureScale),
            static_cast<float>(game.economyLevelForTeam(team)) / static_cast<float>(config::MaxEconomyLevel),
            static_cast<float>(team == PLAYER ? game.playerUpgradeLevel : game.aiUpgradeLevel) / static_cast<float>(config::MaxTechLevel),
            static_cast<float>(game.completedBuildingCount(team, building::Barracks)) / static_cast<float>(ownBarracksCap),
            static_cast<float>(game.totalBuildingCount(team, building::DefenseTower)) / static_cast<float>(ownTowerCap),
            static_cast<float>(ownUnits.size()) / static_cast<float>(config::MaxUnits),
            static_cast<float>(enemyUnits.size()) / static_cast<float>(config::MaxUnits),
            static_cast<float>(ownBase ? ownBase->Health : 0) / 4000.f,
            static_cast<float>(enemyBase ? enemyBase->Health : 0) / 4000.f,
            static_cast<float>(game.unitsNearPoint(enemy, ownBasePoint, 13)) / 20.f,
            static_cast<float>(game.unitsNearPoint(team, enemyBasePoint, 13)) / 20.f
        }};
    }

    const WeightMatrix& baselineWeights()
    {
        return BaselineWeights;
    }

    float scoreAction(const WeightMatrix& weights, Action action, const FeatureVector& features)
    {
        const auto& row = weights[static_cast<std::size_t>(actionIndex(action))];
        float score = 0.f;
        for (std::size_t i = 0; i < FeatureCount; ++i) {
            score += row[i] * features[i];
        }
        return score;
    }
}

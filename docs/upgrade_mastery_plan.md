# Unit Mastery And Mechanic Perk Refactor Plan

Goal: split late-game growth into clear systems: tech rewards change mechanics, CMD mastery grows unit numbers forever, and both systems share a single baseline-driven stat resolver.

## Checklist

- [x] Add data model for per-team, per-unit infinite CMD mastery.
- [x] Add mastery definitions and cost curve sourced from unit baselines.
- [x] Add a stat resolver so final damage/health/range/cooldown are derived from baseline + tech + mastery + mechanism perks.
- [x] Add game operations for buying unit mastery and keep UI, scripted players, and AI on the same dispatcher.
- [x] Add sidebar and hotkey entry points for mastery purchases.
- [x] Convert three-choice rewards from raw stat bonuses into mechanism-changing perks.
- [x] Implement mechanism hooks: shooter multi-target/range cap, cavalry counter immunity, guardian taunt, siege/tower interactions.
- [x] Apply requested attack ranges: melee 1, shooter 3 with perk cap 5, siege 5, tower unchanged.
- [x] Make defense towers scale with flat + max-health percent damage and improve anti-siege priority.
- [x] Remove siege-safe tower stand-point preference so siege is not a free tower delete button.
- [x] Teach AI and scripted simulation to invest in mastery without starving production.
- [x] Add regression tests for mastery costs, infinite levels, stat baseline consistency, perk mechanisms, ranges, and tower behavior.
- [x] Run build, ctest, and accelerated scripted simulations.
- [x] Mark this checklist complete after validation and commit the finished refactor.

## Validation

- [x] `cmake --build build -j$(sysctl -n hw.ncpu)`
- [x] `ctest --test-dir build --output-on-failure`
- [x] `TBS_MAP_SEED=20260617 ./build/sfml_tbs --simulate-plan balanced 900 --simulate-dt 0.25`
- [x] `TBS_MAP_SEED=20260617 ./build/sfml_tbs --simulate-plan greedy 900 --simulate-dt 0.25`
- [x] `./build/sfml_tbs --train-policies 2 60 20260617 --simulate-dt 0.25`

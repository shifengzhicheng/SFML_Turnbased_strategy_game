# Realtime Auto-Battle Refactor Plan

The new game loop is a light RTS with automatic combat. The player should make
high-level economic and production choices while units handle pathing and combat.

## File organization

- `RealtimeConfig.h`: real-time balance constants such as income cadence, AI
  cadence, movement speed, attack cooldowns, and unit-cap tuning.
- `AutoCombat.h/.cpp`: unit autopilot. It chooses targets, refreshes A* paths,
  steps units, and triggers attacks on cooldown.
- `AIController.h/.cpp`: AI economy and production decisions. It should use the
  same resource and production rules as the player.
- Future `Worker`, `Building`, `ProductionQueue`, and `Economy` files will be
  added after the automatic battle core is stable.

## Step-by-step implementation

1. Convert the game loop to real-time updates while keeping the current map,
   bases, resources, unit art, A*, and attack effects.
2. Increase the unit cap and make combat units auto-path toward enemies and
   enemy bases. Units should attack automatically on cooldown instead of spending
   action points.
3. Make resources tick over time instead of only at turn boundaries. Existing
   resource points remain as a temporary bridge until extractor buildings exist.
4. Move AI production to a timed controller so the AI periodically expands army
   count without requiring a turn state.
5. Add resource buildings and workers: clicking a resource creates a build task,
   the base sends a worker, and finished extractors increase income.
6. Add barracks and production queues: multiple barracks run parallel queues,
   which increases spawn speed and enables larger army waves.
7. Add unit unlocks and upgrades so the player focuses on economy, tech, and
   production rather than micro-controlling units.

## Current milestone

This milestone implements steps 1-4 only. It intentionally keeps build buttons
and direct base spawning as a temporary bridge so the game remains playable while
worker/building systems are introduced in later commits.

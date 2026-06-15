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
- `Building.h/.cpp`, `Worker.h/.cpp`, `ProductionQueue.h`: construction,
  extractor harvesting, and per-barracks production queues.
- `PathfindingService.h/.cpp`: background A* worker. It only consumes snapshots
  of the maze and returns paths; SFML state is still touched on the main thread.

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

The current milestone implements the full first RTS loop:

- Click a gold node to queue an extractor. The base auto-assigns workers; extra
  drones now cost CMD and have a training cadence instead of appearing for free.
- Select the base and click open land to queue a barracks. Workers prioritize
  construction, then return to mining when build demand is satisfied. Barracks
  and towers must be inside friendly build influence from the base or a finished
  structure, so forward production requires a real expansion foothold.
- Unit buttons enqueue production into the least-loaded completed barracks.
  Multiple barracks therefore train in parallel and increase army throughput.
- The AI follows the same rules: expand to resources, build barracks, upgrade,
  queue unlocked units, then let automatic pathing/combat resolve the fight.
- The pacing is intentionally slower than the prototype: income ticks, movement,
  attacks, construction, unit training, and AI decisions leave time for the
  player to read the map and make economy choices.
- Press `H` or click `HELP` to open the in-game tutorial with controls, unlocks,
  and automation rules.
- Barracks and towers now have tech-scaled caps, so production ramps from one
  barracks into a stronger late game instead of exploding immediately.
- Tech upgrades advance through levels; each level increases global unit and
  tower damage through `TechDamageBonus`.
- Unit/building costs were raised so early-game choices matter: opening mine,
  first barracks, first tower, and first tech level are now real trade-offs.
- Two advanced units extend the late game: Siege unlocks at LEVEL 2 as an
  anti-building artillery piece, while Guardian unlocks at LEVEL 3 as a slow
  frontline anchor.
- Resource nodes now support contention and capture. A mine stops paying while
  contested, and ownership flips after one side holds the area long enough.
- Buildings have HP. Units can raid towers, mines, and barracks; destroyed mines
  return the tile to a rebuildable resource point.
- Resource layout now includes safer natural nodes plus higher-value central and
  flank nodes, giving the map clearer expansion and conflict zones.
- The map generator carves a main lane plus upper/lower flanks and open resource
  plazas, then adds tributaries, ridge fingers, and broken cover rings around
  resources to make terrain more varied without blocking core routes.
- Combat autopilot rallies early armies through valuable neutral/resource zones
  before base-racing, which makes mines and captures visible in normal play.
- Run `TBS_LOG=1 ./sfml_tbs --simulate 120` from a build directory to print a
  120-second AI/economy timeline without entering the interactive loop.
- Run `TBS_LOG=1 ./sfml_tbs --simulate-player 180` to pit the AI against a
  scripted player opener and inspect pacing under pressure.

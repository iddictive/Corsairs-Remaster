# World-map traffic

## October 5 coherent fleet model — implementation contract

The next batch replaces scene-specific guesses with one fleet descriptor. This
section is the implementation contract, not an installation receipt. The
canonical build and stage-only launcher deliver the engine and script batch;
installation requires the player application to be closed.

- **Identity and composition:** ordinary fleets select their hull roster once
  with the existing nation/class/encounter eligibility rules. Map decisions,
  reconnaissance and sea generation consume that roster; gaining a player rank
  must not silently regenerate an existing fleet. Legacy ordinary descriptors
  acquire a roster on refresh; quests and ALONE retain their authored contracts.
- **Readiness:** derive strength from the entire active fleet, hull role/class,
  hull/sail condition, crew and usable guns/ammunition. Use the same calculation
  for generated fleets and the player. No player rank or personal health enters
  tactical power. Losses and depleted cargo survive sea exit; sunk/captured ships
  must not return through a fresh encounter generation.
- **Purpose:** commerce travels between eligible ports, patrols defend a bounded
  home territory, raiders search shipping routes for profitable targets. Fixed
  captain temperament changes tolerated risk, not hostility or ship stats.
  Most raiders favour merchants; a small reckless minority accepts unfavourable
  odds. Recklessness is assigned once, never rerolled every second.
- **Knowledge and commitment:** acquire only visible opponents. Persist target
  and attack/escape/route intent, including the strength observed when committing.
  A scene transition alone is not a new reason to retreat. Loss of contact,
  actual losses, ammunition exhaustion or newly arrived superior forces can
  change the decision. Do not force doomed ships to fight or lock quest tasks.
- **Battle sides:** sum fleets actually fighting the same opposing side. Two
  hostile squadrons attacking the player do not evaluate themselves as isolated
  ships. One nearby home patrol can rescue its nation's commerce; unrelated
  neutral traffic and distant fleets are not invisible reinforcements.
- **Map lifecycle:** retain the existing finite population and island navigation.
  Arrivals retire commerce and let bounded scheduling create new departures;
  patrol/raider routes continue. Preserve survivor roster/state when entering
  and leaving sea. Paused play freezes simulation. This batch is a fleet
  simulation while the map runs, not a hidden economy or off-screen cannon solver.
- **Reconnaissance UI:** retain the game's parchment/flag vocabulary. Show each
  observed group once with purpose, visible composition and an approximate
  relative-threat sentence grounded in fleet data. Do not present a guaranteed
  win probability or expose hidden cargo/personality as perfect knowledge.
- **Actions:** entering the sea is independent of choosing a target. The Enter
  quick menu exposes both plain sea entry and targeted pursuit when applicable;
  the encounter dialog names attack, pursuit or approach truthfully. Plain entry
  imports nearby traffic without assigning player aggression. Hostile ships
  already committed to attack remain dangerous; this is not a combat immunity
  button. Barrels/boats, storm/story restrictions and deliberate attacks remain.

Falsification scenarios: a reckless raider follows a stronger player and retains
its attack after sea entry; a cautious raider avoids it before contact; a patrol
plus another attacking squadron evaluates their actual combined side; a crippled
or dry-ammunition ship can escape despite commitment. Enter sea beside friendly
shipping and collect a barrel without selecting attack. Re-enter after damaging,
sinking or capturing a ship: survivors keep losses and dead ships do not respawn.
Old saves receive only missing ordinary fields and keep player/quest state.

Reuse the existing eligible hull selector, group task API, encounter descriptors,
navigation, sea import and UI widgets. Replacing the whole world-map engine or
introducing a parallel AI registry would duplicate those owners without closing
a further requirement. Exact native/script probes and canonical staging precede
installation; the player owns final scene/balance acceptance.

## Contract and owners

The map should contain trade travelling between ports, naval patrols and pirate
raiders with local targets. Battles should grow out of those encounters instead
of appearing already engaged around the player. Seeing farther must not give
hostile captains a larger detection radius or make every ship chase the hero.

`tools/metal_living_caribbean.py` owns the exact-hash map-script composition;
`experiments/native-metal/build.sh` owns the ordered engine patch stack. New
traffic uses the existing merchant navigation, encounter descriptions, fleet
generator and paired-fleet sea import. Quest encounters retain their existing
constructors, routing, timers and callbacks. Installed files are untouched while
the player is running the game; the canonical launcher serializes delivery.

## Candidate model

Traffic exists across the whole map. Native population scheduling creates
departures at real ports and keeps a bounded resident population; it does not
spawn traffic in a ring around the hero. Scripts remain the fleet-content and
diplomacy bridge, including existing story/quest encounters. Navigation,
target acquisition, escape, patrol limits and battle timing belong to the engine.

- Most traffic is commerce. A caravan represents an actual generated fleet,
  not several decorative copies of one ship. Routes use live port ownership
  and relations; patrols operate around their home port.
- Raiders prefer trade. Ordinary captains tolerate an enemy up to 1.10 times
  their side's power; 10% are bold (1.40), 2% reckless (1.65). Temperament is
  assigned once. Patrols use 1.10. Visible same-nation fleets already attacking
  the same opponent count as allies; distant traffic does not. After committing,
  retreat requires changed strength (20% own loss or 25% enemy growth) and an
  unfavourable comparison. Trading ships flee.
- Player strength uses every active companion-slot ship, class and hull role,
  weighted by current hull/sail condition (75%/25%). Parked ships and personal
  stats do not count. Ordinary raiders compare the player and NPC prey in the
  same scoring pass; superior player fleets cause escape and cannot force a sea
  encounter through the native contact flag. The player can still enter combat.
  Map entry rebuilds the estimate before creating the entity, including old saves;
  creation assigns awareness immediately and legacy creation refreshes before
  the next native update. No random risk reroll occurs during target evaluation.
- Old ordinary pirate and naval `Follow` encounters acquire the same roster,
  awareness and commitment checks. New ordinary Follow spawning is disabled:
  pursuit comes from resident traffic. Quest/qID/ALONE constructors and tasks
  retain their authored behaviour.
- Opposing fleets become a battle only after approaching each other. Both
  original descriptions and identities enter the sea scene together. Battles
  last at least 24 game hours using the native map clock (up to 48 for close
  forces), with pause freezing combat progress. One same-nation home patrol can
  rescue commerce already fighting pirates; another patrol cannot join that fight.
  All three identities import together even when the sea-entry radius cuts
  through the battle. Twice the enemy's combined strength wins deterministically;
  closer forces use squared-strength odds. Captured commerce expires; patrol
  and pirate survivors keep their stable slots, losses and ammunition wear.
  Overwhelming winners lose no hulls. Surviving losers escape with a 180-second
  cooldown; winners resume their route with a 90-second cooldown.
- Population caps distinguish trade, patrols, pirates and legacy pursuers.
  Increasing all random rates and the global cap is explicitly rejected.
- Saved routes, targets, roster slots and battle state survive map exit/re-entry.
  Old ordinary descriptors gain only missing fields; quest state is excluded.

Strength uses the selected base hulls and their readiness: class/war role,
hull/sails (75%/25%), crew, intact guns and usable ammunition. The same function
reads active player ships and imported NPC ships. Exact hulls are chosen once,
not rerolled with the player's rank. Sea exit saves Ship/RealShip values without
old runtime indexes, including cargo and captured/sunk tombstones; re-entry
allocates fresh runtime slots and restores those values. Aggregate map wear is
applied once and reset after exact sea state is saved. This remains fleet-level
autoresolution, not off-screen cannon-ball physics or an economy simulation.
Population runs while the world-map entity is active.

`tools/sync_metal_gameplay.py` composes the source-only `metal_fleet_sea` and
`metal_fleet_ui` adapters after existing exact-hash owners. They do not install
files independently. AIShip's existing fleet layer runs before the sea bridge.
The Enter menu's plain sea command uses message 31131; targeted approach keeps
31130. Plain entry clears selection but preserves genuine hostile commitments.
The encounter panel shows purpose, surviving composition and approximate
relative power, with truthful approach/pursuit/attack labels. Story restrictions
retain their existing callbacks; barrels and boats remain separate encounters.

## Acceptance

Primary falsifier: generated trade has a valid destination and real fleet;
nearby hostile patrol/pirate fleets acquire each other and form a paired battle,
while a distant player is ignored. Exit/re-entry preserves the pair and route;
sea import targets the opposing fleet, not the player by default. Nearest
negatives: friendly fleets and quest encounters are never retargeted, a removed
partner leaves no dangling pointer, and pause stops traffic timers.
Player-target falsifier: adding a healthy active warship prevents a weak pirate
from pursuing or forcing contact; removing/damaging it restores eligibility.
Comparable prey remains eligible, merchants compete with the player, and
quest encounters and voluntary player engagement remain available.

Compile and disposable script/native probes establish implementation properties.
The player owns the accepting scene replay: travel past a trade route, observe a
patrol intercept, enter that battle, and return to the map. No installed or
visible behavior is accepted until that replay is recorded in `docs/runtime.md`.

## Discovery

The prior layer raised pre-created battle frequency from 0.015 to 0.045 and
follow frequency from 0.025 to 0.035, with a shared 30-ship cap. Follow ships
steer only at the player; merchant ships have reusable destination/pathfinding
and saved coordinates. Existing sea import already includes both members of an
`attack` pair and assigns distinct `egroup__wdm_<id>` group identities.

## Candidate evidence

The canonical native build passes. Disposable probes using actual native
methods pass day-scale timing, pause, one-patrol rescue, saved-ID restoration,
missing partners, decisive superiority, guarded-trade avoidance and quest/friend
exclusion. Exact composed map/init/encgen/reload/sea scripts compile together in
the native VM. VM state probes pass rescue-range inclusion/cursor restoration,
distant/quest negatives and persistent wear with untagged/quest negatives.
The script lane separately checked real class tables/count generation and
maritime locators. Canonical staging and exact installed engine/content hashes
are verified. Renderer/world geometry and creation/scene replay are outside
those fixtures; player scene acceptance remains pending.

October 5 player-target correction: exact composed scripts compile in the native
VM; state probes cover full active fleet, parked-ship exclusion, hull/sail damage,
removed companion and no ship. Disposable probes execute actual native avoidance
and acquisition methods for ordinary/brave thresholds, escape, trade preference,
distant-player and quest/qID/ALONE/missing-state negatives. Read-only independent
review found the first-frame awareness gap; entry/creation refresh closes it.
The initial script compile failed because `makeref` cannot bind an attribute
branch; `makearef` corrected that owner and the final VM probe passes.
The VM additionally refreshes an old pirate Follow descriptor with real class
tables, verifies quest exclusion and checks that bravery is retained across ticks.
Fixture-only enum/nation initialization errors were corrected without product
changes. The broad repository hygiene check remains unresolved: its own
`git check-ignore --stdin` exceeded a bounded 20-second check and was stopped.

October 5 coherent candidate: canonical native compilation and disposable probes
pass committed versus uncommitted acquisition, same-side nearby reinforcement,
quest exclusion, direct-entry selection cleanup and off-screen survivor handling.
The exact final script composition compiles in the native VM. Seeded VM checks
pass old-save roster creation/no rank reroll, zero-survivor tombstones, quest
exclusion, pursuit import, active player fleet and sail readiness, committed entry,
loss-triggered retreat, cargo/damage save and repeat restoration without doubled
wear, proportional ammunition consumption, capture removal and semantic action
labels. Final composition is idempotent and rejects unknown installed revisions.
These are implementation checks, not proof of visual layout or battle enjoyment;
the player owns encounter-panel and gameplay acceptance after installation.

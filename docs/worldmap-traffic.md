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

## October 5 regional shipping routes

Route selection now uses the existing authored island centres as a coarse distance
estimate, while the native pathfinder still sails between verified offshore port
locators. Destination weight is `20 + 80 / (1 + distance² / 400²)`: short voyages
are more likely, but the positive floor preserves interregional trade. Existing
hostility, ownership and encounter eligibility filters still run before selection.
No player position enters departure or route selection.

New departures spread their home ports by `100 / (1 + resident²)`, counting only
living ordinary fleets of the same role whose saved `trafficOrigin` matches that
port. This reduces duplicate home patrols and repeated departure clusters without
increasing the population cap. Old fleets keep their routes and retire normally;
missing origin metadata is not guessed or used to reset their state.

Raiders add a smooth destination bonus from the actual positions of ordinary
merchant fleets: `40 / (1 + distance² / 400²)` per merchant group. Shipping can
attract pirate routes; quest ships, the player and removed encounters cannot.
This changes route preference only, not enemy omniscience or attack permission:
local sight, strength assessment, commitment and the one-patrol rescue cap still
own actual encounters. New origin/destination metadata is saved inside the existing
encounter descriptor, with no second registry or economy.

The design reference was TaleWorlds' documented caravan protection/patrol response
([War Sails release](https://www.taleworlds.com/en/News/587)) and purposeful sea
journeys ([developer update](https://www.taleworlds.com/en/News/598)). Those sources
explain their documented mechanics, not proof that these weights make Corsairs fun.
A larger spawn cap was rejected: it raises the number of actors without giving
them better reasons to cross paths.

Disposable checks compile the changed route helpers and complete creation function
in the native script VM. Seeded VM checks verify nearer-route preference, nonzero
long-distance weight, actual commerce attracting raiders, underrepresented home
selection, unchanged other-role counts, quest exclusion and missing-coordinate
fallback. Exact-hash composition admits the previously installed revision and
rejects unknown edits. Real-game density/enjoyment remains player acceptance:
travel through regional ports after new departures have replaced older traffic.

## Living Caribbean expansion — design, not an installed feature

Requested outcome: resident fleets, trade caravans, robbery, prizes, military
expeditions, fort sieges and port plunder should form one causal simulation with
observable opportunities for the player. The previously installed route weights
are a prerequisite, not completion of this expansion. Implementation and real-game
acceptance of this section are pending.

### Objects and authoritative state

- Fleet: retain the existing encounter ID, fixed survivor roster, cargo snapshots,
  nation, captain temperament, position, current mission and observed opponents.
  Add voyage/service/prize intent to that descriptor, not a second actor registry.
- Port: retain live Colony ownership, fort commander, garrison and Store goods.
  Supply/disruption/recovery state belongs with that colony and its existing
  stock updater; no independent decorative prosperity score.
- Cargo: use one manifest per surviving hull. Store departure, sea import, prizes,
  player looting and destination delivery consume the same quantities. A generated
  cargo reroll cannot stand in for a transported shipment.
- Simulation clock: the existing world-map clock and calendar advance own elapsed
  time. Map pause freezes time. Returning from land/sea advances only the missing
  interval once; saving or entering a scene is not a fresh random resolution.
- Information: visible nearby fleets expose observed purpose/composition; distant
  port reports require a visited port or an ordinary news source and carry an age.
  No exact cargo, hidden temperament, win percentage or omniscient enemy targets.

### Coupled mission loops

1. **Commerce and caravans.** A port selects an eligible allied/neutral buyer using
   live export/import demand, stock, travel cost and known route danger. Load from
   actual available stock within hull capacity, leave reserves for local/player
   supply, and debit once. A caravan is a real fleet with merchant hulls and
   optional escorts; valuable cargo and unsafe routes increase escort demand,
   with capacity/availability bounds. Arrival credits the surviving manifest,
   unloads, services the fleet and chooses the next job. Cheap local coastal
   traffic remains alongside rarer valuable long voyages.
2. **Piracy.** A captain searches locally, compares whole opposing fleets and
   escort/gun/readiness, estimates attainable loot versus losses and time, and
   chooses shadow/chase/attack/escape. Keep fixed bold/reckless temperaments;
   recklessness raises risk tolerance but does not grant hidden knowledge or
   free combat stats. An escort can be drawn away, a damaged straggler attacked,
   or an overmatched merchant coerced into a limited cargo surrender. After
   profitable plunder or major damage, return to a pirate refuge to sell prizes,
   repair and replenish instead of pursuing indefinitely. No reroll at sea entry.
3. **Patrols and relief.** Home patrols defend their territory, respond to observed
   attacks on their nation's commerce, and escort endangered survivors for a
   bounded leg. Preserve the one-patrol rescue cap for a local commerce fight.
   Repeated losses reduce supply and create bounded demand for replacement/relief
   departures from eligible allied ports, rather than instant reinforcements.
4. **Military expeditions.** Only hostile diplomacy permits an offensive port
   mission. Dispatch requires an available squadron, supplies and credible siege
   strength; it competes with home defence. The expedition physically travels,
   can be spotted or intercepted en route, and can abandon a newly untenable
   mission. A small pirate gang must not randomly bombard a powerful fort.
5. **Blockade, bombardment and plunder.** Distinguish interdicting shipping,
   suppressing fort guns, defeating a garrison and carrying away loot. A blockade
   can succeed without storming a fort. Derive siege power from working cannons,
   hulls, ammunition, crew and the live fort/garrison, with stationary-defender
   advantages; close battles resolve over days with accumulated costs. Relief
   ships join only if they actually arrive. Sack requires suppressed defence and
   surviving landing strength/cargo capacity; bounded loot comes from the port.
   Failure costs ships/time/supplies and sends survivors home. Ownership changes
   need a separately admitted full conquest/quest contract; ordinary successful
   raids damage supply and recovery, not authored story availability.
6. **Aftermath and recovery.** Destroyed/captured hulls stay gone. Surviving fleets
   need elapsed service time and available port supplies to regain readiness;
   entering a scene must not restore damage. Disrupted ports recover through the
   existing stock/garrison update and successful deliveries. Scarcity must remain
   recoverable: retain local reserve/recovery floors and cap simultaneous raids,
   so a few raids cannot permanently starve every shop.

### Player jobs and consistent scene transitions

- Observe a convoy, identify its escort, shadow it, intercept it or let it pass.
- Join an ongoing commerce fight on the same observed sides, rescue survivors,
  take a prize, or exploit damaged winners; no freshly regenerated opponents.
- Enter a port sea scene during a blockade/siege: import the same attacking fleet,
  damage/ammunition and mission, and bind its target to the fort/defenders.
  Friendly/neutral players remain optional participants, never universal targets.
- Relieve a port, defeat raiders, deliver scarce supplies, or wait until a shipping
  route becomes safer. Effects must be evident in actual goods/defence and the
  player's ordinary store interactions, not only in a log message.
- Hear a dated report about a disrupted port through ordinary port/news surfaces.
  Map icons summarize observed convoy/combat/port activity without flooding the
  screen. Use existing encounter review; no new omniscient dashboard.
- Preserve direct sea entry, deliberate pursuit, barrels/boats, story encounters
  and save compatibility. Quest fleets do not become economic actors or receive
  autonomous targets, cargo changes or service resets.

### Balance and causal model

Availability, distance, cargo value, observed force ratio and prior route losses
are continuous inputs. A riskier/high-value job should demand more support;
stronger effective defence should make raids less attractive and costlier.
Raider success should make a route less safe until patrol/escort/relief responds,
while merchants can switch to a viable alternative. This is a feedback loop with
bounded pressure and decay, not a player-rank-driven difficulty spawn.

Do not promise constant combat. A powerful player squadron can deter weak gangs
and choose profitable dangerous waters; reckless rivals, equivalent forces,
contested convoys and military operations provide meaningful optional danger.
Do not add global AI knowledge or ensure an enemy victory/loss for spectacle.
Maintain the finite actor budget and one authoritative battle resolution per
encounter. Reports/icon density and regional activity are separate from combat
frequency; increasing all spawn rates is not this design.

### Source discoveries and reuse verdict

- Native `WdmMerchantShip::KillTest` currently deletes commerce on arrival and
  reverses patrol/raider legs; `WdmEnemyShip::Update` ages fleets out. Rework these
  into service/new-voyage transitions for ordinary resident fleets only. Preserve
  explicit sunk/captured retirement and legacy quest lifetime semantics.
- Existing `WdmMerchantShip::StartTrafficBattle/ResolveTrafficBattle` already owns
  local paired battles, one rescue patrol, wear and hull tombstones. Adapt it;
  do not build a second battle scheduler.
- `tools/gameplay/fleet-sea.c` already preserves exact Ship/RealShip cargo/damage
  and imports persistent IDs. Extend its same manifest/task bridge for commerce
  and fort targets, rather than parallel sea-only generators.
- `PROGRAM/store/storeutilite.c` owns Set/Add/RemoveStoreGoods, current quantities,
  prices and daily stock updates. `Colonies[].StoreNum` supplies the actual store.
  Supply consequences must integrate there and reconcile daily regeneration.
- `PROGRAM/sea_ai/AIFort.c` uses Fort_FindCharacter, live colony nation, actual
  cannon quantity, Fort.HP/Ship.HP and fort resurrection. Integrate map siege wear
  after initialization and before fighting; initialization currently resets HP,
  so an attribute-only map patch would be visually false.
- The existing city siege branch in `PROGRAM/scripts/colony.c` creates six hunter
  captains, sets AlwaysEnemy/player hostility, locks a scripted timer and targets
  PLAYER_GROUP. Reuse underlying group/address/fort facilities, not this authored
  player-attack wrapper. It is not an NPC-versus-port simulation.

### Ordered delivery and falsifiers

The implementation uses the existing exact-hash composition and ordered native
patch stack, then canonical staging. The unresolved native fort-target binding
must be proven before accepting the siege bridge; the authored player-attack
wrapper is not evidence of that contract.

1. **Persistent voyages and service:** new/old ordinary descriptors complete a
   route, spend game time servicing, depart again and retain identity. Seed a
   damaged survivor and a dead hull; only the survivor can recover. Quest timers
   and pause remain unchanged. Port capture invalidates incompatible destinations.
2. **Cargo-backed commerce/piracy:** loaded stock equals transported manifest;
   loss/capture/sea exit/delivery conserve goods, with no repeated debit, duplicate
   credit, cargo reroll or loot creation. Saved-state reload is idempotent. Existing
   player store actions and quest cargo are unaffected.
3. **NPC expedition and fort handoff:** a strong hostile supplied fleet can reach
   a defended port, blockade/fight and suffer losses; a weak raider avoids it.
   Import the same damaged actors and fort state to sea; a neutral player is not
   attacked simply for loading. Nearby arriving relief changes the actual battle;
   distant fleets and invalid/quest ports do not.
4. **Consequences and player-readable activity:** a disrupted port loses a bounded
   amount of real supply, successful shipping/recovery restores availability, and
   the normal store reflects it. Far rumours remain dated/imprecise. Native probes
   and script VM establish implementation properties; canonical stage-only proves
   delivery; the player voyage accepts pacing, visibility and enjoyment.

No later milestone may substitute decorative events, instant arbitrary stock
changes, invisible escorts or forced player enemies for these prerequisites.

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
The Enter menu retains contextual approach/attack/pursuit as its default. Both
quick-entry paths use 31130 to review a nearby encounter before entering; only
the explicit sea button in that panel commits through 31131. Plain entry clears
selection but preserves genuine hostile commitments. The encounter panel keeps
a square 160-unit illustration beside the top-aligned summary, a bounded body
scroller and one action row; its initial focus is the contextual action.
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

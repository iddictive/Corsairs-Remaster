# World-map traffic

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
the player is running the game; source/build integration belongs to the parent.

## Candidate model

Traffic exists across the whole map. Native population scheduling creates
departures at real ports and keeps a bounded resident population; it does not
spawn traffic in a ring around the hero. Scripts remain the fleet-content and
diplomacy bridge, including existing story/quest encounters. Navigation,
target acquisition, escape, patrol limits and battle timing belong to the engine.

- Most traffic is commerce. A caravan represents an actual generated fleet,
  not several decorative copies of one ship. Routes use live port ownership
  and relations; patrols operate around their home port.
- Raiders prefer trade and reject fleets stronger than 1.05 times their own
  estimated strength; 10% tolerate 1.25. They flee stronger nearby opponents.
  Trading ships flee; distant enemies do not know the player's position.
- Player strength uses every active companion-slot ship, class and hull role,
  weighted by current hull/sail condition (75%/25%). Parked ships and personal
  stats do not count. Ordinary raiders compare the player and NPC prey in the
  same scoring pass; superior player fleets cause escape and cannot force a sea
  encounter through the native contact flag. The player can still enter combat.
  Map entry rebuilds the estimate before creating the entity, including old saves;
  creation assigns awareness immediately and legacy creation refreshes before
  the next native update. No random risk reroll occurs during target evaluation.
- Old nonquest pirate `Follow` encounters also apply strength-based retreat and
  contact suppression. They remain dedicated pursuits, not commerce-seeking
  traffic. Naval and quest/qID/ALONE encounters retain their existing behavior.
- Opposing fleets become a battle only after approaching each other. Both
  original descriptions and identities enter the sea scene together. Battles
  last at least 24 game hours using the native map clock (up to 48 for close
  forces), with pause freezing combat progress. One same-nation home patrol can
  rescue commerce already fighting pirates; another patrol cannot join that fight.
  All three identities import together even when the sea-entry radius cuts
  through the battle. Twice the enemy's combined strength wins deterministically;
  closer forces use squared-strength odds. Losers expire; winners retain wear,
  resume their route and receive a 90-second cooldown.
- Population caps distinguish trade, patrols, pirates and legacy pursuers.
  Increasing all random rates and the global cap is explicitly rejected.
- Saved route and battle state must survive normal map exit/re-entry; old saves
  without traffic fields continue using the old encounter behavior.

Strength combines actual descriptor fleet counts and the generator's rank/class
ranges: count times squared average class size, with warships weighted twice.
It is an estimate; exact hull types, guns and captains are still instantiated
by the existing sea generator. Condition weakens future engagements and is
carried into generated hull/sails/crew; overwhelming victories do not reduce
crew. This is fleet-level autoresolution, not off-screen cannon-ball physics or
an economy simulation. Population runs while the world-map entity is active.

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

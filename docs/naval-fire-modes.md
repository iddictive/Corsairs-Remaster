# Naval firing modes and battery cadence

## Status

The count-aware quadratic cadence and per-shot moving-target prediction passed
canonical native build/staging as signed engine4a632fcd. Aligned single/salvo
art and ammunition precision multipliers are delivered. A later manual-aim
repair applies mechanical reach checks after longitudinal station assignment;
native actual-function checks and canonical stage-only pass as signed
enginebb71d94c, retained in current engine70d78026; installed firing replay
remains required. Earlier source
`e2ecbbf` was built and staged with fixed 3.5–10-second windows; that historical
delivery does not accept this correction. Preserve real player saves and use a
fresh current-save clone for the pending real-game replay.

## State and consumers

- `pchar.Ship.Cannons.FireMode`: saved script attribute; 1 means single-cannon,
  zero or absent means salvo. It applies to the main ship, not NPC orders.
- `PROGRAM/battle_interface/BattleInterface.c`: main-ship quick-menu command
  `BI_FireMode`, labelled `Огонь: по одной` or `Огонь: залпом`. Switching changes
  only that attribute and the existing command projection; loaded cannons,
  scheduled shots, ammo, powder and reload progress remain intact.
- `src/assets/ui/manifest.json`: separate `fire-mode.tga.tx` with one-gun and
  three-gun art, each in normal/selected states. BattleInterface texture6 selects
  rows0/1 for single fire and2/3 for salvo; refresh uses the saved mode owner.
- Native `AIShip::Fire` and `AIShipCannonController`: every supported manual and
  third-person fire consumer must honor one successful cannon per trigger in
  single mode, including the third-person target/bort iteration loops.
- `AICannon::Fire/Execute/RealFire/Recharge`: existing per-gun scheduled action,
  live muzzle/solver, loaded shot and individual reload/debit lifecycle. No new
  timer queue or cannon/projectile save format is introduced.
- `PROGRAM/sea_ai/AICannon.c`: one captain/crew/morale quality window for ship
  salvos. This returns the full-window quality `Q`: 3.5 seconds at best quality,
  10 seconds at poorest quality. Fort firing retains its existing 20-second
  individual-delay contract. Native salvo strata must retain nonzero spacing
  and jitter at maximum quality. Every gun, including both endpoints of a
  two-gun battery, receives independent positive jitter bounded by the smaller
  of 0.15 seconds and 20% of its stratum. Adjacent delays remain at least 80% of
  a stratum apart; the total window varies by at most 0.15 seconds.
- Native `SalvoWindowForCount` owns count scaling for the actual firing set:
  `W = 0.5 + (Q - 0.5) * ((min(N, 108) - 2) / 106)^2` for `N >= 2`.
  Two guns have a half-second nominal span and finish within one second; 108
  guns span approximately 3.5–10 seconds. Damaged, unavailable and unreachable
  guns do not contribute to `N`; whole-ship inventory/HUD counts are not battery
  counts. Each moving-target gun uses its own queued delay for interception,
  preserving the existing rake offset and ballistic flight estimate. Guns that
  cannot traverse to that intercept are removed before rescaling the survivors.
  A sole ship gun uses one 0.05–0.20-second event sample for both its target lead
  and discharge. Single mode retains the same short-delay range. No timing or
  target fields are added to the cannon save codec.
- Manual aim: reuse the full existing rake distribution, then select the same
  one ready/reachable gun and its mapped target for actual single fire and its
  dispersion/trajectory preview. Ball flight and per-shot random scatter retain
  their existing owner.
  A selected ship first receives longitudinal targets for intact guns; each
  mapped target then passes the existing per-gun traverse, elevation and range
  checks. Rejecting the common depth point before assigning those stations can
  incorrectly discard a whole battery. Terrain and unreachable high mast shots
  retain their mechanical limits; no elevation clamping or vertical snapping is
  introduced. The common far-range fallback remains limited to genuinely
  out-of-range receivers.

Input is unchanged: `Ship_Fire` maps left mouse in first and third person.
`Core::ControlProcess` emits `Control Activation` on `CST_ACTIVATED`, while held
`CST_ACTIVE` does not emit another press. `ProcessControls` calls `Ship_DoFire`,
then `AI_MESSAGE_CANNON_FIRE` reaches the native owner. Repeated presses may fire
other ready cannons while earlier shots reload; no-ready attempts must have no
discharge, hostility event, ammo debit or reload side effect.

## Compatibility and acceptance

Old saves default to salvo without resetting any prior cannon state. The mode
attribute survives the normal script save/load path. Existing native scheduled
gun state remains in its unchanged codec: mode changes cannot cancel or duplicate
queued shots, and loading a mid-salvo save must resume each existing shot once.
At normal speed the window equals wall seconds; normal game pause/time scaling
must affect firing and reload together.

Disposable actual native-VM checks accept quick-menu default/toggle/persistence,
main/companion/gunless eligibility and unchanged ammo/powder/partial charge state.
The actual quality helper compiles and returns 3.5 seconds at full skill/crew
quality, 10 seconds at zero skill/experience or zero optimal crew, and 6.75
seconds at half crew or zero morale; saturated inputs clamp to 3.5 seconds.
The earlier VM event-dispatch evidence belongs to the prior fixed-window
implementation. The current direct ship-delay event returns 0.05–0.20 seconds
for player and NPC ships; the unchanged fort branch returns 0–20 seconds.
The quality is normalized cannon skill times crew experience/optimal staffing
times the bounded morale factor. Existing per-trigger `SHIP_BORT_FIRE` experience
rewards remain unchanged, including when the trigger fires one cannon.

The ordered patch follows `airburst-shell.patch` and precedes
`adaptive-fencing.patch`. Dry application to all five native files and syntax
checks of all three full translation units pass. A disposable actual-function
native fixture passes 64 assertion sites: repeated single fire, individual
reload/resource debit, loaded-gun/no-stock and insufficient-powder cases,
ready/traverse/damage guards, full-battery rake mapping, third-person target and
side loops, 2/4/32-gun 3.5/10-second spacing, mode changes during queued salvos,
pending shots through save/load, pause/delta advancement, teardown and live
muzzle recomputation. The existing cannon/controller Save/Load functions remain
byte-identical. Collision and event dependencies in that fixture are stubs.

Integration rejected fixed first/last delays because two-gun batteries had no
temporal scatter. A later fixed-quality window also stretched two guns across
the full 3.5–10 seconds. The quadratic correction reuses the retained disposable
actual-function fixture and passes 64 seeds for 2/4/32/49/108 firing guns at both
quality endpoints. Two-gun last shots occur at 0.552–0.649 seconds; 108-gun last
shots at 3.550–3.556 or 10.050–10.068 seconds. Both endpoints vary independently,
and adjacent spacing remains at least 80% of each count-scaled stratum.
The same fixture accepts each moving-target gun's delay-based intercept,
108 model guns with only two eligible guns, late traverse rejection followed by
singleton rescaling, single-mode isolation, per-gun reload/resource debit, NPC
mode handling, direct/fort delay fallback, and unchanged pending-shot save/load.
Native syntax checks pass for the three touched full translation units.

The earlier retained isolated script-VM fixture failed compilation with
`Invalid Expression` at its delay-event scenario, including after restoration
of its historical assertion-stripped form. This does not prove a gameplay-source
defect or a passing VM check. Full PROGRAM compilation and installed-app replay
resolve the script compilation gap; visible firing remains a separate gate.

Full startup plus catalogue/initStore/controls/store interface compilation passes
with installed shared headers and exact pending script inputs. Real-game
acceptance still requires a partial-battery single-shot sequence, exhausted ammo,
mode switch, delayed full salvo, selected-gun preview and an ordinary NPC salvo.
Source/mock/VM evidence does not accept those visible actions.

The manual-aim actual-function falsifier seeds the same four guns and a selected
close hull point at y0,z20 with a muzzle at y5 and a depression limit of -0.13
radians. The previous common-point gate returns zero guns although three
longitudinal stations on the 100-metre hull are mechanically reachable. The
candidate returns those three legal guns and queues their shots. An impossible
high mast, steep nearby terrain, ordinary far intent and single-mode mapped
station pass the nearest negative checks. The existing cadence/state fixture
and all three native translation-unit syntax checks pass. This proves the
ordering defect; the player's broader report that aiming at any ship breaks
fire remains unresolved until its actual symptom is reproduced.

Runtime attempt chronology and installed hashes belong to `docs/runtime.md`.

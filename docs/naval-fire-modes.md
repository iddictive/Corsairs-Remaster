# Naval firing modes and battery cadence

## Status

The integrated source passes canonical native build and full PROGRAM compilation
with zero script errors. The player requested a quick-menu single-cannon mode
and a broadside spread over approximately3.5–10 simulation seconds. Independent
endpoint jitter also passes the two-gun case. Installed delivery and actual
firing/preview replay remain pending with the integration owner; the installed
app remains on the preceding batch.

## State and consumers

- `pchar.Ship.Cannons.FireMode`: saved script attribute; 1 means single-cannon,
  zero or absent means salvo. It applies to the main ship, not NPC orders.
- `PROGRAM/battle_interface/BattleInterface.c`: main-ship quick-menu command
  `BI_FireMode`, labelled `Огонь: по одной` or `Огонь: залпом`. Switching changes
  only that attribute and the existing command projection; loaded cannons,
  scheduled shots, ammo, powder and reload progress remain intact.
- Native `AIShip::Fire` and `AIShipCannonController`: every supported manual and
  third-person fire consumer must honor one successful cannon per trigger in
  single mode, including the third-person target/bort iteration loops.
- `AICannon::Fire/Execute/RealFire/Recharge`: existing per-gun scheduled action,
  live muzzle/solver, loaded shot and individual reload/debit lifecycle. No new
  timer queue or cannon/projectile save format is introduced.
- `PROGRAM/sea_ai/AICannon.c`: one captain/crew/morale quality window for ship
  salvos. The best quality retains an approximately 3.5-second window; poorest
  quality approaches 10 seconds. Fort firing retains its existing 20-second
  individual-delay contract. Native salvo strata must retain nonzero spacing
  and jitter at maximum quality. Every gun, including both endpoints of a
  two-gun battery, receives independent positive jitter bounded by the smaller
  of 0.15 seconds and 20% of its stratum. Adjacent delays remain at least 80% of
  a stratum apart; the total window varies by at most 0.15 seconds.
- Manual aim: reuse the full existing rake distribution, then select the same
  one ready/reachable gun and its mapped target for actual single fire and its
  dispersion/trajectory preview. Ball flight and per-shot random scatter retain
  their existing owner.

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
The actual VM event dispatch also accepts the new window handler, the
0.05–0.20-second single delay, 40–60% ship-window lead estimate and unchanged
0–20-second fort delay with zero script errors.
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

Integration rejected the first cadence candidate: its fixed first/last delays
left two-gun batteries with no temporal scatter. The bounded per-gun correction
passes 64 seeds for each of 2/4/32 guns at both 3.5/10-second windows, including
independent endpoint and total-span variation. Two-gun spans observed
3.3580–3.5131 and 9.8580–10.0131 seconds; dense 32-gun gaps remain at least
0.09032/0.25806 seconds. Existing pending-shot, mode-change, save/load and reload
cases still pass. No cannon script or save-codec change was required.

Full startup plus catalogue/initStore/controls/store interface compilation passes
with installed shared headers and exact pending script inputs. Real-game
acceptance still requires a partial-battery single-shot sequence, exhausted ammo,
mode switch, delayed full salvo, selected-gun preview and an ordinary NPC salvo.
Source/mock/VM evidence does not accept those visible actions.

Runtime attempt chronology and installed hashes belong to `docs/runtime.md`.

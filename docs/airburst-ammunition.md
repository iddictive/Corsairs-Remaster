# Airburst naval ammunition

## Status

Native build, full startup PROGRAM compilation and canonical stage-only pass.
The installed signed engine is40c8c300 with all13 content inputs delivered;
signature and exact compiler-input/app/cache checks pass. The player's replay
rejects the current fuze: shells hit ships without an overhead burst, and the
shop lists the new goods outside the bomb group. Native fuze and shop display
order are reopened under the airburst source owner, keeping saved goods IDs.
The distinct drawn icon was never installed; it joins the corrected next batch.
Its earlier delivery-fixture pass was withdrawn after the fixture rejected a
temporary symlink path before testing. No game process is interrupted.

## Contract

`GOOD_AIRBURST` is appended at index 51; existing cargo identifiers stay fixed.
The fifth charge command and key `5` select «Шрапнельные бомбы». It is ship cargo,
uses powder and the normal cannon-loading rules, and reuses the bomb icon with a
distinct selector note and selected-ammunition label.

A descending shell arms after 35 metres and bursts inside a ship's oriented
overhead volume. The volume follows the current native ship transform and hull
bounds; segment/slab intersection prevents a long frame from skipping it. The
shooter and dead/unmounted targets do not trigger it. Islands and forts block the
flight and fragment damage.

A burst makes one hull/crew hit per live nearby ship through `SHIP_HULL_HIT`.
Power falls quadratically to zero at 32 metres from the nearest deck point.
Eight downward fragment sweeps use the existing sail-cloth damage owner. Nearby
friendly ships receive the same damage; the firing ship is excluded. Crew damage
retains captain/doctor defences but bypasses hull protection for the overhead
burst. Direct hits, including forts and rigging, keep ordinary-bomb damage.

The new catalogue values are hull 90, crew 14 and rigging 8. Cost is 480 per goods
unit (eight times ordinary bombs), reload takes 1.65 times as long, and speed 0.8
keeps range below round shot. Shop stock is bounded to 40–120, derived from one
quarter of the existing bomb norm. A failed overhead approach does not grant
radial damage.

Visuals reuse the authored `bomb_smoke`, `blast`, `ShipExplode` and `CreateBlast`
owners. `ShipExplode` supplies fire, smoke trails and gravity-driven fire traces.
Every shell has a blast; heavy fire/smoke and explosion audio are capped to one
start per 120 ms per live balls environment. No renderer/shader replacement or
new texture is required. Actual appearance and frame cost require a sea replay.

## Owners and compatibility

- `experiments/native-metal/airburst-shell.patch`: native fuze, bounded fragment
  sweep, radial dispatch, and selector-note consumption.
- `src/gameplay/PROGRAM/sea_ai/AIBalls.c`: flight marker, arc and burst VFX.
- `src/gameplay/PROGRAM/sea_ai/AIShip.c`: radial damage through existing crime,
  kill-credit, hull, crew and gun-damage consumers.
- `src/gameplay/PROGRAM/sea_ai/AICannon.c`: reload multiplier and the ordinary
  bomb target-height alias. Manual aim already queries the actual scripted
  height multiplier, so the higher arc has one owner.
- `src/gameplay/PROGRAM/store/`: catalogue and missing-only shop initialization.
- `src/gameplay/PROGRAM/seadogs.c`: normal `OnLoad` array/catalogue migration.
- `src/gameplay/PROGRAM/battle_interface/BattleInterface.c` and controls/text
  sources: fifth charge, saved selector-array expansion and Russian labels.
- `src/gameplay/manifest.json`: complete-source delivery and original-byte
  admission; the ordered native patch is declared by `build.sh`.

The native projectile save format iterates four unnamed type lanes. Adding a
fifth lane would consume unrelated saved bytes. New shells therefore use the
existing Bombs lane and preserve identity in the already serialized event string
`@airburst:51`; all native projectile save/load functions remain unchanged.

Old VM saves restore their recorded array sizes. `Airburst_InitGoods` expands a
51-row catalogue to 52 without resetting old goods; the selector expands from
five slots to six at its existing read boundary. `OnLoad` restores the new
catalogue before the regular sea/location consumers. Only absent shop rows are
created; depleted or disabled existing stock is preserved. Fresh stores use the
same initializer. A sea/non-colony store is excluded from the old-save migration.

## Evidence and remaining replay

Disposable native-VM checks pass catalogue/selector expansion, retained ordinary
goods and cargo, missing/depleted/disabled shop rows, fresh initialization and a
save/load round. The initial fixture lacked the engine's required `OnLoad`
callback; adding that callback in the fixture resolved its invalid-function
failure without an engine/harness edit.

Extracted native fuze/marker checks pass swept and already-inside descending
segments; ascending, below-deck, outside-footprint and stationary negatives
reject. Ordinary event strings remain ordinary shots. Exact patch admission
passes; native projectile save/load source is unchanged.

Full PROGRAM compilation caught the unavailable script API `GetTickCount` in
the initial VFX limiter. Its canonical correction uses `PostEvent` and a ready
handler, registered/removed with the balls environment and reset on creation.
The actual native event loop keeps two rapid blasts but only one heavy effect
through119ms, then permits the next heavy effect at120ms, with zero script
errors. The same native compiler accepts the full startup program plus changed
catalogue/store/control segments using installed executable-relative headers;
an initial disposable missing-header setup error is not a product failure.

Canonical build/staging and installed whole-PROGRAM compilation remain with the
integration owner. Acceptance requires an existing-save shop purchase/selection,
an above-target burst with resulting hull/crew/sail state, nearby-friendly risk,
an unarmed ordinary-bomb impact, and a neighboring ordinary-bomb shot. Stock
effects alone do not prove the requested spectacle or game FPS. Runtime attempts
and the consuming engine/content hashes belong to `docs/runtime.md`.

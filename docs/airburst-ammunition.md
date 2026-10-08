# Airburst naval ammunition

## Status

The first installed version40c8c300 was rejected by player replay: no overhead
burst and misplaced shop rows. Corrected source4d42729 is now installed through
canonical stage-only as signed enginefcce6e7b, together with adaptive fencing.
Raised shared aim, both-direction overhead fuze, display-only shop ordering and
the drawn icon are delivered. Full startup PROGRAM and catalogue/store/control
segments, including the store interface, compile with zero script errors.
Deep/strict signature, all five changed cache/app inputs and both changed PROGRAM
compiler inputs match; delivery plans are empty. SAVE/config hashes are preserved.
Root launches no game; a new unknown/player process appears after staging and
changes five startup logs. Corrected salvo/shop replay remains unresolved.

## Contract

`GOOD_AIRBURST` is appended at index 51; existing cargo identifiers stay fixed.
The fifth charge command and key `5` select «Шрапнельные бомбы». It is ship cargo,
uses powder and the normal cannon-loading rules. Store rows display it directly
after bombs without changing saved cargo identifiers. The inventory icon uses
the unused128px goods cell at384,512; normal/selected command icons use cells72/73
of the existing64px atlas. Drawn sources and exact prepared atlas bytes belong to
`src/assets/ui/manifest.json`; all original pixels outside those cells stay intact.

A shell arms after 35 metres and bursts inside a ship's oriented
overhead volume. The volume follows the current native ship transform and hull
bounds; segment/slab intersection prevents a long frame from skipping it. The
shooter and dead/unmounted targets do not trigger it. Rising and falling shells
can enter the overhead volume; a stationary segment cannot. The shared native
`AICannon::CalcHeightFireAngle` adds12m of aim height for cargo51 at ranges of at
least35m. Both real firing and manual trajectory preview use this solver. Islands
and forts block the flight and fragment damage.

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
start per 120 ms per live balls environment. The effects need no renderer/shader
replacement or new effect texture. Actual appearance and frame cost require a
sea replay.

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
- `src/gameplay/PROGRAM/interface/store.c`: display-only goods ordering; row
  transaction indices still refer to the original cargo catalogue.
- `src/gameplay/PROGRAM/seadogs.c`: normal `OnLoad` array/catalogue migration.
- `src/gameplay/PROGRAM/battle_interface/BattleInterface.c` and controls/text
  sources: fifth charge, saved selector-array expansion and Russian labels.
- `src/gameplay/manifest.json`: complete-source delivery and original-byte
  admission; the ordered native patch is declared by `build.sh`.
- `src/assets/ui/manifest.json`: drawn art, original-atlas admission and prepared
  textures; `tools/delivery_state.py` admits only the two exact added atlas paths.

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

The first-version extracted fuze/marker checks passed swept/inside descending
segments; ascending, below-deck, outside-footprint and stationary negatives
rejected. Ordinary event strings remained ordinary shots. Those checks proved
only the geometry predicate; native projectile save/load source stayed unchanged.

The installed first version was rejected by player replay: shells hit the hull
without an overhead burst. Its Bombs target-height alias aimed around2.2m, below
the overhead slab's7m minimum; a height multiplier preserves the aim chord and
cannot raise the destination. Requiring descent also rejected short-range rising
arcs. This earlier geometric-only check did not exercise the firing trajectory.
The correction retains the slab/arming/blockers and moves aim height in the
shared cannon solver. Extracted actual solver and flight equations now enter the
overhead volume at50/150/300m for small and tall hull bounds; the previous solver
misses it in the same cases. Under35m/direct and ordinary-bomb angles stay
unchanged; below/outside/stationary cases reject. Projectile Save/Load remains
byte-identical. Real corrected salvo replay remains required.

The icon producer pixels, decoded TX cells and64/128px comparisons pass. A
disposable check of the real delivery owner admits both exact original atlases,
preserves existing mast/perk art and rejects unknown atlas edits and unrelated
texture paths. An initial fixture used macOS's `/var` symlink and was rejected
before testing; resolving its path fixed the fixture. Both exact prepared atlases
are installed with the corrected fuze; in-game icon readability remains unverified.

Full PROGRAM compilation caught the unavailable script API `GetTickCount` in
the initial VFX limiter. Its canonical correction uses `PostEvent` and a ready
handler, registered/removed with the balls environment and reset on creation.
The actual native event loop keeps two rapid blasts but only one heavy effect
through119ms, then permits the next heavy effect at120ms, with zero script
errors. The same native compiler accepts the full startup program plus changed
catalogue/store/control segments using installed executable-relative headers;
an initial disposable missing-header setup error is not a product failure.

Canonical build/staging and full PROGRAM compilation pass under the integration
owner. Gameplay acceptance requires an existing-save shop purchase/selection,
an above-target burst with resulting hull/crew/sail state, nearby-friendly risk,
an unarmed ordinary-bomb impact, and a neighboring ordinary-bomb shot. Stock
effects alone do not prove the requested spectacle or game FPS. Runtime attempts
and the consuming engine/content hashes belong to `docs/runtime.md`.

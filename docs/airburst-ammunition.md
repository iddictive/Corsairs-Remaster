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
A later player balance request reduces overhead hull damage by25%, from90 to67.5.
The actual scene/source review also found a third icon consumer near the compass;
its source now selects navigator cell40 with the same drawn normal-state art.
Further player requests add a fort airburst before the battery and restrict
ordinary purchases to Bermuda. The complete firing-mode/fort/trade/HUD source
batche2ecbbf passes canonical native build, zero-error full PROGRAM compilation
and canonical stage-only. Signed engineb8d033b7, the fort fuze, lower ship hull
damage, Bermuda trade restriction and navigator icon are installed. Signature,
receipt and seven content inputs match; all252 player files retain hashes.
Real fort/store/HUD/firing replay remains open under a serialized fresh-save
clone actor. The olderfcce6e7b inventory is historical.

The October 8 native correction is staged as signed engine4a632fcd: lower ship
fuze/aim and cosmetic ballistic fragments with real water collisions. Crew
percentages, live damage descriptions, gunner precision multipliers and aligned
single/salvo art are also delivered. Actual salvo/burst/casualty replay remains
unresolved; the later manual-target reach-order repair still awaits native
delivery.

## Contract

`GOOD_AIRBURST` is appended at index 51; existing cargo identifiers stay fixed.
The fifth charge command and key `5` select «Шрапнельные бомбы». It is ship cargo,
uses powder and the normal cannon-loading rules. Store rows display it directly
after bombs without changing saved cargo identifiers. The inventory icon uses
the unused128px goods cell at384,512; normal/selected command icons use cells72/73
of the existing64px atlas. The compass charge indicator uses cell40 at0,320 in
the separate512px `LIST_ICON2` atlas. It reuses command cell72 byte-for-byte;
all earlier navigator, named nation, spyglass and sailing cells stay unchanged.
Drawn sources and exact prepared atlas bytes belong to
`src/assets/ui/manifest.json`; all original pixels outside those cells stay intact.

A shell arms after 35 metres and bursts inside a ship's oriented
overhead volume. The volume follows the current native ship transform and hull
bounds; segment/slab intersection prevents a long frame from skipping it. The
shooter and dead/unmounted targets do not trigger it. Rising and falling shells
can enter the overhead volume; a stationary segment cannot. The shared native
`AICannon::CalcHeightFireAngle` adds8m of aim height for cargo51 at ranges of at
least35m. Both real firing and manual trajectory preview use this solver. Islands
and masonry block the flight and fragment damage. The ship volume now spans
1–7m above its existing deck estimate, instead of4–14m; its footprint, oriented
transform, arming and swept intersection remain unchanged. Fort sphere geometry
and its gun-relative4m minimum remain unchanged.

Fort proximity uses world cannon locators: a swept22m sphere around an undamaged
gun in a normal fort, restricted to at least4m above that gun. The nearest ship
or fort trigger wins. Both queries share35m arming-segment clipping, including
a long frame crossing the arming boundary inside the target volume. Own,
missing-model, already destroyed and non-normal forts do not trigger it.
The32m fort blast reuses `AIFort::AddFortHit`, its gun-damage callback and
`PersistFortCannons`. Visibility to the exposed muzzle excludes masonry/terrain
shielded guns; ordinary contact callers retain their original radius and path.

A burst makes one hull/crew hit per live nearby ship through `SHIP_HULL_HIT`.
Power falls quadratically to zero at 32 metres from the nearest deck point.
Eight fragment sweeps use the existing sail-cloth damage owner: four upwards
and four downwards, so a low deck burst can still hit cloth above it. Nearby
friendly ships receive the same damage; the firing ship is excluded. Crew damage
retains captain/doctor defences but bypasses hull protection for the overhead
burst. Direct hits, including forts and rigging, keep ordinary-bomb damage.

The current catalogue values are hull39, crew1.2% and rigging25.2: twice ordinary
bombs' hull19.5, grapes' crew0.6% and knippels' rigging12.6 respectively. Crew
percentages use the target's current crew before the retained modifiers; the
shared conversion and old-save reconciliation belong to `naval-crew-damage.md`.
These values supersede the earlier67.5/14/8 balance. Direct contacts retain ordinary-bomb
fallback. Fort airbursts use a separate `DamageFort`
coefficient180 with quadratic32m falloff, independent of ship hull damage.
Cost is480 per goods
unit (eight times ordinary bombs), reload takes 1.65 times as long, and speed 0.8
keeps range below round shot. Shop stock is bounded to 40–120, derived from one
quarter of the existing bomb norm. All store rows outside the canonical Bermuda
colony `Pirates` are contraband; only that colony permits buying these shells.
`Store_EnsureAirburst` reconciles saved trade types without resetting quantities,
norms, disabled/depleted stock or player cargo. `Store_CanBuyGoods` owns the buy
gate for table prices, quantity editing and the final transaction, including
non-colony/generated shops. The later unlock quest is explicitly deferred.
A failed overhead approach does not grant radial damage.

Visuals reuse the authored `bomb_smoke`, `blast`, `ShipExplode` and `CreateBlast`
owners. `ShipExplode` supplies fire, smoke trails and gravity-driven fire traces.
Every shell has a blast; heavy fire/smoke and explosion audio are capped to one
start per 120 ms per live balls environment. Each shell also emits16 ballistic
cosmetic fragments using the existing grapeshot atlas cell and `grapes_tracer`.
They spread downward at24m/s under the ordinary native ball gravity. The enlarged
0.30m sprites and authored tracers are visual feedback; they cause no additional
hull, crew, fort or cloth damage. The eight original cloth sweeps remain the
damage owner. Ship, island and fort geometry absorb the cosmetics through pure
`Trace`; the nearest unoccluded water collision calls `SEA::Cannon_Trace`, which
owns the actual wave height and `BALL_WATER_HIT` position. Script selects the
ordinary `splash` effect for fragments rather than the cannon's large-ball splash.
Normal cannon impacts keep their existing effect selection.

Fragments are queued until the active ball iteration finishes, then appended to
the existing Bombs lane. This prevents a broadside from invalidating the bursting
ball's vector reference. They stop and release their tracer at impact or after
3s of projectile time; environment destruction keeps the existing ball cleanup.
There is no renderer/shader replacement or new texture. Actual appearance and
frame cost require a sea replay.

## Owners and compatibility

- `experiments/native-metal/airburst-shell.patch`: native fuze, bounded fragment
  sweep, radial dispatch, and selector-note consumption.
- `src/gameplay/PROGRAM/sea_ai/AIBalls.c`: flight marker, arc and burst VFX.
- `src/gameplay/PROGRAM/sea_ai/AIShip.c`: radial damage through existing crime,
  kill-credit, hull, crew and gun-damage consumers.
- `src/gameplay/PROGRAM/sea_ai/AICannon.c`: reload multiplier and the ordinary
  bomb target-height alias. Manual aim already queries the actual scripted
  height multiplier, so the higher arc has one owner.
- `src/gameplay/PROGRAM/store/`: catalogue, missing-row initialization and saved
  Bermuda-only trade policy; existing empty/disabled stock is preserved.
- `src/gameplay/PROGRAM/interface/store.c`: display-only goods ordering; row
  transaction indices still refer to the original cargo catalogue. Purchase
  projections and the transaction share `Store_CanBuyGoods`.
- `src/gameplay/PROGRAM/seadogs.c`: normal `OnLoad` array/catalogue migration.
- `src/gameplay/PROGRAM/battle_interface/BattleInterface.c` and controls/text
  sources: fifth charge, saved selector-array expansion and Russian labels.
- `src/gameplay/manifest.json`: complete-source delivery and original-byte
  admission; the ordered native patch is declared by `build.sh`.
- `src/assets/ui/manifest.json`: drawn art, original-atlas admission and prepared
  textures; `tools/delivery_state.py` admits only the three exact added atlas paths.

The native projectile save format iterates four unnamed type lanes. Adding a
fifth lane would consume unrelated saved bytes. New shells therefore use the
existing Bombs lane and preserve identity in the already serialized event string
`@airburst:51`; the native projectile save/load binary contract stays unchanged.
Cosmetic fragments use the distinct existing event-string marker
`@airburst-fragment` in that same lane. They suppress custom flight callbacks and
all damage consumers, and restore `grapes_tracer` after loading. The binary
`BALL_PARAMS` codec and four lane layout remain unchanged; only the existing
particle restoration selects a fragment-specific authored effect.

Old VM saves restore their recorded array sizes. `Airburst_InitGoods` expands a
51-row catalogue to 52 without resetting old goods; the selector expands from
five slots to six at its existing read boundary. `OnLoad` restores the new
catalogue before the regular sea/location consumers. Only absent shop rows are
created; depleted or disabled existing stock is preserved. Fresh stores use the
same initializer. Sea/non-colony stores receive no newly invented stock; an
existing special row still receives the outside-Bermuda trade policy.

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

The fort candidate passes36 actual-function/solver/flight cases and14 existing
damage/persistence cases in a disposable harness. At50/150/300m and gun heights
5/30m, bursts enter the22m sphere at least4m above the gun and before the independent
wall plane. The initial50m/high-gun sphere alone burst3.3m below the gun; the
overhead restriction corrects that rejected candidate. Long-frame arming,
unarmed20m, blocked/missing/destroyed/non-normal/own fort, earliest ship/fort and
ordinary next-shot state pass. Existing damaged guns/count survive the actual
persistence loop; projectile and fort Save/Load source are unchanged. Collision,
model and event dependencies are stubs; the installed fort replay remains open.

The actual native VM accepts the store policy and quick-menu toggle with zero
script errors, including a save/load round. Depleted/disabled rows retain stock
and prices while trade types reconcile; Bermuda, other colonies, absent rows
and non-colony cases pass. Ordinary ammunition/cargo remain intact. Toggle
default/persistence, companion/gunless rejection and unchanged loaded/cargo state
pass; full firing lifecycle and real menu pixels are separate acceptance.

The icon producer pixels, decoded TX cells and64/128px comparisons pass. A
disposable check of the real delivery owner admits both exact original atlases,
preserves existing mast/perk art and rejects unknown atlas edits and unrelated
texture paths. An initial fixture used macOS's `/var` symlink and was rejected
before testing; resolving its path fixed the fixture. Both exact prepared atlases
are installed with the corrected fuze; in-game icon readability remains unverified.

The installed navigator atlas copies that prepared normal-state tile into the
separate charge indicator. Its original header and every byte outside cell40
remain identical. The actual delivery owner admits this exact original atlas
and rejects an unknown navigator edit or an unrelated texture. The decoded
48px five-ammunition comparison preserves stock icons and shows the drawn bomb;
installed navigation pixels remain a separate replay requirement.

Full PROGRAM compilation caught the unavailable script API `GetTickCount` in
the initial VFX limiter. Its canonical correction uses `PostEvent` and a ready
handler, registered/removed with the balls environment and reset on creation.
The actual native event loop keeps two rapid blasts but only one heavy effect
through119ms, then permits the next heavy effect at120ms, with zero script
errors. The same native compiler accepts the full startup program plus changed
catalogue/store/control segments using installed executable-relative headers;
an initial disposable missing-header setup error is not a product failure.

The latest correction passes30 disposable actual-function cases under Address
and Undefined Behaviour sanitizers: the shared solver and native flight equations
enter lower ship volumes at50/150/300m for box heights20/60m; sampled burst
heights are9.42–10m and10–16m respectively. Fort paths at those distances and gun
heights5/30m still burst at least4m above the gun and before the wall plane.
Below/outside/stationary, unarmed, blocked, missing-model, destroyed, own and
non-normal cases retain their rejection. Actual `AIBalls::Execute` emits16 water
impact events, gives cosmetics zero damage, restores ordinary next-shot state,
and safely inserts/releases16 tracers after a burst. Actual projectile Save/Load
preserves the fragment marker in the existing lane and restores its tracer.
An independent cloth plane at18m received zero hits from the old downward fan;
the corrected fan reaches it with four sweeps and goods51 through the same cloth
trace owner. The water cases use actual `SEA::Trace`/`SEA::Cannon_Trace` with a
flat-wave fixture; each unoccluded fragment produces one impact event.
The native cloth damage path is `SAIL::Cannon_Trace` → `SAILONE` → `DoSailHole` →
`ProcessSailDamage` → `GetRigDamage`, which reads `AIBalls.CurrentBallType`.
The nearest ordinary following shot resets that field to the original lane good.
Collision/model/particle/event dependencies are bounded stubs; real sea pixels,
wave collision, broadside frame cost and the new script splash branch remain
the integration owner's runtime/compiler checks.

Canonical build/staging and full PROGRAM compilation pass under the integration
owner. Gameplay acceptance requires an existing-save shop purchase/selection,
an above-target burst with resulting hull/crew/sail state, nearby-friendly risk,
an unarmed ordinary-bomb impact, and a neighboring ordinary-bomb shot. Stock
effects alone do not prove the requested spectacle or game FPS. Runtime attempts
and the consuming engine/content hashes belong to `docs/runtime.md`.

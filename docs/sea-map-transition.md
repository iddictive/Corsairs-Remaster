# Sea and global-map camera handoff

## Contract

Sea exit first tilts smoothly to an overhead view of the player's ship, then
lifts vertically with nonlinear scale easing. The visual handoff completes when
its projected size approximately matches the map miniature. Return to sea
reverses that scale path. Overhead is only the scale-matching handoff pose; the
global map settles to its ordinary oblique view. The native map and sea actions invoke the transition;
there is no extra menu button. No loading artwork or black frame belongs to
this transition. Completion requires a real installed-game recording in both
directions, including the first usable destination frame and a denied entry.

## Owners

- `experiments/native-metal/sea_map_transition.hpp`: one transient entity owns
  camera, GPU frame capture, readiness-gated reveal and disposal.
- `world_map_player_model.hpp`: generic GM footprint and projected-area math;
  it normalizes the actual miniature without changing map collision or NPCs.
- `world_map_ship_visual.hpp` and direct engine rigging/worldmap sources: independently
  owned actual player hull, mast hierarchy and static cloth. Live sea departure
  copies the current visual before teardown; saved-map entry reconstructs from
  the canonical player/RealShips descriptor and existing sail-material event.
- `ship_hull_state.hpp`: one persisted hull-cut parser shared by live BLOTS and
  the map reconstruction; malformed records retain the existing rejection.
- Direct engine `sea_cameras.cpp`, `wdm_camera.cpp`, `world_map.cpp` and renderer
  progress route own registration, camera exclusion, reveal and readiness.
- `world_map_resources.hpp` and engine `world_map_resources.cpp`: immutable
  island geometry and the shared34 sea-texture paths, prepared during ordinary
  sea frames. The resource entity owns them within a core epoch; map consumers
  keep a shared lease through reset. Mutable counter-sky geometry stays local.
- Engine WorldMap/Islands/Sea `LoadNext`: incremental map preparation. Ordinary map/save loads
  drain the same preparation synchronously; cinematic loads prepare islands,
  water textures and encounter records on regular engine frames. A complete
  first map render precedes `WorldMap_TransitionReady`.
- Canonical `PROGRAM/sea_ai/sea.c`, `PROGRAM/worldmap/worldmap.c` and
  `PROGRAM/worldmap/worldmap_reload.c`: admission, survivor handback, weather
  ownership, load callbacks, interface restoration and transient flag cleanup.

Final sea distance is solved from the actual hull/mast/cloth bounds, the saved map camera
height/yaw, north-up rotation lock, perspective and its normalized miniature. Equal projected bounding area is
an approximation of visual size, not proof of equal silhouettes. No fixed sky
height or extra map zoom follows the size match. The same-size `StretchRect`
snapshot stays on GPU; the legacy fader's synchronous CPU readback is avoided.
The rejected near-opaque fog and artificial flat cloud overlay stay absent.
Natural scene weather retains its existing owner. The camera tilts for400ms
before an850ms lift; matched frames reveal for300ms while the map eases to its
ordinary1-radian pitch. Saves carrying the former forced1.565 overhead pitch
normalize to that ordinary angle; existing oblique values remain intact.

### Weather at destination entry

`Whr_CreateWeatherEnvironment` binds the native weather state, publishes
`Weather.isDone`, then sends the outside flag (`1464336388`). Only that final
message completes `visualSceneBindPending`: the outside context selects the
authored storm palette. Consuming the flag at `isDone` seeds the previous map
palette and makes the four-second color filter catch up after sea entry.
Ordinary updates without a new bind retain temporal smoothing.

A source-linked native Metal replay of this message order on October25 used a
fixed saved storm and clear/storm palettes. Before the repair, the target was
`00d09030` while the first visual fog was `002c2c2c`; after it both were
`00d09030`. A subsequent outside change without a bind kept the prior visual
color, preserving smoothing. This verifies the native state transition; the
complete installed camera reveal and the player's save still need visual replay.

Live ship translation and heading remain followed during ascent/descent; the
ordinary exit commits its final position, while explicit XZ/AY exits retain their
authored destination. Raw engine frame time drives flight independently of game
acceleration, and the existing pause revision prevents a jump after a pause.
Map simulation/calendar remain fixed while its transition owns the camera, so
night encounter admission cannot become a different date during the effect.

Visual bounds are measured once at departure and once when the returning sea
scene is ready. Cloth bounds read the existing CPU vertex buffer without new
GPU allocations. At the apex the owning visual refreshes its live pose and
cloth. A retained sea traffic host keeps its player visual; only a missing host
uses pending ownership that transfers to the map player or is discarded on abort.
No sea SHIP, collision, weather or live
node callback is retained. Broken branches and saved cloth holes remain visible
state inputs. Ship/material/damage changes invalidate only the player's visual;
ordinary map ships and the physical reference footprint retain their owners.

SEA_CAMERAS Init starts or rearms a resource-only prewarm entity. Each ordinary
sea Execute performs at most one static GM or texture operation, including when
the existing sea traffic host is present; no second map,
weather, calendar, traffic or encounter owner is initialized. WdmObjects cache
misses previously inserted index−1, reloading repeated models; insertion is now
corrected. Immutable islands borrow the retained pool; mutable counter models
keep per-map ownership. Geometry loading restores the caller's texture path.
Missing optional models are memoized within the same epoch. Sea consumers retain
their own renderer references and share the exact original34-texture sequence.

After static preparation, that entity warms the owning player model. At the apex
it refreshes live nodes and cloth without rereading its GM; unchanged hull-cut
signatures retain prepared geometry. Repair/removal invalidates cut state.
The warm copy is bound to its source sea-model entity. A failed reconstruction
is parked for that source, model/character identity and existing visual signature;
changed attributes, a recreated sea model or explicit disposal permit retry.
An unchanged failure must not reload/rebuild the miniature on every Execute.
Ordinary sea Execute continues polling already-admitted miniature material jobs
after warmup succeeds; it does not decode attributes or refresh/reconstruct the
visual. This releases completed worker slots even though the sea never draws
that independent model. Map realization takes over polling after ownership transfer.
Material reconstruction shares SHIP's physical-node domain (excluding `path`)
and validates all required checkpoint records before admitting any node work.
The October17,00:48 player profile binds installed UUID `E755379B`:1157 of1426
main-thread samples are in map-ship preparation, including533 in synchronous
`RebuildLaserSurface`. The [sample and matching signed executable](../.local-archives/metal-renderer-evidence/phonk-current-20261017/player-ram-map-warm-0048/evidence.json)
retain the exact evidence. Cold material restore now constructs its initial
boundary inside the existing cooperative worker, including legacy-cut import.
These source guards remove the unbounded retry path; sustained FPS and damaged
sea/map/save-load parity remain for player replay.
Core reset disposes pending visual references before renderer destruction. Normal
application exit separately calls the same `discardSeaVisual` owner after
`ExitApplication` and before `CoreImpl::CleanUp` releases renderer services.
The pending visual is a static owner; resource-entity reset alone does not cover
Cmd-Q, which otherwise leaves its sail handles for static destruction after the
renderer is gone. This shutdown correction is local source only; installed
Cmd-Q replay remains pending. Abort
rearms an existing resource owner without reset-time entity creation. An immediate
exit before warmup completes still uses cold loading, and one GM operation is
indivisible. The reverse cold fallback's SeaLogin stays synchronous. These limits prevent claiming
universally stall-free transitions.

Map-to-sea preserves `WdmAddEncountersData` then `GreatRecipe_MapAdmission(&wdmLoginToSea)`
before releasing the map, and `GreatRecipe_PrepareSea` before the ordinary
`EVENT_SEA_LOGIN` on cold entry or `QuestsCheck` on retained entry. The
destination's initial rendered frame gates descent.
Return to the retained atlas scene uses `Sea_ResidentResume`, including authored
encounters, offshore dispatch and storm contact. Cold and retained entry share
`Sea_LoginEncounterGroups`; retained entry keeps native actors, counters, berths
and existing group tasks, admitting only missing actors within the existing
capacity. Player/encounter coordinates and wave direction use the retained
atlas frame. Quest redirects precede layer/HUD restoration; GreatRecipe retains
its matching live captain. Missing scenes or failed native handoff still recover
through synchronous SeaLogin;
resource progress calls present the retained GPU map frame. This is not an
asynchronous sea loader, and indivisible I/O remains measurable.

## Transition diagnostics

Ordinary public launches discard stdout/stderr. Use the existing native
`getOrCreateLogger("sea-map-transition")` owner: it writes
`Logs/sea-map-transition.log` beneath the configured `STORM_USERDATA` root
and flushes at transition completion. The quiet-launch and unavailable-storage
CPU replay is in the [transition evidence](evidence/sea-map-transition-20261023/README.md).
Verify that route before relying on a player recording; changing broad logging
settings or asking for another launch does not prove the route works.

Read phase timestamps, the cold-admission reason and step intervals together.
The rejection reason is captured before clearing the cold-login object and
before discarding the resident scene, then emitted when the native transition
starts. Current rejection reasons distinguish missing scene/map and failed
transition creation; successful resident admission clears its
temporary field. If both native transition creation attempts fail, the legacy
fader remains the recovery owner and cannot emit this entity's trace.
CPU durations and eligible inter-execute gaps are different measurements;
neither establishes GPU completion or the absence of visible frozen frames.

## Evidence and rejected approaches

Source checks: the actual complete script chain compiles in the isolated real VM;
inactive sea entry and locked map entry preserve their guards. Native source
syntax and zero-fuzz patch application pass. An independent rectangular-box
projection oracle and 27 boat/heading/map-height combinations pass the size math.
These checks do not accept motion, frame pacing or installed gameplay.

The actual-player candidate passes the canonical native build and full composed
script VM with current descriptor creation, shipless cleanup and inactive sea
entry. Composition is idempotent and retains the current Great Recipe hooks.
Native source probes cover hull-cut validation and current visibility flags;
these do not accept actual model loading, cloth rendering or cold saved-map entry.

A save/load round on that uninitialized whole-game corpus is an invalid fixture:
the ordinary OnSave/OnLoad callbacks require a seeded player/scene and report
uninitialized references. It does not verify a prior player SAVE or establish a
transition defect. Saved-game entry remains part of installed replay.

Removing only the old fader/progress image is rejected: WorldMap constructors
block the engine thread. Background-thread map construction is also rejected:
renderer/core tables, geometry paths, weather and map registries have shared
mutable owners. Incremental preparation occurs after sea teardown, while the
GPU handoff remains alive; no second active weather or fleet owner is introduced.
Cheap loader phases share one6ms frame budget instead of each consuming a full
frame. A cinematic sea exit also skips the old island-area clamp; ordinary
saved-map initialization keeps it. Native land collision still protects the hull.

## Unresolved acceptance

The player clarified on October23 that camera motion must remain, with no loading
pause or frozen frame at the sea/map seam. Removing the flight animation does
not meet that request. The [resident camera replay](evidence/sea-map-transition-20261023/README.md)
isolates stale map pose and a one-frame yaw discontinuity; correcting those
does not establish that preparation or first-render stalls are absent.

Installed sea → map → sea replay must prove ship-size/anchor continuity,
day/night visibility, absence of visible loading stalls, retained encounter
and fleet semantics, normal controls after reveal and no stranded transition.
One model load or label/font batch can still exceed a frame budget. A compiler,
staging receipt or scalar size check must not promote that gap to a visual pass.
Current installation/replay verdict belongs to `docs/runtime.md`.

The oblique-view successor passes the actual angle-body lifecycle, historical
overhead normalization/ordinary-angle negative and27 existing size matches.
Resource actual-body ASan/UBSan cases cover repeat/case/collision keys, null and
missing models, texture scope, reset leases, inactive/transition gates and callback
retry/rearm/disposal. Old/new WdmSea paths/readiness match over36 calls. These are
component/source receipts; no autonomous game launch or player SAVE inspection
is authorized. Player replay owns actual timing/appearance.

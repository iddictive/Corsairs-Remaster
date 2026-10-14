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

Live ship translation and heading remain followed during ascent/descent; the
ordinary exit commits its final position, while explicit XZ/AY exits retain their
authored destination. Raw engine frame time drives flight independently of game
acceleration, and the existing pause revision prevents a jump after a pause.
Map simulation/calendar remain fixed while its transition owns the camera, so
night encounter admission cannot become a different date during the effect.

Visual bounds are measured once at departure and once when the returning sea
scene is ready. Cloth bounds read the existing CPU vertex buffer without new
GPU allocations. At the apex the owning copy obtains its own geometry, buffer
and texture references; pending ownership transfers to the map player, or is
discarded when the transition aborts. No sea SHIP, collision, weather or live
node callback is retained. Broken branches and saved cloth holes remain visible
state inputs. Ship/material/damage changes invalidate only the player's visual;
ordinary map ships and the physical reference footprint retain their owners.

SEA_CAMERAS Init starts or rearms a resource-only prewarm entity. Each ordinary
sea Execute performs at most one static GM or texture operation; no second map,
weather, calendar, traffic or encounter owner is initialized. WdmObjects cache
misses previously inserted index−1, reloading repeated models; insertion is now
corrected. Immutable islands borrow the retained pool; mutable counter models
keep per-map ownership. Geometry loading restores the caller's texture path.
Missing optional models are memoized within the same epoch. Sea consumers retain
their own renderer references and share the exact original34-texture sequence.

After static preparation, that entity warms the owning player model. At the apex
it refreshes live nodes and cloth without rereading its GM; unchanged hull-cut
signatures retain prepared geometry. Repair/removal invalidates cut state.
Core reset disposes pending visual references before renderer destruction. Normal
application exit separately calls the same `discardSeaVisual` owner after
`ExitApplication` and before `CoreImpl::CleanUp` releases renderer services.
The pending visual is a static owner; resource-entity reset alone does not cover
Cmd-Q, which otherwise leaves its sail handles for static destruction after the
renderer is gone. This shutdown correction is local source only; installed
Cmd-Q replay remains pending. Abort
rearms an existing resource owner without reset-time entity creation. An immediate
exit before warmup completes still uses cold loading, and one GM operation is
indivisible. Reverse SeaLogin stays synchronous. These limits prevent claiming
universally stall-free transitions.

Map-to-sea preserves `WdmAddEncountersData` then `GreatRecipe_MapAdmission(&wdmLoginToSea)`
before releasing the map, and `GreatRecipe_PrepareSea` before the ordinary
`EVENT_SEA_LOGIN`. The destination's initial rendered frame gates descent.
Reverse sea construction still uses the established synchronous SeaLogin;
resource progress calls present the retained GPU map frame.
This is not an asynchronous sea loader, and indivisible I/O remains measurable.

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

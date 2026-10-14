# Sea and global-map camera handoff

## Contract

Both directions use a stepped, nonlinear camera move like the requested GTA V
character switch. Sea exit keeps the player's ship as the focal object, lifts
and tilts the camera through three scale impulses, and completes the visual
handoff when its projected size approximately matches the map miniature. Return
to sea reverses that scale path. No loading artwork or black frame belongs to
this transition. Completion requires a real installed-game recording in both
directions, including the first usable destination frame and a denied entry.

## Owners

- `experiments/native-metal/sea_map_transition.hpp`: one transient entity owns
  camera, GPU frame capture, cloud veil, readiness-gated reveal and disposal.
- `world_map_player_model.hpp`: the map player model name, shipped GM-header
  bounds and projected-area math. WorldMap and the transition consume that name.
  Sea hull/mast bounds come from the actual main-character native ship.
- `sea-map-transition.patch`: native registration, map camera input exclusion,
  and the existing progress-present route's animated handoff branch.
- `sea-map-load.patch`: incremental map preparation. Ordinary map/save loads
  drain the same preparation synchronously; cinematic loads prepare islands,
  water textures and encounter records on regular engine frames. A complete
  first map render precedes `WorldMap_TransitionReady`.
- Canonical `PROGRAM/sea_ai/sea.c`, `PROGRAM/worldmap/worldmap.c` and
  `PROGRAM/worldmap/worldmap_reload.c`: admission, survivor handback, weather
  ownership, load callbacks, interface restoration and transient flag cleanup.

Final sea distance is solved from the miniature's GM bounds, the saved map camera
height/yaw, north-up rotation lock, perspective and actual ship bounds. Equal projected bounding area is
an approximation of visual size, not proof of equal silhouettes. No fixed sky
height or extra map zoom follows the size match. The same-size `StretchRect`
snapshot stays on GPU; the legacy fader's synchronous CPU readback is avoided.
Clouds reuse the shipped map texture and existing interface blend technique.

Live ship translation and heading remain followed during ascent/descent; the
ordinary exit commits its final position, while explicit XZ/AY exits retain their
authored destination. Raw engine frame time drives flight independently of game
acceleration, and the existing pause revision prevents a jump after a pause.
Map simulation/calendar remain fixed while its transition owns the camera, so
night encounter admission cannot become a different date during the effect.

Map-to-sea preserves `WdmAddEncountersData` then `GreatRecipe_MapAdmission(&wdmLoginToSea)`
before releasing the map, and `GreatRecipe_PrepareSea` before the ordinary
`EVENT_SEA_LOGIN`. The destination's initial rendered frame gates descent.
Reverse sea construction still uses the established synchronous SeaLogin;
resource progress calls present moving clouds over the retained GPU map frame.
This is not an asynchronous sea loader, and indivisible I/O remains measurable.

## Evidence and rejected approaches

Source checks: the actual complete script chain compiles in the isolated real VM;
inactive sea entry and locked map entry preserve their guards. Native source
syntax and zero-fuzz patch application pass. An independent rectangular-box
projection oracle and 27 boat/heading/map-height combinations pass the size math.
These checks do not accept motion, frame pacing or installed gameplay.

A save/load round on that uninitialized whole-game corpus is an invalid fixture:
the ordinary OnSave/OnLoad callbacks require a seeded player/scene and report
uninitialized references. It does not verify a prior player SAVE or establish a
transition defect. Saved-game entry remains part of installed replay.

Removing only the old fader/progress image is rejected: WorldMap constructors
block the engine thread. Background-thread map construction is also rejected:
renderer/core tables, geometry paths, weather and map registries have shared
mutable owners. Incremental preparation occurs after sea teardown, while the
GPU handoff remains alive; no second active weather or fleet owner is introduced.

## Unresolved acceptance

Installed sea → map → sea replay must prove ship-size/anchor continuity, cloud
appearance at day/night, absence of visible loading stalls, retained encounter
and fleet semantics, normal controls after reveal and no stranded transition.
One model load or label/font batch can still exceed a frame budget. A compiler,
staging receipt or scalar size check must not promote that gap to a visual pass.
Current installation/replay verdict belongs to `docs/runtime.md`.

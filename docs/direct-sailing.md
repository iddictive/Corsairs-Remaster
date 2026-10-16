# Direct sailing between island regions

## Status

October16 correction in progress: the installed automatic sea→map→sea camera
trip was rejected. Its normal survivor/admission chain remains useful, but the
player must stay in the sea view throughout the region handoff.

## Contract

Ordinary local sailing into another visible island's map region automatically
loads that sea scene and resumes sailing in the same camera mode, orbit and zoom.
Automatic travel must show no global map, camera ascent/descent, map interface,
map audio or loading artwork. A brief load must retain the outgoing sea image
until the first complete destination sea frame. Manual map travel keeps its
existing animation. This reuses the normal scene rebuild; it does not claim
continuous simulation or simultaneous island streaming during that rebuild.

The correction reuses one existing GPU frame/camera transition owner in a
sea-only mode, and retains/rebinds the existing sea cameras. The map remains an
invisible descriptor/admission adapter: its Execute prepares the region, while
its Realize, UI, music and calendar never run. Readiness starts the ordinary
return without a second transition. A failed frame capture must cancel before
teardown; a quest redirect must release retained cameras and transient locks.

The decisive checks are no map draw/camera/UI side effects between the two sea
frames, preserved camera state across the new native ship identity, and the
existing formation/motion/survivor cases. Installed Barbados→Antigua and a
scale25↔50 route must still establish the appearance and duration of the load.

## Owners

- `experiments/native-metal/world_map_island_area.hpp`: the same authored
  `_area.gm` membership predicate for the native map and local sea travel.
- Native `worldmap/src/world_map_resources.cpp`: queries the existing immutable
  asset pool once per second after prewarm, in base-locator order. It creates
  no second map or weather owner.
- `PROGRAM/sea_ai/direct_sailing.c`: admission, transient handoff, fleet formation,
  motion/sail and Phonk restoration, and hidden descriptor handback.
- `PROGRAM/sea_ai/sea.c` and `PROGRAM/worldmap/worldmap_reload.c`: the ordinary
  survivor/descriptors, loading, quest admission and scene construction chain.
- Native `worldmap/src/world_map.cpp`: publishes the destination region from
  Execute; direct travel never renders or moves the map camera.
- `sea_map_transition.hpp`: one GPU sea-frame hold, capture failure cancellation,
  translated view and reveal after the new sea has rendered.
- `PROGRAM/sea_ai/AICameras.c` and native `sea_cameras/src`: retain camera
  entities, rebind their old scene caches, and preserve orbit/deck/aim state.

Native module paths are relative to `experiments/native-metal/engine/src/libs`;
PROGRAM paths belong to `src/gameplay`. Its manifest delivers the new script.

## Discovery and rejected alternatives

Previously only `SeaLogin` and `Sea_Load` mounted an island. Sailing did not
change `AISea.Island`, while `wdmCreateMap` exported the accumulated ship
position using `worldMap.zeroX/zeroZ` and scale25/50. Native `_area` meshes
already partition map travel; Antigua covers X768–1024, while its southern
border meets the Barbados area at Z−170.6666. These meshes, rather than distance
from an island centre, own region entry.

Duplicated scale lists in map entry/exit delegate to existing
`worldmap_coords.c:GetSeaToMapScale`, also used by sea coordinates, weather and
traffic. Existing scale25 and scale50 cases retain their values.

Blind `Sea_Save`/`Sea_Load` into a new island is rejected: the native snapshot
contains undelimited old-fort records consumed against the new fort roster.
Moving `Ship.Pos` alone leaves native tasks, projectiles and camera caches
in the old frame. Terrain replacement under live actors needs translated
depth/path/fort/foam/cut consumers and retained old terrain; that is the larger
streaming rewrite excluded here.

## State and acceptance

The normal survivor/admission and SeaLogin chain remains the scene owner; its
map adapter is invisible and does not invoke map UI/audio/init/date events.
Map-entry locks,
boarding, tsunami/tornado, player death and open interfaces defer automatic
travel; it never removes an active battle to force a region change. It retries
silently when ordinary admission becomes possible. Empty ocean, the same region
and hidden islands do not trigger a reload.

Transient `worldMap.directSail` owns one retained sea view and one descriptor
return. Readiness is consumed once; it never starts a reverse map transition.
The ordinary survivor handback preserves traffic damage/crew/cargo; player and
companion persistent Ship attributes retain their owner. Capture immediately
before teardown retains fleet separation, headings, speed, sail state and active
Phonk settings. Existing group-position callbacks and one-shot native impulses
restore formation/motion. No save binary change, new physics setter, terrain
transform or permanent force is introduced. Normal sea-login AI task/reload
semantics remain; this is not arbitrary active-scene migration. Quest redirects
cancel the temporary callback; normal save loading clears an interrupted handoff.

Acceptance requires sailing Barbados→Antigua and a scale25↔scale50 route in
the installed app: the next island, collision, fort and landing choices must
belong to the reached region; map coordinates/heading must agree after return;
fleet damage/cargo/formation and continuing motion must survive. Ordinary manual
map travel must still work. Battle/quest denial must retain the original scene
without repeated logs or a frozen map. Components do not accept the appearance
or duration of the camera/loading transition.

## Verification

The original three native translation units pass the current canonical compiler
flags with `-fsyntax-only`. The isolated actual script VM compiles the complete
projected source chain with zero script errors. A disposable actual-body fixture
passes coordinate-scale, same/hidden-region, battle/quest/boarding/tsunami/UI,
denied admission, single-return, formation/motion/Phonk and callback restoration
cases. Native entity/message calls are stubbed there. The shared area predicate
matches its predecessor over468999 samples on31 installed `_area.gm` meshes;
the Barbados→Antigua and null/nonfinite cases pass. Its Clip fixture is CPU
triangle clipping, not a game render or asset-loader replay.

The corrected automatic path requires a captured sea frame. Failed entity
creation or capture cancels before teardown; it does not fall back to map/loading
art. The ordinary manual-map fader and cinematic remain separate unchanged paths.
Quest redirects restore the formation callback and delete retained cameras;
their destination still owns controls/HUD. Pending quest encounter materialization
in the hidden map adapter remains an inherited acceptance limit: unlike normal
map simulation, it does not drain the map's delayed encounter queue.

Retained compact inputs, source/binary/asset hashes and results belong to
`.local-archives/direct-sailing-20261016`. The component fixture runs through
`experiments/native-metal/.cache/build/bin/script-vm-probe <fixture-directory> Run 0`.
Its optional save-codec run is not accepted: the probe reports `Invalid function`
even on an unrelated three-line saved-object fixture. Current player SAVE loading,
automatic camera roundtrip, native impulses and sea-scene continuity remain
installed-runtime acceptance gaps.

Sea-only correction evidence belongs to
`.local-archives/direct-sailing-sea-only-20261016`: canonical native syntax,
full projected script compilation and actual-body VM cases (native calls stubbed).
The installed sea-only view, load duration, held deck aim, first-frame alignment
and ordinary manual map journey remain pending until this source batch is staged
and replayed. A retained framebuffer is not proof of a continuously running sea.

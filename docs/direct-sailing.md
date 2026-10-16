# Direct sailing between island regions

## Status

Source candidate, October16. Installed activation and player replay are pending.

## Contract

Ordinary local sailing into another visible island's map region automatically
loads that sea scene and resumes sailing, without requiring a manual map visit.
A brief existing camera/loading transition is allowed; simultaneous island
streaming and a large engine rewrite are outside this change.

## Owners

- `experiments/native-metal/world_map_island_area.hpp`: the same authored
  `_area.gm` membership predicate for the native map and local sea travel.
- Native `worldmap/src/world_map_resources.cpp`: queries the existing immutable
  asset pool once per second after prewarm, in base-locator order. It creates
  no second map or weather owner.
- `PROGRAM/sea_ai/direct_sailing.c`: admission, transient handoff, fleet formation,
  motion/sail and Phonk restoration, and automatic return from the map.
- `PROGRAM/sea_ai/sea.c` and `PROGRAM/worldmap/worldmap_reload.c`: the ordinary
  survivor/descriptors, loading, quest admission and scene construction chain.
- Native `worldmap/src/world_map.cpp`: publishes the destination region before
  the first map frame and holds input/calendar/encounters stationary until return.

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

The normal sea→map→sea chain remains the scene transition. Map-entry locks,
boarding, tsunami/tornado, player death and open interfaces defer automatic
travel; it never removes an active battle to force a region change. It retries
silently when ordinary admission becomes possible. Empty ocean, the same region
and hidden islands do not trigger a reload.

Transient `worldMap.directSail` owns one outgoing transition and one return.
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

The three changed native translation units pass the current canonical compiler
flags with `-fsyntax-only`. The isolated actual script VM compiles the complete
projected source chain with zero script errors. A disposable actual-body fixture
passes coordinate-scale, same/hidden-region, battle/quest/boarding/tsunami/UI,
denied admission, single-return, formation/motion/Phonk and callback restoration
cases. Native entity/message calls are stubbed there. The shared area predicate
matches its predecessor over468999 samples on31 installed `_area.gm` meshes;
the Barbados→Antigua and null/nonfinite cases pass. Its Clip fixture is CPU
triangle clipping, not a game render or asset-loader replay.

Review found and repaired the failed-native-transition fallback: its ordinary
fader has no native Done event, so readiness explicitly releases its reload lock.
The VM fixture proves lock retention while a native transition exists, lock
release for the ready fader fallback, and exactly one return afterward.

Retained compact inputs, source/binary/asset hashes and results belong to
`.local-archives/direct-sailing-20261016`. The component fixture runs through
`experiments/native-metal/.cache/build/bin/script-vm-probe <fixture-directory> Run 0`.
Its optional save-codec run is not accepted: the probe reports `Invalid function`
even on an unrelated three-line saved-object fixture. Current player SAVE loading,
automatic camera roundtrip, native impulses and sea-scene continuity remain
installed-runtime acceptance gaps.

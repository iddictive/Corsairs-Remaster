# Resident map camera continuity

The player reports that sea/map entry and return still jump. This receipt
covers two reproduced map-camera defects, not full installed-game acceptance.

`WdmCamera::Init` previously retained the old ship-heading spring and free-camera
blend when the resident map reopened after sailing elsewhere. The actual camera
bodies reproduce a two-radian heading mismatch and, after free-camera use, a
1144.77 map-unit anchor mismatch. Reinitialization now rearms the constructor's
existing first-pose path instead of carrying those transient values across scenes.

Changing between cinematic follow and the saved free-camera mode also updated
the stored yaw while rendering the old local copy for one frame. The first frame
lost the ship heading; switching back counted it twice. Synchronizing the local
copy inside that existing state change removes both discontinuities.

Run from the repository root with an empty, caller-owned scratch directory:

```sh
TASK_SCRATCH_DIR=/path/to/scratch python3 docs/evidence/sea-map-transition-20261023/replay.py
```

The replay compiles the actual `wdm_camera.h/.cpp` bodies from baseline
`adbc57a3c0235273a5961cce8531c5c709088329` and current source. Fixtures provide
only player pose, controls, transition activity and renderer camera capture.
Ordinary new-map initialization and north-up lock are compatibility negatives.
[Results](results.json) show zero heading/anchor errors after the correction;
[the intermediate replay](before-mode-fix.json) isolates the mode-switch defect
after the initialization fix. The million-call component timing is about 21ms
for both versions; it is not a game FPS measurement.

The camera-state correction introduces no per-frame work or allocation. Full sea-to-map
and map-to-sea motion, loading stalls and installed-game FPS remain player-owned
acceptance; an offline camera replay cannot establish those properties.

## Loading-stall evidence boundary

The camera correction adds no per-frame work. Separate, transition-only timing
records phase timestamps and the largest new frame gap of at least50ms to the
existing native logger's `Logs/sea-map-transition.log` in the configured userdata
directory. Pause/revision changes and a zero time scale are
excluded. `resident-entry cpu_ms` measures the retained-host activation;
`first-map-render cpu_ms` measures its initial draw submissions. `map-ready`
follows the script's interface/fort setup; `scene-ready` follows the first sea
frame. These are CPU/wall timings, not GPU execution timings or proof of a fix.

The existing sea traffic host prepares navigation/geometry at sea creation and
calls `PrepareMapView` during ordinary sea execution. Its map rendering remains
inactive until the handoff. Initial map drawing can create uncached Metal pipeline
states synchronously; script fleet snapshots, encounter restoration and interface
creation also remain on the commit/readiness path. No current player trace
distinguishes those candidates. The ordinary launcher disables the existing broad
profiler. The first installed timing attempt used stderr, but the player's
ordinary app process had both streams connected to `/dev/null`; that attempt
captured no transition data. The corrected route uses the existing native
`getOrCreateLogger`/`fs::GetLogsPath` owner, flushes on transition completion and
does not alter player settings or show diagnostics in the game UI. Unavailable
storage disables this optional logger instead of failing scene entry.

The player replayed installed engine `23fc682a041628bd5a730788c484afb51b829c2816bc3da78f90094084cbfa6d`
and reported a return-to-sea freeze and imperfect outbound motion. This reopens
runtime acceptance despite the successful camera component checks. No timing
cause can be inferred from that failed stderr recording.

`logging_replay.py` links the actual existing native diagnostics library and
extracts the current logger factory. With the default logger disabled,
`STORM_QUIET_CLIENT=1` and stdout/stderr both discarded, its canary is written
and flushed. The blocked-directory negative returns the handled failure signal
without aborting. [Raw outcomes](logging-results.json) and the [quiet canary](logging-quiet.txt)
retain that distinction. The [corrected native delivery](logging-build.json) is
installed; another player transition is required to locate the actual stall.

The compatible platform alternative is [asynchronous Metal pipeline creation](https://developer.apple.com/documentation/metal/mtldevice/1433365-newrenderpipelinestatewithdescri?language=objc).
The renderer already caches pipeline states. Defer an asynchronous/cache redesign
until the player's trace identifies a blocking pipeline miss; a first-render
call site alone does not establish it as the observed freeze's cause. No second
map simulation or speculative offscreen scene was introduced.

## First recorded player round trip

[The ordinary-launch trace](player-pass-20261023-184359.txt) comes from installed
engine `540000f178dc790b73a05c337e6e11da18aa270f46ddd9336d81f1a5f72b5f53`.
Outbound commit-to-map-ready took 228.925ms, including resident entry 8.815ms
and the first map render's CPU submissions 27.585ms. The largest recorded
eligible inter-execute gap was 140.713ms. Return commit-to-first-complete-sea
frame took 2136.469ms; the recorded eligible gap reached 271.130ms. These phase
intervals include script execution, scene preparation and rendering; they do
not identify one function or prove one uninterrupted 2.14-second CPU stall.

The next bounded diagnostic adds transition-only step timestamps around map
release, resident resume/cold login, quests, island selection, sound, battle
interface and traffic admission. It preserves the existing call order and
all animation/readiness rules. The native attribute callback uses the same
quiet file logger. A real script-VM probe of the actual `Sea_TransitionTrace`
and `WdmSeaTransitionCommit` bodies returned `Run=0; script_errors=0`: release
still precedes resume, and an absent transition receives no trace attribute.
The surrounding scene callbacks were stubs; this is not game acceptance.

[The second player trace](player-pass-20261023-185616.txt), from the installed
engine in [steps-build.json](steps-build.json), identifies cold sea login:
1590.759ms inside that call, followed by another 361.254ms before the first
complete sea frame. No resident-resume step ran. Outbound daily hooks cost
64.159ms; map creation cost 20.053ms, and first render submissions 24.434ms.
This distinguishes scene reloading from a camera interpolation defect, but
does not record which admission guard selected the cold path.

One admission defect is independently reproduced: native
`WdmPlayerShip::TestInStorm` clears `playerInStorm` when contact ends but retains
the previous nonempty `stormId`. The resident-return gate treated that old
identity as an active authored storm and forced cold loading. It now consults
the existing current-contact flag too. Active authored storms, missing legacy
contact data, selected authored encounters and offshore quest dispatch preserve
their existing admission path; ordinary native weather needs no authored reload.

With an empty caller-owned `TASK_SCRATCH_DIR`, run
`python3 docs/evidence/sea-map-transition-20261023/resume-admission-replay.py`.
The actual script guards execute in the real VM: baseline `Run=2`, current
`Run=0`, both with zero script errors. The full saved-scene/entity preconditions,
later scene creation and frame pacing are outside this CPU oracle. The recorded
player pass does not establish that stale storm contact was its particular
fallback cause. The player forbids further requested replay; do not claim the
complete zero-stall contract passed or ask them to launch again.

The last trace omitted the admission decision itself. The existing diagnostic
now captures its reason before clearing `wdmLoginToSea` or discarding the sea,
then emits `cold-admission:<reason>` through the already-proven native bridge.
It changes no admission verdict, camera timing or scene callback order.
The extended existing CPU replay checks distinct authored-storm, encounter and
offshore reasons, reason capture across login-object clearing, healthy
admission/cleanup and missing optional metadata. `Diagnostic=0; script_errors=0` accompanies the original
baseline/current guard results. Scene/entity construction remains outside that
fixture. This closes the missing diagnostic field; it does not establish the
cause of the earlier player's cold branch or accept freeze-free gameplay.

## Retained admission and outbound visual reuse

The later implementation removes encounter/storm/offshore selection as reasons
to destroy an otherwise retained sea. Cold and retained entry share the existing
encounter importer; retained entry keeps mounted hulls, damage, task state,
counters and harbour reservations. Missing members are admitted within existing
capacity; a stale `AlreadyLoaded` flag cannot suppress a missing member. The
ordinary cold caller remains in its original order. Descriptor consumption follows
successful transition creation, preserving recovery if creation fails. Quest
redirects run before weather movement, layer unfreeze and HUD restoration.

The existing prewarm was disabled by the sea traffic host's `wdmObjects` global.
It now distinguishes that host from the active map. Warmup, hull-job polling and
the apex refresh share the host-owned player visual, avoiding a second owning
copy that resident map activation never consumed. The visual signature follows
the successful refresh, preventing an immediate first-map rebuild. Current pose,
cloth, damage, failed-replacement retention and the nonresident pending fallback
keep their existing owners. The seven nation-flag liveness guards borrow the map
object instead of deep-copying its whole descriptor tree.

[Raw CPU evidence and exact fixtures](retained-return.json) retain the full-chain
coordinate/weather checks, actual resume body with scene callbacks stubbed, and
nine group cases. All report zero script errors. Marked partial group admission
failed before the two-line guard correction and passes afterward. Coordinates
cover a rotated/translated retained frame and repeated normalization; tsunami
direction rotates as a vector. Resume cases include both redirect mechanisms,
offshore preparation, native weather, explicit tornado, tsunami consumption and
missing retained state. The full PROGRAM compiles after replacing only its game
`Main` with an uncalled name; the fixture driver never initializes gameplay.

The same VM's 8000-row/40-check copy-cost component took 0.617/0.644 seconds
after first startup versus 0.261/0.266 seconds with borrowed guards. These are
whole-process CPU observations, not frame timings or evidence of game FPS.
The canonical native build passes. No further player replay, autonomous game
run, SAVE access or GPU/frame-pacing acceptance was performed. Missing retained
scenes and failed native relocation still have synchronous cold recovery.
The historical `resume-admission-replay.py` is pinned to its original repair
commit; its old authored-rejection policy is superseded by this implementation.

## Missing ordinary fleets on retained return — October 24

The player's current transition log reaches `resident-resume:encounters` and
`resident-resume:traffic`. A real-VM replay of the complete PROGRAM reproduces a
separate admission failure: `WdmFleetSeaImport` reserves `trafficInSea`, retained
`Sea_LoginEncounterGroups` skips tagged fleets, and residency treats the unmounted
reservation as a mounted group. A staged two-hull fleet produces zero mounts,
with zero script errors. Camera readiness does not prove fleet admission.

The repair belongs to `sea.c`'s retained admission boundary: clear only imported
reservations without their original live group, then admit those fleets through
the existing `SeaTraffic_Admit` whole-battle allocator. Clear the complete batch
before admitting its first member. Preserve live groups, roster values, authored
encounters, capacity, harbour reservations and the ordinary cold caller.
Reject partial battle admission and a failed admission that stays sea-owned.

Performance contract: this adds two bounded passes over the existing login
encounter slots once per retained return, using the existing allocator; no
per-frame scan, polling interval or fleet cap changes. The pre-repair VM run
takes 0.398s including whole-PROGRAM compilation. Native model-load cost and
rendered handoff latency remain unmeasured by that CPU fixture. Reproduce with
`codex-run-scratch run --max-bytes 536870912 --max-files 5000 --seconds 120
--reserve-bytes 21474836480 -- python3
docs/evidence/sea-map-transition-20261023/traffic-admission-replay.py before`.
The `after` and `compile` modes use current source; the fixture substitutes
physical mounting, resource readiness and surrounding scene callbacks.

[Baseline](traffic-admission-before.json) returns `Run=2` with zero script
errors; [the repaired replay](traffic-admission-after.json) returns `Run=0`.
It covers the new two-hull fleet, retained roster/position, repeated admission,
the complete five-hull battle, insufficient capacity and readiness recovery.
[Unmodified full-PROGRAM compilation](traffic-admission-compile.json) also
passes with zero script errors. The [player trace](traffic-player-return.txt)
establishes the retained route; it contains no fleet-count instrumentation and
does not prove the repaired ships are visible. Native mounting/rendered round
trip remains unverified. The candidate adds 28 source lines in `sea.c` only;
the one-off fixture is retained evidence, outside the permanent test suite.

## Mainland join and cannon crash follow-up

The installed mainland fill has shared XZ positions with different heights;
the maximum split is 10.9196 chart units. The offline repair assigns one height
per packed GM position, retaining the authored stock surface at the inner join.
Native coastal geometry and navigation frames remain unchanged.

Connective faces also extrapolate the stock continent atlas into its yellow
background. A repeating native ground material removed that padding but was
rejected by the player: a tiled strip does not preserve the authored mainland
image or form an acceptable join. That material replacement is removed from
the candidate. The repair must preserve the mainland's authored visual field,
not merely close holes or eliminate yellow pixels. Mesh/material agreement is
checked in the complete native map view before delivery. Native 960x600
round-trip CPU baseline is 14.649ms/frame; this is not an FPS guarantee.

The supplied native crash resolves to missing `Ship.Cannons` in the actual
`AIShipCannonController::UpdateParameters` call. Skipping that controller while
its character/cannon branch is absent prevents the null dereference, with one
extra attribute lookup per ship/frame. The event that removed the branch in the
player's original crashed process is not established by its stack trace.

The full native first-return probe exposes a separate admission defect:
`WdmAddEncountersData` copies a descriptor over the allocated map slot and loses
`bUse`. The next `FindFreeMapEncounterSlot` reports the missing field and reuses
that slot. Restore occupancy immediately after the replacing copy. This removes
the allocator errors, but the first nearby fleet still fails native admission;
the flag correction alone does not satisfy visible-ship acceptance.
The existing allocator/release contract remains the owner; cost is one attribute
assignment per imported encounter, only during scene entry. The native falsifier
is the selected live five-hull fleet appearing near the player on the first
return, with no `encounters_map.c(7)` runtime errors.

The broadened map-art requirement covers all chart islands. Their chart models
currently retain sea-scale repeating ground materials. The accepting material
must instead be a nonrepeating cartographic albedo in atlas coordinates, derived
from the original map's authored relief. This affects chart render assets only;
sea geometry/materials, navigation and locators retain their owners. Projection
and texture baking stay offline, with no per-frame synthesis. The candidate must
be inspected across the mainland join and island bodies before installation.

The first full-field generated image is only 1254 pixels square and visibly
blurs the original inland when applied alone. It is rejected as the primary
map image. The material producer instead retains the original authored relief
in a 4096-pixel atlas bake, feathering past its obsolete coast outlines into
the authored extension. Chart ground uses upward normals and neutral vertex
colour: sea-cliff lighting must not create dark polygon patches over painted
relief. Beaches, reefs, foliage, towns and all sea-scene materials are retained.

The decisive native rejection is `Group_FindGroup`: the selected five-hull
fleet has a saved script group with five members and zero native actors.
Other stale groups even reference slots already reused by another fleet.
Admission now retains a group only when it has this fleet's actual native
actor; a group owned by another identity remains protected. Once the whole
battle passes the unchanged capacity/resource checks, delete only its stale
script group and rebuild through the existing roster mount. No actor is
deleted by this recovery. `HasOriginalGroup` likewise requires a native entity.
Cost is a bounded member scan only when an admission finds an existing group;
the existing residency cadence and ship limits remain unchanged. The primary
falsifier remains the first selected five-hull fleet, not an unrelated boat.

The first native admission after that repair creates all five selected hulls
(actors 940–944) around the player, nearest 86.4 sea units. A subsequent full
three-round-trip run exits 0, reports matching script/native counts of
5 → 32 → 32 → 31, retains six nearby traffic hulls and renders actor 940 through
the normal sea camera. The existing capacity/battle VM replay still passes.
Copying the incoming Task also replaces its default Lock field; restore false
only when absent, preserving an explicit incoming lock.

The replay now headlessly compiles its exact private PROGRAM before creating
the graphical engine. A duplicate function-scope variable exposed by native
startup had shown an OS alert despite the hidden test window; it is corrected.
The probe also uses the normal `DeleteSeaEnvironment` teardown before erasing
all VM entities. An unordered fixture-wide erase had destroyed particle owners
before active projectile children; normal scene teardown exits cleanly. These
are probe lifecycle findings, not evidence of another player cannon crash.
Known weather/island-density warnings from the loaded scene remain in raw
results; the allocator and Task.Lock warnings are absent after repair.

Finalization at the player's explicit stop/install request: deliver the current
candidate without further art iterations. [Three native round trips](native-round-trip.json)
and [the final chart replay](native-chart-views.json) retain the observations;
[Yucatan](native-yucatan.png) and [the mounted fleet](native-fleet.png) are actual
engine frames. The map art is not fully accepted: the north-west extension has
been matched more closely to the original reference, while the other authored
regions still differ in forest scale/colour. Native union boundary points also
need a separate height-interpolation audit; closing internal shared-vertex
splits does not prove every coast/fill boundary watertight. These remain pending
an explicit request to resume, not claimed fixed. The new chart texture travels
through the ordinary hash-bound delivery receipt alongside its models/scripts.

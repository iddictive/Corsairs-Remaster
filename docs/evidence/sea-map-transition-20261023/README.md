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

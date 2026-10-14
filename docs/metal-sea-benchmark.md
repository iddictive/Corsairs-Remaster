# Native sea workload and profiling

`experiments/native-metal/sea_fleet_benchmark.cpp` links the same native engine,
registered entities and Metal backend as the player executable. Its isolated
`sea_fleet_fixture.c` constructs eight stock Manowar1 ships, native rigging,
animated sailors, sea foam, SEA2, weather/sky and ship lights. CORE runs the normal
Execute/Realize/service frame. This is a sea rendering workload, not the complete
game: encounter AI, player input, quests and battle effects are not exercised.

## Ownership and isolation

`tools/run_sea_benchmark.py` owns input extraction, launch admission, run manifests
and distributions. It reads the supported app's PROGRAM catalogue/day/light
definitions and RESOURCE assets, generates an isolated fixture and uses a hidden
SDL window. It neither executes `seadogs.c`, loads SAVE, opens/focuses a game
window nor changes player data. A live game holder defers measurement. Build,
GPU workload and installed-app staging have one serial integration owner.

The native CMake `sea-fleet-benchmark` target inherits the engine module force-load
and link contract. Before measurement the runner builds that incremental target
with two workers, verifies unchanged native inputs and records executable and
native-source hashes. Ninja dry runs cannot establish freshness here: the
CONFIGURE_DEPENDS glob check always schedules a CMake recheck. Stock input,
fixture and main hull hashes bind the run; the remaining assets are read from
the same app RESOURCE tree. This does not independently hash every texture/GM
dependency or make images bitwise deterministic: the normal asynchronous solar
readback and exposure remain active.

Each run lives under the ignored canonical
`experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/` owner. Editable
diagnostic inputs are tracked; generated scripts, traces, images and summaries
are reconstructible diagnostics. Unique comparison baselines and profiling
evidence remain there while the performance investigation is active.

## Measurement contract

The default backbuffer is 2560×1655, checked through the actual D3D surface;
VSync and frame cap are disabled only in the fixture. Graphics options come from
the normal settings adapter. Core simulation/visual delta is fixed at16ms;
native wall and GPU clocks remain real. Run384frames with128warm-up frames for
the matched short population. Report cold and warmed data separately.

`STORM_METAL_FRAME_TRACE` enables buffered renderer diagnostics. Ordinary clients
leave it disabled. Completed command buffers carry frame IDs, GPUStartTime,
GPUEndTime and completion status. The runner validates matching CPU/render/GPU
populations and merges overlapping command intervals before calculating GPU
execution per frame. CPU Core wall time, GPU execution and overlapping stage
costs are distinct quantities; none is independently displayed-game FPS.
Percentiles use nearest rank and median uses the middle pair.

Coverage requires actual sail draws, animated draws, main sea, current cloud
field and complete point-shadow cubes on every warmed frame. Presence counters
are insufficient visual proof: inspect the entire readback scene, including
water/reflections, hulls, rigging, crew, sky and material consistency. Missing
coverage fails the run. `performance_acceptance=false` is deliberate until the
requested whole workload and delivery acceptance are established.

## Native profiling routes

`--profile cpu` attaches `/usr/bin/sample` to the owned benchmark PID after cold
setup, with bounded duration and symbol-processing timeout. `--profile metal`
uses native xctrace; a trace that never finalizes or lacks its template is
rejected evidence, even if recording reported success. Profiling runs are
marked instrumented and are not used for matched timing acceptance.

`--capture` uses the existing MTLCaptureManager sea-frame admission after120
eligible frames. Native `gpudebug --oneshot` can replay/profile it without Xcode
or UI automation. Embed profiling data in the owned diagnostic trace if a later
session must inspect it. Directory navigation needs trailing `/`; inspect
`/performance/encoders/` and each encoder's info/counters. Use overlapping GPU
execution and do not add vertex/fragment/compute durations as serial frame time.
One-frame counter profiles identify expensive work; warmed uninstrumented runs
determine whether its repair improves the whole workload.

Apple contracts: [completed command GPU start](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpustarttime),
[GPU end](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpuendtime),
[GPU counter analysis](https://developer.apple.com/documentation/xcode/analyzing-apple-gpu-performance-using-counter-statistics).

## Proven local work and rejection rules

The foam carcass index topology is immutable between creation and destruction.
`TCarcass::Execute` now locks/unlocks its IB only on first index creation; VB
updates and draw geometry are unchanged. Actual old/new bodies match over
222024 CPU assertions with ASan/UBSan, including seeded warm frames, both
windings and destroy/recreate. The full workload falls from36 to20 allocations
per warmed frame with the same draw/pass counts. This is an allocation repair,
not proof of a large FPS multiplier.

Cloud density's detail noise only erodes its base. Zero height envelopes and
nonpositive base samples can return zero before unnecessary 3D texture reads.
The cloud resolution, march depth, decks, noise assets, cadence, lighting and
positive-density calculation remain composed. GPU before/after comparison must
cover clear, ordinary, overcast and storm fields, positive and empty samples.
The four-weather GPU comparison covers2097152 output bytes;88 differ by at
most one half-float ULP (upper/solar/alpha errors at most0.000488281, optical
distance0.0625). This is rounding equivalence, not byte identity. The complete
matched scene preserves all pass minima and shows no visible loss. Whole-scene
checks and matched frame distributions remain the acceptance owner.
The pre-existing dynamic_sky-probe star-opacity assertion fails with both the
unchanged baseline shader and the repaired shader. Keep that negative visible;
this work does not repair its separate fixture/star contract or claim a green
aggregate sky suite.

Reflection layers15/16/17 are SEA query lists. Registering them as ordinary
Realize layers draws reflection sky after main sea and erases the water;
reject such a fixture instead of repairing the game. SKY.dir requires its stock
child hierarchy and root hour. Root native declarations must precede library
directives: the legacy compiler processes queued includes later. Explicit trace
flush follows completed GPU readback because retained COM refs can postpone the
device destructor. These are fixture/tooling contracts, not gameplay defects.

Remaining acceptance: primary CPU/GPU repair and stutter reduction must improve
the complete equivalent workload past the requested budget without quality loss;
canonical staging must consume that batch in the original app. Installed player
FPS, camera/zoom/spyglass and scene transitions are not proven by this fixture.

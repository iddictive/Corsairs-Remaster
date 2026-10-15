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
The runner disables VSync and frame cap in its fixture; separately delivered
original-app pacing is recorded in `docs/runtime.md`. Graphics options come from
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

Stock `Ship_StartLoad` selects `ships/<name>/hull<variant>/` before native
Mount, and `Ship_EndLoad` clears the directory. The isolated fixture does not
load AIShip's handlers: it now selects the installed Manowar1 Hull1 directory
around Mount and clears it before sailors load. Earlier runs through
`20261014-160729` omitted this step. Main hull materials were untextured and
white; some other materials resolved unrelated root textures. Those runs prove
their recorded mechanisms/counters only, not complete material fidelity or
matching whole-scene performance. The former visual-fidelity verdict is withdrawn.
Corrected `20261014-165042` renders the authored yellow/dark hulls and retains
all eight-ship pass minima: CPU median13.983ms/p9916.617ms; GPU union
median15.599ms/p9919.779ms. Its material workload differs from the old baseline,
so the two timings are not a matched optimization comparison. The target remains
unmet. Native texture labels confirm the selected directory; sails, crew, sea,
sky and reflections remain present after the directory reset.

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

The fixed-function material specialization candidate uses Apple's function
constants for at most two active structural stages. `ffp_specialization.hpp`
owns its128-entry asynchronous cache, generic fallback and cancellation/drain;
`backend.mm` supplies the final Uniform and binds through the shared encoder
cache. Texture presence, alpha coverage, fog, colors, transforms, lighting and
fire values stay dynamic. Generic FFP fragments also require an explicitly empty
constant-values set when created from this library. Plain `newFunctionWithName`
returns an unusable unspecialized fragment for the pipeline descriptor.

`STORM_METAL_FFP_SPECIALIZATION=0|1` compares the same binary. The current fleet
manifest does not capture that variable; retain the exact invocation-to-run map
in the measurement receipt. Off includes the new generic loop/unrolling, so
Off/On measures the marginal specialization gain, not the full patch against old
source. See `docs/runtime.md` for accepted measurements. The native component
proof must retain the untouched shader oracle, undefined-constant fallback,
dynamic-value transitions and a seeded stale-key rejection; a faster inner
shader alone is insufficient.

The platform mechanism follows [Apple's function-specialization contract](https://developer.apple.com/documentation/metal/using-function-specialization-to-build-pipeline-variants)
and its [asynchronous material compilation guidance](https://developer.apple.com/videos/play/wwdc2023/10127/).

Apple contracts: [completed command GPU start](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpustarttime),
[GPU end](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpuendtime),
[GPU counter analysis](https://developer.apple.com/documentation/xcode/analyzing-apple-gpu-performance-using-counter-statistics).

## Proven local work and rejection rules

Prepared fixed-function stages and material bindings belong to `backend.mm`'s
existing Device/EncoderBindings lifetimes. Stage state/texture-transform changes,
reset and grass's temporary state writes invalidate stage entries. Effective
texture type/presence/stochastic flags are checked each draw; GPU handles and
samplers remain live. Encoder finish resets bindings, and external volumetric-sky
bindings invalidate them. Draw-specific matrices, viewport, fog, light and alpha
inference are never cached. One completion-owned256-aligned arena upload holds
the final1328-byte base or3008-byte fire uniform for both stages; allocation or
alignment failure retains direct bytes. `STORM_METAL_PREPARED_MATERIALS=0` is the
matched control; default is enabled. No shader or geometry arithmetic changes.

Matched002903/002948/003109 Off/On/Off reduces CPU preparation estimates by about
0.25ms, but Core wall means12.019/12.012/12.043ms show no throughput gain. The
receipt retains the switch missing from the runner manifest. Eleven existing
native GPU probes cover resource transitions, material/lighting/alpha, sea,
skinning, point/sun and MetalFX/HUD composition. A task-local ASan/UBSan driver
compares complete final uniforms and seeded dynamic/reset/fallback cases; its
sources/claim receipt remain under001520, without a raw child log or parent rerun.
The CPU profile's48/326 samples in preparation/binding were an inclusive ceiling,
not a predicted saving. Its remaining20 warm allocations are mostly dynamic
vertex uploads; FrameArena samples are bookkeeping, not proof of allocation.

Tile-local3D cloud light-factor reuse is rejected. The prototype retains view
density, march/decks/resolution and lighting queries as its untouched oracle,
then approximates only the seven-query light attenuation.32/64 height grids give
about2x/1.6x isolated warmed speed, but daytime RGB max errors about0.38/0.16
and mean about0.02 fail fidelity. Helper extraction also changes half-float
rounding, so bit identity is not claimed. Native whole-scene integration stops
at that gate; production shaders remain unchanged. Rejected sources, mosaics,
commands and timings belong to001520/cloud-light-rejected. The applicable reuse
reference is [Nubis Cubed's lighting acceleration](https://advances.realtimerendering.com/s2023/Nubis%20Cubed%20%28Advances%202023%29.pdf);
its applicability does not prove this particular approximation acceptable.

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
distance0.0625). This is rounding equivalence, not byte identity. The historical
scene preserves pass minima but its omitted hull material selection invalidates
the complete-scene fidelity claim. Whole-scene checks and matched frame
distributions remain the acceptance owner.
The pre-existing dynamic_sky-probe star-opacity assertion fails with both the
unchanged baseline shader and the repaired shader. Keep that negative visible;
this work does not repair its separate fixture/star contract or claim a green
aggregate sky suite.

Animated topology now belongs to `IndexedSpanFrameCache`, alongside the validated
addressing it derives from. The former256-entry four-way topology cache cannot
retain280 independent sailor sources. Actual old bodies rebuild186 lists on
every warmed cycle; the unified owner retains all280 through120 changed poses
with zero warm index reads. Revisions, source identities, LRU eviction, Reset
and teardown release both span and list. Only animated draws request topology;
the4096-index eligibility and4MiB payload budget remain, with full-index fallback.
ASan/UBSan covers16/32-bit indices, negative base, offset, invalid revisions,
recycled identities, oversized fallback and budget eviction. The existing span
probe remains valid. Draw order and all GPU passes remain.

Guarded conservative animated bounds now reuse immutable per-bone boxes in that
same entry and4MiB budget. The current palette and local X reflection transform
their union; convex finite weights and outward rounding contain the exact skin.
Invalid inputs, unavailable topology or more than64 potential receiver lamps
retain exact bounds. A cached conservative result is rejected when the lamp
guard closes. `STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS=0` restores the exact
path; default is enabled. The CPU proof covers8,467,278 coordinates across11
retained meshes plus revisions, addressing, eviction/reset and64/65-light
transitions. Expanded boxes can increase point-shadow survivors, so containment
alone is insufficient: existing GPU-skinned catalog receiver and point/sun
shadow fixtures must pass, and the complete fleet must retain its pass mix.

The matched Off/On/Off runs235049/235106/235140 reduce bounds preparation from
about1.76 to1.07ms/frame. GPU union medians11.326/11.291/11.264ms establish no
whole-frame FPS gain. Measured GPU waits grow by about0.6ms as CPU work shrinks;
CPU wall time minus measured GPU/drawable waits has medians10.764/10.156/10.937ms.
That subtraction is a CPU preparation estimate, not player FPS or a sum with GPU
execution. All coverage, draws, uploads and allocations remain composed.

After material specialization, native capture234159 ranks cloud tile30.59%,
main geometry13.43% and traversal receivers8.41%. Its4.904ms cloud compute
statistic is not serialized critical-path time. Bounded cloud loop-unrolling
and platform size-optimization experiments preserve16,777,216 compared bytes
but improve aggregate component timings by only about2%; they are not installed.

Separate-queue scheduling is also rejected. The prototype moved unseen tiles
only, ordered recycled writes and visible rotation through two MTLEvents, and
counted both queues. Matched000857/001001/001058 Off/On/Off main-completion
cadence means12.055/11.941/12.076ms establish only about1% throughput improvement;
Core median worsens. Added cross-queue lifecycle complexity is not justified.
The source and runner are restored; rejected inputs and the unrun pixel/lifecycle
fixture stay in capture234159's diagnostic owner. For future multi-queue work,
frame-tagged GPU interval union alone is not throughput: report completed main
cadence, warmed wall cost and both-queue coverage without adding overlapping
durations. Follow Apple's [cross-queue event ordering](https://developer.apple.com/documentation/metal/synchronizing-events-within-a-single-device);
tracked-resource ordering on one queue does not order independent queues.

The bounded Metal dispatch experiment uses the current native pipeline's
reported SIMD width32 and maximum1024 threads. Larger groups and128/256/512
occupancy hints preserve bytes but show no stable material gain over8×8 after
warming. Retain the existing dispatch. Native compiler occupancy hints are the
strongest applicable built-in facility; see Apple's
[Metal renderer tuning](https://developer.apple.com/videos/play/wwdc2023/10127/).
This rejected component variant does not establish a hardware FPS ceiling.

The subsequent native counter profile still ranks cloud tile first (27.80% cost,
4.077ms compute statistic versus the former7.171ms). Moving the weather lookup
inside density is rejected at the chosen numerical gate:319 of2097152 bytes
change, up to four half-float ULPs. That experimental source is not installed.

The accepted ray window uses the existing height-gradient support: below every
profile start and above the maximum possible profile end, density is exactly
zero. Macro noise is RGBA8Unorm; `cloudTypeVariation` owns both the density's
perturbation and the envelope bound. Gradient extrema and a conservative margin
define the interval. Outward ray radius increases monotonically; point positions
are computed directly from index, so skipped empty samples cannot alter later
positions, transmission or optical moments. Four complete weather fields compare
2097152 bytes with zero differences. This reuses the existing spherical deck and
gradient owners, adding no density hierarchy, texture assets or temporal state.
Whole eight-ship timing still exceeds the requested budget.

Population scaling retains the frozen eight-ship placement and centers smaller
populations. The former fixed offset put a single ship's crew outside the camera;
the native animated-coverage check correctly rejected it instead of reporting
empty-scene FPS. A valid one-ship control retains sails, crew, water/current sky
and point cubes. It is a causal scaling control, not a substitute for the fleet
target. Required stock-root absence also rejects before scene process creation.

## Corrected-material profiling and rejected component work

Native capture20261014-170328 binds the repaired Hull1 fixture. Its overlapping
profile ranks main Scene geometry23.54%, reflection geometry10.02%, traversal
receivers8.89%, then cloud tile8.33%. These costs are not serialized durations;
the cloud compute statistic4.865ms does not establish that many milliseconds
of critical-path frame time. Main scene reports Instruction Throughput
Limiter44.77% and FS Occupancy17.68%; traversal receivers report Clip Unit
Limiter79.34%. The earlier cloud-first ranking came from incomplete materials
and must not govern the complete-scene priority.

Two shader-only prototypes are rejected after paired native measurements across
eight actual2048-map tile offsets. Separate deck kernels, retaining RGBA32
intermediate optical moments, do not improve warmed cost; four256 fields also
change2634 of2097152 bytes, alpha error up to0.003173828. A static32-cube base-noise
envelope uses R maxima/GBA minima over periodic trilinear support to reject only
empty samples, retaining original higher-mip evaluation. It matches the four
fields exactly but adds about10–15% cost; one full-resolution tile differs in10
bytes. Neither prototype enters production source. This is the local fit limit
of [min/max empty-space bounds](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-39-volume-rendering-techniques),
not evidence that volume acceleration in general cannot work.

A separate actual LandShadow encoder comparison admits known current-pose
animated bounds to the existing solar cascade rejection.393216 GPU depth bytes
match across12 changing poses, both cascades and retained unknown bounds/poses.
Full native20261014-171431 keeps all pass minima but CPU median14.919ms and GPU
median16.622ms establish no whole-workload gain over corrected baseline165042.
The runs are separated in time: they cannot establish a causal slowdown either.
The production predicate is restored; the candidate/proof stays diagnostic.

Next bounded owner is backend material shader specialization. The current
`shadeFixedFunction` evaluates up to eight uniform-driven stages and general
argument/operation branches; applyState owns the shared pipeline cache across
raw, compact, bridge, fog, fire and aiming consumers. Apple's
[function specialization](https://developer.apple.com/videos/play/wwdc2023/10127/)
is the compatible built-in candidate. Before production changes, prove an
actual-shader pixel oracle and material-state/cache-key transitions; do not
duplicate the shader or accept a fixed common material that breaks other stages.

The corrected-material CPU sample181629 localizes the main-thread work to
rendering: ProcessRealize335/348 samples, ProcessExecute12/348 and Device::draw
213/348 inclusive. Reflection traversal71 and shadow resolve46 overlap those
parents; they are not additional percentages. The main branch has no sampled
GPU-wait symbol. Background semaphore/condition waits are not evidence that
the rendering thread is blocked. Its approximately1700 draws expose repeated
geometry traversal, state/resource binding, constant uploads and driver command
encoding. Existing frame/geometry/index caches have not removed that submission
work. Instrumented frame times cannot establish an optimization gain.

Keep the two limits separate: CPU submission/state preparation, GPU interpreted
material/light shading and repeated geometry/receiver rasterization. The
shader's general eight-stage combiner and up-to64-lamp fragment loop identify
specialization/admission candidates, not measured savings. Do not halve rendering
quality, remove reflections/crew/shadows or claim120FPS from an isolated pass.

Current-source qualification: backend raw and converted receiver paths already
filter authored lamps by sphere/AABB overlap before setting `lighting.flags.w`.
64 is a capacity, not proof of64 iterations for every fragment; the eight complete
cubes are a separate shadow-light count. Per-frame diagnostic rows181629 submit
median6869055 vertex/index references, not that many unique vertices or proven
GPU vertex invocations. Existing optimization counters time bounds preparation
at229–231ms per120-frame window (about1.9ms/frame), including current-pose CPU
skinning for bounds. Metal still owns rasterization; the CPU work is traversal,
geometry/wave/bounds preparation, resource/constant binding and command encoding.

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

# Native sea workload and profiling

`experiments/native-metal/sea_fleet_benchmark.cpp` links the same native engine,
registered entities and Metal backend as the player executable. Its isolated
`sea_fleet_fixture.c` constructs eight stock Manowar1 ships, native rigging,
animated sailors, sea foam, SEA2, weather/sky and ship lights. CORE runs the normal
Execute/Realize/service frame. This is a sea rendering workload, not the complete
game: encounter AI, player input, quests and battle effects are not exercised.

## Ownership and isolation

`SAIL::Execute` may dispatch ordinary disjoint cloth work through
`MetalCpuParallelFor`; free/falling cloth must retain ascending serial execution.
Its `goWave` callees consume the process-wide native `rand()` state. October17's
unconditional parallel dispatch lost RNG progression on the installed libc;
the repaired dispatch keeps free cloth in the original serial bounds loop.
Rope's disjoint numeric dispatch remains parallel. [Native RNG and extracted
scheduling evidence](../.local-archives/metal-renderer-evidence/gemini-cleanup-20261017/rigging/NOTES.txt)
includes before/after sanitizer runs and ordinary/free/deleted/empty cases;
the scheduling stand-in does not verify actual cloth appearance or game FPS.

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

### October15 exclusion matrix

The player's closed-app experiment uses one frozen linked engine baseline from
105609, with only the diagnostic driver and SEA exclusion owner rebuilt. Native
source writers in the separate gameplay tasks were not compiled or staged.
`001520/ablation/build-receipt.json` binds the exact executable, compile/link
commands and every linked input hash. No full-current-source fingerprint is
claimed for this frozen diagnostic. The14 sequential112549 cases each run384
frames/128warm-up at2560×1655 with native upscaling0 and the same fixture/settings.

| Excluded rendering | Completion cadence ms | GPU execution median ms | Preparation estimate median ms |
| --- | ---: | ---: | ---: |
| None, opening control |15.275|16.091|14.419|
| Dynamic shadows |12.047|7.610|11.895|
| Volumetric sky/atmosphere and sky drawing |13.312|10.436|13.157|
| Reflection refresh, both environment and sun road |12.183|12.294|11.335|
| Ship hull/model rendering |8.873|5.240|8.596|
| Sails |13.316|12.867|11.705|
| Sailors |12.433|11.984|11.441|
| Rigging/flags/vants |14.346|13.073|14.175|
| Cinematic/shafts/AA flags |16.882|14.460|16.229|
| Foam and ShipLights rendering |14.862|13.997|14.597|
| Shadows, sky and reflection refresh together |8.755|2.961|8.202|
| All listed render groups |8.332|1.432|1.112|
| None, closing control |15.782|14.660|14.938|

These are diagnostic exclusions, not FPS optimization acceptance. Costs are
coupled and cannot be added: excluding Ship rendering also removes the receiver
snapshots that seed all eight point cubes. Its8.873ms is not the cost of a hull
fragment shader alone. Excluding clouds also disables atmosphere-to-surface
lighting. The postfx toggle establishes no benefit; cinematic grading is already
interior-only. Background host work and control drift limit small differences.

The first no-water early return invalidates the comparison: animated draw count
rises311→498, registry admission exceeds its budget and all point cubes vanish.
Reject that18.006ms result. The corrected113132 driver replaces only water
fragment shading with a constant, keeping CPU water geometry, depth, uploads,
reflections and all152sail/311animated/1sea/1sky/8cube minima. Full/flat/full
completion means16.617/16.751/16.962ms; GPU medians16.528/16.133/16.588ms.
There is no material whole-frame saving. Whole-scene readbacks confirm the flat
water material and preserved hull/sail/rigging/sky composition, not motion parity.

The empty-render case spends6.834ms median in `nextDrawable`; the full controls
spend0.023/0.025ms. Its approximately120FPS completion cadence is a presentation
floor in that empty hidden-window experiment, not a CPU/GPU rendering ceiling
or6.8ms recoverable from the populated fleet. Normal CPU/GPU work overlaps.
All rows and per-run manifests/readbacks remain under001520/ablation and the
timestamped run directories; retained driver is in the private renderer-evidence
owner. The diagnostic-only native source is retained in `ablation-source` and
restored from the next gameplay delivery batch. Installed runtime, SAVE and
settings are unchanged.

The embedded012905 overlapping GPU profile has53 shader/pipeline rows. GPU
shader owners are unchanged fromc87e66d to current HEAD (the normal fragment
path also excludes the diagnostic-only flat shader). Reopening its existing
profile therefore requires no new capture or replay timing. Native `gpudebug`
`/performance/shaders/` groups repeated pipeline variants as follows: sky_clouds
33.905%, point_shadow_cutout+point_shadow_vertex14.954%, vs_landraw+vs_skinned+
fs_landlit_skyfog18.879%, local_weather_segments+integrate8.895%, raw/skinned
point receiver vertices+land_point_shadow8.700%, dynamic_sky_fs3.226%, and water/
sun-road/foam fragments3.952%. These are profiler cost estimates, not additive
critical-path milliseconds or warmed throughput.53 rows sum97.509%; unassigned
overhead is not silently attributed to a shader. The cloud shader reports1239
instructions,73temporary registers,32texture reads and zero spills. Full native
outputs and normalized rows are in001520/ablation/shader-costs*.json*.

The default backbuffer is 2560×1655, checked through the actual D3D surface;
The runner disables VSync and frame cap in its fixture; separately delivered
original-app pacing is recorded in `docs/runtime.md`. Graphics options come from
the normal settings adapter. Core simulation/visual delta is fixed at16ms;
native wall and GPU clocks remain real. Run384frames with128warm-up frames for
the matched short population. Report cold and warmed data separately.

For native-quality comparisons explicitly set `STORM_METAL_UPSCALING=0` and
verify the resulting manifest against the reference: the normal adapter otherwise
inherits the player's MetalFX selection.022134's unpinned Quality1 cannot be
compared with012905's native0 despite identical binary/stock/model hashes and
output resolution. Record changed power state separately; a4% Battery Power
readback accompanies that outlier but does not prove its cause.

Do not assume a hidden SDL window means the benchmark needs an App Nap wrapper.
The linked SDL2.32.10 disables the screensaver by default and its Cocoa backend
creates an IOKit PreventUserIdleDisplaySleep assertion. CPU-only runtime readback
confirms one level255 assertion during video initialization and zero before/after
SDL_Quit; Low Power Mode is false and thermal state nominal. This matches
[SDL's implementation](https://github.com/libsdl-org/SDL/blob/release-2.32.10/src/video/cocoa/SDL_cocoaevents.m)
and [Apple's App Nap conditions](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/power_efficiency_guidelines_osx/AppNap.html).
001520/power-policy-probe retains source and output. An extra activity token has
no demonstrated missing responsibility or performance benefit.

`STORM_METAL_FRAME_TRACE` enables buffered renderer diagnostics. Ordinary clients
leave it disabled. Completed command buffers carry frame IDs, GPUStartTime,
GPUEndTime and completion status. The runner validates matching CPU/render/GPU
populations and merges overlapping command intervals before calculating GPU
execution per frame. CPU Core wall time, GPU execution and overlapping stage
costs are distinct quantities; none is independently displayed-game FPS.
Percentiles use nearest rank and median uses the middle pair.
The runner also records warmed Core mean, the mean interval between first/last
main-frame GPU completions, and CPU wall minus explicit GPU/drawable waits as a
preparation estimate. That estimate can still contain implicit driver stalls.
GPU intervals may overlap across frames, so their median cannot establish
throughput. The manifest retains the actual STORM_METAL environment, including
experimental switches and generated trace paths, alongside normal settings.

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

## October15 AC control

After the player's AC-power response, native Off/Quality1/Off runs083658,
084245 and084358 bind the same full-quality fleet,384/128 frame population,
2560×1655 output and all pass minima. Main completion means are13.4541,
12.3552 and12.9805ms: about74.3/80.9/77.0 benchmark FPS. Native CPU preparation
medians remain12.0749/12.0739ms; Quality1 remains12.0673ms. All runs retain
20 allocations, about17.47MB uploads,1717 median draws,152 sails,311 animated
draws and eight complete point cubes. These uninstrumented results supersede
the critically low-battery timing context;120FPS remains unmet.

The corresponding AC precision probe rejects the previous small half-sum
candidate: summed medians25.4822ms versus25.0678ms FP32 across453 completed
commands. The palette-hash micro candidate saves about0.28ms CPU preparation
without improving whole-frame completion; it was restored to HEAD at the
player's request. Neither candidate is installed. The next structural shadow
cache proof and remaining production gates are owned by `docs/metal-frame-pacing.md`.

## Native profiling routes

### In-process diagnostic overhead is bounded separately

October15 frozen-linked full-fleet control133653 runs384 frames/128 warm-up
with `STORM_METAL_PROFILE`1/0/1 and otherwise identical generated inputs/settings.
Core means13.937/13.712/13.894ms bound the observed diagnostic cost near0.2ms;
enabled main-completion means14.020/13.969ms retain every expected pass minimum.
Disabled diagnostics intentionally produce no matching GPU/counter population,
so its Core wall time is the comparison, not invented GPU throughput. Final
images differ even between the two enabled controls; their hashes are retained,
not presented as pixel parity. This does not explain several missing frame
milliseconds or establish current installed-engine FPS. Earlier "uninstrumented"
timings mean no external sample/capture; in-process counters remain enabled.
The frozen ablation binary/input receipt is reused without rebuilding current
gameplay. Source/results belong to the private profile-cost-fit owner and run
outputs to133653-profile-cost-*; the source identity is the historical frozen
baseline, not the current installed gameplay batch.

### External sample and capture

On macOS27, `/usr/bin/metalperftrace listen --json --pid <pid> --output <file>`
reads already-collected Metal performance metrics without launching, capturing
or controlling a window. Its native help owns the process filter and output
contract; SIGINT closes the bounded listener and finalizes its JSON array.
Apple documents this [always-on metrics route](https://developer.apple.com/videos/play/wwdc2026/388/).
The October25,22:50 installed-engine observation is retained at
`experiments/native-metal/.cache/diagnostics/game-20261025-225010-89300.metal.ndjson`.
The raw stdout is a stream of pretty JSON objects despite the CLI's NDJSON
description: parse successive objects with a JSON decoder. Nine updates cover
477 presented frames over8.979732 active seconds:53.1196FPS, mean on-GPU5.8875ms,
mean Metal end-to-end CPU12.8827ms. CPU span is not CPU utilization or the complete
game tick. No state metadata identifies the scene/fleet; these observations do
not accept a120FPS target or establish performance of the later repair batch.

`--profile cpu` attaches `/usr/bin/sample` to the owned benchmark PID after cold
setup, with bounded duration and symbol-processing timeout. `--profile metal`
uses native xctrace; a trace that never finalizes or lacks its template is
rejected evidence, even if recording reported success. Profiling runs are
marked instrumented and are not used for matched timing acceptance.

`--capture` uses the existing MTLCaptureManager sea-frame admission after120
eligible frames. Native `gpudebug --oneshot` can replay/profile it without Xcode
or UI automation. Wait for `Replayer ready` before `profile run --embed`:
012905's early request reported success but returned empty encoder statistics.
Only its later nonempty `gpu-profile-ready.jsonl` is accepted. Embed profiling
data in the owned diagnostic trace if a later session must inspect it. Exit
interactive sessions with `exit --terminate`. Directory navigation needs trailing `/`; inspect
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

### October25 CPU index-cache invalidation

Dynamic vertex uploads formerly evicted validated index ranges and unique
topology even when the index buffer, draw bounds and vertex capacity were
unchanged. `IndexedSpanFrameCache` now omits only vertex content revision from
its address key; animated bone boxes still compare the actual vertex revision,
offset, stride and bone count. Index revision, identity, capacity, locked-buffer
bypass, LRU and topology-budget/reset behavior retain their existing owners.

The [raw same-consumer measurements and exact input recipe](evidence/metal-index-cache-20261025.json)
cover250 frames ×64 ranges ×4 passes. Dynamic validation scans fall16000→64;
validation-only time123.225→1.950ms, and validation/topology/bones/pose together
455.719→245.727ms in the final canonical-header run. Outputs/checksums match;
bone vertex reads remain65536000, so bounds are not stale. Immutable full-path
time33.643→34.722ms is within this single-pass cohort's unresolved noise.
The existing probe and disposable ASan/UBSan mutation/identity/capacity/bone/LRU
negatives pass. These CPU-component timings do not establish game frame time.

### Reflection aggregation fit

`sea/src/env_map.cpp::EnvMap_Render` visits five cube directions above water and
six below. Eight fixture ships therefore receive40 above-water reflection
realizations, each repeating model/node/material visibility traversal; this is
not40 complete GPU hull draws because each view culls separately. The warm
sun-road producer updates one direction per frame and explicitly excludes Ship,
Sail and Island. Fixture ShipLights handles that message. Sail and Flag list
membership does not imply drawing: their message handlers lack the reflection
case; Sailors belongs only to the main SEA_REALIZE layer. Do not multiply all311
animated fixture draws through reflection faces.

The player's whole-vessel aggregation idea maps most closely to reusable
geometry/material command templates plus Metal layered cubemap rendering.
Apple's [reflection sample](https://developer.apple.com/documentation/metal/rendering-reflections-with-fewer-render-passes)
and [texture-slice contract](https://developer.apple.com/documentation/metal/rendering-to-multiple-texture-slices-in-a-draw-command)
support per-instance face matrices and render-target array selection. This can
remove repeated traversal/encoding; vertices and fragments still execute for
every admitted direction. Different materials, skinning, alpha/cull/blend and
view-dependent fog/specular require preserved packet/per-view state. Main-camera
color cannot reconstruct surfaces hidden from that camera. A separately baked
vessel image must first pay its current geometry rendering and retain depth for
inter-vessel composition.

Next admission is an existing-reflection-stream inventory: draw count D, unique
compatible packet count C, face masks, and traversal/preparation time separate
from encoding. The observed800-draw refresh exclusion is an opportunity budget,
not proof that D/C is large or that aggregation reaches120FPS. Preserve the
current background/depth split and modern water's depth-aware reprojection;
then require per-face color/depth, camera/pose and material negative cases before
a whole-fleet comparison. No aggregation candidate is implemented or installed.

The stronger current candidate instances compatible rigid partitions across
ships within each unchanged view. Exact referenced vertex/index normalization
of the captured registry finds390 rigid draws in78 groups of five, each with
five distinct VB identities and WORLD matrices. Stride/UV/color/cull/primitive,
alpha reference/function/test and encoded alpha-resource token retain all78
groups. This is geometry repetition, not full color-material or FPS proof.
Source geometry loads allocate separate VB/IB owners; residency is keyed by
identity/revision, so equal authored models do not already share native meshes.

The follow-up actual33404-call API inventory and two bound uniform buffers
qualify770 main/reflection draws into154 groups (main390→78 and one reflection
view380→76), preserving PSO/depth/cull/textures/samplers and exact1328-byte FFP
material payload after excluding only MVP. Another364 reflection draws are
outside the receiver geometry lookup and remain unknown. WORLD/raw lighting,
catalogue selection and local fog are per-instance/per-view responsibilities;
captured catalogue counts vary0–9 and cannot be replaced by one shared lamp set.
Safe ordering/Equal-depth overlap, damaged geometry invalidation, mixed fleets
and whole-frame benefit remain unproved. Private CPU helpers belong to
`.local-archives/metal-renderer-evidence/ship-instance-fit`; hash-bound outputs
and native fetch/API receipts belong to001520/reflection-aggregation-fit.
Actual command order further restricts the proposal: among3125 native render
draw calls, no two adjacent qualified draws share a batch key. There are12
uninterrupted qualified-draw blocks; grouping inside those blocks has a ceiling
of312 eliminated draws, versus616 only with wider reordering. The390-draw main
block is the first bounded gather candidate. Reflection groups spanning other
draws require an additional ordering contract. These are call-count bounds,
not measured savings. The retained order receipt is
`command-instance-order-fit.json` beside the other capture outputs.
A separate64² offscreen fixture on the actual M3 Max now compares ten original
raw-hull draws with one instanced draw using derived current production shaders.
All four cases have zero changed color/depth bytes: separated instances with
catalogue counts0–9, depth overlap, Equal-depth original order and changed camera.
Synthetic alpha holes remain exact. Sharing the first instance's catalogue or
raw lighting changes380/870 separated pixels; reversing coincident instances
changes50 pixels, confirming that ordering and individual lighting matter.
Actual CPU ABI and complete shader-source assembly are hash-bound; a CPU C++
emitter evaluates the production macro-composed cloud source, and fragment
functions use the backend's empty function-constant specialization contract.
Source, provenance and readbacks belong to the private ship-instance-probe owner.
This proves synthetic shader payload equivalence, not captured hull/material
replay, safe reordered queues, damaged-geometry lifetime or whole-frame cadence.
There is no production instancing delta or FPS gain.

The follow-up screen-disjoint admission computes the actual captured main MVP
against referenced vertices and then the eight corners of each local AABB.
Stable greedy grouping moves a compatible later draw only across disjoint
expanded rectangles; every9009 reversed pair is independently checked. Both
models admit390→234 draws:78 batches of three and156 singles,156 calls removed.
That is5% of3125 captured native render draws, not the full earlier312 ceiling.
Touching/overlapping rectangles reject390 merge attempts; original instance
order remains. Eight-corner bounds retain all merges with26 full-viewport clip
fallbacks versus13 for exact referenced positions. Transform count falls from
737275 to3120; this counts operations, not time. Existing rawBoundsCache owns
the reusable local bounds, so a prototype must not rescan geometry per frame.

Results agree across2–32px margins at logical2560×1655,2px at256×166 and0.05NDC.
Actual native viewport fields are opaque in the API inventory, so pixel margins
remain conditional; runtime must use its actual raster viewport. The outward
binary32 dot envelope does not independently certify Metal fast-math or raster
coverage. Private rigid-order-admission source/result bind217 inputs and retain
the accepted/rejected moves. This supports a bounded gather prototype, not
production enablement or a120FPS promise; unchanged shader payload, actual
resource lifetime, ordered GPU parity and whole-frame cost still own acceptance.

### Reflection geometry budget and coarse-detail candidate

The player explicitly permits omitting fine geometry from water reflections;
main-view, physics and temporal capture contracts stay with `sea-reflections.md`.
Captured390 rigid main draws submit538665 triangles at2560x1655;380 matching
reflection draws submit538315 triangles at256x256. The reflection dimensions
are independently bound by `sea.cpp::kReflectionSize` and its target/depth
creation, as well as captured Uniform.viewport. In reflection522879 triangles
have area below1px squared and diameter below2px; main has261615. These are
small-primitive counts, not a safe removal fraction or a measured FPS saving.

Topology matters:78 distinct geometry groups contain23447 indexed connected
components, or5851 after exact-position grouping. Actual meshoptimizer v1.3,
pinned to9e1f07b159d3cb777f1c67ed31fc11fd117986f4, reduces the largest33847-triangle
cohort by68.7% at combined relative error.01. That cohort maps to Manowar1.gm
Part20/inzanegun1.tga. Independent sampled surface/attribute errors reject using
the QEM error as a pointwise main-view fidelity certificate. Conservative
reflection-only component tiers preserve retained triangles but save only23.5%
at1px across the three investigated cohorts, not97% of the reflected scene.

A private diagnostic cooks all78 captured groups with normals/UV/color-aware
simplification at relative error.01, retaining original vertex bytes. Distinct
group indices fall323199 to174936 (45.87%). A content-hashed backend replacement
selects these indices only for matching rigid raw geometry in the current sea
reflection cube; main/sun/point/crew/sail geometry remains original. It preserves
unknown/revised-source fallback, render uniforms and source index/base offsets.
The exact source/header closure and51 frozen link inputs build successfully as
binary d723dc5c669254dd98ca9b42dcc459d2a80736fde7d570dfe696808ef7880528.
Source, cooked data, receipt and full-fleet Off/On/Off runner belong to the
private reflection-lod-candidate owner. It is neither production code nor an
installed candidate; source-associated reduction still needs actual admission,
whole-scene native readback and full-workload cadence. The former player99471
wait is historical; the matched native comparison below ran after all player
holders exited, without launching or controlling the installed game.

The frozen d723dc5c binary now completes sequential Off/On/Off at2560x1655,
384 frames with128 warmup. Runs153350-reflection-lod-0-0 and
153632-reflection-lod-{1-1,2-0} on October15 retain identical coverage:
152 sails,334 animated draws, one sea/current sky and eight complete point cubes.
The candidate actually reduces repeated reflected index references by47.4%.
Main GPU completion cadence is14.902/13.959/15.652ms, or67.1/71.6/63.9FPS;
GPU execution-union medians are14.649/13.558/15.302ms. The bracketed FPS gain is
6.8–12.1% (9.4% against the mean control cadence), not a2x result. Control drift
precludes treating the midpoint as an exact speedup. CPU preparation medians
remain12.856/12.831/12.587ms; this geometry change does not remove that CPU floor.
Warm allocation counts remain20 and median uploads17.585MB; candidate maximum
GPU allocation is6MiB above either control. Full-scene readbacks retain the
visible fleet/water/sky, but the user reopens pre-existing daytime ship/sail
darkness. Motion, other weather, mixed assets and installed-player acceptance
remain open. The content-hashed78-group prototype is not a general delivered LOD
pipeline and is not installed. Result and comparison images stay with the
private reflection-lod-candidate owner.

The five-vector source investigation also bounds alternatives: main/reflection
batching must retain per-packet Raw/Sampling; retaining shared material bindings
limits original-order merging to50 of770 qualified draws. The unoptimized sparse point-ICB
candidate belongs to `metal-frame-pacing.md`; local-weather origin/window reuse
belongs to `metal-dynamic-sky.md`. No standalone gain is inferred from either
population.

### Ordered heterogeneous materials: native candidate, not delivered

The complete33404-call inventory qualifies770 rigid main/reflection draws into
12 consecutive runs when geometry and material payloads become per-packet:
main390→1, retained reflection380→11. Original triangle order, raster state and
global shadow/fog textures16–27 remain barriers. This removes at most758 of the
capture's3125 render draws; it is not a measured whole-frame improvement.
Raw1280 and SkyFog192 inline bytes are opaque in the capture and must be retained
from actual submission;95 distinct U and191 Sampling values in the main run
rule out sharing a final draw's material/light state.

The native Tier2 table is240 bytes per packet with five GPU addresses,
signed baseVertex and eight native IDs each for2D textures, samplers and cubes.
UInt32 indices encode high12 packet ID/low20 original source index; every one
of3230940 captured references round-trips without triangle reordering. Source
indices reach46397, below the20-bit limit. The combined index data is12.924MB;
immutable IB identity/revision/offset/count/type and original packet boundaries
own its cache key. CPU-only checks preserve prior valid data across revision,
order/count/identity changes, reject range/packing/capacity overflow, and hold
evicted immutable data safely. One thousand warm frames produce zero topology
rebuilds, about0.0043ms lookup/frame; cold construction takes1.98ms separately
from the5.85ms exhaustive oracle. These are CPU component measurements only.

An actual Metal shader probe verifies the240-byte native ABI and exact depth
on heterogeneous meshes/materials/lighting/fog. Color differs in144 of408
covered separated-fixture pixels, maximum11/255; the user judges the displayed
comparison visually indistinguishable. Color byte equality is diagnostic,
not the requested product criterion. Per-input negative controls detect
collapsed U/Raw/Sampling/fog/texture/sampler/cube state and changed packet IDs.
No tiny-fixture result accepts the complete sea scene or other weather.

A private full-backend adapter is compiled against51 frozen native link inputs
from the coherent4ee15fd/88b3ebe source. The unchanged benchmark driver's1241
recorded dependencies exclude all headers changed from its earlier source to
that archive batch. The candidate defers original per-packet API binds,
retains original registry packets and provides original-draw fallback,
completion-owned resources, shared-binding barriers and bounded immutable
index storage. Samplers explicitly support argument buffers; actual allocations
and metadata uploads are counted. Its same-binary Off/On/Off fleet runner is
prepared; whole-workload performance, visual acceptance and120FPS remain open.
Private owners are ordered-material-batch-fit, ordered-material-batch-probe and
ordered-material-batch-candidate; no production renderer change is delivered.

October15's corrected private adapter now includes rawWorld, the actual GM
main-view family; a rawSeaLit-only gate omitted those hulls. It builds against
87b4acb with a freshly compiled same-generation driver. Cumulative admission
reports use the existing120-frame profile gate because Device teardown does
not run in this diagnostic lifecycle. Actual admission includes390 main hull
draws/frame plus reflection draws, about1100 packets into20 batches/frame,
with zero original-draw fallback. This is real aggregation, not an FPS win.

The complete384/128 Off/On/Off runs164207 give cadence13.335/16.694/14.705ms.
A second bracket165150, with per-frame actual thread/process CPU clocks and
background-process sampling, gives13.857/17.896/14.074ms; GPU medians are
13.483/17.510/13.509ms. Coverage is identical:152 sails,334 animated draws,
one sea/sky and eight complete cubes. Candidate uploads grow from17.585MB to
19.482MB/frame. Reject this generic-shader adapter as a performance change;
fewer API calls alone do not establish lower cost. Generic shading and added
resource/table work remain candidate causes, not separately measured causes.
The actual-CPU versus wall-time distinction and contention limits belong to
metal-frame-pacing.md. No candidate from these runs is installed or accepted.

Captured012905 command lists now bound the reflection aggregation proposal:
environment face draws149/86/11/542/11 total799; sun-road adds1. Each environment
face has10 dynamic-sky draws and1 generic sky/fog draw, leaving138/75/0/531/0
raw-lit ship draws. Without merging different packets, layered execution needs
at least542 calls (the largest face), so removes at most257/799 calls; for raw
ship geometry the bound is213/744, not a fivefold reduction. This does not bound
CPU traversal savings or prove actual packet equivalence. Native command output
and `opportunity.json` belong to001520/reflection-aggregation-fit; no new GPU
workload or game control was used.

The separate proposal to gather packets before color, build maps and fuse sea
shadows into main color also needs a different preserving shader path. Existing
Location `shadeLand` applies shadow before material/fog; sea's receiver multiplies
the already fogged material output by a lighting-only ratio. A single-channel
analytical counterexample (lighting.8→.32, texture.5, fog transmission.5/color.2)
gives.156 post-receiver versus.18 direct. This is an expression incompatibility,
not a Metal pixel result or proof of desired appearance. The retained
`direct-shadow-expression-fit.json` binds the source hash. Coincident Equal-depth
packets, receiver/main alpha differences, solar sampling and current late-map
availability remain separate gates; do not substitute the Location shader as
an already equivalent implementation.

The pending mixed-precision cloud candidate changes only normalized shape/detail
fBm sums to half intermediates and half literals, then returns to float before
the existing remaps. Large planet/world positions, domain warp, gradient,
coverage, pow and repeated optical/light accumulators remain float. It reuses
Apple's [16-bit intermediate guidance](https://developer.apple.com/videos/play/tech-talks/111373/)
without reducing cloud resolution, rays or cadence. Host/source expansion and
GPU compilation pass.453 completed commands cover eight production-domain tiles
and four weathers; all optical/displayed outputs are finite, maximum RGB error
0.003106. Paired partial-field images show no obvious candidate difference.
Speed is unresolved: within-case timing varies up to9.76x, and2.74% gain from
summed case medians does not establish a repeatable benefit. Three of four
weather sums worsen. Keep it uninstalled pending stable value and full consumers.
001520/cloud-half-pending retains the two-context derivation, frozen complete
libraries, reused eight-tile/four-weather A/B driver and parent receipt. The
historical one-ULP fixture exit rule is not the user's no-visible-loss contract:
report finite moment/displayed-radiance errors and inspect paired output, then
the actual full consumers if component timing earns that next step. Incomplete
neighbour tiles are explicitly insufficient for full-field/motion acceptance.

Actual-main Boolean query rejection is not currently safe for the traversal
receiver. Temporary probe011928/012710 observes about240 zero results among701
receivers per frame, but every packet retains alpha test. Native fixtures prove
that alpha discard or original Less can produce zero while the later Equal
receiver still affects coincident geometry. All eligible counts remain zero;
unknown pipeline/nonmonotonic/clear/depth-replacement epochs retain the original
draw. Encoder finish and unrelated depth targets preserve their epochs. The
probe never changes drawing and its complete Off/On color/depth, completion,
fallback and reset/destruction cases pass. Apple's
[visibility result contract](https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/visibilityresultbuffer)
does not waive those receiver-specific conditions. `meaningfulAlpha()==false`
also cannot prove opacity: that existing classifier returns false for an entirely
transparent texture as well as an opaque one.

Whole-packet clip-frustum rejection is separately disproven as a gain for this
scene. The conservative eight-corner homogeneous classifier validates finite
local bounds, current posed bounds/palette, matrix and index metadata, inflates
rounding boundaries and retains W-crossing/unknown cases. Native clip planes,
near/side intersections, invalid inputs, boundary cases and375 independently
sampled CPU boxes pass.012710's240 warm completions contain164640 known/3600
unknown packets and zero off-frustum packets;57855 zero-query packets still have
intersecting bounds. Do not infer that Clip/Cull counter pressure implies whole
draws can be culled. Both prototypes are restored; source/fixture/evidence belong
to001520/receiver-visibility-rejected.

Fresh current-source GPU captures012905/012947 use the same binary,2560x1655,
eight ships and unchanged full-quality settings, with conservative animated
bounds On/Off. The48 cube-face encoders submit11287218/10755964 vertices
(On+4.94%); the receiver remains2560437 vertices/853479 primitives in both.
Its zero-area counter is33.64/33.66%, while backface rejection is about50.25%.
These are input/opportunity measurements, not a throughput win or proof that
zero-area triangles are removable from source topology.
Cloud tile remains the largest individual cost statistic,33.90/26.33%; its
4.8961/4.5550ms compute statistic is not serial critical-path time. One captured
frame cannot establish the bounds change's FPS cost. Retained receipt:
001520/current-bounds-gpu-profile.json; production/app are unchanged.

Static topology fit015539 rejects a receiver index-filter module: all390
qualified static receiver packets/538665 triangles contain zero repeated-index
or distinct-index bit-identical finite-position candidates. Its701-packet
inventory matches the capture, but311 animated packets/314814 triangles remain
unknown. Main geometry has2752 repeated triangles in eight draws of one VB/IB,
absent from reflection/receiver; this is not the receiver's33.64% zero-area
population. The actual-class ASan/UBSan fixture passes addressing, all-index
validation before position access, revisions, provenance and one-frame lifetime.
No GPU command changes; instrumented fit timings are not FPS acceptance. The
probe is removed; artifacts belong to001520/degenerate-static-rejected.

Animated topology fit020528 closes that remaining population using the actual
immutable Shared44-byte bridge upload, its current valid palette and source/
flight provenance. All311 animated packets/314814 triangles qualify: zero
repeated indices or distinct-index bit-identical XYZ+weight+packedBones pairs.
Lists/strips, signed base/offset, full addressing before vertex reads, changed
revisions, distinct influences and nonfinite/bone/palette/capacity rejection
pass the actual-class ASan/UBSan fixture. Together the probes cover all853479
receiver triangles and reject this topology filter. No pose-only coincidence,
collinearity or subpixel claim follows. Probe source is removed; inputs and
receipt belong to001520/animated-degenerate-rejected. Tighter conservative-bound
fit now uses actual captured sources/palettes and eight cube matrices fetched
from012905 through native gpudebug, under001520/captured-bounds-fit; its admission
oracle remains unresolved. Native `Vertices` describes submitted vertices but
does not define an ICB correction; the CLI has no working ICB info handler.
CPU admission totals11367981 refs versus profiler11287218:45 faces differ by
-701 and three have additional differences. Do not normalize those counters or
attribute the531254 On/Off capture difference to bounds.

The conditional same-pose fit nevertheless rejects a pair-bound prototype.
Current/exact/pair CPU admission totals11367981/11346681/11364249 refs. Pair
removes only3732 refs (0.0328%, one draw on one face), while bone-box transforms
rise4434→10308 distinct blended endpoints (2.325x). Rebuilt current boxes give
the same48 CPU admission totals as the fetched current boxes; maximum world
coordinate delta is7.65e-6. Pair median volume/exact improves1.456→1.230 and
contains this captured pose, but general float/FMA/padding/overflow containment
is unproven. These CPU results are conditional opportunity evidence, not GPU
or FPS acceptance; no new production module or GPU comparison follows.

The traversal receiver ICB prototype is rejected as a standalone optimization.
Fit run004432 admits701 receivers/311 skinned with one contiguous cull/viewport
run and557 geometry resources. The prototype retains CPU light admission and
original order, uses bounded completion-owned uploads/ICBs, and derives a texture
argument wrapper from the unchanged canonical fragment body. Apple's
[ICB fragment argument contract](https://developer.apple.com/documentation/metal/mtlindirectrendercommand/setfragmentbuffer(_:offset:at:))
requires an argument buffer for textures: parent standalone texture bindings
cannot create the supporting PSO. Native MSL `set_vertex_buffer(pointer,stride,index)`
does not accept a byte offset; add it to the pointer and use the two-argument
setter. The nonzero-offset fixture exposes this mismatch before acceptance.

Final direct/ICB native color+depth bytes match across raw/skinned geometry,
strips/lists,16/32-bit indices, offsets/base vertex, mixed cull/all-six viewport
fields, sun/points/fog, then seeded no-light/capacity/allocation/lease/reset and
destruction cases.005914/010009/010059 Off/On/Off Core means12.196/12.530/12.038ms
and completion means12.258/12.586/12.108ms reject the module despite lower CPU
preparation estimates. Source is restored; candidate/fixture/evidence remain in
001520/receiver-icb-rejected, not production. The proposed GPU visibility branch
is superseded by the actual-main query/frustum fits above: its safely eligible
population is zero. Less alone and alpha-discard zero results cannot reject the
later Equal receiver, and no whole packet is outside the frustum. Do not reopen
that module without a new independently proved admission population.

Read-only adjacent finding: the existing caster registry kernel also supplies
vertexOffset as the three-argument setter's stride. Its nonzero-offset runtime
effect is unverified and it was not changed by the rejected receiver experiment.

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

Private storage for immutable cloud-noise textures is also rejected. Copying all
shape/detail/weather mip levels to private textures preserves16,777,216 compared
bytes, but192 paired warmed samples give shared/private medians0.440/0.543ms and
sums126.471/205.947ms. The production shared textures already permit
[GPU-optimized contents by default](https://developer.apple.com/documentation/metal/mtltexturedescriptor/allowgpuoptimizedcontents).
Private storage is not an assumed speed guarantee; this exact-shader native probe
does not justify a staging/upload change. Inputs/results stay under001520's
cloud-storage-rejected diagnostics owner.

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

### October15 image-derived frame interpolation

VideoToolbox low-latency temporal interpolation produces valid GPU-generated
frames on this M3Max/macOS27.0.1. The private module loads its session off the
render thread and retains CoreVideo surfaces/session through command completion.
It converts only the new native2560×1655 input to NV12 1280×828, retains the previous
input in the ordered three-flight ring, and reconstructs generated BGRA separately.
Real/native scene color and the ordinary presentation path remain unchanged.
Disabled, pending-setup cancellation, unsupported dimensions, history gap/reset
and reset during an in-flight command pass the standalone caller. Its60 warmed
samples measure10.389ms median GPU generation and0.044ms median CPU encoding.

Both full-fleet variants use frozen native87b4acb archives and the same CPU-timed
driver,384frames/128warm-up, native2560×1655 and Off/On/Off controls. Fifty linked
inputs remain hash-identical before/after; each warmed population retains152sail
draws,334animated draws,1sea,1current sky and8complete shadow cubes.

| Generated images per native frame | Off/On/Off completion cadence ms | Native frames/s during On | Free-presentation output ceiling |
| --- | --- | --- | --- |
| One midpoint |14.188 /23.892 /13.132|41.85|83.71/s|
| Three phases, .25/.5/.75 |13.724 /39.557 /13.121|25.28|101.12/s|

These ceilings are arithmetic upper bounds, not displayed FPS: generated images
are encoded before HUD but deliberately never presented. Actual output pacing,
HUD integration, motion smoothness, end-to-end input latency and player replay
are unproven. Main-thread CPU medians remain about12ms; generation adds GPU work
rather than removing that preparation. Three-phase GPU median/p99/max is
40.140/51.278/67.112ms. Raw per-process CPU samples and clocks remain with each run.
Each On run generates383pairs with one seed and zero fallback. The three-phase
run proves all six NV12 planes fully replace their sentinels and all three phase
images have distinct hashes. Whole-frame observation retains the complete visible
fleet/sky/water composition; generated rigging and hull detail are visibly softer.
Stills do not accept temporal artifacts or HUD behavior.

SDK sizing matters: temporal-only mode uses spatialScaleFactor1 and explicit
macOS27 maximum-dimension/pixel queries. The earlier spatial2 request exceeded
its supported input size and produced a uniform green image despite successful
session/command completion; reject its timing. Optical-flow/full-RGB conversion
attempts contain cold samples only, not warmed performance verdicts. Constructor
argument2 permits three dyadic phases; the installed configuration getter reports
zero for both arguments1 and2, so an echo-equals2 guard was invalid. Its failed
attempt is retained separately; actual phase writes are the runtime oracle.

Disposition: reject both measured NV12/VideoToolbox variants for integration.
They fail the requested large FPS gain while reducing native update cadence and
softening generated images. No backend, settings or installed-app change is
delivered. This does not reject unmeasured motion-vector-based MetalFX or other
frame-generation implementations. Editable prototypes, API probes, binary/input
receipts and results remain under private `frame-interpolation-candidate` and
`frame-interpolation-multi-candidate` in the renderer-evidence owner. Raw full-fleet
runs are `20261015-170122-fi-cost-*` and `20261015-172134-fi-multi-cost-*`.
The pre-lighting canonical renderer archive and its two source owners are
retained under frozen-87-renderer/manifest.json for coherent reconstruction;
later canonical archive replacement is not the measured87 input generation.

## October15 — shadow command candidates rejected

Ordered static index ranges preserve original vertices, indices, materials,
triangle order and whole-draw culling. At128 triangles per range, all48 actual
material depth faces match the unsplit reference bit-for-bit in the same frame.
Nevertheless, serial136-frame runs at2560x1655,4xMSAA+FXAA and no upscaling give
whole-GPU medians18.716ms unsplit,26.805ms at128 and21.734ms at512. The128 case
reduces rigid index references to26% of baseline in the captured CPU fit, but
increases live commands and ICB storage. Fewer references do not prove cheaper
execution. Reject both variants; no canonical renderer change is delivered.
Private source, input identities, exact depth and timing receipts belong to
`point-cluster-fit` under the renderer-evidence owner.

The existing Apple ICB optimizer is also measured before attempting custom
compaction. It optimizes each exact subsequently executed face range and resets
the full completion-owned ICB before count-dependent boundaries can move.
All48 same-frame actual depth faces again match. The same-binary Off/On/Off
whole-GPU medians are19.995/21.787/20.341ms; optimization plus reset costs more
than skipping sparse commands. Reject integration. An empty-command count alone
does not justify a custom compactor. `sparse-shadow-whole-frame-fit` retains the
source, binary, four terminal runs and213 bound inputs. Earlier source-only
`sparse-shadow-command-fit` is superseded by this measured rejection.

## October15 — compute stages and current CPU sample

The existing stage observer now also uses the installed SDK's serial compute
pass sample attachments. Whole-GPU Off/On/Off medians21.114/23.038/21.244ms expose
about1.9ms observer overhead in this cohort; these absolute times are not a new
renderer regression or accepted FPS result. Warm compute stage medians are
cloud tile1.939ms, local-weather segments1.342ms, weather integration.968ms,
AO.427ms and all point-ICB encoding.146ms. Atmosphere/solar/fog are each below
.1ms. These stage intervals may overlap other GPU work and cannot be added to
the render-stage totals as critical-path time.

A separate1000-frame run disables the pass observer and samples the actual
main-thread stack after warmup. Indexed draw preparation/submission and repeated
reflection traversal remain substantial; no single sampled function establishes
a2x saving. This is a representative native rendering workload, not a player
scene or isolated-machine CPU ceiling. `gpu-all-pass-counters` retains the
preflight, bracketed136-frame runs,1000-frame sample, binary and213 input hashes.

## October15 — layered point-cube passes

The preserving native alternative consolidates each cube's six render passes
into one layered pass, retaining every original command/cull/material/pose and
the original texture layout. The eight-lamp scene falls48 to8 render passes.
Both private same-frame actual-material oracles compare all48 depth faces
bit-for-bit. The actual production header additionally passes32 reused-slot,
count/order/reset and seeded layer-clear cases against independent b942a25
source:192 faces equal. That fixture's checker alpha proves structural coverage;
the full-fleet oracle separately supplies actual-material evidence.

At2560x1655, native4xMSAA+FXAA and no upscaling, serial384/128 Off/On/Off runs
without GPU pass counters give whole-GPU medians20.804/17.806/20.461ms and mean
completion cadences19.135/16.470/18.885ms. Every arm retains152 sails,334 animated
draws, one sea/sky and eight complete cubes. Instrumented runs independently
show point vertex/fragment stage sums3.557/2.448 to1.766/1.469ms; those overlapping
stage sums are not additional whole-frame savings. The uninstrumented comparison
removes the concern that fewer counter samples alone explain the improvement.

Sources, retained generation1, production-compatible generation2, binary/input
hashes,32-case canonical-header oracle and all run CSVs belong to private
`point-layered-pass-fit`. Source integration is limited to the existing registry.
Canonical staging and fresh integrated HUD/fleet384 Native/Performance runs pass;
`runtime.md` owns their delivery receipt. The integrated runs retain minima152
sails,334 animated draws, one sea/current sky and eight complete cubes. Their
warm GPU median/p95 values are16.387/19.756ms Native and12.356/16.882ms Performance;
these are acceptance observations, not another paired speedup measurement.
Both use shadow tier1 (sun2048, points512); the current player record selects
tier2 (sun4096, points512), so their absolute times do not predict that setting.
Native warm RSS ends596.7MB, Performance1349.3MB; these short runs do not establish
long-run allocation stability. Exact inputs, distributions and frames belong to
`layered-installed-acceptance` under the same private renderer-evidence owner.
The measured cadence is about61FPS in this cohort, not the requested120FPS.

The proposed main/reflection ICB path with original PSOs is separately rejected
as a broad implementation: retained1134 raw-lit draws require1047 consecutive
executions, maximum run length2. All1043 inherited-state splits are texture slot0.
Native ICB cannot remove those texture changes on the macOS15 contract; the
earlier1100-to20 generic-material batches are a different, slower shader path.
Persistent per-draw command reuse has no multi-frame identity/address evidence.

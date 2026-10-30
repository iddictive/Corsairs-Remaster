# Reactive ship flight, 2026-10-29

## Current installed successor

Engine `f0a0d3f910fee9582ab1dc4a7d09726d8a6647c8c861b0bc46e7576c0cec8529`
is installed with a verified bundle signature; the preceding `170fa6c1` engine
is retained by the installer for recovery. The source reuses native sailing
inertia and queued impulses during wet contact, trims nozzle forces around the
retained centre of mass, damps angular rate at the current attitude, and clears
transient neighbours when control returns to sailing. Gravity, tilt and momentum
remain physical; there is no automatic levelling or height target.

The real SEA/SAIL/SHIP lifecycle baseline and candidate are
`baseline-material-ready-sailing-transition-output.txt` and
`native-drive-trim-sailing-transition-output.txt`. The candidate preserves wet
forward motion and returns control to ordinary sailing after release. The frozen
`sailing-transition-runner.py.txt` and companion C++ files replay that consumer;
optional `JET_PRIVATE_SAVE` projects an immutable private input without loading
gameplay. Its generated attributes and regional layout remain outside Git.

The latest private saved ship is Manowar1, not the earlier representative Brig1.
Its actual installed-resource, 27-region ground baseline retains 42,898 collision
parts and measures 40.90ms mean native Execute time, 42.39ms during ground contact
(`manowar-installed-27-ground-baseline-sailing-transition-output.txt`). This is a
contact/sliding fixture, not proof of quiet resting contact or whole-game FPS.
**Ground-contact performance remains unresolved.** The older 7.60/7.73ms Brig1
results below do not close this report. Bootstrap and rehydrated trials with
invalid lifecycle setup are excluded from acceptance.

`flight-camera-replay.py.txt` retains the native before/after framing check:
the camera uses the actual ship altitude as its orbit reference while retaining
sea collision clearance. At 100m the mast top moves from NDC 1.345 to 0.692;
ordinary sailing, manual orbit and zoom remain unchanged. Translation checks
cover 0–2000m with maximum NDC difference 1.15e-6.

The environment successor recentres the existing sky/fog/sun frame smoothly at
altitude and handles the active reflected view independently of the cached main
camera. Native Metal readbacks remove black top pixels at 3010m and above,
preserve six ordinary main/reflection frames exactly, and restore the incoming
view and cached camera. `flight-environment-result.json`, the before/after raw
outputs and the frozen replay/native sources retain these component checks.

The plume now draws through the existing late ship-effects owner after the sea.
A typed world-colour bridge temporarily selects the existing compact draw path
and restores the sea's shader/declaration bindings. The real native water replay
previously rejected the late draw (`late_admitted=0`); it now admits it and changes
the water pixel while preserving submerged and foreground occlusion. See
`exhaust-water-order-{before,after}.json` and its replay. This covers modern-water
mode 1, not every water material or the player's final appearance/FPS judgement.

## Reopened installed-runtime acceptance

The installed experimental batch reproduced the user's severe jet/falling FPS
regression. A three-second native process sample records 1,528 of 1,801 main
thread samples in `AdvancePhonkJet -> ReactiveWorld::Step`: 901 in contact
collision work and 627 in continuous collision casts. Raw process evidence is
private in application diagnostics `performance-20261029/installed-fps-sample.txt`.
This confirms the known contact limitation affects gameplay; successful component
physics checks did not establish usable flight performance. The earlier Brig1 native
consumer measures7.60ms intact and7.73ms after a real canonical material cut.
`certified-cover-60hz-native-consumer-output.txt` and its damaged counterpart
retain controls, codec, prediction, death and terrain-pose checks. Retained-time
recovery measures96/48/16ms input at31.61/15.66/5.59ms CPU; no time is discarded.
Installed-game FPS remains the user's check.

The existing additive plume also clips to pure white on a bright water substrate.
`exhaust-contrast-replay.py.txt` and `exhaust-contrast-result.json` retain native
Metal compact-primitive readbacks on bright water, dark water and grey backgrounds.
The source-over, denser purple plume retains colour on all three; the luminous
water halo keeps its additive state. Depth occlusion and zero-alpha tail preserve
the background. Vertex count, topology, depth policy and shader remain unchanged;
one extra draw is possible only while both the halo and plume are present. This
is component contrast evidence, with appearance in gameplay still user-owned.

Consumer: the main native `SHIP` in local-sea Phonk mode. Ctrl applies four
body-aligned nozzle forces; W/S and A/D distribute them to pitch and roll.
Force uses the authored full-load engine rating. Actual retained hull/cargo mass,
shape inertia, gravity and attitude determine acceleration. There is no altitude
target, pose interpolation, automatic levelling or flight trajectory.

The first installed package used all 44,206 Brig1 yaw-prism contact parts.
The repaired candidate retains that original shape for mass, inertia and water
integrals, while a derived collision cover uses 16,407 parts. Connected subsets
are admitted only when every vertex is within a 2cm Euclidean offset of one
original convex prism. Convexity bounds the entire new hull by that same offset;
float construction has reserved headroom. Cut support cells retain their original
parts, and material identity/revision invalidates the derived cover. No damage
consumer is moved to proxy subshape IDs. The independent final actual-Jolt-vertex
oracle admits all16,407 hulls and measures maximum anchor distance19.9871182mm;
its complete result is `final-certified-vertices-collision-projection-output.txt`.
Original-volume tilted-water and cargo-scaled mass/inertia pairs agree exactly.

Native Nevis contact falls from 84.11ms to 7.600ms per 16ms input frame with the
certified cover and maximum 1/60s Jolt steps. Full CCD and all elapsed time remain;
forces, gravity, COM, mass, inertia and original-shape buoyancy retain their owners.
The integration tradeoff is measured: lift height differs by 5.4cm after0.96s,
and sustained pitch/roll differs by9.57cm after3s between30/120Hz frame cadences
(velocity difference below0.001m/s). These are trajectory differences, separate
from the2cm collision-cover bound. First hull preparation costs373ms; this remains
a cold-admission stutter. Whole-game FPS and appearance remain user-owned.

Earlier stress outputs stop at an invalid late momentum assertion: model-origin
velocity is not COM velocity for an asymmetric rotating body. The corrected
`projection-physics-contract.cpp.txt` uses true COM momentum; the existing
translation-only case now exits0 with `PROJECTION_CONTRACT_PASS` and momentum
10000 exactly. Its result is
`solver60-translation-corrected-projection-physics-contract-output.txt`.
It establishes300m/s translation CCD without crossing (11.07mm worst overlap,
2.59mm final overlap) and complete elapsed-time gravity. Rotating impacts show temporary vertex overlap
at both60Hz (0.443m) and240Hz (0.861m), with the body staying on the entry side.
This is the pinned linear-CCD rotational limitation, not a claimed cure for all
high-speed rotating contacts. Rejected persistent-body, coarse760-part and
240Hz-certified candidates remain labelled by their raw evidence filenames.
The native codec fixture now keeps its model and fake blots owner distinct,
matching production Mount/Load ownership. The earlier aliased fixture destroyed
its model and fell back to44k parts; that result is not a production fast-path
performance observation.

`ship_reactive_jet.cpp` owns native input/state/geometry projection and exhaust
delivery. `ReactiveWorld` in the existing `roundhouse_ragdoll.cpp` owns Jolt 5.6
integration, retained convex-part collision, point impulses and buoyancy. Both
physics consumers retain the same existing registration lifetime. The shared
`TriangleSink` preserves native collision polygons and nested callback cleanup.
Jolt's [force API](https://jrouwe.github.io/JoltPhysics/class_body.html) and
[architecture](https://github.com/jrouwe/JoltPhysics/blob/v5.6.0/Docs/Architecture.md)
are the reused facility; the previous Phonk `Flight` helper supplied a cinematic
ballistic/attitude path and could not satisfy this force-controlled feature.

## Native state and contacts

`SHIP_STATE` remains the authoritative pose and linear/angular momentum. Native
XZ speed uses knots; the adapter converts to world metres/second and back. Jolt
centre-of-mass velocity is converted explicitly to the native model origin.
Prediction exports no neighbor impulse, heat, grounding change or saved-state
mutation. Ordinary TOUCH excludes the free body so contacts are not solved twice.
Nearby ordinary ships retain their native XZ/yaw degrees of freedom; this does
not replace the whole fleet with unrestricted 3D rigid bodies.

Terrain comes from `ISLAND_BASE::GetWorldSurfaces()` authored poses and current
`MODEL::Clip` polygons. Geometry identity/generation/collision revision, node
transforms, authored pose and spatial coverage invalidate the derived mesh.
The render pose is restored after extraction. Water uses Jolt buoyancy/drag over
the sampled sea plane, calibrated to native loaded draft; draft and shape changes
invalidate the submerged-volume query. This is a local sea-plane approximation,
not a fluid dynamics simulation. Land support can remain physical after mode-off;
settled upright water contact hands control back without snapping the pose.

Ctrl release, mode-off and a restored in-flight save keep passive motion. The
existing binary layout is unchanged; character attribute
`Ship.ReactiveFlight.Version=1/Active=1` selects continuation. Normal save ordering
is source-bound: `COMPILER::SaveState` executes `OnSave` before SCodec/variable
serialization; canonical `OnSave -> Sea_Save -> SEA_AI::Save -> AIShip::Save ->
SHIP::Save` writes the attribute first. The isolated codec replay tests native
`SHIP::Save/Load`; it is not a full-world gameplay save replay.

## Reproduction and evidence ceiling

For a new jet/falling performance report, use this diagnosis path before
changing the collider or solver:

1. Confirm the played executable and sample its live process without game UI
   automation. Separate render work, `ReactiveWorld::Step`, shape preparation and
   dry versus contact cost; a fast dry frame does not establish contact cost.
2. When compound collision/CCD dominates, inspect leaf count and the pinned
   [compound cast loop](https://github.com/jrouwe/JoltPhysics/blob/v5.6.0/Jolt/Physics/Collision/Shape/CompoundShape.cpp)
   and [inner-radius calculation](https://github.com/jrouwe/JoltPhysics/blob/v5.6.0/Jolt/Physics/Collision/Shape/CompoundShape.h).
   The cast visits child shapes, and the smallest child sets the compound inner
   radius. Fine material partitioning can therefore dominate CCD despite a small
   body count. Compare the retained persistent-body counterfactual before
   attributing that cost to body recreation; caching remains valid when its own
   measured work dominates.
3. Reuse `ship_reactive_collision.hpp` for both free and constrained ship bodies.
   Keep original mass/COM/inertia and water integrals independent of the derived
   collider. Require a bound against the complete retained union, actual Jolt
   vertices and cut gaps: matching plane supports alone admitted the rejected
   coarse cover. Treat solver cadence as a separate measured accuracy/cost
   choice, retaining all elapsed time and reporting trajectory differences.
4. Replay the same native consumer with intact and damaged material, slow-frame
   recovery, prediction, codec and original-volume water checks. Compare full
   step costs and retained output, then deliver through the normal installer.
   Samples dominated by rendering or ordinary non-jet sailing select their own
   owners; they do not justify changing this collision representation.

Use the already configured native build, staged assets and matching SDK/compiler;
build `engine` after header changes so every virtual consumer has the same ABI.
These archived one-off probes add no permanent test target. Each Python runner
takes an existing empty task directory as argument and creates only isolated
fixture files there. It never loads player SAVE, runs `seadogs.c`, raises a window
or changes live settings. The native renderer is initialized only in a hidden
component fixture; no game interaction or visual observation is performed.

```sh
JET_CADENCE_REPORT=1 python3 docs/evidence/phonk-reactive-flight-20261029/production-component-runner.py.txt "$workdir"
python3 docs/evidence/phonk-reactive-flight-20261029/native-consumer-runner.py.txt "$workdir"
```

`JET_CADENCE_REPORT=1` reports the measured60Hz trajectory difference instead of
the historical sub-millimetre frame-invariance assertion; all other component
assertions remain active. Reproduce the corrected independent translation case
with `JET_COMPONENT_SOURCE=projection-physics-contract.cpp.txt` and
`JET_TRANSLATION_ONLY=1` on the same production-component runner. Its default
rotating-wall stress retains the stricter50mm gate and can fail as documented;
it must not be presented as an accepted rotating-contact guarantee.

Use a separate empty directory for each run. The optional second native-runner
argument is a scene-layout JSON with `asset_inputs` rows (`path`, `offset:[x,z]`)
for the 27 resident terrain roots, including Nevis. That layout remains in its
private diagnostic data owner; it is neither copied into Git nor emitted by the
runner. Native MODELR assets and their authored layout are reused without loading
gameplay. The representative arrangement measures the same `SHIP::Move` projection
path; it does not establish whole-game FPS or the complete fort/city surface set.

`production-component-output.txt` contains independent F/m, gravity, ballistic,
frame-cadence, floor/ceiling/side, water, reaction-momentum, point-inertia and
ragdoll/jet lifetime results. `native-consumer-output.txt` contains actual native
controls, ordinary-state/prediction negatives, mode-off, native codec continuation,
real island contact and authored-pose invalidation. The 27-resident counterpart is
`native-consumer-27-output.txt`. Timings include their named consumer path only;
GPU frame time and the player's jet appearance remain unverified.

The actual Brig1 native replay passes Ctrl/WASD, COM ballistic continuation,
prediction, save/load, death and real-terrain contact. Geometric COM must use
the chosen prism integrals, not the underlying true-solid `piece.volume`.
Immutable shape-mass caching and a transformed all-dry bound reduce its dry
native frame from 7,286 to 28.2 microseconds with identical control output.
First hull admission takes 161 ms. Same-process RSS rises by 163.8 MB for
authored hull/material and another 55.4 MB for the physics projection.
That original 240Hz package measured 84.1 ms/frame over the Nevis replay;
the simple floor/ceiling probe averages 17.6/23.6 ms and water 2.81 ms.
These are component CPU costs, not game FPS. The 27-resident replay remains
unaccepted; its retained older failure must not be read as final acceptance.

`hull-normalization*` retains the successful all-prism affine support/ray and
mass/inertia checks. `native-hull-before-cache-output.txt` is the original
real-hull cost observation. `true-hull-source*` preserves the unfinished true
convex-solid/coalescing experiment: 138.33 m³ rather than the conservative
548.26 m³ prism volume, with five then two ill-conditioned builder failures.
The latest experimental source adds independent support/ray measurement and
uses Jolt's supported default hull tolerance; it has not been run or promoted.
`capacity-fixture-rejected*` records an unsuccessful overflow hypothesis:
9,000 overlapping parts were reduced successfully by Jolt, so that fixture
does not prove capacity-failure cleanup. Production handles returned error
bits unconditionally and publishes no partial body/neighbor state; transient
neighbors are released on every exit. The normal component replay passes.

Terrain cache identity includes actual `CLIP_ENABLE`/`CLIP_ENABLE_TREE` bits.
The lighting owner's native phase fixture proves day/night changes those bits,
while palette-only changes retain them. The final hash change is source-bound;
no additional full scene replay was run for this small integration delta.

The initial 2 MiB allocator was rejected: the configured solver requested a
3,932,160-byte block even with one active body. Its raw output is retained in
`production-component-rejected-allocator.txt`; flight now uses the existing native
8 MiB budget with Jolt's malloc fallback. The first collision oracle incorrectly
treated body-aligned hover thrust as world-vertical during contact-induced tilt.
`collision-jolt-reference*` isolates body recreation, and the corrected gravity-only
case measures a closed horizontal momentum system without modifying the solver.

# Reactive ship flight, 2026-10-29

Consumer: the main native `SHIP` in local-sea Phonk mode. Ctrl applies four
body-aligned nozzle forces; W/S and A/D distribute them to pitch and roll.
Force uses the authored full-load engine rating. Actual retained hull/cargo mass,
shape inertia, gravity and attitude determine acceleration. There is no altitude
target, pose interpolation, automatic levelling or flight trajectory.

The immediate package retains the native yaw-prism contact projection, with
all 44,206 Brig1 parts normalized from doubles into Jolt scaled convex shapes.
The actual retained-solid optimization remains unfinished and is not wired to
the game. Ground-contact performance remains an experimental limitation.

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

Use the already configured native build, staged assets and matching SDK/compiler;
build `engine` after header changes so every virtual consumer has the same ABI.
These archived one-off probes add no permanent test target. Each Python runner
takes an existing empty task directory as argument and creates only isolated
fixture files there. It never loads player SAVE, runs `seadogs.c`, raises a window
or changes live settings. The native renderer is initialized only in a hidden
component fixture; no game interaction or visual observation is performed.

```sh
python3 docs/evidence/phonk-reactive-flight-20261029/production-component-runner.py.txt "$workdir"
python3 docs/evidence/phonk-reactive-flight-20261029/native-consumer-runner.py.txt "$workdir"
```

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
Ground contact remains expensive: 84.1 ms/frame over the original Nevis replay;
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

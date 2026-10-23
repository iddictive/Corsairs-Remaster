# Dart salvo: moving muzzle and stern-camera visibility

Source/component evidence for the October 23 repair. The
[shared build and delivery receipt](../scene-color-grade-20261023/build.json)
retains the source hashes and verified installed signed-engine identity.
Real stern-camera appearance and full-frame performance remain unverified.
This replay does not launch or control the game.

## Reproduction and result

Baseline is Git commit `8e635ad50f95ead29c5212ab5c4ad4790b3e30f6`.
`replay.py` extracts the actual `AICannon::Fire`, `RealFire` and
`CalcHeightFireAngle` methods from that commit and the candidate checkout,
then compiles each against small event/pose stubs and the real `CVECTOR`.
It records the emitted shot direction/elevation. An independent analytical
trajectory checks the accepted world endpoint.

The 48 cases per source cover four queue modes, delays 0.05/0.5/3.5/10 seconds,
own transverse speed 8m/s plus 0.3m/s heave, and three precomputed target
intercepts for target speeds −6/0/+6m/s. Maximum point-target error falls from
80.1028595m to 0.000140201373m. Each set of 12 ordinary, explicit manual-sky,
and loaded captured-vector controls emits identical yaw/elevation/raw angle.
The loaded-vector control clears the transient point flag as current `Load`
does; this focused replay does not decode a real saved cannon stream.
The existing `Save`, `Load` and `AIBalls::Execute` bodies remain byte-identical;
there is no codec change or steering after launch.

The point already contains the existing controller's target lead and per-gun
queue delay: `AIShipCannonController::Fire` predicts the target through flight
and firing delay, and `Fire2Position` adjusts each gun's target for its actual
delay. The repair re-solves that accepted point from the muzzle at firing.
It preserves the controller's current intercept approximation; these CPU cases
do not prove interception of arbitrary target manoeuvres. Explicit sky launch
angles retain their captured vector. Loaded queues retain the existing saved
vector fallback because the accepted point is transient. The point is captured
regardless of the optional impact-camera toggle, including water receivers.

The same replay extracts the actual `AIBalls::Realize` body into a renderer-state
stub. With a previously inherited world translation (400,0,250), the baseline
applies that transform again to world-space tracer vertices: 471.699066m error.
The candidate draws them with identity world and restores the caller's world:
both errors are zero. This reproduces the state contract failure; it does not
claim the player's previous frame had that particular transform.

At 1285px viewport height and 60° vertical field, the baseline sprite's outer
support is 1.780548/0.890274/0.445137/0.211970px at 500/1000/2000/4200m.
The candidate keeps it at 8px and preserves the larger near sprite. The actual
256×256 A8R8G8B8 atlas has 14.044% alpha≥128 coverage: at 8px outer support its
opaque area is 8.9883px² and central shaft width about 0.9151px. It remains a
thin dart. A strictly axial 7m tracer geometrically projects to a point; the
sprite supplies visible end-on support.
The actual authored 8×8 mip has nine pixels with alpha≥128 and maximum alpha255;
the 2×2/1×1 mips used by a subpixel dart have zero such pixels and maxima86/52.
This is atlas/CPU projection evidence, not a native raster capture.
The existing three line segments,
two mixed-ammunition sprite draws and ordinary 0.35m control radius are retained.
No extra draw, vertex, particle, allocation or physical collision size is added.

## Exact replay

Set `TASK_SCRATCH` to an existing, empty, caller-owned disposable directory.
Run from the repository root, one compiler at a time:

```sh
CODEX_SCRATCH_DIR="${TASK_SCRATCH:?Set TASK_SCRATCH first}" python3 docs/evidence/dart-salvo-20261023/replay.py
CODEX_SCRATCH_DIR="${TASK_SCRATCH:?Set TASK_SCRATCH first}" python3 docs/evidence/dart-salvo-20261023/replay.py --syntax
```

The script writes generated sources, binaries and the compiler module cache
inside `CODEX_SCRATCH_DIR`; the caller owns their cleanup. It uses Python's
standard library, the repository source and `/usr/bin/c++`. There are no retained
binary inputs.
The `--syntax` mode reads the current Ninja sea_ai compile arguments, runs actual
translation-unit syntax checks without shared cache/object writes, and compiles
the existing `cannon_aim_geometry_probe.cpp` in scratch: both source checks pass,
and the existing probe reports 758 checks, zero failures. `syntax.txt` records
the complete argv with portable `<repo>`/`<scratch>` path substitutions.

Environment: Xcode27.1 build27A9275, Apple clang21.0.0
`clang-2100.3.34.2`, arm64, macOS27.0.1 build26A434. Existing native compiler
selection is `/usr/bin/c++`, C++20, deployment floor macOS15.0. No game or GPU
workload or full-frame timing was taken.

`results.txt` contains all 96 emitted-shot rows, baseline/candidate render rows,
source/atlas SHA-256 values and control results. `replay.py` is retained solely
as reproduction evidence; no permanent test-suite or shipping entrypoint changes.
The canonical Git baseline and source/atlas paths resolve all inputs without
chat history or task-temporary files. Native stern appearance, camera occlusion,
and unchanged delivered frame time remain unverified.

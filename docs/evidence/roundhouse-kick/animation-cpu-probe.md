# Native animation CPU baseline

Baseline captured on Apple M3 Max, Apple clang 21.0.0, Release `-O3`, arm64,
macOS deployment target 15.0. Exact compile argv, input/object/source SHA-256,
OS, Git revision and timestamp are in [baseline metadata](animation-cpu-before-metadata.json).
The baseline animation sources were unchanged from the recorded Git revision.

The [probe](native-animation-cpu-probe.cpp) links the existing native animation
object files directly. It loads installed `man.an` and `man2.an` by the same
public `AnimationInfo`/`Bone` calls and ordering as `AnimationServiceImp::LoadAN`.
It uses walk frames 2–18, confirmed in both installed descriptors, with either
one playing action or two phase-offset blended copies of that action.
No engine, window, SDL runtime, game process, installation or shared build runs.
Abort-only core methods and service teardown satisfy linkage and are never
called. Instances live until process exit; retained allocation is bounded.

Each configuration warms up once and measures seven rounds of 4,000 steps
at a simulated 16 ms delta. Timed work includes native `AnimationImp::Execute`,
its actual `BuildAnimationMatrices`, and reading all 16 floats of every
resulting bone matrix into an observable checksum. Loading, action setup,
hashing and isolation checks are outside the timing region.

| Rig | Playing actions | 1 instance, µs/step | 32 instances, µs/instance/step |
|---|---:|---:|---:|
| man: 123 bones, 8,698 frames | 1 | 4.875896 | 5.015168 |
| man | 2 | 7.872698 | 8.164617 |
| man2: 79 bones, 4,085 frames | 1 | 3.309635 | 3.290103 |
| man2 | 2 | 5.288552 | 5.351999 |

All eight configurations produced finite palettes. All four configurations
with 32 instances preserved the first instance's palette while advancing the
last instance, and their phase-separated palettes differed. The
[raw baseline](animation-cpu-before.txt) retains all samples, deterministic
120-step full-palette sequence hashes and timed checksums. Unchanged animation
behavior should preserve those hashes/checksums on this compiler/architecture.

The [after run](animation-cpu-after.txt) preserved all eight full-palette
sequence hashes, all 56 timed checksums, finite output and instance isolation.
Median CPU changes ranged from -3.42% to +1.42%; the 32-instance cases ranged
from -3.42% to +0.54%. These samples show no material idle-animation regression
under this probe; they do not measure active ragdoll or scene frame time.
[After metadata](animation-cpu-after-metadata.json) records the rebuilt inputs.
Portable metadata substitutes `${REPO}`, `${SCRATCH}` and
`${INSTALLED_ANIMATION}` for local paths.

Reproduce after the native animation objects have been rebuilt:

```sh
probe_output=$(mktemp -d /tmp/corsairs-roundhouse-animation-after.XXXXXX)
python3 docs/evidence/roundhouse-kick/run-native-animation-cpu-probe.py "$probe_output" \
  '/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/animation'
```

The [driver](run-native-animation-cpu-probe.py) derives compiler flags/includes
from the existing Ninja build, compiles only the probe and abort-only core
link support into the requested task-owned output, and leaves the shared build
unchanged. Compare every quality hash and timed checksum before interpreting
timing. Preserve material after results here, then clean task-owned temporary
directories with `codex-delete-temp`.

This is CPU animation evidence, not scene FPS or GPU skinning evidence. It
does not execute script events, engine scheduling, concurrent animation,
GPU upload/rendering, player interaction or the new procedural action itself.
Steady-clock samples include normal OS scheduling noise; the raw rounds expose
that noise instead of implying a hard real-time bound.

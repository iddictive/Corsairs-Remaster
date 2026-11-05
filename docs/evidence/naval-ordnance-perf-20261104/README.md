# Airburst workload comparison

## 2026-11-05 fleet and airburst pilots

At source `6b674af7cef8642aba7f3cf902bf11a93cea5be8`, the native
`sea-fleet-benchmark` was built from the current source and run while the game
was closed on Apple M5 Max. A pilot with eight ships, 160 frames, 1280×800 and
32 warmup frames produced CPU median 8.792 ms and GPU command-union median
5.478 ms. It is ordinary fleet rendering, not a firing scenario. Its
`manifest.json`, `summary.json`, `cpu-frames.csv`, and `renderer-frames.csv`
retain input hashes, settings, and observations. Reproduce with
`python3 tools/run_sea_benchmark.py --frames 160 --warmup 32 --width 1280 --height 800 --ships 8`.

The same binary then ran the existing airburst `fixture.c`, 160 frames at
640×480, using `engine.ini`, `day.c`, `ship-lights.c` and `ship-type.c` from
`docs/evidence/cannon-darts-20261021/native-fixture/`, and the installed
RESOURCE tree. It used `STORM_METAL_PROFILE=1`, `STORM_METAL_MODERN_EFFECTS=1`,
`STORM_QUIET_CLIENT=1`, and a fresh absolute `STORM_METAL_FRAME_TRACE` path.
The fixed fixture launches four shells per frame for the first 110 frames and
completed with seven ships. Frames 32–109 have CPU mean/median 8.312/8.446 ms;
frames 110–159 have 8.310/8.218 ms. Raw CPU and renderer rows are
`airburst-20261105-cpu.csv` and `airburst-20261105-renderer.csv`. Binary SHA-256
was `13d02b629619af3ddc1720cb1c78f03586b66ca3500723f3b509edb7ea0a6440`;
the fixture SHA-256 was `b84361d7af0c363a9eabea94d4ad83a288f6182a5193e41581c0cd698f71cf3e`.
This synthetic fixture stubs script damage and part of the explosion effects;
it does not represent a held 60-salvo burst or establish real-game FPS. These
are current observations, not a paired comparison with the earlier runs below.

The retained fixture runs the native 640×480 `sea-fleet-benchmark` for 160
frames, with eight mounted ships and four airburst shells spawned per frame for
the first 110 frames. At least one real airburst must occur or the fixture
removes all ships; both runs reported seven ships at the final acknowledgement.
The shell and cosmetic fragment trajectories, particle systems and native
collision paths execute. Script blast damage is stubbed, and the explosion
handler counts events instead of creating the four gameplay explosion cones.
This is therefore neither full combat nor a test of every damage result.

Baseline source: `fb2d7c7a9bfaaaf189e894acd5738370c397c320`. Candidate:
the source changes in the commit containing this evidence. Both use the same
fixture, assets and configured native build, Apple clang 21, arm64 Metal, 16 ms
simulation steps and the installed game's RESOURCE tree. The game was closed.
Each row is one measurement; CPU wall time includes the whole benchmark frame,
not just projectile processing. The one baseline/candidate pair is not a
statistical guarantee.

| Frames | Baseline mean / median | Candidate mean / median |
| --- | --- | --- |
| 32–109, shooting | 19.58 / 20.24 ms | 17.09 / 17.25 ms |
| 110–159, settling | 16.13 / 15.76 ms | 15.11 / 14.92 ms |

`cpu-baseline.csv`, `cpu-candidate.csv`, `baseline-log.txt` and
`candidate-log.txt` are raw observations.
To replay, copy `fixture.c` and `engine.ini`, `day.c`, `ship-lights.c`,
`ship-type.c` from `docs/evidence/cannon-darts-20261021/native-fixture/` into a
temporary directory, symlink its `RESOURCE` to the prepared runtime resources,
build `sea-fleet-benchmark`, and run it with the directory, `160 640 480` and
`STORM_METAL_PROFILE=1 STORM_METAL_MODERN_EFFECTS=1 STORM_QUIET_CLIENT=1`.
Set `STORM_METAL_FRAME_TRACE` to a new absolute temporary filename for each run.
Compare identical source revisions and resource inputs; game FPS and the
60-salvo-per-second extreme remain unmeasured.

## Nuclear launch probe

The separate `nuclear-probe.cpp.txt` uses the same native scene and the real
`SHIP::ExecutePhonkOrdnance` with a task-owned fake fire control. After scene
setup it calls the weapon owner 160 times at 16 ms with a fixed valid manual
target: 16 missiles were simultaneously in flight and eight impacts occurred.
Its fixture stubs combat callbacks; it does not prove real-game aim, damage,
frame rate or the entire ten-second ramp. The normal-mode negative held for
2.56 seconds without launching and
launched one missile on release. The logs retain both results. Separately, a
source-body scheduler probe produced 311 launches (initial plus 310 repeats)
in ten seconds at both 30 and 60 updates per second, with target and effects
stubbed.

For either native probe, copy `nuclear-probe.cpp.txt` as `probe.cpp`, use
`nuclear-fixture.c` as `fixture.c`, and copy the four supporting fixture files
listed above. For the normal negative, change its fixture's
`InstantCannonReload` from 1 to 0 and insert `fake.fire=false;
ship->ExecutePhonkOrdnance(16);` after the 160-iteration direct loop before the
result is printed. Reuse the documented
`docs/evidence/particle-tracer-capacity-20261029/build-probe.py.txt` compile and
link recipe, adding the ship source include directory and
`-DSTORM_METAL_SHIP_FIRE=1` to match the ship library's ABI. The ABI flag is
required: the generic benchmark target lacks it. Use the same resource symlink
and launch variables as above. The normal control is synthetic; no game save or
player input is used.

The separate [nuclear wreck lifecycle fixture](../nuclear-wreck-20261104/README.md)
checks native wreck persistence and fire-state restoration.

## Island region type-check proxy

The 2026-11-05 installed-process sample `game-20261105-083534-9434.sample.txt`
in the local diagnostics cache contained 137 of 1193 main-thread samples under
`AIBalls::Execute` → `ISLAND::Cannon_Trace`; 53 were under collision tracing
and 42 directly in `__dynamic_cast`. These are stack sample counts, not elapsed
time or a full-path before/after measurement. The sample is an ignored local
diagnostic and may not survive cache cleanup.

`island-rtti-probe.cpp.txt` compares the old cast with a candidate exact-type
fast path followed by the original dynamic-cast fallback. It models the class
inheritance and checks two region instances, a base island, another derived
island, an unrelated entity and null. All six returned identical pointers. It
also times 9.6 million casts on 64 region pointers for five rounds. On Apple M5
Max, macOS 27.2, Apple clang 21.0.0, SDK 27.2, using `-O2 -std=c++20`, old
times were 86.476, 91.946, 92.397, 90.747, 91.336 ms; candidate times were
7.787, 7.614, 7.723, 7.812, 7.658 ms. To reproduce, compile the retained text
as C++ and run the resulting executable, for example `xcrun clang++ -x c++
-O2 -std=c++20 island-rtti-probe.cpp.txt -o /tmp/island-rtti-probe`.

This proxy does not instantiate the real entity manager, island models,
geometry, live cuts, projectile events or game frame. It cannot establish game
performance, collision parity or lifecycle safety by itself. The real manager
constructs an ID from a millisecond timestamp and slot index; in a separate
source-derived probe two IDs for the same slot in one millisecond matched, so
a lookup of a replacement of another type could succeed. The fallback remains
necessary. Full same-consumer timing and collision/event parity remain pending.

## Projectile attribute write proxy

The [attribute write proxy](ball-attributes-proxy.md) records the sampled
hot path, retained source, reproduction command, raw timings and limits. It
does not measure game FPS or establish combat event parity.

# Airburst workload comparison

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

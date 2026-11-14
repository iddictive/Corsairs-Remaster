# Mid-air volume follow-up: rejected

The standalone candidate sampled the separate mid-air RG8 volume, accelerated
its opening, added a brief initiation flash, and replaced background refraction
with a depth-tested soft compression shell. Its shader remains an experiment:
the upright mapping produced a tall flame jet (`rejected-upright.png`), while
compressing that mapping produced a flat horizontal patch (`rejected-air.png`,
`rejected-land.png`). Neither passes the requested visual quality. The actual
renderer and backend are unchanged; only the delivery manifest was extended.

`baseline.csv` and `candidate.csv` are raw GPU samples from the existing
standalone Metal fixture at 2056×1290, input radius 42, camera (0,60,180), and
one or eight overlapping airborne effects. The renderer applies a 2.5× visual
radius; the background is synthetic and there is no ship or battle. One warmup
precedes three recorded samples. Calculate medians from the CSV: the fixture's
printed “median” is its minimum. For eight effects at 0.30, 0.55 and 1.40 s,
the baseline medians were 4.297, 5.187 and 9.273 ms, versus 2.498, 2.484 and
3.601 ms for the rejected candidate. These timings do not establish acceptable
quality or in-game performance.

`rejected-renderer.hpp.txt` and `benchmark.mm.txt` retain the candidate and
fixture. The fixture loads both canonical volume files; to replay, copy the
candidate into a temporary directory as `cluster_impact_render.hpp`, copy the
fixture there as `benchmark.mm`, then compile it from the repository root
with Apple clang 21, C++20, `-O2 -fobjc-arc -framework Metal -framework AppKit
-I experiments/native-metal` and run with an output directory and `single 6`.
The baseline instead uses `baseline-benchmark.mm.txt` and the original
`renderer-snapshot.hpp.txt` from the thermobaric evidence folder. This does
not build or run the game.

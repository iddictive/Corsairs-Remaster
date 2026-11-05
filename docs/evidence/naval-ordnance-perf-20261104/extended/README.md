# Sustained airburst and nuclear-visual follow-up

The prior 160-frame fixture stopped before the reported delayed slowdown. For
this follow-up the same 640×480 native scene runs for 400 frames, firing four
shells per frame through frame 350. The real projectile and particle owners
run; script blast damage and explosion cones remain stubbed as described in the
parent README. Both runs acknowledge seven ships. Source baseline is
`dc306fca59a5c3e977a4d9af84d0762305ac8e51`; the candidate is this commit's
projectile and particle changes. The game was closed. The two CSV files and
logs are the raw single-run observations.

| Frames | Baseline mean / p95 | Candidate mean / p95 |
| --- | --- | --- |
| 32–99 | 18.00 / 21.46 ms | 8.83 / 9.13 ms |
| 100–199 | 22.64 / 25.42 ms | 8.93 / 9.37 ms |
| 200–299 | 23.26 / 28.09 ms | 8.95 / 9.33 ms |
| 300–349 | 21.88 / 23.83 ms | 8.98 / 9.30 ms |

The candidate at this stage limits simultaneous shrapnel tracer systems at the same origin;
projectile sprites, simulated fragment contacts and actual shell damage are
not removed by that tracer limit. The pre-existing limit on duplicate visual
airbursts remains. A longer unpaired 650-frame candidate pilot with eight
shells per frame through frame 600 is retained as `long-fixture.c`,
`long-cpu.csv` and `long-log.txt`. Its 100-frame mean ranged from 10.2 to
11.5 ms after warmup, with p95 up to 19.05 ms and maximum resident memory about
3.6 GiB. This is not a 40-gun, 60-salvo-per-second battle, and it cannot prove
full-game FPS or absence of stutters.

A second paired 650-frame workload fires 20 airburst shells per frame for 600
frames (about 1,250 shells per second at the fixture's fixed 16 ms simulation
step). Its baseline already includes the tracer limit and lazy interpolation;
its candidate additionally limits same-origin shrapnel-shell smoke systems.
Both runs acknowledge seven ships. The fixture, raw CSVs and logs are prefixed
`dense-`. During frames 100–199, mean / p95 CPU wall time fell from 22.93 /
26.51 ms to 12.66 / 13.19 ms; during frames 500–599 it fell from 19.85 /
21.18 ms to 11.23 / 11.68 ms. Frames 300–399 in the candidate still reached
26.99 ms, and observed resident memory rose to roughly 3.5 GiB, so neither
stutter-free operation nor a memory ceiling is proven. The workload has no real
script damage callbacks and is below the requested 40-gun, 60-salvo-per-second
endpoint; use the same command as the long fixture with `dense-fixture.c`.

The baseline fixture derives from the parent's `fixture.c` by replacing
`BenchFrames <= 110` with `BenchFrames <= 350` and `BenchFrames==160` with
`BenchFrames==400`. Run the existing native benchmark with the same setup and
environment in the parent README, changing the frame count to 400. Use 650 for
the retained long fixture. Sampled main-thread stacks in the baseline included
`ParticleSystem::Execute`, `BaseEmitter::BornParticles` and
`BillBoardProcessor::AddParticle`; that sample does not establish their share of
full-game CPU or GPU time.

## Nuclear rendering

The GPU input derives from the retained `nuclear-shockwave-20261029/replay.mm.txt`
using 640×480, one camera, age four seconds and eight repetitions (discard two).
With sixteen fully rendered blasts at the same location, median GPU time was
11.64 ms. Rendering all sixteen with four full and twelve lighter plumes and
pressure hazes measured 4.13 ms; 256 total with four full measured 26.69 ms.
The single paired samples are not a full-frame benchmark. Older effects use
eight rather than 64 volume samples, omit local shadow taps and use pressure
haze without scene refraction; they remain active for their existing lifetime.
Four full blasts produced identical image hashes before and after the change
(`4c60305efb79c67030f96500f9cb0bb9ea31a260721863caeba27c29a6137ad4`).

`nuclear-distinct.mm.txt` is the selected real-Metal probe: a fifth, older blast
is at a different location from four newer ones. `all5.png` includes it and
`omit-old.png` uses `OMIT_OLD=1`; 69,149 RGB pixels differ by more than two
levels, proving this particular older effect contributes to the rendered frame.
The current engine was also linked into the parent's synthetic nuclear launch
probe; `nuclear-integration-log.txt` records 16 concurrent missiles and 37
impacts in that fixture. It does not prove gameplay damage or visuals.
The test does not prove gameplay visuals, all impact positions, maximum nuclear
cadence, or full-resolution FPS. The lighter representation is an explicit
quality tradeoff, not an identical rendering.

Compile the probe with the command from the nuclear-shockwave README, using
`-DFIXTURE_WIDTH=640 -DFIXTURE_HEIGHT=480`; invoke it with a temporary output
directory, a prefix, `1 0 5 8 650`, and repeat with `OMIT_OLD=1` for the
negative. For timing the fully overlapping case, replace the center expression
with `0.f`, omit the `OMIT_OLD` branch, and use 16 or 256 in place of 5. Use
the baseline Git revision and its unmodified renderer for the all-full run.

## Rejected nuclear/fire compositing candidate

`composite-rejected-candidate.patch` preserves the rejected candidate against
`348a7beb174f9ad1af1b82987002f545da78845a`; it was removed from the working
renderer. `composite-probe.mm` uses the actual nuclear and ship-fire Metal
renderers with a synthetic scene. Its candidate path reproduces the candidate's
layer/sort/composite sequence; the baseline submits blasts before ship fire.
The full game renderer, sea, ships, presentation and refraction are not in this
fixture. Inputs are 320×180 on Apple M5 Max, one fire emitter, age one second,
pressure lifetime 0.1 second, and two or four blasts; four-blast timing includes
one old eight-step plume. Timings are medians of six frames after three warmups.

The overlapping two-blast fixture places large/small centres at z=6/4 or 4/6,
with radii 2/0.6, fires at z=2 or 9, and also no fire. Both creation orders
are rendered. The reference is a whole-object centre-ordered pass, which is
useful for exposing order dependence but is not an oracle for physically
interpenetrating volumes. The candidate's large-behind case differs from this
reference at 601 pixels (596 with fire in front); reversing creation order
changes zero candidate pixels. The reference comparison and the known sorting
key show the order chosen is wrong for this fixture, not that all overlapping
volume pixels have a single physically correct object order.

The retained paired logs report baseline 1.004 ms GPU / 1.182 ms wall versus
candidate 1.698 ms GPU / 1.945 ms wall for this specific effects-only workload.
An earlier run with reversed execution order gave 1.639 ms baseline and
1.047 ms candidate GPU. The direction changed between runs, so these samples
do not establish a performance regression or improvement; neither run measures
full-frame or gameplay performance.
The earlier 16×16 single-blast Metal probe separately showed foreground fire
on 10 pixels, the behind-fire path identical to its control, and 10/10 matching
foreground-red pixels with an active pressure wave. It did not test refraction
against a textured moving scene or multiple other blasts.

To repeat, compile `composite-probe.mm` with `xcrun clang++ -std=c++17 -O2
-fobjc-arc -I experiments/native-metal` and link `-framework Metal -framework
AppKit`. For the baseline, add `-DBUILD_BASELINE` and put unmodified renderer
and shader headers from the stated commit first on the include path. For the
candidate, apply the retained patch only in an isolated checkout of that commit
and use its renderer headers first. The baseline and candidate commands both
run the same fixture; `composite-baseline-log.txt` and
`composite-candidate-log.txt` retain their raw output.

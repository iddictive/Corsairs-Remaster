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

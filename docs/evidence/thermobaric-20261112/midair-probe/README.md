# Mid-air volume experiment — rejected

This is an offscreen experiment, not the renderer used by the game. The
JangaFX source, selected VDB frames, conversion profile, packaged RG8 volume
and hashes are in `src/assets/cluster-vfx/midair-vdb/`. The candidate header
is preserved here only to make this failed comparison reproducible; it must not
replace the production header. Its fixture substitutes the mid-air texture for
the entire existing ground-volume input, so it cannot prove preservation of
other warheads or integration into the app.

`rejected-air.mp4` contains 91 frames at 30 fps from the standalone Metal
fixture. It shows a synthetic sea and sky, no ship or real game scene. Selected
frames show a bright but slow-growing fireball followed by upward streaming
flames; the desired fast blast and convincing later smoke are not established.
The existing refractive wave was disabled in the experiment because it
distorted the horizon; the requested shock remains unsolved. Continuous
playback, land/sea impacts, in-game behavior and user acceptance were not
independently verified.

The comparison uses 2056×1290, input radius 42, camera (0,60,180), and one
or eight overlapping airborne impacts. The shader applies its own 2.5× visual
radius. Both CSVs contain one warmup and three measured trials per case; only
the measured values appear. The fixture's printed “median” is actually its
minimum; calculate medians from CSV instead. For eight effects at 0.30, 0.55
and 1.40 s, baseline medians were 4.382, 6.287 and 5.318 ms GPU; candidate
medians were 5.207, 3.958 and 5.062 ms. This is not an in-game frame-time
measurement, and the few trials show substantial variation.

To replay, copy `renderer-candidate.hpp.txt` as `cluster_impact_render.hpp`
and either fixture as a `.mm` file into one task-owned temporary directory.
From the repository root, compile the fixture with Apple clang 21, C++20,
`-O2 -fobjc-arc -framework Metal -framework AppKit -I experiments/native-metal`,
then run the standalone binary with an output directory and `single 6`.
The fixture references the packaged volume by repository-relative path; it
also reads the existing cluster atlases. The experiment never builds, installs,
starts or inspects the game.

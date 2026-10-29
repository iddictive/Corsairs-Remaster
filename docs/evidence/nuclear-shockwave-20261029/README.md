# Condensation pressure front

The previous pressure front integrated a density slab from the visible receiver
depth. On water it read as a broad pale wash; its nearest arc was present, not
missing. A pressure-disabled comparison changes21,665 daylight pixels in the
near-arc ROI, with a mean4.05RGB levels. `pressure-isolation.json` retains
the diagnostic; its source is baseline `e106aac` with the pressure branch disabled.

The first density-only world annulus made its rounded shape readable but looked
like a gray tube (`rejected-density-torus.png`). The final material combines a
broken rolling condensation field with refraction of the current scene. It uses
the existing noise, projection, depth and incremental transmittance facilities;
[PBRT's transmittance description](https://pbr-book.org/4ed/Volume_Scattering/Transmittance)
supports the reused optical-depth integration, not a claim of fluid simulation.

## Owner and visible tradeoff

`phonk_ordnance_shaders.hpp` owns the finite annular field, at most two radial
intervals with six samples each, one light-density tap, shading and refraction.
`phonk_ordnance_render.hpp` owns one reusable same-size/format scene snapshot.
It copies the canonical current target immediately before each blast. Earlier
blasts are included; the blast being drawn is excluded. The existing pre-water
snapshot and later HUD snapshot have incompatible contents/lifetime for this job.

Depth clips the world volume behind terrain and ships. The former all-relief
surface extrusion is deliberately removed: a front can disappear behind a hill
instead of becoming a painted band on its slopes. Preserving that projection
would require a different terrain-height receiver. No new height atlas, renderer
framework, force, damage, radius, width, lifetime, blast-slot or mushroom change
is included. Refraction rejects nearer occluders, sky/terrain crossings and
receiver jumps outside the local band support.

## Evidence and reproduction

`final-source.json` binds the final two headers; `replay.mm.txt` is the frozen
windowless Metal input. It exercises low eye height8m at650m, mixed water/sloped
land at1600m, and an opaque foreground. The scene, field and renderer are real
GPU code; no player state, game loop or window is involved. The baseline is the
two headers at `e106aac45f510eca8208e61a0fdd12b45a6c02c9`.

Compile the input from repository root with `clang++ -std=c++20 -O2 -fobjc-arc
-I. -Iexperiments/native-metal -framework Cocoa -framework Metal INPUT -o OUTPUT`.
Arguments are `OUTPUT_DIRECTORY PREFIX TIER SCENE SLOTS EYE_HEIGHT DISTANCE`;
scenes0/1/2 are water, mixed slope and complete foreground. To select baseline,
extract its renderer/shader headers from Git into the temporary include directory
as `baseline_renderer.hpp` and `phonk_ordnance_shaders.hpp`, add that include path
and `-DSHOCK_RENDERER_HEADER='"baseline_renderer.hpp"'`. Full resolution adds
`-DFIXTURE_WIDTH=2056 -DFIXTURE_HEIGHT=1329`.

`replay-results.json` is the raw output; its literal newline separators are
normalized into `metrics.json`. Each workload has24 repeats, discards four,
and records median/P95 GPU command, CPU encoding and command-to-completion wall
time. Shader-cache state is not flushed; first-encode times are fresh-object
observations, not cold-install guarantees. The pre-run check found no game;
other component GPU windows were coordinated, without global GPU isolation.

At2056×1329 with four blasts, daylight age4 GPU median is6.934→7.171ms and
P95 is7.133→7.339ms; CPU encoding median is0.023→0.044ms. Age1 GPU median
is6.965→7.034ms; age7.8 is8.201→8.245ms. These are paired component observations,
including scene copy and all four effect draws, not whole-game FPS acceptance.
The BGRA8 snapshot retains about10.42MiB at that resolution; active pressure adds one
copy per blast, while the existing effect-pass count stays unchanged.
The paired timing precedes the final clip-W guard, whose exact source hash and
delta are recorded in `final-source.json`: two comparisons outside the marches,
with no copy/pass/sample/allocation change. `projection-guard.json` reproduces
the former nonfinite bend at W0 and accepts zero bend for W0/negative, while
valid displacements remain bounded. Final-source GPU lifecycle was replayed;
full timing was not repeated for that guard.

`controls.json` records exact foreground parity at all sampled ages, exact
age0/expiry parity, and maximum1-byte difference in23 pixels for plume-only
age8.2. `motion.mm.txt` and `motion.json` retain the full0..9.2s lifecycle and
exact cold-late/warm parity. `motion-scene0.gif` and `motion-scene1.gif` show
water/shore motion at10fps and480×270; palette/size reduction makes them motion
records, not color or performance oracles. Full-size PNGs retain static material
and occlusion comparisons. Installed gameplay appearance remains a separate check.

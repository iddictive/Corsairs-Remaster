# Missile VFX standalone Metal probe

The later reported ribbed-smoke correction and its before/after measurements
are in [morphology-followup](morphology-followup/README.md).

`volley-cpu-probe.mm` samples only the CPU-side emission loop for 64 sources over 1,000 synthetic frames. One local run before and after the cohort limit change recorded 4.45 ms versus 4.68 ms in total and 2,043 versus 2,880 retained particles. Compile with `xcrun clang++ -std=c++20 -O2 -fobjc-arc -mmacosx-version-min=15.0 -I experiments/native-metal -framework Metal -framework Foundation docs/evidence/missile-trail-offscreen-20261111/volley-cpu-probe.mm -o <temp>/vfxbench`. The prior run used the header from commit `1d34755`; this is not a GPU, full-frame, or in-game FPS measurement.

This is a synthetic component probe, not a game capture or acceptance of the
integrated renderer. `proof.mm.txt` is the exact driver; it uses the existing
synthetic sea/sky scene helper from `cluster_impact_offscreen.mm`. It does not
run the game. `invariants.mm.txt` checks the bounded pool, long-frame reset and
invalid input. The only image input is `src/assets/cluster-vfx/WispySmoke02_8x8.png`.

From repository root, using a task-owned temporary output directory:

```sh
xcrun clang++ -std=c++20 -O2 -fobjc-arc -mmacosx-version-min=15.0 -I experiments/native-metal -framework Metal -framework AppKit -x objective-c++ docs/evidence/missile-trail-offscreen-20261111/proof.mm.txt -o "$PROBE_DIR/proof"
"$PROBE_DIR/proof" "$PROBE_DIR"
"$PROBE_DIR/proof" "$PROBE_DIR" close
"$PROBE_DIR/proof" "$PROBE_DIR" post
xcrun clang++ -std=c++20 -O2 -fobjc-arc -mmacosx-version-min=15.0 -I experiments/native-metal -framework Metal -framework Foundation -x objective-c++ docs/evidence/missile-trail-offscreen-20261111/invariants.mm.txt -o "$PROBE_DIR/invariants"
"$PROBE_DIR/invariants"
```

The scene is 960x540, 55-degree perspective, 90 frames at 60 Hz, with
45 m/s synthetic straight trajectories and a one-shot initial burst. It draws
0, 1, 16 or 64 missiles of mixed profiles. Only the most recent 2048 puffs
are retained globally, so the 64-missile picture does not preserve every
trail. The `post` frame is 0.5 seconds after emission and motor submission
stop; it shows the remaining world-space smoke. The rendering does not include
missile meshes, actual curved trajectories, ship occlusion or the game's
scene composition. The flashes in the fixture are fixed synthetic positions.

On Apple M5 Max, macOS 27.2, Apple clang 21, SDK 27.2: the serial wide-view
run's 12 warm samples are in `timings.csv`. Median CPU encode including the
synthetic scene was 0.0077 / 0.0191 / 0.0544 / 0.0520 ms for 0 / 1 / 16 / 64
missiles; median complete command-buffer GPU time was 0.0225 / 0.0886 /
0.7262 / 0.4063 ms respectively. GPU p90 was 0.0226 / 0.6916 / 1.3595 /
0.6930 ms; samples were variable. The 64 case is cheaper than 16 in this
fixture because the global pool evicts old particles and the camera sees a
different distribution. No conclusion about real frame time or frame rate is
supported. Initial shader/pipeline compilation, queue wait and game overhead
are outside the CPU encode timing. A single isolated update median was below
0.001 ms even at 64 missiles, too short for a reliable fine-grained estimate.

Visual result: wispy smoke persists, widens and drifts; the procedural exhaust
and flash remain stylized, and the requested Hollywood realism is not proven.
Installed/runtime quality and performance must be checked by the integration
owner and user.

## Integration follow-up

`integration-origin-probe.mm.txt` reruns the synthetic fixture against the
integration version of the module. A `shift` argument moves the renderer's
world origin and camera by 100 m while leaving the simulated smoke in its
original world coordinates. Against the unshifted output, 0 / 1 / 16 / 64
missile images differ in 0 / 4 / 14 / 0 color channels respectively, with a
maximum difference of one 8-bit level. `integration-lifecycle-probe.mm.txt`
checks launch persistence, expiry, capped long-gap emission and reset. These
are component tests, not a replay through the game bridge.

`integration-timings.csv` contains 12 warmed 960x540 samples from the
unshifted run on the same M5 Max. CPU encoding medians for 0 / 1 / 16 / 64
missiles were 0.0086 / 0.0182 / 0.0736 / 0.0728 ms; GPU medians were 0.0132 /
0.0447 / 0.3921 / 0.3002 ms. The 64 case is constrained by the global pool.
Startup, actual backend frame overhead and real gameplay performance remain
unmeasured. The atlas was loaded from the source asset for this fixture; it
does not establish that the separate runtime delivery has occurred.

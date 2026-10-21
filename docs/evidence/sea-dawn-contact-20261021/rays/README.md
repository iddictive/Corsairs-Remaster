# Downward sea fog projection

This packet covers the radial water artifact reported after returning from the world map. It proves a native Sea2 projection mechanism and its bounded correction. It does not replay the installed player's complete world-map transition or accept unchanged player FPS. No light-shaft feature, scene lifecycle, transition controller, permanent test or CMake target was changed.

The fog consumer clamped every negative view elevation to zero while keeping azimuth. Under the existing overhead map-transition camera, the entire water view therefore sampled one authored horizon row as radial wedges. A direct overhead view before the map role produces the exact same pixels as a valid upper view → WDM model role/projection → overhead return. Transition history is unnecessary to reproduce this mechanism. The [before](baseline/direct-topdown-water.png) and [after](candidate/direct-topdown-water.png) use marked synthetic authored faces and the actual imported Sea2 vertex/fragment programs.

`SKY::GenerateSky` mirrors all twenty upper vertices in Y, including the upper cap. The corrected consumer inverts the existing signed elevation lattice; the existing producer intersects the mirrored bottom cap as well as the sides and upper cap. Both retain `authoredSkyCoordinate` as the sole face/UV owner. Output stays 256×128 with the existing dispatch, sampling and update cadence. These are two header changes, based on commit `2f8fc4a`.

## Retained quality and rejected oracle

The disposable [native driver](projection-finite-field.cpp.txt) uses the existing `storm_map_player` perspective, overhead pitch and map pitch, plus `StormMetalBeginWdmModel`/`EndWdmModel`. It uses twenty generated 128×128 textures, including deliberately discontinuous side/upper markers. The CPU oracle intersects a finite six-plane cube, folds Y and samples through the canonical authored face mapping. The direct map-return comparison is byte-exact on both versions. Complete upper-view and no-fog images, and the positive-ray half of the horizon view, are byte-exact before/after; [full comparison](retained-negatives.json) also retains the intentionally changed downward half.

| RGB RMSE against finite mirrored SKY | Before | Candidate |
| --- | ---: | ---: |
| Downward, camera height 0 | 106.397644 | 0.281036 |
| Downward, camera height −0.6 | 59.023105 | 0.250155 |
| Downward, camera height +0.6, retained field resolution | 70.250838 | 0.251567 |

The [first candidate](rejected-direct-oracle/run.log) failed its original positive-height direct-source threshold: 13.348610 versus the threshold 8. That result remains rejected and retained. The [independent CPU field calculation](oracle-filter.cpp.txt) shows that the existing 256×128 lattice and linear resampling alone differ from the discontinuous direct source by 13.349527 RMSE. Against that finite representation, the same candidate readback differs by 0.251567 RMSE, maximum channel 0.998085; the old readback still fails at 70.250838. No production edit or resolution increase followed this oracle correction. The [original source](projection-direct-oracle.cpp.txt), [seam errors](direct-oracle-seam-errors.log), and both raw results remain available.

The earlier fog packet's lower-ray horizon-clamp negative is superseded by this source-backed mirrored-SKY contract. Its original receipt is preserved; it cannot accept an overhead camera.

## Timing and reconstruction

Both versions run the same forty-eight final water frames. Timing uses completed GPU-command interval unions for warmed pre-readback frames 25–56; frame 57 includes readback and is excluded. Each selected frame retains one draw, three vertices, zero buffer allocations, 5,120 uploaded bytes and 148,439,040 reported GPU bytes, with zero GPU queue wait.

The initial separated runs measured GPU median/p95 0.8333/1.9356 → 2.2946/2.7900 ms; their [raw traces](baseline/frames.csv) and [candidate trace](candidate/frames.csv) remain retained. One admitted [back-to-back old→new pair](paired/comparison.json), using the exact retained executables without a rebuild, measured 0.7655/1.0805 → 0.9701/1.2419 ms. Process wall was 0.72395 → 0.73331 s. The separated-run rise was not reproduced at that magnitude, but the pair still measures +0.2046 ms median and +0.1614 ms p95. This is a bounded 128×128 component observation, not proof of unchanged full-game speed or an isolated causal cost estimate.

[Snapshots](snapshots.json) bind source, executable and input hashes. `<REPO>` denotes the canonical checkout. Raw `.rgba` captures contain 128×128 little-endian BGRA8 pixels; PNG previews are lossless channel reorderings. Sources generate every marked texture; no player save, screenshot, proprietary texture bytes or personal paths are included.

Expand `<REPO>` in each retained compile/run command. Copy `projection-direct-oracle.cpp.txt` or `projection-finite-field.cpp.txt` to the specified task-local source path and redirect compiler output to a fresh `tools/metal_storage.py:diagnostic_run` directory. Link the existing renderer/cloud-noise static libraries, SDL and listed system frameworks. Run `<EXE> <TECHNIQUES> <RUN> baseline|candidate` with `STORM_METAL_DYNAMIC_SKY=1`, `STORM_METAL_MODERN_WATER=1`, `STORM_METAL_CINEMATIC=0`, `STORM_METAL_MODERN_LIGHTING=0`, `STORM_METAL_PROFILE=1`, and `STORM_METAL_FRAME_TRACE=<RUN>/frames.csv`. The driver explicitly flushes the native frame trace before releasing the device. The first unflushed baseline produced no timing data and is not a comparator.

Exact baseline and candidate executables remain in the existing ignored diagnostic owner at `experiments/native-metal/.cache/diagnostics/dawn-20261021-rays/{baseline-v2,candidate-v2}/projection`; their identities are in the snapshots manifest. The retained commands and prechange Git source reconstruct the native dependency path. The CPU-only finite-field oracle consumes a retained positive-height BGRA readback and needs no GPU or application launch.

# Scene color LUT

The candidate adds one immutable 32³ RGBA16Float LUT (256 KiB). FXAA and the LUT share the existing scene pass. With FXAA disabled, grading requires a scene snapshot plus a simple fullscreen pass. Cinematic controls grading; the explicit pre-HUD boundary keeps interface colors unchanged. Present fallback is excluded because it may already contain HUD.

`comparison.png`: original left, GPU LUT output right, each 640×480. Clouds occupy the upper third, open sea the lower area, with overlapping ships clipped at both sides and a separate central ship. The right panel has deeper blue water, warmer sails and stronger separation in the water pattern. This retained source frame does not establish installed-game behavior or a dramatic whole-game transformation.

`source.ppm.gz` retains the exact input independently of other unfinished work. Its uncompressed SHA-256 is `06fcb8db020c618a4d86a21009e797d63be4f441802f5fb5480ee288ddc41996`. `preview.json` records the native GPU comparison. `controls.json` records a 256-step gray ramp: exact black/white, unchanged RGB0–8, no luminance reversal and preserved varying alpha. The existing FXAA probe also passed diagonal reconstruction, low-contrast/flat exit, exact Off copy, alpha, HUD sharpness and resize/reset.
The control input is P6, 256×1, maximum255, pixel `i` equal to RGB(i,i,i).
Pass any fourth argument to `preview` to vary alpha by pixel index; read the
right-hand 256 output pixels for the graded ramp and compare successive
luminance using coefficients(0.2126,0.7152,0.0722).

`timing.json` preserves baseline/candidate pass timing at 2560×1655. Median FXAA increment was 0.051 ms. LUT-only was 0.083 ms, excluding the backend snapshot copy. These component measurements are not full-game FPS acceptance. The live game remained running; no shared build/install was used for these probes.

Reproduce from repository root in a disposable scratch directory (`$TASK_SCRATCH` below). Compile using Xcode 27.1 / Apple clang 21, C++20, ARC, optimization O2, deployment floor macOS15; observed host macOS27.0.1 on Apple M3 Max.

```sh
xcrun clang++ -std=c++20 -fobjc-arc -O2 -mmacosx-version-min=15.0 -framework Foundation -framework Metal -Iexperiments/native-metal docs/evidence/scene-color-grade-20261023/measurement.mm -o "$TASK_SCRATCH/measurement"
"$TASK_SCRATCH/measurement" 1 0
"$TASK_SCRATCH/measurement" 1 1
"$TASK_SCRATCH/measurement" 0 1
"$TASK_SCRATCH/measurement" 0 0
git show 8e635ad50f95ead29c5212ab5c4ad4790b3e30f6:experiments/native-metal/fxaa.hpp > "$TASK_SCRATCH/fxaa.hpp"
xcrun clang++ -std=c++20 -fobjc-arc -O2 -mmacosx-version-min=15.0 -framework Foundation -framework Metal -I"$TASK_SCRATCH" docs/evidence/scene-color-grade-20261023/baseline_measurement.mm -o "$TASK_SCRATCH/baseline"
"$TASK_SCRATCH/baseline"
gzip -dc docs/evidence/scene-color-grade-20261023/source.ppm.gz > "$TASK_SCRATCH/source.ppm"
xcrun clang++ -std=c++20 -fobjc-arc -O2 -mmacosx-version-min=15.0 -framework Foundation -framework Metal -Iexperiments/native-metal docs/evidence/scene-color-grade-20261023/preview.mm -o "$TASK_SCRATCH/preview"
"$TASK_SCRATCH/preview" "$TASK_SCRATCH/source.ppm" "$TASK_SCRATCH/comparison.ppm"
```

Installed scene/HUD replay and comparable full-frame timing remain required before claiming the requested in-game performance and visual outcome.
`build.json` retains the successful shared offline build's source/engine hashes,
this batch's five exact source hashes and the installed signed-engine hash.
Installation and signature were verified without launching the game; gameplay
and full-frame performance acceptance remain separate.

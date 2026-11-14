# Distant foam patch artefacts

The user-supplied 1280×800 view matches the eight-ship high-altitude fixture. The earlier `nofoam.png` removed the square patches, identifying the foam pass, while its prior texture-only filtering left them visible. The remaining large squares came from the interpolated vertex-height alpha, not the foam texture. This change fades that ordinary alpha as its world-space pixel footprint grows; event foam retains its own coverage. `before.png` and `after.png` show the paired 3,800 m view; the 40 m before/after PPM SHA-256 was identical (`e940a30d7304fafb51d9c562afe0d500a752ed2cd3c7e02a7909408eb3ff6d77`). `nuclear-near.png` is a separate single-wave smoke observation, not full battle acceptance. Ordinary foam fades with distance, while water reflection and event-specific coverage remain.

On Apple M5 Max, in 60-frame sequential 1280×800 modern-water runs, median GPU time after warmup was 2.893 ms before and 3.335 ms after; CPU was 8.028 and 8.611 ms. These short samples are noisy and do not establish battle performance. Near-view, other AA modes, multiple nuclear waves and installed gameplay remain outside this evidence.

From the repo root, the saved `replay.py` reuses the prior altitude fixture and captures a PPM plus CPU/GPU CSV to `OUTPUT_FRAME`. Use `PREVIOUS=1` for the captured previous shader and omit it for the current shader. Run each case sequentially under `codex-run-scratch`, for example:

```sh
CANDIDATE=1 FOAMFIX=prod STORM_METAL_MODERN_WATER=1 PROBE_FRAMES=60 PROBE_HEIGHT=3800 PROBE_WIDTH=1280 PROBE_VIEW_HEIGHT=800 OUTPUT_FRAME=/path/in/task-temp/frame.ppm /Users/md/.codex/bin/codex-run-scratch run --max-bytes 2147483648 --max-files 30000 --seconds 300 --reserve-bytes 10737418240 -- python3 docs/evidence/sea-high-altitude-20261105/foam-pixel-20261105/replay.py
```

The prior source is `before-sea_shaders.hpp.txt`. The original fleet fixture, build/toolchain inputs and attribution remain in the parent evidence directory. Rendering is offscreen, not gameplay.

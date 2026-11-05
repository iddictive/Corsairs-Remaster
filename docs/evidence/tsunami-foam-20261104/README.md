# Tsunami foam mask investigation

Later nuclear-wave material investigation: [nuclear foam investigation](../nuclear-foam-20261105/README.md).

The selected candidate bends the existing texture domain with continuous sine terms and retains the original texture warp of 1.15. It uses the existing six texture taps. The comparison is a planar isolated mask, not a rendered game crest: horizontal dark bands remain visible, and actual crest geometry, perspective, fog, lighting, animation quality and frame cost are unverified.

## Conditions and results

Snapshots were extracted from `experiments/native-metal/anti_tiling_msl.hpp` and `sea_shaders.hpp` in the checkout based on commit `2f6df20a3605726a3fcb225225dd09bf5204b643`, with the latter then modified in the working tree. The snapshots are standalone Metal shader fixtures, not product source. Only the `SeaU` address space and fog are adapted to the fixture; fog is identity. Each image represents 180 × 180 world units on a 1024 × 1024 planar render, with severity 1, native foam 0.8, amplitudes 14, 28 and 56, and times 0 and 20. The input is the seven mip levels of the original 512 × 512 PENA texture, loaded with repeating linear sampling.

The source texture used was `experiments/native-metal/.cache/runtime/RESOURCE/Textures/WEATHER/SEA/PENA/pena.tga.tx`, SHA-256 `ac356b4e560652ca46d6b7a378780b1f833d885ec37b3cef1105448e8e0be878`. The installed app also contains the same file at `Contents/Resources/resource/Textures/WEATHER/SEA/PENA/pena.tga.tx`. The texture is not included here; reproduction requires a copy with that hash. Its future availability is not guaranteed by this evidence directory.

`measurements.txt` contains every measured case, coverage mean and standard deviation, mean absolute neighboring-pixel difference, one-tile non-wrapping Pearson correlation in each axis, and all 20 sorted warm GPU time samples. The one-tile metric is a limited periodicity proxy, not a proof that visible repetition is absent; neighboring-pixel difference is a fragmentation proxy, not a perceptual-quality score. Baseline to sine-115 averages across six frames: x correlation 0.0760 → 0.0473, y 0.0422 → 0.0285, mask mean 0.5861 → 0.5803, neighboring difference 0.1984 → 0.2044. Individual scale/time results matter; see the data rather than relying on these averages.

Images show the full mask at amplitude 28 and times 0 and 20; brighter pixels have higher foam alpha. The sine-180 image shows the stronger warp alternative, which lowered correlations further but also reduced coverage and increased fine fragmentation. The measurements were on Apple M5 Max, macOS 27.2, Apple clang 21.0.0, SDK 27.2. GPU readings vary with scheduling and clock state, measure one offscreen full-screen draw and do not establish game FPS or whole-scene performance.

## Reproduce

From the repository root, set `FOAM_WORK` to an existing writable scratch directory and `FOAM_PYTHON` to a Python 3 interpreter with NumPy and Pillow. The texture file must exist and match the hash above.

```sh
xcrun clang++ -std=c++20 -O2 -fobjc-arc docs/evidence/tsunami-foam-20261104/probe.mm -framework Foundation -framework Metal -framework CoreGraphics -framework ImageIO -o "$FOAM_WORK/probe"
"$FOAM_PYTHON" docs/evidence/tsunami-foam-20261104/compare.py "$FOAM_WORK/probe" experiments/native-metal/.cache/runtime/RESOURCE/Textures/WEATHER/SEA/PENA/pena.tga.tx "$FOAM_WORK/frames" > "$FOAM_WORK/measurements.txt"
```

`baseline.metal`, `sine-115.metal`, and `sine-180.metal` are the exact shader inputs; `compare.py` substitutes amplitude and emits all 18 frames and measurements. The render compilation emits a harmless mixed-enum warning from the Core Graphics bitmap flags. Reproduction must preserve the fixture's output and limits; it is not a game acceptance test.

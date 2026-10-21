# Horizon cut: water-path exclusion evidence

2026-10-21, Apple M3 Max, macOS 27.0.1 (26A434), arm64; Apple clang 21.0.0 (clang-2100.3.34.2), SDK 27.0, deployment target 15.0, C++20/O2. Checkout base was `999c349bc9a2c5d48b8939aee43adb800679c21a` plus the active task's earlier fog repair. These are bounded native diagnostics, **not player-scene acceptance or proof that the reported horizon cut is fixed**.

The retained [source snapshot](opaque-rope-water.cpp.txt) uses the real installed opaque DXT1 `sail_rope.tga.tx` base level, buffered XYZ|NORMAL|TEX1 geometry, ShipRope's alpha-test/blend/combiner pattern, a foreground strip at z=2 and Sea2 at z=4. Alpha reference is 128 rather than the real entity's 160; the consumed texture is fully opaque, so both accept its fragments. The geometry is synthetic, not a real ROPE entity or complete deck scene. Its source SHA256 is `c73e7f746be305c8eb6287095d6a468fe19c27b8af3642a9c661b722fbd28b31`; the discarded executable was `87a6dc663d9ded3d8e2deb8f625a8540eca935246755ae33c5a3249607eafd0a`.

Actual asset: `/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/Textures/sail_rope.tga.tx`, SHA256 `70fa91be3f40346817dc7f5b96b5bea86ef057fc6af4a780e79c821efbd0f5f6`, TX header 64×256, five mips, DXT1, 8192-byte base level. The proprietary asset stays with the installed application; it is not copied into Git.

| Captured run | Requested settings | Depth write on | Deliberately off |
| --- | --- | --- | --- |
| [Default](opaque-rope-default.log) | modern water | `ff5c3418 → ff5c3418` | `ff5c3418 → 00472f17` |
| [AA mode 2](opaque-rope-aa-mode2.log) | `STORM_METAL_AA_MODE=2` | identical | identical |
| [AA mode 2 + MetalFX](opaque-rope-aa-mode2-metalfx1.log) | additionally `STORM_METAL_UPSCALING=1` | `ff523116 → ff523116` | `ff523116 → 003f2c14` |

The three small logs preserve console tool stdout/stderr verbatim; they were captured in the chat execution results rather than shell-redirected at execution. The mode-2 runs establish a negative result under that requested configuration; private MSAA attachment use was not independently captured. Startup `samples 1` names the public backbuffer. MetalFX activation is explicitly logged as 85×85 → 128×128. Readback between the rope and water forces an encoder boundary, so this does not prove every uninterrupted gameplay transition.

The [real entity benchmark log](real-entity-benchmark.log) and [manifest](real-entity-benchmark-manifest.json) are copied from the existing 1920×1080, eight-ship, 160-frame run, with the absolute trace directory replaced by `<BENCHMARK_RUN>`. Its SAIL_ROPE and VANT signatures show FVF `00000112`, alpha test on, reference 160, blend on and depth write on. This independently rejects an assumed normal-technique depth-write failure. That fixture omits SUNGLOW, so it cannot accept the late solar-overlay path.

Reconstruction, from the repository root after its normal native build prerequisites are available; `$TASK_TMP` must be a fresh task-owned temporary directory:

```sh
cp docs/evidence/sea-dawn-contact-20261021/horizon/opaque-rope-water.cpp.txt "$TASK_TMP/probe.cpp"
clang++ -std=c++20 -O2 -mmacosx-version-min=15.0 \
  -I experiments/native-metal \
  -I experiments/native-metal/inputs/native/d3d9/sdl2-fixed/include/SDL2 \
  "$TASK_TMP/probe.cpp" experiments/native-metal/.cache/build/libstorm_metal.a \
  experiments/native-metal/.cache/build/libcloud_noise_assets.a \
  -L experiments/native-metal/inputs/native/d3d9/sdl2-fixed/lib -lSDL2 \
  -Wl,-rpath,"$PWD/experiments/native-metal/inputs/native/d3d9/sdl2-fixed/lib" \
  -framework Metal -framework MetalFX -framework Cocoa -framework QuartzCore \
  -o "$TASK_TMP/probe"
"$TASK_TMP/probe" experiments/native-metal/engine/src/techniques
STORM_METAL_AA_MODE=2 "$TASK_TMP/probe" experiments/native-metal/engine/src/techniques
STORM_METAL_UPSCALING=1 STORM_METAL_AA_MODE=2 "$TASK_TMP/probe" experiments/native-metal/engine/src/techniques
```

The source snapshot is evidence, not registered test infrastructure. Rebuilding against later backend changes is a new condition; the original static archive hash was not captured at link time. Real entity benchmark reproduction uses the existing `python3 tools/run_sea_benchmark.py --frames 160 --warmup 128 --width 1920 --height 1080 --ships 8` entrypoint and manifest settings; the original benchmark executable identity is in its manifest.

Source handoff: `core/include/entity.h` declares unsigned `priority_t`; `EntityManager::AddToLayer` sorts ascending. Gameplay SunGlow priority -2 therefore runs after Sea65530 and Rope33. `SUNGLOW::Realize` submits the additive, depth-disabled `sunglow_noz` halo clipped at `y=cameraY`. That is the concrete next candidate for the upper/lower foreground color discontinuity; the solar owner must prove it with actual SUNGLOW, near rope, distant island and unobstructed sky before accepting a repair. No water or rigging production behavior was changed by this diagnostic.

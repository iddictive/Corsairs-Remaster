#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
native=$(cd "$root/../native-storm" && pwd)
engine_repo=${STORM_ENGINE_REPO:-/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs}
revision=4860fe13245b747973682c88b2f1d9e4850c8c40
mkdir -p "$root/.cache"
baseline_ref=df1e021ffa18f43daf35d01645978f6f22a9b238
python3 - "$root" "$baseline_ref" <<'PY'
from pathlib import Path
import subprocess, sys
root = Path(sys.argv[1])
source = subprocess.check_output(['git', '-C', str(root / '../..'), 'show',
 sys.argv[2] + ':experiments/native-storm/CMakeLists.txt'], text=True)
# The pinned build definition keeps using the baseline's reusable dependency,
# audio and texture helper paths, while STORM_SOURCE points to our own snapshot.
source = source.replace('${CMAKE_CURRENT_SOURCE_DIR}', '${NATIVE_ROOT}')
target = root / '.cache/native-project/CMakeLists.txt'
target.parent.mkdir(parents=True, exist_ok=True)
if not target.exists() or target.read_text() != source:
 target.write_text(source)
PY
git -C "$root/../.." show "$baseline_ref:experiments/native-storm/native.patch" > "$root/.cache/native.patch"
patch_hash=$(shasum -a 256 "$root/.cache/native.patch" | cut -d ' ' -f 1)
base_fingerprint="$revision:$patch_hash:metal-window-v1"
platform_hash=$(shasum -a 256 "$root/platform.patch" | cut -d ' ' -f 1)
fingerprint="$base_fingerprint:$platform_hash"
if [[ -f "$root/.cache/source-fingerprint" ]]; then
 current_fingerprint=$(cat "$root/.cache/source-fingerprint")
 if [[ "$current_fingerprint" == "$base_fingerprint" ]]; then
 patch --dry-run -p1 -d "$root/.cache/storm" < "$root/platform.patch"
 patch -p1 -d "$root/.cache/storm" < "$root/platform.patch"
 printf '%s\n' "$fingerprint" > "$root/.cache/source-fingerprint"
 elif [[ "$current_fingerprint" != "$fingerprint" ]]; then
 echo 'Metal snapshot inputs changed; reconcile the isolated snapshot before rebuilding.' >&2
 exit 1
 fi
else
 mkdir -p "$root/.cache/storm"
 git -C "$engine_repo" archive "$revision" | tar -x -C "$root/.cache/storm"
 patch -p1 -d "$root/.cache/storm" < "$root/.cache/native.patch"
 python3 - "$root/.cache/storm/src/libs/window/src/sdl_window.cpp" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
assert s.count('flags |= SDL_WINDOW_VULKAN;') == 1
p.write_text(s.replace('flags |= SDL_WINDOW_VULKAN;', 'flags |= SDL_WINDOW_METAL;'))
PY
 patch -p1 -d "$root/.cache/storm" < "$root/platform.patch"
 printf '%s\n' "$fingerprint" > "$root/.cache/source-fingerprint"
fi
# Verify the actual source, not only its stamp: git apply inside this nested
# archive can silently exclude paths relative to the enclosing repository.
if ! rg -q 'SDL_PeepEvents\(&quitEvent' "$root/.cache/storm/src/apps/engine/src/main.cpp"; then
 patch --dry-run -p1 -d "$root/.cache/storm" < "$root/platform.patch"
 patch -p1 -d "$root/.cache/storm" < "$root/platform.patch"
fi
# Validate and migrate the whole ordered stack, including overlapping patches.
python3 "$root/apply_source_patches.py" --source "$root/.cache/storm" \
 --state "$root/.cache/source-patches.json" \
 "$root/xcode27-compat.patch" "$root/save-areference-liveness.patch" "$root/attributes-parent-chain.patch" "$root/frame-limiter.patch" "$root/timing.patch" "$root/scene-hud-boundary.patch" "$root/interface-back-scene-metal.patch" "$root/weather-visual-temporal.patch" "$root/dynamic-sky-bridge.patch" "$native/compiler-extern.patch" "$native/controls-telemetry.patch" "$native/gunfire-audio.patch" \
 "$root/deck-walk.patch" "$root/character-skinning-normals.patch" "$root/scene-lighting.patch" "$root/ship-point-shadow.patch" "$root/authored-window-light-shaft-mesh.patch" \
 "$root/model-gpu-skinning.patch" "$root/sea-geometry.patch" "$root/sea-reflection-budget.patch" "$root/island-geometry.patch" "$root/scene-light-catalog.patch" "$root/stable-shadow-lamp.patch" "$root/multi-shadow-lamps-v2.patch" "$root/godrays-locator-v2.patch" "$root/ship-light-ownership.patch" "$root/camera-relative-lighting.patch" "$root/smooth-location-lights.patch" "$root/location-light-distance.patch" "$root/rockk2-system.patch" "$root/dynamic-town-shadows.patch" "$root/rigging-lighting.patch" "$root/location-camera-follow-reset.patch" "$root/metal-camera-view-restore.patch" "$root/renderer-buffer-guard.patch" "$root/group-target-liveness.patch" \
 "$root/renderer-grass.patch" "$root/vegetation-continuity.patch" "$root/sea-island-vegetation.patch" "$root/renderer-particles-fx.patch" "$root/renderer-rain-weather.patch" "$root/renderer-sky-astronomy.patch" "$root/renderer-ui-fonts.patch" "$root/renderer-world-map.patch" "$root/sea-postprocess-uv.patch" "$root/texture-loader-mips.patch"
python3 "$root/renderer_consumer_coverage_probe.py"
# Reuse installed headers and dependencies without rebuilding the playable port.
for required in "$native/.cache/sse2neon.h" "$native/.cache/stb_image.h" \
 "$native/.cache/d3d9/dxvk-native/include/native/directx/d3d9.h" \
 "$native/.cache/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib"; do
 test -f "$required" || { echo "Missing baseline dependency: $required" >&2; exit 1; }
done
cmake -S "$root" -B "$root/.cache/build" -G Ninja \
 -DCMAKE_BUILD_TYPE=Release -DSTORM_ENABLE_AUDIO=ON \
 -DSDL2_DIR="$native/.cache/d3d9/sdl2-fixed/lib/cmake/SDL2" \
 > "$root/.cache/configure.log" 2>&1 || {
 python3 - "$root/.cache/configure.log" <<'PY'
from pathlib import Path
import sys
lines = Path(sys.argv[1]).read_text().splitlines()
errors = [i for i, line in enumerate(lines) if 'CMake Error' in line]
selected = sorted({n for i in errors for n in range(i, min(i + 14, len(lines)))})
if not selected:
 selected = list(range(max(0, len(lines) - 20), len(lines)))
for i in selected[:60]:
 print(lines[i], file=sys.stderr)
print('Full configure log: ' + sys.argv[1], file=sys.stderr)
PY
 exit 1
}
 cmake --build "$root/.cache/build" --target engine fxaa-probe texture-loader-mip-probe alpha_sampler-probe anti_tiling_gpu-probe anti_tiling_policy-probe sea_point_shadow-probe sea_point_shadow_gpu-probe ffp_lighting_gpu-probe lighting-scene-probe metal-probe sea-probe particles-caustic-probe land_shader-probe postprocess-probe stars-probe ffp_generated-probe depth_water-probe water_camera-probe lighting-probe outdoor_lighting-probe outdoor_point_shadow-probe cinematic-probe cinematic_postfx-probe scene_hud_boundary-probe menu_unlit_native-probe dynamic_sky-probe dynamic_sky_backend-probe frame-arena-probe frame-flight-probe ffp-persistent-cache-probe ambient-occlusion-gpu-probe volumetric-light-shafts-gpu-probe shadow-probe weapon-probe indexed-probe native_legacy3d-probe indexed_span_frame_cache-probe progressive_shadow_softness-probe timing-probe frame-limiter-probe dynamic_lighting-probe character_contact-probe ffp_frame_cache-probe island-pn-probe gpu-skinning-parity-probe gpu-skinning-offscreen-probe light-shaft-apertures-probe -j 10 > "$root/.cache/build.log" 2>&1 || {
 rg 'error:|FAILED:|undefined|Undefined|ld:' "$root/.cache/build.log" >&2 || true
 exit 1
}
file "$root/.cache/build/bin/engine-1"
if otool -L "$root/.cache/build/bin/engine-1" | rg -i 'dxvk|moltenvk|vulkan'; then
 echo 'Rejected: candidate still links a Vulkan translation library.' >&2
 exit 1
fi
patch --batch --reverse --dry-run -p1 -d "$root/.cache/storm" < "$native/compiler-extern.patch" >/dev/null
python3 - "$root" "$native/compiler-extern.patch" <<'PY_RECEIPT'
from pathlib import Path
import hashlib, json, sys
root = Path(sys.argv[1])
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
(root / '.cache/gameplay-compiler.json').write_text(json.dumps({
 'engine_sha256': digest(root / '.cache/build/bin/engine-1'),
 'patch_sha256': digest(Path(sys.argv[2])),
 'graphics_layer_sha256': digest(root / '../../tools/metal_graphics_settings.py'),
 'gameplay_patches': {
 name: digest(root / '../native-storm' / name)
 for name in ('deck-walk.patch',)
 }
}, indent=2) + '\n')
PY_RECEIPT

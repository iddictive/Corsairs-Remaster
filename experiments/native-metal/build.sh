#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
native=$(cd "$root/../native-storm" && pwd)
inputs="$root/inputs"
if [[ -d "$inputs" ]]; then
 python3 "$root/../../tools/prepare_metal_inputs.py" --check
 export PATH="$inputs/toolchain/bin:$PATH"
fi
engine_repo=${STORM_ENGINE_REPO:-/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs}
revision=4860fe13245b747973682c88b2f1d9e4850c8c40
mkdir -p "$root/.cache"
baseline_ref=df1e021ffa18f43daf35d01645978f6f22a9b238
legacy_platform_ref=9d111d61c6aaab1be9f1f7335f8d9766100f41e6
python3 - "$root" "$baseline_ref" <<'PY'
from pathlib import Path
import subprocess, sys
root = Path(sys.argv[1])
captured = root / 'inputs/baseline/CMakeLists.txt'
source = captured.read_text() if captured.is_file() else subprocess.check_output(
 ['git', '-C', str(root / '../..'), 'show',
  sys.argv[2] + ':experiments/native-storm/CMakeLists.txt'], text=True)
# The pinned build definition keeps using the baseline's reusable dependency,
# third-party audio dependencies and texture helper paths, while STORM_SOURCE
# and the Metal audio implementation point to their own owners.
source = source.replace('${CMAKE_CURRENT_SOURCE_DIR}/.cache', '${METAL_DEPENDENCIES}')
source = source.replace('${CMAKE_CURRENT_SOURCE_DIR}', '${NATIVE_ROOT}')
source = source.replace('include_directories(SYSTEM /opt/homebrew/include)', '')
source = source.replace(
 'set(STORM_NATIVE_AUDIO_SOURCE \"${NATIVE_ROOT}/audio\")',
 'set(STORM_NATIVE_AUDIO_SOURCE \"${METAL_AUDIO_SOURCE}\")')
assert 'set(STORM_NATIVE_AUDIO_SOURCE \"${METAL_AUDIO_SOURCE}\")' in source
target = root / '.cache/native-project/CMakeLists.txt'
target.parent.mkdir(parents=True, exist_ok=True)
if not target.exists() or target.read_text() != source:
 target.write_text(source)
PY
if [[ -f "$inputs/baseline/native.patch" ]]; then
 cp "$inputs/baseline/native.patch" "$root/.cache/native.patch"
else
 git -C "$root/../.." show "$baseline_ref:experiments/native-storm/native.patch" > "$root/.cache/native.patch"
fi
patch_hash=$(shasum -a 256 "$root/.cache/native.patch" | cut -d ' ' -f 1)
base_fingerprint="$revision:$patch_hash:metal-window-v1"
legacy_platform_hash=ca223f8e386f0d22dd03080c8818fd27bb814d44b9e90febaf2d163d1bafc8c9
legacy_fingerprint="$base_fingerprint:$legacy_platform_hash"
fingerprint="$base_fingerprint:ordered-platform-stack-v1"
bootstrap_prefix=()
state_status() {
 python3 "$root/apply_source_patches.py" --source "$root/.cache/storm" \
  --state "$root/.cache/source-patches.json" --state-status platform.patch
}
if [[ -f "$root/.cache/source-fingerprint" ]]; then
 current_fingerprint=$(cat "$root/.cache/source-fingerprint")
 case "$current_fingerprint" in
  "$base_fingerprint")
   case "$(state_status)" in
    absent|promoted) ;;
    *)
     echo 'Metal base snapshot state is not safe to resume.' >&2
     exit 1
     ;;
   esac
   ;;
  "$fingerprint") ;;
  "$legacy_fingerprint")
   case "$(state_status)" in
    legacy)
     legacy_platform="$root/.cache/platform-legacy/platform.patch"
     mkdir -p "${legacy_platform%/*}"
     if [[ -f "$inputs/baseline/platform-legacy.patch" ]]; then
      cp "$inputs/baseline/platform-legacy.patch" "$legacy_platform"
     else
      git -C "$root/../.." show "$legacy_platform_ref:experiments/native-metal/platform.patch" > "$legacy_platform"
     fi
     test "$(shasum -a 256 "$legacy_platform" | cut -d ' ' -f 1)" = "$legacy_platform_hash" || {
      echo 'Recorded legacy platform patch bytes no longer match their approved hash.' >&2
      exit 1
     }
     bootstrap_prefix=(--bootstrap-prefix "$legacy_platform")
     ;;
    promoted) ;;
    *)
     echo 'Metal legacy platform snapshot state is not safe to migrate.' >&2
     exit 1
     ;;
   esac
   ;;
  *)
   echo 'Metal snapshot inputs changed; reconcile the isolated snapshot before rebuilding.' >&2
   exit 1
   ;;
 esac
else
 mkdir -p "$root/.cache/storm"
 if [[ -f "$inputs/engine.tar" ]]; then
  tar -xf "$inputs/engine.tar" -C "$root/.cache/storm"
 else
  git -C "$engine_repo" archive "$revision" | tar -x -C "$root/.cache/storm"
 fi
 patch -p1 -d "$root/.cache/storm" < "$root/.cache/native.patch"
 python3 - "$root/.cache/storm/src/libs/window/src/sdl_window.cpp" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
assert s.count('flags |= SDL_WINDOW_VULKAN;') == 1
p.write_text(s.replace('flags |= SDL_WINDOW_VULKAN;', 'flags |= SDL_WINDOW_METAL;'))
PY
 printf '%s\n' "$base_fingerprint" > "$root/.cache/source-fingerprint"
fi
# Validate and migrate the whole ordered stack, including the platform patch.
python3 "$root/apply_source_patches.py" --source "$root/.cache/storm" \
 --state "$root/.cache/source-patches.json" \
 ${bootstrap_prefix[@]+"${bootstrap_prefix[@]}"} \
 "$root/platform.patch" "$root/xcode27-compat.patch" "$root/save-areference-liveness.patch" "$root/attributes-parent-chain.patch" "$root/frame-limiter.patch" "$root/timing.patch" "$root/scene-hud-boundary.patch" "$root/interface-back-scene-metal.patch" "$root/weather-visual-temporal.patch" "$root/dynamic-sky-bridge.patch" "$native/compiler-extern.patch" "$native/controls-telemetry.patch" "$native/gunfire-audio.patch" \
 "$root/deck-walk.patch" "$root/character-skinning-normals.patch" "$root/scene-lighting.patch" "$root/ship-point-shadow.patch" "$root/authored-window-light-shaft-mesh.patch" \
 "$root/model-gpu-skinning.patch" "$root/sea-geometry.patch" "$root/sea-reflection-budget.patch" "$root/island-geometry.patch" "$root/scene-light-catalog.patch" "$root/stable-shadow-lamp.patch" "$root/multi-shadow-lamps-v2.patch" "$root/godrays-locator-v2.patch" "$root/ship-light-ownership.patch" "$root/camera-relative-lighting.patch" "$root/smooth-location-lights.patch" "$root/location-light-distance.patch" "$root/rockk2-system.patch" "$root/dynamic-town-shadows.patch" "$root/rigging-lighting.patch" "$root/location-camera-follow-reset.patch" "$root/metal-camera-view-restore.patch" "$root/cannon-trajectory-aim.patch" "$root/cannon-ball-volume.patch" "$root/deck-eye-height.patch" "$root/deck-aim-zoom.patch" "$root/renderer-buffer-guard.patch" "$root/group-target-liveness.patch" \
 "$root/renderer-grass.patch" "$root/vegetation-continuity.patch" "$root/sea-island-vegetation.patch" "$root/renderer-particles-fx.patch" "$root/renderer-rain-weather.patch" "$root/renderer-sky-astronomy.patch" "$root/renderer-ui-fonts.patch" "$root/renderer-world-map.patch" "$root/sea-postprocess-uv.patch" "$root/texture-loader-mips.patch" "$root/metal-graphics-live.patch" "$root/baked-static-shadow-casters.patch" "$root/external-url.patch" "$root/player-fight-push.patch" "$root/sun-below-horizon.patch"
printf '%s\n' "$fingerprint" > "$root/.cache/source-fingerprint"
python3 "$root/renderer_consumer_coverage_probe.py"
# Reuse installed headers and dependencies without rebuilding the playable port.
dependency_root="$native/.cache"
conan_data="${STORM_CONAN_DATA:-$HOME/Library/Application Support/CorsairsBuild/conan-home/.conan/data}"
if [[ -f "$inputs/manifest.json" ]]; then
 dependency_root="$inputs/native"
 conan_data="$inputs/conan"
fi
for required in "$dependency_root/sse2neon.h" "$dependency_root/stb_image.h" \
 "$dependency_root/d3d9/dxvk-native/include/native/directx/d3d9.h" \
 "$dependency_root/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib"; do
 test -f "$required" || { echo "Missing baseline dependency: $required" >&2; exit 1; }
done
cmake -S "$root" -B "$root/.cache/build" -G Ninja \
 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DSTORM_ENABLE_AUDIO=ON \
 -DMETAL_DEPENDENCIES="$dependency_root" -DCONAN_DATA="$conan_data" \
 -DSDL2_DIR="$dependency_root/d3d9/sdl2-fixed/lib/cmake/SDL2" \
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
# Normal staging needs the engine only; diagnostic probes remain explicit CMake targets.
cmake --build "$root/.cache/build" --target engine -j 10 > "$root/.cache/build.log" 2>&1 || {
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
 'engine_deck_patch_sha256': digest(root / 'deck-walk.patch'),
 'graphics_layer_sha256': digest(root / '../../tools/metal_graphics_settings.py'),
 'menu_branding_layer_sha256': digest(root / '../../tools/metal_menu_branding.py'),
 'external_url_patch_sha256': digest(root / 'external-url.patch'),
 'gameplay_patches': {
 name: digest(root / '../native-storm' / name)
 for name in ('deck-walk.patch',)
 }
}, indent=2) + '\n')
PY_RECEIPT

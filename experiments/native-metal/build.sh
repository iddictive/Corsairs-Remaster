#!/bin/bash
set -euo pipefail
metal_build_jobs=${CORSAIRS_METAL_BUILD_JOBS:-2}
if [[ ! "$metal_build_jobs" =~ ^[1-9][0-9]*$ ]]; then
 echo 'CORSAIRS_METAL_BUILD_JOBS must be a positive integer.' >&2
 exit 2
fi
root=$(cd "$(dirname "$0")" && pwd)
native=$(cd "$root/../native-storm" && pwd)
inputs="$root/inputs"
mkdir -p "$root/.cache"
python3 "$root/../../tools/metal_storage.py"
trap 'build_status=$?; python3 "$root/../../tools/metal_storage.py" || { [[ "$build_status" -ne 0 ]] || exit 1; }; exit "$build_status"' EXIT
if [[ -d "$inputs" ]]; then
 python3 "$root/../../tools/prepare_metal_inputs.py" --check
 export PATH="$inputs/toolchain/bin:$PATH"
fi
# Git-tracked sources are the build inputs. Ordinary builds never rewrite them.
source_identity=$(python3 "$root/../../tools/metal_engine_source.py" fingerprint)
python3 "$root/renderer_consumer_coverage_probe.py"
dependency_root="$native/.cache"
conan_data="${STORM_CONAN_DATA:-$HOME/Library/Application Support/CorsairsBuild/conan-home/.conan/data}"
if [[ -f "$inputs/manifest.json" ]]; then
 dependency_root="$inputs/native"
 conan_data="$inputs/conan"
fi
for required in "$dependency_root/sse2neon.h" "$dependency_root/stb_image.h" \
 "$dependency_root/d3d9/dxvk-native/include/native/windows/windows_base.h" \
 "$dependency_root/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib"; do
 test -f "$required" || { echo "Missing baseline dependency: $required" >&2; exit 1; }
done
cmake -S "$root" -B "$root/.cache/build" -G Ninja \
 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DSTORM_ENABLE_AUDIO=ON \
 -DSTORM_SOURCE="$root/engine" -DMETAL_DEPENDENCIES="$dependency_root" -DCONAN_DATA="$conan_data" \
 -DSDL2_DIR="$dependency_root/d3d9/sdl2-fixed/lib/cmake/SDL2" \
 > "$root/.cache/configure.log" 2>&1 || {
 tail -50 "$root/.cache/configure.log" >&2
 exit 1
}
cmake --build "$root/.cache/build" --target engine -j "$metal_build_jobs" > "$root/.cache/build.log" 2>&1 || {
 rg 'error:|FAILED:|undefined|Undefined|ld:' "$root/.cache/build.log" >&2 || true
 exit 1
}
file "$root/.cache/build/bin/engine-1"
if otool -L "$root/.cache/build/bin/engine-1" | rg -i 'dxvk|moltenvk|vulkan'; then
 echo 'Rejected: candidate still links a Vulkan translation library.' >&2
 exit 1
fi
python3 "$root/../../tools/metal_engine_source.py" record \
 --engine "$root/.cache/build/bin/engine-1" --expected "$source_identity"

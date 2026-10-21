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
fingerprint="$revision:$patch_hash:metal-window-v1"
if [[ -f "$root/.cache/source-fingerprint" ]]; then
    if [[ $(cat "$root/.cache/source-fingerprint") != "$fingerprint" ]]; then
        echo 'Metal snapshot inputs changed; reconcile the isolated snapshot before rebuilding.' >&2
        exit 1
    fi
else
    mkdir -p "$root/.cache/storm"
    git -C "$engine_repo" archive "$revision" | tar -x -C "$root/.cache/storm"
    git -C "$root/.cache/storm" apply "$root/.cache/native.patch"
    python3 - "$root/.cache/storm/src/libs/window/src/sdl_window.cpp" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
assert s.count('flags |= SDL_WINDOW_VULKAN;') == 1
p.write_text(s.replace('flags |= SDL_WINDOW_VULKAN;', 'flags |= SDL_WINDOW_METAL;'))
PY
    printf '%s\n' "$fingerprint" > "$root/.cache/source-fingerprint"
fi
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
cmake --build "$root/.cache/build" --target engine metal-probe -j 10 > "$root/.cache/build.log" 2>&1 || {
    rg 'error:|FAILED:|undefined|Undefined|ld:' "$root/.cache/build.log" >&2 || true
    exit 1
}
file "$root/.cache/build/bin/engine-1"
if otool -L "$root/.cache/build/bin/engine-1" | rg -i 'dxvk|moltenvk|vulkan'; then
    echo 'Rejected: candidate still links a Vulkan translation library.' >&2
    exit 1
fi

#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
engine_repo=${STORM_ENGINE_REPO:-/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs}
revision=4860fe13245b747973682c88b2f1d9e4850c8c40
mkdir -p "$root/.cache"
if [[ ! -d "$root/.cache/storm" ]]; then
    mkdir "$root/.cache/storm"
    git -C "$engine_repo" archive "$revision" | tar -x -C "$root/.cache/storm"
fi
if git -C "$root/.cache/storm" apply --check "$root/native.patch" 2>/dev/null; then
    git -C "$root/.cache/storm" apply "$root/native.patch"
elif ! git -C "$root/.cache/storm" apply --reverse --check "$root/native.patch" 2>/dev/null; then
    echo "Native source differs from the pinned patch; preserve and reconcile it before building." >&2
    exit 1
fi
if git -C "$root/.cache/storm" apply --check "$root/compiler-extern.patch" 2>/dev/null; then
    git -C "$root/.cache/storm" apply "$root/compiler-extern.patch"
elif ! git -C "$root/.cache/storm" apply --reverse --check "$root/compiler-extern.patch" 2>/dev/null; then
    echo "Native compiler source differs from the extern-call patch; preserve and reconcile it before building." >&2
    exit 1
fi
if git -C "$root/.cache/storm" apply --check "$root/deck-walk.patch" 2>/dev/null; then
    git -C "$root/.cache/storm" apply "$root/deck-walk.patch"
elif ! git -C "$root/.cache/storm" apply --reverse --check "$root/deck-walk.patch" 2>/dev/null; then
    echo "Native deck source differs from the deck-walk patch; reconcile it before building." >&2
    exit 1
fi
if [[ ! -f "$root/.cache/sse2neon.h" ]]; then
    curl -fL 'https://raw.githubusercontent.com/DLTcollab/sse2neon/60fc9391e378b58c60899791c0e9ee9cdaf43c08/sse2neon.h' -o "$root/.cache/sse2neon.h"
fi
test "$(shasum -a 256 "$root/.cache/sse2neon.h" | cut -d ' ' -f 1)" = 0624a1270fc88a67e6b886cd0e33e322d93de4b3a760b3373dc3a66e713bd43f
if [[ ! -f "$root/.cache/stb_image.h" ]]; then
    curl -fL 'https://raw.githubusercontent.com/nothings/stb/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_image.h' -o "$root/.cache/stb_image.h"
fi
test "$(shasum -a 256 "$root/.cache/stb_image.h" | cut -d ' ' -f 1)" = 594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3
d3d9="$root/.cache/d3d9/warpowers-build/src/d3d9/libdxvk_d3d9.0.dylib"
if [[ ! -f "$root/.cache/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib" ]]; then
    "$root/build-sdl2.sh"
fi
if [[ ! -f "$d3d9" ]]; then
    "$root/build-d3d9.sh"
fi
cmake -S "$root" -B "$root/.cache/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DNATIVE_D3D9_LIBS="$d3d9" \
    -DSTORM_ENABLE_AUDIO=ON > "$root/.cache/configure.log" 2>&1
cmake --build "$root/.cache/build" --target engine -j 10 > "$root/.cache/build.log" 2>&1 || {
    rg 'error:|FAILED:|undefined|Undefined|ld:' "$root/.cache/build.log" >&2
    exit 1
}
file "$root/.cache/build/bin/engine-1"

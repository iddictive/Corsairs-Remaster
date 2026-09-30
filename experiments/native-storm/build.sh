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
patcher="$root/../native-metal/apply_source_patches.py"
patches=(
    "$root/native.patch"
    "$root/compiler-extern.patch"
    "$root/controls-telemetry.patch"
    "$root/gunfire-audio.patch"
    "$root/sailor-collision.patch"
    "$root/deck-walk.patch"
)
if [[ ! -f "$root/.cache/source-patches.json" ]]; then
    if ! python3 "$patcher" --source "$root/.cache/storm" \
        --state "$root/.cache/source-patches.json" "${patches[@]}"; then
        echo "Existing native source predates patch receipts. Migrate it explicitly:" >&2
        echo "  python3 $root/migrate_source_patches.py" >&2
        exit 1
    fi
else
    python3 "$patcher" --source "$root/.cache/storm" \
        --state "$root/.cache/source-patches.json" "${patches[@]}"
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
python3 - "$root" "$revision" "${patches[@]}" <<'PY_RECEIPT'
import hashlib
import json
from pathlib import Path
import sys

root = Path(sys.argv[1])
engine = root / '.cache/build/bin/engine-1'
patches = [Path(path) for path in sys.argv[3:]]
receipt = {
    'version': 1,
    'engine_revision': sys.argv[2],
    'engine_sha256': hashlib.sha256(engine.read_bytes()).hexdigest(),
    'gameplay_patches': {
        patch.name: hashlib.sha256(patch.read_bytes()).hexdigest()
        for patch in patches
    },
}
target = root / '.cache/gameplay-build.json'
temporary = target.with_name(target.name + '.tmp')
temporary.write_text(json.dumps(receipt, indent=2) + '\n')
temporary.replace(target)
PY_RECEIPT

#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
game=${CORSAIRS_GAME_DIR:-/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL}
runtime="$root/.cache/runtime"
bundle="$root/.cache/CorsairsNative.app"
launch_installed() {
    [[ -x "$bundle/Contents/MacOS/native-engine" ]] || { echo "Installed game is missing; run run.sh --stage-only first." >&2; exit 1; }
    export STORM_USERDATA="$root/.cache/userdata"
    export DYLD_LIBRARY_PATH="$root/.cache/d3d9/sdl2-fixed/lib:$root/.cache/d3d9/warpowers-build/src/d3d9:$root/.cache/d3d9/sdk/MoltenVK/MoltenVK/MoltenVK/dynamic/dylib/macOS:/opt/homebrew/lib"
    export DXVK_CONFIG_FILE="$root/dxvk.conf"
    export DXVK_WSI_DRIVER=SDL2 DXVK_LOG_LEVEL=info MVK_CONFIG_LOG_LEVEL=1
    echo "Starting native Storm. Log: $root/.cache/launch.log"
    cd "$runtime"
    exec "$bundle/Contents/MacOS/native-engine" > "$root/.cache/launch.log" 2>&1
}
mode=${1:-launch}
[[ "$mode" == launch || "$mode" == --stage-only || "$mode" == --launch-installed ]] || {
    echo 'Usage: run.sh [--stage-only|--launch-installed]' >&2; exit 2;
}
if ps -axo comm= | rg -q '(engine\.exe|/(native|metal)-engine)$'; then
    echo "Close the running game before launching this native candidate." >&2
    exit 1
fi
if [[ "$mode" == --launch-installed ]]; then
    launch_installed
fi
"$root/build.sh"
"$root/build-icon.sh"
mkdir -p "$runtime" "$bundle/Contents/MacOS" "$bundle/Contents/Resources/resource"
for directory in PROGRAM RESOURCE; do
    if [[ ! -d "$runtime/$directory" ]]; then
        /bin/cp -cR "$game/$directory" "$runtime/"
    fi
done
if [[ ! -d "$runtime/SAVE" ]]; then
    save_source=$(cd "$game/SAVE" && pwd -P)
    /bin/cp -cR "$save_source" "$runtime/SAVE"
fi
for name in engine.ini options project.df; do
    if [[ ! -f "$runtime/$name" ]]; then
        /bin/cp -c "$game/$name" "$runtime/$name"
    fi
done
python3 - "$runtime/engine.ini" <<'PY'
from pathlib import Path
import re, sys
p = Path(sys.argv[1])
s = p.read_text()
s = re.sub(r'^adapter\s*=.*$', 'adapter = 0', s, flags=re.M)
s = re.sub(r'^full_screen\s*=.*$', 'full_screen = 1', s, flags=re.M)
p.write_text(s)
PY
/bin/cp "$root/Info.plist" "$bundle/Contents/Info.plist"
/bin/cp "$root/.cache/Corsairs.icns" "$bundle/Contents/Resources/Corsairs.icns"
/bin/cp "$root/launch-app.sh" "$bundle/Contents/MacOS/launch"
chmod +x "$bundle/Contents/MacOS/launch"
python3 "$root/deck_walk.py" check
/bin/cp -c "$root/.cache/build/bin/engine-1" "$bundle/Contents/MacOS/native-engine"
python3 "$root/deck_walk.py" apply
python3 "$root/../../tools/settings_visibility.py" apply "$runtime"
/bin/cp -cR "$root/.cache/storm/src/libs/shared_headers/include/shared" "$bundle/Contents/Resources/resource/"
if [[ "$mode" == --stage-only ]]; then
    echo "Staged $bundle"
    exit 0
fi
launch_installed

#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
native=$(cd "$root/../native-storm" && pwd)
inputs="$root/inputs"
baseline="$native/.cache/runtime"
dependency_root="$native/.cache"
if [[ -f "$inputs/manifest.json" ]]; then
    baseline="$inputs/gameplay"
    dependency_root="$inputs/native"
fi
runtime="$root/.cache/runtime"
bundle="$root/.cache/CorsairsMetal.app"
launch_installed() {
    [[ -x "$bundle/Contents/MacOS/metal-engine" ]] || { echo "Installed game is missing; run run.sh --stage-only first." >&2; exit 1; }
    export STORM_USERDATA="$root/.cache/userdata"
    export STORM_METAL_MODERN_EFFECTS="${STORM_METAL_MODERN_EFFECTS:-1}"
    export STORM_METAL_ISLAND_GEOMETRY="${STORM_METAL_ISLAND_GEOMETRY:-0}"
    export STORM_METAL_PROFILE="${STORM_METAL_PROFILE:-1}"
    export STORM_METAL_ROCKK2="${STORM_METAL_ROCKK2:-0}"
    export STORM_TRACE_DECK_WALK="${STORM_TRACE_DECK_WALK:-1}"
    export DYLD_LIBRARY_PATH="$dependency_root/d3d9/sdl2-fixed/lib"
    unset DXVK_CONFIG_FILE DXVK_WSI_DRIVER DXVK_LOG_LEVEL MVK_CONFIG_LOG_LEVEL
    printf 'Starting Metal candidate. Log: %s\n' "$root/.cache/launch.log"
    cd "$runtime"
    exec python3 "$root/../../tools/metal_graphics_settings.py" launch "$runtime" --binary "$bundle/Contents/MacOS/metal-engine" > "$root/.cache/launch.log" 2>&1
}
mode=${1:-launch}
[[ "$mode" == launch || "$mode" == --stage-only || "$mode" == --launch-installed || "$mode" == --settings-hmr || "$mode" == --dev || "$mode" == dev ]] || {
    echo 'Usage: run.sh [--dev|--stage-only|--launch-installed|--settings-hmr]' >&2; exit 2;
}
if [[ "$mode" == --dev || "$mode" == dev ]]; then
    shift || true
    exec python3 "$root/../../tools/dev_runtime.py" launch "$@"
fi
if [[ "$mode" == --settings-hmr ]]; then
    exec python3 "$root/settings_hmr.py" "$runtime"
fi
if ps -axo comm= | rg '(engine\.exe|/(native|metal)-engine|/engine-1)$' >/dev/null; then
    echo 'Close the running game before staging or launching the Metal candidate.' >&2
    exit 1
fi
if [[ "$mode" == --launch-installed ]]; then
    python3 "$root/background_alpha.py" apply "$runtime/PROGRAM/locations/init"
    launch_installed
fi
if [[ -f "$inputs/manifest.json" && ! -d "$runtime/RESOURCE/techniques" ]]; then
    python3 "$root/../../tools/restore_metal_runtime.py"
fi
"$root/build.sh"
mkdir -p "$runtime" "$bundle/Contents/MacOS" "$bundle/Contents/Resources/resource"
for directory in PROGRAM RESOURCE SAVE; do
    if [[ ! -d "$runtime/$directory" ]]; then
        /bin/cp -cR "$baseline/$directory" "$runtime/"
    fi
done
for technique in Rope.fx Vant.fx; do
    /bin/cp "$root/.cache/storm/src/techniques/ship/$technique" "$runtime/RESOURCE/techniques/ship/$technique"
done
# Manual-aim overlay techniques (ShipAimArc/ShipAimVolume) live in engine-source
# _dev/ship.fx; without this copy the overlay draws nothing (silent no-op).
/bin/cp "$root/.cache/storm/src/techniques/_dev/ship.fx" "$runtime/RESOURCE/techniques/_dev/ship.fx"
python3 - "$root" "$runtime" <<'SUN_GLOW_STAGE'
import hashlib
import sys
from pathlib import Path

root, runtime = map(Path, sys.argv[1:])
sys.path.insert(0, str(root.parents[1] / "tools"))
from runtime_script_patch import atomic_write

relative = Path("techniques/weather/SunGlow.fx")
source = root / ".cache/storm/src" / relative
target = runtime / "RESOURCE" / relative
backup = root / ".cache/material-originals" / relative
originals = {
    "326efdd05bdea7fd257b23126b3ffe64f9afbeb8eca51868f78dcf960e8a0f0c",
    "11abcfe7065d4f652c0204f32f048da1f1e24adf1cfe4085b3a3d3d76c5662cd",
}
expected = "014bbf13890f74450369f8b74269f5518356457fdbcee4be890374aeeb2b527e"
digest = lambda data: hashlib.sha256(data).hexdigest()
incoming, current = source.read_bytes(), target.read_bytes()
assert digest(incoming) == expected, "Unreviewed SunGlow source"
assert digest(current) in originals | {expected}, "SunGlow changed outside staging"
if backup.exists():
    assert digest(backup.read_bytes()) in originals, "SunGlow backup changed"
else:
    assert digest(current) in originals, "SunGlow original backup is missing"
    atomic_write(backup, current)
if current != incoming:
    atomic_write(target, incoming)
print("Sun glow: hash-verified depth-write correction")
SUN_GLOW_STAGE
python3 "$root/background_alpha.py" apply "$runtime/PROGRAM/locations/init"
python3 "$root/materials/stage.py" "$runtime" "$root/.cache/material-originals" "${STORM_METAL_MATERIALS:-0}"
python3 "$root/materials/rockk2-candidates/stage.py" "$runtime" "${STORM_METAL_ROCKK2:-1}"
for name in engine.ini options project.df; do
    if [[ ! -f "$runtime/$name" ]]; then
        /bin/cp -c "$baseline/$name" "$runtime/$name"
    fi
done
python3 - "$runtime/engine.ini" "$native/Info.plist" "$bundle/Contents/Info.plist" <<'PY'
import plistlib, re, sys
from pathlib import Path
ini = Path(sys.argv[1])
s = ini.read_text()
for key, value in [('msaa', 0)]:
    s, count = re.subn(r'^' + key + r'\s*=.*$', f'{key} = {value}', s, flags=re.M)
    assert count == 1, f'Expected one {key} setting'
ini.write_text(s)
p = plistlib.loads(Path(sys.argv[2]).read_bytes())
p.update(CFBundleExecutable='launch', CFBundleIdentifier='com.corsairs.metal-candidate', CFBundleName='Corsairs Metal')
Path(sys.argv[3]).write_bytes(plistlib.dumps(p))
PY
if [[ -f "$dependency_root/Corsairs.icns" ]]; then
    /bin/cp "$dependency_root/Corsairs.icns" "$bundle/Contents/Resources/Corsairs.icns"
fi
/bin/cp "$root/launch-app.sh" "$bundle/Contents/MacOS/launch"
chmod +x "$bundle/Contents/MacOS/launch"
/bin/cp -c "$root/.cache/build/bin/engine-1" "$bundle/Contents/MacOS/metal-engine"
python3 "$root/../../tools/patch_metal_portobello_ground.py" --apply-runtime "$runtime"
python3 "$root/../../tools/patch_metal_portobello_shore.py" --apply-runtime "$runtime"
python3 "$root/dynamic_town_shadow_probe.py"
python3 "$root/baked_static_shadow_caster_probe.py"
python3 "$root/branding/stage.py" "$runtime"
python3 "$root/../../tools/sync_metal_gameplay.py" check
python3 "$root/../../tools/sync_metal_gameplay.py" apply
python3 "$root/../../tools/metal_graphics_settings.py" initialize "$runtime"
/bin/cp -cR "$root/.cache/storm/src/libs/shared_headers/include/shared" "$bundle/Contents/Resources/resource/"
# Every stage also refreshes the played /Applications engine (skips when the
# game bundle is absent or already matches; refuses while the game runs).
"$root/install-engine.sh"
if [[ "$mode" == --stage-only ]]; then
    echo "Staged $bundle"
    exit 0
fi
launch_installed

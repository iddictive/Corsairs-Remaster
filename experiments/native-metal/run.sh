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
[[ "$mode" == launch || "$mode" == --stage-only || "$mode" == --launch-installed || "$mode" == --settings-hmr ]] || {
    echo 'Usage: run.sh [--stage-only|--launch-installed|--settings-hmr]' >&2; exit 2;
}
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
# Engine-source techniques are staged as one reviewed group. Keep unknown
# runtime edits instead of silently replacing them with a raw copy.
python3 - "$root" "$runtime" <<'TECHNIQUE_STAGE'
import sys
from pathlib import Path

root, runtime = map(Path, sys.argv[1:])
sys.path.insert(0, str(root.parents[1] / "tools"))
from delivery_state import DeliveryState, LEGACY_TECHNIQUES, digest, transact

changes = {}
state = DeliveryState(runtime)
for name, known in LEGACY_TECHNIQUES.items():
    relative = Path("techniques") / name
    source = root / ".cache/storm/src" / relative
    target = runtime / "RESOURCE" / relative
    backup = root / ".cache/material-originals" / relative
    if any(path.is_symlink() for path in (source, target, backup)):
        raise RuntimeError(f"Linked technique: {name}")
    incoming, current = source.read_bytes(), target.read_bytes()
    key = "RESOURCE/" + relative.as_posix()
    state.admit(key, current, incoming, known)
    state.track(key, incoming)
    if backup.exists() and digest(backup.read_bytes()) not in known:
        raise RuntimeError(f"Technique backup changed: {name}")
    if current != incoming and not backup.exists():
        if digest(current) not in known:
            backup = root / ".cache/native-resource-backups" / digest(current)
            if backup.exists() and backup.read_bytes() != current:
                raise RuntimeError(f"Technique snapshot changed: {name}")
        if not backup.exists():
            changes[backup] = (None, current)
    changes[target] = (current, incoming)
changes[state.receipt] = state.change()
transact(changes)
print("Engine techniques: reviewed runtime revisions staged")
TECHNIQUE_STAGE
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

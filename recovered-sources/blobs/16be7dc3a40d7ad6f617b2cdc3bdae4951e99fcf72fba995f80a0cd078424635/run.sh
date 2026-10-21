#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
native=$(cd "$root/../native-storm" && pwd)
runtime="$root/.cache/runtime"
bundle="$root/.cache/CorsairsMetal.app"
mode=${1:-launch}
[[ "$mode" == launch || "$mode" == --stage-only ]] || {
    echo 'Usage: run.sh [--stage-only]' >&2; exit 2;
}
if ps -axo comm= | rg -q '(engine\.exe|/(native|metal)-engine)$'; then
    echo 'Close the running game before staging or launching the Metal candidate.' >&2
    exit 1
fi
"$root/build.sh"
mkdir -p "$runtime" "$bundle/Contents/MacOS" "$bundle/Contents/Resources/resource"
for directory in PROGRAM RESOURCE SAVE; do
    if [[ ! -d "$runtime/$directory" ]]; then
        /bin/cp -cR "$native/.cache/runtime/$directory" "$runtime/"
    fi
done
for name in engine.ini options project.df; do
    if [[ ! -f "$runtime/$name" ]]; then
        /bin/cp -c "$native/.cache/runtime/$name" "$runtime/$name"
    fi
done
python3 - "$runtime/engine.ini" "$native/Info.plist" "$bundle/Contents/Info.plist" <<'PY'
import plistlib, re, sys
from pathlib import Path
ini = Path(sys.argv[1])
s = ini.read_text()
for key, value in [('full_screen', 1), ('msaa', 0)]:
    s, count = re.subn(r'^' + key + r'\s*=.*$', f'{key} = {value}', s, flags=re.M)
    assert count == 1, f'Expected one {key} setting'
ini.write_text(s)
p = plistlib.loads(Path(sys.argv[2]).read_bytes())
p.update(CFBundleExecutable='launch', CFBundleIdentifier='com.corsairs.metal-candidate', CFBundleName='Corsairs Metal')
Path(sys.argv[3]).write_bytes(plistlib.dumps(p))
PY
if [[ -f "$native/.cache/Corsairs.icns" ]]; then
    /bin/cp "$native/.cache/Corsairs.icns" "$bundle/Contents/Resources/Corsairs.icns"
fi
cat > "$bundle/Contents/MacOS/launch" <<'SH'
#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../../../.." && pwd)
exec "$root/run.sh"
SH
chmod +x "$bundle/Contents/MacOS/launch"
/bin/cp -c "$root/.cache/build/bin/engine-1" "$bundle/Contents/MacOS/metal-engine"
/bin/cp -cR "$root/.cache/storm/src/libs/shared_headers/include/shared" "$bundle/Contents/Resources/resource/"
if [[ "$mode" == --stage-only ]]; then
    echo "Staged $bundle"
    exit 0
fi
export STORM_USERDATA="$root/.cache/userdata"
export STORM_METAL_MODERN_WATER="${STORM_METAL_MODERN_WATER:-1}"
export STORM_METAL_MODERN_EFFECTS="${STORM_METAL_MODERN_EFFECTS:-1}"
export DYLD_LIBRARY_PATH="$native/.cache/d3d9/sdl2-fixed/lib:/opt/homebrew/lib"
unset DXVK_CONFIG_FILE DXVK_WSI_DRIVER DXVK_LOG_LEVEL MVK_CONFIG_LOG_LEVEL
printf 'Starting Metal candidate. Log: %s\n' "$root/.cache/launch.log"
cd "$runtime"
exec "$bundle/Contents/MacOS/metal-engine" > "$root/.cache/launch.log" 2>&1

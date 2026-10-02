#!/bin/bash
# Installs the staged Metal engine into the played /Applications bundle.
# Same binary contract as tools/export_metal_app.py: machine-local rpaths
# stripped, bundle Frameworks rpath wired, ad-hoc codesigned. Keeps one
# backup per installed hash for instant rollback. Safe to run on every
# stage: skips when the installed engine already matches the candidate.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
staged="$root/.cache/build/bin/engine-1"
dest_app="/Applications/Corsairs Iddictive Remaster.app"
dest="$dest_app/Contents/MacOS/metal-engine"
backup_dir="$root/.cache/installed-engine-backup"
if ps -axo comm= | rg '(engine\.exe|/(native|metal)-engine|/engine-1)$' >/dev/null; then
    echo 'Close the running game before installing the Metal engine.' >&2
    exit 1
fi
if [[ ! -f "$staged" ]]; then
    echo 'Staged engine missing; run run.sh --stage-only first.' >&2
    exit 1
fi
if [[ ! -x "$dest" ]]; then
    echo "Played app missing at $dest; install skipped (staged app still updated)."
    exit 0
fi
tmp=$(mktemp "$backup_dir/.candidate.XXXXXX" 2>/dev/null || { mkdir -p "$backup_dir"; mktemp "$backup_dir/.candidate.XXXXXX"; })
trap 'rm -f "$tmp"' EXIT
cp -c "$staged" "$tmp"
for rp in $(otool -l "$tmp" | awk '/ path /{print $2}' | grep -E '^/(opt/homebrew|usr/local|Users/)' || true); do
    install_name_tool -delete_rpath "$rp" "$tmp"
done
if ! otool -l "$tmp" | grep -q '@executable_path/../Frameworks'; then
    install_name_tool -add_rpath "@executable_path/../Frameworks" "$tmp"
fi
if grep -q -a -E '/Users/|05_Repo/Corsairs' "$tmp"; then
    rm -f "$tmp"
    echo 'Staged engine leaks machine paths; rebuild with -ffile-prefix-map.' >&2
    exit 1
fi
# Compare the installable, signed bytes, not an unsigned temporary with a
# different hash on every run. The final executable keeps a stable identity.
codesign --force -s - --identifier metal-engine "$tmp"
candidate_hash=$(shasum -a 256 "$tmp" | awk '{print $1}')
installed_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
mkdir -p "$backup_dir"
if [[ ! -f "$backup_dir/metal-engine.$installed_hash" ]]; then
    cp -c "$dest" "$backup_dir/metal-engine.$installed_hash"
    echo "Backed up installed engine $installed_hash"
fi
# Validate every resource before touching the installed app. Unknown local edits
# must be reviewed, even when the development runtime has accepted its own copy.
python3 - "$root" "$dest_app" "$tmp" "$dest" <<'INSTALL'
from pathlib import Path
import hashlib
import subprocess
import sys

root, app, candidate, engine = map(Path, sys.argv[1:])
sys.path.insert(0, str(root.parents[1] / "tools"))
from runtime_script_patch import atomic_write

originals = {
    "ship/Rope.fx": {"5f0f0bef056f652515f2ef2b4b99b16f936ce30b5dd1f0417a13eec17ddbb9e1", "873feea8d12addfef10b0c6dabf106b93e88fc0574c54fb2b14e544eff887e69"},
    "ship/Vant.fx": {"00a92cad48c211d0465fccc19f2481a29310d94676b0988700357eefe566d65f"},
    "_dev/ship.fx": {"e868be15c45b8b2d91489b939933b0c420669f241ac6ea5cde1c9e04465fd8e9", "1629e130e608483aa5ab640d8d429d3d882810889306875ad3d9e515f87b46f9"},
    "weather/SunGlow.fx": {"326efdd05bdea7fd257b23126b3ffe64f9afbeb8eca51868f78dcf960e8a0f0c", "11abcfe7065d4f652c0204f32f048da1f1e24adf1cfe4085b3a3d3d76c5662cd", "014bbf13890f74450369f8b74269f5518356457fdbcee4be890374aeeb2b527e"},
}
digest = lambda data: hashlib.sha256(data).hexdigest()
changes = {}
for name, known in originals.items():
    source = root / ".cache/runtime/RESOURCE/techniques" / name
    target = app / "Contents/Resources/RESOURCE/techniques" / name
    if source.is_symlink() or target.is_symlink() or not source.is_file() or not target.is_file():
        raise RuntimeError(f"Missing or linked technique: {name}")
    before, after = target.read_bytes(), source.read_bytes()
    if after != (root / ".cache/storm/src/techniques" / name).read_bytes():
        raise RuntimeError(f"Technique does not match the built source: {name}")
    if before != after and digest(before) not in known:
        raise RuntimeError(f"Installed technique changed outside staging: {name}")
    if before != after:
        changes[target] = (before, after)
before, after = engine.read_bytes(), candidate.read_bytes()
if before != after:
    changes[engine] = (before, after)
if not changes:
    print("Played app engine and techniques already match; install skipped.")
else:
    written = []
    try:
        for target, (before, after) in changes.items():
            if target.read_bytes() != before:
                raise RuntimeError(f"Concurrent installed-app change: {target}")
            atomic_write(target, after)
            written.append(target)
        # The candidate executable is already signed. Seal only the outer
        # bundle so signing cannot rewrite nested files outside this transaction.
        subprocess.run(["codesign", "--force", "-s", "-", str(app)], check=True)
        subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True)
    except BaseException as error:
        failures = []
        for target in reversed(written):
            try:
                if target.read_bytes() != changes[target][1]:
                    raise RuntimeError(f"Concurrent change prevents installed rollback: {target}")
                atomic_write(target, changes[target][0])
            except (OSError, RuntimeError) as rollback_error:
                failures.append(str(rollback_error))
        if written and not failures:
            result = subprocess.run(["codesign", "--force", "-s", "-", str(app)], check=False)
            if result.returncode:
                failures.append("could not restore the installed bundle signature")
        if failures:
            raise RuntimeError(f"{error}; installed rollback incomplete: {'; '.join(failures)}") from error
        raise
    print("Installed engine and reviewed techniques; bundle signature verified.")
INSTALL
rm -f "$tmp"
final_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
rev=$(git -C "$root/../.." rev-parse --short HEAD 2>/dev/null || echo unknown)
echo "$final_hash  installed from $staged (commit $rev; signed candidate $candidate_hash)"
echo "Played app engine: $final_hash at $dest_app"

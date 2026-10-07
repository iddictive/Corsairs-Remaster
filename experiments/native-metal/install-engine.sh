#!/bin/bash
# Installs the staged Metal engine into the played /Applications bundle.
# Same binary contract as tools/export_metal_app.py: machine-local rpaths
# stripped, bundle Frameworks rpath wired, ad-hoc codesigned. Keeps one
# backup per installed hash for instant rollback. Safe to run on every
# stage: skips when the installed engine already matches the candidate.
# Also syncs the reviewed public launcher resource (the player log redirect
# owner) so a staged launcher change reaches the played app with the engine.
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
import subprocess
import sys

root, app, candidate, engine = map(Path, sys.argv[1:])
sys.path.insert(0, str(root.parents[1] / "tools"))
from delivery_state import DeliveryState, ENGINE, LEGACY_TECHNIQUES, digest, player_guard, transact

changes = {}
state = DeliveryState(app / "Contents/Resources", app)
runtime_state = DeliveryState(root / ".cache/runtime")
# Shared messages are compiled by the script VM as well as the engine. Deliver
# the built header to both consumers; stale commands abort startup compilation.
header_source = root / ".cache/storm/src/libs/shared_headers/include/shared/messages.h"
header_known = {
    "a3904843de09a05cafd0bb4bb78766953747bdcd33175fb1bdfdc3171444dc69",
    "38bab60eeeb5cb90370f3ea4c51b9be5169555ac09df74e15debd630257125a2",
}
if header_source.is_symlink() or not header_source.is_file():
    raise RuntimeError("Missing or linked built messages.h")
header_after = header_source.read_bytes()
for target in (
    app / "Contents/Resources/resource/shared/messages.h",
    root / ".cache/runtime/resource/shared/messages.h",
):
    if target.is_symlink() or not target.is_file():
        raise RuntimeError(f"Missing or linked script header: {target}")
    before = target.read_bytes()
    owner = state if target.is_relative_to(app) else runtime_state
    owner.admit("resource/shared/messages.h", before, header_after, header_known)
    owner.track("resource/shared/messages.h", header_after)
    if before != header_after:
        changes[target] = (before, header_after)
for name, known in LEGACY_TECHNIQUES.items():
    source = root / ".cache/runtime/RESOURCE/techniques" / name
    target = app / "Contents/Resources/RESOURCE/techniques" / name
    if source.is_symlink() or target.is_symlink() or not source.is_file() or not target.is_file():
        raise RuntimeError(f"Missing or linked technique: {name}")
    before, after = target.read_bytes(), source.read_bytes()
    if after != (root / ".cache/storm/src/techniques" / name).read_bytes():
        raise RuntimeError(f"Technique does not match the built source: {name}")
    relative = "RESOURCE/techniques/" + name
    state.admit(relative, before, after, known)
    state.track(relative, after)
    if before != after:
        changes[target] = (before, after)
# The public launcher owns the player log redirect, so a stale copy keeps
# unbounded logging. Sync it like a technique: reviewed revisions only.
launcher_source = root / "public_launcher.py"
launcher_target = app / "Contents/Resources/public_launcher.py"
# Reviewed launcher revisions: the pre-rotation head and this bounded one.
launcher_known = {
    "89aa9a965743798983d0b06e69f585edab0961c9a5fdbff265861b32e94e32f8",
    "fdeca54d99f34062f7bc858e7ab60066743183df266fd076e90e29a215e761fd",
}
for path in (launcher_source, launcher_target):
    if path.is_symlink() or not path.is_file():
        raise RuntimeError(f"Missing or linked launcher: {path}")
before, after = launcher_target.read_bytes(), launcher_source.read_bytes()
state.admit("public_launcher.py", before, after, launcher_known)
state.track("public_launcher.py", after)
if before != after:
    changes[launcher_target] = (before, after)
before, after = engine.read_bytes(), candidate.read_bytes()
# Existing installations predate receipts. Preserve their signed-engine
# bootstrap contract; subsequent replacements require the recorded bytes.
if ENGINE not in state.files:
    subprocess.run(["codesign", "--verify", "--strict", str(engine)], check=True)
state.admit(ENGINE, before, after, {digest(before)})
state.track(ENGINE, after)
if before != after:
    changes[engine] = (before, after)
changes[state.receipt] = state.change()
changes[runtime_state.receipt] = runtime_state.change()
with player_guard(app):
    updated = transact(changes, app)
if not updated:
    print("Played app engine, script header, techniques and launcher already match; install skipped.")
else:
    print("Engine resource delivery completed; ownership receipts updated.")
INSTALL
rm -f "$tmp"
final_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
rev=$(git -C "$root/../.." rev-parse --short HEAD 2>/dev/null || echo unknown)
echo "$final_hash  installed from $staged (commit $rev; signed candidate $candidate_hash)"
echo "Played app engine: $final_hash at $dest_app"

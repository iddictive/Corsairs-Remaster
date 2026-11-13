#!/bin/bash
# Installs the staged Metal engine into the played /Applications bundle.
# Same binary contract as tools/export_metal_app.py: machine-local rpaths
# stripped, bundle Frameworks rpath wired, ad-hoc codesigned. Keeps one
# prior-engine backup for instant rollback. Safe to run on every
# stage: skips when the installed engine already matches the candidate.
# Also syncs the reviewed public launcher and graphics adapter so their
# staged changes reach the played app even when the engine is unchanged.
set -euo pipefail
usage='Usage: install-engine.sh [--wait SECONDS] [--seamless | --verified-engine FILE SHA256 PREDECESSOR_SHA256 MANIFEST MANIFEST_SHA256]'
wait_seconds=0
if [[ "${1:-}" == "--help" && "$#" == 1 ]]; then
    printf '%s\n' "$usage"
    exit 0
fi
if [[ "${1:-}" == "--wait" ]]; then
    if [[ ! "${2:-}" =~ ^[0-9]+$ ]]; then
        printf '%s\n' "$usage" >&2
        exit 2
    fi
    wait_seconds=$2
    shift 2
fi
seamless=0
verified=0
if [[ "${1:-}" == "--seamless" && "$#" == 1 ]]; then
    seamless=1
elif [[ "${1:-}" == "--verified-engine" && "$#" == 6 ]]; then
    verified=1
    verified_path=$2
    verified_hash=$3
    verified_predecessor=$4
    verified_manifest=$5
    verified_manifest_hash=$6
    [[ "$verified_hash" =~ ^[0-9a-f]{64}$ && "$verified_predecessor" =~ ^[0-9a-f]{64}$ && "$verified_manifest_hash" =~ ^[0-9a-f]{64}$ ]] || {
        echo 'Verified delivery requires lowercase SHA-256 hashes.' >&2; exit 2;
    }
elif [[ "$#" != 0 ]]; then
    printf '%s\n' "$usage" >&2
    exit 2
fi
root=$(cd "$(dirname "$0")" && pwd)
staged="$root/.cache/build/bin/engine-1"
if [[ "$verified" == 1 ]]; then
    staged=$verified_path
fi
dest_app="/Applications/Corsairs Iddictive Remaster.app"
dest="$dest_app/Contents/MacOS/metal-engine"
backup_dir="$root/.cache/installed-engine-backup"
python3 "$root/../../tools/delivery_state.py" --wait-for-idle "$wait_seconds"
if [[ ! -f "$staged" || ( "$verified" == 1 && -L "$staged" ) ]]; then
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
if [[ "$verified" == 1 ]]; then
    [[ "$(shasum -a 256 "$tmp" | awk '{print $1}')" == "$verified_hash" ]] || {
        echo 'Verified engine hash mismatch; no app files changed.' >&2; exit 1;
    }
    codesign --verify --strict "$tmp"
    if otool -l "$tmp" | awk '/ path /{print $2}' | grep -E '^/(opt/homebrew|usr/local|Users/)' >/dev/null ||
       ! otool -l "$tmp" | grep -q '@executable_path/../Frameworks'; then
        echo 'Verified engine has incompatible runtime paths; no app files changed.' >&2; exit 1
    fi
else
    for rp in $(otool -l "$tmp" | awk '/ path /{print $2}' | grep -E '^/(opt/homebrew|usr/local|Users/)' || true); do
        install_name_tool -delete_rpath "$rp" "$tmp"
    done
    if ! otool -l "$tmp" | grep -q '@executable_path/../Frameworks'; then
        install_name_tool -add_rpath "@executable_path/../Frameworks" "$tmp"
    fi
fi
if grep -q -a -E '/Users/|05_Repo/Corsairs' "$tmp"; then
    rm -f "$tmp"
    echo 'Staged engine leaks machine paths; rebuild with -ffile-prefix-map.' >&2
    exit 1
fi
# Compare the installable, signed bytes, not an unsigned temporary with a
# different hash on every run. The final executable keeps a stable identity.
if [[ "$verified" == 0 ]]; then
    codesign --force -s - --identifier metal-engine "$tmp"
fi
candidate_hash=$(shasum -a 256 "$tmp" | awk '{print $1}')
installed_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
if [[ "$verified" == 1 && "$installed_hash" != "$verified_predecessor" ]]; then
    echo 'Installed predecessor changed; no app files changed.' >&2; exit 1
fi
if [[ "$verified" == 1 && ( -L "$backup_dir" || -L "$backup_dir/metal-engine.$installed_hash" ) ]]; then
    echo 'Linked engine backup is not allowed; no app files changed.' >&2; exit 1
fi
mkdir -p "$backup_dir"
if [[ ! -f "$backup_dir/metal-engine.$installed_hash" ]]; then
    cp -c "$dest" "$backup_dir/metal-engine.$installed_hash"
    echo "Backed up installed engine $installed_hash"
fi
if [[ "$verified" == 1 && "$(shasum -a 256 "$backup_dir/metal-engine.$installed_hash" | awk '{print $1}')" != "$installed_hash" ]]; then
    echo 'Installed engine backup hash mismatch; no app files changed.' >&2; exit 1
fi
# Validate every resource before touching the installed app. Unknown local edits
# must be reviewed, even when the development runtime has accepted its own copy.
python3 - "$root" "$dest_app" "$tmp" "$dest" "$seamless" "$verified" "${verified_predecessor:-}" "${verified_manifest:-}" "${verified_manifest_hash:-}" <<'INSTALL'
import json
from pathlib import Path
import re
import subprocess
import sys

root, app, candidate, engine = map(Path, sys.argv[1:5])
sys.path.insert(0, str(root.parents[1] / "tools"))
from delivery_state import DeliveryState, ENGINE, LEGACY_SHARED_HEADERS, LEGACY_TECHNIQUES, digest, player_guard, safe_path, transact
import gameplay_sources

changes = {}
gameplay = {}
assets = {}
state = DeliveryState(app / "Contents/Resources", app)
runtime_state = DeliveryState(root / ".cache/runtime")
if sys.argv[6] == "1":
    manifest_path = Path(sys.argv[8]).absolute()
    safe_path(manifest_path.parent, Path(manifest_path.name))
    manifest_bytes = manifest_path.read_bytes()
    if digest(manifest_bytes) != sys.argv[9]:
        raise RuntimeError("Verified delivery manifest hash mismatch; no app files changed.")
    manifest = json.loads(manifest_bytes)
    base_keys = {"version", "candidate_sha256", "predecessor_sha256", "resources"}
    if (not isinstance(manifest, dict) or type(manifest.get("version")) is not int
            or manifest["version"] not in (1, 2, 3)
            or set(manifest) != (base_keys if manifest["version"] == 1 else
                                 base_keys | {"gameplay"} if manifest["version"] == 2 else
                                 base_keys | {"gameplay", "assets"})
            or manifest["candidate_sha256"] != digest(candidate.read_bytes())
            or manifest["predecessor_sha256"] != sys.argv[7] or not isinstance(manifest["resources"], dict)
            or not manifest["resources"]):
        raise RuntimeError("Invalid verified delivery manifest; no app files changed.")
    changes[manifest_path] = (manifest_bytes, manifest_bytes)
    for name, expected in manifest["resources"].items():
        if (not isinstance(name, str) or Path(name).as_posix() != name or not name.startswith(("RESOURCE/", "PROGRAM/", "resource/shared/"))
                or not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected)):
            raise RuntimeError("Invalid verified delivery resource identity; no app files changed.")
        path = safe_path(app / "Contents/Resources", Path(name))
        current = path.read_bytes()
        if digest(current) != expected:
            raise RuntimeError(f"Verified delivery resource changed: {name}; no app files changed.")
        changes[path] = (current, current)
    if manifest["version"] >= 2:
        gameplay = manifest["gameplay"]
        if not isinstance(gameplay, dict) or not gameplay:
            raise RuntimeError("Invalid verified gameplay selection; no files changed.")
        for name, expected in gameplay.items():
            if (not isinstance(name, str) or not isinstance(expected, dict)
                    or set(expected) != {"source_sha256", "runtime_sha256"}
                    or any(not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value)
                           for value in expected.values())):
                raise RuntimeError("Invalid verified gameplay identity; no files changed.")
    if manifest["version"] == 3:
        assets = manifest["assets"]
        if not isinstance(assets, dict) or not assets:
            raise RuntimeError("Invalid verified asset selection; no files changed.")
        for name, expected in assets.items():
            if (not isinstance(name, str) or not isinstance(expected, dict)
                    or set(expected) != {"source_sha256", "sha256"}
                    or any(not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value)
                           for value in expected.values())):
                raise RuntimeError("Invalid verified asset identity; no files changed.")
# Shared messages are compiled by the script VM as well as the engine. Deliver
# the built header to both consumers; stale commands abort startup compilation.
for name, known in LEGACY_SHARED_HEADERS.items():
    header_source = root / "engine/src/libs/shared_headers/include/shared" / name
    if header_source.is_symlink() or not header_source.is_file():
        raise RuntimeError(f"Missing or linked built header: {name}")
    header_after = header_source.read_bytes()
    relative = "resource/shared/" + name
    for target in (app / "Contents/Resources" / relative, root / ".cache/runtime" / relative):
        if target.is_symlink() or not target.is_file():
            raise RuntimeError(f"Missing or linked script header: {target}")
        before = target.read_bytes()
        owner = state if target.is_relative_to(app) else runtime_state
        owner.admit(relative, before, header_after, known)
        owner.track(relative, header_after)
        if before != header_after:
            changes[target] = (before, header_after)
for name, known in LEGACY_TECHNIQUES.items():
    source = root / "engine/src/techniques" / name
    target = app / "Contents/Resources/RESOURCE/techniques" / name
    if source.is_symlink() or target.is_symlink() or not source.is_file() or not target.is_file():
        raise RuntimeError(f"Missing or linked technique: {name}")
    before, after = target.read_bytes(), source.read_bytes()
    relative = "RESOURCE/techniques/" + name
    state.admit(relative, before, after, known)
    state.track(relative, after)
    if before != after:
        changes[target] = (before, after)
# The public launcher imports the graphics adapter on each normal app start.
# Deliver both through the same transaction, retaining reviewed bootstrap
# bytes and source snapshots rather than relying on a fresh public export.
repo = root.parents[1]
launch_helpers = {
    "public_launcher.py": (root / "public_launcher.py", {
        "89aa9a965743798983d0b06e69f585edab0961c9a5fdbff265861b32e94e32f8",
        "fdeca54d99f34062f7bc858e7ab60066743183df266fd076e90e29a215e761fd",
    }),
    "metal_graphics_settings.py": (repo / "tools/metal_graphics_settings.py", {
        "3474827dcd0a911e4efbee8ce3b9db4f4ab8f51b8bb4e503d49aa25fdb16f4ea",
    }),
}
for relative, (source, known) in launch_helpers.items():
    target = app / "Contents/Resources" / relative
    for path in (source, target):
        if path.is_symlink() or not path.is_file():
            raise RuntimeError(f"Missing or linked launch helper: {path}")
    before, after = target.read_bytes(), source.read_bytes()
    state.admit(relative, before, after, known)
    state.track(relative, after, source=source.relative_to(repo).as_posix())
    changes[source] = (after, after)
    if before != after:
        changes[target] = (before, after)
before, after = engine.read_bytes(), candidate.read_bytes()
if sys.argv[6] == "1" and digest(before) != sys.argv[7]:
    raise RuntimeError("Installed predecessor changed; no app files changed.")
# Existing installations predate receipts. Preserve their signed-engine
# bootstrap contract; subsequent replacements require the recorded bytes.
if ENGINE not in state.files:
    subprocess.run(["codesign", "--verify", "--strict", str(engine)], check=True)
state.admit(ENGINE, before, after, {digest(before)})
state.track(ENGINE, after)
if before != after:
    changes[engine] = (before, after)
if sys.argv[5] == "1":
    from seamless_sea_delivery import prepare
    changes.update(prepare(state, root / ".cache/build/bin/engine-1"))
gameplay_targets = set()
if gameplay:
    source_set = gameplay_sources.read(gameplay.keys())
    for name, expected in gameplay.items():
        source = safe_path(gameplay_sources.ROOT, Path(name))
        if (digest(source_set[1][source][0]) != expected["source_sha256"]
                or digest(source_set[0][name][0]) != expected["runtime_sha256"]):
            raise RuntimeError(f"Verified gameplay source changed: {name}; no files changed.")
    for target, owner in ((app / "Contents/Resources", state), (root / ".cache/runtime", runtime_state)):
        selected = gameplay_sources.prepare_delivery(target, source_set, owner)
        for path, value in selected.items():
            if path in changes and changes[path] != value:
                # A manifest resource is frozen above, then explicitly replaced
                # here by the same verified gameplay input.
                if (not path.is_relative_to(target) or path.relative_to(target).as_posix() not in gameplay
                        or changes[path] != (value[0], value[0])):
                    raise RuntimeError(f"Conflicting verified delivery input: {path}; no files changed.")
            changes[path] = value
        gameplay_targets.update(target / name for name in gameplay)
if assets:
    import sync_metal_gameplay
    for target, owner in ((app / "Contents/Resources", state), (root / ".cache/runtime", runtime_state)):
        managed, inputs = {}, {}
        sync_metal_gameplay.verified_asset_changes(owner, assets, managed, inputs)
        for path, value in inputs.items():
            if path in changes and changes[path] != value:
                raise RuntimeError(f"Concurrent verified asset input: {path}; no files changed.")
        changes.update(inputs)
        for name, (before, incoming) in managed.items():
            path = target / name
            if path in changes and changes[path] != (before, before):
                raise RuntimeError(f"Conflicting verified asset input: {path}; no files changed.")
            changes.update(gameplay_sources.backup_change(before, incoming))
            changes[path] = (before, incoming)
            gameplay_targets.add(path)
changes[state.receipt] = state.change()
changes[runtime_state.receipt] = runtime_state.change()
if sys.argv[6] == "1":
    permitted = {engine, state.receipt, runtime_state.receipt} | gameplay_targets
    permitted.update(path for path in changes if path.is_relative_to(gameplay_sources.BACKUPS))
    unexpected = [str(path) for path, (old, new) in changes.items() if old != new and path not in permitted]
    if unexpected:
        raise RuntimeError("Verified delivery would change unrelated files: " + ", ".join(unexpected))
with player_guard(app):
    updated = transact(changes, app)
if not updated:
    print("Played app engine, script header, techniques and launch helpers already match; install skipped.")
else:
    print("Engine resource delivery completed; ownership receipts updated.")
INSTALL
rm -f "$tmp"
final_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
if [[ "$verified" == 0 ]]; then
    python3 "$root/../../tools/metal_storage.py" --installed-hash "$final_hash"
fi
rev=$(git -C "$root/../.." rev-parse --short HEAD 2>/dev/null || echo unknown)
echo "$final_hash  installed from $staged (commit $rev; signed candidate $candidate_hash)"
echo "Played app engine: $final_hash at $dest_app"

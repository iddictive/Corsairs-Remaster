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
candidate_hash=$(shasum -a 256 "$tmp" | awk '{print $1}')
installed_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
if [[ "$candidate_hash" == "$installed_hash" ]]; then
    rm -f "$tmp"
    echo "Played app already runs $installed_hash; install skipped."
    exit 0
fi
mkdir -p "$backup_dir"
if [[ ! -f "$backup_dir/metal-engine.$installed_hash" ]]; then
    cp -c "$dest" "$backup_dir/metal-engine.$installed_hash"
    echo "Backed up installed engine $installed_hash"
fi
cp -c "$tmp" "$dest"
rm -f "$tmp"
codesign --force -s - "$dest"
codesign --force --deep -s - "$dest_app" 2>/dev/null || true
final_hash=$(shasum -a 256 "$dest" | awk '{print $1}')
rev=$(git -C "$root/../.." rev-parse --short HEAD 2>/dev/null || echo unknown)
echo "$final_hash  installed from $staged (commit $rev; pre-codesign $candidate_hash)"
echo "Installed Metal engine $final_hash to $dest_app"

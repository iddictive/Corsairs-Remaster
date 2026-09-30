#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
source_icon="$root/.cache/storm/src/apps/engine/rsrc/icon1.ico"
iconset="$root/.cache/Corsairs.iconset"
fingerprint=$(shasum -a 256 "$source_icon" "$0" | shasum -a 256 | cut -d ' ' -f 1)
if [[ -f "$root/.cache/Corsairs.icns" && -f "$root/.cache/icon.sha256" && "$(cat "$root/.cache/icon.sha256")" == "$fingerprint" ]]; then
    exit 0
fi
mkdir -p "$iconset"
sips -s format png "$source_icon" --out "$root/.cache/icon-source.png" >/dev/null
for size in 16 32 128 256 512; do
    sips -z "$size" "$size" "$root/.cache/icon-source.png" --out "$iconset/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    sips -z "$double" "$double" "$root/.cache/icon-source.png" --out "$iconset/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$iconset" -o "$root/.cache/Corsairs.icns"
printf '%s\n' "$fingerprint" > "$root/.cache/icon.sha256"

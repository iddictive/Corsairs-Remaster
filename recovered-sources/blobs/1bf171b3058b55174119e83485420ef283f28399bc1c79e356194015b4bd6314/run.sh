#!/bin/zsh
set -euo pipefail
probe_root="${0:A:h}"
game_root="${CORSAIRS_GAME_DIR:-${HOME}/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL}"
model_path="${1:-${game_root}/RESOURCE/MODELS/Ships/FrigateQueen1/FrigateQueen1.gm}"
build_root="${probe_root}/.build"
app_root="${build_root}/StormMetalProbe.app"
mkdir -p "${app_root}/Contents/MacOS"
cp "${probe_root}/Info.plist" "${app_root}/Contents/Info.plist"
binary="${app_root}/Contents/MacOS/metal-probe"
if [[ ! -x "${binary}" || "${probe_root}/probe.mm" -nt "${binary}" ]]; then
  xcrun clang++ -std=c++17 -fobjc-arc -framework Cocoa -framework Metal -framework MetalKit \
    "${probe_root}/probe.mm" -o "${binary}"
fi
if [[ "${1:-}" == "--harbor" ]]; then
  scene_root="${build_root}/harbor"
  python3 "${probe_root}/compose.py" "${game_root}" "${scene_root}"
else
  scene_root="${build_root}/scene"
  python3 "${probe_root}/prepare.py" "${model_path}" "${game_root}/RESOURCE/Textures" "${scene_root}"
fi
exec "${binary}" "${scene_root}" "${probe_root}/scene.metal"

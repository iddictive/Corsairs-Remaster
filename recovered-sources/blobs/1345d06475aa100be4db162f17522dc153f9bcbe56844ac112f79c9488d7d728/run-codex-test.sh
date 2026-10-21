#!/bin/zsh

set -euo pipefail

crossover_app="${CROSSOVER_APP:-${HOME}/Applications/CrossOver.app}"
bottle="${CORSAIRS_BOTTLE:-GAMES}"
game_dir="${CORSAIRS_GAME_DIR:-${HOME}/Library/Application Support/CrossOver/Bottles/${bottle}/drive_c/Games/KVL-Codex-Test}"
wine="${crossover_app}/Contents/SharedSupport/CrossOver/bin/wine"
engine="${game_dir}/engine.exe"

if [[ ! -x "${wine}" ]]; then
  print -u2 "CrossOver Wine not found: ${wine}"
  exit 1
fi

if [[ ! -f "${engine}" ]]; then
  print -u2 "Test engine not found: ${engine}"
  exit 1
fi

exec "${wine}" \
  --bottle "${bottle}" \
  --no-update \
  --no-wait \
  --workdir "${game_dir}" \
  "${engine}"

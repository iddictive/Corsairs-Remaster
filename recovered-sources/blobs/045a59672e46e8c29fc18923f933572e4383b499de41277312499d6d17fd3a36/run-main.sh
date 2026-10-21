#!/bin/zsh

set -euo pipefail

script_dir="${0:A:h}"
export CORSAIRS_GAME_DIR="/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL"
export CORSAIRS_PEER_ENGINE="/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/KVL-Codex-Test/engine.exe"
export CORSAIRS_RUNTIME_LABEL="Primary"
export CORSAIRS_EXPECTED_BASENAME="KVL"
exec "${script_dir}/run-codex-test.sh"

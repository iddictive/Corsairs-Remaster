#!/bin/bash
# Fast dev-runtime launcher for Corsairs Metal.
# Skips engine recompilation and probe runs when the engine binary is ready.
# Auto-syncs gameplay/ workspace changes.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
exec python3 "$root/tools/dev_runtime.py" launch "$@"


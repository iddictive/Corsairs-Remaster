#!/bin/bash
# Installed remaster launcher. Content-only edits reuse its native engine.
# --build uses canonical staging and refuses active dev edits before mutation.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
exec python3 "$root/tools/dev_runtime.py" launch "$@"

#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
exec python3 "$root/../../tools/dev_mode.py" "$@"

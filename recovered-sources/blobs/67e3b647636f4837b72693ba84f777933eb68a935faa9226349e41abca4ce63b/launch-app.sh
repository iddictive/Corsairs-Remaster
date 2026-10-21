#!/bin/bash
set -euo pipefail
launcher_dir=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$launcher_dir/../../../.." && pwd)
exec "$root/run.sh"

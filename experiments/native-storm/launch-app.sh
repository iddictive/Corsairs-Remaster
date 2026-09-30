#!/bin/bash
set -euo pipefail
export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
launcher_dir=$(cd "$(dirname "$0")" && pwd -P)
root=$(cd "$launcher_dir/../../../.." && pwd)
exec "$root/run.sh" --launch-installed

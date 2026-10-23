"""Verify the actual logger factory with the ordinary launch's null streams.

Run from the repository root; TASK_SCRATCH_DIR must name an empty directory.
Uses existing native diagnostics library and its configured compiler includes.
No game, window, player data or renderer is created.
"""
import json
import os
from pathlib import Path
import shlex
import subprocess

root = Path.cwd()
work = Path(os.environ['TASK_SCRATCH_DIR'])
build = root / 'experiments/native-metal/.cache/build'
lines = (build / 'build.ninja').read_text().splitlines()
start = next(i for i, row in enumerate(lines) if 'diagnostics.dir/src/logging.cpp.o:' in row)
fields = {}
for line in lines[start + 1:]:
    if not line.strip():
        break
    if '=' in line:
        key, value = line.strip().split('=', 1)
        fields[key.strip()] = value.strip()
flags = shlex.split(fields['DEFINES']) + shlex.split(fields['INCLUDES'])
header = (root / 'experiments/native-metal/sea_map_transition.hpp').read_text()
factory = header[header.index('    static storm::logging::logger_ptr log()'):header.index('    void tracePhase(')]
unit = work / 'logging.cpp'
unit.write_text('#include "logging.hpp"\n#include <spdlog/spdlog.h>\n' + factory + r'''
int main() {
    spdlog::set_level(spdlog::level::off);
    const auto logger = log();
    if (!logger) return 7;
    logger->info("quiet-launch-canary cpu_ms=0.125");
    logger->flush();
    return 0;
}
''')
binary = work / 'logging'
sdl = root / 'experiments/native-metal/inputs/native/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib'
subprocess.run(['clang++', '-std=c++20', '-O2', '-mmacosx-version-min=15.0',
                *flags, str(unit), str(build / 'lib/libdiagnostics.a'), str(sdl),
                '-Wl,-rpath,' + str(sdl.parent),
                '-o', str(binary)], check=True, capture_output=True)
results = {}
for case in ('quiet', 'blocked'):
    data = work / case
    if case == 'blocked':
        data.write_text('fixture blocks directory creation')
    env = dict(os.environ, STORM_USERDATA=str(data), STORM_QUIET_CLIENT='1')
    result = subprocess.run([str(binary)], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    path = data / 'Logs/sea-map-transition.log'
    content = path.read_text() if path.is_file() else ''
    results[case] = {'exit': result.returncode, 'canary_written': 'quiet-launch-canary cpu_ms=0.125' in content}
print(json.dumps(results, indent=2))

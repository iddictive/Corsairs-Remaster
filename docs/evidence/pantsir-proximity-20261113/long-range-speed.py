#!/usr/bin/env python3
"""Sample source-derived trajectories; never starts the engine or game."""
import hashlib
import importlib.util
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('matrix', here / 'matrix.py')
matrix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matrix)
inputs = [matrix.ENGINE, matrix.SHIP / 'phonk_ordnance.hpp', matrix.SHIP / 'phonk_missile_profiles.hpp',
          matrix.MATH / 'c_vector.h', here / 'matrix.py', here / 'long-range-speed.cpp', pathlib.Path(__file__).resolve()]
def digest():
    return {str(p.relative_to(matrix.ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
before = digest()
runtime = matrix.ENGINE.read_text()
prefix = '#include <cstdint>\nusing entid_t=uint64_t;\n#include "phonk_ordnance.hpp"\n#include <cstdio>\nusing namespace storm::ship::phonk;\n'
code = prefix + matrix.function(runtime, 'shotVariation') + matrix.function(runtime, 'varyMissile') + (here / 'long-range-speed.cpp').read_text()
tmp = tempfile.mkdtemp(prefix='corsairs-long-speed-')
try:
    src, binary = pathlib.Path(tmp) / 'probe.cpp', pathlib.Path(tmp) / 'probe'
    src.write_text(code)
    subprocess.run(['xcrun', 'clang++', '-std=c++20', '-O2', '-I'+str(matrix.SHIP), '-I'+str(matrix.MATH), str(src), '-o', str(binary)], check=True)
    output = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
    if before != digest():
        raise RuntimeError('source changed during run; discard results')
    print('# 19000m launch, 8000m spherical detection, 300 synthetic serials, 10ms sampling')
    for path, sha in before.items(): print('#', sha, path)
    print('type,detected,min_speed_mps,mean_speed_mps,max_speed_mps,mean_detection_alt_m,mean_peak_alt_m,mean_speed_over_profile')
    print(output, end='')
finally:
    subprocess.run(['/Users/md/.codex/bin/codex-delete-temp', tmp], check=True, stdout=subprocess.DEVNULL)

#!/usr/bin/env python3
"""Compile the independent salvo experiment with functions from current source."""
import argparse
import hashlib
import importlib.util
import pathlib
import subprocess
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('matrix', HERE / 'matrix.py')
matrix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matrix)

parser = argparse.ArgumentParser()
parser.add_argument('--trials', type=int, default=300)
parser.add_argument('--seed', type=int, default=19)
parser.add_argument('--unlimited-charges', action='store_true', help='disable only the replenishing charge bucket')
parser.add_argument('--defense', choices=['random', 'min', 'max'], default='random')
args = parser.parse_args()
if args.trials < 1 or args.seed < 1:
    parser.error('trials and seed must be positive')
paths = [matrix.ENGINE, matrix.SHIP / 'phonk_ordnance.hpp', matrix.SHIP / 'phonk_missile_profiles.hpp',
         matrix.SHIP / 'phonk_air_intercept.hpp', matrix.SHIP / 'phonk_air_intercept_batch.hpp',
         matrix.MATH / 'c_vector.h', matrix.ROOT / 'src/gameplay/PROGRAM/sea_ai/sea_traffic.c',
         HERE / 'matrix.py', HERE / 'random-salvo.cpp', pathlib.Path(__file__).resolve()]
digest = lambda: {str(p.relative_to(matrix.ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
before = digest()
runtime = matrix.ENGINE.read_text()
assert 'GetAttributeAsDword("AirDefenseType", 2)' in runtime
names = ['turnMissileToward', 'interceptorProfile', 'patriotMotor', 'initializePatriot', 'advancePatriot',
         'shotVariation', 'interceptorFuseDelay', 'advanceDefenseReaction', 'defenseReactionReady',
         'interceptorAimSpread', 'interceptorAimError', 'interceptorTimingUncertainty', 'interceptorTrackingError',
         'interceptorEstimate', 'interceptorEstimateAccelerating', 'interceptorEstimateShtorm', 'observeShtorm',
         'observeBallistic', 'observedMissileAcceleration', 'interceptorEstimateCruise', 'estimatePantsir',
         'interceptorForwardSector', 'varyMissile', 'evadeInterceptor']
prefix = '#include "phonk_air_intercept_batch.hpp"\n#include <cstdint>\nusing entid_t=uint64_t;\n#include "phonk_ordnance.hpp"\n#include <limits>\n#include <cstdio>\n#include <random>\nusing namespace storm::ship::phonk;\nstruct SEA_BASE { float WaveXZ(float,float) {return 0.f;} };\nstruct InterceptorProfile { float planningSpeed, initialSpeed, boostAcceleration, sustainAcceleration, maxSpeed; };\n'
prefix += '#define TRIALS %du\n#define SEED %du\n' % (args.trials, args.seed)
prefix += '#define UNLIMITED_CHARGES %d\n' % args.unlimited_charges
prefix += '#define DEFENSE_EXTREME %d\n' % {'random': 0, 'min': 1, 'max': 2}[args.defense]
functions = [matrix.function(runtime, n) for n in names]
i = names.index('defenseReactionReady')
functions[i] = matrix.replace_once(functions[i], '.12f + .16f*shotVariation', '.05f + .20f*shotVariation')
code = prefix + ''.join(functions) + (HERE / 'random-salvo.cpp').read_text()
tmp = tempfile.mkdtemp(prefix='corsairs-random-salvo-')
try:
    source, binary = pathlib.Path(tmp) / 'driver.cpp', pathlib.Path(tmp) / 'driver'
    source.write_text(code)
    subprocess.run(['xcrun', 'clang++', '-std=c++20', '-O2', '-I'+str(matrix.SHIP), '-I'+str(matrix.MATH), str(source), '-o', str(binary)], check=True)
    output = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
    if before != digest():
        raise RuntimeError('input changed during run; discard output')
    print('# isolated model, seed=%d trials=%d unlimited_charges=%s defense=%s' % (args.seed, args.trials, args.unlimited_charges, args.defense))
    for path, sha in before.items():
        print('#', sha, path)
    print(output, end='')
finally:
    subprocess.run(['/Users/md/.codex/bin/codex-delete-temp', tmp], check=True, stdout=subprocess.DEVNULL)

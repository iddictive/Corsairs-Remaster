#!/usr/bin/env python3
"""Build and run isolated player-defense approximation against current helpers."""
import argparse
import hashlib
import importlib.util
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('matrix', here / 'matrix.py')
matrix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matrix)
p = argparse.ArgumentParser()
p.add_argument('--trials', type=int, default=300)
p.add_argument('--seed', type=int, default=19)
p.add_argument('--dt', type=float, default=.025)
p.add_argument('--custom-delays', action='store_true')
p.add_argument('--disabled', action='store_true')
args = p.parse_args()
if args.trials < 1 or args.seed < 1 or not .001 <= args.dt <= .05:
    p.error('invalid parameters')
paths = [matrix.ENGINE, matrix.SHIP / 'phonk_ordnance.hpp', matrix.SHIP / 'phonk_missile_profiles.hpp',
         matrix.SHIP / 'phonk_air_intercept.hpp', matrix.SHIP / 'phonk_air_intercept_batch.hpp',
         matrix.MATH / 'c_vector.h', here / 'matrix.py', here / 'player-simulation.cpp', pathlib.Path(__file__).resolve()]
digest = lambda: {str(x.relative_to(matrix.ROOT)): hashlib.sha256(x.read_bytes()).hexdigest() for x in paths}
before = digest()
runtime = matrix.ENGINE.read_text()
names = ['turnMissileToward', 'interceptorProfile', 'patriotMotor', 'initializePatriot', 'advancePatriot',
         'shotVariation', 'interceptorFuseDelay', 'advanceDefenseReaction', 'defenseReactionReady',
         'interceptorAimSpread', 'interceptorAimError', 'interceptorTimingUncertainty', 'interceptorTrackingError',
         'interceptorEstimate', 'interceptorEstimateAccelerating', 'interceptorEstimateShtorm', 'observeShtorm',
         'observeBallistic', 'observedMissileAcceleration', 'interceptorEstimateCruise', 'estimatePantsir',
         'varyMissile', 'evadeInterceptor']
functions = [matrix.function(runtime, name) for name in names]
if args.custom_delays:
    i = names.index('defenseReactionReady')
    functions[i] = matrix.replace_once(functions[i], '.12f + .16f*shotVariation', '.05f + .20f*shotVariation')
    i = names.index('interceptorFuseDelay')
    functions[i] = matrix.replace_once(functions[i], '.001f + .002f * shotVariation', '.002f + .005f * shotVariation')
prefix = '#include "phonk_air_intercept_batch.hpp"\n#include <cstdint>\nusing entid_t=uint64_t;\n#include "phonk_ordnance.hpp"\n#include <limits>\n#include <cstdio>\nusing namespace storm::ship::phonk;\nstruct SEA_BASE { float WaveXZ(float,float) {return 0.f;} };\nstruct InterceptorProfile { float planningSpeed, initialSpeed, boostAcceleration, sustainAcceleration, maxSpeed; };\n'
prefix += '#define TRIALS %du\n#define SEED %du\n#define DT %.8ff\n#define DISABLED %d\n' % (args.trials, args.seed, args.dt, args.disabled)
code = prefix + ''.join(functions) + (here / 'player-simulation.cpp').read_text()
tmp = tempfile.mkdtemp(prefix='corsairs-player-model-')
try:
    src, binary = pathlib.Path(tmp) / 'driver.cpp', pathlib.Path(tmp) / 'driver'
    src.write_text(code)
    subprocess.run(['xcrun', 'clang++', '-std=c++20', '-O2', '-I'+str(matrix.SHIP), '-I'+str(matrix.MATH), str(src), '-o', str(binary)], check=True)
    output = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
    if before != digest(): raise RuntimeError('source changed during run; discard output')
    print('# trials=%d seed=%d dt=%g custom_delays=%s disabled=%s' % (args.trials, args.seed, args.dt, args.custom_delays, args.disabled))
    for path, sha in before.items(): print('#', sha, path)
    print(output, end='')
finally:
    subprocess.run(['/Users/md/.codex/bin/codex-delete-temp', tmp], check=True, stdout=subprocess.DEVNULL)

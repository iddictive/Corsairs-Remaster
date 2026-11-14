#!/usr/bin/env python3
"""Run the isolated matrix against functions extracted from the current engine."""
import argparse
import hashlib
import pathlib
import re
import subprocess
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SHIP = ROOT / 'experiments/native-metal/engine/src/libs/ship/src'
MATH = ROOT / 'experiments/native-metal/engine/src/libs/math/include'
ENGINE = SHIP / 'phonk_ordnance.cpp'


def function(source, name):
    match = re.search(r'(?m)^(?:constexpr\s+)?(?:void|bool|float|CVECTOR|InterceptorProfile)\s+' + name + r'\s*\(', source)
    if not match:
        raise RuntimeError('runtime function missing: ' + name)
    start = match.start()
    brace = source.index('{', match.end())
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == '{':
            depth += 1
        elif source[pos] == '}':
            depth -= 1
            if depth == 0:
                return source[start:pos + 1] + '\n'
    raise RuntimeError('unclosed function: ' + name)


def replace_once(value, old, new):
    if value.count(old) != 1:
        raise RuntimeError('probe template changed: ' + old[:80])
    return value.replace(old, new)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--trials', type=int, default=500)
    parser.add_argument('--seed', type=int, default=1, help='first serial, 1..2^32-1')
    parser.add_argument('--fuse-max-ms', type=float, default=3., help='uniform fuse delay between 1 ms and this value')
    parser.add_argument('--fuse-min-ms', type=float, default=1.)
    parser.add_argument('--reaction-min-ms', type=float, default=120.)
    parser.add_argument('--reaction-max-ms', type=float, default=280.)
    args = parser.parse_args()
    if args.trials < 1 or args.seed < 1 or args.seed + args.trials >= 2**32 or not 0 <= args.fuse_min_ms <= args.fuse_max_ms <= 100 or not 0 <= args.reaction_min_ms <= args.reaction_max_ms <= 5000:
        parser.error('invalid trial count or seed range')
    inputs = [ENGINE, SHIP / 'phonk_ordnance.hpp', SHIP / 'phonk_missile_profiles.hpp',
              SHIP / 'phonk_air_intercept.hpp', SHIP / 'phonk_air_intercept_batch.hpp', MATH / 'c_vector.h',
              HERE / 'matrix_driver.cpp', pathlib.Path(__file__).resolve()]
    before = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    runtime = ENGINE.read_text()
    body = (HERE / 'matrix_driver.cpp').read_text()
    names = ['turnMissileToward', 'interceptorProfile', 'patriotMotor', 'initializePatriot', 'advancePatriot',
             'shotVariation', 'interceptorFuseDelay', 'advanceDefenseReaction', 'defenseReactionReady', 'interceptorAimSpread', 'interceptorAimError', 'interceptorTimingUncertainty',
             'interceptorTrackingError', 'interceptorEstimate', 'interceptorEstimateAccelerating',
             'interceptorEstimateShtorm', 'observeShtorm', 'observeBallistic', 'observedMissileAcceleration',
             'interceptorEstimateCruise', 'estimatePantsir', 'interceptorForwardSector', 'varyMissile', 'evadeInterceptor']
    prefix = '#include "phonk_air_intercept_batch.hpp"\n#include <cstdint>\nusing entid_t=uint64_t;\n#include "phonk_ordnance.hpp"\n#include <limits>\n#include <cstdio>\nusing namespace storm::ship::phonk;\nstruct SEA_BASE { float WaveXZ(float,float) {return 0.f;} };\nstruct InterceptorProfile { float planningSpeed, initialSpeed, boostAcceleration, sustainAcceleration, maxSpeed; };\n'
    prefix += '#define MATRIX_SEED %du\n#define MATRIX_TRIALS %du\n' % (args.seed, args.trials)
    prefix += '#define MATRIX_FUSE_MAX_MS %.6ff\n' % args.fuse_max_ms
    prefix += '#define MATRIX_FUSE_MIN_MS %.6ff\n' % args.fuse_min_ms
    prefix += '#define MATRIX_REACTION_MIN_MS %.6ff\n#define MATRIX_REACTION_MAX_MS %.6ff\n' % (args.reaction_min_ms, args.reaction_max_ms)
    extracted = [function(runtime, n) for n in names]
    index = names.index('defenseReactionReady')
    extracted[index] = replace_once(extracted[index], '.12f + .16f*shotVariation', '(MATRIX_REACTION_MIN_MS*.001f) + ((MATRIX_REACTION_MAX_MS-MATRIX_REACTION_MIN_MS)*.001f)*shotVariation')
    code = prefix + ''.join(extracted) + body
    tmp = tempfile.mkdtemp(prefix='corsairs-interceptor-matrix-')
    try:
        src = pathlib.Path(tmp) / 'matrix.cpp'
        binary = pathlib.Path(tmp) / 'matrix'
        src.write_text(code)
        subprocess.run(['xcrun', 'clang++', '-std=c++20', '-O2', '-I' + str(SHIP), '-I' + str(MATH), str(src), '-o', str(binary)], check=True)
        output = subprocess.run([str(binary)], check=True, text=True, capture_output=True).stdout
        after = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
        if before != after:
            raise RuntimeError('source changed during matrix run; discard output and rerun after writer finishes')
        print('# isolated model; source sha256:')
        for path, digest in before.items():
            print('#', digest, path)
        print('# seed=%d trials_per_cell=%d; detector radius in metres; fuse %g–%g ms; reaction %g–%g ms' % (args.seed, args.trials, args.fuse_min_ms, args.fuse_max_ms, args.reaction_min_ms, args.reaction_max_ms))
        print('range_m,attack,defense,attacks,acquired,launches,hits,hit_per_attack_percent,armed,evaded')
        print(output, end='')
    finally:
        subprocess.run(['/Users/md/.codex/bin/codex-delete-temp', tmp], check=True, stdout=subprocess.DEVNULL)


if __name__ == '__main__':
    main()

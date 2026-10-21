"""One sequential, frozen-binary fleet exclusion experiment; no app delivery."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import time

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT / 'tools'))
from metal_storage import diagnostic_run, ensure_space
from run_sea_benchmark import summarize

BASE = ROOT / 'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark'
SOURCE = BASE / '20261015-105609'
OWNER = BASE / '20261015-001520/ablation'
BINARY = OWNER / 'sea-fleet-ablation'
water_only = '--water' in sys.argv[1:]
if water_only: BINARY = OWNER / 'sea-fleet-ablation-v2'
original = json.loads((SOURCE / 'manifest.json').read_text())
receipt_name = 'build-v2-receipt.json' if water_only else 'build-receipt.json'
build = json.loads((OWNER / receipt_name).read_text())
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(BINARY) == build['binary_sha256']
assert not any('/metal-engine' in row or row.strip().endswith('/engine-1')
               for row in os.popen('ps -axo pid=,comm=').read().splitlines())
ensure_space(path=BASE)
stamp = time.strftime('%Y%m%d-%H%M%S')
cases = [
    ('full_start', []), ('no_shadows', ['shadows']),
    ('no_clouds', ['clouds']), ('no_reflections', ['reflections']),
    ('no_water', ['water']), ('no_hulls', ['ships']),
    ('no_sails', ['sails']), ('no_crew', ['crew']),
    ('no_rigging', ['rigging']), ('no_postfx', ['postfx']),
    ('no_effects', ['effects']),
    ('no_shadow_cloud_reflection', ['shadows', 'clouds', 'reflections']),
    ('all_render_groups_off', ['shadows', 'clouds', 'reflections', 'water', 'ships', 'sails', 'crew', 'rigging', 'postfx', 'effects']),
    ('full_end', []),
]
results = []
if water_only: cases = [('full_start', []), ('flat_water_fragment', ['water']), ('full_end', [])]
for number, (label, omitted) in enumerate(cases):
    run = BASE / f'{stamp}-ablation-{number:02}-{label}'
    with diagnostic_run(run) as (scratch, budget):
        for name in ('fixture.c', 'ship-type.c', 'ship-lights.c', 'day.c', 'native-declarations.c', 'engine.ini'):
            shutil.copyfile(SOURCE / name, run / name)
        (run / 'RESOURCE').symlink_to((SOURCE / 'RESOURCE').resolve(), target_is_directory=True)
        env = {k: v for k, v in os.environ.items() if not k.startswith('STORM_')}
        env.update(original['backend_environment'])
        env.update(STORM_METAL_SHIP_LOCAL_SHADOWS='0', STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS='1',
                   STORM_METAL_BENCHMARK_EXCLUDE=','.join(omitted), STORM_QUIET_CLIENT='1',
                   STORM_METAL_FRAME_TRACE=str(run / 'renderer-frames.csv'), TMPDIR=str(scratch) + os.sep)
        env.pop('STORM_METAL_GPU_CAPTURE', None)
        effective = dict(original['settings'])
        if 'shadows' in omitted: effective['STORM_METAL_DYNAMIC_LIGHTING'] = '0'
        if 'clouds' in omitted: effective['STORM_METAL_DYNAMIC_SKY'] = '0'
        if 'postfx' in omitted:
            for key in ('STORM_METAL_CINEMATIC', 'STORM_METAL_LIGHT_SHAFTS', 'STORM_METAL_ANTIALIASING'): effective[key] = '0'
        manifest = dict(original, binary_sha256=sha(BINARY), frames=384, instrumented=False,
                        diagnostic_ablation=True, omitted=omitted, settings=effective,
                        build_receipt=str(OWNER / receipt_name),
                        generated_inputs_sha256={name: sha(run / name) for name in ('fixture.c', 'ship-type.c', 'ship-lights.c', 'day.c', 'native-declarations.c', 'engine.ini')})
        (run / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        with (run / 'run.log').open('w') as log:
            process = budget.start([str(BINARY), str(run), '384', '2560', '1655'], env=env, stdout=log, stderr=log)
            code = budget.wait(process, 90)
        if code:
            print(label, 'FAILED', code, (run / 'run.log').read_text()[-3000:], flush=True)
            raise SystemExit(code)
        summary = summarize(run, 384, 128)
        coverage = summary['required_pass_minima']
        for group, field in (('shadows', 'shadow_cubes_complete'), ('clouds', 'sky_current'), ('water', 'sea_draws'), ('sails', 'sails'), ('crew', 'animated')):
            if group in omitted and not (water_only and group == 'water'): assert coverage[field] == 0, (label, field, coverage[field])
        if not omitted or water_only: assert coverage == {'sails': 152, 'animated': 311, 'sea_draws': 1, 'sky_current': 1, 'shadow_cubes_complete': 8}, coverage
        summary.update(diagnostic_ablation=True, omitted=omitted, performance_acceptance=False)
        (run / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
        result = dict(label=label, run=str(run.relative_to(ROOT)),
                      frame_ms=summary['main_completion_cadence_mean_ms'],
                      cpu_wall_ms=summary['cpu_core_wall_mean_ms'],
                      cpu_prepare_median_ms=summary['cpu_preparation_estimate_ms']['median'],
                      gpu_median_ms=summary['gpu_execution_union_ms']['median'],
                      draws=summary['draws']['median'], coverage=coverage)
        results.append(result)
        (OWNER / ('matrix-water-results.json' if water_only else 'matrix-results.json')).write_text(json.dumps(results, indent=2) + '\n')
        print(json.dumps(result), flush=True)

"""Matched frozen full-fleet Off/On/Off; refuses an active player process."""
from pathlib import Path
import csv, hashlib, json, os, re, shutil, subprocess, sys, time

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from metal_storage import diagnostic_run, ensure_space
from run_sea_benchmark import summarize

sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((HERE/'build-receipt.json').read_text())
binary=HERE/'sea-fleet-reflection-lod'
assert sha(binary)==receipt['binary_sha256']
for p,expected in receipt['source_sha256'].items():assert sha(Path(p))==expected,p
BASE=ROOT/'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark'
SOURCE=BASE/'20261015-105609'
baseline=json.loads((SOURCE/'manifest.json').read_text())
ensure_space(128*1024*1024,BASE)
results=[];stamp=time.strftime('%Y%m%d-%H%M%S');reference_coverage=None
# Resume a completed Off control rather than repeat an unchanged measurement.
# The current frozen binary defines the matched population, not a different
# earlier executable's incidental animated-draw count.
if len(sys.argv)>1:
    prior=Path(sys.argv[1]).resolve();assert prior.is_relative_to(BASE.resolve())
    previous=json.loads((prior/'manifest.json').read_text())
    assert previous['binary_sha256']==sha(binary) and previous['lod'] is False
    assert previous['frames']==384 and previous['warmup']==128
    for name,expected in previous['inputs_sha256'].items():assert sha(prior/name)==expected and sha(SOURCE/name)==expected,name
    old=json.loads((prior/'summary.json').read_text());reference_coverage=old['required_pass_minima']
    assert reference_coverage['sails']==152 and reference_coverage['animated']>=311
    assert reference_coverage['sea_draws']==1 and reference_coverage['sky_current']==1 and reference_coverage['shadow_cubes_complete']==8
    results.append({'run':str(prior.relative_to(ROOT)),'lod':False,'summary':old,'lod_admission':[],'scene_sha256':sha(prior/'scene.ppm')})
for index,enabled in enumerate((False,True,False)):
    if index==0 and results:continue
    holders=subprocess.check_output(['ps','-axo','pid=,state=,comm='],text=True)
    assert not any(('/metal-engine' in row or row.rstrip().endswith('/engine-1')) and ' Z ' not in row for row in holders.splitlines()),'Player running; no diagnostic workload started'
    run=BASE/f'{stamp}-reflection-lod-{index}-{int(enabled)}'
    with diagnostic_run(run) as (scratch,budget):
        names=('fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini')
        for name in names:shutil.copyfile(SOURCE/name,run/name)
        (run/'RESOURCE').symlink_to((SOURCE/'RESOURCE').resolve(),target_is_directory=True)
        env={k:v for k,v in os.environ.items() if not k.startswith('STORM_')};env.update(baseline['backend_environment'])
        env.update(STORM_METAL_PROFILE='1',STORM_METAL_SHIP_LOCAL_SHADOWS='0',STORM_METAL_UPSCALING='0',STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS='1',STORM_METAL_FRAME_TRACE=str(run/'renderer-frames.csv'),STORM_QUIET_CLIENT='1',TMPDIR=str(scratch)+os.sep)
        for name in ('STORM_METAL_GPU_CAPTURE','STORM_METAL_BENCHMARK_EXCLUDE','STORM_METAL_REFLECTION_LOD_DATA'):env.pop(name,None)
        if enabled:env['STORM_METAL_REFLECTION_LOD_DATA']=str(HERE/'indices.bin')
        manifest={'binary_sha256':sha(binary),'frozen_build_receipt':str(HERE/'build-receipt.json'),'lod':enabled,'frames':384,'warmup':128,'backend_environment':{k:v for k,v in env.items() if k.startswith('STORM_')},'inputs_sha256':{n:sha(run/n) for n in names},'not_proven':['current installed engine performance','120FPS','visual quality acceptance']}
        (run/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        with (run/'run.log').open('w') as log:
            process=budget.start([str(binary),str(run),'384','2560','1655'],env=env,stdout=log,stderr=log)
            code=budget.wait(process,90)
        assert code==0,(run,code)
        summary=summarize(run,384,128)
        coverage=summary['required_pass_minima']
        if reference_coverage is None:
            assert coverage['sails']==152 and coverage['animated']>=311
            assert coverage['sea_draws']==1 and coverage['sky_current']==1 and coverage['shadow_cubes_complete']==8
            reference_coverage=coverage
        assert coverage==reference_coverage,(coverage,reference_coverage)
        log_text=(run/'run.log').read_text()
        admission=[tuple(map(int,m)) for m in re.findall(r'reflection-lod enabled=1 cached=\d+ draws=\d+ reduced=(\d+) source_refs=(\d+) submitted_refs=(\d+)',log_text)]
        if enabled:assert admission and all(draws>0 and 0<submitted<source for draws,source,submitted in admission),'Candidate did not replace actual geometry'
        (run/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
        row={'run':str(run.relative_to(ROOT)),'lod':enabled,'summary':summary,'lod_admission':admission,'scene_sha256':sha(run/'scene.ppm')};results.append(row)
        (HERE/'result.json').write_text(json.dumps(results,indent=2)+'\n')
        print(json.dumps(row),flush=True)

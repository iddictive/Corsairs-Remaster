"""Same-binary native fleet Off/On/Off, no original app launch or SAVE access."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,sys,time

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from metal_storage import diagnostic_run,ensure_space
from run_sea_benchmark import summarize
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((HERE/'build-receipt.json').read_text());binary=HERE/'sea-fleet-ordered-batch'
assert sha(binary)==receipt['binary_sha256']
for p,h in receipt['source_sha256'].items():assert sha(Path(p))==h,p
BASE=ROOT/'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark';SOURCE=BASE/'20261015-105609'
baseline=json.loads((SOURCE/'manifest.json').read_text());ensure_space(128*1024*1024,BASE)
def idle():
 rows=subprocess.check_output(['ps','-axo','pid=,state=,comm='],text=True)
 assert not any(('/metal-engine' in r or r.rstrip().endswith('/engine-1')) and ' Z ' not in r for r in rows.splitlines()),'Player active; benchmark stops without controlling it'
results=[];coverage=None;stamp=time.strftime('%Y%m%d-%H%M%S')
for index,enabled in enumerate((False,True,False)):
 idle();run=BASE/f'{stamp}-ordered-batch-{index}-{int(enabled)}'
 with diagnostic_run(run) as (scratch,budget):
  names=('fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini')
  for name in names:shutil.copyfile(SOURCE/name,run/name)
  (run/'RESOURCE').symlink_to((SOURCE/'RESOURCE').resolve(),target_is_directory=True)
  env={k:v for k,v in os.environ.items() if not k.startswith('STORM_')};env.update(baseline['backend_environment'])
  env.update(STORM_METAL_PROFILE='1',STORM_METAL_SHIP_LOCAL_SHADOWS='0',STORM_METAL_UPSCALING='0',STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS='1',STORM_METAL_FRAME_TRACE=str(run/'renderer-frames.csv'),STORM_QUIET_CLIENT='1',STORM_METAL_ORDERED_BATCH=str(int(enabled)),TMPDIR=str(scratch)+os.sep)
  for name in ('STORM_METAL_GPU_CAPTURE','STORM_METAL_BENCHMARK_EXCLUDE','STORM_METAL_REFLECTION_LOD_DATA'):env.pop(name,None)
  manifest={'binary_sha256':sha(binary),'frozen_build_receipt':str(HERE/'build-receipt.json'),'ordered_batch':enabled,'frames':384,'warmup':128,'dimensions':[2560,1655],'resource_root':str((run/'RESOURCE').resolve()),'backend_environment':{k:v for k,v in env.items() if k.startswith('STORM_')},'inputs_sha256':{n:sha(run/n) for n in names},'acceptance':'full workload and observed scene; color-byte equality diagnostic only','not_proven':['installed player outcome','sustained120FPS']}
  (run/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
  with (run/'run.log').open('w') as log:
   process=budget.start([str(binary),str(run),'384','2560','1655'],env=env,stdout=log,stderr=log);code=budget.wait(process,90)
  assert code==0,(run,code)
  idle();summary=summarize(run,384,128);current=summary['required_pass_minima']
  assert current['sails']==152 and current['animated']>=311 and current['sea_draws']==1 and current['sky_current']==1 and current['shadow_cubes_complete']==8,current
  if coverage is None:coverage=current
  assert current==coverage,(current,coverage)
  log_text=(run/'run.log').read_text();admission=re.findall(r'\[StormMetal ordered\] ([^\n]+)',log_text)
  admission=[{k:int(v) for k,v in re.findall(r'(\w+)=(\d+)(?= |$)',line)} for line in admission]
  if enabled:assert admission and any(r.get('admitted_world',0)>0 and r.get('world_main',0)>0 and 0<r['batches']<r['admitted'] for r in admission),'No actual world/main batching admission'
  row={'run':str(run.relative_to(ROOT)),'ordered_batch':enabled,'summary':summary,'admission':admission,'scene_sha256':sha(run/'scene.ppm')};results.append(row)
  (HERE/'result.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(row),flush=True)

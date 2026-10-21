"""Finite cost-only native fleet Off/On/Off; generated images are never presented."""
from pathlib import Path
import csv,hashlib,json,os,re,shutil,statistics,subprocess,sys,time
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from metal_storage import diagnostic_run,ensure_space
from run_sea_benchmark import summarize
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
r=json.loads((HERE/'build-receipt.json').read_text());binary=HERE/'sea-fleet-fi-multi-cost'
assert sha(binary)==r['binary_sha256']
for p,h in {**r['frozen_link_inputs_sha256'],**r['source_sha256'],**r['backend_dependency_sha256']}.items():assert sha(p)==h,p
assert sha(r['driver_object'])==r['driver_object_sha256']
BASE=ROOT/'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark';SOURCE=BASE/'20261015-105609'
baseline=json.loads((SOURCE/'manifest.json').read_text());ensure_space(192*1024*1024,BASE)
def snapshot(owned=None):
    rows=[]
    for line in subprocess.check_output(['ps','-axo','pid=,time=,comm='],text=True).splitlines():
        parts=line.strip().split(None,2)
        if len(parts)!=3:continue
        pid,cpu,command=parts;seconds=0.
        for part in cpu.split(':'):seconds=seconds*60+float(part)
        if '/metal-engine' in command or command.endswith('/engine-1') or ('/sea-fleet-' in command and int(pid)!=owned):raise RuntimeError('Runtime holder appeared; own pilot stops without controlling player: '+line)
        rows.append({'pid':int(pid),'cpu_seconds':seconds,'command':command})
    return {'monotonic':time.monotonic(),'processes':rows}
results=[];coverage=None;stamp=time.strftime('%Y%m%d-%H%M%S')
for index,enabled in enumerate((0,1,0)):
    snapshot();run=BASE/f'{stamp}-fi-multi-cost-{index}-{enabled}';samples=[]
    with diagnostic_run(run) as (scratch,budget):
        names=('fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini')
        for name in names:shutil.copyfile(SOURCE/name,run/name)
        (run/'RESOURCE').symlink_to((SOURCE/'RESOURCE').resolve(),target_is_directory=True)
        env={k:v for k,v in os.environ.items() if not k.startswith('STORM_')};env.update(baseline['backend_environment'])
        env.update(STORM_METAL_PROFILE='1',STORM_METAL_SHIP_LOCAL_SHADOWS='0',STORM_METAL_UPSCALING='0',STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS='1',STORM_METAL_FRAME_TRACE=str(run/'renderer-frames.csv'),STORM_QUIET_CLIENT='1',STORM_METAL_FI_COST_PROBE=str(enabled),TMPDIR=str(scratch)+os.sep)
        for name in ('STORM_METAL_GPU_CAPTURE','STORM_METAL_BENCHMARK_EXCLUDE','STORM_METAL_REFLECTION_LOD_DATA','STORM_METAL_ORDERED_BATCH'):env.pop(name,None)
        manifest={'binary_sha256':sha(binary),'frozen_build_receipt':str(HERE/'build-receipt.json'),'fi_cost_probe':enabled,'frames':384,'warmup':128,'dimensions':[2560,1655],'resource_root':str((run/'RESOURCE').resolve()),'backend_environment':{k:v for k,v in env.items() if k.startswith('STORM_')},'inputs_sha256':{n:sha(run/n) for n in names},'acceptance':'Generation cost plus native scene; no generated presentation or displayedFPS claim'}
        (run/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        with (run/'run.log').open('w') as log:
            process=budget.start([str(binary),str(run),'384','2560','1655'],env=env,stdout=log,stderr=log);deadline=time.monotonic()+90
            print(json.dumps({'phase':'running','case':index,'enabled':enabled,'pid':process.pid,'run':str(run)}),flush=True)
            while os.waitid(os.P_PID,process.pid,os.WEXITED|os.WNOHANG|os.WNOWAIT) is None:
                budget.check();samples.append(snapshot(process.pid))
                if time.monotonic()>deadline:raise RuntimeError('Finite pilot deadline')
                time.sleep(.5)
            code=budget.wait(process,5)
        (run/'process-cpu-samples.json').write_text(json.dumps(samples)+'\n');assert code==0,(code,run)
    snapshot();summary=summarize(run,384,128);current=summary['required_pass_minima']
    assert current['sails']==152 and current['animated']>=311 and current['sea_draws']==1 and current['sky_current']==1 and current['shadow_cubes_complete']==8,current
    if coverage is None:coverage=current
    assert current==coverage,(coverage,current)
    log=(run/'run.log').read_text();found=re.findall(r'\[FI cost\] enabled=(\d+) generated=(\d+) seeded=(\d+) fallback=(\d+) admitted=(\d+) status=(\w+)',log);assert len(found)==1,found
    on,generated,seeded,fallback,admitted,status=found[0];assert int(on)==enabled
    if enabled:assert int(generated)>=256 and int(admitted)==int(generated) and int(fallback)==0 and status=='ready',found
    if enabled:
        phases=re.findall(r'\[FI phase\] phase=(\d+) plane=(\d+) changed=(\d+) total=(\d+) range=(\d+)\.\.(\d+)',log)
        assert len(phases)==6 and all(int(changed)>int(total)*.99 for _,_,changed,total,_,_ in phases),phases
        assert len({sha(run/f'generated-{phase}.ppm') for phase in range(3)})==3,'Phases must produce distinct images in this moving scene'
    cpu=list(csv.DictReader((run/'cpu-frames.csv').open()))[128:]
    row={'run':str(run),'enabled':enabled,'summary':summary,'main_thread_cpu_ms':statistics.median(float(x['main_thread_cpu_ms']) for x in cpu),'process_cpu_ms':statistics.median(float(x['process_cpu_ms']) for x in cpu),'generation':{'generated':int(generated),'seeded':int(seeded),'fallback':int(fallback),'status':status},'scene_sha256':sha(run/'scene.ppm')}
    results.append(row);(HERE/'result.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps({'phase':'complete','case':index,'run':str(run),'main_thread_cpu_ms':row['main_thread_cpu_ms'],'generation':row['generation']}),flush=True)
for p,h in r['frozen_link_inputs_sha256'].items():assert sha(p)==h,p
print('terminal_all_cases=0',flush=True)

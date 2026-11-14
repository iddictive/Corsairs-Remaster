#!/usr/bin/env python3
"""Isolated paired fleet experiment. Run under codex-run-scratch; no shared build writes."""
import csv, hashlib, json, os, pathlib, shlex, shutil, statistics, subprocess, sys, time
R=pathlib.Path('/Users/md/05_Repo/Corsairs'); B=R/'experiments/native-metal/.cache/build'; E=R/'docs/evidence/nuclear-foam-20261105/slow-wave-20261111'; T=pathlib.Path(os.environ['CODEX_SCRATCH_DIR']); N=R/'experiments/native-metal/inputs/toolchain/bin/ninja'; FIX=E/'fixture'; RESOURCE=pathlib.Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE'); REF='45154403d0364d97bbb159db1cc2797d21560dc7'
if os.getenv('WAVE_EVIDENCE_DIR'): E=pathlib.Path(os.environ['WAVE_EVIDENCE_DIR'])
sys.path.insert(0,str(R/'tools'))
import metal_engine_source
source_fingerprint_before=metal_engine_source.fingerprint()
def run(args,**kw):
 try: return subprocess.run(args,check=True,**kw)
 except subprocess.CalledProcessError as error:
  if error.stderr: print('\n'.join(line for line in error.stderr.decode(errors='replace').splitlines() if 'error:' in line or 'fatal error:' in line),file=sys.stderr)
  raise
def get(target): return subprocess.check_output([str(N),'-C',str(B),'-t','commands',target],text=True).splitlines()
def rewrite(args, out):
 for flag,name in [('-o',out),('-MF',out+'.d'),('-MT',out)]: args[args.index(flag)+1]=name
 if '-v' in args:args.remove('-v')
 return args
def hashfile(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
source=R/'experiments/native-metal/engine/src/libs/sea/src'; shared=R/'experiments/native-metal/engine/src/libs/shared_headers/include/shared/sea_nuclear_wave.hpp'
sea_target='native-engine/storm/libs/sea/CMakeFiles/sea.dir/src/sea.cpp.o'; sea_cmd=shlex.split(get(sea_target)[-1]); env_target='native-engine/storm/libs/sea/CMakeFiles/sea.dir/src/env_map.cpp.o'; env_cmd=shlex.split(get(env_target)[-1]); driver_target='CMakeFiles/sea-fleet-benchmark.dir/sea_fleet_benchmark.cpp.o'
commands=get('bin/sea-fleet-benchmark'); driver_line=next(c for c in commands if ' -c ' in c and c.endswith('/sea_fleet_benchmark.cpp')); driver_cmd=shlex.split(driver_line)
link=shlex.split(commands[-1]); assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2]
# Driver injects deterministic source locations into the same SEA_BASE each frame.
driver=(R/'experiments/native-metal/sea_fleet_benchmark.cpp').read_text();driver=driver.replace('core_internal.SetDeltaTime(16);','core_internal.SetDeltaTime(100);');assert driver.count('#include "ship_base.h"')==1;driver=driver.replace('#include "ship_base.h"','#include "ship_base.h"\n#include "sea_base.h"');needle='        const auto begin = std::chrono::steady_clock::now();';assert driver.count(needle)==1;driver=driver.replace(needle,needle+'''\n        if (frame == 1 && std::getenv("FLEET_FAR_SHIP")) {\n            auto *ship=contactShip(7); if (!ship) return 79;\n            ship->SetPos(CVECTOR(12000.f,0.f,0.f));\n        }\n        if (frame && (std::getenv("FLEET_WAVE_BURSTS") || (std::getenv("FLEET_WAVE_SINGLE") && frame == 100))) {\n            const int interval=std::getenv("FLEET_WAVE_INTERVAL") ? std::atoi(std::getenv("FLEET_WAVE_INTERVAL")) : 1;\n            auto *sea = dynamic_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));\n            if (!sea || interval < 1) return 78;\n            if (std::getenv("FLEET_WAVE_SINGLE") || frame % interval == 0) {\n                float x = float((frame * 197) % 3000 - 1500);\n                float z = float((frame * 311) % 3000 - 1500);\n                const float tier=std::getenv("FLEET_WAVE_TIER") ? float(std::atof(std::getenv("FLEET_WAVE_TIER"))) : 1.f;\n                sea->StartNuclearWave(x, z, unsigned(frame % 4), true, tier);\n            }\n            if (std::getenv("FLEET_FAR_SHIP")) (void)sea->WaveXZ(12000.f,0.f);\n        }''');
if os.getenv('WAVE_PICTURE'):
 old='float z = float((frame * 311) % 3000 - 1500);';assert driver.count(old)==1;driver=driver.replace(old,old+' x=0.f; z=180.f;')
drv=T/'driver.cpp';drv.write_text(driver);dc=rewrite(driver_cmd,str(T/'driver.o'));dc[-1]=str(drv);run(dc,cwd=B,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
# Bind all unchanged libraries identically and detect concurrent build mutation.
libs=sorted((B/'lib').glob('*.a')); before={str(p):hashfile(p) for p in libs}
backend_before={str(p):hashfile(p) for p in [R/'experiments/native-metal/backend.mm',R/'experiments/native-metal/sea_shaders.hpp']}
combined_backend=None
if os.getenv('FLEET_COMBINED'):
 backend_target='CMakeFiles/storm_metal.dir/backend.mm.o';backend_cmd=shlex.split(get(backend_target)[-1]);backend_obj=T/'backend.mm.o'
 # Offscreen ablation: silence only the foam fragment output in scratch.
 shader=(R/'experiments/native-metal/sea_shaders.hpp').read_text()
 for anchor in (['fragment float4 seafoam_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]) {', 'float4 seaFoamModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,array<sampler,8> s) {'] if os.getenv('WAVE_ABLATE','1')=='1' else []):
  assert shader.count(anchor)==1;shader=shader.replace(anchor,anchor+'return float4(0);')
 (T/'sea_shaders.hpp').write_text(shader)
 for name in ('caustic_shaders.hpp','land_shaders.hpp','postprocess_shaders.hpp','particles_shaders.hpp'):
  (T/name).write_bytes((R/'experiments/native-metal'/name).read_bytes())
 (T/'backend.mm').write_bytes((R/'experiments/native-metal/backend.mm').read_bytes())
 backend_cmd[-1]=str(T/'backend.mm');backend_cmd.insert(1,'-I'+str(R/'experiments/native-metal'))
 run(rewrite(backend_cmd,str(backend_obj)),cwd=B,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 combined_backend=T/'libstorm_metal.a';shutil.copy2(B/'libstorm_metal.a',combined_backend);run(['ar','-r',str(combined_backend),str(backend_obj)],cwd=B,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
for variant in ['baseline','candidate']:
 d=T/variant;d.mkdir();(d/'shared').mkdir();
 for path in ['sea.cpp','sea.h','sea_geomorph.h','env_map.cpp']:
  content=(source/path).read_bytes();(d/path).write_bytes(content)
 if variant=='baseline' and not os.getenv('WAVE_GEOMETRY_PAIRED'):
  original=subprocess.check_output(['git','show',f'{REF}:experiments/native-metal/engine/src/libs/sea/src/sea.cpp'],text=True)
  p=d/'sea.cpp'; code=p.read_text()
  def section(s,a,b): return s[s.index(a):s.index(b,s.index(a))]
  for a,b in [('    const float nuclearHeightBound = NuclearWaveHeightBound();','    if (!isVisibleBBox('),('void SEA::StartNuclearWave(', '\nvoid SEA::AdvanceNuclearWaves(')]:
   old=section(code,a,b) if a.startswith('void') else section(code,'    const float fGSize = fGridStep * iSize;','    if (!isVisibleBBox(')
   replacement=section(original,a,b) if a.startswith('void') else section(original,'    const float fGSize = fGridStep * iSize;','    if (!isVisibleBBox(')
   assert code.count(old)==1;code=code.replace(old,replacement)
  p.write_text(code)
 # Count actual active sources at once per simulated second in both variants.
 p=d/'sea.cpp';code=p.read_text(); needle='void SEA::AdvanceNuclearWaves(float seconds)\n{'
 assert code.count(needle)==1
 code=code.replace(needle,needle+'\n    static int diagnosticFrame = 0; if (++diagnosticFrame % 63 == 0) std::fprintf(stderr, "[WaveCount] frame=%d active=%zu\\n", diagnosticFrame, nuclearWaves.size());')
 code=code.replace('#include "sea.h"','#include "sea.h"\n#include <cstdio>',1);p.write_text(code)
 if variant=='baseline' and os.getenv('FLEET_COMBINED') and not any(os.getenv(k) for k in ('WAVE_GEOMETRY_PAIRED','WAVE_PICTURE','WAVE_ABLATE_RATE1')):
  p=d/'sea.cpp';code=p.read_text();a='const CVECTOR4 eventSeverity(event.severity, event.damageFraction, 0.f, 0.f);';assert code.count(a)==1;code=code.replace(a,'const CVECTOR4 eventSeverity(event.severity, event.damageFraction, nuclearWavesActive ? 1.f : 0.f, 0.f);');p.write_text(code)
 if variant=='candidate' and os.getenv('FLEET_WAVE_DIAG'):
  code=(d/'sea.cpp').read_text();code=code.replace('#include "sea.h"','#include "sea.h"\n#include <chrono>\n#include <cstdio>\nstatic std::atomic<unsigned long> waveEdgeCalls{0},waveOutsideCalls{0};')
  a='void SEA::BuildNuclearField(float heightBound)\n{';assert code.count(a)==1;code=code.replace(a,a+'\n    const auto probeStart = std::chrono::steady_clock::now();')
  a='    std::atomic_store(&nuclearSnapshot, std::shared_ptr<const NuclearSnapshot>(std::move(snapshot)));';assert code.count(a)==1;code=code.replace(a,a+'''\n    static unsigned probeFrame = 0;\n    if (++probeFrame % 60 == 0) std::fprintf(stderr,"[WaveProbe] frame=%u build_ms=%.4f edge=%lu outside=%lu\\n",probeFrame,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-probeStart).count(),waveEdgeCalls.exchange(0),waveOutsideCalls.exchange(0));''')
  a='        const auto exact = ExactNuclearWaveSample(*snapshot, x, z);';assert code.count(a)==1;code=code.replace(a,'        waveEdgeCalls.fetch_add(1, std::memory_order_relaxed);\n'+a)
  a='    return ExactNuclearWaveSample(*snapshot, x, z);';assert code.count(a)==1;code=code.replace(a,'    waveOutsideCalls.fetch_add(1, std::memory_order_relaxed);\n'+a)
  (d/'sea.cpp').write_text(code)
 h=((R/'docs/evidence/nuclear-foam-20261105/organic-wave-20261111/before-nuclear.hpp').read_bytes() if os.getenv('WAVE_GEOMETRY_PAIRED') else subprocess.check_output(['git','show',f'{REF}:experiments/native-metal/engine/src/libs/shared_headers/include/shared/sea_nuclear_wave.hpp'])) if variant=='baseline' else shared.read_bytes();(d/'shared/sea_nuclear_wave.hpp').write_bytes(h)
 # The first include wins for the copied snapshot header.
 args=rewrite(sea_cmd.copy(),str(d/'sea.cpp.o'));args.insert(1,'-I'+str(source));args.insert(1,'-I'+str(d));args[-1]=str(d/'sea.cpp');run(args,cwd=B,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 args=rewrite(env_cmd.copy(),str(d/'env_map.cpp.o'));args.insert(1,'-I'+str(source));args.insert(1,'-I'+str(d));args[-1]=str(d/'env_map.cpp');run(args,cwd=B,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 archive=d/'libsea.a';shutil.copy2(B/'lib/libsea.a',archive);run(['ar','-r',str(archive),str(d/'sea.cpp.o'),str(d/'env_map.cpp.o')],cwd=B,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 args=link.copy();args=[str(archive) if a in ('lib/libsea.a',str(B/'lib/libsea.a')) else str(T/'driver.o') if a.endswith('/sea_fleet_benchmark.cpp.o') or a=='CMakeFiles/sea-fleet-benchmark.dir/sea_fleet_benchmark.cpp.o' else a for a in args];args[args.index('-o')+1]=str(d/'fleet');
 if combined_backend:args=[str(combined_backend) if a in ('libstorm_metal.a',str(B/'libstorm_metal.a')) else a for a in args]
 if '-v' in args:args.remove('-v')
 run(args,cwd=B,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 if {str(p):hashfile(p) for p in libs}!=before:raise RuntimeError('shared libraries changed during paired link')
print('linked isolated variants',flush=True)
baseenv=dict(os.environ);baseenv.update(json.loads((FIX/'manifest.json').read_text())['settings']);baseenv['STORM_METAL_PROFILE']='1';baseenv['STORM_METAL_MODERN_EFFECTS']='1';baseenv['STORM_METAL_UPSCALING']='0';baseenv['TMPDIR']=str(T)
# First warm normal controls; burst workloads extend until >50 seconds simulation.
records=[]
cases=[(v,rate,tier,frames,warmup) for rate,tier,frames,warmup in [(0,1,240,100),(-1,1,1200,300),(1,1,2400,1800),(1,10,6000,5400),(60,10,6000,5400)] for v in ('baseline','candidate')]
if os.getenv('WAVE_OPTIMIZED'):
 cases=[('candidate',rate,tier,frames,warmup) for rate,tier,frames,warmup in [(-1,1,1200,300),(1,1,2400,1800),(1,10,6000,5400),(60,10,3300,2700)]]
if os.getenv('WAVE_GEOMETRY_PAIRED'):
 cases=[(v,rate,10,frames,warmup) for rate,frames,warmup in [(1,2400,1800),(60,1500,900)] for v in ('baseline','candidate')]
if os.getenv('WAVE_PICTURE'):
 cases=[('candidate',-1,1,n,100) for n in (120,150,220)]
if os.getenv('WAVE_ABLATE_RATE1'):
 cases=[('candidate',1,10,2400,1800)]
if os.getenv('FLEET_WAVE_MATRIX'):
 cases=[(variant,rate,tier,3300 if rate else 180,3000 if rate else 60) for variant in ('baseline','candidate') for rate,tier in ((0,1),(1,1),(60,1),(1,10),(60,10))]
for case in cases:
 if len(case)==5: variant,rate,tier,frames,warmup=case;bursts=rate>0
 else: variant,bursts,frames,warmup=case;rate=60 if bursts else 0;tier=1
 if os.getenv('FLEET_WAVE_DIAG_TIER'):tier=int(os.environ['FLEET_WAVE_DIAG_TIER'])
 label=f'{variant}-rate{rate}-tier{tier}-frames{frames}'
 out=T/label;out.mkdir()
 for name in ['fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini']:shutil.copy2(FIX/name,out/name)
 (out/'RESOURCE').symlink_to(RESOURCE.resolve(),target_is_directory=True)
 env=baseenv.copy();env['STORM_METAL_FRAME_TRACE']=str(out/'renderer-frames.csv');
 if bursts:env.update(FLEET_WAVE_BURSTS='1',FLEET_WAVE_INTERVAL='63' if rate==1 else '1',FLEET_WAVE_TIER=str(tier))
 if rate==-1:env['FLEET_WAVE_SINGLE']='1'
 if os.getenv('FLEET_WAVE_MATRIX'):env['FLEET_FAR_SHIP']='1'
 print(f'running {variant} bursts={bursts} frames={frames}',flush=True)
 with (out/'run.log').open('w') as log: p=subprocess.run([str(T/variant/'fleet'),str(out),str(frames),'1280','800'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=400)
 if p.returncode:raise RuntimeError(f'{variant} bursts={bursts} exit={p.returncode}: '+(out/'run.log').read_text()[-4000:])
 rows=list(csv.DictReader((out/'cpu-frames.csv').open()));values=sorted(float(r['cpu_wall_ms']) for r in rows[warmup:]);
 if len(values)!=frames-warmup:raise RuntimeError('frame count mismatch')
 stats={'variant':variant,'bursts':bursts,'rate':rate,'tier':tier,'frames':frames,'warmup':warmup,'median_ms':statistics.median(values),'p95_ms':values[(95*len(values)+99)//100-1],'p99_ms':values[(99*len(values)+99)//100-1],'max_ms':max(values)};records.append(stats);print(json.dumps(stats),flush=True)
 for name in ['cpu-frames.csv','renderer-frames.csv','run.log','scene.ppm']:
  shutil.copy2(out/name,E/(('diag-' if os.getenv('FLEET_WAVE_DIAG') else '')+label+'-'+name))
if {str(p):hashfile(p) for p in libs}!=before:raise RuntimeError('shared libraries changed during runs')
if combined_backend and {str(p):hashfile(p) for p in map(pathlib.Path,backend_before)}!=backend_before:raise RuntimeError('backend sources changed during combined paired run')
(E/'results.json').write_text(json.dumps({'baseline_ref':REF,'libraries':before,'backend_sources':backend_before,'source_fingerprint_before':source_fingerprint_before,'source_fingerprint_after':metal_engine_source.fingerprint(),'results':records},indent=2)+'\n')

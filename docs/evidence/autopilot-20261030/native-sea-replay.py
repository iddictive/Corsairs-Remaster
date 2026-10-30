"""Compile/link one headless native controller fixture against canonical libraries.

Run under codex-run-scratch after root announces coherent native build readiness.
No shared build mutation; compile/link outputs and all core userdata stay scratch.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[3]
OUT=Path(__file__).resolve().parent
NATIVE=ROOT/'experiments/native-metal'
BUILD=NATIVE/'.cache/build'
WORK=Path(os.environ['CODEX_SCRATCH_DIR'])
mode=sys.argv[1]
assert mode in ('compile','run')
lines=subprocess.check_output([str(NATIVE/'inputs/toolchain/bin/ninja'),'-C',str(BUILD),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
compiles=[s for s in lines if ' -c ' in s and '/sea_fleet_benchmark.cpp' in s]
links=[s for s in lines if ' -o bin/sea-fleet-benchmark ' in s]
assert len(compiles)==len(links)==1
compile=shlex.split(compiles[0]); original='native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
for flag in ('-MD','-MT','-MF'):
    if flag in compile:
        index=compile.index(flag);del compile[index:index+(1 if flag=='-MD' else 2)]
compile[compile.index(str(NATIVE/'sea_fleet_benchmark.cpp'))]=str(OUT/'native-sea-controller.cpp')
compile[compile.index(original)]=str(WORK/'probe.o')
compile += ['-UNDEBUG','-I'+str(NATIVE/'engine/src/libs/sea_ai/src')]
compile += ['-I'+str(WORK)]
if '-v' in compile:compile.remove('-v')
ship_source=(NATIVE/'engine/src/libs/sea_ai/src/ai_ship.cpp').read_text()
fire=re.findall(r'if \(isAIControlled\(\)\)\s*\{\s*if \(dtFireTime.Update\(fDeltaTime\)\)\s*Fire\(true\);\s*\}',ship_source)
assert len(fire)==1
(WORK/'native-fire-dispatch.inc').write_text(fire[0]+'\n')
setup=re.findall(r'dtFireTime.Setup\([^;]+;',ship_source)
assert len(setup)==1
(WORK/'native-fire-setup.inc').write_text(setup[0]+'\n')
result={'scope':'Real linked native AI task/move/rotate/speed/touch controllers, real ATTRIBUTES and core entity lookup; deterministic fixture SHIP_BASE supplies pose/braking and records output. Exact-source fire dispatch fragment uses real Active gate/DTimer and a recorded Fire callback, not actual cannons. No game/window/render/save.',
        'fire_dispatch_sha256':hashlib.sha256(fire[0].encode()).hexdigest(),
        'fire_setup_sha256':hashlib.sha256(setup[0].encode()).hexdigest(),
        'source_sha256':hashlib.sha256((OUT/'native-sea-controller.cpp').read_bytes()).hexdigest()}
def clean(text):return text.replace(str(ROOT),'$REPO').replace(str(WORK),'$SCRATCH')
def run(argv,name):
    start=time.perf_counter();r=subprocess.run(argv,cwd=BUILD,env=dict(os.environ,STORM_USERDATA=str(WORK/'userdata'),TMPDIR=str(WORK)),capture_output=True,text=True,timeout=100)
    result[name]={'exit':r.returncode,'seconds':time.perf_counter()-start,'stdout':clean(r.stdout),'stderr':clean(r.stderr[-6000:])}
    return r.returncode
if run(compile,'compile'):
    print(json.dumps(result));raise SystemExit(1)
if mode=='compile':print(json.dumps({'compile':result['compile']['exit'],'source_sha256':result['source_sha256']}));raise SystemExit(0)
link=shlex.split(links[0]);assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2]
link[link.index(original)]=str(WORK/'probe.o');link[link.index('bin/sea-fleet-benchmark')]=str(WORK/'native-probe')
if '-v' in link:link.remove('-v')
libs=[Path(p) if Path(p).is_absolute() else BUILD/p for p in link if p.endswith('.a')]
before={str(p.relative_to(BUILD)) if p.is_relative_to(BUILD) else str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in libs}
if not run(link,'link'):
    subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(WORK/'native-probe')],capture_output=True,text=True,check=True)
    os.chdir(WORK)
    start=time.perf_counter();r=subprocess.run([str(WORK/'native-probe')],cwd=WORK,env=dict(os.environ,STORM_USERDATA=str(WORK/'userdata')),capture_output=True,text=True,timeout=30)
    result['run']={'exit':r.returncode,'seconds':time.perf_counter()-start,'stdout':clean(r.stdout),'stderr':clean(r.stderr[-6000:])}
result['libraries_sha256']=before
result['libraries_unchanged']=all(hashlib.sha256(p.read_bytes()).hexdigest()==before[str(p.relative_to(BUILD)) if p.is_relative_to(BUILD) else str(p)] for p in libs)
(OUT/'native-sea-controller.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ('run','libraries_unchanged') if k in result}))
raise SystemExit(result.get('run',result['link'])['exit'])

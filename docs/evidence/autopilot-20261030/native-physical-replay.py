"""One hidden component trajectory; real mounted Ship/SAIL/water, no game or SAVE.

Run via codex-run-scratch, max4GB/180s; one compiler and canonical read-only libs.
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

ROOT=Path(__file__).resolve().parents[3];OUT=Path(__file__).resolve().parent
NATIVE=ROOT/'experiments/native-metal';BUILD=NATIVE/'.cache/build';WORK=Path(os.environ['CODEX_SCRATCH_DIR'])
sys.path.insert(0,str(ROOT/'tools'))
import run_sea_benchmark as maintained
resources=maintained.APP/'Contents/Resources'
inputs={'fixture':NATIVE/'sea_fleet_fixture.c','ship-catalog':resources/'PROGRAM/ships/ships_init.c',
        'ship-lights':resources/'PROGRAM/SEA_AI/ShipLights.c','day':resources/'PROGRAM/Weather/Init/Day.c',
        'units':resources/'PROGRAM/utils.c','native-declarations':resources/'PROGRAM/seadogs.c',
        'cannon-declarations':resources/'PROGRAM/cannons/cannons.h'}
texts={name:maintained.read(path) for name,path in inputs.items()}
marker='makeref(refShip,ShipsTypes[SHIP_MANOWAR]);';catalog=texts['ship-catalog'];assert catalog.count(marker)==1
start=catalog.index(marker)+len(marker);end=catalog.index('makeref(refShip,ShipsTypes[',start)
ship=re.sub(r'\brefship\b','ShipType',catalog[start:end],flags=re.I)
cannon=re.findall(r'^#define CANNON_TYPE_CANNON_LBS42\s+[0-9]+',texts['cannon-declarations'],re.M);assert len(cannon)==1
(WORK/'ship-type.c').write_text(cannon[0]+'\n'+maintained.function(texts['units'],'int Tonnes2CWT(int iTonnes)')+'void InitShipType(){\n'+ship+'\nShipType.Name="Manowar1";\n}\n')
declarations=re.findall(r'^native [^\n]+;',texts['native-declarations'],re.M);assert len(declarations)==11
fixture=texts['fixture'];assert fixture.count('int FleetCount = 8;')==fixture.count('#include "native-declarations.c"')==1
fixture=fixture.replace('int FleetCount = 8;','int FleetCount = 1;').replace('#include "native-declarations.c"','\n'.join(declarations))
(WORK/'fixture.c').write_text(fixture)
(WORK/'ship-lights.c').write_text(maintained.function(texts['ship-lights'],'void InitShipLights()'))
(WORK/'day.c').write_text(texts['day']);(WORK/'native-declarations.c').write_text('\n'.join(declarations)+'\n')
(WORK/'RESOURCE').symlink_to(resources/'RESOURCE',target_is_directory=True)
(WORK/'engine.ini').write_text('full_screen=0\nscreen_x=640\nscreen_y=480\nprogram_directory=.\nrun=fixture.c\nmax_fps=0\nvsync=0\ntexture_degradation=0\nUseLargeBackBuffer=1\nNearClipPlane=0.115\nFarClipPlane=32000\nstartFontIniFile=resource\\ini\\fonts.ini\nfont=interface_normal\nlogs=0\n[compatibility]\ntarget_version=teho\n[sea]\nfoamenable=1\n[script]\ndebuginfo=1\ncodefiles=0\nruntimelog=0\ntracefiles=0\n')
prefix=(NATIVE/'sea_fleet_benchmark.cpp').read_text();prefix=prefix[:prefix.index('int main(')]
source=prefix+(OUT/'native-physical-controller.cpp').read_text();(WORK/'probe.cpp').write_text(source)
lines=subprocess.check_output([str(NATIVE/'inputs/toolchain/bin/ninja'),'-C',str(BUILD),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
commands=[s for s in lines if ' -c ' in s and '/sea_fleet_benchmark.cpp' in s];links=[s for s in lines if ' -o bin/sea-fleet-benchmark ' in s]
assert len(commands)==len(links)==1
compile=shlex.split(commands[0]);original='native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
for flag in ('-MD','-MT','-MF'):
    if flag in compile:
        i=compile.index(flag);del compile[i:i+(1 if flag=='-MD' else 2)]
compile[compile.index(str(NATIVE/'sea_fleet_benchmark.cpp'))]=str(WORK/'probe.cpp');compile[compile.index(original)]=str(WORK/'probe.o')
compile+=['-UNDEBUG','-DSTORM_METAL_SHIP_FIRE=1','-I'+str(NATIVE/'engine/src/libs/sea_ai/src'),'-I'+str(NATIVE/'engine/src/libs/ship/src')]
if '-v' in compile:compile.remove('-v')
link=shlex.split(links[0]);assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2]
link[link.index(original)]=str(WORK/'probe.o');link[link.index('bin/sea-fleet-benchmark')]=str(WORK/'native-probe')
if '-v' in link:link.remove('-v')
libraries=[Path(p) if Path(p).is_absolute() else BUILD/p for p in link if p.endswith('.a')]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p.relative_to(BUILD)):sha(p) for p in libraries}
result={'scope':'One actual sea_fleet_fixture mounted Manowar1 with normal core/SHIP/SAIL/sea integration and actual individual AI controllers after core frame; hidden SDL component only. Fixture publishes mounted authored State.fShipSpeedZ/Y into character Ship.MaxSpeedZ/Y because the isolated bootstrap omits Ship_UpdateParameters; no crew/weather modifiers are simulated. No game/Main script/SAVE/input/pose injection after mount; camera/whole AISea behavior not accepted.',
        'inputs_sha256':{name:sha(path) for name,path in inputs.items()},'model_sha256':sha(resources/'RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm'),
        'fixture_sha256':hashlib.sha256(fixture.encode()).hexdigest(),'native_probe_sha256':hashlib.sha256(source.encode()).hexdigest(),'libraries_sha256':before}
result['projection_contract']={
    'ordinary_writer':'src/gameplay/PROGRAM/sea_ai/AIShip.c::Ship_UpdateParameters publishes Ship.MaxSpeedZ and Ship.MaxSpeedY after weather/skills/sail/speed modifiers',
    'native_consumers':'SHIP::GetMaxSpeedZ/Y read these character attrs; CalculateNewSpeedVector multiplies normalized input; Move converts knots to metres/s and integrates vRotate.y as radians/s',
    'fixture_projection':'SHIP::Mount seeds State.fShipSpeedZ=SP.fSpeedRate and State.fShipSpeedY=SP.fTurnRate/444; this fixture publishes these authored limits directly, omitting gameplay modifiers',
    'units':{'MaxSpeedZ':'knots','MaxSpeedY':'radians/second'},
    'limits':'Authored-limit physical composition only; not full Ship_UpdateParameters policy or installed gameplay acceptance'}
def clean(value):return value.replace(str(ROOT),'$REPO').replace(str(WORK),'$SCRATCH')
def execute(argv,name,cwd,timeout,env=None):
    start=time.perf_counter();r=subprocess.run(argv,cwd=cwd,env=env,capture_output=True,text=True,timeout=timeout)
    result[name]={'exit':r.returncode,'seconds':time.perf_counter()-start,'stdout':clean(r.stdout),'stderr':clean(r.stderr[-6000:])};return r.returncode
code=execute(compile,'compile',BUILD,60)
if not code:code=execute(link,'link',BUILD,40)
if not code:
    subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(WORK/'native-probe')],capture_output=True,text=True,check=True)
    env=dict(os.environ,STORM_USERDATA=str(WORK/'userdata'),TMPDIR=str(WORK),STORM_QUIET_CLIENT='1',STORM_METAL_MODERN_EFFECTS='1')
    for name in ['STORM_METAL_GPU_CAPTURE','STORM_METAL_FRAME_TRACE','STORM_METAL_CONTACT_DIAGNOSTIC']:env.pop(name,None)
    code=execute([str(WORK/'native-probe'),str(WORK)],'run',WORK,90,env)
result['libraries_unchanged']=all(sha(p)==before[str(p.relative_to(BUILD))] for p in libraries)
result['logs']={p.name:clean(p.read_text(errors='replace')[-4000:]) for p in (WORK/'userdata/Logs').glob('*') if p.stat().st_size}
(OUT/'native-physical-controller.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({name:result[name] for name in ['compile','link','run','logs','libraries_unchanged'] if name in result}));raise SystemExit(code)

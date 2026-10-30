"""One mounted native shipyard reconciliation replay, no game/UI/SAVE.

Use existing codex-run-scratch: 4GiB, 20000 files, 180s, 10GiB reserve.
Requires completed canonical native build. Intermediates are disposable;
this source and selected JSON preserve reproduction for release acceptance.
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
sys.path.insert(0,str(ROOT/'tools'))
import run_sea_benchmark as maintained

resources=maintained.APP/'Contents/Resources'
inputs={'fixture':NATIVE/'sea_fleet_fixture.c','ship-catalog':resources/'PROGRAM/ships/ships_init.c',
        'ship-lights':resources/'PROGRAM/SEA_AI/ShipLights.c','day':resources/'PROGRAM/Weather/Init/Day.c',
        'units':resources/'PROGRAM/utils.c','native-declarations':resources/'PROGRAM/seadogs.c',
        'cannon-declarations':resources/'PROGRAM/cannons/cannons.h',
        'cannon-policy':ROOT/'src/gameplay/PROGRAM/sea_ai/AICannon.c',
        'ship-policy':ROOT/'src/gameplay/PROGRAM/sea_ai/AIShip.c'}
texts={name:maintained.read(path) for name,path in inputs.items()}
marker='makeref(refShip,ShipsTypes[SHIP_MANOWAR]);'
catalog=texts['ship-catalog'];assert catalog.count(marker)==1
start=catalog.index(marker)+len(marker);end=catalog.index('makeref(refShip,ShipsTypes[',start)
ship=re.sub(r'\brefship\b','ShipType',catalog[start:end],flags=re.I)
marker2='makeref(refShip,ShipsTypes[SHIP_LUGGER]);';assert catalog.count(marker2)==1
start2=catalog.index(marker2)+len(marker2);end2=catalog.index('makeref(refShip,ShipsTypes[',start2)
lugger=re.sub(r'\brefship\b','ShipTypeLugger',catalog[start2:end2],flags=re.I)
defaults=catalog[catalog.index('aref cannonl, cannonr, cannonb, cannonf;'):catalog.index(';',catalog.index('cannonl.FireAngMax'))+1]
defaults=re.sub(r'ShipsTypes\[idx\]|\brefship\b','ShipType',defaults,flags=re.I)
cannon=re.findall(r'^#define CANNON_TYPE_CANNON_LBS(?:6|42)\s+[0-9]+',texts['cannon-declarations'],re.M);assert len(cannon)==2
(WORK/'ship-type.c').write_text('\n'.join(cannon)+'\n'+maintained.function(texts['units'],'int Tonnes2CWT(int iTonnes)')+'\nfloat Degree2Radian(float x){return x*3.14159265/180.0;}\nvoid InitShipType(){\n'+defaults+'\n'+ship+'\nShipType.Name="Manowar1";\n}\nvoid InitLugger(){\n'+defaults.replace('ShipType','ShipTypeLugger')+'\n'+lugger+'\nShipTypeLugger.Name="Lugger1";\n}\n')
declarations=re.findall(r'^native [^\n]+;',texts['native-declarations'],re.M);assert len(declarations)==11
fixture=texts['fixture'];assert fixture.count('int FleetCount = 8;')==fixture.count('#include "native-declarations.c"')==1
fixture=fixture.replace('int FleetCount = 8;','int FleetCount = 3;').replace('#include "native-declarations.c"','\n'.join(declarations)).replace('object ShipType;','object ShipType;\nobject ShipTypeLugger;')
# Execute actual load/unload policy against a finite isolated goods ledger.
# Fort/unlimited mode are excluded; goods-map storage is the explicit seam.
needle='    InitShipType();';assert fixture.count(needle)==1
fixture=fixture.replace(needle,needle+'\n    InitLugger();\n    SetEventHandler("Shp_GetCurBallsNum","Ship_GetCurrentBallsNum",0);\n    SetEventHandler("CannLoad","Cannon_LoadBall",0);\n    SetEventHandler("CannUnload","Cannon_UnloadBall",0);\n    SetEventHandler("CannGRTime","RechargeTime",0);')
needle2='        Fleet[i].nation = 0;';assert fixture.count(needle2)==1
fixture=fixture.replace(needle2,needle2+'\n        Fleet[i].Ship.Cannons.Charge.Type=0;\n        Fleet[i].fixtureGoods.good0=125;\n        Fleet[i].fixtureGoods.good1=7;\n        Fleet[i].fixtureGoods.good2=150;')
fixture+='\n#define GOOD_POWDER 2\nfloat RechargeTime(){return 10.0;}\nbool SeaPhonk_HasUnlimitedAmmo(aref captain){return false;}\nbool Fort_LoadCannon(aref captain){return false;}\nvoid Fort_UnloadCannon(aref captain){}\nint GetCannonsNum(aref captain){return 0;}\nint GetCargoGoods(aref captain,int good){string key="good"+good;return sti(captain.fixtureGoods.(key));}\nvoid AddCharacterGoodsCannon(aref captain,int good,int quantity){string key="good"+good;captain.fixtureGoods.(key)=sti(captain.fixtureGoods.(key))+quantity;}\n'
fixture+='\n'+maintained.function(texts['cannon-policy'],'bool Cannon_LoadBall()')
fixture+='\n'+maintained.function(texts['cannon-policy'],'void Cannon_UnloadBall()')
fixture+='\n'+maintained.function(texts['ship-policy'],'int Ship_GetCurrentBallsNum()')
(WORK/'fixture.c').write_text(fixture)
(WORK/'ship-lights.c').write_text(maintained.function(texts['ship-lights'],'void InitShipLights()'))
(WORK/'day.c').write_text(texts['day'])
(WORK/'RESOURCE').symlink_to(resources/'RESOURCE',target_is_directory=True)
(WORK/'engine.ini').write_text('full_screen=0\nscreen_x=640\nscreen_y=480\nprogram_directory=.\nrun=fixture.c\nmax_fps=0\nvsync=0\ntexture_degradation=0\nUseLargeBackBuffer=1\nNearClipPlane=0.115\nFarClipPlane=32000\nstartFontIniFile=resource\\ini\\fonts.ini\nfont=interface_normal\nlogs=0\n[compatibility]\ntarget_version=teho\n[sea]\nfoamenable=1\n[script]\ndebuginfo=1\ncodefiles=0\nruntimelog=0\ntracefiles=0\n')
prefix=(NATIVE/'sea_fleet_benchmark.cpp').read_text();prefix=prefix[:prefix.index('int main(')]
source=prefix+(OUT/'live-hulls-probe.cpp').read_text();(WORK/'probe.cpp').write_text(source)
lines=subprocess.check_output([str(NATIVE/'inputs/toolchain/bin/ninja'),'-C',str(BUILD),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
commands=[s for s in lines if ' -c ' in s and '/sea_fleet_benchmark.cpp' in s]
links=[s for s in lines if ' -o bin/sea-fleet-benchmark ' in s]
assert len(commands)==len(links)==1
compile=shlex.split(commands[0]);original='native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
for flag in ('-MD','-MT','-MF'):
    if flag in compile:
        i=compile.index(flag);del compile[i:i+(1 if flag=='-MD' else 2)]
compile[compile.index(str(NATIVE/'sea_fleet_benchmark.cpp'))]=str(WORK/'probe.cpp')
compile[compile.index(original)]=str(WORK/'probe.o')
compile+=['-UNDEBUG','-DSTORM_METAL_SHIP_FIRE=1','-I'+str(NATIVE/'engine/src/libs/sea_ai/src'),'-I'+str(NATIVE/'engine/src/libs/ship/src')]
if '-v' in compile:compile.remove('-v')
link=shlex.split(links[0]);assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2]
link[link.index(original)]=str(WORK/'probe.o');link[link.index('bin/sea-fleet-benchmark')]=str(WORK/'native-probe')
if '-v' in link:link.remove('-v')
libraries=[Path(p) if Path(p).is_absolute() else BUILD/p for p in link if p.endswith('.a')]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p.relative_to(BUILD)):sha(p) for p in libraries}
result={'scope':'Actual mounted SHIP/SAIL/sea, AIGroup and AIShip hull refresh/retirement. Hidden native component only; no game script/SAVE/UI. Actual Cannon_LoadBall/UnloadBall policy over finite isolated goods-map seam (125 old-type rounds,7 new-type rounds,150 powder per actor); ten-second recharge, no firing, fort/unlimited mode excluded. Retained task/camera identity is structural; normal camera scene interaction remains untested.',
        'inputs_sha256':{name:sha(path) for name,path in inputs.items()},
        'fixture_sha256':hashlib.sha256(fixture.encode()).hexdigest(),
        'probe_sha256':hashlib.sha256(source.encode()).hexdigest(),'libraries_sha256':before}
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
(OUT/'live-hulls-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({name:result[name] for name in ['compile','link','run','logs','libraries_unchanged'] if name in result}))
raise SystemExit(code)

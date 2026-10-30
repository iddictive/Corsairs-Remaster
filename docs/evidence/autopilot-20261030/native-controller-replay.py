"""No-UI exact-native-method replay; bounded fixture seams are reported explicitly.

Run through codex-run-scratch. Never initializes engine, SDL, window, or game.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
WORK = Path(os.environ['CODEX_SCRATCH_DIR'])
BASE = ROOT / 'experiments/native-metal/engine/src/libs'


def body(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    cursor = brace + 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


def sha(data):
    return hashlib.sha256(data).hexdigest()


phase = sys.argv[1]
assert phase in ('baseline', 'candidate')
paths = ['worldmap/src/wdm_player_ship.cpp', 'worldmap/src/wdm_enemy_ship.cpp', 'worldmap/src/wdm_ship.h']
base_commit='60984eef27ca52717cdba9cac88a4ebd848d0b47'
sources = {name: (subprocess.check_output(['git','show',base_commit+':'+str((BASE/name).relative_to(ROOT))],cwd=ROOT)
                  if phase=='baseline' else (BASE/name).read_bytes()) for name in paths}
player = body(sources[paths[0]].decode(), 'void WdmPlayerShip::Move(float dltTime)')
enemy = body(sources[paths[1]].decode(), 'void WdmEnemyShip::Move(float dltTime)')
frozen = {'phase': phase, 'base_commit':base_commit,'source_sha256': {name: sha(data) for name, data in sources.items()},
          'player_move': player, 'enemy_move': enemy}
if phase == 'baseline':
    (OUT / 'native-baseline-frozen.json').write_text(json.dumps(frozen, indent=2) + '\n')
    constants = '\n'.join(re.findall(r'^#define WDM_SHIP_.*$', sources[paths[2]].decode(), re.M))
    code = r'''
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
enum CONTROL_STATE_TYPE {CST_INACTIVE, CST_ACTIVE, CST_INACTIVATED, CST_ACTIVATED};
struct CONTROL_STATE {CONTROL_STATE_TYPE state; float fValue; int lValue;};
struct Controls {std::map<std::string,CONTROL_STATE_TYPE> keys; int reads=0;
 bool GetControlState(const char* name,CONTROL_STATE& out){++reads;out={keys[name],0,0};return true;}};
struct {Controls* Controls;} core;
struct WdmPlayerShip {bool goForward=false;float speed=0,turnspd=0;void Move(float);};
'''
    code += constants + '\n' + player + r'''
bool near(float a,float b){return fabsf(a-b)<1.e-5f;}
int main(){Controls controls;core.Controls=&controls;WdmPlayerShip ship;int failures=0;
 ship.Move(.1f);failures+=!(near(ship.speed,0)&&near(ship.turnspd,0)&&!ship.goForward);
 controls.keys["WMapShipSailUp"]=CST_ACTIVATED;ship.Move(.1f);
 failures+=!(near(ship.speed,2.5f)&&ship.goForward);
 controls.keys.clear();ship.Move(.1f);failures+=!near(ship.speed,5.f);
 controls.keys["WMapShipSailDown1"]=CST_ACTIVE;ship.Move(.1f);
 failures+=!(near(ship.speed,5.f)&&!ship.goForward);
 controls.keys.clear();controls.keys["WMapShipTurnLeft1"]=CST_ACTIVE;ship.Move(.1f);
 failures+=!near(ship.turnspd,-.25f);
 controls.keys.clear();controls.keys["WMapShipTurnRight"]=CST_ACTIVE;ship.Move(.1f);
 failures+=!near(ship.turnspd,0.f);
 controls.keys.clear();ship.speed=.5f;controls.keys["WMapShipSailDown"]=CST_ACTIVE;ship.Move(.1f);
 failures+=!near(ship.speed,0.f);
 controls.keys.clear();ship={};int loops=200000;auto start=std::chrono::steady_clock::now();
 for(int i=0;i<loops;i++)ship.Move(.016f);
 auto elapsed=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/loops;
 printf("{\"failures\":%d,\"manual_cases\":7,\"control_reads\":%d,\"frames\":%d,\"ns_per_move_fixture\":%.3f,\"checksum\":%.3f}\n",failures,controls.reads,loops,elapsed,ship.speed+ship.turnspd);
 return failures?1:0;}
'''
    source = WORK / 'baseline.cpp'
    source.write_text(code)
    compile_argv = ['/usr/bin/clang++', '-std=c++20', '-O2', str(source), '-o', str(WORK / 'baseline')]
    compiled = subprocess.run(compile_argv, capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stderr
    run = subprocess.run([str(WORK / 'baseline')], capture_output=True, text=True, timeout=30)
    result = {'scope': 'Exact original WdmPlayerShip::Move body; fixture Controls and scalar ship members. No physics/render/game acceptance.',
              'source_sha256': frozen['source_sha256'], 'compiler': subprocess.check_output(['/usr/bin/clang++','--version'],text=True).splitlines()[0],
              'compile_argv': [v.replace(str(WORK),'$SCRATCH') for v in compile_argv],
              'probe_source_sha256': sha(code.encode()), 'exit': run.returncode, 'result': json.loads(run.stdout)}
    (OUT / 'native-baseline.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    raise SystemExit(run.returncode)
frozen=json.loads((OUT/'native-baseline-frozen.json').read_text())
ship_source=(BASE/'worldmap/src/wdm_ship.cpp').read_text()
steer=body(ship_source,'void WdmShip::SteerToDirection(')
collide=body(sources[paths[0]].decode(),'void WdmPlayerShip::Collide()')
old=frozen['enemy_move']
old=old[old.index('    const auto dl ='):old.rindex('}')]
constants='\n'.join(re.findall(r'^#define WDM_SHIP_.*$', sources[paths[2]].decode(), re.M))
code=r'''
#include "matrix.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <random>
#include <string>
enum CONTROL_STATE_TYPE {CST_INACTIVE,CST_ACTIVE,CST_INACTIVATED,CST_ACTIVATED};
struct CONTROL_STATE{CONTROL_STATE_TYPE state;float fValue;int lValue;};
struct Controls{std::map<std::string,CONTROL_STATE_TYPE>keys;int reads=0;
bool GetControlState(const char*n,CONTROL_STATE&o){++reads;o={keys[n],0,0};return true;}};
struct{Controls*Controls;}core;
struct ATTRIBUTES{float x=1000,z=1000;const char*status="sailing";
float GetAttributeAsFloat(const char*n){return !strcmp(n,"targetX")?x:z;}
const char*GetAttribute(const char*){return status;}};
struct Islands{int routes=0;CVECTOR forced=0.f;void FindDirection(const CVECTOR&,const CVECTOR&,CVECTOR&d){++routes;if(~forced>0)d=forced;}};
struct Objects{bool isPause=false;Islands*islands=nullptr;}objects;
Objects*wdmObjects=&objects;
'''+constants+r'''
struct WdmShip{float speed=0,turnspd=0,kMaxSpeed=1;CMatrix mtx;bool recovering=false;
 int cancels=0,recoveries=0;void CancelRouteRecovery(){recovering=false;++cancels;}
 bool BeginRouteRecovery(const CVECTOR&,float){recovering=true;++recoveries;return true;}
 void SteerToDirection(const CVECTOR&,float,float=1.f);
 void OldDirection(const CVECTOR&direction,float dltTime,float slowingAlfa){float dx=direction.x,dz=direction.z;
'''+old+r'''}
};
struct WdmPlayerShip:WdmShip{bool goForward=false,autopilotHadControl=false;CVECTOR autopilotTarget=0.f;ATTRIBUTES*state=nullptr;
 ATTRIBUTES*AutopilotState()const{return state;}void Move(float);void Collide();};
'''+constants+'\n'+steer+'\n'+player+'\n'+collide+r'''
bool near(float a,float b){return fabsf(a-b)<1.e-5f;}
int main(){Controls controls;core.Controls=&controls;Islands islands;objects.islands=&islands;
int failures=0;auto check=[&](bool b,const char*n){printf("%s=%s\n",n,b?"pass":"FAIL");failures+=!b;};
std::mt19937 rng(4137);std::uniform_real_distribution<float>v(-100,100),t(0,.5),k(0,5),d(0,1);
float maxError=0;for(int i=0;i<100000;i++){WdmShip a,b;a.speed=b.speed=v(rng);a.turnspd=b.turnspd=v(rng);
a.kMaxSpeed=b.kMaxSpeed=k(rng);a.mtx.BuildRotateY(v(rng));b.mtx=a.mtx;CVECTOR direction(v(rng),0,v(rng));
float dt=t(rng),slowing=d(rng);a.OldDirection(direction,dt,slowing);b.SteerToDirection(direction,dt,slowing);
maxError=std::max(maxError,std::max(fabsf(a.speed-b.speed),fabsf(a.turnspd-b.turnspd)));}
check(maxError==0,"npc_steering_numeric_oracle_100000");
WdmPlayerShip ship;ship.Move(.1f);check(near(ship.speed,0)&&near(ship.turnspd,0),"manual_idle");
controls.keys["WMapShipSailUp"]=CST_ACTIVATED;ship.Move(.1f);check(near(ship.speed,2.5f)&&ship.goForward,"manual_latch");
controls.keys.clear();ship.Move(.1f);check(near(ship.speed,5.f),"manual_coast");
controls.keys["WMapShipSailDown1"]=CST_ACTIVE;ship.Move(.1f);check(near(ship.speed,5.f)&&!ship.goForward,"manual_brake");
controls.keys.clear();controls.keys["WMapShipTurnLeft1"]=CST_ACTIVE;ship.Move(.1f);check(near(ship.turnspd,-.25f),"manual_left");
controls.keys.clear();controls.keys["WMapShipTurnRight"]=CST_ACTIVE;ship.Move(.1f);check(near(ship.turnspd,0),"manual_right");
controls.keys.clear();ship.speed=.5f;controls.keys["WMapShipSailDown"]=CST_ACTIVE;ship.Move(.1f);check(near(ship.speed,0),"manual_brake_floor");
ship={};controls.keys.clear();ATTRIBUTES state;ship.state=&state;int reads=controls.reads;ship.Move(.1f);
check(ship.speed>0&&ship.turnspd>0&&controls.reads==reads&&islands.routes==1&&!ship.goForward,"autopilot_route_ignores_manual");
float before=ship.speed;objects.isPause=true;ship.Move(.1f);check(ship.speed==before&&islands.routes==1,"paused_no_controller_work");objects.isPause=false;
state.status="idle";ship.Move(.1f);check(ship.speed<before,"idle_brakes");
state.status="sailing";ship.Collide();check(ship.recovering&&ship.recoveries==1,"collision_requests_existing_recovery");
int routes=islands.routes;ship.Move(.1f);check(islands.routes==routes,"recovery_owns_motion");
state.x=2000;ship.Move(.1f);check(!ship.recovering&&islands.routes==routes+1,"new_target_clears_old_recovery");
state.x=std::numeric_limits<float>::quiet_NaN();ship.speed=5;ship.Move(.1f);check(near(ship.speed,2.5f),"invalid_target_brakes");
ship.state=nullptr;controls.keys["WMapShipTurnLeft"]=CST_ACTIVE;before=ship.turnspd;ship.Move(.1f);
check(!ship.autopilotHadControl&&near(ship.turnspd,before-.25f),"off_returns_manual_immediately");
controls.keys.clear();ship={};int loops=200000;auto start=std::chrono::steady_clock::now();
for(int i=0;i<loops;i++)ship.Move(.016f);
double ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/loops;
printf("failures=%d numeric_max_error=%.9g frames=%d manual_ns_per_move_fixture=%.3f checksum=%.3f\n",failures,maxError,loops,ns,ship.speed+ship.turnspd);
return failures?1:0;}
'''
source=WORK/'candidate.cpp';source.write_text(code)
compile_argv=['/usr/bin/clang++','-std=c++20','-O2','-Wno-implicit-const-int-float-conversion',
              '-I'+str(ROOT/'experiments/native-metal'),'-I'+str(BASE/'math/include'),str(source),'-o',str(WORK/'candidate')]
compiled=subprocess.run(compile_argv,capture_output=True,text=True,timeout=60)
assert compiled.returncode==0,compiled.stderr
run=subprocess.run([str(WORK/'candidate')],capture_output=True,text=True,timeout=30)
result={'scope':'Exact native player Move/Collide and shared steering source, real CVECTOR/CMatrix; fixture state lookup, Controls, island route output, and recovery request. No actual island routing/physics/render/game acceptance.',
        'source_sha256':{name:sha(data) for name,data in sources.items()}|{'worldmap/src/wdm_ship.cpp':sha(ship_source.encode())},
        'baseline_oracle_sha256':sha(old.encode()),'probe_source_sha256':sha(code.encode()),'exit':run.returncode,'stdout':run.stdout,'stderr':run.stderr}
(OUT/'native-chart-candidate.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result));raise SystemExit(run.returncode)

"""Tiny CPU adapter for the exact production admission/aim-refresh statements.
No model, geometry, renderer, player input, SAVE, native library or GPU is used.
"""
from pathlib import Path
import hashlib,json,os,re,resource,subprocess,time

HERE=Path(__file__).resolve().parent
REPO=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
SOURCE=REPO/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp'
LIMITS=REPO/'experiments/native-metal/ship_hull_damage_bridge.hpp'
text=SOURCE.read_text();limits=LIMITS.read_text()
admission=re.findall(r'phonkLaser\.cuts\.size\(\) \+ phonkLaserWork\.requests\.size\(\) \+ bool\(phonkLaserWork\.active\) >=\s*storm::ship::phonk::maxLaserCuts',text)
assert len(admission)==1
start=text.index('    if (phonkLaser.sampleClock < laserSampleTime) return;')
end=text.index('    pRS->GetTransform(storm_render::TransformView, view);',start)+len('    pRS->GetTransform(storm_render::TransformView, view);')
prefix=text[start:end]
split=int(re.search(r'MaxSplitPlanes=(\d+)',limits)[1]);samples=int(re.search(r'MaxBeamSamples=(\d+)',limits)[1])
source='''#include <vector>
#include <algorithm>
#include <iostream>
namespace storm_hull_damage {constexpr unsigned MaxBeamSamples=SAMPLES;}
namespace storm {namespace ship {namespace phonk {constexpr unsigned maxLaserCuts=SPLIT;}}}
namespace storm_render {constexpr int TransformView=0;}
constexpr float laserSampleTime=.035f;
struct State {std::vector<int> cuts,pendingBeam;float sampleClock=.04f;};
struct Work {std::vector<int> requests;bool active=false;};
struct Receiver {State phonkLaser;Work phonkLaserWork;bool admit(){return !(ADMISSION);}};
struct CMatrix {int camera=0;};
struct Renderer {unsigned refreshes=0;int currentCamera=77;void GetTransform(int,CMatrix&view){++refreshes;view.camera=currentCamera;}};
struct Emitter {State phonkLaser;Receiver&receiver;Renderer*pRS;bool commitBeam(){return receiver.admit();}void frame(){PREFIX
}};
int main(){
struct Case{unsigned cuts,queued,pending;bool active;const char*name;};
const Case cases[]={{31,0,16,false,"room_for_one"},{32,0,15,false,"full_receiver_partial_batch"},{32,0,16,false,"full_receiver_full_batch"},{31,1,16,false,"queued_reservation_full"},{31,0,16,true,"active_reservation_full"}};
for(const auto&c:cases){Receiver receiver;receiver.phonkLaser.cuts.resize(c.cuts);receiver.phonkLaserWork.requests.resize(c.queued);receiver.phonkLaserWork.active=c.active;Renderer renderer;Emitter emitter{{},receiver,&renderer};emitter.phonkLaser.pendingBeam.resize(c.pending);emitter.frame();renderer.currentCamera=99;emitter.phonkLaser.sampleClock=.04f;emitter.frame();std::cout<<c.name<<','<<receiver.admit()<<','<<renderer.refreshes<<','<<emitter.phonkLaser.pendingBeam.size()<<'\\n';}
}
'''.replace('SAMPLES',str(samples)).replace('SPLIT',str(split)).replace('ADMISSION',admission[0]).replace('PREFIX',prefix)
case=HERE/'sustained-queue-audit';case.mkdir(exist_ok=False)
(case/'probe.cpp').write_text(source)
(case/'production-statements.txt').write_text(admission[0]+'\n\n'+prefix+'\n')
scratch=Path(os.environ['CODEX_SCRATCH_DIR'])
compile_command=['/usr/bin/c++','-std=c++17','-O0',str(case/'probe.cpp'),'-o',str(scratch/'queue-probe')]
before=resource.getrusage(resource.RUSAGE_CHILDREN);started=time.monotonic()
compiled=subprocess.run(compile_command,capture_output=True,text=True,timeout=10)
(case/'compile.log').write_text(compiled.stdout+compiled.stderr);assert compiled.returncode==0
ran=subprocess.run([str(scratch/'queue-probe')],capture_output=True,text=True,timeout=2)
(case/'results.csv').write_text('case,receiver_admits,camera_refreshes_across_two_changed_views,pending_samples\n'+ran.stdout)
assert ran.returncode==0
after=resource.getrusage(resource.RUSAGE_CHILDREN)
receipt={'source_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'limits_sha256':hashlib.sha256(LIMITS.read_bytes()).hexdigest(),'limits':{'saved_cut_limit':split,'pending_sample_limit':samples},'compile_command':compile_command,'run_command':[str(scratch/'queue-probe')],'compile_exit':compiled.returncode,'run_exit':ran.returncode,'wall_seconds':time.monotonic()-started,'cpu_seconds':after.ru_utime+after.ru_stime-before.ru_utime-before.ru_stime,'scope':'Exact production scalar admission and pre-camera-refresh control-flow statements; no material or player runtime inference','claim':'Full receiver +16 retained pending samples skips both current-camera refreshes despite changed camera input; room-for-one and partial-batch cases refresh both','native_or_gpu':False,'product_edits':False}
(case/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(ran.stdout,end='');print(json.dumps(receipt),flush=True)

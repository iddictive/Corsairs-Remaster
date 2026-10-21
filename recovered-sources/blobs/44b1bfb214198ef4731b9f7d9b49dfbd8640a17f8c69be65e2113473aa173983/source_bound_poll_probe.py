"""Execute the unchanged production Poll body with value-only owner mocks."""
from pathlib import Path
import hashlib,json,os,resource,subprocess,time

HERE=Path(__file__).resolve().parent
SOURCE=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp')
source=SOURCE.read_text();start=source.index('void SHIP::PollPhonkLaserWork(uint32_t delta)');end=source.index('\nbool SHIP::ApplyPhonkLaserCutNow',start);body=source[start:end]
prefix=r'''#define STORM_METAL_SHIP_HULL_DAMAGE 1
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
struct CVECTOR {float x=0,y=0,z=0;};
struct Matrix {CVECTOR axes[4]={{1,0,0},{0,1,0},{0,0,1},{0,0,0}};CVECTOR Vx()const{return axes[0];}CVECTOR Vy()const{return axes[1];}CVECTOR Vz()const{return axes[2];}CVECTOR Pos()const{return axes[3];}};
struct Projection {bool changed=false;};struct Result {bool changed=false;};
struct Geo {uint64_t generation=1,revision=0;int staged=0,committed=0,discarded=0;uint64_t GeometryGeneration(){return generation;}uint64_t CollisionRevision(){return revision;}bool StageLaserProjection(uint64_t,Projection&&p,Result&r){++staged;r.changed=p.changed;return true;}void CommitLaserProjection(uint64_t){++committed;}void DiscardLaserProjection(uint64_t){++discarded;}};
using GEOS=Geo;
struct NODE {Geo*geo=nullptr;std::string name;Matrix loc_mtx;const char*GetName(){return name.c_str();}};
struct Model {std::vector<NODE*>nodes;NODE*GetNode(int n){return n<int(nodes.size())?nodes[n]:nullptr;}};
struct Plane {std::vector<int>beams{1};};
struct Request {Plane plane;uint64_t attacker=0;int owner=0;bool payPartialDamage=false;};
struct Target {int node=0;std::string name;Matrix local;struct Input {uint64_t generation=1,revision=0;}input;Projection projection;Result result;};
namespace storm {namespace ship {namespace phonk {struct LaserCutWork {uint64_t epoch=1,model=42;std::string modelName="case";Request request;std::vector<Target>targets;bool valid=true;};}}}
using Work=storm::ship::phonk::LaserCutWork;
struct Future {bool has=true;Work work;bool valid(){return has;}std::future_status wait_for(std::chrono::seconds){return std::future_status::ready;}Work get(){has=false;return std::move(work);}};
struct Queue {Future future;uint64_t epoch=1;std::optional<Request>active=Request{};std::unique_ptr<int>cancel;std::deque<Request>requests;};
struct Entity {virtual~Entity()=default;};
struct Core {Entity*GetEntityPointer(uint64_t){return nullptr;}void Trace(const char*){}}core;
inline void returnImpact(int&,const Request&){}
struct SHIP:Entity {bool bMounted=true;Model*model=nullptr;std::string cShipIniName="case";uint64_t blots_id=7;Queue phonkLaserWork;int phonkLaser=0,applied=0;Model*GetModel(){return model;}uint64_t GetModelEID(){return 42;}bool IsAuthoredMastNode(NODE*){return false;}int*GetACharacter(){return nullptr;}bool ApplyPhonkLaserCutNow(const Plane&,bool,int,int*,bool,const Work*){++applied;return true;}bool StartPhonkLaserWork(){return true;}void PollPhonkLaserWork(uint32_t);};
'''
suffix=r'''
int main(){
std::cout<<"case,root_changed,yard_changed,drift_node,root_stage_calls,yard_stage_calls,root_commit_calls,yard_commit_calls,requeued,applied,expected_root_commit\n";
struct Case{const char*name;bool yardChanged;int driftNode;int expectedRootCommit;};
const Case cases[]={{"stable_root_changed_yard_unchanged",false,-1,1},{"rotating_UNCHANGED_yard",false,1,1},{"drifting_CHANGED_root",false,0,0},{"rotating_CHANGED_yard",true,1,0}};
for(const auto&c:cases){Geo rootGeo,yardGeo;NODE root{&rootGeo,"hull",{}},yard{&yardGeo,"yard",{}};Model model{{&root,&yard}};SHIP ship;ship.model=&model;ship.phonkLaserWork.future.work.targets={{0,"hull",root.loc_mtx,{},Projection{true},{}},{1,"yard",yard.loc_mtx,{},Projection{c.yardChanged},{}}};if(c.driftNode>=0)model.nodes[c.driftNode]->loc_mtx.axes[0].x=std::nextafter(1.f,2.f);ship.PollPhonkLaserWork(20);std::cout<<c.name<<",1,"<<c.yardChanged<<','<<c.driftNode<<','<<rootGeo.staged<<','<<yardGeo.staged<<','<<rootGeo.committed<<','<<yardGeo.committed<<','<<ship.phonkLaserWork.requests.size()<<','<<ship.applied<<','<<c.expectedRootCommit<<'\n';}
}
'''
case=HERE/'unchanged-yard-audit';case.mkdir(exist_ok=False)
(case/'probe.cpp').write_text(prefix+body+suffix);(case/'production-Poll.cpp').write_text(body)
scratch=Path(os.environ['CODEX_SCRATCH_DIR']);command=['/usr/bin/c++','-std=c++17','-O0',str(case/'probe.cpp'),'-o',str(scratch/'poll-probe')]
before=resource.getrusage(resource.RUSAGE_CHILDREN);started=time.monotonic();compiled=subprocess.run(command,capture_output=True,text=True,timeout=10);(case/'compile.log').write_text(compiled.stdout+compiled.stderr);assert compiled.returncode==0
ran=subprocess.run([str(scratch/'poll-probe')],capture_output=True,text=True,timeout=2);(case/'results.csv').write_text(ran.stdout);assert ran.returncode==0;after=resource.getrusage(resource.RUSAGE_CHILDREN)
receipt={'source_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'extracted_method_sha256':hashlib.sha256(body.encode()).hexdigest(),'compile_command':command,'run_command':[str(scratch/'poll-probe')],'compile_exit':compiled.returncode,'run_exit':ran.returncode,'wall_seconds':time.monotonic()-started,'cpu_seconds':after.ru_utime+after.ru_stime-before.ru_utime-before.ru_stime,'scope':'Full unchanged production PollPhonkLaserWork body; mocks replace resolved geometry staging, ready value worker result, core identity, follow-up snapshot start and gameplay publication. No native or material algorithm execution.','oracle':'Root changed, yard unchanged: one-ULP yard transform drift must not block root publication. Changed root/yard transform drift must reject all publication and requeue once.','baseline_failure':'rotating_UNCHANGED_yard rejects/requeues root despite unchanged yard projection','nearest_negatives':'stable target commits; either changed-node drift rejects/requeues atomically','native_or_gpu':False,'product_edits':False}
(case/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(ran.stdout,end='');print(json.dumps(receipt),flush=True)

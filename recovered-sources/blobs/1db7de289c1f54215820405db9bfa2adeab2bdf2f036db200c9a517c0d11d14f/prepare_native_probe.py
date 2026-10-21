from pathlib import Path
import hashlib, json, shlex, shutil, subprocess, sys

root = Path.cwd()
base = root / '.local-archives/metal-renderer-evidence/destruction-player-followup-20261018'
native = root / 'experiments/native-metal'
source = (native / 'sea_fleet_benchmark.cpp').read_text()
source = source.replace('#include "weather_engine.hpp"', '#include "weather_engine.hpp"\n#include "ship_base.h"\n#include "model.h"\n#include <thread>')
before = '''    const auto started=std::chrono::steady_clock::now();
    std::ofstream latency(cwd/"damage-latency.csv");
    latency<<"wall_ms,frame,phase,material_revision,collision_revision,accepted\\n";
    SHIP_BASE *victim=nullptr,*ramVictim=nullptr;MODEL *victimModel=nullptr,*ramModel=nullptr;
    bool cold=false,hot=false,ram=false,coldDone=false,hotDone=false,ramDone=false;
    uint64_t hotCollision=0,ramCollision=0;
    auto elapsed=[&]{return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();};
    auto record=[&](int f,const char*phase,MODEL*m,bool accepted){auto*g=m?m->GetNode(0)->geo:nullptr;latency<<elapsed()<<','<<f<<','<<phase<<','<<(g?g->HullMaterialRevision():0)<<','<<(g?g->CollisionRevision():0)<<','<<accepted<<'\\n';latency.flush();};
    auto hit=[&](SHIP_BASE*s,MODEL*m,float z){
        const auto a=m->mtx*CVECTOR(100.f,0.f,z),b=m->mtx*CVECTOR(0.f,0.f,z);
        const float t=m->Trace(a,b);if(t>1.f)return false;
        const auto point=a+(b-a)*t;const auto normal=!m->mtx.MulNormal(CVECTOR(1.f,0.f,0.f));
        return s->ApplyHullProjectileImpact(point,normal,-normal,100.f,-1,HullProjectileProfile::RoundShot,100000.f);
    };
'''
# CMatrix's supported normal transform is MulNormalByMatrix, so use the
# equivalent point-difference form without depending on a helper overload.
before = before.replace('m->mtx.MulNormal(CVECTOR(1.f,0.f,0.f))', '(m->mtx*CVECTOR(1.f,0.f,0.f)-m->mtx.Pos())')
before = before.replace('elapsed', 'damageWallMs')
thermal=len(sys.argv)>1 and sys.argv[1]=='thermal'
if thermal:
    source=source.replace('#include "model.h"', '''#include "model.h"
#include "ship.h"
// Fixture-only access to the real thermal endpoint; no player API/test hook.
struct ThermalEndpoint {
    using type=bool(SHIP::*)(const storm::ship::phonk::LaserPlane&,bool,int32_t,ATTRIBUTES*,bool,const CVECTOR&,float);
    friend type endpoint(ThermalEndpoint);
};
template<class Tag,typename Tag::type Member>struct FixtureEndpoint {friend typename Tag::type endpoint(Tag){return Member;}};
template struct FixtureEndpoint<ThermalEndpoint,&SHIP::ApplyPhonkLaserCut>;
''')
    start=before.index('    auto hit=');end=before.index('    };',start)+len('    };')
    before=before[:start]+'''    auto hit=[&](SHIP_BASE*s,MODEL*m,float z){
        const auto a=m->mtx*CVECTOR(100.f,0.f,z),b=m->mtx*CVECTOR(0.f,0.f,z);
        const float t=m->Trace(a,b);if(t>1.f)return false;
        CVECTOR point;m->mtx.MulToInv(a+(b-a)*t,point);
        storm::ship::phonk::LaserPlane field;field.normal=CVECTOR(1.f,0.f,0.f);field.distance=point.x;field.charred=true;
        storm_hull_damage::BeamSample beam;beam.start={point.x+.02f,point.y,point.z};beam.direction={-1,0,0};beam.tangent={0,1,0};beam.radius=.16f;beam.depth=.84f;beam.backDepth=.025f;
        field.beams.push_back(std::move(beam));
        return (dynamic_cast<SHIP*>(s)->*endpoint(ThermalEndpoint{}))(field,false,-1,nullptr,false,CVECTOR(0.f),0.f);
    };'''+before[end:]
marker = '    for (int frame = 0; frame < frames; ++frame) {'
assert source.count(marker) == 1
source = source.replace(marker, before + marker)
step = '''        if(frame>0){
            if(!victim){const auto ids=core.GetEntityIds("ship");if(ids.size()>=2){victim=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ids[0]));ramVictim=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ids[1]));victimModel=dynamic_cast<MODEL*>(core.GetEntityPointer(victim->GetModelEID()));ramModel=dynamic_cast<MODEL*>(core.GetEntityPointer(ramVictim->GetModelEID()));}}
            if(victimModel){auto*g=victimModel->GetNode(0)->geo;
                if(!cold){cold=hit(victim,victimModel,0.f);record(frame,"cold_capture",victimModel,cold);}
                if(cold&&!coldDone&&g->HullMaterialRevision()>=2){coldDone=true;record(frame,"cold_publication",victimModel,true);}
                else if(coldDone&&!hot){hotCollision=g->CollisionRevision();hot=hit(victim,victimModel,8.f);record(frame,"hot_capture",victimModel,hot);}
                if(hot&&!hotDone&&g->CollisionRevision()>hotCollision){hotDone=true;record(frame,"hot_publication",victimModel,true);}
            }
            if(ramModel&&hotDone){auto*g=ramModel->GetNode(0)->geo;HULL_CONTACT_MATERIAL material;
                if(!ram&&ramVictim->GetHullContactMaterial(material)){
                    const auto a=ramModel->mtx*CVECTOR(100.f,0.f,0.f),b=ramModel->mtx*CVECTOR(0.f);
                    const float t=ramModel->Trace(a,b);CVECTOR local;ramModel->mtx.MulToInv(a+(b-a)*t,local);
                    HULL_CONTACT_BREAK request;request.point=local;request.normal=CVECTOR(1.f,0.f,0.f);request.depth=3.f;request.specificWork=5000.;request.expectedRevision=material.revision;request.crossSection={{{-.5f,-4.f}},{{.5f,-4.f}},{{.5f,4.f}},{{-.5f,4.f}}};request.eventWorldPosition=ramModel->mtx.Pos();
                    HULL_CONTACT_BREAK_RESULT result;ramCollision=g->CollisionRevision();ram=ramVictim->CommitHullContactBreak(request,result);record(frame,"ram_capture",ramModel,ram);
                }
                if(ram&&!ramDone&&g->CollisionRevision()>ramCollision){ramDone=true;record(frame,"ram_publication",ramModel,true);}
            }
        }
'''
marker = '        const auto end = std::chrono::steady_clock::now();'
assert source.count(marker) == 1
source = source.replace(marker, step + marker)
marker = "              << ',' << rss << ',' << peak << ',' << commit << '\\n';"
assert source.count(marker) == 1
source = source.replace(marker, marker + '\n        if((coldDone&&hotDone&&ramDone)||damageWallMs()>45000.)break;\n        std::this_thread::sleep_for(std::chrono::milliseconds(2));')
source = source.replace('    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));\n    if (!renderer', '    std::printf("damage_complete=%d elapsed_ms=%.3f\\n",coldDone&&hotDone&&ramDone,damageWallMs());\n    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));\n    if (!renderer')
probe = base / 'native_damage_probe.cpp'
probe.write_text(source)
commands = subprocess.check_output(['ninja','-C',str(native/'.cache/build'),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
compile_line = [c for c in commands if 'sea_fleet_benchmark.cpp' in c and ' -c ' in c]
link_line = [c for c in commands if ' -o bin/sea-fleet-benchmark ' in c]
assert len(compile_line)==len(link_line)==1
compile_args=shlex.split(compile_line[0]);index=compile_args.index('-o');compile_args[index+1]=str(base/'native_damage_probe.o');compile_args[compile_args.index('-c')+1]=str(probe)
compile_args[compile_args.index('-MF')+1]=str(base/'native_damage_probe.o.d')
for directory in ['ship/include','model/include']:
    compile_args += ['-I',str(native/'engine/src/libs'/directory)]
if thermal:
    compile_args+=['-I',str(native/'engine/src/libs/ship/src')]
    ship_compile=[shlex.split(c) for c in commands if 'ship_phonk_laser.cpp' in c and ' -c ' in c]
    assert len(ship_compile)==1
    compile_args += [a for a in ship_compile[0] if a.startswith('-D') and a not in compile_args]
link_args=shlex.split(link_line[0]);link_args[link_args.index('-o')+1]=str(base/'native_damage_probe')
assert link_args[:2]==[':', '&&'] and link_args[-2:]==['&&', ':']
link_args=link_args[2:-2]
old_objects=[i for i,a in enumerate(link_args) if a.endswith('/sea_fleet_benchmark.cpp.o')];assert len(old_objects)==1
link_args[old_objects[0]]=str(base/'native_damage_probe.o')
receipt={'compile':compile_args,'link':link_args,'driver_sha256':hashlib.sha256(probe.read_bytes()).hexdigest(),'mode':'thermal' if thermal else 'projectile','native_inputs':'current engine-linked libraries; no new CMake target or player engine launch'}
(base/'native-probe-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
with (base/'native-probe-build.log').open('w') as log:
    subprocess.run(compile_args,cwd=native/'.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
    subprocess.run(link_args,cwd=native/'.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
baseline=native/'.cache/diagnostics/sea-fleet-benchmark/20261018-000123'
run=base/'baseline-native';run.mkdir(exist_ok=True)
for name in ('fixture.c','native-declarations.c','ship-type.c','ship-lights.c','day.c','engine.ini'):
    shutil.copy2(baseline/name,run/name)
resource=(baseline/'RESOURCE').resolve()
if (run/'RESOURCE').is_symlink():assert (run/'RESOURCE').resolve()==resource
else:(run/'RESOURCE').symlink_to(resource,target_is_directory=True)
print(base/'native_damage_probe',run)

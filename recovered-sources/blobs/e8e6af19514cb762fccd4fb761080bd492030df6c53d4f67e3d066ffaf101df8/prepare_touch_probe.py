from pathlib import Path
import json,subprocess
r=Path.cwd();b=r/'.local-archives/metal-renderer-evidence/destruction-player-followup-20261018';s=(r/'experiments/native-metal/sea_fleet_benchmark.cpp').read_text()
s=s.replace('#include "weather_engine.hpp"','#include "weather_engine.hpp"\n#include "ship_base.h"\n#include "model.h"\n#include <thread>')
marker='    for (int frame = 0; frame < frames; ++frame) {';assert s.count(marker)==1
s=s.replace(marker,'''
    const auto started=std::chrono::steady_clock::now();
    SHIP_BASE*a=nullptr,*v=nullptr;MODEL*am=nullptr,*vm=nullptr;entid_t touch=0;uint64_t firstRevision=0;
    bool setup=false,physical=false,visual=false;int eventFrame=0;double peak=0;
    std::ofstream latency(cwd/"touch-latency.csv");latency<<"ms,frame,phase,material,collision,mass_fraction\\n";
    auto record=[&](int frame,const char*phase){HULL_CONTACT_MATERIAL material;const bool ready=v->GetHullContactMaterial(material);latency<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<<','<<frame<<','<<phase<<','<<vm->GetNode(0)->geo->HullMaterialRevision()<<','<<vm->GetNode(0)->geo->CollisionRevision()<<','<<(ready?material.retainedMassFraction:-1)<<'\\n';latency.flush();};
'''+marker)
marker='        const auto end = std::chrono::steady_clock::now();';assert s.count(marker)==1
s=s.replace(marker,'''
        if(frame==1){const auto ships=core.GetEntityIds("Ship");if(ships.size()<2)return 77;
            a=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ships[0]));v=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ships[1]));
            am=dynamic_cast<MODEL*>(core.GetEntityPointer(a->GetModelEID()));vm=dynamic_cast<MODEL*>(core.GetEntityPointer(v->GetModelEID()));
            a->SetPos(CVECTOR(0.f,0.f,45.f));a->State.vAng=CVECTOR(0.f);a->State.vSpeed=CVECTOR(0.f,0.f,300.f);a->SetFixedSpeed(true,300.f);
            v->SetPos(CVECTOR(0.f,0.f,100.f));v->State.vAng=CVECTOR(0.f,1.57079632679f,0.f);v->State.vSpeed=CVECTOR(0.f);v->SetFixedSpeed(true,0.f);
            firstRevision=vm->GetNode(0)->geo->CollisionRevision();setup=true;record(frame,"approach_300_knots");
        }
        if(frame==3){touch=core.CreateEntity("touch");core.Send_Message(touch,"li",50300,a->GetId());core.Send_Message(touch,"li",50300,v->GetId());core.AddToLayer(2,touch,1);}
        if(setup){
            if(!physical&&vm->GetNode(0)->geo->HullMaterialRevision()>1){physical=true;eventFrame=frame;record(frame,"physical_contact");a->SetFixedSpeed(false,0);}
            if(physical&&!visual&&vm->GetNode(0)->geo->CollisionRevision()>firstRevision){visual=true;record(frame,"visual_publication");}
            if(frame%20==0)std::printf("TOUCH frame=%d attacker_z=%.3f target_z=%.3f material=%llu collision=%llu\\n",frame,a->GetPos().z,v->GetPos().z,(unsigned long long)vm->GetNode(0)->geo->HullMaterialRevision(),(unsigned long long)vm->GetNode(0)->geo->CollisionRevision());
        }
'''+marker)
marker="              << ',' << rss << ',' << peak << ',' << commit << '\\n';";assert s.count(marker)==1
s=s.replace(marker,marker+'''
        if(frame>1)peak=std::max(peak,std::chrono::duration<double,std::milli>(end-begin).count());
        if((visual&&frame>eventFrame+40)||frame>=300)break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
''')
# mi_process_info owns a local peak variable; keep the diagnostic wall separate.
s=s.replace('double peak=0;','double touchPeak=0;').replace('if(frame>1)peak=std::max(peak,','if(frame>1)touchPeak=std::max(touchPeak,')
s=s.replace('    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));\n    if (!renderer','    std::printf("TOUCH_RESULT physical=%d visual=%d peak_frame_ms=%.3f\\n",physical,visual,touchPeak);\n    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));\n    if (!renderer')
s=s.replace('    std::puts("pilot_complete=1 performance_acceptance=0");','    if(!physical||!visual)return 77;\n    std::puts("pilot_complete=1 performance_acceptance=0");')
p=b/'native_touch_probe.cpp';p.write_text(s)
d=json.loads((b/'native-probe-build.json').read_text());compile=d['compile'];link=d['link'];compile[compile.index('-c')+1]=str(p);compile[compile.index('-o')+1]=str(b/'native_touch_probe.o');compile[compile.index('-MF')+1]=str(b/'native_touch_probe.o.d');link[link.index('-o')+1]=str(b/'native_touch_probe');idx=next(i for i,a in enumerate(link) if a==str(b/'native_damage_probe.o'));link[idx]=str(b/'native_touch_probe.o');(b/'touch-build.json').write_text(json.dumps({'compile':compile,'link':link},indent=2)+'\n')
with (b/'touch-build.log').open('w') as log:
 subprocess.run(compile,cwd=r/'experiments/native-metal/.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(link,cwd=r/'experiments/native-metal/.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)

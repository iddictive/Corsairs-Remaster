from pathlib import Path
import json,subprocess
base=Path.cwd()/'.local-archives/metal-renderer-evidence/destruction-player-followup-20261018'
subprocess.run(['python3',str(base/'prepare_native_probe.py'),'thermal'],check=True,stdout=subprocess.DEVNULL)
p=base/'native_damage_probe.cpp';s=p.read_text()
marker='template struct FixtureEndpoint<ThermalEndpoint,&SHIP::ApplyPhonkLaserCut>;'
assert s.count(marker)==1
s=s.replace(marker,marker+'''
struct QueueEndpoint {using type=storm::ship::phonk::LaserCutQueue SHIP::*;friend type endpoint(QueueEndpoint);};
template struct FixtureEndpoint<QueueEndpoint,&SHIP::phonkLaserWork>;
''')
marker='    uint64_t hotCollision=0,ramCollision=0;'
assert s.count(marker)==1
s=s.replace(marker,marker+'''
    bool dragStarted=false,dragDone=false,restoreDone=false;int dragSamples=0,dragAccepted=0;
    std::vector<std::unique_ptr<ATTRIBUTES>> restoreAttributes;
    bool quality=true;
''')
marker='            if(ramModel&&hotDone)'
assert s.count(marker)==1
s=s.replace(marker,'''
            if(victimModel&&hotDone&&!dragDone){
                if(!dragStarted){record(frame,"drag_capture",victimModel,true);dragStarted=true;}
                if(dragSamples<30){dragAccepted+=hit(victim,victimModel,-8.f+dragSamples*.55f);++dragSamples;if(dragSamples==30)record(frame,"drag_input_end",victimModel,true);}
                else {auto&q=dynamic_cast<SHIP*>(victim)->*endpoint(QueueEndpoint{});if(!q.active&&q.requests.empty()&&!q.future.valid()){dragDone=true;record(frame,"drag_publication_end",victimModel,dragAccepted>0);}}
            }
            if(ramModel&&hotDone&&dragDone)''')
marker='        const auto end = std::chrono::steady_clock::now();'
assert s.count(marker)==1
s=s.replace(marker,'''
        if(ramDone&&!restoreDone){
            auto*original=dynamic_cast<SHIP*>(victim);
            const std::string revision=static_cast<const char*>(original->GetACharacter()->FindAClass(original->GetACharacter(),"Ship.HullMaterial")->GetAttribute("revision"));
            const auto originalTail=victimModel->GetNode(0)->geo->HullMaterialTail().size();
            record(frame,"restore_capture",victimModel,true);
            auto attrs=std::make_unique<ATTRIBUTES>(original->GetACharacter()->Copy());
            attrs->SetAttributeUseDword("index",99);
            const auto restoredId=core.CreateEntity("Ship");auto*restored=dynamic_cast<SHIP*>(core.GetEntityPointer(restoredId));
            core.Send_Message(restoredId,"lll",51015,2,3);
            core.Send_Message(restoredId,"laa",50300,attrs.get(),original->GetAShip());
            auto*model=dynamic_cast<MODEL*>(core.GetEntityPointer(restored->GetModelEID()));
            bool rays=true;for(float z:{-8.f,0.f,8.f,16.f}){const auto from=victimModel->mtx*CVECTOR(100.f,0.f,z),to=victimModel->mtx*CVECTOR(0.f,0.f,z);const auto a=victimModel->Trace(from,to);const auto b=model->Trace(model->mtx*CVECTOR(100.f,0.f,z),model->mtx*CVECTOR(0.f,0.f,z));rays&=std::abs(a-b)<.00002f;}
            const auto*state=attrs->FindAClass(attrs.get(),"Ship.HullMaterial");
            const bool restoredQuality=model->GetNode(0)->geo->HullMaterialReady()&&model->GetNode(0)->geo->HullMaterialTail().size()==originalTail&&state&&revision==static_cast<const char*>(state->GetAttribute("revision"))&&rays;
            quality&=restoredQuality;record(frame,"restore_publication",model,restoredQuality);
            std::printf("RESTORE ready=%d rays=%d tail=%zu expected_tail=%zu revision=%s\\n",model->GetNode(0)->geo->HullMaterialReady(),rays,model->GetNode(0)->geo->HullMaterialTail().size(),originalTail,revision.c_str());
            restoreAttributes.push_back(std::move(attrs));
            auto bad=std::make_unique<ATTRIBUTES>(original->GetACharacter()->Copy());bad->SetAttributeUseDword("index",100);
            auto*badState=bad->FindAClass(bad.get(),"Ship.HullMaterial");badState->SetAttribute("model","foreign_fixture_model");
            const auto rejectedId=core.CreateEntity("Ship");auto*rejected=dynamic_cast<SHIP*>(core.GetEntityPointer(rejectedId));
            core.Send_Message(rejectedId,"lll",51015,2,3);core.Send_Message(rejectedId,"laa",50300,bad.get(),original->GetAShip());
            auto*badModel=dynamic_cast<MODEL*>(core.GetEntityPointer(rejected->GetModelEID()));
            const bool rejectedQuality=!badModel->GetNode(0)->geo->HullMaterialReady()&&std::string(static_cast<const char*>(badState->GetAttribute("model")))=="foreign_fixture_model"&&revision==static_cast<const char*>(badState->GetAttribute("revision"));
            quality&=rejectedQuality;record(frame,"foreign_state_preserved",badModel,rejectedQuality);
            restoreAttributes.push_back(std::move(bad));restoreDone=true;
            std::printf("DRAG accepted=%d samples=%d foreign_preserved=%d quality=%d\\n",dragAccepted,dragSamples,rejectedQuality,quality);
        }
'''+marker)
s=s.replace('(coldDone&&hotDone&&ramDone)','(coldDone&&hotDone&&ramDone&&dragDone&&restoreDone)')
s=s.replace('    std::puts("pilot_complete=1 performance_acceptance=0");','    if(!quality||!restoreDone)return 77;\n    std::puts("pilot_complete=1 performance_acceptance=0");')
marker='    core_internal.EraseEntities();'
assert s.count(marker)==1
s=s.replace(marker,'''
    const auto pendingId=core.CreateEntity("MODELR");
    core.Send_Message(pendingId,"ls",20500,"ships/Manowar1/Manowar1");
    auto*pendingModel=dynamic_cast<MODEL*>(core.GetEntityPointer(pendingId));
    auto*pendingGeometry=pendingModel?pendingModel->GetNode(0)->geo:nullptr;
    if(!pendingGeometry)return 78;
    pendingGeometry->SetHullContactTransform(123456,{1,0,0,0,1,0,0,0,1,0,0,0});
    pendingGeometry->PrepareHullMaterial(123456);
    if(!pendingGeometry->HasPendingHullRebuild())return 79;
    std::puts("TEARDOWN queued_material_worker=1");
'''+marker)
p.write_text(s)
receipt=json.loads((base/'native-probe-build.json').read_text());receipt['mode']='thermal_drag_restore';(base/'full-probe-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
with (base/'full-probe-build.log').open('w') as log:
 subprocess.run(receipt['compile'],cwd=Path.cwd()/'experiments/native-metal/.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(receipt['link'],cwd=Path.cwd()/'experiments/native-metal/.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)


    location->supervisor.Update(0.0f);
    const float separation=std::sqrt(~(a->curPos-b->curPos));
    require(std::fabs(separation-(neutral?1.1f:1.5f))<.002f,"ordinary native combat spacing");
    if (test=="far") { b->curPos.z=a->curPos.z+2.4f; b->oldPos=b->curPos; }
    const float finalSeparation=std::sqrt(~(a->curPos-b->curPos));
    const bool boundary=test=="target-escapes" || test=="target-ineligible" || test=="target-erased" || test=="after-strike";
    const bool offset=test=="offset-right" || test=="offset-left" || boundary;
    const bool orbit=test=="orbit-right" || test=="orbit-left";
    const float side=test=="offset-left" || test=="orbit-left" ? -1.0f : 1.0f;
    const float initialAngle=offset ? side*25.0f*3.14159265359f/180.0f : 0.0f;
    const auto moveTarget=[&](float time) {
        const float angle=initialAngle+(orbit ? side*40.0f*3.14159265359f/180.0f*time : 0.0f);
        b->curPos=a->curPos+CVECTOR(std::sin(angle)*1.5f,0,std::cos(angle)*1.5f);
        b->oldPos=b->curPos;
        b->ay=angle+3.14159265359f;
        b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);
    };
    if (offset || orbit) moveTarget(0.0f);
    const bool nativeSelected=a->FindAttackCharacter()==b;
    if (offset || orbit) require(nativeSelected,"ordinary native attack selects offset/moving target");
    a->Model()->mtx.BuildMatrix(CVECTOR(0.0f,0.0f,0.0f),a->curPos);
    b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);
    if (test=="wall") {
        CMatrix transform; transform.BuildMatrix(CVECTOR(1.57079632679f,0,0),CVECTOR(0.0f));
        const CVECTOR wallPoint=a->curPos+CVECTOR(0,.7f,.35f);
        transform.Pos()=wallPoint-transform*initial;
        auto *wall=model(); wall->root=node(geo,transform); addModel(*location,*wall);
        require(location->Trace(a->curPos+CVECTOR(0,1.2f,0),b->curPos+CVECTOR(0,1.6f,0))<.995f,"actual GM wall intersects contact line");
    }
    if (test=="immortal") b->AttributesPointer->CreateSubAClass(b->AttributesPointer,"chr_ai")->SetAttributeUseDword("immortal",1);
    if (test=="same-group") {
        auto *ai=a->AttributesPointer->FindAClass(a->AttributesPointer,"chr_ai");
        b->AttributesPointer->CreateSubAClass(b->AttributesPointer,"chr_ai")->SetAttribute("group",static_cast<const char *>(ai->GetAttribute("group")));
    }
    if (test=="backend-failure") location->ragdollWorld=std::make_unique<roundhouse::RagdollWorld>();
    require(a->AttributesPointer->GetAttributeAsFloat("ReplayJumpDamage")==2*a->AttributesPointer->GetAttributeAsFloat("ReplayBaseDamage"),"jump damage coefficient");
    require(a->StartRoundhouse(test=="jump"),"actual kick accepted");
    float minimum=1000; bool crossedWindup=false; std::vector<CMatrix> initialFall,latest;
    float clearedYaw=0, strikeYaw=0; bool cleared=false, headingLocked=false;
    const auto cpuStart=std::clock();
    for (int frame=0;frame<140;++frame) {
        if ((offset || orbit) && !boundary && !b->IsRoundhouseControlled()) moveTarget((frame+1)/60.0f);
        if (boundary && frame==18 && test!="after-strike") {
            clearedYaw=a->ay;
            if (test=="target-escapes") {
                b->curPos=a->curPos+CVECTOR(0,0,3.0f); b->oldPos=b->curPos;
                b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);
            } else if (test=="target-ineligible") {
                auto *ai=a->AttributesPointer->FindAClass(a->AttributesPointer,"chr_ai");
                b->AttributesPointer->CreateSubAClass(b->AttributesPointer,"chr_ai")->SetAttribute("group",static_cast<const char *>(ai->GetAttribute("group")));
            } else {
                core.EraseEntity(b->GetId());
                require(!core.IsEntityValid(b->GetId()),"actual CORE entity invalidated");
                location->supervisor.DelCharacter(b);
            }
            cleared=true;
        }
        if (test=="after-strike" && frame==55) {
            const float angle=-45.0f*3.14159265359f/180.0f;
            b->curPos=a->curPos+CVECTOR(std::sin(angle)*1.5f,0,std::cos(angle)*1.5f);
            b->oldPos=b->curPos; b->ay=angle+3.14159265359f;
            b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);
        }
        location->StepRagdolls(1.0f/60.0f);
        location->supervisor.Update(1.0f/60.0f);
        if (a->roundhouseState && a->roundhouseState->phase==RoundhouseState::Phase::Kick) {
            if (cleared) {
                require(a->roundhouseState->aimTarget==invalid_entity,"windup target cleared without reacquisition");
                require(std::fabs(a->ay-clearedYaw)<1e-6f,"no steering after target is lost");
            }
            if (a->roundhouseState->time>=a->roundhouseState->strikeStartTime) {
                if (!headingLocked) { strikeYaw=a->ay; headingLocked=true; }
                require(std::fabs(a->ay-strikeYaw)<1e-6f,"heading remains frozen after strike starts");
            }
        }
        if (a->roundhouseState && b->PrepareRoundhouse()) {
            auto *aa=a->Model()->GetAnimation(), *ba=b->Model()->GetAnimation();
            const auto heel=roundhouse::JointPointWorld(roundhouse::AnimationJointWorld(aa->GetInverseBindMatrix(a->roundhouseState->heel),aa->GetAnimationMatrix(a->roundhouseState->heel),a->Model()->mtx),a->roundhouseState->heelOffset);
            const auto head=roundhouse::JointPointWorld(roundhouse::AnimationJointWorld(ba->GetInverseBindMatrix(b->roundhouseState->head),ba->GetAnimationMatrix(b->roundhouseState->head),b->Model()->mtx),b->roundhouseState->headOffset);
            minimum=std::min(minimum,std::sqrt(~(heel-head)));
            const auto torso=roundhouse::AnimationJointWorld(aa->GetInverseBindMatrix(a->roundhouseState->rig.bodies[1].joint),aa->GetAnimationMatrix(a->roundhouseState->rig.bodies[1].joint),a->Model()->mtx).Pos();
            if (a->roundhouseState->time>=a->roundhouseState->strikeStartTime && a->roundhouseState->time<=a->roundhouseState->strikeEndTime &&
                std::sqrt(~(heel-head))<.22f && location->Trace(torso,head)>1 && location->Trace(a->roundhouseState->strikeOrigin,head)<.995f)
                crossedWindup=true;
        }
        if (b->IsPhysicalRagdoll()) {
            auto *world=location->ExistingRagdollWorld(); require(world && world->Size()==1,"actual Jolt body creation");
            require(world->GetPose(b->roundhouseState->handle,latest),"actual physical pose");
            if (initialFall.empty()) initialFall=latest;
        }
    }
    const double cpuMilliseconds=1000.0*(std::clock()-cpuStart)/CLOCKS_PER_SEC;
    const int hits=integer("Hits");
    const float hp=variable("Victim")->GetAClass()->FindAClass(variable("Victim")->GetAClass(),"chr_ai")->GetAttributeAsFloat("hp");
    float displacement=0;
    if (!initialFall.empty()) displacement=std::sqrt(~(latest[2].Pos()-initialFall[2].Pos()));
    std::printf("case=%s supervisor_spacing=%.6f final_spacing=%.6f heel_head_min=%.6f hits=%d hp=%.6f ragdolls=%zu physical_displacement=%.6f crossed_windup=%d no_window=%d\n",test.c_str(),separation,finalSeparation,minimum,hits,hp,location->ExistingRagdollWorld()?location->ExistingRagdollWorld()->Size():0,displacement,crossedWindup,core.GetWindow()==nullptr);
    std::printf("native_selected=%d final_yaw=%.6f replay_cpu_ms=%.6f frames=140\n",nativeSelected,a->ay,cpuMilliseconds);
    const bool observedOnly=std::atoi(argv[3])<0;
    if (!observedOnly) require(hits==(expected?1:0),"normal damage exactly once or expected rejection");
    require(hits==0 || hits==1,"no duplicate damage");
    const bool expectPhysics=hits>0 && test!="backend-failure";
    require((hits ? hp<500 : hp==500) && (expectPhysics ? displacement>.01f : initialFall.empty()),"HP and physical reaction match contact");
    if (test=="wall") require(crossedWindup,"wall negative reached an already-crossed windup contact");
    require(core.GetWindow()==nullptr,"no native window after replay");
    std::fflush(nullptr);
    // The disposable process owns fixture-only entities and CPU buffers. Avoid
    // invoking game presentation destructors on intentionally uninitialized GPU state.
    std::_Exit(0);
} catch (const std::exception &e) { std::fprintf(stderr,"FAIL exception: %s\n",e.what()); return 2; }

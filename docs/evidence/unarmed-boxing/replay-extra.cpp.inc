// Appended to the shared roundhouse CPU replay's scene setup by replay.py.
// No window or player save is opened. The input animation and patch are installed files.
    location->supervisor.Update(0.0f);
    auto *ani=a->Model()->GetAnimation();
    core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"sword",0.0,0,0);
    core.Send_Message(a->GetId(),"lsl",MSG_CHARACTER_EX_MSG,"SetFightWOWeapon",1);
    require(a->isFightWOWps && a->isUnarmedForced,"forced unarmed set");
    core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"",0.0,0,0);
    require(!a->isFight,"unequip exits fight mode");
    core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"unarmed",0.0,0,0);
    core.Send_Message(a->GetId(),"lsl",MSG_CHARACTER_EX_MSG,"SetFightWOWeapon",0);
    require(a->isFightWOWps && a->isUnarmedEquipped && !a->isUnarmedForced,"equipped unarmed survives forced reset");
    a->SetFightMode(true,false); require(a->isFight,"fight mode after unequip");
    if(test=="dodge-left" || test=="dodge-right") {
        location->bSwimming=true;
        a->strafe_l.SetName("straif_weapon_left"); a->strafe_r.SetName("straif_weapon_right"); a->UpdateActionsData();
        if(test=="dodge-left") a->StrafeLeft(); else a->StrafeRight();
        const auto expected=test=="dodge-left" ? Character::fgt_strafe_l : Character::fgt_strafe_r;
        require(a->fgtSetType==expected,"native dodge requested"); a->UpdateAnimation(); a->UpdateUnarmedPose(0);
        const char *pose=test=="dodge-left"?"unarmed_dodge_left":"unarmed_dodge_right";
        std::printf("DODGE case=%s pose=%s selected=%d\n",test.c_str(),a->unarmedAction.c_str(),int(a->fgtCurType));
        require(a->fgtCurType==expected && a->unarmedAction==pose,"native dodge pose selected");
        bool started=false,ended=false; int first=-1,last=-1;
        for(int frame=0;frame<100;frame++) {
            static_cast<AnimationImp *>(ani)->Execute(16);
            if(a->isRecoilState) { if(!started) first=frame; started=true; last=frame; }
            if(started && !a->isRecoilState) ended=true;
            a->UpdateAnimation(); a->UpdateUnarmedPose(0.016f);
        }
        std::printf("DODGE window_start=%d window_last=%d ended=%d final_action=%d\n",first,last,ended,int(a->fgtCurType));
        require(started && ended && !a->isRecoilState,"dodge window opens and closes"); std::fflush(stdout); std::_Exit(0);
    }
    if(test=="sneak" || test=="sneak-reject" || test=="sneak-vm") {
        core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"sword",0.0,0,0);
        a->SetFightMode(false,false); require(a->SetPriorityAction("ambush_punch"),"sneak priority action accepted");
        a->UpdateAnimation(); bool cross=false; float closest=10;
        for(int i=0;i<(test=="sneak-vm"?31:80);i++) {
            static_cast<AnimationImp *>(ani)->Execute(16); a->UpdateUnarmedPose(0.016f);
            if(a->unarmedAction=="unarmed_cross") {
                cross=true; const float distance=std::fabs(ani->Player(0).GetPosition()-10.0f/34.0f);
                if(distance<closest) closest=distance;
            }
        }
        require(cross && closest<0.04f,"sneak cross pose reaches authored strike phase");
        if(test=="sneak-vm") {
            core.Event("ReplaySneak");
            const float hp=variable("Victim")->GetAClass()->FindAClass(variable("Victim")->GetAClass(),"chr_ai")->GetAttributeAsFloat("hp");
            std::printf("SNEAK_VM cross=%d hp=%.1f physical=%d\n",cross,hp,b->IsPhysicalRagdoll()); std::fflush(stdout);
            require(hp<500 && b->IsPhysicalRagdoll(),"dialog script causes damage and ragdoll"); std::_Exit(0);
        }
        if(test=="sneak-reject") b->isDialog=true;
        const auto accepted=core.Send_Message(b->GetId(),"ls",MSG_CHARACTER_EX_MSG,"TriggerRagdollFall");
        const bool physical=b->IsPhysicalRagdoll();
        std::printf("SNEAK case=%s cross=%d base_phase_error=%.6f ragdoll_accepted=%llu physical=%d\n",test.c_str(),cross,closest,(unsigned long long)accepted,physical);
        require(test=="sneak" ? (accepted && physical) : (!accepted && !physical),"native ragdoll admission");
        if(physical) { auto *world=location->ExistingRagdollWorld(); require(world && world->Size()==1,"one ragdoll"); location->StepRagdolls(0.016f); }
        std::fflush(stdout); std::_Exit(0);
    }
    if(test=="sword-block-unarmed" || test=="sword-block-armed") {
        core.Send_Message(b->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,test=="sword-block-unarmed" ? "unarmed" : "sword",0.0,0,0);
        b->Block(); b->UpdateAnimation(); require(b->fgtCurType==Character::fgt_block,"defender entered block");
    }
    if(test=="armed" || test=="block-sword" || test=="block-null" || test=="sword-block-unarmed" || test=="sword-block-armed") {
        core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"sword",0.0,0,0);
        require(!a->isFightWOWps,"sword clears unarmed");
    }
    if(test=="block-sword" || test=="block-null") {
        auto *weapon=core.Event("eGetWeaponID","s",a->characterID);
        std::printf("weapon_handler=%s\n",weapon ? "present" : "null");
        require(bool(weapon)==(test=="block-sword"),"requested handler state");
        a->Block(); std::printf("block_requested=%d\n",int(a->fgtSetType)); std::fflush(stdout);
        a->UpdateAnimation(); std::printf("block_current=%d\n",int(a->fgtCurType));
        require(a->fgtCurType==Character::fgt_block,"block selected"); std::fflush(stdout); std::_Exit(0);
    }
    if(test=="fast-blocked") {
        b->Block(); b->UpdateAnimation();
        require(b->fgtCurType==Character::fgt_block,"defender entered block");
    }
    const Character::FightAction action=test=="force" ? Character::fgt_attack_force : test=="break" ? Character::fgt_attack_break : Character::fgt_attack_fast;
    a->Attack(b,action);
    std::printf("requested=%s set=%d index=%d\n",test.c_str(),int(a->fgtSetType),a->fgtSetIndex); std::fflush(stdout);
    require(a->fgtSetType==action,"native Attack accepted requested action");
    a->UpdateAnimation();
    std::printf("pose=%s pending=%d current=%d\n",a->unarmedAction.c_str(),a->unarmedStrikePending,int(a->fgtCurType)); std::fflush(stdout);
    if(test!="armed" && test!="sword-block-unarmed" && test!="sword-block-armed") {
        const char *pose=test=="force"?"unarmed_cross":test=="break"?"unarmed_uppercut":"unarmed_jab";
        require(a->unarmedAction==pose && a->unarmedStrikePending,"selected expected pose bank");
    } else require(!a->unarmedStrikePending && !a->unarmedBank,"armed attack has no fist bank");
    for(int i=0;i<240;i++) {
        int before=integer("Hits"); static_cast<AnimationImp *>(ani)->Execute(16);
        if(integer("Hits")>before) {
            float sampled=a->unarmedBank ? a->unarmedBank->Player(0).GetPosition() : -1.0f;
            std::printf("CONTACT frame=%d sampled=%.6f expected=%.6f\n",i,sampled,a->unarmedContactPhase);
            if(test!="armed" && test!="sword-block-unarmed" && test!="sword-block-armed") {
                require(std::fabs(sampled-a->unarmedContactPhase)<0.0001f,"damage uses contact pose");
                std::vector<CMatrix> actual;
                for(int j=0;j<ani->GetNumBones();j++) actual.push_back(ani->GetAnimationMatrix(j));
                ani->ClearPoseOverride(); float delta=0;
                for(int j=0;j<ani->GetNumBones();j++) for(int x=0;x<4;x++) for(int y=0;y<4;y++)
                    delta+=std::fabs(actual[j].m[x][y]-ani->GetAnimationMatrix(j).m[x][y]);
                std::printf("CONTACT pose_matrix_delta=%.6f bones=%d\n",delta,ani->GetNumBones());
                require(delta>0.01f,"contact palette differs from base animation"); a->SampleUnarmedPose(*ani,sampled);
            }
        }
        a->UpdateAnimation(); a->UpdateUnarmedPose(1.0f/60.0f);
    }
    const int hits=integer("Hits");
    const float hp=variable("Victim")->GetAClass()->FindAClass(variable("Victim")->GetAClass(),"chr_ai")->GetAttributeAsFloat("hp");
    const int blocked=integer("BlockedHits");
    std::printf("RESULT case=%s hits=%d blocked=%d hp=%.1f pose=%s no_window=%d\n",test.c_str(),hits,blocked,hp,a->unarmedAction.c_str(),core.GetWindow()==nullptr); std::fflush(stdout);
    require(hits==1 && (test=="sword-block-unarmed" ? (blocked==1 && hp<500) : test=="sword-block-armed" ? (blocked==1 && hp==500) : (blocked==0 && hp<500)),"single hit and expected block damage"); std::_Exit(0);
} catch (const std::exception &e) { std::fprintf(stderr,"EXCEPTION %s\n",e.what()); return 2; }

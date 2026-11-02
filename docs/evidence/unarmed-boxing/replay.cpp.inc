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
    if(test=="armed" || test=="block-sword" || test=="block-null") {
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
    if(test!="armed") {
        const char *pose=test=="force"?"unarmed_cross":test=="break"?"unarmed_uppercut":"unarmed_jab";
        require(a->unarmedAction==pose && a->unarmedStrikePending,"selected expected pose bank");
    } else require(!a->unarmedStrikePending && !a->unarmedBank,"armed attack has no fist bank");
    for(int i=0;i<240;i++) {
        int before=integer("Hits"); static_cast<AnimationImp *>(ani)->Execute(16);
        if(integer("Hits")>before) {
            float sampled=a->unarmedBank ? a->unarmedBank->Player(0).GetPosition() : -1.0f;
            std::printf("CONTACT frame=%d sampled=%.6f expected=%.6f\n",i,sampled,a->unarmedContactPhase);
            if(test!="armed") {
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
    require(hits==1 && (test=="fast-blocked" ? (blocked==1 && hp==500) : (blocked==0 && hp<500)),"single hit and expected block damage"); std::_Exit(0);
} catch (const std::exception &e) { std::fprintf(stderr,"EXCEPTION %s\n",e.what()); return 2; }

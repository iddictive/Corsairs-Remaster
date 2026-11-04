    location->supervisor.Update(0.0f);
    auto *ani=static_cast<AnimationImp *>(a->Model()->GetAnimation());
    auto *bani=static_cast<AnimationImp *>(b->Model()->GetAnimation());
    auto *controls=static_cast<ReplayControls *>(core.Controls);
    auto ai=[](Character *c) { return c->AttributesPointer->FindAClass(c->AttributesPointer,"chr_ai"); };
    const auto hp=[&] { return ai(b)->GetAttributeAsFloat("hp"); };
    const auto energy=[&] { return ai(a)->GetAttributeAsFloat("energy"); };
    const auto button=[&](const char *name,CONTROL_STATE_TYPE state) {
        controls->states[name]={state,state==CST_ACTIVATED||state==CST_ACTIVE ? 1.0f:0.0f,
            state==CST_ACTIVATED||state==CST_ACTIVE ? 1:0};
    };
    const auto step=[&] {
        a->Player::Move(1.0f/60.0f);
        a->UpdateAnimation(); a->UpdateUnarmedPose(1.0f/60.0f);
        if (!b->IsRoundhouseControlled()) { b->UpdateAnimation(); b->UpdateUnarmedPose(1.0f/60.0f); }
        ani->Execute(16); bani->Execute(16);
        location->StepRagdolls(1.0f/60.0f); b->UpdateRoundhouse(1.0f/60.0f);
    };
    core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"unarmed",0.0,0,0);
    a->SetFightMode(true,false);
    a->task.task=NPCharacter::npct_none; a->command.cmd=AICharacter::aicmd_none;
    b->task.task=NPCharacter::npct_none; b->command.cmd=AICharacter::aicmd_none;
    ai(a)->SetAttributeUseFloat("energy",100.0f);
    ai(b)->SetAttributeUseFloat("energyMax",100.0f);
    ai(b)->SetAttributeUseFloat("energy",test=="high-victim"||test=="uppercut"?100.0f:20.0f);
    require(a->FindAttackCharacter()==b,"actual Player target lookup selects victim");
    const float initialHP=hp(),initialEnergy=energy();
    if(test.rfind("guard-",0)==0) {
        const bool armed=test=="guard-armed";
        if(armed) core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"sword",0.0,0,0);
        button("ChrBlock",CST_ACTIVATED); step();
        require(a->fgtCurType==Character::fgt_block,"actual Player::Move enters block");
        button("ChrBlock",CST_ACTIVE);
        const char *direction=test=="guard-right"?"ChrStrafeRight":test=="guard-back"?"ChrBackward":"ChrStrafeLeft";
        button(direction,CST_ACTIVATED); step();
        const auto action=test=="guard-right"?Character::fgt_strafe_r:test=="guard-back"?Character::fgt_recoil:Character::fgt_strafe_l;
        std::printf("GUARD_ACTIVATION case=%s current=%d expected=%d energy=%.6f\n",test.c_str(),a->fgtCurType,action,energy());
        require(a->fgtCurType==(armed?Character::fgt_block:action),"held guard admits unarmed fresh dodge and preserves armed state");
        button(direction,CST_INACTIVE);
        bool naturalEnd=false;
        for(int frame=0;frame<180;++frame) {
            step();
            if(a->fgtCurType==Character::fgt_none) naturalEnd=true;
            if(a->fgtCurType==Character::fgt_block && (!armed||frame>2)) {
                require(armed||naturalEnd,"guard resumes after natural dodge end");
                require(energy()>=97.99f && energy()<initialEnergy,"ordinary dodge energy spent once");
                require(integer("Hits")==0 && hp()==initialHP,"dodge causes no attack damage");
                std::printf("RESULT case=%s restored_guard=1 frames=%d hits=%d energy=%.6f no_window=%d\n",test.c_str(),frame,integer("Hits"),energy(),core.GetWindow()==nullptr);
                std::fflush(stdout); std::_Exit(0);
            }
        }
        require(false,"guard restored within ordinary animation deadline");
    }
    const bool blocked=test=="blocked";
    const bool miss=test=="miss";
    const bool triple=test=="double-jab"||test=="uppercut";
    const bool cancellation=test=="swap"||test=="dialogue"||test=="death";
    if(blocked) {b->Block();b->UpdateAnimation();require(b->fgtCurType==Character::fgt_block,"victim guard seeded");}
    if(miss) {b->curPos=a->curPos+CVECTOR(0.0f,0.0f,4.0f);b->oldPos=b->curPos;b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);}
    button("ChrAttackFast",CST_ACTIVATED); step(); button("ChrAttackFast",CST_INACTIVE);
    require(a->fgtCurType==Character::fgt_attack_fast && a->unarmedStrikePending,"real Player input starts jab");
    int queued=0,starts=1,previous=Character::fgt_attack_fast;
    int lastStartFrame=0,secondStartFrame=-1;
    bool sawFall=false,sawGetup=false,recovered=false,cancelled=false;
    for(int frame=0;frame<900;++frame) {
        if(a->unarmedStrikeDelivered && queued < starts && queued < (triple?2:1) && !cancelled) {
            if(blocked && queued==0) b->StopFightAnimation();
            if(miss && queued==0) {b->curPos=a->curPos+CVECTOR(0,0,1.5f);b->oldPos=b->curPos;b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);}
            const char *control=triple && queued==0 && test=="double-jab"?"ChrAttackFast":queued==1 && test=="uppercut"?"ChrAttackBreak":"ChrAttackForce";
            const auto expected=std::string(control)=="ChrAttackFast"?Character::fgt_attack_fast:std::string(control)=="ChrAttackBreak"?Character::fgt_attack_break:Character::fgt_attack_force;
            if(test=="uppercut" && queued==1) ai(b)->SetAttributeUseFloat("energy",20.0f);
            button(control,CST_ACTIVATED);
            a->Player::Move(1.0f/60.0f); button(control,CST_INACTIVE);
            require(a->unarmedBufferedAttack==expected,"real input buffered during natural recovery");
            std::printf("BUFFER frame=%d stage=%d current=%d next=%d base_phase=%.6f hits=%d\n",frame,queued,a->fgtCurType,expected,ani->Player(0).GetPosition(),integer("Hits"));
            ++queued;
            if(cancellation) {
                if(test=="swap") core.Send_Message(a->GetId(),"lsfll",MSG_CHARACTER_SETBLADE,"sword",0.0,0,0);
                else if(test=="dialogue") a->isDialog=true;
                else a->liveValue=-1;
                a->UpdateUnarmedPose(1.0f/60.0f); cancelled=true;
                require(a->unarmedBufferedAttack==Character::fgt_none,"runtime cancellation clears queued punch");
            }
        }
        const auto before=a->fgtCurType;
        const bool wasDelivered=a->unarmedStrikeDelivered;
        step();
        if((a->fgtCurType==Character::fgt_attack_fast||a->fgtCurType==Character::fgt_attack_force||a->fgtCurType==Character::fgt_attack_break) &&
           ((int(a->fgtCurType)!=previous)|| (wasDelivered&&!a->unarmedStrikeDelivered))) {
            ++starts;previous=a->fgtCurType;lastStartFrame=frame;if(secondStartFrame<0)secondStartFrame=frame;
            std::printf("START frame=%d ordinal=%d current=%d hits=%d\n",frame,starts,a->fgtCurType,integer("Hits"));
        }
        if(b->IsPhysicalRagdoll()) {
            sawFall=true;require(location->ExistingRagdollWorld() && location->ExistingRagdollWorld()->Size()==1,"actual Jolt body exists");
        }
        if(b->roundhouseState && b->roundhouseState->phase==RoundhouseState::Phase::GetUp)sawGetup=true;
        if(sawFall && !b->IsRoundhouseControlled()) {recovered=true;break;}
        if(!sawFall && frame>240)break;
    }
    const int expectedHits=cancellation||miss?1:triple?3:2;
    const bool shouldFall=!cancellation&&!blocked&&!miss&&test!="high-victim";
    std::printf("RESULT case=%s hits=%d expected_hits=%d starts=%d queued=%d hp=%.6f energy=%.6f fall=%d getup=%d recovered=%d second_start=%d last_start=%d no_window=%d\n",test.c_str(),integer("Hits"),expectedHits,starts,queued,hp(),energy(),sawFall,sawGetup,recovered,secondStartFrame,lastStartFrame,core.GetWindow()==nullptr);
    require(integer("Hits")==expectedHits,"one damage event per actual punch");
    require(hp()<initialHP && energy()<initialEnergy,"ordinary damage and energy consumer ran");
    require(sawFall==shouldFall,"knockout requires connected combo and low victim energy");
    if(shouldFall)require(sawGetup && recovered && hp()>0,"living physical knockout follows ordinary getup lifecycle");
    if(!cancellation)require(starts==(triple?3:2) && secondStartFrame>0,"natural animation ending consumes buffered next attack");
    std::fflush(stdout); std::_Exit(0);
} catch (const std::exception &e) { std::fprintf(stderr,"EXCEPTION %s\n",e.what()); return 2; }

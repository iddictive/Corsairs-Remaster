    location->supervisor.Update(0.0f);
    auto *world=location->GetRagdollWorld(); require(world,"world");
    std::vector<CVECTOR> triangles;
    auto floor=[&](float x0,float x1,float y){float z=b->curPos.z;triangles.insert(triangles.end(),{CVECTOR(x0,y,z-50),CVECTOR(x0,y,z+50),CVECTOR(x1,y,z+50),CVECTOR(x0,y,z-50),CVECTOR(x1,y,z+50),CVECTOR(x1,y,z-50)});};
    const CVECTOR launch=b->curPos;
    floor(launch.x-50,test=="floor" ? launch.x+50 : launch.x+.2f,launch.y); if(test!="void") floor(launch.x-50,launch.x+50,launch.y-(test=="shallow" ? 1.0f : 3.0f));
    require(world->SetStaticTriangles(triangles),"two level floor");
    if(test=="immortal") b->AttributesPointer->CreateSubAClass(b->AttributesPointer,"chr_ai")->SetAttributeUseDword("immortal",1);
    require(b->BeginRoundhouseFall(launch+CVECTOR(0,1.5f,0),CVECTOR(250.0f,20.0f,0.0f),a->GetId()),"fall admission");
    float lowest=launch.y;
    for(int frame=0;frame<540;++frame){
      location->StepRagdolls(1.0f/60);b->UpdateRoundhouse(1.0f/60);
      if(b->roundhouseState && !b->roundhouseState->pose.empty()) {
        const auto p=b->roundhouseState->pose[b->roundhouseState->rig.bodies[0].joint].Pos();lowest=std::min(lowest,p.y);
        if(frame%60==59)std::printf("t=%g phase=%d pelvisY=%g rootY=%g launchY=%g\n",(frame+1)/60.0,int(b->roundhouseState->phase),p.y,b->curPos.y,launch.y);
      }
    }
    const float hp=b->AttributesPointer->FindAClass(b->AttributesPointer,"chr_ai")->GetAttributeAsFloat("hp");
    std::printf("RESULT dead=%d hp=%g phase=%d handles=%zu rootY=%g\n",b->IsDead(),hp,int(b->roundhouseState->phase),world->Size(),b->curPos.y);
    require((test=="deep" || test=="void") ? b->IsDead() && hp==0 && b->roundhouseState->phase==RoundhouseState::Phase::Falling && b->curPos.y<launch.y-2 : !b->IsDead(),"fall death and protection policy");
    if(test=="floor" || test=="immortal") require(b->roundhouseState->phase==RoundhouseState::Phase::Idle && world->Size()==0,"living recovery cleanup");
    if(test=="void") require(world->Size()==0,"void corpse body retired");
    std::printf("lowest=%g launch=%g final=%g\n",lowest,launch.y,b->curPos.y);std::fflush(stdout);std::_Exit(0);
}catch(const std::exception &e){std::fprintf(stderr,"FAIL exception %s\n",e.what());return 1;}

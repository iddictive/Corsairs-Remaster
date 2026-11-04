    b->curPos=b->oldPos=initial+CVECTOR(0.0f,0.0f,1.5f);
    b->Model()->mtx.BuildMatrix(CVECTOR(0,b->ay,0),b->curPos);
    auto *left=actor("VictimLeft",*location,initial+CVECTOR(-1.299038f,0,.75f),true,3.14159265f);
    auto *right=actor("VictimRight",*location,initial+CVECTOR(1.299038f,0,.75f),true,3.14159265f);
    location->supervisor.Update(0.0f);
    for(auto *target : {left,b,right}) std::printf("START %s x=%g z=%g\n",target->characterID,target->curPos.x-a->curPos.x,target->curPos.z-a->curPos.z);
    require(a->StartRoundhouse(true),"aerial kick starts");
    const auto start=std::clock();
    for(int i=0;i<140;++i) {location->StepRagdolls(1.0f/60);location->supervisor.Update(1.0f/60);}
    int total=0;
    for(auto *target : {left,b,right}) {
      const int hits=target->AttributesPointer->GetAttributeAsDword("ReplayHits",0);
      const float hp=target->AttributesPointer->FindAClass(target->AttributesPointer,"chr_ai")->GetAttributeAsFloat("hp");
      total+=hits;std::printf("TARGET %s hits=%d hp=%g physical=%d\n",target->characterID,hits,hp,target->IsPhysicalRagdoll());
    }
    std::printf("TOTAL %d cpu_ms=%g\n",total,1000.0*(std::clock()-start)/CLOCKS_PER_SEC);
    require(total==3,"circular kick reaches all three normally spaced targets"); for(auto *target : {left,b,right}) require(target->AttributesPointer->GetAttributeAsDword("ReplayHits",0)==1,"each target takes damage once");std::fflush(stdout);std::_Exit(0);
}catch(const std::exception &e){std::fprintf(stderr,"FAIL %s\n",e.what());return 2;}

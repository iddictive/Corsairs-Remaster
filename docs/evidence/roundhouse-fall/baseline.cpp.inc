    location->supervisor.Update(0.0f);
    auto *world=location->GetRagdollWorld(); require(world,"world");
    std::vector<CVECTOR> triangles;
    auto floor=[&](float x0,float x1,float y){float z=b->curPos.z;triangles.insert(triangles.end(),{CVECTOR(x0,y,z-50),CVECTOR(x0,y,z+50),CVECTOR(x1,y,z+50),CVECTOR(x0,y,z-50),CVECTOR(x1,y,z+50),CVECTOR(x1,y,z-50)});};
    const CVECTOR launch=b->curPos;
    floor(launch.x-50,launch.x+.2f,launch.y); floor(launch.x-50,launch.x+50,launch.y-3);
    require(world->SetStaticTriangles(triangles),"two level floor");
    require(b->BeginRoundhouseFall(launch+CVECTOR(0,1.5f,0),CVECTOR(250.0f,20.0f,0.0f)),"fall admission");
    float lowest=launch.y;
    for(int frame=0;frame<540;++frame){
      location->StepRagdolls(1.0f/60);b->UpdateRoundhouse(1.0f/60);
      if(b->roundhouseState && !b->roundhouseState->pose.empty()) {
        const auto p=b->roundhouseState->pose[b->roundhouseState->rig.bodies[0].joint].Pos();lowest=std::min(lowest,p.y);
        if(frame%60==59)std::printf("t=%g phase=%d pelvisY=%g rootY=%g launchY=%g\n",(frame+1)/60.0,int(b->roundhouseState->phase),p.y,b->curPos.y,launch.y);
      }
    }
    std::printf("lowest=%g launch=%g final=%g\n",lowest,launch.y,b->curPos.y);std::fflush(stdout);std::_Exit(0);
}catch(const std::exception &e){std::fprintf(stderr,"FAIL exception %s\n",e.what());return 1;}

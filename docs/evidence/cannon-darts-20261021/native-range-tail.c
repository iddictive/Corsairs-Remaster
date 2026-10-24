    CreateEntity(&BenchAI,"sea_ai");
    CreateEntity(&AIBalls,"AIBalls");
    LayerAddObject(2,&AIBalls,-1);
    LayerAddObject(3,&AIBalls,65532);
    AIBalls.SpeedMultiply=2.0;
    AIBalls.Texture="AllBalls.tga";
    AIBalls.DartTexture="BALLS\\dart.tga";
    AIBalls.DartSize=0.8;
    AIBalls.SubTexX=2;
    AIBalls.SubTexY=2;
    AIBalls.Balls.Grapes.GoodIndex=1;
    AIBalls.Balls.Grapes.Size=0.055;
    AIBalls.Balls.Grapes.SubTexIndex=1;
    AIBalls.Balls.Knippels.GoodIndex=2;
    AIBalls.Balls.Knippels.Size=0.24;
    AIBalls.Balls.Knippels.SubTexIndex=3;
    AIBalls.Balls.Balls.GoodIndex=0;
    AIBalls.Balls.Balls.Size=0.2;
    AIBalls.Balls.Balls.SubTexIndex=2;
    AIBalls.Balls.Bombs.GoodIndex=3;
    AIBalls.Balls.Bombs.Size=0.3;
    AIBalls.Balls.Bombs.SubTexIndex=0;
    AIBalls.isDone=1;
    InitRangeSource();
    SetEventHandler("frame","RangeFrame",0);
    SetEventHandler("Shp_HullHit","RangeHullHit",0);
    SetEventHandler("ShpMastDamage","RangeNoDamage",0);
    SetEventHandler("ShpHullDamage","RangeNoDamage",0);
    BenchmarkReady=1;
}

float RangeNoDamage() { return 0.0; }

void RangeHullHit()
{
    aref nativeShip=GetEventData();
    int owner=GetEventData();
    int receiver=GetEventData();
    if (sti(AIBalls.CurrentBallType)!=52) RangeBad++;
    if (owner==1000 && receiver==0) RangeHits0++;
    else if (owner==1001 && receiver==1) RangeHits1++;
    else RangeBad++;
}

void RangeVolley(int kind)
{
    int shooter=kind+2;
    int cannonIndex=kind+14;
    float offset=kind*200.0;
    float distance=stf(Fleet[kind].ship.pos.x);
    Fleet[shooter].Ship.Cannons.Type=cannonIndex;
    Fleet[shooter].Ship.Cannons.Charge.Type=52;
    Fleet[shooter].TmpSkill.Accuracy=1.0;
    Fleet[shooter].TmpSkill.Cannons=1.0;
    Fleet[shooter].perks.list.GunProfessional=1;
    Fleet[shooter].perks.list.LongRangeShoot=1;
    Fleet[shooter].TmpPerks.LongRangeShoot=1;
    float speed=stf(Cannon[cannonIndex].SpeedV0)*stf(Goods[52].SpeedV0)*1.15;
    for (int volley=0; volley<20; volley++) {
        float aimY=8.6+(frnd()-0.5)*4.3*0.3;
        for (int gun=0; gun<51; gun++) {
            float x=stf(RangeGun[gun].x);
            float y=stf(RangeGun[gun].y);
            float z=stf(RangeGun[gun].z)+offset;
            float aimZ=-35.790977478+gun*(69.490001679/50.0)+offset;
            float dx=distance-x;
            float dz=aimZ-z;
            float dy=aimY-y;
            float horizontal=sqrt(dx*dx+dz*dz);
            float b=9.81*dy-speed*speed;
            float disc=b*b-9.81*9.81*(horizontal*horizontal+dy*dy);
            float flight=sqrt((-b-sqrt(disc))/(9.81*9.81/2.0));
            float elevation=atan2(dy+0.5*9.81*flight*flight,horizontal);
            float yaw=atan2(dx,dz);
            float raw=atan2(dy,horizontal);
            Ball_AddBall(&Fleet[shooter],x,y,z,speed,yaw,elevation,0.0,distance,raw);
        }
    }
}

void RangeFrame()
{
    BenchFrames++;
    if (BenchFrames==2) {
        // Keep real mounted model/BSP receivers at one supported stationary pose.
        for (int i=0;i<4;i++) LayerDelObject(2,&Fleet[i]);
    }
    if (BenchFrames==4) {
        BenchmarkSeed(20261021);
        RangeVolley(0);
        RangeVolley(1);
        AIBalls.queryActiveCount=1;
        if (sti(AIBalls.activeCount)!=2040) RangeBad++;
    }
    if (BenchFrames==512) {
        // Existing VM serializer, task-local file; never touches the player SAVE.
        int snapshot=SaveEngineState("range-state.bin");
        if (snapshot==0) RangeBad++;
        if (RangeBad!=0 || RangeHits0<663 || RangeHits0>765 || RangeHits1<663 || RangeHits1>765)
            DeleteEntitiesByType("Ship");
        else
            DeleteClass(&Fleet[7]);
    }
}

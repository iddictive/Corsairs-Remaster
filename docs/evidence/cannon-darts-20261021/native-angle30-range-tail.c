void FinalVolley(int kind)
{
    int shooter=kind+2;
    float offset=kind*200.0;
    float distance=stf(Fleet[kind].ship.pos.x);
    FinalProfile(shooter,kind);
    float velocity=stf(Fleet[shooter].Ship.Cannons.SpeedV0);
    for (int volley=0;volley<20;volley++) {
        float aimY=8.6+(frnd()-0.5)*4.3*0.3;
        for (int ordinal=0;ordinal<51;ordinal++) {
            float aimZ=-35.790977478+ordinal*(69.490001679/50.0)+offset;
            if (DartNativeQueue(&Fleet[shooter],ordinal,velocity,distance,aimY,aimZ,-100.0,0)!=1) RangeBad++;
        }
    }
}
void FinalFrame()
{
    BenchFrames++;
    if (BenchFrames==2) {
        for (int actor=0;actor<4;actor++) LayerDelObject(2,&Fleet[actor]);
    }
    if (BenchFrames==4) {
        BenchmarkSeed(20261021);
        FinalVolley(0);
        FinalVolley(1);
        AIBalls.queryActiveCount=1;
        if (sti(AIBalls.activeCount)!=2040) RangeBad++;
    }
    if (BenchFrames==512) {
        int snapshot=SaveEngineState("range-state.bin");
        if (snapshot==0 || RangeBad!=0 || RangeHits0<663 || RangeHits0>765 || RangeHits1<663 || RangeHits1>765)
            DeleteEntitiesByType("Ship");
        else DeleteClass(&Fleet[7]);
    }
}

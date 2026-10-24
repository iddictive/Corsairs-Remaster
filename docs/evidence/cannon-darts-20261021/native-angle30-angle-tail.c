void FinalFrame()
{
    BenchFrames++;
    if (BenchFrames==2) { LayerDelObject(2,&Fleet[2]); }
    if (BenchFrames==4) {
        AngleMode=1;
        FinalProfile(2,0);
        float velocity=stf(Fleet[2].Ship.Cannons.SpeedV0);
        for (int shot=0;shot<6;shot++) {
            float degrees=20.0;
            if (shot==1) degrees=30.0;
            if (shot==2) degrees=40.0;
            if (shot==3) degrees=60.0;
            if (shot>=4) degrees=80.0;
            int codec=0;
            if (shot==5) codec=1;
            float beta=atan(tan(Degree2Radian(degrees))/0.5773502692);
            if (DartNativeQueue(&Fleet[2],0,velocity,-5000.0,10.0,-27.4,beta,codec)!=1) AngleBad++;
        }
    }
    if (BenchFrames>=4) DartNativeSample();
    if (BenchFrames==512) {
        AIBalls.queryActiveCount=1;
        if (AngleFires!=6 || AngleBad!=0 || AngleLoaded!=1 || RangeBad!=0 || sti(AIBalls.activeCount)!=0) RangeBad++;
        int snapshot=SaveEngineState("range-state.bin");
        if (snapshot==0 || RangeBad!=0) DeleteEntitiesByType("Ship");
        else DeleteClass(&Fleet[7]);
    }
}

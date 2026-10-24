// Isolated diagnostic bootstrap: no main game, menus, SAVE or user controls.
native float Bring2RangeNoCheck(float fMin1, float fMax1, float fMin2, float fMax2, float fValue);
native float Bring2Range(float fMin1, float fMax1, float fMin2, float fMax2, float fValue);
native float Clampf(float fValue);
native float Degree2Radian(float fDegree);
native int RDTSC_B();
native int RDTSC_E(int iRDTSC);
native int SetTexturePath(int iLevel, string sPath);
native int SetGlowParams(float fBlurBrushSize, int Intensivity, int BlurPasses);
native int RPrint(int x, int y, string sPrint);
native int GetTexture(string fileName);
native void ReleaseTexture(int texId);
void BenchmarkSeed(int seed) {}
int DartNativeQueue(aref actor, int gunOrdinal, float velocity, float targetX, float targetY, float targetZ, float rawBeta, int codec) { return 0; }
void DartNativeSample() {}
void DartNativeLegacy() {}

#libriary "script_libriary_test"
#libriary "dx9render_script_libriary"

object Render, Sea, Weather, Sky, ShipLights;
object BenchAI, AIBalls;
int BenchFrames=0;
object BenchSeaSave, BenchCameraData;
int nMainCharacterIndex=999;
int RangeHits0=0, RangeHits1=0, RangeBad=0;
int AngleMode=0, AngleFires=0, AngleBad=0, AngleLoaded=0;
object Sail, Rope, Flag, Vant, VantL, VantZ, Sailors;
object ShipType;
object Weathers[32];
object Fleet[16];
object Cloth;
object WeatherState;
object Seafoam;
int iScriptVersion = 54128;
int FleetCount = 8;
int BenchmarkReady = 0;
int iShipPriorityExecute = 2;
int iShipPriorityRealize = 31;
#include "ship-type.c"
#include "ship-lights.c"
#include "day.c"
#include "native-angle30-source.c"

ref GetCloth()
{
    return &Cloth;
}

void FixtureMain()
{
    Cloth.normalTex = "ships\\parus_common.tga";
    Cloth.sailscolor = argb(255,255,255,255);
    Cloth.MaxSP = 100;
    SetEventHandler("GetSailTextureData", "GetCloth", 0);
    InitShipType();
    Whr_InitDay(0);
    Render.BackColor = 4278652691;
    // Reflection lists are traversed only by SEA, never as main-frame layers.

    CreateEntity(&Weather, "Weather");
    LayerAddObject(2, &Weather, 1000000);
    LayerAddObject(3, &Weather, 65530);
    Weather.Time.time = 12.0;
    Weather.Time.speed = 0.0;
    Weather.Wind.Speed = 12.0;
    Weather.Wind.Angle = 0.6;
    Weather.Sun.Color = Weathers[1].Sun.Color;
    Weather.Sun.Ambient = Weathers[1].Sun.Ambient;
    Weather.Sun.Height = 1.1;
    Weather.Sun.BegTime = 5.5;
    Weather.Sun.EndTime = 19.0;
    Weather.Sun.BegAngle = 0.0;
    Weather.Sun.EndAngle = 3.14159265;
    Weather.Fog.Enable = Weathers[1].Fog.Enable;
    Weather.Fog.Color = Weathers[1].Fog.Color;
    Weather.Fog.Density = Weathers[1].Fog.Density;
    Weather.Fog.Start = Weathers[1].Fog.Start;
    Weather.Sun.HeightAngle = Weathers[1].Sun.HeightAngle;
    Weather.Sun.AzimuthAngle = Weathers[1].Sun.AzimuthAngle;
    Weather.isDone = "";

    CreateEntity(&Sky, "Sky");
    LayerAddObject(15, &Sky, 1);
    LayerAddObject(3, &Sky, 1);
    Sky.dir.d1 = Weathers[1].Sky.Dir;
    Sky.dir = 12.0;
    Sky.Color = Weathers[1].Sky.Color;
    Sky.RotateSpeed = 0.0;
    Sky.Angle = Weathers[1].Sky.Angle;
    Sky.Size = Weathers[1].Sky.Size;
    Sky.FogHeight = Weathers[1].Fog.Height;
    Sky.techSky = "Sky";
    Sky.techSkyBlend = "SkyBlend";
    Sky.techSkyAlpha = "skyblend_alpha";
    Sky.techSkyFog = "SkyFog";
    Sky.isDone = "";

    CreateEntity(&Sea, "sea");
    LayerAddObject(2, &Sea, 4);
    LayerAddObject(3, &Sea, 65530);
    Sea.Clear = "";
    Sea.Sea2.WaterColor = Weathers[1].Sea2.WaterColor;
    Sea.Sea2.SkyColor = Weathers[1].Sea2.SkyColor;
    Sea.Sea2.Amp1 = Weathers[1].Sea2.Amp1;
    Sea.Sea2.Scale1 = Weathers[1].Sea2.Scale1;
    Sea.Sea2.AnimSpeed1 = Weathers[1].Sea2.AnimSpeed1;
    Sea.Sea2.MoveSpeed1 = Weathers[1].Sea2.MoveSpeed1;
    Sea.Sea2.Amp2 = Weathers[1].Sea2.Amp2;
    Sea.Sea2.Scale2 = Weathers[1].Sea2.Scale2;
    Sea.Sea2.AnimSpeed2 = Weathers[1].Sea2.AnimSpeed2;
    Sea.Sea2.MoveSpeed2 = Weathers[1].Sea2.MoveSpeed2;
    Sea.Sea2.BumpScale = Weathers[1].Sea2.BumpScale;
    Sea.Sea2.Reflection = Weathers[1].Sea2.Reflection;
    Sea.Sea2.Transparency = Weathers[1].Sea2.Transparency;
    Sea.Sea2.Attenuation = Weathers[1].Sea2.Attenuation;
    Sea.Sea2.Frenel = Weathers[1].Sea2.Frenel;
    Sea.Sea2.PosShift = Weathers[1].Sea2.PosShift;
    Sea.Sea2.FoamK = Weathers[1].Sea2.FoamK;
    Sea.Sea2.FoamV = Weathers[1].Sea2.FoamV;
    Sea.Sea2.FoamUV = Weathers[1].Sea2.FoamUV;
    Sea.Sea2.FoamTexDisturb = Weathers[1].Sea2.FoamTexDisturb;
    Sea.Sea2.LodScale = 0.399;
    Sea.Sea2.GridStep = 0.07;
    Sea.Sea2.FoamEnable = 1;
    Sea.MaxSeaHeight = 200.0;
    Sea.Sea2.SimpleSea = 0;
    Sea.isDone = "";
    CreateEntity(&ShipLights, "ShipLights");
    InitShipLights();
    LayerAddObject(2, &ShipLights, 0);
    LayerAddObject(3, &ShipLights, -1);
    LayerAddObject(17, &ShipLights, -1);

    CreateEntity(&Sail, "Sail");
    LayerAddObject(2, &Sail, 3); LayerAddObject(3, &Sail, 32);
    LayerAddObject(19, &Sail, 10); LayerAddObject(16, &Sail, 3);
    CreateEntity(&Rope, "Rope");
    LayerAddObject(2, &Rope, 4); LayerAddObject(3, &Rope, 33);
    CreateEntity(&Flag, "Flag");
    LayerAddObject(2, &Flag, 5); LayerAddObject(3, &Flag, 34);
    LayerAddObject(16, &Flag, 3);
    CreateEntity(&Vant, "Vant");
    CreateEntity(&VantL, "VantL");
    CreateEntity(&VantZ, "VantZ");
    LayerAddObject(2, &Vant, 6); LayerAddObject(3, &Vant, 35);
    LayerAddObject(2, &VantL, 6); LayerAddObject(3, &VantL, 35);
    LayerAddObject(2, &VantZ, 6); LayerAddObject(3, &VantZ, 35);
    CreateEntity(&Sailors, "Sailors");
    LayerAddObject(2, &Sailors, 7); LayerAddObject(3, &Sailors, 36);
    for (int i = 0; i < FleetCount; i++) {
        Fleet[i].index = i;
        if (i==2) Fleet[i].index=1000;
        if (i==3) Fleet[i].index=1001;
        if (i==6) Fleet[i].index=999;
        Fleet[i].nation = 0;
        Fleet[i].MainCharacter = 0;
        if (i == 6) Fleet[i].MainCharacter = 1;
        Fleet[i].ship.pos.x=-3000.0-i*100.0;
        if (i==0) Fleet[i].ship.pos.x=2109.502672267;
        if (i==1) Fleet[i].ship.pos.x=4225.523291676;
        if (i==2 || i==3) Fleet[i].ship.pos.x=0.0;
        Fleet[i].ship.pos.z=0.0;
        if (i==1 || i==3) Fleet[i].ship.pos.z=200.0;
        Fleet[i].ship.ang.y = 0.0;
        Fleet[i].ship.speed.z = 0.0;
        Fleet[i].ship.Crew.Quantity = 830;
        Fleet[i].ship.HP = 10000;
        Fleet[i].ship.SP = 100;
        Fleet[i].ship.lights = 1;
        Fleet[i].ship.flares = 1;
        CreateEntity(&Fleet[i], "Ship");
        // SHIP::Init seeds libc from wall time; diagnostic-only reseeding
        // precedes mounting and crew construction without changing game code.
        BenchmarkSeed(1337 + i);
        SendMessage(&Fleet[i], "lll", 51015, 2, 3);
        // Stock Ship_StartLoad selects the hull texture directory before Mount;
        // this isolated bootstrap has no AIShip event handlers.
        SetTexturePath(0, "ships\\" + ShipType.Name + "\\hull1\\");
        SendMessage(&Fleet[i], "laa", 50300, &Fleet[i], &ShipType);
        SetTexturePath(0, "");
        SendMessage(&Sailors, "lis", 51001, &Fleet[i], "Manowar1");
    }
    CreateEntity(&Seafoam, "Seafoam");
    LayerAddObject(2, &Seafoam, -1);
    LayerAddObject(3, &Seafoam, -1);
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
    SetEventHandler("frame","FinalFrame",0);
    SetEventHandler("CannLoad","FinalLoad",0);
    SetEventHandler("CannGRTime","FinalReload",0);
    SetEventHandler("CannFire","FinalFire",0);
    SetEventHandler("SeaLoad_GetPointer","FinalPointer",0);
    SetEventHandler("Shp_HullHit","FinalHull",0);
    SetEventHandler("ShpMastDamage","FinalNoDamage",0);
    SetEventHandler("ShpHullDamage","FinalNoDamage",0);
    BenchmarkReady=1;
}
int FinalLoad() { return 1; }
float FinalReload() { return 1.0; }
float FinalNoDamage() { return 0.0; }
ref FinalPointer() { string kind=GetEventData(); int index=GetEventData(); return &BenchSeaSave; }
void FinalProfile(int actor,int kind)
{
    Fleet[actor].Ship.Cannons.Type=kind+14;
    Fleet[actor].Ship.Cannons.Charge.Type=52;
    Fleet[actor].TmpSkill.Accuracy=1.0;
    Fleet[actor].TmpSkill.Cannons=1.0;
    Fleet[actor].perks.list.GunProfessional=1;
    Fleet[actor].perks.list.LongRangeShoot=1;
    Fleet[actor].TmpPerks.LongRangeShoot=1;
    Cannon_RecalculateParameters(actor);
    if (sti(Fleet[actor].Ship.Cannons.FixedBallisticFrame)!=1 || abs(stf(Fleet[actor].Ship.Cannons.BallHeightMultiply)-0.5773502692)>0.00001 || abs(stf(Fleet[actor].Ship.Cannons.BallTimeSpeedMultiply)-2.0)>0.00001) RangeBad++;
}
void FinalFire()
{
    aref actor=GetEventData();
    float x=GetEventData();
    float y=GetEventData();
    float z=GetEventData();
    float velocity=GetEventData();
    float yaw=GetEventData();
    float beta=GetEventData();
    float bort=GetEventData();
    float limit=GetEventData();
    float raw=GetEventData();
    if (AngleMode) {
        AngleFires++;
        float degrees=20.0;
        if (AngleFires==2) degrees=30.0;
        if (AngleFires==3) degrees=40.0;
        if (AngleFires==4) degrees=60.0;
        if (AngleFires>=5) degrees=80.0;
        float expected=atan(tan(Degree2Radian(degrees))/0.5773502692);
        if (abs(beta-expected)>0.00001 || abs(raw)>0.000001) AngleBad++;
        if (AngleFires==6 && sti(AIBalls.TargetValid)==0) AngleLoaded=1;
        AIBalls.Type="Balls"; AIBalls.Event="@dart:52";
        AIBalls.CharacterIndex=actor.index;
        AIBalls.x=x; AIBalls.y=y; AIBalls.z=z;
        AIBalls.SpdV0=velocity; AIBalls.Dir=yaw; AIBalls.Ang=beta;
        AIBalls.RawAng=raw;
        AIBalls.HeightMultiply=Ball_GetHeightMultiply(actor);
        AIBalls.SizeMultiply=1.0; AIBalls.TimeSpeedMultiply=2.0;
        AIBalls.CannonType=actor.Ship.Cannons.Type;
        AIBalls.MaxFireDistance=10000.0;
        AIBalls.Add=1;
    } else Ball_AddBall(actor,x,y,z,velocity,yaw,beta,bort,limit,raw);
}
void FinalHull()
{
    aref nativeShip=GetEventData(); int owner=GetEventData(); int receiver=GetEventData();
    if (sti(AIBalls.CurrentBallType)!=52) RangeBad++;
    if (owner==1000 && receiver==0) RangeHits0++;
    else if (owner==1001 && receiver==1) RangeHits1++;
    else RangeBad++;
}
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
void AngleFrameParseOnly()
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

int ParseOnly() { return 0; }

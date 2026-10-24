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
native void BenchmarkSeed(int seed);
native int DartNativeQueue(aref actor, int gunOrdinal, float velocity, float targetX, float targetY, float targetZ, float rawBeta, int codec);
native void DartNativeSample();
native void DartNativeLegacy();
#libriary "DartNativeAcceptanceLibrary"
#libriary "script_libriary_test"
#libriary "dx9render_script_libriary"
#libriary "SeaBenchmarkLibrary"
object Render, Sea, Weather, Sky, ShipLights;
object BenchAI, AIBalls;
int BenchFrames=0;
object BenchSeaSave, BenchCameraData;
int nMainCharacterIndex=999;
int BenchStage=0;
int BenchBad=0;
int BenchOrdinaryHits=0;
int BenchDartHits=0;
int BenchAirburstHits=0;
int BenchLoadedDartHits=0;
int BenchExplosions=0;
int BenchFreshMetadata=0;
int BenchLoadedMetadata=0;
int BenchPhonkChecks=0;
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

ref GetCloth()
{
    return &Cloth;
}

void Main()
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
        if (i==6) Fleet[i].index=999;
        Fleet[i].nation = 0;
        Fleet[i].MainCharacter = 0;
        if (i == 6) Fleet[i].MainCharacter = 1;
        Fleet[i].ship.pos.x = (i - (FleetCount - 2) * 0.5) * 70.0;
        Fleet[i].ship.pos.z = 100.0 + (i % 2) * 100.0;
        Fleet[i].ship.ang.y = 0.1;
        Fleet[i].ship.speed.z = 4.0;
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
    CreateEntity(&BenchAI, "sea_ai");
    CreateEntity(&AIBalls, "AIBalls");
    LayerAddObject(2, &AIBalls, -1);
    LayerAddObject(3, &AIBalls, 65532);
    AIBalls.BallFlySoundDistance=15.0;
    AIBalls.BallFlySoundStereoMultiplyer=2.0;
    AIBalls.SpeedMultiply=2.0;
    AIBalls.Texture="AllBalls.tga";
    AIBalls.DartTexture="BALLS\\dart.tga";
    AIBalls.DartSize=0.8;
    AIBalls.SubTexX=2;
    AIBalls.SubTexY=2;
    AIBalls.Balls.Grapes.SubTexIndex=1;
    AIBalls.Balls.Grapes.Size=0.055;
    AIBalls.Balls.Grapes.GoodIndex=1;
    AIBalls.Balls.Knippels.SubTexIndex=3;
    AIBalls.Balls.Knippels.Size=0.24;
    AIBalls.Balls.Knippels.GoodIndex=2;
    AIBalls.Balls.Balls.SubTexIndex=2;
    AIBalls.Balls.Balls.Size=0.2;
    AIBalls.Balls.Balls.GoodIndex=0;
    AIBalls.Balls.Bombs.SubTexIndex=0;
    AIBalls.Balls.Bombs.Size=0.3;
    AIBalls.Balls.Bombs.GoodIndex=3;
    AIBalls.isDone=1;
    SetEventHandler("frame", "DartContractFrame", 0);
    SetEventHandler("SeaLoad_GetPointer", "DartContractPointer", 0);
    SetEventHandler("Shp_HullHit", "DartContractHull", 0);
    SetEventHandler("ShpMastDamage", "DartContractNoDamage", 0);
    SetEventHandler("ShpHullDamage", "DartContractNoDamage", 0);
    SetEventHandler("Ball_AirburstExplosion", "DartContractExplosion", 0);
    BenchmarkReady=1;
}

ref DartContractPointer()
{
    string type=GetEventData();
    int index=GetEventData();
    if (type=="seasave") return &BenchSeaSave;
    return &BenchCameraData;
}

float DartContractNoDamage()
{
    return 0.0;
}

void DartContractExplosion()
{
    BenchExplosions++;
}

// This callback is emitted only after SHIP::Cannon_Trace gets actual model BSP hit.
void DartContractHull()
{
    aref nativeShip=GetEventData();
    int owner=GetEventData();
    int target=GetEventData();
    float x=GetEventData();
    float y=GetEventData();
    float z=GetEventData();
    if (owner!=999 || target!=0 || x< -225.0 || x> -195.0 || y<0.0 || y>15.0 || z<45.0 || z>145.0) BenchBad++;
    int good=sti(AIBalls.CurrentBallType);
    if (BenchStage==1 && good==0) BenchOrdinaryHits++;
    else if (BenchStage==2 && good==52) BenchDartHits++;
    else if (BenchStage==3 && good==3) BenchAirburstHits++;
    else if (BenchStage==4 && good==52) BenchLoadedDartHits++;
    else BenchBad++;
}

void DartContractFire(string type, string identity)
{
    AIBalls.clear=1;
    AIBalls.Type=type;
    AIBalls.Event=identity;
    AIBalls.CharacterIndex=999;
    AIBalls.x=-210.0;
    AIBalls.y=5.0;
    AIBalls.z=45.0;
    AIBalls.SpdV0=200.0;
    AIBalls.HeightMultiply=0.4;
    AIBalls.SizeMultiply=1.0;
    AIBalls.TimeSpeedMultiply=1.0;
    AIBalls.CannonType=0;
    AIBalls.MaxFireDistance=2000.0;
    AIBalls.Ang=0.0;
    AIBalls.Dir=0.0;
    AIBalls.RawAng=0.0;
    AIBalls.TargetX=-210.0;
    AIBalls.TargetY=5.0;
    AIBalls.TargetZ=100.0;
    AIBalls.TargetIndex=0;
    AIBalls.TargetValid=1;
    AIBalls.add=1;
}

void DartContractFrame()
{
    BenchFrames++;
    if (BenchFrames==4) {
        BenchStage=1;
        AIBalls.clear=1;
        DartNativeLegacy();
    }
    if (BenchFrames==16) {
        AIBalls.queryActiveCount=1;
        if (BenchOrdinaryHits!=1 || sti(AIBalls.activeCount)!=0 || sti(AIBalls.DartView.inFlight)!=0) BenchBad++;
    }
    if (BenchFrames==20) {
        BenchStage=2;
        DartContractFire("Balls", "@dart:52");
    }
    if (BenchFrames==19) {
        Sea.Phonk.Active=1;
        Sea.Phonk.Weapon=0;
    }
    if (BenchFrames==21) {
        if (sti(Sea.Phonk.Weapon)==0) BenchPhonkChecks++;
        if (sti(AIBalls.DartView.inFlight)==1 && sti(AIBalls.DartView.target)==0 && stf(AIBalls.DartView.dz)>0.0 && sti(AIBalls.DartView.sequence)>0)
            BenchFreshMetadata=1;
    }
    if (BenchFrames==36) {
        if (sti(Sea.Phonk.Weapon)==0) BenchPhonkChecks++;
        AIBalls.queryActiveCount=1;
        if (BenchDartHits!=1 || sti(AIBalls.activeCount)!=0 || sti(AIBalls.DartView.hits)!=1 || sti(AIBalls.DartView.inFlight)!=0) BenchBad++;
    }
    if (BenchFrames==38) Sea.Phonk.Active=0;
    // Under-35m contact intentionally preserves the existing unarmed Bombs path.
    if (BenchFrames==40) {
        BenchStage=3;
        DartContractFire("Bombs", "@airburst:51");
    }
    if (BenchFrames==56) {
        AIBalls.queryActiveCount=1;
        if (BenchAirburstHits!=1 || BenchExplosions!=0 || sti(AIBalls.activeCount)!=0 || sti(AIBalls.DartView.inFlight)!=0) BenchBad++;
    }
    if (BenchFrames==60) {
        BenchStage=4;
        DartContractFire("Balls", "@dart:52");
        // Real native CSaveLoad/SEA_AI codec: script-owned memory, no SAVE files.
        SendMessage(&BenchAI, "l", 51042);
        if (!CheckAttribute(&BenchSeaSave, "save")) BenchBad++;
        AIBalls.clear=1;
        SendMessage(&BenchAI, "l", 51043);
        AIBalls.queryActiveCount=1;
        if (sti(AIBalls.activeCount)!=1) BenchBad++;
    }
    if (BenchFrames==61) {
        AIBalls.queryActiveCount=1;
        // CurrentBallType is per-collision scratch, not a persistent shot field.
        if (sti(AIBalls.activeCount)==1 && sti(AIBalls.DartView.inFlight)==0)
            BenchLoadedMetadata=1;
    }
    if (BenchFrames==78) {
        AIBalls.queryActiveCount=1;
        if (BenchLoadedDartHits!=1 || sti(AIBalls.activeCount)!=0 || sti(AIBalls.DartView.inFlight)!=0) BenchBad++;
    }
    if (BenchFrames==159) {
        BenchStage=5;
        DartContractFire("Balls", "@dart:52");
        AIBalls.clear=1;
        AIBalls.y=36.5;
        AIBalls.z=-60.0;
        AIBalls.SpdV0=100.0;
        for (int n=0; n<3; n++) {
            AIBalls.x=(n-1)*4.0;
            AIBalls.Dir=(n-1)*0.9;
            AIBalls.add=1;
        }
    }
    if (BenchFrames==160) {
        AIBalls.queryActiveCount=1;
        // Existing VM serializer, confined to this diagnostic's owned cwd.
        int snapshot=SaveEngineState("dart-state.bin");
        if (snapshot==0) BenchBad++;
        if (BenchBad!=0 || BenchOrdinaryHits!=1 || BenchDartHits!=1 || BenchAirburstHits!=1 || BenchLoadedDartHits!=1 || BenchFreshMetadata!=1 || BenchLoadedMetadata!=1 || BenchPhonkChecks!=2 || sti(AIBalls.activeCount)!=3)
            DeleteEntitiesByType("Ship");
        else
            // A seven-ship acknowledgement proves the final predicate executed.
            DeleteClass(&Fleet[7]);
    }
}

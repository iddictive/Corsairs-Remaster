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
#libriary "script_libriary_test"
#libriary "dx9render_script_libriary"
#libriary "SeaBenchmarkLibrary"
object Render, Sea, Weather, Sky, ShipLights;
object Sail, Rope, Flag, Vant, VantL, VantZ, Sailors;
object ShipType;
object Weathers[32];
object Fleet[16];
object Cloth;
object WeatherState;
object Seafoam;
object SceneEvents;
int iScriptVersion = 54128;
int FleetCount = 2;
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
    SceneEvents.calls=0;
    InitShipType();
    SetEventHandler("Ship_StartLoad","Ship_StartLoad",0);
    SetEventHandler("Ship_EndLoad","Ship_EndLoad",0);
    SetEventHandler("Shp_GetCurBallsNum","SceneBalls",0);
    SetEventHandler("CannLoad","SceneLoad",0);
    SetEventHandler("CannGRTime","SceneRecharge",0);
    SetEventHandler("SeaPhonk_HullCut","SceneHullCut",0);
    SetEventHandler("SeaPhonk_Flood","SceneFlood",0);
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
        Fleet[i].nation = 0;
        Fleet[i].MainCharacter = 0;
        if (i == 0) Fleet[i].MainCharacter = 1;
        Fleet[i].ship.pos.x = (i - (FleetCount - 2) * 0.5) * 70.0;
        Fleet[i].ship.pos.z = 100.0 + (i % 2) * 100.0;
        Fleet[i].ship.ang.y = 0.1;
        Fleet[i].ship.speed.z = 0.0;
        Fleet[i].ship.Crew.Quantity = ShipType.MaxCrew;
        Fleet[i].ship.HP = ShipType.HP;
        Fleet[i].ship.Cannons.SpeedV0 = 300;
        Fleet[i].ship.Cannons.Borts = "";
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
        // Mount and crew construction follow the first CORE frame in the native driver.
    }
    CreateEntity(&Seafoam, "Seafoam");
    LayerAddObject(2, &Seafoam, -1);
    LayerAddObject(3, &Seafoam, -1);
    BenchmarkReady = 1;
}

// Same path selection as installed Ship_StartLoad -> SetTextureForShip for
// this exact known real-ship name/hull; no lookup of player RealShips/SAVE.
void Ship_StartLoad(){SetTexturePath(0,"ships\\"+ShipType.Name+"\\hull1\\");}
void Ship_EndLoad(){SetTexturePath(0,"");}
int SceneBalls(){return 1000;}
int SceneLoad(){return 1;}
float SceneRecharge(){return 20.0;}
int SceneHullCut(){
    aref character=GetEventData();
    int owner=GetEventData();
    float removed=GetEventData();
    int structural=GetEventData();
    int partial=GetEventData();
    SceneEvents.calls=makeint(SceneEvents.calls)+1;
    SceneEvents.owner=owner;
    SceneEvents.removed=removed;
    SceneEvents.structural=structural;
    SceneEvents.partial=partial;
    return 0; // No game death/quest handler; never claim gameplay acceptance.
}
void SceneFlood(){
    aref character=GetEventData();
    int owner=GetEventData();
    float fraction=GetEventData();
    SceneEvents.floodFraction=fraction;
}

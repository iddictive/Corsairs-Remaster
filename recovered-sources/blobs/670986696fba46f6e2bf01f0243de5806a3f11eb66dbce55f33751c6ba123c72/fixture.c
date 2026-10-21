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


object BattleInterface;
int HudPictures[4];
int HudClass;
int HudPictureCalls = 0;
int HudSeaCalls = 0;
int RecalculateHIcon(int value) { return value; }
int RecalculateVIcon(int value) { return value; }
ref HudPictureData() { int type=GetEventData(); int who=GetEventData(); HudPictureCalls++; HudPictures[0]=76; HudPictures[1]=77; HudPictures[2]=1; HudPictures[3]=0; return &HudPictures; }
ref HudShipClass() { int who=GetEventData(); HudClass=sti(ShipType.Class); return &HudClass; }
void HudSeaState() { for(int j=0;j<7;j++) { int value=GetEventData(); } HudSeaCalls++; }
void HudSelectShip() { int one=GetEventData(); int two=GetEventData(); }
void InitStressHud() {
 float fHtRatio=stf(Render.screen_y)/900.0;
 SetEventHandler("BI_GetData","HudPictureData",0);
 SetEventHandler("evntGetCharacterShipClass","HudShipClass",0);
 SetEventHandler("BI_SetSeaState","HudSeaState",0);
 SetEventHandler("evntBISelectShip","HudSelectShip",0);
BattleInterface.ShipIcon.sailorfontid			= "interface_normal";
	BattleInterface.ShipIcon.sailorfontcolor		= argb(255,255,255,255);
    BattleInterface.ShipIcon.sailorfontscale		= 1.1 * fHtRatio;
    BattleInterface.ShipIcon.sailorfontoffset       = RecalculateHIcon(makeint(-14* fHtRatio))+","+RecalculateVIcon(makeint(18* fHtRatio));

	BattleInterface.ShipIcon.shipnamefontid			= "interface_normal";
	BattleInterface.ShipIcon.shipnamefontcolor		= argb(255,255,255,255);
	BattleInterface.ShipIcon.shipnamefontscale		= 1.1 * fHtRatio;
    BattleInterface.ShipIcon.shipnamefontoffset       = RecalculateHIcon(makeint(-14* fHtRatio))+","+RecalculateVIcon(makeint(40* fHtRatio));

	BattleInterface.ShipIcon.backtexturename		= "battle_interface\ShipBackIcon.tga.tx";
	BattleInterface.ShipIcon.backcolor				= argb(255,128,128,128);
	BattleInterface.ShipIcon.backuv					= "0.0,0.0,1.0,1.0";
	BattleInterface.ShipIcon.backoffset				= RecalculateHIcon(makeint(-2* fHtRatio))+","+RecalculateVIcon(makeint(-2* fHtRatio));
    BattleInterface.ShipIcon.backiconsize			= RecalculateHIcon(makeint(128* fHtRatio))+","+RecalculateVIcon(makeint(128* fHtRatio));

	BattleInterface.ShipIcon.shipstatetexturename	= "battle_interface\ShipState.tga.tx";
	BattleInterface.ShipIcon.shipstatecolor			= argb(255,128,128,128);
	BattleInterface.ShipIcon.shiphpuv				= "0.0,0.109,0.5,0.6875";
	BattleInterface.ShipIcon.shipspuv				= "0.5,0.109,1.0,0.6875";

    BattleInterface.ShipIcon.shiphpoffset			= RecalculateHIcon(makeint(-32* fHtRatio))+","+RecalculateVIcon(makeint(-13* fHtRatio));
    BattleInterface.ShipIcon.shipspoffset			= RecalculateHIcon(makeint(32* fHtRatio))+","+RecalculateVIcon(makeint(-13* fHtRatio));
	
    BattleInterface.ShipIcon.shiphpiconsize			= RecalculateHIcon(makeint(64* fHtRatio))+","+RecalculateVIcon(makeint(74* fHtRatio));
    BattleInterface.ShipIcon.shipspiconsize			= RecalculateHIcon(makeint(64* fHtRatio))+","+RecalculateVIcon(makeint(74* fHtRatio));

	BattleInterface.ShipIcon.shipclasstexturename	= "battle_interface\ShipClass.tga.tx";
	BattleInterface.ShipIcon.shipclasscolor			= argb(255,128,128,128);
	BattleInterface.ShipIcon.shipclassuv			= "0.0,0.0,1.0,1.0";

    BattleInterface.ShipIcon.shipclassoffset		= RecalculateHIcon(makeint(-14* fHtRatio))+","+RecalculateVIcon(makeint(-52* fHtRatio));

    BattleInterface.ShipIcon.shipclassiconsize		= RecalculateHIcon(makeint(64* fHtRatio))+","+RecalculateVIcon(makeint(16* fHtRatio));

	BattleInterface.ShipIcon.gunchargeprogress		= "0.0625, 0.211, 0.359, 0.5, 0.633, 0.781, 0.983"; 

	BattleInterface.ShipIcon.shiptexturename		= "battle_interface\ship_icons2.tga.tx";
	BattleInterface.ShipIcon.shipcolor				= argb(255,128,128,128);

	BattleInterface.ShipIcon.shipoffset				= RecalculateHIcon(makeint(-14* fHtRatio))+","+RecalculateVIcon(makeint(-12* fHtRatio));
	BattleInterface.ShipIcon.shipiconsize			= RecalculateHIcon(makeint(64* fHtRatio))+","+RecalculateVIcon(makeint(64* fHtRatio));
    BattleInterface.ShipIcon.commandlistverticaloffset = makeint(-40 * fHtRatio);
	
    BattleInterface.ShipIcon.iconoffset1 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(70* fHtRatio));
	BattleInterface.ShipIcon.iconoffset2 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(198* fHtRatio));
	BattleInterface.ShipIcon.iconoffset3 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(326* fHtRatio));
	BattleInterface.ShipIcon.iconoffset4 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(454* fHtRatio));
	BattleInterface.ShipIcon.iconoffset5 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(582* fHtRatio));
	BattleInterface.ShipIcon.iconoffset6 = RecalculateHIcon(makeint(70* fHtRatio))+","+RecalculateVIcon(makeint(710* fHtRatio));	

	BattleInterface.CommandList.CommandMaxIconQuantity = 12;
	BattleInterface.CommandList.CommandIconSpace = 1;
	BattleInterface.CommandList.CommandIconLeft = makeint(108 * fHtRatio);
	BattleInterface.CommandList.CommandIconWidth = RecalculateHIcon(makeint(48 * fHtRatio));
	BattleInterface.CommandList.CommandIconHeight = RecalculateVIcon(makeint(48 * fHtRatio));

	BattleInterface.CommandList.CommandNoteFont = "interface_normal";
	BattleInterface.CommandList.CommandNoteColor = argb(255,255,255,255);
	BattleInterface.CommandList.CommandNoteScale = 1.0 * fHtRatio;
	BattleInterface.CommandList.CommandNoteOffset = RecalculateHIcon(0) + "," + RecalculateVIcon(makeint(-42 * fHtRatio));

	BattleInterface.CommandList.UDArrow_Texture = "battle_interface\arrowly.tga.tx";
	BattleInterface.CommandList.UDArrow_UV_Up = "0.0,1.0,1.0,0.0";
	BattleInterface.CommandList.UDArrow_UV_Down = "0.0,0.0,1.0,1.0";
	BattleInterface.CommandList.UDArrow_Size = RecalculateHIcon(makeint(-32 * fHtRatio)) + "," + RecalculateVIcon(makeint(32 * fHtRatio));
	BattleInterface.CommandList.UDArrow_Offset_Up = RecalculateHIcon(makeint(-41 * fHtRatio)) + "," + RecalculateVIcon(makeint(-30 * fHtRatio));
	BattleInterface.CommandList.UDArrow_Offset_Down = RecalculateHIcon(makeint(-41 * fHtRatio)) + "," + RecalculateVIcon(makeint(46 * fHtRatio));

	
 BattleInterface.MainChrIndex=0;
 BattleInterface.ShowCommands=1;
 BattleInterface.ShowNavigator=0;
 BattleInterface.boardRadius=50;
 BattleInterface.freeDistance=500;
 CreateEntity(&BattleInterface,"battle_interface");
 LayerAddObject(2,&BattleInterface,-1);
 LayerAddObject(3,&BattleInterface,-1);
 for(int j=0;j<FleetCount;j++) { Fleet[j].ship.name="Manowar"; SendMessage(&BattleInterface,"llaall",46011,j,&Fleet[j],&ShipType,j<6,1); }
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
        Fleet[i].nation = 0;
        Fleet[i].MainCharacter = 0;
        if (i == 0) Fleet[i].MainCharacter = 1;
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
    InitStressHud();
    BenchmarkReady = 1;
}

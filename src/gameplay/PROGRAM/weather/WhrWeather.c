#include "weather\WhrUtils.c" //кодировка UTF-8 Konstrush
#include "weather\WhrLightning.c"
#include "weather\WhrRain.c"
#include "weather\WhrSea.c"
#include "weather\WhrSky.c"
#include "weather\WhrSun.c"
#include "weather\WhrTornado.c"
#include "weather\WhrAstronomy.c"
#include "weather\WhrSeaPresets.c"

#define WIND_NORMAL_POWER		20.0

#define MAX_WEATHERS   34

object	Weather, WeatherParams, WhrCommonParams;
int		iNextWeatherNum = -1;
int		iCurWeatherNum = -1;
int		iBlendWeatherNum = -1;
int		iCurWeatherHour = -1;
bool	bCurWeatherStorm = false;
int		iTotalNumWeathers = 0;
string	sLightingPath = "day";
string	sLmLightingPath = "day";
string	sInsideBack = "";
string	sNewExecuteLayer, sNewRealizeLayer;
string	sCurrentFog;
float	fWeatherDelta = 0.0;
float	fWeatherAngle, fWeatherSpeed;
float	fFogDensity;
int		iShadowDensity[2];
int		iPrevWeather = -1;
int		sunIsShine = true;
bool	bWeatherLoaded = false;
bool	bWeatherIsNight = false;
bool	bWeatherIsLight = false;
bool	bWeatherIsRain = false;
bool	bWeatherIsStorm = false;
int		iUpdateReducer = 0;	// evganat - уменьшение частоты изменения скорости ветра

object Weathers[MAX_WEATHERS];

extern int InitWeather();

#event_handler("EWhr_GetWindAngle", "Whr_GetWindAngle");
#event_handler("EWhr_GetWindSpeed", "Whr_GetWindSpeed");
#event_handler("EWhr_GetShadowDensity", "Whr_GetShadowDensity");
#event_handler("EWhr_GetFogDensity", "Whr_GetFogDensity");
#event_handler("WeatherTimeUpdate", "Whr_TimeUpdate" );

void SetNextWeather(string sWeatherID)
{
	// find weather
	iNextWeatherNum = -1; 
	for (int i=0; i<MAX_WEATHERS; i++)
	{
		if (!CheckAttribute(&Weathers[i], "id")) { continue; }
		if (Weathers[i].id == sWeatherID)
		{
			iNextWeatherNum = i;
			if (bBettaTestMode)
			{
				Trace("iNextWeatherNum = " + iNextWeatherNum);
				Log_SetStringToLog("ПОГОДА: " + Weathers[i].Lighting);
			}
			return;
		}
	}
}

void WeatherInit()
{
	//DeleteAttribute(&WeatherParams,"");

	if (LoadSegment("weather\WhrInit.c"))
	{
		iTotalNumWeathers = InitWeather();
		UnloadSegment("weather\WhrInit.c");
	}
}

void DeleteWeatherEnvironment()
{
	iBlendWeatherNum = -1;
	if (isEntity(&Weather))
	{
		DeleteClass(&Weather);
	}
	DeleteAttribute(&Weather,"");
	//WeatherParams.Tornado = false; // включает-выключает торнадо на суше Hirurg39
	//DeleteAttribute(&WeatherParams,"");

	WhrDeleteRainEnvironment();
	WhrDeleteSkyEnvironment();
	WhrDeleteSunGlowEnvironment();
	WhrDeleteLightningEnvironment();
	WhrDeleteTornadoEnvironment();
	WhrDeleteSeaEnvironment();
	WhrDeleteAstronomyEnvironment();

	bWeatherLoaded = false;
}

aref GetCurrentWeather()
{
	//Boyer fix for when iCurWeatherNum = -1...borrowed from function Whr_LoadNextWeather(int nPlus)
	if (iCurWeatherNum < 0)
		iCurWeatherNum = iTotalNumWeathers - 1;
	//End fix
	aref arWeather;
	makearef(arWeather,Weathers[iCurWeatherNum]);
	return arWeather;
}

void CreateWeatherEnvironment()
{
    DelEventHandler("frame","Whr_OnWindChange");
    // Presets supply asset names and location metadata; native WEATHER owns conditions.
    if (isEntity(&Weather)) SendMessage(&Weather,"l",1464336389);
    int iHour = MakeInt(GetHour());
    int requestedPreset = iNextWeatherNum;
    iNextWeatherNum = -1;
    iCurWeatherHour = iHour;
    iCurWeatherNum = FindWeatherByHour(iHour);
    if (iCurWeatherNum < 0) iCurWeatherNum = 0;
    iBlendWeatherNum = -1;
    sunIsShine = true;
    int locationIndex = FindLocation(pchar.location);
    int islandIndex = FindIsland(pchar.location);
    bool eventStorm = false;
    bool eventTornado = false;
    if (bSeaActive && CheckAttribute(&WeatherParams,"Storm")) eventStorm=sti(WeatherParams.Storm);
    if (bSeaActive && CheckAttribute(&WeatherParams,"Tornado")) eventTornado=sti(WeatherParams.Tornado);
    // A natural map front continues at the entered sea coordinates; it must not
    // become an independent five-minute whole-scene storm request.
    if (bSeaActive && !eventTornado && CheckAttribute(&worldMap,"nativeWeatherStorm") &&
        sti(worldMap.nativeWeatherStorm)) eventStorm=false;
    DeleteAttribute(&worldMap,"nativeWeatherStorm");
    if (locationIndex >= 0) {
        ref location = &Locations[locationIndex];
        if (CheckAttribute(location,"storm") || CheckAttribute(location,"alwaysStorm") || CheckAttribute(location,"alwaysStorm_2")) eventStorm=true;
        if (CheckAttribute(location,"tornado")) eventTornado=true;
        if (CheckAttribute(location,"QuestlockWeather")) requestedPreset=FindWeather(location.QuestlockWeather);
        if (CheckAttribute(location,"lockWeather") && location.lockWeather=="Inside") sunIsShine=false;
    } else {
        if (islandIndex >= 0) {
            ref island = &Islands[islandIndex];
            if (CheckAttribute(island,"storm") || CheckAttribute(island,"alwaysStorm")) eventStorm=true;
            if (CheckAttribute(island,"tornado")) eventTornado=true;
            if (CheckAttribute(island,"QuestlockWeather")) requestedPreset=FindWeather(island.QuestlockWeather);
        }
    }
    DeleteAttribute(&Weather,"");
    if (!isEntity(&Weather)) CreateEntity(&Weather,"Weather");
    if (!CheckAttribute(&WeatherParams,"NativeState")) WeatherParams.NativeState.version=0;
    Whr_BindSolarPalette();
    aref savedWeather; makearef(savedWeather,WeatherParams.NativeState);
    Whr_BindNativeState(savedWeather);
    // Leaving a quest location releases its lock; finite world-map events continue.
    SendMessage(&Weather,"l",1464336390);
    if (eventStorm || eventTornado) {
        Whr_RequestWeather(1.0,1.0,30.0,300.0,eventTornado,false);
        WeatherParams.Storm=false; WeatherParams.Tornado=false;
    }
    if (requestedPreset >= 0) Whr_RequestWeatherPreset(requestedPreset);
    // Clearable request flags are not the active native condition.
    bWeatherIsStorm = Whr_IsStorm();
    bCurWeatherStorm = bWeatherIsStorm;
    if (bSeaActive) bStorm = bWeatherIsStorm;
    aref aCurWeather = GetCurrentWeather();
    sLightingPath=Whr_GetString(aCurWeather,"Lighting");
    sLmLightingPath=Whr_GetString(aCurWeather,"LightingLm");
    sInsideBack=Whr_GetString(aCurWeather,"InsideBack");
    bWeatherIsNight=Whr_GetLong(aCurWeather,"Night");
    bWeatherIsLight=Whr_GetLong(aCurWeather,"Lights");
    sCurrentFog="Fog";
    Weather.Fog.Enable=sunIsShine;
    Weather.Time.time=GetTime();
    Weather.Time.speed=350.0;
    if (bSeaActive && !bAbordageStarted && iArcadeSails==1) Weather.Time.speed=250.0;
    Weather.Time.updatefrequence=12;
    Weather.isDone="";
    bool outside=sunIsShine && Whr_isRainEnable();
    if (bMainMenu) outside=true;
    SendMessage(&Weather,"ll",1464336388,outside);
    if (outside) WhrCreateRainEnvironment(); else ClearRainEnvironment();
    WhrCreateSunGlowEnvironment();
    WhrCreateLightningEnvironment();
    WhrCreateSkyEnvironment();
    WhrSetSeaPreset(iCurWeatherNum, WhrGetSeaPresetFromWind(Whr_GetWindSpeed()), Whr_GetWindSpeed());
    WhrCreateSeaEnvironment();
    WhrCreateAstronomyEnvironment();
    Particles.windpower=Whr_GetWindSpeed()/WIND_NORMAL_POWER;
    Particles.winddirection.x=sin(Whr_GetWindAngle());
    Particles.winddirection.z=cos(Whr_GetWindAngle());
    bWeatherLoaded=true;
    if (bSeaActive && !bAbordageStarted) {
        Island.LightingPath=GetLightingPath();
        Island.FogDensity=Whr_GetFogDensity();
    }

}

void Whr_UpdateWeather()
{
	if (!isEntity(&Weather)) { return; }

//слишком много инитов
//	WeatherInit();

	CreateWeatherEnvironment();
	MoveWeatherToLayers(sNewExecuteLayer, sNewRealizeLayer);
}

void Whr_LoadNextWeather(int nPlus)
{
	if (!isEntity(&Weather)) { return; }
	//WeatherInit();  //слишком много инитов

	iCurWeatherHour = MakeInt(GetHour());
	iCurWeatherNum = iCurWeatherNum + nPlus;

	if (iCurWeatherNum < 0)
	{
		iCurWeatherNum = iTotalNumWeathers - 1;
	}
	if (iCurWeatherNum >= iTotalNumWeathers)
	{
		iCurWeatherNum = 0;
	}

	if (CheckAttribute(&Weathers[iCurWeatherNum], "Skip")) 
	{ 
		if (sti(Weathers[iCurWeatherNum].skip)) 
		{ 
			Whr_LoadNextWeather(nPlus);
			return;
		}
	}
	Log_TestInfo("ПОГОДА: " + Weathers[iCurWeatherNum].id + " ОСВЕЩЕНИЕ: " + Weathers[iCurWeatherNum].Lighting);
	Log_TestInfo("Sun: HeightAngle: " + Weathers[iCurWeatherNum].Sun.HeightAngle);

	iNextWeatherNum = iCurWeatherNum;
	CreateWeatherEnvironment();
	MoveWeatherToLayers(sNewExecuteLayer, sNewRealizeLayer);

	aref	aCurWeather = GetCurrentWeather();
	iCurWeatherHour = sti(aCurWeather.Hour.Min);

	SetCurrentTime(iCurWeatherHour, 0);
	Weather.Time.time = GetTime();
}

float Whr_BlendFloat(float fBlend, float f1, float f2)
{
	float f = f1 + fBlend * (f2-f1);
	return f;
}

int Whr_BlendLong(float fBlend, int i1, int i2)
{
	int i = makeint(i1 + fBlend * (i2-i1));
	return i;
}

int Whr_BlendColor(float fBlend, int col1, int col2)
{
	int a1 = shr(and(col1,4278190080), 24); // get alpha 1
	int a2 = shr(and(col2,4278190080), 24); // get alpha 2
	int r1 = shr(and(col1,16711680), 16); // get red color 1
	int r2 = shr(and(col2,16711680), 16); // get red color 2

	int g1 = shr(and(col1,65280), 8); // get green color 1
	int g2 = shr(and(col2,65280), 8); // get green color 2

	int b1 = and(col1,255); // get blue color 1
	int b2 = and(col2,255); // get blue color 2

	int r = r1 + MakeInt(fBlend * (r2-r1));
	int g = g1 + MakeInt(fBlend * (g2-g1));
	int b = b1 + MakeInt(fBlend * (b2-b1));
	int a = a1 + MakeInt(fBlend * (a2-a1));

	return argb(0,r,g,b);
}

void Whr_TimeUpdate()
{

    float fTime=GetEventData();
    int oldHour=sti(Environment.date.hour);
    int hour=MakeInt(fTime);
    Environment.time=fTime;
    Environment.date.hour=hour;
    Environment.date.min=MakeInt((fTime-hour)*60);
    Environment.date.sec=MakeInt(((fTime-hour)*60-sti(Environment.date.min))*60);
    worldMap.date.hour=hour; worldMap.date.min=Environment.date.min;
    if (hour<oldHour) { AddDataToCurrent(0,0,1); Weather.Time.time=GetTime(); }
    if (hour!=oldHour) {
        iCurWeatherNum=FindWeatherByHour(hour);
        if (iCurWeatherNum<0) iCurWeatherNum=0;
        Whr_UpdateWeatherHour();
    }
    // Calendar and asset animation only. No weather target, rain or wind writes.
    Sky.TimeUpdate=fTime;
    WhrFillSunGlowData(iCurWeatherNum,-1);
    SunGlow.isDone=true;

}

#event_handler("eChangeDayNight", "eChangeDayNight");

void Whr_UpdateWeatherHour()
{
	bool bOldIsDay = Whr_IsDay();
	bool doLightChange = false;
	int i, j, iCharIdx;

	bWeatherIsLight = Whr_GetLong(&Weathers[iCurWeatherNum],"Lights");
	bWeatherIsNight = Whr_GetLong(&Weathers[iCurWeatherNum],"Night");

	
	aref aCurWeather = GetCurrentWeather();
	aref aStars;
	makearef(aStars, aCurWeather.Stars);
	FillStars(aStars);
	Astronomy.isDone = true;
	Astronomy.TimeUpdate = 1;

	if( Whr_IsDay() != bOldIsDay )
	{ // меняем источники освещения
		Whr_ChangeDayNight();
		Event("eChangeDayNight");
        doLightChange = true;
 	}
 	if (bSeaActive && !bAbordageStarted)
	{
	    bool isSeaEnt = false;
	    if (IsEntity(&Sea))
            isSeaEnt = true;
        Whr_WindChange();
        // Native profiles follow wind continuously; refresh moving island limits.
        if (isSeaEnt) {
            WhrUpdateNativeSeaCap();
            WhrBindNativeSeaProfiles();
        }
        if(doLightChange && isSeaEnt) {
            doShipLightChange(aCurWeather);
        }
 	}
}

void doShipLightChange(ref aCurWeather)
{
    int j, iCharIdx;

    ref rChar;
    Sea.Lights = aCurWeather.Lights;
    for(j = 0; j < iNumShips; j++)
	{
        iCharIdx = Ships[j];
        if (iCharIdx < 0 || iCharIdx >= TOTAL_CHARACTERS) continue;
        rChar = GetCharacter(Ships[j]);
        Ship_SetLightsAndFlares(rChar);
        SendMessage(rChar, "ll", MSG_SHIP_LIGHTSRESET, sti(Sea.Lights));
    }
}

void Whr_ChangeDayNight()
{
	int loadLocationIndex = FindLoadedLocation();
	if( loadLocationIndex >= 0 )
	{
		LocationSetLights(&Locations[loadLocationIndex]);
		if( Whr_IsDay() )
		{
			SendMessage( &Locations[loadLocationIndex], "ls", MSG_LOCATION_EX_MSG, "DelFlys" );
		}
	}
}

int FindWeatherByHour(int nHour)
{
	for (int n=0; n<MAX_WEATHERS; n++)
	{
		if (!CheckAttribute(&Weathers[n], "hour")) {continue;}
		if (CheckAttribute(&Weathers[n], "skip") && sti(Weathers[n].skip)==true) {continue;}
		if (CheckAttribute(&Weathers[n], "Storm") && sti(Weathers[n].Storm)==true) {continue;}

		if( sti(Weathers[n].hour.min) == nHour ) {return n;}
	}
	return -1;
}

int FindBlendWeather(int iWeatherNum)
{
	// чтоб вырубить return -1;
	if( iWeatherNum<0 || iWeatherNum>=MAX_WEATHERS ) {return -1;}

	if( !CheckAttribute(&Weathers[iWeatherNum], "hour") ) {return -1;}
	if( CheckAttribute(&Weathers[iWeatherNum],"Storm") && sti(Weathers[iWeatherNum].Storm)==true ) {return -1;}
	if( CheckAttribute(&Weathers[iWeatherNum],"skip") && sti(Weathers[iWeatherNum].skip)==true ) {return -1;}

	int nBlendWeatherHour = sti(Weathers[iWeatherNum].hour.min) + 1;
	if( nBlendWeatherHour>=24 ) {nBlendWeatherHour = 0;}

	return FindWeatherByHour(nBlendWeatherHour);
}

void CreateWeather(int sExecuteLayer, int sRealizeLayer)
{
	CreateWeatherEnvironment();
	MoveWeatherToLayers(sExecuteLayer, sRealizeLayer);
}

void DeleteWeather()
{
	DeleteWeatherEnvironment();
}

void MoveWeatherToLayers(int sExecuteLayer, int sRealizeLayer)
{
	LayerDelObject(EXECUTE,&Weather);
	LayerDelObject(REALIZE,&Weather);
	LayerDelObject(SEA_EXECUTE,&Weather);
	LayerDelObject(SEA_REALIZE,&Weather);

	sNewExecuteLayer = sExecuteLayer;
	sNewRealizeLayer = sRealizeLayer;

	LayerAddObject(sExecuteLayer,&Weather,1000000);
	LayerAddObject(sRealizeLayer,&Weather,65530);

	MoveRainToLayers(sExecuteLayer, sRealizeLayer);
	MoveLightningToLayers(sExecuteLayer, sRealizeLayer);
	MoveSkyToLayers(sExecuteLayer, sRealizeLayer);
	MoveSunGlowToLayers(sExecuteLayer, sRealizeLayer);
	MoveAstronomyToLayers(sExecuteLayer, sRealizeLayer);
}

float Whr_GetFogDensity() { return Whr_NativeValue(4); }

float Whr_GetWindAngle() { return Whr_NativeValue(0); }

float Whr_GetWindSpeed() { return Whr_NativeValue(1); }

ref Whr_GetShadowDensity()
{
	aref	aCurWeather = GetCurrentWeather();

	iShadowDensity[0] = argb(255,96,96,96);
	iShadowDensity[1] = argb(255,96,96,64);

	if (bWeatherLoaded)
	{
		iShadowDensity[0] = Whr_GetColor(aCurWeather,"Shadow.Density.Head");
		iShadowDensity[1] = Whr_GetColor(aCurWeather,"Shadow.Density.Foot");
	}
	
	return &iShadowDensity;
}

string	Whr_GetCurrentFog() { return sCurrentFog; }

string	Whr_GetInsideBack() { return sInsideBack; }
string	GetLightingPath()	
{
/*navy -->
	if (CheckAttribute(&WeatherParams, "Rain") && sti(WeatherParams.Rain))
	{
		return Whr_GetRainyLightningPath();
	}
navy <--*/
	return sLightingPath; 
}
string	GetLmLightingPath() 
{ 
/*navy -->
	if (CheckAttribute(&WeatherParams, "Rain") && sti(WeatherParams.Rain))
	{
		return "storm";
	}
navy <--*/
	return sLmLightingPath; 
}

bool	Whr_IsDay() { return !bWeatherIsNight; }
bool	Whr_IsNight() { return bWeatherIsNight; }
bool	Whr_IsLight() { return bWeatherIsLight; }
bool	Whr_IsRain() { return Whr_NativeValue(2)>0.03; }
bool	Whr_IsStorm() { return Whr_NativeValue(3)>0.5; }

int FindWeather(string sWeatherID)
{
	for (int i=0;i<MAX_WEATHERS;i++)
	{
		if (!CheckAttribute(&Weathers[i], "id")) 
		{ 
			continue; 
		}
		if(Weathers[i].id == sWeatherID)
		{
			return i;
		}
	}
	return -1;
}

//navy --> Rain вернул
void Whr_SetRainSound(bool _set, bool _isNight)
{
//	return;
	// пока закрыл, тк работает метод CalcLocalTime, нужно понять где лучше и оставть один

	string scheme = "land";
	//ищем локацию ГГ, три схемы: land, sea, town
	//можно и больше...
	if (!bSeaActive || bAbordageStarted)
	{
		int locationIndex = FindLocation(pchar.location);
		if (locationIndex >= 0)
		{
			ref location = &Locations[locationIndex];
			if (CheckAttribute(location, "type"))
			{
				switch (location.type)
				{
				case "town":
					scheme = "town";
					break;
				case "land":
					scheme = "land";
					break;
				case "deck":
					scheme = "deck";
					break;
				}
			}
		}
	}
	else
	{
		scheme = "sea";
	}

	//проверка - день/ночь
	if (_isNight)
    {
		scheme = scheme+"_night";
    }
	else
	{
		scheme = scheme+"_day";
	}

	//если устанавливаем звук дождя
	if (_set)
	{
		scheme = scheme + "_rain";
	}

	trace(": Set Rain Sound... " + _set);
	SetSoundScheme(scheme);
}

string Whr_GetRainyLightningPath()
{
	//trace("__GET__COL__");
	return "storm1";
}
//navy <--

// boal -->
void Whr_WindChange()
{
    // Existing callers now refresh their consumer snapshot, never generate wind.
    fWeatherAngle=Whr_GetWindAngle(); fWeatherSpeed=Whr_GetWindSpeed();
}

bool Whr_isRainEnable()
{
	int locationIndex=FindLocation(pchar.location);
	if (locationIndex>=0) {
		ref location=&Locations[locationIndex];
		if (CheckAttribute(location,"environment.weather.rain") && !sti(location.environment.weather.rain)) return false;
		if (CheckAttribute(location,"lockWeather") && location.lockWeather=="Inside") return false;
		if (CheckAttribute(location,"type")) {
			if (location.type=="residence" || location.type=="tavern" || location.type=="house" || location.type=="shop" || location.type=="shipyard" || location.type=="church" || location.type=="cave") return false;
		}
	}
	return true;
}
// boal <--
float Whr_NativeValue(int field)
{
    float value=0.0;
    if (isEntity(&Weather)) SendMessage(&Weather,"lle",1464336386,field,&value);
    return value;
}
void Whr_RequestWeather(float rain, float storm, float wind, float seconds, bool tornado, bool locked)
{
    if (isEntity(&Weather)) SendMessage(&Weather,"lffffll",1464336387,rain,storm,wind,seconds,tornado,locked);
}
void Whr_RequestWeatherPreset(int index)
{
    float rain=Whr_GetFloat(&Weathers[index],"Rain.NumDrops")/5000.0;
    float storm=0.0;
    if (Whr_GetLong(&Weathers[index],"Storm")) storm=1.0;
    Whr_RequestWeather(rain,storm,Whr_GetFloat(&Weathers[index],"Wind.Speed"),0.0,Whr_GetLong(&Weathers[index],"Tornado"),true);
}
#event_handler("NativeWeatherChanged","Whr_NativeChanged");
void Whr_NativeChanged()
{
    bool rain=GetEventData();
    bool storm=GetEventData();
    bWeatherIsRain=rain; bWeatherIsStorm=storm; bCurWeatherStorm=storm;
    if (bSeaActive) {
        bStorm=storm;
        if (isEntity(&Sea)) WhrUpdateNativeSeaCap();
    }
    // Menu/transition weather has no active gameplay sound scene yet.
    if (!bSeaActive && !bLandInterfaceStart) return;
    if (!isEntity(&worldMap)) Whr_SetRainSound(rain && Whr_isRainEnable(),bWeatherIsNight);
}

void Whr_BindMapWeather()
{
    if (isEntity(&Weather)) SendMessage(&Weather,"l",1464336389);
    if (!isEntity(&Weather)) CreateEntity(&Weather,"Weather");
    if (!CheckAttribute(&WeatherParams,"NativeState")) WeatherParams.NativeState.version=0;
    Whr_BindSolarPalette();
    aref savedWeather; makearef(savedWeather,WeatherParams.NativeState);
    Whr_BindNativeState(savedWeather);
    // A location's quest lock ends when leaving it for the global map.
    SendMessage(&Weather,"l",1464336390);
    SendMessage(&Weather,"ll",1464336388,false);
    Weather.Time.speed=0.0;
    LayerAddObject(EXECUTE,&Weather,1000000);
}

void Whr_GetMapPosition(ref position)
{
    position.x=0.0; position.z=0.0;
    if (CheckAttribute(&worldMap,"playerShipX")) position.x=stf(worldMap.playerShipX);
    if (CheckAttribute(&worldMap,"playerShipZ")) position.z=stf(worldMap.playerShipZ);
    if (bSeaActive && !isEntity(&worldMap) && CheckAttribute(&worldMap,"zeroX") &&
        CheckAttribute(&worldMap,"zeroZ") && CheckAttribute(pchar,"Ship.Pos.x") && CheckAttribute(pchar,"Ship.Pos.z")) {
        float scale=wdmMapToSeaScale();
        position.x=stf(worldMap.zeroX)+stf(pchar.Ship.Pos.x)/scale;
        position.z=stf(worldMap.zeroZ)+stf(pchar.Ship.Pos.z)/scale;
    }
    if (bSeaActive || isEntity(&worldMap)) return;
    // Land scenes sample their chart location, not the last offshore ship pose.
    string islandId=pchar.location;
    string chartLocation=pchar.location;
    int locationIndex=FindLocation(pchar.location);
    if (locationIndex>=0) {
        ref location=&Locations[locationIndex];
        if (CheckAttribute(location,"islandId")) islandId=location.islandId;
        if (CheckAttribute(location,"townsack")) chartLocation=location.townsack+"_town";
    }
    islandId=GetIslandByArealName(islandId);
    if (!CheckAttribute(&worldMap,"islands."+islandId)) return;
    aref chart; makearef(chart,worldMap.islands.(islandId));
    if (CheckAttribute(chart,pchar.location+".position")) chartLocation=pchar.location;
    if (CheckAttribute(chart,chartLocation+".position.x") && CheckAttribute(chart,chartLocation+".position.z")) {
        position.x=stf(chart.(chartLocation).position.x);
        position.z=stf(chart.(chartLocation).position.z);
    } else if (CheckAttribute(chart,"position.x") && CheckAttribute(chart,"position.z")) {
        position.x=stf(chart.position.x); position.z=stf(chart.position.z);
    }
}

void Whr_BindNativeState(aref savedWeather)
{
    object position; Whr_GetMapPosition(&position);
    SendMessage(&Weather,"lalllfff",1464336385,savedWeather,GetDataMonth(),GetDataYear(),GetDataDay(),GetTime(),stf(position.x),stf(position.z));
}

void Whr_UpdateWeatherPosition()
{
    if (!isEntity(&Weather)) return;
    object position; Whr_GetMapPosition(&position);
    SendMessage(&Weather,"lff",1464336391,stf(position.x),stf(position.z));
}

void Whr_BindSolarPalette()
{
    // Native WEATHER blends the shipped hourly and storm colors with one parcel.
    for (int hour=0;hour<24;hour++) {
        int index=FindWeatherByHour(hour);
        if (index<0) index=0;
        string key="h"+hour;
        WeatherParams.NativeState.palette.(key).fog=Whr_GetColor(&Weathers[index],"Fog.Color");
        WeatherParams.NativeState.palette.(key).ambient=Whr_GetColor(&Weathers[index],"Sun.Ambient");
        WeatherParams.NativeState.palette.(key).sun=Whr_GetColor(&Weathers[index],"Sun.Color");
    }
    for (int stormIndex=0;stormIndex<4;stormIndex++) {
        int stormPreset=FindWeather("Storm0"+(stormIndex+1));
        string stormKey="storm"+stormIndex;
        if (stormPreset<0) {
            DeleteAttribute(&WeatherParams,"NativeState.palette."+stormKey);
            continue;
        }
        WeatherParams.NativeState.palette.(stormKey).fog=Whr_GetColor(&Weathers[stormPreset],"Fog.Color");
        WeatherParams.NativeState.palette.(stormKey).ambient=Whr_GetColor(&Weathers[stormPreset],"Sun.Ambient");
        WeatherParams.NativeState.palette.(stormKey).sun=Whr_GetColor(&Weathers[stormPreset],"Sun.Color");
    }
}

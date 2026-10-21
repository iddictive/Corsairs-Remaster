#define true 1
#define false 0
#define MAX_WEATHERS 34
#define EXECUTE 0
#define REALIZE 1
#define SEA_EXECUTE 2
#define SEA_REALIZE 3
#define SEA_REFLECTION 4
object Weather;
object Weathers[MAX_WEATHERS];
int iCurWeatherNum;
int iTotalNumWeathers = 24;
int bSeaActive;
float TimeScaleCounter;
float FixtureTime;
float GetTime(){return FixtureTime;}
float Whr_GetWindSpeed(){return 5.0;}
float Whr_GetFloat(aref base,string key){return stf(base.(key));}
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
	// ÑÑÐ¾Ð± Ð²ÑÑÑÐ±Ð¸ÑÑ return -1;
	if( iWeatherNum<0 || iWeatherNum>=MAX_WEATHERS ) {return -1;}

	if( !CheckAttribute(&Weathers[iWeatherNum], "hour") ) {return -1;}
	if( CheckAttribute(&Weathers[iWeatherNum],"Storm") && sti(Weathers[iWeatherNum].Storm)==true ) {return -1;}
	if( CheckAttribute(&Weathers[iWeatherNum],"skip") && sti(Weathers[iWeatherNum].skip)==true ) {return -1;}

	int nBlendWeatherHour = sti(Weathers[iWeatherNum].hour.min) + 1;
	if( nBlendWeatherHour>=24 ) {nBlendWeatherHour = 0;}

	return FindWeatherByHour(nBlendWeatherHour);
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
int Whr_GetColor(aref arRef, string sAttribute)
{
	return sti(arRef.(sAttribute));
}
#event_handler("frame", "UpdateSky");

object Sky;

float fPrevCameraAX = 0;

void WhrDeleteSkyEnvironment()
{
	if(isEntity(&Sky))
	{
		DeleteClass(&Sky);
	}
	
	DeleteAttribute(&Sky, "");
}

void WhrCreateSkyEnvironment()
{
	aref aCurWeather = GetCurrentWeather();
	aref aSky;
	makearef(aSky, aCurWeather.Sky);

	DeleteAttribute(&Sky, "")
	if(!isEntity(&Sky))
	{
		CreateEntity(&Sky, "Sky");
		if(bSeaActive)
		{
			LayerAddObject(SEA_REFLECTION,&Sky,1);
		}
        else
		{
			LayerAddObject(SEA_REFLECTION,&Sky,11);
		}
	}

	FillSkyDir(&Sky);
	//Sky.Dir = Whr_GetString(aSky, "Dir");

	Sky.Color = Whr_GetColor(aSky, "Color");
	Sky.RotateSpeed = Whr_GetFloat(aSky, "Rotate"); // Warship 02.06.09 - ротация неба ниже
	Sky.Angle = Whr_GetFloat(aSky, "Angle");
	Sky.Size = Whr_GetFloat(aSky, "Size");
	Sky.FogHeight = Whr_GetFloat(aCurWeather, "Fog.Height");
	
	  if(bSeaActive) 
	{
        Sky.Size = Whr_GetFloat(aSky, "Size");
        Sky.techSky = "Sky";
        Sky.techSkyBlend = "SkyBlend";
        Sky.techskyAlpha = "skyblend_alpha";
        Sky.techSkyFog = "SkyFog";
	}
	/* else
	{
         Sky.Size = 2048; 
	    Sky.techSky = "SkyLand";
        Sky.techSkyBlend = "SkyBlendLand";
        Sky.techskyAlpha = "skyblend_alphaLand";
        Sky.techSkyFog = "SkyFogLand";
	}  //// *//////======>> Hirurg39 убрал прозрачную текстуру в небе при торнадо на суше

	Sky.isDone = "";
}

// Warship 02.06.09 - апдейт параметров неба (например, скорость ротации в зависимости от силы ветра)
void UpdateSky()
{
	float windSpeed = 5.0;
	float timeScale = 1.0 + TimeScaleCounter * 0.25; // Текущее ускорение времени
	
	// Вычисление делителя для ускорения, чтоб на x8 бешенно не крутились
	if(timeScale <= 2)
	{
		timeScale = 1;
	}
	else
	{
		timeScale /= 2;
	}
	
	if(isEntity(&Weather))
	{
		windSpeed = Whr_GetWindSpeed();
		// Texture time and tint must advance together, including midnight.
		// Reuse the original hourly colors and the existing color blend.
		int nextWeather = FindBlendWeather(iCurWeatherNum);
		aref currentWeather = GetCurrentWeather();
		int skyColor = Whr_GetColor(currentWeather, "Sky.Color");
		if (nextWeather >= 0)
		{
			float skyTime = GetTime();
			skyColor = Whr_BlendColor(skyTime - MakeInt(skyTime), skyColor,
				Whr_GetColor(&Weathers[nextWeather], "Sky.Color"));
		}
		Sky.Color = skyColor;
	}
	
	// Sky.RotateSpeed == 0.05 - это уже много
	Sky.RotateSpeed = windSpeed / 4000 / timeScale;
}

void FillSkyDir(aref aSky)
{
	int i;
	string satr;
	aref aCurWeather = GetCurrentWeather();
	string sDir;

	DeleteAttribute(aSky,"Dir");
	// Native WEATHER still owns time. Authored panoramas must retain the
	// original full-day blend even when no script weather blend is active.
	{
		for (i=0;i<MAX_WEATHERS;i++)
		{
			if (!CheckAttribute(&Weathers[i], "Hour")) {continue;}
			if (CheckAttribute(&Weathers[i], "Skip") && sti(Weathers[i].Skip)==true) {continue;}
			if (CheckAttribute(&Weathers[i], "Storm")&& sti(Weathers[i].Storm)==true) {continue;}

			satr = "d" + sti(Weathers[i].Hour.Min);
			if( satr=="d24" ) {continue;}
			
			sDir = Weathers[i].Sky.Dir;
			// Rain.ThisDay belongs to the retired script forecast. Replacing
			// night slots with its single daytime storm image breaks the clock.
			aSky.Dir.(satr) = sDir;
		}
		aSky.Dir = GetTime();
	}
}

void MoveSkyToLayers(int sExecuteLayer, int sRealizeLayer)
{
	LayerDelObject(EXECUTE,&Sky);
	LayerDelObject(REALIZE,&Sky);
	LayerDelObject(SEA_EXECUTE,&Sky);
	LayerDelObject(SEA_REALIZE,&Sky);
	
    LayerAddObject(sExecuteLayer,&Sky,2);
    LayerAddObject(sRealizeLayer,&Sky,3);
}

void FixtureTint(){
int nextWeather = FindBlendWeather(iCurWeatherNum);
		aref currentWeather = GetCurrentWeather();
		int skyColor = Whr_GetColor(currentWeather, "Sky.Color");
		if (nextWeather >= 0)
		{
			float skyTime = GetTime();
			skyColor = Whr_BlendColor(skyTime - MakeInt(skyTime), skyColor,
				Whr_GetColor(&Weathers[nextWeather], "Sky.Color"));
		}
		Sky.Color = skyColor;
}
int VerifySky(){
Weathers[0].Hour.Min=0; Weathers[0].Sky.Dir="weather\skies\24\"; Weathers[0].Sky.Color=argb(0,64,64,64);
Weathers[1].Hour.Min=1; Weathers[1].Sky.Dir="weather\skies\01\"; Weathers[1].Sky.Color=argb(0,64,64,64);
Weathers[2].Hour.Min=2; Weathers[2].Sky.Dir="weather\skies\02\"; Weathers[2].Sky.Color=argb(0,64,64,64);
Weathers[3].Hour.Min=3; Weathers[3].Sky.Dir="weather\skies\03\"; Weathers[3].Sky.Color=argb(0,64,64,64);
Weathers[4].Hour.Min=4; Weathers[4].Sky.Dir="weather\skies\04\"; Weathers[4].Sky.Color=argb(0,64,64,64);
Weathers[5].Hour.Min=5; Weathers[5].Sky.Dir="weather\skies\05\"; Weathers[5].Sky.Color=argb(0,64,64,64);
Weathers[6].Hour.Min=6; Weathers[6].Sky.Dir="weather\skies\06\"; Weathers[6].Sky.Color=argb(0,255,255,255);
Weathers[7].Hour.Min=7; Weathers[7].Sky.Dir="weather\skies\07\"; Weathers[7].Sky.Color=argb(0,255,255,255);
Weathers[8].Hour.Min=8; Weathers[8].Sky.Dir="weather\skies\08\"; Weathers[8].Sky.Color=argb(0,255,255,255);
Weathers[9].Hour.Min=9; Weathers[9].Sky.Dir="weather\skies\09\"; Weathers[9].Sky.Color=argb(0,255,255,255);
Weathers[10].Hour.Min=10; Weathers[10].Sky.Dir="weather\skies\10\"; Weathers[10].Sky.Color=argb(0,255,255,255);
Weathers[11].Hour.Min=11; Weathers[11].Sky.Dir="weather\skies\11\"; Weathers[11].Sky.Color=argb(255,255,255,255);
Weathers[12].Hour.Min=12; Weathers[12].Sky.Dir="weather\skies\12\"; Weathers[12].Sky.Color=argb(255,255,255,255);
Weathers[13].Hour.Min=13; Weathers[13].Sky.Dir="weather\skies\13\"; Weathers[13].Sky.Color=argb(255,255,255,255);
Weathers[14].Hour.Min=14; Weathers[14].Sky.Dir="weather\skies\14\"; Weathers[14].Sky.Color=argb(255,255,255,255);
Weathers[15].Hour.Min=15; Weathers[15].Sky.Dir="weather\skies\15\"; Weathers[15].Sky.Color=argb(255,255,255,255);
Weathers[16].Hour.Min=16; Weathers[16].Sky.Dir="weather\skies\16\"; Weathers[16].Sky.Color=argb(255,255,255,255);
Weathers[17].Hour.Min=17; Weathers[17].Sky.Dir="weather\skies\17\"; Weathers[17].Sky.Color=argb(255,255,255,255);
Weathers[18].Hour.Min=18; Weathers[18].Sky.Dir="weather\skies\18\"; Weathers[18].Sky.Color=argb(255,255,255,255);
Weathers[19].Hour.Min=19; Weathers[19].Sky.Dir="weather\skies\19\"; Weathers[19].Sky.Color=argb(0,255,255,255);
Weathers[20].Hour.Min=20; Weathers[20].Sky.Dir="weather\skies\20\"; Weathers[20].Sky.Color=argb(0,255,255,255);
Weathers[21].Hour.Min=21; Weathers[21].Sky.Dir="weather\skies\21\"; Weathers[21].Sky.Color=argb(0,255,255,255);
Weathers[22].Hour.Min=22; Weathers[22].Sky.Dir="weather\skies\22\"; Weathers[22].Sky.Color=argb(0,255,255,255);
Weathers[23].Hour.Min=23; Weathers[23].Sky.Dir="weather\skies\23\"; Weathers[23].Sky.Color=argb(0,255,255,255);
FixtureTime=14.25; FillSkyDir(&Sky);
aref dirs; makearef(dirs,Sky.Dir);
if(GetAttributesNum(dirs)!=24)return 1;
for(int h=0;h<24;h++){string key="d"+h; if(!CheckAttribute(dirs,key))return 2;}
FixtureTime=2.0; iCurWeatherNum=2; FixtureTint(); if(sti(Sky.Color)!=argb(0,64,64,64))return 10;
FixtureTime=14.0; iCurWeatherNum=14; FixtureTint(); if(sti(Sky.Color)!=argb(0,255,255,255))return 11;
FixtureTime=20.0; iCurWeatherNum=20; FixtureTint(); if(sti(Sky.Color)!=argb(0,255,255,255))return 12;
FixtureTime=23.5; iCurWeatherNum=23; FixtureTint(); if(sti(Sky.Color)!=argb(0,160,160,160))return 13;
FixtureTime=0.0; iCurWeatherNum=0; FixtureTint(); if(sti(Sky.Color)!=argb(0,64,64,64))return 14;
FixtureTime=5.5; iCurWeatherNum=5; FixtureTint(); if(sti(Sky.Color)!=argb(0,159,159,159))return 15;
return 0;
}

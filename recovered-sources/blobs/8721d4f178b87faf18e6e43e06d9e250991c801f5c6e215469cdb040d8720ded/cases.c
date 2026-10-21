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
#libriary "script_libriary_test"
#libriary "dx9render_script_libriary"
#define MAX_WEATHERS 28
object Weathers[28];
object WeatherParams;
int Whr_GetColor(aref weather, string field) { return sti(weather.(field)); }
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
int CheckPalette() {
 if(WeatherParams.NativeState.parcel.marker!="123456")return 1;
 if(WeatherParams.NativeState.palette.h7.fog!="107")return 2;
 for(int n=0;n<4;n++) {
  string key="storm"+n;
  if(sti(WeatherParams.NativeState.palette.(key).fog)!=2000+n)return 3;
  if(sti(WeatherParams.NativeState.palette.(key).ambient)!=3000+n)return 4;
  if(sti(WeatherParams.NativeState.palette.(key).sun)!=4000+n)return 5;
 }
 return 0;
}

int BindPaletteFromOldState() {
 for(int hour=0;hour<24;hour++) {
  Weathers[hour].Hour.Min=hour;
  Weathers[hour].Fog.Color=100+hour;
  Weathers[hour].Sun.Ambient=200+hour;
  Weathers[hour].Sun.Color=300+hour;
 }
 for(int n=0;n<4;n++) {
  int i=24+n;
  Weathers[i].id="Storm0"+(n+1);
  Weathers[i].Storm=true;
  Weathers[i].Fog.Color=2000+n;
  Weathers[i].Sun.Ambient=3000+n;
  Weathers[i].Sun.Color=4000+n;
 }
 WeatherParams.NativeState.version=1;
 WeatherParams.NativeState.parcel.marker=123456;
 WeatherParams.NativeState.palette.h7.fog=999;
 Whr_BindSolarPalette();
 return CheckPalette();
}

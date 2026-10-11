object Rain;

void WhrDeleteRainEnvironment()
{
	if (isEntity(&Rain))
	{
		DeleteClass(&Rain);
	}
	DeleteAttribute(&Rain,"");
}

void WhrCreateRainEnvironment()
{
	bWeatherIsRain = false;

	if (!isEntity(&Rain))
	{
		CreateEntity(&Rain,"rain");
	}

	FillRainData(iCurWeatherNum,iBlendWeatherNum);
	Rain.isDone = "";

	if( sti(Rain.NumDrops) > 0 ) {bWeatherIsRain = true;}
}

void ClearRainEnvironment()
{
	if (!isEntity(&Rain))
	{
		CreateEntity(&Rain,"rain");
	}

	FillRainData(iCurWeatherNum,iBlendWeatherNum);
	Rain.NumDrops = 0.0;
	Rain.DropLength = 5.0;
	Rain.isDone = "";

	bWeatherIsRain = false;
}

void FillRainData(int nw1, int nw2)
{
	if( nw1 < 0 || nw1 >= MAX_WEATHERS ) {return;}

	aref aRain1; makearef(aRain1, Weathers[nw1].Rain);
	aref aRainbow1; makearef(aRainbow1, Weathers[nw1].Rainbow);

	Rain.Clear = "";
	DeleteAttribute(&Rain,"");

    Rain.NumDrops=0;
    Rain.Color=argb(0,90,100,115);
    Rain.DropLength=5.0; Rain.Height=80.0; Rain.Radius=35.0;
    Rain.Speed=25.0; Rain.Jitter=5.0; Rain.WindSpeedJitter=0.1;
    Rain.TimeBlend=1000; Rain.MaxBlend=1000;
    Rain.RainDrops.DropsNearNum=30;
    Rain.RainDrops.DropsFarNum=70;
    Rain.RainDrops.DropsNearRadius=Whr_GetFloat(aRain1,"DropsNearRadius");
    Rain.RainDrops.DropsFarRadius=Whr_GetFloat(aRain1,"DropsFarRadius");
    Rain.RainDrops.DropsLifeTime=Whr_GetFloat(aRain1,"DropsLifeTime");
    Rain.RainDrops.DropsSize=Whr_GetFloat(aRain1,"DropsSize");
    Rain.RainDrops.DropsColor=Whr_GetLong(aRain1,"DropsColor");
    Rain.RainDrops.DropsTexture=Whr_GetString(aRain1,"DropsTexture");
    Rain.Rainbow.Enable=false;

}

void MoveRainToLayers(int sExecuteLayer, int sRealizeLayer)
{
	LayerDelObject(EXECUTE,&Rain);
	LayerDelObject(REALIZE,&Rain);
	LayerDelObject(SEA_EXECUTE,&Rain);
	LayerDelObject(SEA_REALIZE,&Rain);

	LayerAddObject(sExecuteLayer, &Rain, 65537);
	LayerAddObject(sRealizeLayer, &Rain, 65537);
}

void ShowRain(int bShow)
{
	SendMessage(&Rain,"ll",1,bShow);
}

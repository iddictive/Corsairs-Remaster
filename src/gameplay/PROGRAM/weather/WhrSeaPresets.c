
// -------------------------------------------------------------
// Mett: настройки моря
// заготовки на разные типы волнения:
// штиль, лёгкий бриз, бриз, умеренный бриз, сильный бриз, шторм
// -------------------------------------------------------------

float WhrSeaPresetMinWind(int index)
{
    switch (index) {
        case 0: return 0.0; break;
        case 1: return 4.0; break;
        case 2: return 7.5; break;
        case 3: return 11.0; break;
        case 4: return 14.5; break;
        case 5: return 17.5; break;
    }
    return 0.0;
}

string WhrSeaWindPreset(int index)
{
    switch (index) {
        case 0: return "calm"; break;
        case 1: return "light_breeze"; break;
        case 2: return "breeze"; break;
        case 3: return "moderate_breeze"; break;
        case 4: return "strong_breeze"; break;
        case 5: return "strong wind"; break;
    }
    return "calm";
}

string WhrGetSeaPresetFromWind(float fWind)
{
	ref mchr = GetMainCharacter();
	string sLocation = mchr.location;
	string sPreset = "calm";
    for (int i = 1; i < 6; i++) {
        if (fWind > WhrSeaPresetMinWind(i)) sPreset = WhrSeaWindPreset(i);
    }
    if (Whr_IsStorm()) sPreset = "storm";
	if(FindLocation(sLocation) != -1){
		if (fWind > 10.0) sPreset = "strong_breeze_land";
		else{
			sPreset = "light_breeze_land";
		}
	}
	return sPreset;
}

void WhrSetSeaPreset(int iCurWeatherNum, string sPreset, float fWind)
{
	ref Whr = &Weathers[iCurWeatherNum];
	Sea.MaxSeaDistance= 8000.0;
    WhrFillSeaPreset(Whr, sPreset, fWind);
    if (sPreset == "strong_breeze_land" && CheckAttribute(&WeatherParams, "Storm") && sti(WeatherParams.Storm) == 1) {
        Sea.Fog.SeaDensity = 0.02;
        Sea.Sea2.Reflection = 0.03;
        Sea.Sea2.WaterColor = argb(0,0,30,30);
    }
    SM_ModifyLightness();
    Log_TestInfo("WhrSetSeaPreset: выбран пресет " + sPreset);
}

// Pure profile authoring: both ordinary scripts and native continuous weather
// consume this switch. Sampling at wind0/1 retains its affine wind dependence.
void WhrFillSeaPreset(ref Whr, string sPreset, float fWind)
{
 /* Whr.Sea2.Reflection = 0.8;
    Whr.Sea2.Transparency = 0.3;
    Whr.Sea2.Frenel = 0.5;
    Whr.Sea2.Attenuation = 0.3;
    Whr.Sea2.WaterColor = argb(0, 10, 30, 50);
    Whr.Sea2.SkyColor = argb(0, 255, 255, 255);
    Whr.Sea2.BumpScale = 0.075; //ћ?Ћ »? ¬ќЋЌџ */
    // Штиль Whr.Sea2.Amp1 = 1.0; 0 - 4 м/с
	//лёгкий бриз Whr.Sea2.Amp1 = 4.0; 4 - 7.5
	//бриз 2 Whr.Sea2.Amp1 = 8.0; 7.5 - 11.0
	//бриз 3 Whr.Sea2.Amp1 = 12.0; 11 - 14.5
	//ветрено 4 Whr.Sea2.Amp1 = 16.0; 14.5 - 18
	//шторм 5 Whr.Sea2.Amp1 = 32.0;
	float SeaAmp = 0.0;

	switch(sPreset)
	
	{
			case "calm": //штиль 0
				SeaAmp = fWind * 0.6 + 1; // 4*х+х=3

				Whr.Sea2.Amp1 = SeaAmp;	// 1.0			
		        Whr.Sea2.AnimSpeed1 = 1.0;			
		        Whr.Sea2.Scale1 = 1.25;				
		        Whr.Sea2.MoveSpeed1 = "1.5, 0.0, 1.5";	

		        Whr.Sea2.Amp2 = 1.25;					
		        Whr.Sea2.AnimSpeed2 = 1.25;				
		        Whr.Sea2.Scale2 = 2.0;					
		        Whr.Sea2.MoveSpeed2 = "-1.5, 0.0, -1.5";

				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.0;//остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;
				
				Whr.Sea2.FoamK = 0.4;//яркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = 2.5;////высота с которой начинаетс§ пена
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.75;
	            Whr.Sea2.Transparency = 0.3;
	            Whr.Sea2.Frenel = 0.5;
	            Whr.Sea2.Attenuation = 0.2 + 0 * 0.05;
				Whr.Sea2.WaterColor = argb(0, 0, 40, 70);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;

			case "light_breeze": //лёгкий бриз 1
			    SeaAmp = fWind * 0.4 + 1;  
				
				Whr.Sea2.Amp1 = SeaAmp; // 4.0
		        Whr.Sea2.AnimSpeed1 = 1.25;
		        Whr.Sea2.Scale1 = 0.6;
		        Whr.Sea2.MoveSpeed1 = "0.0, 0.0, 2.0";

		        Whr.Sea2.Amp2 = 2.0;
		        Whr.Sea2.AnimSpeed2 = 2.0;
		        Whr.Sea2.Scale2 = 1.5;
		        Whr.Sea2.MoveSpeed2 = "-2.0, 0.0, 0.0";

				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.0;//остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;

				Whr.Sea2.FoamK = 0.4;//§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = SeaAmp * 0.85;//высота с которой начинаетс§ пена
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.4;
	            Whr.Sea2.Transparency = 0.1;
	            Whr.Sea2.Frenel = 0.5;
	            // Whr.Sea2.Attenuation = 0.2 + 1 * 0.05;
				Whr.Sea2.Attenuation = 0.2;
			    Whr.Sea2.WaterColor = argb(0, 0, 40, 70);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;

			case "breeze":	//бриз 2 8.15
			    SeaAmp = fWind * 0.60 + 1;  
				
				Whr.Sea2.Amp1 = SeaAmp; // 8.0
		        Whr.Sea2.AnimSpeed1 = 1.5;
		        Whr.Sea2.Scale1 = 0.35;
		        Whr.Sea2.MoveSpeed1 = "0.0, 0.0, 3.0";

		        Whr.Sea2.Amp2 = 3.0;
		        Whr.Sea2.AnimSpeed2 = 3.0;
		        Whr.Sea2.Scale2 = 1.25;
		        Whr.Sea2.MoveSpeed2 = "1.0, 0.0, 1.0";

				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.0;//остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;

				Whr.Sea2.FoamK = 0.45;//§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = SeaAmp * 0.8;//высота с которой начинаетс§ пена 6.0
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.4;
	            Whr.Sea2.Transparency = 0.1;
	            Whr.Sea2.Frenel = 0.6;
	            // Whr.Sea2.Attenuation = 0.2 + 2 * 0.05;
				Whr.Sea2.Attenuation = 0.2;
			    Whr.Sea2.WaterColor = argb(0, 0, 40, 70);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.3;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;

			case "moderate_breeze":	//бриз 3
			    SeaAmp = fWind * 0.75 + 1;  
				
				Whr.Sea2.Amp1 = SeaAmp; // 12.0 - 9.25
		        Whr.Sea2.AnimSpeed1 = 1.65;
		        Whr.Sea2.Scale1 = 0.25;
		        Whr.Sea2.MoveSpeed1 = "0.0, 0.0, 4.0";

		        Whr.Sea2.Amp2 = 3.5;
		        Whr.Sea2.AnimSpeed2 = 3.5;
		        Whr.Sea2.Scale2 = 1.15;
		        Whr.Sea2.MoveSpeed2 = "0.0, 0.0, 1.0";

				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.0; //остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;
				
				Whr.Sea2.FoamK = 0.4;//§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = SeaAmp * 0.75;//высота с которой начинаетс§ пена 8.0
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.4;
	            Whr.Sea2.Transparency = 0.1;
	            Whr.Sea2.Frenel = 0.6;
	            // Whr.Sea2.Attenuation = 0.2 + 3 * 0.05;
				Whr.Sea2.Attenuation = 0.2;
			    Whr.Sea2.WaterColor = argb(0, 0, 40, 70);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;

			case "strong_breeze"://ветрено 4
			    SeaAmp = fWind * 0.85 + 1;  
				
				Whr.Sea2.Amp1 = SeaAmp; // 16.0 - 13
		        Whr.Sea2.AnimSpeed1 = 1.75;
		        Whr.Sea2.Scale1 = 0.20;
		        Whr.Sea2.MoveSpeed1 = "0.0, 0.0, 5.0";

		        Whr.Sea2.Amp2 = 4.0;
		        Whr.Sea2.AnimSpeed2 = 4.0;
		        Whr.Sea2.Scale2 = 1.0;
		        Whr.Sea2.MoveSpeed2 = "0.0, 0.0, -1.0";

				
				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.0; //остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;

				Whr.Sea2.FoamK = 0.2; //§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = SeaAmp * 0.7; //высота с которой начинаетс§ пена 10.5
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.4;
	            Whr.Sea2.Transparency = 0.1;
	            Whr.Sea2.Frenel = 0.6;
	            // Whr.Sea2.Attenuation = 0.2 + 4 * 0.05;
				Whr.Sea2.Attenuation = 0.2;
			    Whr.Sea2.WaterColor = argb(0, 0, 40, 70);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;
			
            case "strong wind"://ветрено 5 17.5 - 20
			    SeaAmp = fWind * 1.55 + 1;  
				
				Whr.Sea2.Amp1 = SeaAmp; // 32.0 - 28.0
		        Whr.Sea2.AnimSpeed1 = 3.75;
		        Whr.Sea2.Scale1 = 0.125;
		        Whr.Sea2.MoveSpeed1 = "8.0, 0.0, 0.0";

		        Whr.Sea2.Amp2 = 6.0;
		        Whr.Sea2.AnimSpeed2 = 6.0;
		        Whr.Sea2.Scale2 = 0.75;
		        Whr.Sea2.MoveSpeed2 = "0.0, 0.0, -1.0";

				
				Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 2.5;//остроконечность волн
                Whr.Sea2.LodScale = 0.399;
				Whr.Sea2.GridStep = 0.081;

				Whr.Sea2.FoamK = 0.15;//§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = SeaAmp * 0.65;//высота с которой начинаетс§ пена 21.0
				Whr.Sea2.FoamUV = 0.4; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.7;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.3;
	            Whr.Sea2.Transparency = 0.1;
	            Whr.Sea2.Frenel = 0.6;
	            // Whr.Sea2.Attenuation = 0.2 + 5 * 0.05;
				Whr.Sea2.Attenuation = 0.2;
			    Whr.Sea2.WaterColor = argb(0, 10, 30, 50);
				
			if (GetTime() < 6.0 || GetTime() >= 21.0) {
				Whr.Sea2.FoamK = 0.1;
				Whr.Sea2.Reflection = 0.5;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }else {
				
			if (CheckAttribute(&WeatherParams,"Rain") && sti(WeatherParams.Rain) == true){
				Whr.Sea2.Reflection = 0.05;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
			}}	
			break;
			
			case "storm"://шторм 5
				
				Whr.Sea2.Amp1 = 64.0;
		        Whr.Sea2.AnimSpeed1 = 4.25;
		        Whr.Sea2.Scale1 = 0.075;
 		        Whr.Sea2.MoveSpeed1 = "8.0, 0.0, 0.0";

		        Whr.Sea2.Amp2 = 9.0;
		        Whr.Sea2.AnimSpeed2 = 8.0;
		        Whr.Sea2.Scale2 = 0.5;
		        Whr.Sea2.MoveSpeed2 = "12.0, 0.0, 0.0";

				Whr.Sea2.BumpScale = 0.1; //Ю?О†ї? ђЭОНЯ
				Whr.Sea2.PosShift = 4.25;//остроконечность волн
                Whr.Sea2.LodScale = 0.5;
				Whr.Sea2.GridStep = 0.05;

				Whr.Sea2.FoamK = 0.05;//§ркость пены по высоте 0.1 - пена §рка§ с 10 метров 1.0 через метр после начала
				Whr.Sea2.FoamV = 42.0;//высота с которой начинаетс§ пена
				Whr.Sea2.FoamUV = 0.3; //тайлинг пены
				Whr.Sea2.FoamTexDisturb = 0.5;//сдвиг по нормали. х.з. что это надо поюзать. значение от 0.0 до 1.0

				Whr.Sea2.Reflection = 0.35;
	            Whr.Sea2.Transparency = 0.25;
	            Whr.Sea2.Frenel = 0.3;
	            Whr.Sea2.Attenuation = 0.2 + 5 * 0.05;
			    Whr.Sea2.WaterColor = argb(0, 10, 25, 40);
				
			if (GetTime() < 6.0 || GetTime() >= 20.0) {
				Whr.Sea2.FoamK = 0.02;
				Whr.Sea2.Reflection = 0.3;
				Whr.Sea2.Transparency = 0.1;
				Whr.Sea2.WaterColor = argb(0,5,10,20);
				
	        }
			break;
			
case "light_breeze_land":	//лёгкий ветер суша
			Whr.Sea2.Amp1 = 0.5;
			Whr.Sea2.AnimSpeed1 = 1.0;
			Whr.Sea2.Scale1 = 1.75;
			Whr.Sea2.MoveSpeed1 = "0.5, 0.0, 0.0";

			Whr.Sea2.Amp2 = 0.5;
			Whr.Sea2.AnimSpeed2 = 1.0;
			Whr.Sea2.Scale2 = 1.75;
			Whr.Sea2.MoveSpeed2 = "0.0, 0.0, -0.5";

			Whr.Sea2.BumpScale = 0.15; 
			Whr.Sea2.PosShift = 1.75;

			Whr.Sea2.FoamV = 5.0;
			Whr.Sea2.FoamK = 0.0;
			Whr.Sea2.FoamUV = 0.3;
			Whr.Sea2.FoamTexDisturb = 0.7;
			Whr.Sea2.Frenel = 0.3;
			Whr.Sea2.Attenuation = 0.3;
			Whr.Sea2.WaterColor = argb(0,0,255,225);
		break;

		case "strong_breeze_land": //сильный ветер суша
			Whr.Sea2.Amp1 = 3.0;
			Whr.Sea2.AnimSpeed1 = 1.0;
			Whr.Sea2.Scale1 = 2.5;
			Whr.Sea2.MoveSpeed1 = "0.5, 0.0, 0.0";

			Whr.Sea2.Amp2 = 3.0;
			Whr.Sea2.AnimSpeed2 = 1.0;
			Whr.Sea2.Scale2 = 2.5;
			Whr.Sea2.MoveSpeed2 = "0.0, 0.0, -0.5";

			Whr.Sea2.BumpScale = 0.15; 
			Whr.Sea2.PosShift = 1.3;
			// Whr.Sea2.BumpScale = 0.08; //Ю?О†ї? ђЭОНЯ
			// Whr.Sea2.PosShift = 2.0;//остроконечность волн

			Whr.Sea2.FoamV = 0.8;
			Whr.Sea2.FoamK = 3.1;
			Whr.Sea2.FoamUV = 0.5;
			Whr.Sea2.FoamTexDisturb = 0.5;
			Whr.Sea2.Frenel = 0.3;
			Whr.Sea2.Attenuation = 0.2;	
			Whr.Sea2.WaterColor = argb(0,0,170,150);
            
			if (CheckAttribute(&WeatherParams, "Storm") && sti(WeatherParams.Storm) == 1){
			Whr.Sea2.SkyColor = argb(0,60,70,80);	
			Whr.Sea2.FoamK = 1.2;
			}/* else {
			if (GetTime() < 6.0 || GetTime() >= 20.0)	{
			if (CheckAttribute(&WeatherParams, "Storm") && sti(WeatherParams.Storm) == 1){
			Whr.Sea2.SkyColor = argb(0,60,70,80);	
			Sea.Fog.SeaDensity = 0.02;
			Whr.Sea2.FoamK = 1.2;
			Sea.Sea2.Reflection = 0.03;
			Sea.Sea2.WaterColor = argb(0,0,30,30);
			}}} */
            break;	
	}
	
}	

void WhrBindNativeSeaProfiles()
{
    // Location, fort, shore and cabin overrides keep their existing owner.
    if (bCabinStarted || FindLocation(pchar.location) != -1) return;
    aref entry, sample;
    string key;
    Sea.NativeSeaProfiles.Count = 6;
    for (int i = 0; i < 6; i++) {
        key = "p" + i;
        makearef(entry, Sea.NativeSeaProfiles.(key));
        entry.MinWind = WhrSeaPresetMinWind(i);
        makearef(sample, entry.AtZero);
        WhrFillSeaPreset(sample, WhrSeaWindPreset(i), 0.0);
        makearef(sample, entry.AtOne);
        WhrFillSeaPreset(sample, WhrSeaWindPreset(i), 1.0);
    }
    makearef(sample, Sea.NativeSeaProfiles.Storm);
    WhrFillSeaPreset(sample, "storm", 0.0);
    Sea.NativeSeaProfilesReady = true;
}
	
// подсветку делаем динамической, где 2 часа ночи - мин, 2 часа дня - макс. +обновляем блик под новую текстуру отражения
void SM_ModifyLightness()
{
	float lightIntencity = SM_CurrentHourToLightIntencity();
	int r, g, b;
		
	r = 255;
	g = 255;
	b = 255;
	
	r = makeint(r * lightIntencity);
	g = makeint(g * lightIntencity);
	b = makeint(b * lightIntencity);
	
	aref weather = GetCurrentWeather();
	
	weather.Sky.Color = argb(0, r, g, b);
	Weather.Sky.Color = weather.Sky.Color;
	
	weather.Sun.Reflection.Size = 100 + 500 * lightIntencity;
	
Log_TestInfo("SM_ModifyLightness: выбрано значение " + lightIntencity);
}	




/*
 * Пересчет игровых координат глобальной карты в реальные (условные) географические координаты Карибского архипелага
 * Ugeen - 12.08.09
 */
#event_handler("frame", "CheckMapCoordinateQuest");
 
#define WDM_RealCoordStartX		60	// начало координат по X - градусы западной долготы
#define WDM_RealCoordEndX		85	// конец координат по X - градусы западной долготы
#define WDM_RealCoordStartZ		10	// начало координат по Z - градусы северной широты
#define WDM_RealCoordEndZ		25	// конец координат по Z - градусы северной широты

float fShiftX = 1024.0; // смещение игровых координат по X
float fShiftZ = 1024.0; // смещение игровых координат по Z

float fScaleX = 2048.0; 
float fScaleZ = 2048.0;

string sCurCoordinateQuest = "";

void SetMapCoordinateQuest(string _sQuest)
{
	SetEventHandler(_sQuest, _sQuest, 0); // Ставим евент
	sCurCoordinateQuest = _sQuest;
}

void CheckMapCoordinateQuest() // Процессирование всего
{
	PostEvent(sCurCoordinateQuest, 10);
}

void DeleteMapCoordinateQuest(string _sQuest)
{
	sCurCoordinateQuest = "";
	DelEventHandler(_sQuest, _sQuest); // Удалим евент
}

/*
--> пример использования координатной сетки для квестов  
void CheckQuestCoordinates()
{
	int coordinateDegreeX = 70;
	int coordinateDegreeZ = 15;

	if(IsEntity(worldMap)) // если на глобальной карте
	{
		Log_SetStringToLog("X : " + GetMapCoordDegreeX(makefloat(worldMap.playerShipX)) + " Z : " + GetMapCoordDegreeZ(makefloat(worldMap.playerShipZ)));
		if( (GetMapCoordDegreeX(makefloat(worldMap.playerShipX)) == coordinateDegreeX) &&
			(GetMapCoordDegreeZ(makefloat(worldMap.playerShipZ)) == coordinateDegreeZ) )
		{
			DeleteMapCoordinateQuest("CheckQuestCoordinates");
			Log_SetStringToLog("Текущие координаты : " + Map_GetRealCoordZ(makefloat(worldMap.playerShipZ)) + "  " + Map_GetRealCoordX(makefloat(worldMap.playerShipX)));
			Log_SetStringToLog("Достигли искомой точки - 70 гр. з.д. 15 гр. с.ш.");
		}
	}
}
*/

// получить масштаб глобальной карты отосительно режима "море"
int GetSeaToMapScale()
{
	int scale = WDM_MAP_TO_SEA_SCALE;
	if (worldMap.island == "Cuba1" || worldMap.island == "Cuba2" || worldMap.island == "Beliz" || worldMap.island == "SantaCatalina" 
		|| worldMap.island == "PortoBello" || worldMap.island == "Cartahena" || worldMap.island == "Maracaibo"
		|| worldMap.island == "Caracas" || worldMap.island == "Cumana")
	{
		scale = 25;
	}
	return scale;
}

bool Sea_HasAtlasFrame()
{
	return CheckAttribute(&AISea, "worldFrame.version") && sti(AISea.worldFrame.version) == 2 &&
		CheckAttribute(&AISea, "worldFrame.sea_to_atlas.a") &&
		CheckAttribute(&AISea, "worldFrame.atlas_to_sea.a") &&
		CheckAttribute(&AISea, "worldFrame.scale") && sti(AISea.worldFrame.scale) > 0;
}

float Sea_FramePointX(aref frame, float x, float z)
{
	return stf(frame.a)*x + stf(frame.b)*z + stf(frame.tx);
}

float Sea_FramePointZ(aref frame, float x, float z)
{
	return stf(frame.c)*x + stf(frame.d)*z + stf(frame.tz);
}

void Sea_ComposeFrame(ref result, aref outer, aref inner)
{
	// Read both operands before writing: result may alias either source.
	float a = stf(outer.a); float b = stf(outer.b);
	float c = stf(outer.c); float d = stf(outer.d);
	float tx = stf(outer.tx); float tz = stf(outer.tz);
	float e = stf(inner.a); float f = stf(inner.b);
	float g = stf(inner.c); float h = stf(inner.d);
	float ux = stf(inner.tx); float uz = stf(inner.tz);
	result.a = a*e+b*g; result.b = a*f+b*h;
	result.c = c*e+d*g; result.d = c*f+d*h;
	result.tx = a*ux+b*uz+tx; result.tz = c*ux+d*uz+tz;
}

float Sea_MapX(float x, float z)
{
	if (!Sea_HasAtlasFrame()) return GetSeaShipX(x);
	aref frame; makearef(frame, AISea.worldFrame.sea_to_atlas);
	return Sea_FramePointX(frame, x, z)/stf(AISea.worldFrame.scale);
}

float Sea_MapZ(float x, float z)
{
	if (!Sea_HasAtlasFrame()) return GetSeaShipZ(z);
	aref frame; makearef(frame, AISea.worldFrame.sea_to_atlas);
	return Sea_FramePointZ(frame, x, z)/stf(AISea.worldFrame.scale);
}

float Sea_LocalX(float x, float z)
{
	if (!Sea_HasAtlasFrame()) return GetSeaLocalX(x);
	aref frame; makearef(frame, AISea.worldFrame.atlas_to_sea);
	return Sea_FramePointX(frame, x*stf(AISea.worldFrame.scale), z*stf(AISea.worldFrame.scale));
}

float Sea_LocalZ(float x, float z)
{
	if (!Sea_HasAtlasFrame()) return GetSeaLocalZ(z);
	aref frame; makearef(frame, AISea.worldFrame.atlas_to_sea);
	return Sea_FramePointZ(frame, x*stf(AISea.worldFrame.scale), z*stf(AISea.worldFrame.scale));
}

float Sea_MapHeading(float ay)
{
	if (!Sea_HasAtlasFrame()) return ay;
	return ay + atan2(stf(AISea.worldFrame.sea_to_atlas.b), stf(AISea.worldFrame.sea_to_atlas.d));
}

float Sea_LocalHeading(float ay)
{
	if (!Sea_HasAtlasFrame()) return ay;
	return ay + atan2(stf(AISea.worldFrame.atlas_to_sea.b), stf(AISea.worldFrame.atlas_to_sea.d));
}

bool Sea_HasWorldFrame()
{
	if (Sea_HasAtlasFrame()) return true;
	return CheckAttribute(&AISea, "worldFrame.version") && sti(AISea.worldFrame.version) == 1 &&
		CheckAttribute(&AISea, "worldFrame.zeroX") && CheckAttribute(&AISea, "worldFrame.zeroZ") &&
		CheckAttribute(&AISea, "worldFrame.scale") && sti(AISea.worldFrame.scale) > 0;
}

void Sea_InitWorldFrame()
{
	if (Sea_HasWorldFrame()) return;
	AISea.worldFrame.zeroX = stf(worldMap.zeroX);
	AISea.worldFrame.zeroZ = stf(worldMap.zeroZ);
	AISea.worldFrame.scale = GetSeaToMapScale();
	AISea.worldFrame.version = 1;
}

int GetSeaFrameScale()
{
	if (Sea_HasWorldFrame()) return sti(AISea.worldFrame.scale);
	return GetSeaToMapScale();
}

float GetSeaShipX(float X)
{
	if (Sea_HasAtlasFrame()) return Sea_MapX(X, stf(pchar.Ship.Pos.z));
	float zeroX = MakeFloat(worldMap.zeroX);
	if (Sea_HasWorldFrame()) zeroX = stf(AISea.worldFrame.zeroX);
	int scale = GetSeaFrameScale();
	float ShipX = (X/scale) + zeroX;

	return ShipX;
}

float GetSeaShipZ(float Z)
{
	if (Sea_HasAtlasFrame()) return Sea_MapZ(stf(pchar.Ship.Pos.x), Z);
	float zeroZ = MakeFloat(worldMap.zeroZ);
	if (Sea_HasWorldFrame()) zeroZ = stf(AISea.worldFrame.zeroZ);
	int scale = GetSeaFrameScale();
	float ShipZ = (Z/scale) + zeroZ;	
	
	return ShipZ;
}

float GetSeaLocalX(float mapX)
{
	float zeroX = stf(worldMap.zeroX);
	if (Sea_HasWorldFrame()) zeroX = stf(AISea.worldFrame.zeroX);
	return (mapX - zeroX) * GetSeaFrameScale();
}

float GetSeaLocalZ(float mapZ)
{
	float zeroZ = stf(worldMap.zeroZ);
	if (Sea_HasWorldFrame()) zeroZ = stf(AISea.worldFrame.zeroZ);
	return (mapZ - zeroZ) * GetSeaFrameScale();
}

void Sea_RegionFrame(string islandID, ref frame)
{
	frame.a = 1.0; frame.b = 0.0; frame.c = 0.0; frame.d = 1.0;
	frame.tx = 0.0; frame.tz = 0.0;
	if (!CheckAttribute(&AISea, "worldFrame.regions.count")) return;
	aref regions; makearef(regions, AISea.worldFrame.regions);
	for (int i = 0; i < sti(regions.count); i++)
	{
		string key = "r" + i;
		if (regions.(key).id != islandID) continue;
		if (Sea_HasAtlasFrame())
		{
			aref outer, inner;
			makearef(outer, AISea.worldFrame.atlas_to_sea);
			makearef(inner, regions.(key).local_to_atlas);
			Sea_ComposeFrame(frame, outer, inner);
		}
		else
		{
			frame.tx = GetSeaLocalX(stf(regions.(key).mapX));
			frame.tz = GetSeaLocalZ(stf(regions.(key).mapZ));
		}
		return;
	}
}

void Sea_LegacyRegionFrame(string islandID, ref frame)
{
	Sea_RegionFrame(islandID, frame);
	if (!CheckAttribute(&AISea, "worldFrame.legacy_resident_frame.regions.count")) return;
	aref legacy; makearef(legacy, AISea.worldFrame.legacy_resident_frame);
	for (int i = 0; i < sti(legacy.regions.count); i++)
	{
		string key = "r" + i;
		if (legacy.regions.(key).id != islandID) continue;
		float dx = (stf(legacy.regions.(key).mapX)-stf(legacy.zeroX))*stf(legacy.scale);
		float dz = (stf(legacy.regions.(key).mapZ)-stf(legacy.zeroZ))*stf(legacy.scale);
		frame.tx = stf(frame.tx)-stf(frame.a)*dx-stf(frame.b)*dz;
		frame.tz = stf(frame.tz)-stf(frame.c)*dx-stf(frame.d)*dz;
		return;
	}
}

float Sea_RegionX(string islandID, float x, float z)
{
	object frame; Sea_RegionFrame(islandID, &frame);
	return Sea_FramePointX(&frame, x, z);
}

float Sea_RegionZ(string islandID, float x, float z)
{
	object frame; Sea_RegionFrame(islandID, &frame);
	return Sea_FramePointZ(&frame, x, z);
}

float Sea_RegionHeading(string islandID, float ay)
{
	object frame; Sea_RegionFrame(islandID, &frame);
	return ay + atan2(stf(frame.b), stf(frame.d));
}

void Sea_PoseRegionModel(ref model, string islandID, float x, float y, float z)
{
	object frame; Sea_RegionFrame(islandID, &frame);
	SendMessage(model, "lffffffffffff", MSG_MODEL_SET_POSITION,
		Sea_FramePointX(&frame, x, z), y, Sea_FramePointZ(&frame, x, z),
		stf(frame.a), 0.0, stf(frame.c), 0.0, 1.0, 0.0, stf(frame.b), 0.0, stf(frame.d));
}

float Sea_RegionOffsetX(string islandID)
{
	return Sea_RegionX(islandID, 0.0, 0.0);
}

float Sea_RegionOffsetZ(string islandID)
{
	return Sea_RegionZ(islandID, 0.0, 0.0);
}

// секунды долготы относительно начала координат (глобальная карта)
int Map_GetRCoordSecondsX(float X)
{
	int iSecondsX = (WDM_RealCoordEndX - WDM_RealCoordStartX) * 3600;
	int iSecX = makeint((fShiftX - X)/fScaleX * iSecondsX);
	return iSecX;
}

// градусы долготы относительно начала координат (глобальная карта)
int Map_GetRCoordDegreeX(float X)
{
	int iDegX = Map_GetRCoordSecondsX(X);
	return makeint(iDegX/3600.0);
}

// секунды широты относительно начала координат (глобальная карта)
int Map_GetRCoordSecondsZ(float Z)
{
	int iSecondsZ = (WDM_RealCoordEndZ - WDM_RealCoordStartZ) * 3600;
	int iSecZ = makeint((fShiftZ + Z)/fScaleZ * iSecondsZ);
	return iSecZ;
}

// градусы широты относительно начала координат (глобальная карта)
int Map_GetRCoordDegreeZ(float Z)
{
	int iDegZ = Map_GetRCoordSecondsZ(Z);
	return makeint(iDegZ/3600.0);
}

int GetMapCoordDegreeX(float X)
{
	int iRDegX = Map_GetRCoordDegreeX(X);
	return iRDegX + WDM_RealCoordStartX;
}

int GetMapCoordMinutesX(float X)
{
	int iRDegX = Map_GetRCoordDegreeX(X);
	int iRSecX = Map_GetRCoordSecondsX(X);
	
	return makeint((iRSecX - iRDegX * 3600)/60);
}

int GetMapCoordDegreeZ(float Z)
{
	int iRDegZ = Map_GetRCoordDegreeZ(Z);
	return iRDegZ + WDM_RealCoordStartZ;
}

int GetMapCoordMinutesZ(float Z)
{
	int iRDegZ = Map_GetRCoordDegreeZ(Z);
	int iRSecZ = Map_GetRCoordSecondsZ(Z);
	
	return makeint((iRSecZ - iRDegZ * 3600)/60);
}

int GetSeaCoordDegreeX(float X)
{
	float ShipX = GetSeaShipX(X);
	return GetMapCoordDegreeX(ShipX);
}

int GetSeaCoordDegreeZ(float Z)
{
	float ShipZ = GetSeaShipZ(Z);
	return GetMapCoordDegreeZ(ShipZ);
}

int GetSeaCoordMinutesX(float X)
{
	float ShipX = GetSeaShipX(X);	
	return GetMapCoordMinutesX(X);
}

int GetSeaCoordMinutesZ(float Z)
{
	float ShipZ = GetSeaShipZ(Z);
	return GetMapCoordMinutesZ(Z);
}

//  строка координат долготы на глобальной карте
string Map_GetRealCoordX(float X)
{
	int iRSecX, iRMinX, iRDegX;
	
	iRDegX = Map_GetRCoordDegreeX(X);
	iRSecX = Map_GetRCoordSecondsX(X);
	
	iRMinX = makeint((iRSecX - iRDegX * 3600)/60);
	iRSecX = makeint(iRSecX - iRDegX * 3600 - iRMinX * 60);	
	iRDegX = iRDegX + WDM_RealCoordStartX;
	
	string sCoordX = iRDegX + " гр. " + iRMinX + "' " + iRSecX + "'' " + XI_ConvertString("LongitudeShort");	
	return sCoordX;
}

// строка координат широты на глобальной карте
string Map_GetRealCoordZ(float Z)
{
	int iRSecZ, iRMinZ, iRDegZ;
	
	iRDegZ = Map_GetRCoordDegreeZ(Z);
	iRSecZ = Map_GetRCoordSecondsZ(Z);
	
	iRMinZ = makeint((iRSecZ - iRDegZ * 3600)/60);
	iRSecZ = makeint(iRSecZ - iRDegZ * 3600 - iRMinZ * 60);	
	iRDegZ = iRDegZ + WDM_RealCoordStartZ;
	
	string sCoordZ = iRDegZ + " гр. " + iRMinZ + "' " + iRSecZ + "'' " + XI_ConvertString("LatitudeShort");
	return sCoordZ;
}

//  строка координат долготы в режиме "море"
string Sea_GetRealCoordX(float X)
{
	float ShipX = GetSeaShipX(X);
	string sCoordX = Map_GetRealCoordX(ShipX);
	return sCoordX;
}

// строка координат широты в режиме "море" 
string Sea_GetRealCoordZ(float Z)
{
	float ShipZ = GetSeaShipZ(Z);
	string sCoordZ = Map_GetRealCoordZ(ShipZ);
	return sCoordZ;
}

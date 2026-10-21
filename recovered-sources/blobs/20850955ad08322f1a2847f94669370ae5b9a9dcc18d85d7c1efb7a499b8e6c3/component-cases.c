bool bCanSwitchCameras,Redirect; int Flags,DeletedCameras;
#define COMPANION_MAX 3
#define MSG_SHIP_ADD_MOVE_IMPULSE 50308
#define SEA_PHONK_TRACK "phonk"
object AISea,worldMap,pchar,Sea,SeaMapTransition,InterfaceStates;

object Characters[3],Islands[2];

bool bSeaActive,bSeaLoaded,bSeaReloadStarted,bAbordageStarted,bDisableMapEnter,bQuestDisableMapEnter,bTornado,bSeaTsunamiEncounter,bSkipSeaLogin,wdmLockReload;

int Entries,Returns,Impulses,Music,Enabled,Target,Companions[3];

float CurrentScale;

bool AllowEntry;

bool FixtureEntity(aref a){return CheckAttribute(a,"exists");
}
bool LAi_IsDead(aref a){return CheckAttribute(a,"dead");
}
float GetSeaShipX(float x){return stf(worldMap.zeroX)+x/CurrentScale;
}
float GetSeaShipZ(float z){return stf(worldMap.zeroZ)+z/CurrentScale;
}
int FindIsland(string id){if(id=="Antigua")return 0;
return -1;
}
bool Sea_TryMapLoad(bool automatic){if(!AllowEntry)return false;
Entries++;
bSeaReloadStarted=true;
return true;
}
int GetCompanionIndex(aref a,int slot){return Companions[slot];
}
ref GetCharacter(int index){return &Characters[index];
}
float Ship_GetSailState(aref a){return stf(a.sail);
}
bool SeaPhonk_IsActive(){return CheckAttribute(&Sea,"Phonk.Active");
}
void SeaPhonk_InitControls(){Enabled++;
}
void SeaPhonk_Frame(){}
bool SeaPhonk_Music(string track){Music++;
return true;
}
void Ship_SetSailState(int index,float sails){Characters[index].sail=sails;
}
void SeaLogin(ref login){bSkipSeaLogin=Redirect;
}
void wdmReloadToSea(){Returns++;
wdmLockReload=true;
}
void FixtureMessage(aref a,string sig,int code,bool inertia,float x,float y,float z,float rx,float ry,float rz){a.impulse.x=x;
a.impulse.z=z;
Impulses++;
}
void BI_ShowInterface(bool visible){}
void DeleteSeaCamerasEnvironment(){DeletedCameras++;}
void wdmSetNationFlag(int nation){Flags++;}
void Sea_MapTransitionCommit(){}
void Sea_MapTransitionDone()
{
	DelEventHandler("Sea_MapTransitionDone", "Sea_MapTransitionDone");
	wdmLockReload = false;
	bSeaReloadStarted = false;
	bCanSwitchCameras = true;
	if (bSeaActive) BI_ShowInterface(true);
	DeleteAttribute(&worldMap, "seaTransition");
	DeleteAttribute(&worldMap, "transitionReady");
}
// Region travel reuses the ordinary survivor handback and sea login. Never
// replace terrain underneath an active battle or bypass map-entry admission.
#event_handler("Sea_DirectSailPosition", "Sea_DirectSailPosition");
#event_handler("Sea_DirectSailArea", "Sea_DirectSailArea");
#event_handler("WorldMap_TransitionReady", "Sea_DirectSailMapReady");
#event_handler("Sea_DirectSailAbort", "Sea_DirectSailAbort");
#event_handler("Sea_DirectSailShipPosition", "Sea_DirectSailShipPosition");

float SeaDirectSailShipPosition[3];

ref Sea_DirectSailPosition()
{
    AISea.directSail.enabled = 0;
    if (!bSeaActive || !bSeaLoaded || bSeaReloadStarted || bAbordageStarted ||
        bDisableMapEnter || bQuestDisableMapEnter || bTornado || bSeaTsunamiEncounter) return &AISea;
    if (FixtureEntity(&worldMap) || FixtureEntity(&SeaMapTransition) ||
        CheckAttribute(&worldMap, "directSail") || sti(InterfaceStates.Launched) || LAi_IsDead(pchar)) return &AISea;
    if (!CheckAttribute(pchar, "Ship.Pos.x") || !CheckAttribute(pchar, "Ship.Pos.z")) return &AISea;
    AISea.directSail.x = GetSeaShipX(stf(pchar.Ship.Pos.x));
    AISea.directSail.z = GetSeaShipZ(stf(pchar.Ship.Pos.z));
    AISea.directSail.enabled = 1;
    return &AISea;
}

void Sea_DirectSailArea()
{
    string island = GetEventData();
    ref position = Sea_DirectSailPosition();
    if (!sti(position.directSail.enabled) || island == "" || island == pchar.location) return;
    int index = FindIsland(island);
    if (index < 0 || !sti(Islands[index].visible)) return;
    worldMap.directSail = 1;
    if (!Sea_TryMapLoad(true)) DeleteAttribute(&worldMap, "directSail");
}

void Sea_DirectSailCapture()
{
    if (!CheckAttribute(&worldMap, "directSail")) return;
    DeleteAttribute(&worldMap, "directSail.fleet");
    if (CheckAttribute(pchar, "GroupShipPos_event")) worldMap.directSail.previousPositionEvent = pchar.GroupShipPos_event;
    for (int i = 0; i < COMPANION_MAX; i++)
    {
        int index = GetCompanionIndex(pchar, i);
        if (index < 0) continue;
        ref captain = GetCharacter(index);
        if (!FixtureEntity(captain) || !CheckAttribute(captain, "Ship.Pos.x")) continue;
        string key = "c" + index;
        worldMap.directSail.fleet.(key).dx = stf(captain.Ship.Pos.x) - stf(pchar.Ship.Pos.x);
        worldMap.directSail.fleet.(key).dz = stf(captain.Ship.Pos.z) - stf(pchar.Ship.Pos.z);
        worldMap.directSail.fleet.(key).ay = captain.Ship.Ang.y;
        worldMap.directSail.fleet.(key).speedX = captain.Ship.Speed.x;
        worldMap.directSail.fleet.(key).speedZ = captain.Ship.Speed.z;
        worldMap.directSail.fleet.(key).sails = Ship_GetSailState(captain);
    }
    if (SeaPhonk_IsActive())
    {
        aref source, destination;
        makearef(source, Sea.Phonk);
        makearef(destination, worldMap.directSail.phonk);
        CopyAttributes(destination, source);
    }
}

void Sea_DirectSailBind()
{
    if (CheckAttribute(&worldMap, "directSail.fleet")) pchar.GroupShipPos_event = "Sea_DirectSailShipPosition";
}

void Sea_DirectSailLogin(ref login)
{
    bool retainedCameras = CheckAttribute(&worldMap, "directSail.retainCameras");
    Sea_DirectSailBind();
    SeaLogin(login);
    if (bSkipSeaLogin)
    {
        Sea_DirectSailCancel();
        if (retainedCameras) DeleteSeaCamerasEnvironment();
    }
}

ref Sea_DirectSailShipPosition()
{
    int slot = GetEventData();
    float x = GetEventData();
    float ay = GetEventData();
    float z = GetEventData();
    aref captain = GetEventData();
    string path = "directSail.fleet.c" + captain.index;
    SeaDirectSailShipPosition[0] = x - slot * 250.0 * sin(ay);
    SeaDirectSailShipPosition[1] = ay;
    SeaDirectSailShipPosition[2] = z - slot * 250.0 * cos(ay);
    if (CheckAttribute(&worldMap, path))
    {
        SeaDirectSailShipPosition[0] = x + stf(worldMap.(path).dx);
        SeaDirectSailShipPosition[1] = stf(worldMap.(path).ay);
        SeaDirectSailShipPosition[2] = z + stf(worldMap.(path).dz);
    }
    return &SeaDirectSailShipPosition;
}

void Sea_DirectSailCancel()
{
    if (!CheckAttribute(&worldMap, "directSail")) return;
    if (CheckAttribute(&worldMap, "directSail.previousPositionEvent"))
        pchar.GroupShipPos_event = worldMap.directSail.previousPositionEvent;
    else if (CheckAttribute(pchar, "GroupShipPos_event") && pchar.GroupShipPos_event == "Sea_DirectSailShipPosition")
        DeleteAttribute(pchar, "GroupShipPos_event");
    DeleteAttribute(&worldMap, "directSail");
}

void Sea_DirectSailRestore()
{
    if (!CheckAttribute(&worldMap, "directSail.fleet")) return;
    for (int i = 0; i < COMPANION_MAX; i++)
    {
        int index = GetCompanionIndex(pchar, i);
        string path = "directSail.fleet.c" + index;
        if (index < 0 || !CheckAttribute(&worldMap, path)) continue;
        ref captain = GetCharacter(index);
        if (!FixtureEntity(captain)) continue;
        float ay = stf(captain.Ship.Ang.y);
        float sx = stf(worldMap.(path).speedX);
        float sz = stf(worldMap.(path).speedZ);
        // A one-shot native impulse restores motion without inventing a new
        // physics setter or keeping a permanent force on the ship.
        FixtureMessage(captain, "llffffff", MSG_SHIP_ADD_MOVE_IMPULSE, false,
            sx * cos(ay) + sz * sin(ay), 0.0, sz * cos(ay) - sx * sin(ay), 0.0, 0.0, 0.0);
        Ship_SetSailState(index, stf(worldMap.(path).sails));
    }
    if (CheckAttribute(&worldMap, "directSail.phonk"))
    {
        aref source, destination;
        makearef(source, worldMap.directSail.phonk);
        makearef(destination, Sea.Phonk);
        CopyAttributes(destination, source);
        SeaPhonk_InitControls();
        SetEventHandler("frame", "SeaPhonk_Frame", 0);
        SeaPhonk_Music(SEA_PHONK_TRACK);
    }
    Sea_DirectSailCancel();
}

void Sea_DirectSailMapReady()
{
    if (!CheckAttribute(&worldMap, "directSail") || !FixtureEntity(&worldMap) ||
        !FixtureEntity(&SeaMapTransition) || !CheckAttribute(&SeaMapTransition, "seaOnly")) return;
    if (CheckAttribute(&worldMap, "directSail.returning")) return;
    worldMap.directSail.returning = 1;
    // Populate ordinary descriptors without starting the map interface,
    // audio, calendar or its delayed EventWorldMapInit quest event.
    worldMap.isLoaded = "true";
    worldMap.update = "";
    worldMap.addQuestEncounters = "updateQuest";
    wdmSetNationFlag(sti(pchar.nation));
    wdmLockReload = false;
    wdmReloadToSea();
}

void Sea_DirectSailAbort()
{
    // Native capture failed before teardown. Keep the original sea and audio.
    if (!CheckAttribute(&worldMap, "directSail") || !bSeaActive) return;
    DelEventHandler("Sea_MapTransitionCommit", "Sea_MapTransitionCommit");
    Sea_DirectSailCancel();
    Sea_MapTransitionDone();
}

void Setup(){
DeleteAttribute(&worldMap,"");
DeleteAttribute(&pchar,"");
DeleteAttribute(&Sea,"");
DeleteAttribute(&SeaMapTransition,"");
DeleteAttribute(&AISea,"");
DeleteAttribute(&InterfaceStates,"");

bSeaActive=true;
bSeaLoaded=true;
bSeaReloadStarted=false;
bAbordageStarted=false;
bDisableMapEnter=false;
bQuestDisableMapEnter=false;
bTornado=false;
bSeaTsunamiEncounter=false;
wdmLockReload=false;
AllowEntry=true;
Entries=0;
Returns=0;
Impulses=0;

InterfaceStates.Launched=0;
pchar.location="Barbados";
pchar.Ship.Pos.x=2500.0;
pchar.Ship.Pos.z=5000.0;
worldMap.zeroX=900.0;
worldMap.zeroZ=-400.0;
CurrentScale=50.0;
Islands[0].visible=1;

for(int i=0;i<3;i++){Companions[i]=i;
DeleteAttribute(&Characters[i],"");
Characters[i].exists=1;
Characters[i].index=i;
Characters[i].Ship.Pos.x=2500.0+i*123.0;
Characters[i].Ship.Pos.z=5000.0-i*71.0;
Characters[i].Ship.Ang.y=0.7;
Characters[i].Ship.Speed.x=2.0;
Characters[i].Ship.Speed.z=17.0;
Characters[i].sail=0.5;
Characters[i].Ship.HP=333.0;
Characters[i].cargo=777;
}
}
float FixtureAbs(float value){if(value<0.0)return -value;
return value;
}
int Run(){
Setup(); pchar.nation=0;
ref state=Sea_DirectSailPosition();
if(!sti(state.directSail.enabled) || stf(state.directSail.x)!=950.0 || stf(state.directSail.z)!=-300.0)return 1;

CurrentScale=25.0;
state=Sea_DirectSailPosition();
if(stf(state.directSail.x)!=1000.0)return 2;
CurrentScale=50.0;

Event("Sea_DirectSailArea","s","Barbados");
if(Entries!=0 || CheckAttribute(&worldMap,"directSail"))return 3;

Islands[0].visible=0;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 4;
Islands[0].visible=1;

bDisableMapEnter=true;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 5;
bDisableMapEnter=false;

bQuestDisableMapEnter=true;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 6;
bQuestDisableMapEnter=false;

bAbordageStarted=true;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 7;
bAbordageStarted=false;

bSeaTsunamiEncounter=true;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 8;
bSeaTsunamiEncounter=false;

InterfaceStates.Launched=1;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0)return 9;
InterfaceStates.Launched=0;

AllowEntry=false;
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=0 || CheckAttribute(&worldMap,"directSail"))return 10;
AllowEntry=true;

pchar.GroupShipPos_event="existing_callback";
Sea.Phonk.Active=1;
Sea.Phonk.Weapon=7;

Event("Sea_DirectSailArea","s","Antigua");
Event("Sea_DirectSailArea","s","Antigua");
if(Entries!=1 || !CheckAttribute(&worldMap,"directSail"))return 11;

Sea_DirectSailCapture();
Sea_DirectSailBind();
if(pchar.GroupShipPos_event!="Sea_DirectSailShipPosition")return 12;

ref positionCaptain=GetCharacter(1);
float targetX=-250.0;
Event("Sea_DirectSailShipPosition","lfffa",1,targetX,0.7,800.0,positionCaptain);
if(SeaDirectSailShipPosition[0]!=-127.0 || SeaDirectSailShipPosition[2]!=729.0 || SeaDirectSailShipPosition[1]<0.69  ||  SeaDirectSailShipPosition[1]>0.71)return 13;

worldMap.exists=1;
wdmLockReload=true;
SeaMapTransition.exists=1; SeaMapTransition.seaOnly=1;
Event("WorldMap_TransitionReady");
Event("WorldMap_TransitionReady");
if(Returns!=1 || Flags!=1 || !wdmLockReload)return 30;
if(!CheckAttribute(&worldMap,"directSail.returning"))return 31;
DeleteAttribute(&Sea,"Phonk");
Sea_DirectSailRestore();
if(Impulses!=3 || Music!=1 || Enabled!=1 || sti(Sea.Phonk.Weapon)!=7 || pchar.GroupShipPos_event!="existing_callback" || CheckAttribute(&worldMap,"directSail"))return 15;

for(int i=0;i<3;i++){if(stf(Characters[i].impulse.x)<12.480385  ||  stf(Characters[i].impulse.x)>12.482385 || stf(Characters[i].impulse.z)<11.712882  ||  stf(Characters[i].impulse.z)>11.714882 || stf(Characters[i].Ship.HP)!=333.0 || sti(Characters[i].cargo)!=777)return 16;
}
worldMap.directSail=1; worldMap.directSail.retainCameras=1;
Redirect=true; Sea_DirectSailLogin(&worldMap);
if(DeletedCameras!=1 || CheckAttribute(&worldMap,"directSail"))return 32;
Redirect=false; worldMap.directSail=1; bSeaReloadStarted=true; bCanSwitchCameras=false;
SetEventHandler("Sea_MapTransitionCommit","Sea_MapTransitionCommit",0);
SetEventHandler("Sea_MapTransitionDone","Sea_MapTransitionDone",0);
Sea_DirectSailAbort();
if(bSeaReloadStarted || !bCanSwitchCameras || CheckAttribute(&worldMap,"directSail"))return 33;
return 0;
}

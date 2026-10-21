#include "sea_ai\PortSave.c"
#include "SEA_AI\PortTravelRig.c"

object PortRoles, PortBoat, PortRetiredHero;
bool PortPreparing = false;
bool PortRetiring = false;
bool PortAddressOnly = false;
int PortPrepareCursor = 0;

bool Port_IsResident()
{
    if (PortRetiring || !IsEntity(&PortRoles) || !IsEntity(&AISea)) return false;
    return CheckAttribute(pchar, "PortTravel.resident") && sti(pchar.PortTravel.resident) == 1;
}

bool Port_IsLandHero()
{
    return Port_IsResident() && CheckAttribute(pchar, "PortTravel.mode") && pchar.PortTravel.mode == "land";
}

bool Port_HeroAwayFromShip()
{
    return Port_IsResident() && CheckAttribute(pchar, "PortTravel.mode") &&
        (pchar.PortTravel.mode == "land" || pchar.PortTravel.mode == "boat");
}

bool Port_PreparingContext()
{
    return PortPreparing || locResidentPreparing;
}

bool Port_IsSupportedLocation(string id)
{
    int index = FindLocation(id);
    if (index < 0) return false;
    ref loc = &Locations[index];
    if (id == "SanJuan_town") return true;
    if (!CheckAttribute(loc, "type")) return false;
    if (loc.type != "tavern" && loc.type != "house" && loc.type != "shop" && loc.type != "shipyard" &&
        loc.type != "residence" && loc.type != "church" && loc.type != "jail") return false;
    if (CheckAttribute(loc, "PortContext.region") && loc.PortContext.region == "PuertoRico") return true;
    if (CheckAttribute(loc, "townsack") && loc.townsack == "SanJuan") return true;
    return findsubstr(id, "SanJuan_", 0) == 0;
}

bool Port_PrepareContext(int index)
{
    if (index < 0) return false;
    ref loc = &Locations[index];
    if (IsEntity(loc) && CheckAttribute(loc, "PortContext.prepared")) return true;
    PortSave_ApplyPreparedContext(loc);
    // Preparation runs after OnLoad has released its flag. Keep the saved
    // population's deaths and encounter state while rebuilding its entities.
    bool previousLoadFlag = actLoadFlag;
    if (CheckAttribute(loc, "PortSave.population")) actLoadFlag = true;
    bool result;
    if (loc.id == "SanJuan_town")
    {
        loc.PortResident = "PuertoRico";
        result = LocPrepareResident(loc, stf(PortRoles.town.x), stf(PortRoles.town.y),
                                   stf(PortRoles.town.z), stf(PortRoles.town.yaw));
    }
    else result = LocPrepareResident(loc, 0.0, 0.0, 0.0, 0.0);
    actLoadFlag = previousLoadFlag;
    if (!result) return false;
    DeleteAttribute(loc, "PortSave.population");
    loc.PortContext.region = "PuertoRico";
    SendMessage(loc, "l", MSG_LOCATION_TLPCHRPOSITIONS);
    return true;
}

bool Port_PrepareRegion()
{
    if (!IsEntity(&AISea) || !CheckAttribute(&AISea, "Island") || AISea.Island != "PuertoRico") return false;
    if (!IsEntity(&PortRoles))
    {
        PortRoles.config = "RESOURCE\INI\sea_towns\puerto_rico.ini";
        if (!CreateEntity(&PortRoles, "PortEntityRoles")) return false;
        pchar.PortTravel.version = 1;
        pchar.PortTravel.region = "PuertoRico";
        pchar.PortTravel.resident = 1;
        if (!CheckAttribute(pchar, "PortTravel.mode")) pchar.PortTravel.mode = "ship";
        aref rig;
        makearef(rig, pchar.PortTravel.rig);
        PortTravel_InitRig(rig);
        Sea_ClearTownPopulation();
    }
    if (!Port_PrepareContext(FindLocation("SanJuan_town"))) return false;
    if (!Port_IsLandHero())
    {
        loadedLocation = &Locations[FindLocation("SanJuan_town")];
        SendMessage(loadedLocation, "lsl", MSG_LOCATION_EX_MSG, "SetActiveContext", true);
    }
    LayerFreeze(EXECUTE, false);
    LayerFreeze(REALIZE, false);
    return true;
}

void Port_SetSeaCamera(bool enabled)
{
    if (enabled)
    {
        LayerAddObject(SEA_EXECUTE, &SeaShipCamera, iShipPriorityExecute + 5);
        LayerAddObject(SEA_EXECUTE, &SeaFreeCamera, 1);
        LayerAddObject(SEA_EXECUTE, &SeaDeckCamera, iShipPriorityExecute + 5);
        LayerAddObject(SEA_REALIZE, &SeaDeckCamera, 65531);
    }
    else
    {
        LayerDelObject(SEA_EXECUTE, &SeaShipCamera);
        LayerDelObject(SEA_EXECUTE, &SeaFreeCamera);
        LayerDelObject(SEA_EXECUTE, &SeaDeckCamera);
        LayerDelObject(SEA_REALIZE, &SeaDeckCamera);
    }
    bCanSwitchCameras = enabled;
}

bool Port_BindShip()
{
    if (!Port_IsLandHero()) return true;
    // Log off while the canonical object still addresses its Character entity.
    LAi_CharacterLogoff(pchar);
    if (!SendMessage(&PortRoles, "lee", 0, &pchar, &PortRetiredHero)) return false;
    DeleteClass(&PortRetiredHero);
    pchar.location = "PuertoRico";
    pchar.location.group = "reload";
    pchar.location.locator = "reload_1";
    pchar.location.from_sea = "SanJuan_town";
    return true;
}

bool Port_CreateLandHero(int index, string group, string locator, bool usePosition)
{
    if (!Port_PrepareContext(index)) return false;
    ref loc = &Locations[index];
    SendMessage(loc, "lsl", MSG_LOCATION_EX_MSG, "SetActiveContext", true);
    loadedLocation = loc;
    pchar.location = loc.id;
    pchar.location.group = group;
    pchar.location.locator = locator;
    pchar.location.from_sea = "SanJuan_town";
    if (!CreateCharacter(pchar)) return false;
    if (!LAi_CharacterLogin(pchar, loc.id)) return false;
    if (!TeleportCharacterToLocator(pchar, group, locator)) return false;
    if (usePosition)
        TeleportCharacterToPosAy(pchar, stf(pchar.PortTravel.heroX), stf(pchar.PortTravel.heroY), stf(pchar.PortTravel.heroZ), stf(pchar.PortTravel.heroYaw));
    LAi_PostLoginInit(pchar);
    pchar.PortTravel.mode = "land";
    bSeaActive = false;
    Port_SetSeaCamera(false);
    DeleteBattleInterface();
    InitBattleLandInterface();
    if (!LocActivateResident(loc)) return false;
    LayerFreeze(EXECUTE, false);
    LayerFreeze(REALIZE, false);
    LayerFreeze(SEA_EXECUTE, false);
    LayerFreeze(SEA_REALIZE, false);
    return true;
}

bool Port_EnsureBoat()
{
    if (IsEntity(&PortBoat)) return true;
    if (!CreateEntity(&PortBoat, "PORT_ROWBOAT")) return false;
    aref descriptor;
    makearef(descriptor, pchar.PortTravel);
    pchar.PortTravel.enabled = 1;
    return SendMessage(&PortBoat, "laa", 0, pchar, descriptor);
}

bool Port_SelectPier(float x, float z)
{
    ref town = &Locations[FindLocation("SanJuan_town")];
    if (!CheckAttribute(town, "locators.reload.reload1_back")) return false;
    string selected = "reload1_back";
    if (CheckAttribute(town, "locators.reload.reload2_back"))
    {
        float a = GetDistance2D(x, z, stf(town.locators.reload.reload1_back.x), stf(town.locators.reload.reload1_back.z));
        float b = GetDistance2D(x, z, stf(town.locators.reload.reload2_back.x), stf(town.locators.reload.reload2_back.z));
        if (b < a) selected = "reload2_back";
    }
    aref pier;
    makearef(pier, town.locators.reload.(selected));
    pchar.PortTravel.pier = selected;
    pchar.PortTravel.targetX = pier.x;
    pchar.PortTravel.targetY = pier.y;
    pchar.PortTravel.targetZ = pier.z;
    pchar.PortTravel.targetYaw = pier.ay;
    return true;
}

bool Port_EnterBoat()
{
    if (!Port_IsResident() || DialogRun || sti(InterfaceStates.Launched) != 0) return false;
    if (!Port_EnsureBoat()) return false;
    bool land = Port_IsLandHero();
    if (land)
    {
        if (!SendMessage(&PortRoles, "le", 3, &pchar)) return false;
        if (pchar.location != "SanJuan_town") return false;
        float x, y, z, ay;
        if (!GetCharacterPos(pchar, &x, &y, &z)) return false;
        if (!Port_SelectPier(x, z)) return false;
        if (GetDistance2D(x, z, stf(pchar.PortTravel.targetX), stf(pchar.PortTravel.targetZ)) > 3.0) return false;
        GetCharacterAy(pchar, &ay);
        pchar.PortTravel.sourceX = x;
        pchar.PortTravel.sourceY = y;
        pchar.PortTravel.sourceZ = z;
        pchar.PortTravel.sourceYaw = ay;
        if (!CheckAttribute(pchar, "PortTravel.x"))
        {
            float dx = x - stf(PortRoles.town.x);
            float dz = z - stf(PortRoles.town.z);
            float length = sqrt(dx*dx + dz*dz);
            if (length < 0.1) return false;
            pchar.PortTravel.x = x + dx * 2.2 / length;
            pchar.PortTravel.y = y;
            pchar.PortTravel.z = z + dz * 2.2 / length;
            pchar.PortTravel.yaw = ay;
        }
        if (!SendMessage(&PortBoat, "li", 1, 0)) return false;
        LocSuspendResident(loadedLocation);
        if (!Port_BindShip()) { SendMessage(&PortBoat, "l", 3); return false; }
    }
    else
    {
        if (SeaCameras.Camera != "SeaDeckCamera") return false;
        if (!SendMessage(&PortBoat, "li", 1, &SeaDeckCamera)) return false;
    }
    pchar.PortTravel.mode = "boat";
    bSeaActive = true;
    Port_SetSeaCamera(false);
    BI_ShowInterface(false);
    procUpdateControlKeyGroup();
    return true;
}

void Port_TryLeaveBoat()
{
    if (!IsEntity(&PortBoat) || !CheckAttribute(pchar, "PortTravel.state") || sti(pchar.PortTravel.state) != 2) return;
    // Native admission independently validates range, relative speed and solids.
    if (stf(pchar.Ship.HP) > 0.0 && SendMessage(&PortBoat, "li", 2, &SeaDeckCamera)) return;
    if (!Port_SelectPier(stf(pchar.PortTravel.x), stf(pchar.PortTravel.z))) return;
    SendMessage(&PortBoat, "li", 2, 0);
}

void Port_CompleteHandoff()
{
    if (!CheckAttribute(pchar, "PortTravel.heroOwned") || sti(pchar.PortTravel.heroOwned) == 0) return;
    if (sti(pchar.PortTravel.handoffDeck) != 0)
    {
        pchar.PortTravel.mode = "deck";
        bSeaActive = true;
        SeaCameras.Camera = "SeaDeckCamera";
        SendMessage(&SeaCameras, "lia", AI_CAMERAS_SET_CAMERA, &SeaDeckCamera, pchar);
        Port_SetSeaCamera(true);
        InitBattleInterface();
        StartBattleInterface();
        RefreshBattleInterface();
    }
    else
    {
        if (!Port_CreateLandHero(FindLocation("SanJuan_town"), "reload", pchar.PortTravel.pier, true)) return;
    }
    SendMessage(&PortBoat, "l", 4);
    procUpdateControlKeyGroup();
}

bool Port_ChangeLandLocation(string id, string group, string locator)
{
    if (!Port_IsLandHero() || !Port_IsSupportedLocation(id)) return false;
    int destination = FindLocation(id);
    if (!Port_PrepareContext(destination)) return false;
    ref previous = loadedLocation;
    object previousAddress;
    aref address;
    makearef(address, pchar.location);
    CopyAttributes(&previousAddress, address);
    if (!SendMessage(&PortRoles, "lee", 1, &pchar, &PortRetiredHero)) return false;
    LocSuspendResident(previous);
    LAi_CharacterLogoff(pchar);
    SendMessage(previous, "lsl", MSG_LOCATION_EX_MSG, "SetContextVisible", false);
    if (!Port_CreateLandHero(destination, group, locator, false))
    {
        if (CheckAttribute(pchar, "chr_ai.login") && sti(pchar.chr_ai.login)) LAi_CharacterLogoff(pchar);
        if (SendMessage(&PortRoles, "lee", 2, &pchar, &PortRetiredHero))
        {
            DeleteClass(&PortRetiredHero);
            makearef(address, pchar.location);
            CopyAttributes(address, &previousAddress);
            LAi_CharacterLogin(pchar, previous.id);
            LAi_PostLoginInit(pchar);
            LocActivateResident(previous);
        }
        return false;
    }
    DeleteClass(&PortRetiredHero);
    return true;
}

bool Port_ReloadLandLocation(int destination, aref data)
{
    // Reuse stock common-house backlinks, authored destination options and
    // daily encounter setup before activating the already prepared context.
    object previousAddress;
    aref address;
    makearef(address, pchar.location);
    CopyAttributes(&previousAddress, address);
    PortAddressOnly = true;
    int configured = ReloadToLocation(destination, data);
    PortAddressOnly = false;
    string group = pchar.location.group;
    string locator = pchar.location.locator;
    makearef(address, pchar.location);
    CopyAttributes(address, &previousAddress);
    if (!configured) return false;
    return Port_ChangeLandLocation(Locations[destination].id, group, locator);
}

void Port_LeaveForOrdinaryLocation()
{
    object address;
    aref current;
    makearef(current, pchar.location);
    CopyAttributes(&address, current);
    Port_RetireRegion();
    if (IsEntity(&AISea)) DeleteSeaEnvironment();
    makearef(current, pchar.location);
    CopyAttributes(current, &address);
}

bool Port_RestoreAfterSea()
{
    if (!Port_IsResident() || !CheckAttribute(pchar, "PortTravel.restore.pending")) return false;
    string mode = pchar.PortTravel.mode;
    bool result = true;
    if (mode == "land")
    {
        string id = pchar.PortTravel.restore.location;
        if (!Port_IsSupportedLocation(id)) id = "SanJuan_town";
        string group = "reload";
        string locator = "reload1_back";
        if (CheckAttribute(pchar, "PortTravel.restore.location.group")) group = pchar.PortTravel.restore.location.group;
        if (CheckAttribute(pchar, "PortTravel.restore.location.locator")) locator = pchar.PortTravel.restore.location.locator;
        result = Port_CreateLandHero(FindLocation(id), group, locator, false);
        if (result && CheckAttribute(pchar, "PortTravel.restore.hero"))
        {
            aref from, to;
            makearef(from, pchar.PortTravel.restore.hero);
            makearef(to, pchar.saveposition);
            CopyAttributes(to, from);
            SendMessage(loadedLocation, "l", MSG_LOCATION_TLPCHRPOSITIONS);
        }
    }
    else if (mode == "boat")
    {
        result = Port_EnsureBoat();
        if (result)
        {
            if (CheckAttribute(pchar, "PortTravel.transferDeck") && sti(pchar.PortTravel.transferDeck))
                result = SendMessage(&PortBoat, "li", 5, &SeaDeckCamera);
            else result = SendMessage(&PortBoat, "li", 5, 0);
        }
        if (result)
        {
            bSeaActive = true;
            Port_SetSeaCamera(false);
            BI_ShowInterface(false);
        }
    }
    else
    {
        pchar.PortTravel.mode = mode;
        Port_SetSeaCamera(true);
        if (mode == "deck")
        {
            SeaCameras.Camera = "SeaDeckCamera";
            SendMessage(&SeaCameras, "lia", AI_CAMERAS_SET_CAMERA, &SeaDeckCamera, pchar);
        }
        if (CheckAttribute(pchar, "PortTravel.state") && sti(pchar.PortTravel.state) == 4 && Port_EnsureBoat())
        {
            if (CheckAttribute(pchar, "PortTravel.handoffDeck") && sti(pchar.PortTravel.handoffDeck))
                SendMessage(&PortBoat, "li", 5, &SeaDeckCamera);
            else SendMessage(&PortBoat, "li", 5, 0);
        }
    }
    if (!result)
    {
        Trace("Port restore: travel presentation unavailable, retaining naval state and recovering to pier");
        SendMessage(&PortRoles, "lee", 0, &pchar, &PortRetiredHero);
        DeleteClass(&PortRetiredHero);
        result = Port_CreateLandHero(FindLocation("SanJuan_town"), "reload", "reload1_back", false);
    }
    if (result)
    {
        DeleteAttribute(pchar, "PortTravel.restore");
        procUpdateControlKeyGroup();
    }
    return result;
}

void Port_RetireRegion()
{
    if (!IsEntity(&PortRoles)) return;
    if (Port_IsLandHero())
    {
        LocSuspendResident(loadedLocation);
        Port_BindShip();
    }
    PortRetiring = true;
    DeleteClass(&PortBoat);
    for (int i = 0; i < nLocationsNum; i++)
    {
        if (!CheckAttribute(&Locations[i], "PortContext.region")) continue;
        if (Locations[i].PortContext.region != "PuertoRico") continue;
        LocRetireResident(&Locations[i]);
        DeleteAttribute(&Locations[i], "PortSave.population");
    }
    if (LAi_IsInitedAI) LocAi_Release();
    DeleteClass(&PortRoles);
    DeleteAttribute(pchar, "PortTravel");
    PortPrepareCursor = 0;
    PortRetiring = false;
}

#event_handler("frame", "Port_Frame");
void Port_Frame()
{
    if (PortPreparing || PortRetiring || bSeaReloadStarted || !bSeaLoaded || !IsEntity(&AISea)) return;
    if (!CheckAttribute(&AISea, "Island") || AISea.Island != "PuertoRico") return;
    if (!Port_IsResident())
    {
        if (!Port_PrepareRegion()) return;
    }
    if (CheckAttribute(pchar, "PortTravel.restore.pending"))
    {
        Port_RestoreAfterSea();
        return;
    }
    if (IsEntity(&PortBoat) && CheckAttribute(pchar, "PortTravel.state") && sti(pchar.PortTravel.state) == 4)
        Port_CompleteHandoff();
    if (DialogRun || sti(InterfaceStates.Launched) != 0) return;
    // Prepare original service destinations before the player uses their doors.
    for (int i = PortPrepareCursor; i < nLocationsNum; i++)
    {
        PortPrepareCursor = i + 1;
        if (!Port_IsSupportedLocation(Locations[i].id) || IsEntity(&Locations[i])) continue;
        Port_PrepareContext(i);
        break;
    }
}

#event_handler("Control Activation", "Port_Control");
void Port_Control()
{
    string control = GetEventData();
    if (control != "PortTravelAction" || !Port_IsResident()) return;
    if (DialogRun || sti(InterfaceStates.Launched) != 0) return;
    if (pchar.PortTravel.mode == "boat") Port_TryLeaveBoat();
    else Port_EnterBoat();
}

void Port_RegisterControls()
{
    CI_CreateAndSetControls("PrimaryLand", "PortTravelAction", CI_GetKeyCode("KEY_E"), 0, true);
    MapControlToGroup("PortTravelAction", "Sailing1Pers");
    MapControlToGroup("PortTravelAction", "PortRowboat");
    MapControlToGroup("ChrForward", "PortRowboat");
    MapControlToGroup("ChrBackward", "PortRowboat");
    MapControlToGroup("ChrStrafeLeft", "PortRowboat");
    MapControlToGroup("ChrStrafeRight", "PortRowboat");
}

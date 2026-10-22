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
    if (IsEntity(&worldMap) || IsEntity(&SeaMapTransition) ||
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
        if (!IsEntity(captain) || !CheckAttribute(captain, "Ship.Pos.x")) continue;
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
        if (!IsEntity(captain)) continue;
        float ay = stf(captain.Ship.Ang.y);
        float sx = stf(worldMap.(path).speedX);
        float sz = stf(worldMap.(path).speedZ);
        // A one-shot native impulse restores motion without inventing a new
        // physics setter or keeping a permanent force on the ship.
        SendMessage(captain, "llffffff", MSG_SHIP_ADD_MOVE_IMPULSE, false,
            sx * cos(ay) + sz * sin(ay), 0.0, sz * cos(ay) - sx * sin(ay), 0.0, 0.0, 0.0);
        Ship_SetSailState(index, stf(worldMap.(path).sails));
    }
    if (CheckAttribute(&worldMap, "directSail.phonk"))
    {
        aref source, destination;
        makearef(source, worldMap.directSail.phonk);
        makearef(destination, Sea.Phonk);
        SeaPhonk_SetActive(true);
        CopyAttributes(destination, source);
        SeaPhonk_InitControls();
        SetEventHandler("frame", "SeaPhonk_Frame", 0);
        SeaPhonk_Music(SEA_PHONK_TRACK);
    }
    Sea_DirectSailCancel();
}

void Sea_DirectSailMapReady()
{
    if (!CheckAttribute(&worldMap, "directSail") || !IsEntity(&worldMap) ||
        !IsEntity(&SeaMapTransition) || !CheckAttribute(&SeaMapTransition, "seaOnly")) return;
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

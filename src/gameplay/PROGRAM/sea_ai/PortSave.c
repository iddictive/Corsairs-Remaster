// Port state extends ordinary attributes; the existing naval binary is unchanged.
bool portSaveLegacyQueued = false;

void PortSave_CopyHeroAddress(bool legacy)
{
    DeleteAttribute(pchar, "PortTravel.restore");
    pchar.PortTravel.restore.pending = 1;
    pchar.PortTravel.restore.legacy = legacy;
    pchar.PortTravel.restore.localCoordinates = legacy;
    aref source, destination;
    makearef(source, pchar.location);
    makearef(destination, pchar.PortTravel.restore.location);
    CopyAttributes(destination, source);
    if (CheckAttribute(pchar, "saveposition"))
    {
        makearef(source, pchar.saveposition);
        makearef(destination, pchar.PortTravel.restore.hero);
        CopyAttributes(destination, source);
    }
}

void Port_BeforeSave()
{
    if (!Port_IsResident()) return;
    pchar.PortTravel.version = 1;
    pchar.PortTravel.region = "PuertoRico";
    for (int i = 0; i < nLocationsNum; i++)
    {
        ref loc = &Locations[i];
        if (!IsEntity(loc) || !CheckAttribute(loc, "PortContext.prepared")) continue;
        if (!CheckAttribute(loc, "PortContext.region") || loc.PortContext.region != "PuertoRico") continue;
        SendMessage(loc, "l", MSG_LOCATION_SETCHRPOSITIONS);
        loc.PortSave.population = 1;
        PostEvent("SeaDogs_ClearSaveData", 400, "a", loc);
    }
    LAi_SaveInfo();
}

// Called before ordinary scene dispatch. A land-only legacy save must never
// enter native SEALOAD: it does not contain the CSaveLoad stream.
bool Port_LoadSaved()
{
    portSaveLegacyQueued = false;
    if (!CheckAttribute(pchar, "PortTravel.resident") || !sti(pchar.PortTravel.resident))
    {
        if (seadogs_saveFrom == "location" && Port_IsSupportedLocation(pchar.location))
            portSaveLegacyQueued = true;
        return false;
    }
    if (!CheckAttribute(pchar, "PortTravel.region") || pchar.PortTravel.region != "PuertoRico") return false;
    PortSave_CopyHeroAddress(false);
    if (CheckAttribute(pchar, "PortTravel.version") && sti(pchar.PortTravel.version) == 1 &&
        seadogs_saveFrom == "sea" && CheckAttribute(&oSeaSave, "save") && strlen(oSeaSave.save) > 8)
    {
        Sea_Load();
        return true;
    }
    // Keep inventory, quests, health and Ship attributes; recover to a visible,
    // ordinary pier rather than fabricating a missing fleet or invisible hero.
    pchar.PortTravel.error = "saved_naval_state_unavailable";
    pchar.PortTravel.resident = 0;
    pchar.PortTravel.mode = "land";
    DeleteAttribute(pchar, "PortTravel.restore");
    Trace("Port restore: naval state unavailable; recovered to San Juan pier");
    LoadMainCharacterInFirstLocation("SanJuan_town", "reload1_back", "SanJuan_town");
    return true;
}

void Port_AfterLoad()
{
    if (!portSaveLegacyQueued) return;
    portSaveLegacyQueued = false;
    SetEventHandler("PortSave_MigrateLegacy", "PortSave_MigrateLegacy", 0);
    PostEvent("PortSave_MigrateLegacy", 1);
}

void PortSave_MigrateLegacy()
{
    DelEventHandler("PortSave_MigrateLegacy", "PortSave_MigrateLegacy");
    if (IsEntity(&AISea) || !Port_IsSupportedLocation(pchar.location)) return;
    int current = FindLoadedLocation();
    int town = FindLocation("SanJuan_town");
    int island = FindIsland("PuertoRico");
    if (current < 0 || town < 0 || island < 0) return;
    if (!CheckAttribute(pchar, "Ship.Type") || GetCharacterShipType(pchar) == SHIP_NOTUSED)
    {
        pchar.PortTravel.error = "legacy_ship_unavailable";
        Trace("Port migration: no existing player ship; retained ordinary land scene");
        return;
    }
    // Resolve the authored coast exit before retiring anything.
    aref exits, exit;
    makearef(exits, Locations[town].reload);
    bool found = false;
    for (int i = 0; i < GetAttributesNum(exits); i++)
    {
        exit = GetAttributeN(exits, i);
        if (exit.go != "PuertoRico") continue;
        string emerge = FindEmergeLocator(&Islands[island], exit.emerge);
        if (emerge == "" || !CheckAttribute(&Islands[island], "reload." + emerge + ".ships.l0.x")) continue;
        found = true;
        break;
    }
    if (!found)
    {
        pchar.PortTravel.error = "legacy_berth_unavailable";
        Trace("Port migration: authored berth unavailable; retained ordinary land scene");
        return;
    }
    ref loc = &Locations[current];
    SendMessage(loc, "l", MSG_LOCATION_SETCHRPOSITIONS);
    LAi_SaveInfo();
    PortSave_CopyHeroAddress(true);
    loc.PortSave.population = 1;
    pchar.PortTravel.version = 1;
    pchar.PortTravel.region = "PuertoRico";
    pchar.PortTravel.mode = "land";
    // Ordinary UnloadLocation clears canonical location fantoms. Retire only
    // entities and the old land environment, retaining their saved attributes.
    LocSuspendResident(loc);
    LocRetireResident(loc);
    DeleteCoastFoamEnvironment();
    DeleteParticles();
    DeleteAnimals();
    DeleteSea();
    DeleteWeather();
    DeleteShipEnvironment();
    Ship_Walk_Delete();
    for (i = 0; i < locNumShips; i++) DeleteClass(&locShips[i]);
    locNumShips = 0;
    pchar.PortTravel.resident = 1;
    ReloadToSea(island, exit);
}

void PortSave_TransformPosition(aref position)
{
    float x = stf(position.x);
    float z = stf(position.z);
    float yaw = stf(PortRoles.town.yaw);
    position.x = stf(PortRoles.town.x) + x * cos(yaw) + z * sin(yaw);
    position.y = stf(PortRoles.town.y) + stf(position.y);
    position.z = stf(PortRoles.town.z) + z * cos(yaw) - x * sin(yaw);
    position.ay = stf(position.ay) + yaw;
}

// Called after PortRoles has read canonical layout, before native actor login.
void PortSave_ApplyPreparedContext(ref loc)
{
    if (CheckAttribute(loc, "PortSave.population")) loc.PortContext.population = 1;
    if (loc.id != "SanJuan_town" || !CheckAttribute(pchar, "PortTravel.restore.localCoordinates") ||
        !sti(pchar.PortTravel.restore.localCoordinates)) return;
    for (int i = 0; i < TOTAL_CHARACTERS; i++)
    {
        ref chr = &Characters[i];
        if (!CheckAttribute(chr, "location") || chr.location != loc.id || !CheckAttribute(chr, "saveposition")) continue;
        aref position;
        makearef(position, chr.saveposition);
        PortSave_TransformPosition(position);
    }
    if (CheckAttribute(pchar, "PortTravel.restore.hero") && pchar.PortTravel.restore.location == loc.id)
    {
        aref hero;
        makearef(hero, pchar.PortTravel.restore.hero);
        PortSave_TransformPosition(hero);
    }
    pchar.PortTravel.restore.localCoordinates = 0;
}

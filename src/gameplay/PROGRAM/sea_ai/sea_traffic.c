// One descriptor owns each ordinary fleet; native map navigation is its distant
// projection, and a mounted sea group is its exclusive physical projection.
object SeaTraffic;
object SeaTrafficFreeSlots;

// Existing nuclear relevance reaches 6000 sea units: retain 1km of loading/
// approach margin and 2km hysteresis. Actor/frame cost and visibility remain runtime gates.
#define SEA_TRAFFIC_ADMIT_DISTANCE 7000.0
#define SEA_TRAFFIC_RELEASE_DISTANCE 9000.0

#event_handler("SeaTraffic_WorldState", "SeaTraffic_WorldState");
#event_handler("SeaTraffic_SceneState", "SeaTraffic_SceneState");
#event_handler("SeaTraffic_ReleaseSnapshot", "SeaTraffic_ReleaseSnapshot");
#event_handler("SeaTraffic_Released", "SeaTraffic_Released");
#event_handler("SeaTraffic_ReleaseRejected", "SeaTraffic_ReleaseRejected");
#event_handler("SeaTraffic_Residency", "SeaTraffic_Residency");
#event_handler("SeaTraffic_CanRelease", "SeaTraffic_CanReleaseEvent");

bool SeaTraffic_CanReleaseEvent() { return SeaTraffic_CanRelease(GetEventData()); }

ref SeaTraffic_WorldState() { return &worldMap; }

ref SeaTraffic_Native()
{
    if (IsEntity(&worldMap)) return &worldMap;
    return &SeaTraffic;
}

bool SeaTraffic_IsActive() { return IsEntity(&worldMap) || IsEntity(&SeaTraffic); }

void SeaTraffic_Notify(string path)
{
    if (IsEntity(&SeaTraffic)) SendMessage(&SeaTraffic, "ls", MSG_WORLDMAP_NOTIFY, path);
}

void SeaTraffic_Start()
{
    if (IsEntity(&SeaTraffic) || IsEntity(&worldMap) || !bSeaActive) return;
    if (CheckAttribute(&worldMap, "encounters"))
    {
        aref descriptors; makearef(descriptors, worldMap.encounters);
        for (int i = 0; i < GetAttributesNum(descriptors); i++)
        {
            aref descriptor = GetAttributeN(descriptors, i);
            DeleteAttribute(descriptor, "trafficReleasePending");
        }
    }
    SeaTraffic.execute = sCurrentSeaExecute;
    wdmCreateWindInterface();
    CreateEntity(&SeaTraffic, "WorldMapSeaTraffic");
    if (IsEntity(&SeaTraffic)) WdmTrafficRefresh();
    for (int s = 0; s < iNumShips; s++)
    {
        ref captain = &Characters[Ships[s]];
        if (!IsEntity(captain) || !WdmFleetSeaActorOwned(captain, false)) continue;
        string entry = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
        worldMap.(entry).seaLoaded = 1;
    }
    SeaTraffic_RegisterHarbours();
}

void SeaTraffic_Stop()
{
    if (bSeaPreserveMapHost) return;
    if (IsEntity(&SeaTraffic))
    {
        SendMessage(&SeaTraffic, "ls", MSG_WORLDMAP_NOTIFY, "$shutdown");
        DeleteClass(&SeaTraffic);
    }
    DeleteAttribute(&SeaTrafficFreeSlots, "");
}

void SeaTraffic_ResumePoses()
{
    if (!CheckAttribute(&worldMap, "encounters")) return;
    aref descriptors; makearef(descriptors, worldMap.encounters);
    for (int d = 0; d < GetAttributesNum(descriptors); d++)
    {
        aref descriptor = GetAttributeN(descriptors, d);
        if (!CheckAttribute(descriptor, "trafficInSea") || !sti(descriptor.trafficInSea)) continue;
        string groupID = "egroup__wdm_" + GetAttributeName(descriptor);
        if (CheckAttribute(descriptor, "trafficActorGroup")) groupID = descriptor.trafficActorGroup;
        SendMessage(&AISea, "lsfff", AI_MESSAGE_RELOCATE_GROUP, groupID,
            Sea_LocalX(stf(descriptor.x), stf(descriptor.z)),
            Sea_LocalZ(stf(descriptor.x), stf(descriptor.z)), Sea_LocalHeading(stf(descriptor.ay)));
    }
}

ref SeaTraffic_SceneState()
{
    AISea.traffic.enabled = bSeaActive && bSeaLoaded && !bSeaReloadStarted && !bAbordageStarted;
    if (!sti(AISea.traffic.enabled)) return &AISea;
    AISea.traffic.x = Sea_MapX(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z));
    AISea.traffic.z = Sea_MapZ(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z));
    AISea.traffic.ay = Sea_MapHeading(stf(pchar.Ship.Ang.y));
    AISea.traffic.time = GetTime();
    AISea.traffic.sec = Environment.date.sec;
    AISea.traffic.min = Environment.date.min;
    AISea.traffic.hour = Environment.date.hour;
    AISea.traffic.day = Environment.date.day;
    AISea.traffic.month = Environment.date.month;
    AISea.traffic.year = Environment.date.year;
    return &AISea;
}

void SeaTraffic_Residency()
{
    if (!bSeaActive || !bSeaLoaded || bSeaReloadStarted || bAbordageStarted || sti(InterfaceStates.Launched)) return;
    SeaTraffic_LoginRegionGroups(pchar.location);
    SeaTraffic_ReportPositions();
    float playerX = stf(pchar.Ship.Pos.x);
    float playerZ = stf(pchar.Ship.Pos.z);
    aref descriptors; makearef(descriptors, worldMap.encounters);
    for (int d = 0; d < GetAttributesNum(descriptors); d++)
    {
        aref descriptor = GetAttributeN(descriptors, d);
        if (!WdmTrafficIsOrdinary(descriptor) || CheckAttribute(descriptor, "needDelete") ||
            CheckAttribute(descriptor, "trafficReleasePending")) continue;
        string identity = GetAttributeName(descriptor);
        string groupID = "egroup__wdm_" + identity;
        if (CheckAttribute(descriptor, "trafficActorGroup")) groupID = descriptor.trafficActorGroup;
        if (CheckAttribute(descriptor, "trafficRollback"))
        {
            descriptor.trafficReleasePending = 1;
            SendMessage(&AISea, "ls", AI_MESSAGE_RELEASE_GROUP, groupID);
            continue;
        }
        float x = Sea_LocalX(stf(descriptor.x), stf(descriptor.z));
        float z = Sea_LocalZ(stf(descriptor.x), stf(descriptor.z));
        float dx = x - playerX;
        float dz = z - playerZ;
        bool mounted = CheckAttribute(descriptor, "trafficInSea") && sti(descriptor.trafficInSea);
        // Distinct boundaries prevent churn; actor/battle borrowers outrank distance.
        if (mounted)
        {
            if (dx * dx + dz * dz > SEA_TRAFFIC_RELEASE_DISTANCE * SEA_TRAFFIC_RELEASE_DISTANCE && SeaTraffic_CanRelease(groupID))
            {
                descriptor.trafficReleasePending = 1;
                SendMessage(&AISea, "ls", AI_MESSAGE_RELEASE_GROUP, groupID);
            }
            continue;
        }
        if (dx * dx + dz * dz <= SEA_TRAFFIC_ADMIT_DISTANCE * SEA_TRAFFIC_ADMIT_DISTANCE) SeaTraffic_Admit(descriptor);
    }
    SeaTraffic_Notify("deleteUpdate");
}

void SeaTraffic_ReportPositions()
{
    for (int g = 0; g < MAX_SHIP_GROUPS; g++)
    {
        ref group = &AIGroups[g];
        if (!CheckAttribute(group, "trafficFleetID")) continue;
        string path = "encounters." + group.trafficFleetID;
        if (!CheckAttribute(&worldMap, path)) continue;
        for (int slot = 0; slot < MAX_CHARACTERS; slot++)
        {
            int index = Group_GetCharacterIndexR(group, slot);
            if (index < 0) break;
            ref captain = &Characters[index];
            if (!IsEntity(captain) || !WdmFleetSeaAlive(captain) || !WdmFleetSeaActorOwned(captain, true)) continue;
            if (captain.trafficFleetID != group.trafficFleetID) continue;
            WdmFleetSeaPublishGroup(group);
            worldMap.(path).x = Sea_MapX(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z));
            worldMap.(path).z = Sea_MapZ(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z));
            worldMap.(path).ay = Sea_MapHeading(stf(captain.Ship.Ang.y));
            break;
        }
    }
}

void SeaTraffic_SaveMotion(ref captain, aref descriptor, aref entry)
{
    float ay = Sea_LocalHeading(stf(descriptor.ay));
    float dx = stf(captain.Ship.Pos.x) - Sea_LocalX(stf(descriptor.x), stf(descriptor.z));
    float dz = stf(captain.Ship.Pos.z) - Sea_LocalZ(stf(descriptor.x), stf(descriptor.z));
    entry.motion.across = dx * cos(ay) - dz * sin(ay);
    entry.motion.forward = dx * sin(ay) + dz * cos(ay);
    entry.motion.heading = stf(captain.Ship.Ang.y) - ay;
    entry.motion.speedX = captain.Ship.Speed.x;
    entry.motion.speedZ = captain.Ship.Speed.z;
    entry.motion.sails = Ship_GetSailState(captain);
}

bool SeaTraffic_RestorePosition(aref captain, float x, float z, float ay)
{
    if (!CheckAttribute(captain, "trafficFleetID") || !CheckAttribute(captain, "trafficRosterSlot")) return false;
    string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot + ".motion";
    if (!CheckAttribute(&worldMap, path)) return false;
    float across = stf(worldMap.(path).across);
    float forward = stf(worldMap.(path).forward);
    WdmFleetSeaShipPositionResult[0] = x + across * cos(ay) + forward * sin(ay);
    WdmFleetSeaShipPositionResult[1] = ay + stf(worldMap.(path).heading);
    WdmFleetSeaShipPositionResult[2] = z - across * sin(ay) + forward * cos(ay);
    return true;
}

void SeaTraffic_RestoreMotion(ref captain)
{
    string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot + ".motion";
    if (!CheckAttribute(&worldMap, path)) return;
    float ay = stf(captain.Ship.Ang.y);
    float sx = stf(worldMap.(path).speedX);
    float sz = stf(worldMap.(path).speedZ);
    SendMessage(captain, "llffffff", MSG_SHIP_ADD_MOVE_IMPULSE, false,
        sx * cos(ay) + sz * sin(ay), 0.0, sz * cos(ay) - sx * sin(ay), 0.0, 0.0, 0.0);
    Ship_SetSailState(sti(captain.index), stf(worldMap.(path).sails));
}

bool SeaTraffic_HarbourPresent(string colony)
{
    if (!CheckAttribute(&worldMap, "encounters")) return false;
    aref descriptors; makearef(descriptors, worldMap.encounters);
    for (int d = 0; d < GetAttributesNum(descriptors); d++)
    {
        aref descriptor = GetAttributeN(descriptors, d);
        if (!CheckAttribute(descriptor, "trafficHarbour") || descriptor.trafficHarbour != colony ||
            CheckAttribute(descriptor, "needDelete")) continue;
        if (CheckAttribute(descriptor, "encdata.trafficRoster.ship0.dead") && sti(descriptor.encdata.trafficRoster.ship0.dead)) continue;
        return true;
    }
    return false;
}

void SeaTraffic_ClearHarbour(string colony)
{
    if (!CheckAttribute(&worldMap, "encounters")) return;
    aref descriptors; makearef(descriptors, worldMap.encounters);
    for (int d = 0; d < GetAttributesNum(descriptors); d++)
    {
        aref descriptor = GetAttributeN(descriptors, d);
        if (CheckAttribute(descriptor, "trafficHarbour") && descriptor.trafficHarbour == colony)
            descriptor.needDelete = "Harbour defence cleared";
    }
    worldMap.deleteUpdate = "";
    SeaTraffic_Notify("deleteUpdate");
}

void SeaTraffic_PublishRegion(string island)
{
    if (island == pchar.location || !Sea_IsResidentIslandReady(island)) return;
    pchar.location = island;
    AISea.Island = island;
    worldMap.island = island;
    sIslandID = island;
    for (int c = 1; c < COMPANION_MAX; c++)
    {
        int index = GetCompanionIndex(pchar, c);
        if (index >= 0 && IsEntity(&Characters[index])) Characters[index].location = island;
    }
    bCanEnterToLand = false;
    sIslandLocator = "";
    Sea_SelectResidentIsland(island);
    BI_SetIslandData();
    // Normal sea entry checks location quest predicates. Its other listener,
    // LAi_SetRestoreStates, belongs to actor/scene reload and must not run here.
    QuestsCheck();
    if (!bSeaActive || bSeaReloadStarted) return;
    GenerateIslandShips(island);
    SeaTraffic_LoginRegionGroups(island);
    SeaTraffic_RegisterHarbours();
    PostEvent(SHIP_CHECK_RELOAD_ENABLE, 1);
}

void SeaTraffic_LoginRegionGroups(string island)
{
    // Authored quest groups retain priority over generated harbour actors.
    for (int pass = 0; pass < 2; pass++)
    {
        for (int g = 0; g < MAX_SHIP_GROUPS; g++)
        {
            ref group = &AIGroups[g];
            if (!CheckAttribute(group, "id") || !CheckAttribute(group, "location") || group.location != island ||
                CheckAttribute(group, "AlreadyLoaded")) continue;
            int index = Group_GetGroupCommanderIndexR(group);
            if (index < 0) continue;
            bool harbour = CheckAttribute(&Characters[index], "IslandShips");
            if ((pass == 0 && harbour) || (pass == 1 && !harbour)) continue;
            int required = 0;
            for (int slot = 0; slot < MAX_CHARACTERS; slot++)
            {
                int member = Group_GetCharacterIndexR(group, slot);
                if (member < 0) break;
                if (!LAi_IsDead(&Characters[member]) && !IsEntity(&Characters[member])) required++;
            }
            if (iNumShips + required > MAX_SHIPS_ON_SEA) continue;
            Sea_LoginGroup(group.id);
            WdmFleetSeaApplyTask(group.id);
        }
    }
    SeaTraffic_RegisterHarbours();
}

void SeaTraffic_RegisterHarbours()
{
    for (int s = 0; s < iNumShips; s++)
    {
        ref captain = &Characters[Ships[s]];
        if (!IsEntity(captain) || !CheckAttribute(captain, "IslandShips") || CheckAttribute(captain, "trafficFleetID") ||
            CheckAttribute(captain, "isquest") || CheckAttribute(captain, "qID") || IsCompanion(captain)) continue;
        string groupID = Ship_GetGroupID(captain);
        int g = Group_FindGroup(groupID);
        if (g < 0 || Group_GetCharacterIndexR(&AIGroups[g], 1) >= 0) continue;
        ref group = &AIGroups[g];
        int realIndex = GetCharacterShipType(captain);
        if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) continue;
        if (!CheckAttribute(&worldMap, "trafficHarbourSerial")) worldMap.trafficHarbourSerial = 0;
        worldMap.trafficHarbourSerial = sti(worldMap.trafficHarbourSerial) + 1;
        string id = "harbour" + worldMap.trafficHarbourSerial;
        aref descriptor; makearef(descriptor, worldMap.encounters.(id));
        descriptor.type = "Merchant"; descriptor.modelName = "ship";
        descriptor.x = Sea_MapX(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z)); descriptor.z = Sea_MapZ(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z));
        descriptor.ay = Sea_MapHeading(stf(captain.Ship.Ang.y)); descriptor.gotoX = descriptor.x; descriptor.gotoZ = descriptor.z;
        descriptor.kMaxSpeed = 1.0; descriptor.liveTime = 1000000000.0;
        descriptor.trafficFleetID = id; descriptor.trafficActorGroup = groupID;
        descriptor.trafficHarbour = captain.IslandShips;
        descriptor.trafficRole = 2; descriptor.trafficNation = captain.nation;
        descriptor.trafficVersion = 1; descriptor.trafficLifecycle = "harbour";
        if (CheckAttribute(group, "Task") && (sti(group.Task) == AITASK_DRIFT || sti(group.Task) == AITASK_NONE))
            descriptor.trafficLifecycle = "service";
        descriptor.trafficHomePort = captain.IslandShips; descriptor.trafficCurrentPort = captain.IslandShips;
        descriptor.trafficHomeX = descriptor.x; descriptor.trafficHomeZ = descriptor.z;
        descriptor.trafficHomeRadius = 180.0;
        descriptor.trafficIntent = "service"; descriptor.trafficCondition = 1.0; descriptor.trafficRisk = 1.10;
        descriptor.trafficTargetPlayer = 0;
        descriptor.trafficInSea = 1;
        descriptor.encdata.Nation = captain.nation; descriptor.encdata.Type = captain.Ship.Mode;
        descriptor.encdata.RealEncounterType = ENCOUNTER_TYPE_PATROL_SMALL;
        descriptor.encdata.NumWarShips = 1; descriptor.encdata.NumMerchantShips = 0;
        descriptor.encdata.trafficRoster.count = 1;
        descriptor.encdata.trafficRoster.ship0.baseType = RealShips[realIndex].BaseType;
        descriptor.encdata.trafficRoster.ship0.mode = captain.Ship.Mode;
        descriptor.encdata.trafficRoster.ship0.dead = 0; descriptor.encdata.trafficRoster.ship0.seaLoaded = 1;
        WdmFleetSeaCopyIdentity(captain, descriptor); captain.trafficRosterSlot = 0;
        WdmFleetSeaCopyIdentity(group, descriptor);
        descriptor.trafficReleasePending = 1;
        WdmFleetSeaSaveFleet(id);
        DeleteAttribute(descriptor, "trafficReleasePending");
        descriptor.encdata.trafficRoster.ship0.seaLoaded = 1;
    }
}

int SeaTraffic_AllocateCaptain(ref fleet, int ordinal)
{
    int characterIndex = -1;
    string slotKey = "";
    for (int i = 0; i < GetAttributesNum(&SeaTrafficFreeSlots); i++)
    {
        aref entry = GetAttributeN(&SeaTrafficFreeSlots, i);
        if (!CheckAttribute(entry, "index") || !CheckAttribute(entry, "character")) continue;
        int candidate = sti(entry.index);
        if (candidate < FANTOM_CHARACTERS || candidate >= TOTAL_CHARACTERS || IsEntity(&Characters[candidate])) continue;
        aref cleared; makearef(cleared, entry.character);
        if (!SeaTraffic_SameAttributes(cleared, &Characters[candidate])) continue;
        characterIndex = candidate;
        slotKey = GetAttributeName(entry);
        break;
    }
    bool append = characterIndex < 0;
    if (append) characterIndex = FANTOM_CHARACTERS + iNumFantoms;
    string key = "ship" + ordinal;
    aref hull; makearef(hull, fleet.trafficRoster.(key));
    if (Fantom_CreateShipFromBaseAtIndex(sti(hull.baseType), fleet.GroupName, hull.mode,
        sti(fleet.RealEncounterType), sti(fleet.Nation), characterIndex) < 0) return -1;
    if (append) iNumFantoms++;
    else DeleteAttribute(&SeaTrafficFreeSlots, slotKey);
    ref captain = &Characters[characterIndex];
    WdmFleetSeaCopyIdentity(captain, fleet);
    captain.trafficRosterSlot = ordinal;
    captain.GroupShipPos_event = "WdmFleetSeaShipPosition";
    return characterIndex;
}

bool SeaTraffic_SameAttributes(aref left, aref right)
{
    if (GetAttributeValue(left) != GetAttributeValue(right) || GetAttributesNum(left) != GetAttributesNum(right)) return false;
    for (int i = 0; i < GetAttributesNum(left); i++)
    {
        aref child = GetAttributeN(left, i);
        string key = GetAttributeName(child);
        if (!CheckAttribute(right, key)) return false;
        aref other; makearef(other, right.(key));
        if (!SeaTraffic_SameAttributes(child, other)) return false;
    }
    return true;
}

bool SeaTraffic_Mount(ref fleet, float x, float z, float ay)
{
    string groupID = fleet.GroupName;
    if (Group_FindGroup(groupID) >= 0) return false;
    ref group = Group_FindOrCreateGroup(groupID);
    Group_SetXZ_AY(groupID, x, z, ay);
    Group_SetType(groupID, fleet.Type);
    WdmFleetSeaCopyIdentity(group, fleet);
    if (CheckAttribute(fleet, "Task"))
    {
        aref from, to;
        makearef(from, fleet.Task); makearef(to, group.Task);
        CopyAttributes(to, from);
        if (!CheckAttribute(group, "Task.Lock")) group.Task.Lock = false;
    }
    int commander = -1;
    int mounted = 0;
    for (int ordinal = 0; ordinal < sti(fleet.trafficRoster.count); ordinal++)
    {
        string key = "ship" + ordinal;
        if (!CheckAttribute(fleet, "trafficRoster." + key + ".baseType") || sti(fleet.trafficRoster.(key).dead)) continue;
        int index = SeaTraffic_AllocateCaptain(fleet, ordinal);
        if (index < 0) break;
        if (commander < 0) commander = index;
        Sea_MountEncounterCharacter(fleet, index, mounted, commander, groupID, pchar.location, false);
        if (!IsEntity(&Characters[index])) break;
        mounted++;
    }
    string descriptor = "encounters." + fleet.trafficFleetID;
    if (mounted != WdmFleetSeaHullCount(fleet))
    {
        return false;
    }
    return true;
}

void SeaTraffic_Admit(aref descriptor)
{
    if (SeaTraffic_HasOriginalGroup(descriptor))
    {
        Sea_LoginGroup(descriptor.trafficActorGroup);
        SeaTraffic_RegisterHarbours();
        return;
    }
    object bundle;
    int hulls = 0;
    string root = "";
    if (CheckAttribute(descriptor, "trafficBattleRoot")) root = descriptor.trafficBattleRoot;
    aref descriptors, candidate, source, fleet;
    string id;
    makearef(descriptors, worldMap.encounters);
    for (int d = 0; d < GetAttributesNum(descriptors); d++)
    {
        candidate = GetAttributeN(descriptors, d);
        if (GetAttributeName(candidate) != GetAttributeName(descriptor) &&
            (root == "" || !CheckAttribute(candidate, "trafficBattleRoot") || candidate.trafficBattleRoot != root)) continue;
        if (CheckAttribute(candidate, "trafficInSea") && sti(candidate.trafficInSea)) continue;
        if (!WdmTrafficIsOrdinary(candidate) || !CheckAttribute(candidate, "encdata.trafficRoster.count")) return;
        id = GetAttributeName(candidate);
        makearef(source, candidate.encdata); makearef(fleet, bundle.(id));
        CopyAttributes(fleet, source); WdmFleetSeaCopyIdentity(fleet, candidate);
        fleet.GroupName = "egroup__wdm_" + id;
        int existing = Group_FindGroup(fleet.GroupName);
        if (existing >= 0)
        {
            ref existingGroup = &AIGroups[existing];
            if (!CheckAttribute(existingGroup, "trafficFleetID") || existingGroup.trafficFleetID != fleet.trafficFleetID) return;
            for (int slot = 0; slot < MAX_CHARACTERS; slot++)
            {
                int index = Group_GetCharacterIndexR(existingGroup, slot);
                if (index < 0) break;
                // Saved group members may already have been recycled into a
                // different fleet. Only this fleet's native hull owns residency.
                if (IsEntity(&Characters[index]) && CheckAttribute(&Characters[index], "trafficFleetID") &&
                    Characters[index].trafficFleetID == fleet.trafficFleetID) return;
            }
            fleet.trafficStaleGroup = 1;
        }
        if (!WdmFleetSeaAssemblyReady(fleet)) return;
        hulls = hulls + WdmFleetSeaHullCount(fleet);
    }
    if (hulls == 0 || iNumShips + hulls > MAX_SHIPS_ON_SEA) return;
    // Reserve the whole battle before the first hull is mounted.
    int available = TOTAL_CHARACTERS - FANTOM_CHARACTERS - iNumFantoms + GetAttributesNum(&SeaTrafficFreeSlots);
    if (hulls > available || hulls > REAL_SHIPS_QUANTITY - GetRealShipsCount()) return;
    for (int i = 0; i < GetAttributesNum(&bundle); i++)
    {
        fleet = GetAttributeN(&bundle, i);
        id = GetAttributeName(fleet);
        makearef(candidate, worldMap.encounters.(id));
        // Drop only the stale script projection, after the complete battle
        // passes capacity/resource admission. The saved roster owns its hulls.
        if (CheckAttribute(fleet, "trafficStaleGroup")) Group_DeleteGroup(fleet.GroupName);
        if (!WdmFleetSeaImport(fleet, candidate, id)) { SeaTraffic_Rollback(&bundle); return; }
        fleet.trafficAdmissionLease = 1;
        worldMap.retireEncounter = id; SeaTraffic_Notify("retireEncounter");
        if (!SeaTraffic_Mount(fleet, Sea_LocalX(stf(candidate.x), stf(candidate.z)), Sea_LocalZ(stf(candidate.x), stf(candidate.z)), Sea_LocalHeading(stf(candidate.ay))))
        { SeaTraffic_Rollback(&bundle); return; }
    }
    for (i = 0; i < GetAttributesNum(&bundle); i++)
    {
        fleet = GetAttributeN(&bundle, i);
        id = GetAttributeName(fleet);
        makearef(candidate, worldMap.encounters.(id));
        ref group = Group_GetGroupByID(fleet.GroupName);
        group.Task = AITASK_MOVE;
        group.Task.Pos.x = Sea_LocalX(stf(candidate.gotoX), stf(candidate.gotoZ));
        group.Task.Pos.z = Sea_LocalZ(stf(candidate.gotoX), stf(candidate.gotoZ));
        if (group.trafficIntent == "service") group.Task = AITASK_DRIFT;
        string target = "";
        if (CheckAttribute(candidate, "trafficTargetID") && candidate.trafficTargetID != "") target = "egroup__wdm_" + candidate.trafficTargetID;
        if (CheckAttribute(candidate, "trafficBattle") && candidate.trafficBattle != "") target = "egroup__wdm_" + candidate.trafficBattle;
        if (CheckAttribute(candidate, "trafficTargetPlayer") && sti(candidate.trafficTargetPlayer)) target = PLAYER_GROUP;
        if (target != "" && Group_FindGroup(target) >= 0)
        {
            group.Task = AITASK_ATTACK; group.Task.Target = target;
            if (group.trafficIntent == "escape") group.Task = AITASK_RUNAWAY;
        }
        WdmFleetSeaApplyTask(fleet.GroupName);
    }
}

void SeaTraffic_Rollback(ref bundle)
{
    for (int i = 0; i < GetAttributesNum(bundle); i++)
    {
        aref fleet = GetAttributeN(bundle, i);
        if (!CheckAttribute(fleet, "trafficAdmissionLease")) continue;
        string id = GetAttributeName(fleet);
        if (Group_FindGroup(fleet.GroupName) >= 0)
        {
            worldMap.encounters.(id).trafficRollback = 1;
            worldMap.encounters.(id).trafficReleasePending = 1;
            SendMessage(&AISea, "ls", AI_MESSAGE_RELEASE_GROUP, fleet.GroupName);
        }
        else
        {
            DeleteAttribute(&worldMap, "encounters." + id + ".trafficInSea");
            worldMap.restoreEncounter = id; SeaTraffic_Notify("restoreEncounter");
        }
    }
}

bool SeaTraffic_HasOriginalGroup(aref descriptor)
{
    if (!CheckAttribute(descriptor, "trafficActorGroup")) return false;
    int g = Group_FindGroup(descriptor.trafficActorGroup);
    if (g < 0) return false;
    int index = Group_GetGroupCommanderIndexR(&AIGroups[g]);
    return index >= 0 && IsEntity(&Characters[index]) && CheckAttribute(&Characters[index], "trafficFleetID") &&
        Characters[index].trafficFleetID == descriptor.trafficFleetID && WdmFleetSeaAlive(&Characters[index]);
}

bool SeaTraffic_CanAdvanceFleet(aref descriptor, ref group)
{
    if (!bSeaActive || !bSeaLoaded || bSeaReloadStarted || bAbordageStarted || sti(InterfaceStates.Launched) ||
        !WdmTrafficIsOrdinary(descriptor) || CheckAttribute(descriptor, "trafficHarbour") ||
        CheckAttribute(descriptor, "trafficMission") || CheckAttribute(descriptor, "trafficBattle") ||
        CheckAttribute(descriptor, "trafficReleasePending") || CheckAttribute(descriptor, "trafficRollback")) return false;
    if (CheckAttribute(group, "Task") && sti(group.Task) != AITASK_MOVE &&
        sti(group.Task) != AITASK_DRIFT && sti(group.Task) != AITASK_NONE) return false;
    int count = 0;
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int index = Group_GetCharacterIndexR(group, slot);
        if (index < 0) break;
        ref captain = &Characters[index];
        if (!WdmFleetSeaAlive(captain)) continue;
        if (!IsEntity(captain) || !WdmFleetSeaActorOwned(captain, true) ||
            CheckAttribute(captain, "ShipTaskLock") || CheckAttribute(captain, "SeaSurrender") ||
            CheckAttribute(captain, "Surrendered")) return false;
        if (CheckAttribute(captain, "Ship.Stopped") && sti(captain.Ship.Stopped)) return false;
        if (CheckAttribute(captain, "SeaAI.Task") && sti(captain.SeaAI.Task) != AITASK_MOVE &&
            sti(captain.SeaAI.Task) != AITASK_DRIFT && sti(captain.SeaAI.Task) != AITASK_NONE) return false;
        count++;
    }
    return count > 0;
}

void SeaTraffic_FleetCourse(aref descriptor, ref group)
{
    float x = Sea_LocalX(stf(descriptor.gotoX), stf(descriptor.gotoZ));
    float z = Sea_LocalZ(stf(descriptor.gotoX), stf(descriptor.gotoZ));
    if (CheckAttribute(group, "Task.Pos.x") && CheckAttribute(group, "Task.Pos.z") &&
        sti(group.Task) == AITASK_MOVE && abs(stf(group.Task.Pos.x) - x) < 1.0 &&
        abs(stf(group.Task.Pos.z) - z) < 1.0) return;
    group.Task = AITASK_MOVE; group.Task.Pos.x = x; group.Task.Pos.z = z;
    group.trafficIntent = descriptor.trafficIntent;
    WdmFleetSeaApplyTask(group.id);
}

// Apply only service-owned values. Entity identity, pose, task, reload and
// independently observed damage remain with the existing physical actors.
bool SeaTraffic_ApplySails(aref descriptor, ref captain, aref entry)
{
    if (!CheckAttribute(entry, "trafficService.nativeSails")) return true;
    float remaining = stf(entry.trafficService.nativeSails);
    float previous = stf(captain.Ship.SP);
    int broken = 0;
    aref masts; makearef(masts, captain.Ship.Masts);
    for (int m = 0; m < GetAttributesNum(masts); m++)
    {
        aref mast = GetAttributeN(masts, m);
        int number = BI_MastRepairMainNumber(GetAttributeName(mast));
        if (number > 0 && number < 100 && stf(GetAttributeValue(mast)) >= 1.0) broken++;
    }
    if (CheckAttribute(entry, "trafficService.nativeMasts") && broken > 0)
    {
        object before; aref source; makearef(source, captain.Ship); CopyAttributes(&before, source);
        BI_MastRepairRestore(captain, broken);
        if (SendMessage(captain, "l", MSG_SHIP_REPAIR_MASTS) != broken)
        {
            aref saved, target;
            makearef(saved, before.Masts); makearef(target, captain.Ship.Masts); CopyAttributes(target, saved);
            DeleteAttribute(captain, "Ship.Sails");
            if (CheckAttribute(&before, "Sails"))
            { makearef(saved, before.Sails); makearef(target, captain.Ship.Sails); CopyAttributes(target, saved); }
            captain.Ship.SP = before.SP;
            descriptor.trafficServiceBlocked = "mast-geometry";
            return false;
        }
    }
    DeleteAttribute(entry, "trafficService.nativeMasts");
    remaining = remaining - stf(captain.Ship.SP) + previous;
    if (remaining > 0.0) remaining = remaining - ProcessSailRepair(captain, remaining);
    if (remaining > 0.01)
    {
        entry.trafficService.nativeSails = remaining;
        descriptor.trafficServiceBlocked = "sail-geometry";
        return false;
    }
    DeleteAttribute(entry, "trafficService.nativeSails");
    return true;
}

bool SeaTraffic_ApplyService(aref descriptor, ref group)
{
    bool applied = true;
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int index = Group_GetCharacterIndexR(group, slot);
        if (index < 0) break;
        ref captain = &Characters[index];
        if (!WdmFleetSeaAlive(captain)) continue;
        string key = "ship" + captain.trafficRosterSlot;
        aref entry; makearef(entry, descriptor.encdata.trafficRoster.(key));
        float hp = stf(entry.Ship.HP);
        float previousHP = stf(captain.Ship.HP);
        float sp = stf(entry.Ship.SP);
        if (hp > previousHP)
        {
            captain.Ship.HP = hp;
            int type = GetCharacterShipType(captain);
            if (hp >= stf(RealShips[type].HP)) Ship_ResetHullDamage(captain);
            else Ship_QueueHullMaterialRepair(captain, 100.0 * (hp - previousHP) / stf(RealShips[type].HP));
        }
        SetCrewQuantity(captain, sti(entry.trafficCrewQuantity));
        for (int good = 0; good < GOODS_QUANTITY; good++)
        {
            int quantity = WdmTrafficEntryGoods(entry, good);
            if (quantity != GetCargoGoods(captain, good)) SetCharacterGoods(captain, good, quantity);
        }
        if (!CheckAttribute(entry, "trafficService.nativeSails") && sp > stf(captain.Ship.SP))
        {
            entry.trafficService.nativeSails = sp - stf(captain.Ship.SP);
            if (sp >= 100.0) entry.trafficService.nativeMasts = 1;
        }
        if (CheckAttribute(entry, "Ship.Cannons.Borts"))
        {
            aref borts; makearef(borts, entry.Ship.Cannons.Borts);
            for (int b = 0; b < GetAttributesNum(borts); b++)
            {
                aref bort = GetAttributeN(borts, b);
                if (!CheckAttribute(bort, "damages")) continue;
                string side = GetAttributeName(bort);
                aref damages; makearef(damages, bort.damages);
                for (int c = 0; c < GetAttributesNum(damages); c++)
                {
                    aref damage = GetAttributeN(damages, c);
                    string cannon = GetAttributeName(damage);
                    captain.Ship.Cannons.Borts.(side).damages.(cannon) = GetAttributeValue(damage);
                }
            }
        }
        RecalculateCargoLoad(captain);
        if (!SeaTraffic_ApplySails(descriptor, captain, entry)) applied = false;
    }
    if (!applied) return false;
    DeleteAttribute(descriptor, "trafficServiceBlocked");
    return true;
}

void SeaTraffic_AdvanceFleet(aref descriptor)
{
    if (!SeaTraffic_HasOriginalGroup(descriptor)) return;
    string groupID = descriptor.trafficActorGroup;
    ref group = Group_GetGroupByID(groupID);
    if (!SeaTraffic_CanAdvanceFleet(descriptor, group)) return;
    // Retry only the unapplied paid rigging delta before observing fresh state;
    // never replay a stale crew/cargo/hull snapshot over the physical actor.
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int member = Group_GetCharacterIndexR(group, slot);
        if (member < 0) break;
        ref actor = &Characters[member];
        if (!WdmFleetSeaAlive(actor)) continue;
        string key = "ship" + actor.trafficRosterSlot;
        aref entry; makearef(entry, descriptor.encdata.trafficRoster.(key));
        if (!SeaTraffic_ApplySails(descriptor, actor, entry)) return;
    }
    int commander = Group_GetGroupCommanderIndexR(group);
    ref captain = &Characters[commander];
    descriptor.x = Sea_MapX(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z));
    descriptor.z = Sea_MapZ(stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z));
    if (descriptor.trafficLifecycle != "service")
    {
        float dx = stf(descriptor.x) - stf(descriptor.gotoX);
        float dz = stf(descriptor.z) - stf(descriptor.gotoZ);
        float radius = 12.0;
        if (CheckAttribute(descriptor, "gotoRad") && stf(descriptor.gotoRad) > radius) radius = stf(descriptor.gotoRad);
        if (dx * dx + dz * dz > radius * radius) { SeaTraffic_FleetCourse(descriptor, group); return; }
        if (sti(descriptor.trafficRole) == 1 || CheckAttribute(descriptor, "trafficServiceOnArrival"))
        {
            WdmFleetSeaSnapshotFleet(descriptor.trafficFleetID, false);
            descriptor.trafficLifecycle = "service"; descriptor.trafficIntent = "service";
            WdmTrafficArriveFleet(descriptor);
            if (!SeaTraffic_ApplyService(descriptor, group)) return;
            group.Task = AITASK_DRIFT; group.trafficIntent = "service";
            WdmFleetSeaApplyTask(groupID);
        }
        else
        {
            if (!CheckAttribute(descriptor, "trafficReturnX") || !CheckAttribute(descriptor, "trafficReturnZ")) return;
            float x = stf(descriptor.trafficReturnX);
            float z = stf(descriptor.trafficReturnZ);
            descriptor.trafficReturnX = descriptor.gotoX; descriptor.trafficReturnZ = descriptor.gotoZ;
            descriptor.gotoX = x; descriptor.gotoZ = z;
            SeaTraffic_FleetCourse(descriptor, group);
            return;
        }
    }
    WdmFleetSeaSnapshotFleet(descriptor.trafficFleetID, false);
    WdmTrafficVoyageAdvance(descriptor);
    if (!SeaTraffic_ApplyService(descriptor, group)) return;
    group.trafficIntent = descriptor.trafficIntent;
    if (!CheckAttribute(descriptor, "trafficNextLocator")) return;
    object endpoint;
    aref resources;
    if (!GetEntity(&resources, "WdmStaticResources"))
    { descriptor.trafficRouteBlocked = "resources"; return; }
    if (!SendMessage(resources, "sas", "SeaLocator", &endpoint, descriptor.trafficNextLocator))
    { descriptor.trafficRouteBlocked = "locator"; return; }
    descriptor.gotoX = endpoint.x; descriptor.gotoZ = endpoint.z; descriptor.gotoRad = 2.0;
    DeleteAttribute(descriptor, "trafficNextLocator"); DeleteAttribute(descriptor, "trafficRouteBlocked");
    descriptor.trafficLifecycle = "voyage"; descriptor.trafficVoyage = sti(descriptor.trafficVoyage) + 1;
    WdmTrafficDepartFleet(descriptor);
    SeaTraffic_FleetCourse(descriptor, group);
}

void WdmFleetSeaApplyTask(string groupID)
{
    ref group = Group_GetGroupByID(groupID);
    switch (sti(group.Task))
    {
        case AITASK_DRIFT: WdmFleetSeaSetDrift(groupID); break;
        case AITASK_RUNAWAY: Group_SetTaskRunAway(groupID, group.Task.Target); break;
        case AITASK_ATTACK: Group_SetTaskAttack(groupID, group.Task.Target); break;
        case AITASK_MOVE:
            if (CheckAttribute(group, "Task.Pos")) Group_SetTaskMove(groupID, stf(group.Task.Pos.x), stf(group.Task.Pos.z));
            else if (CheckAttribute(group, "Task.Target.Pos")) Group_SetTaskMove(groupID, stf(group.Task.Target.Pos.x), stf(group.Task.Target.Pos.z));
            else Group_SetTaskMove(groupID, 10000.0 * sin(stf(group.Pos.ay)), 10000.0 * cos(stf(group.Pos.ay)));
        break;
    }
    if (CheckAttribute(group, "EmptyFantom")) return;
    ref commander = Group_GetGroupCommanderR(group);
    int relation = GetRelation(nMainCharacterIndex, sti(commander.index));
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int index = Group_GetCharacterIndexR(group, slot);
        if (index < 0) break;
        SetCharacterRelationBoth(index, nMainCharacterIndex, relation);
        GreatRecipe_AllianceRegisterPair(index, relation);
    }
}

bool SeaTraffic_CanRelease(string groupID)
{
    int g = Group_FindGroup(groupID);
    if (g < 0 || bAbordageStarted || sti(InterfaceStates.Launched)) return false;
    ref group = &AIGroups[g];
    if (!CheckAttribute(group, "trafficFleetID")) return false;
    string path = "encounters." + group.trafficFleetID;
    if (!CheckAttribute(&worldMap, path)) return false;
    if (WdmMilitarySeaActiveBalls() != 0) return false;
    // Both sides leave the physical simulation together; their mutual task
    // references are released only by the native batch, never one at a time.
    if (CheckAttribute(&worldMap, path + ".trafficBattleRoot") && !CheckAttribute(&worldMap, path + ".trafficRollback"))
    {
        string root = worldMap.(path).trafficBattleRoot;
        aref descriptors; makearef(descriptors, worldMap.encounters);
        for (int d = 0; d < GetAttributesNum(descriptors); d++)
        {
            aref other = GetAttributeN(descriptors, d);
            if (!CheckAttribute(other, "trafficBattleRoot") || other.trafficBattleRoot != root ||
                !CheckAttribute(other, "trafficInSea") || !sti(other.trafficInSea)) continue;
            float dx = Sea_LocalX(stf(other.x), stf(other.z)) - stf(pchar.Ship.Pos.x);
            float dz = Sea_LocalZ(stf(other.x), stf(other.z)) - stf(pchar.Ship.Pos.z);
            if (dx * dx + dz * dz <= SEA_TRAFFIC_RELEASE_DISTANCE * SEA_TRAFFIC_RELEASE_DISTANCE) return false;
        }
    }
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int index = Group_GetCharacterIndexR(group, slot);
        if (index < 0) break;
        ref captain = &Characters[index];
        if (!WdmFleetSeaActorOwned(captain, false) || !CrimeSea_CanReleaseActor(captain)) return false;
    }
    return true;
}

bool SeaTraffic_ReleaseSnapshot()
{
    string groupID = GetEventData();
    if (!SeaTraffic_CanRelease(groupID)) return false;
    ref group = Group_GetGroupByID(groupID);
    SeaTraffic_ReportPositions();
    WdmFleetSeaSaveFleet(group.trafficFleetID);
    return true;
}

void SeaTraffic_Released()
{
    string groupID = GetEventData();
    int g = Group_FindGroup(groupID);
    if (g < 0) return;
    ref group = &AIGroups[g];
    string identity = group.trafficFleetID;
    string path = "encounters." + group.trafficFleetID;
    for (int slot = 0; slot < MAX_CHARACTERS; slot++)
    {
        int index = Group_GetCharacterIndexR(group, slot);
        if (index < 0) break;
        ref captain = &Characters[index];
        bool reusable = CrimeSea_CanReleaseActor(captain) && !IsCompanion(captain);
        if (reusable) CrimeSea_ReleaseActor(captain);
        if (CheckAttribute(captain, "Ship.Sounds.WaterID")) ReleaseSound(sti(captain.Ship.Sounds.WaterID));
        if (CheckAttribute(captain, "Ship.Sounds.SailsID")) ReleaseSound(sti(captain.Ship.Sounds.SailsID));
        if (CheckAttribute(captain, "Ship.Sounds.TurnID")) ReleaseSound(sti(captain.Ship.Sounds.TurnID));
        if (reusable && (index >= FANTOM_CHARACTERS || (CheckAttribute(captain, "trafficHarbour") && CheckAttribute(captain, "IslandShips"))))
        {
            int realIndex = GetCharacterShipType(captain);
            InitCharacter(captain, index);
            if (realIndex >= 0 && realIndex < REAL_SHIPS_QUANTITY) DeleteAttribute(&RealShips[realIndex], "");
            if (index >= FANTOM_CHARACTERS)
            {
                string freeSlot = "c" + index;
                SeaTrafficFreeSlots.(freeSlot).index = index;
                aref cleared; makearef(cleared, SeaTrafficFreeSlots.(freeSlot).character);
                CopyAttributes(cleared, captain);
            }
        }
        for (int s = iNumShips - 1; s >= 0; s--)
        {
            if (Ships[s] != index) continue;
            for (int move = s; move < iNumShips - 1; move++) Ships[move] = Ships[move + 1];
            iNumShips--;
        }
    }
    Group_DeleteGroup(groupID);
    if (CheckAttribute(&worldMap, path))
    {
        DeleteAttribute(&worldMap, path + ".trafficReleasePending");
        DeleteAttribute(&worldMap, path + ".trafficInSea");
        DeleteAttribute(&worldMap, path + ".trafficActorGroup");
        DeleteAttribute(&worldMap, path + ".trafficRollback");
        worldMap.restoreEncounter = identity;
        SeaTraffic_Notify("restoreEncounter");
    }
}

void SeaTraffic_ReleaseRejected()
{
    string groupID = GetEventData();
    int g = Group_FindGroup(groupID);
    if (g < 0 || !CheckAttribute(&AIGroups[g], "trafficFleetID")) return;
    DeleteAttribute(&worldMap, "encounters." + AIGroups[g].trafficFleetID + ".trafficReleasePending");
}

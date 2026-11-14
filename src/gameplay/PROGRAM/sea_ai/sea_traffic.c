// One descriptor owns each ordinary fleet; native map navigation is its distant
// projection, and a mounted sea group is its exclusive physical projection.
object SeaTraffic;
object SeaTrafficFreeSlots;
object SeaPhonkTest;

#define PHONK_TEST_GROUP "phonk_test_enemy"

bool SeaPhonkTest_OwnsIndex(int index)
{
    if (index < FANTOM_CHARACTERS || index >= TOTAL_CHARACTERS) return false;
    string key = "c" + index;
    if (!CheckAttribute(&SeaPhonkTest, "enemies." + key + ".group") ||
        !CheckAttribute(&SeaPhonkTest, "enemies." + key + ".shipType")) return false;
    string groupID = SeaPhonkTest.enemies.(key).group;
    ref captain = &Characters[index];
    if (!CheckAttribute(captain, "PhonkTestOwner") || captain.PhonkTestOwner != groupID ||
        !CheckAttribute(captain, "trafficFleetID") || captain.trafficFleetID != groupID ||
        !CheckAttribute(captain, "SeaAI.Group.Name") || captain.SeaAI.Group.Name != groupID ||
        !CheckAttribute(captain, "Ship.Type")) return false;
    return sti(captain.Ship.Type) == sti(SeaPhonkTest.enemies.(key).shipType) && !IsCompanion(captain);
}

int SeaPhonkTest_GroupIndex(string groupID)
{
    if (!CheckAttribute(&SeaPhonkTest, "enemies")) return -1;
    aref enemies; makearef(enemies, SeaPhonkTest.enemies);
    for (int i = 0; i < GetAttributesNum(enemies); i++)
    {
        aref entry = GetAttributeN(enemies, i);
        if (CheckAttribute(entry, "group") && entry.group == groupID && CheckAttribute(entry, "index")) return sti(entry.index);
    }
    return -1;
}

bool SeaPhonkTest_Start()
{
    return SeaPhonkTest_StartSide(false);
}

bool SeaPhonkTest_StartAlly()
{
    return SeaPhonkTest_StartSide(true);
}

bool SeaPhonkTest_StartSide(bool allied)
{
    if (!bSeaActive || !bSeaLoaded || !IsEntity(&AISea)) return false;
    if (GetRealShipsCount() >= REAL_SHIPS_QUANTITY) { Log_Info("Нет свободных слотов кораблей"); return false; }
    float x = 0.0;
    float z = 0.0;
    float heading = stf(pchar.Ship.Ang.y);
    float angle = heading;
    float offset = 0.0;
    bool found = false;
    for (int attempt = 0; attempt < 128; attempt++)
    {
        int step = (attempt + 1) / 2;
        offset = 6.2831853 * step / 128.0;
        if (attempt % 2 == 0) offset = -offset;
        float a = heading + offset;
        float radius = 8250.0;
        if (allied) radius = 600.0;
        x = stf(pchar.Ship.Pos.x) + radius * sin(a);
        z = stf(pchar.Ship.Pos.z) + radius * cos(a);
        if (SendMessage(&AISea, "lff", AI_MESSAGE_TEST_SPAWN_POSITION, x, z)) { found = true; angle = a; break; }
    }
    if (!found) { Log_Info("Не найдено безопасное место для корабля"); return false; }

    int serial = 0;
    if (CheckAttribute(&SeaPhonkTest, "nextGroup")) serial = sti(SeaPhonkTest.nextGroup);
    string groupID = PHONK_TEST_GROUP + "_" + serial;
    while (Group_FindGroup(groupID) >= 0 || SeaPhonkTest_GroupIndex(groupID) >= 0)
    {
        serial++;
        groupID = PHONK_TEST_GROUP + "_" + serial;
    }
    SeaPhonkTest.nextGroup = serial + 1;
    object fleet;
    fleet.GroupName = groupID;
    fleet.trafficFleetID = groupID;
    fleet.RealEncounterType = ENCOUNTER_TYPE_PATROL_SMALL;
    fleet.Nation = PIRATE;
    fleet.Type = "war";
    bool custom = CheckAttribute(pchar, "SeaPhonk.Spawn.Custom") && sti(pchar.SeaPhonk.Spawn.Custom) != 0;
    int shipChoice = 0;
    if (CheckAttribute(pchar, "SeaPhonk.Spawn.ShipClass")) shipChoice = sti(pchar.SeaPhonk.Spawn.ShipClass);
    if (shipChoice < 1 || shipChoice > 5) shipChoice = 1 + rand(4);
    int shipType = SHIP_BILANCETTA;
    switch (shipChoice)
    {
        case 2: shipType = SHIP_CORVETTE; break;
        case 3: shipType = SHIP_FRIGATE; break;
        case 4: shipType = SHIP_BATTLESHIP; break;
        case 5: shipType = SHIP_MANOWAR; break;
    }
    fleet.trafficRoster.ship0.baseType = shipType;
    fleet.trafficRoster.ship0.mode = "war";
    int index = SeaTraffic_AllocateCaptain(&fleet, 0);
    if (index < 0) { Log_Info("Не удалось создать корабль для ПВО"); return false; }
    ref captain = &Characters[index];
    if (!allied) captain.AlwaysEnemy = true;
    ref group = Group_FindOrCreateGroup(groupID);
    Group_SetXZ_AY(groupID, x, z, angle + 3.1415926);
    Group_SetType(groupID, "war");
    group.trafficFleetID = groupID;
    Sea_MountEncounterCharacter(&fleet, index, 0, index, groupID, pchar.location, false);
    if (!IsEntity(captain))
    {
        Group_DeleteGroup(groupID);
        int realIndex = GetCharacterShipType(captain);
        InitCharacter(captain, index);
        if (realIndex >= 0 && realIndex < REAL_SHIPS_QUANTITY) DeleteAttribute(&RealShips[realIndex], "");
        string freeSlot = "c" + index;
        SeaTrafficFreeSlots.(freeSlot).index = index;
        aref cleared; makearef(cleared, SeaTrafficFreeSlots.(freeSlot).character);
        CopyAttributes(cleared, captain);
        Log_Info("Не удалось разместить корабль для ПВО");
        return false;
    }
    string key = "c" + index;
    captain.PhonkTestOwner = groupID;
    // Per-ship loadout for the native test-enemy launcher; it must consume
    // these attributes before they can affect missile behavior.
    captain.PhonkTest.MissileType = rand(2); // 0 Kinzhal, 1 Kalibr, 2 Shtorm
    int warheadChoice = rand(3);
    captain.PhonkTest.Warhead = 1;
    if (warheadChoice == 1) captain.PhonkTest.Warhead = 2;
    if (warheadChoice == 2) captain.PhonkTest.Warhead = 4;
    if (warheadChoice == 3) captain.PhonkTest.Warhead = PHONK_WARHEAD_THERMOBARIC;
    captain.PhonkTest.CooldownMin = 2 + rand(4);
    captain.PhonkTest.CooldownMax = sti(captain.PhonkTest.CooldownMin) + 3 + rand(3);
    if (shipType == SHIP_MANOWAR) captain.PhonkTest.Volley = 3 + rand(2);
    else captain.PhonkTest.Volley = 1 + rand(2);
    if (custom)
    {
        captain.PhonkTest.Custom = 1;
        captain.PhonkTest.MissileType = sti(pchar.SeaPhonk.Spawn.MissileType);
        captain.PhonkTest.Warhead = sti(pchar.SeaPhonk.Spawn.Warhead);
        captain.PhonkTest.Volley = sti(pchar.SeaPhonk.Spawn.Volley);
        captain.PhonkTest.CooldownMin = stf(pchar.SeaPhonk.Spawn.FireInterval);
        captain.PhonkTest.CooldownMax = stf(pchar.SeaPhonk.Spawn.FireInterval);
    }
    bool defenseCustom = custom;
    if (CheckAttribute(pchar, "SeaPhonk.Spawn.DefenseCustom")) defenseCustom = sti(pchar.SeaPhonk.Spawn.DefenseCustom) != 0;
    captain.PhonkTest.DefenseSettings = 1;
    captain.PhonkTest.DefenseCustom = defenseCustom;
    int defenseType = 2;
    if (CheckAttribute(pchar, "SeaPhonk.Spawn.AirDefenseType")) defenseType = sti(pchar.SeaPhonk.Spawn.AirDefenseType);
    if (defenseType < 0 || defenseType > 1) defenseType = rand(1);
    captain.PhonkTest.AirDefenseType = defenseType;
    if (defenseCustom)
    {
        captain.PhonkTest.DefenseEnabled = sti(pchar.SeaPhonk.Spawn.DefenseEnabled);
        captain.PhonkTest.DefenseRange = stf(pchar.SeaPhonk.Spawn.DefenseRange);
        captain.PhonkTest.DefenseInterval = stf(pchar.SeaPhonk.Spawn.DefenseInterval);
        captain.PhonkTest.DefenseMax = sti(pchar.SeaPhonk.Spawn.DefenseMax);
        captain.PhonkTest.DefenseSpeed = stf(pchar.SeaPhonk.Spawn.DefenseSpeed);
    }
    else
    {
        captain.PhonkTest.DefenseEnabled = 1;
        captain.PhonkTest.DefenseRange = 1200 + rand(24) * 100;
        captain.PhonkTest.DefenseInterval = 0.3 + rand(9) * 0.1;
        if (defenseType == 1) captain.PhonkTest.DefenseInterval = stf(captain.PhonkTest.DefenseInterval) * 1.5;
        captain.PhonkTest.DefenseMax = 2 + rand(6);
        captain.PhonkTest.DefenseSpeed = 450 + rand(6) * 50;
    }
    SeaPhonkTest.enemies.(key).index = index;
    SeaPhonkTest.enemies.(key).group = groupID;
    SeaPhonkTest.enemies.(key).shipType = captain.Ship.Type;
    if (allied)
    {
        SetCharacterRelationAsOtherCharacter(index, nMainCharacterIndex);
        SeaPhonkTest.enemies.(key).allied = 1;
        Sea.Phonk.TestAllies.(key) = index;
    }
    else
    {
        SetCharacterRelationBoth(index, nMainCharacterIndex, RELATION_ENEMY);
        Sea.Phonk.TestEnemies.(key) = index;
    }
    UpdateRelations();
    WdmFleetSeaSetDrift(groupID);
    if (allied)
    {
        Log_Info("Союзный ракетный корабль появился рядом");
        return true;
    }
    string description = "Вражеский корабль: " + GetStrSmallRegister(XI_ConvertString(RealShips[sti(captain.Ship.Type)].BaseName));
    if (offset == 0.0) Log_Info(description + ": прямо по курсу, 8,25 км.");
    else if (offset >= 3.1415926) Log_Info(description + ": позади, 8,25 км.");
    else if (offset > 0.0) Log_Info(description + ": вправо от курса на " + sti(offset * 57.29578 + 0.5) + "°, 8,25 км.");
    else Log_Info(description + ": влево от курса на " + sti(-offset * 57.29578 + 0.5) + "°, 8,25 км.");
    return true;
}

void SeaPhonkTest_UpdateAlly(int index)
{
    int nearest = -1;
    float nearestDistance = 0.0;
    ref ally = &Characters[index];
    if (!CheckAttribute(ally, "Ship.Pos.x") || !CheckAttribute(ally, "Ship.Pos.z")) return;
    for (int s = 0; s < iNumShips; s++)
    {
        int other = Ships[s];
        if (other < 0 || other >= TOTAL_CHARACTERS || other == index || other == nMainCharacterIndex) continue;
        ref target = &Characters[other];
        if (!IsEntity(target) || LAi_IsDead(target) || !CheckAttribute(target, "Ship.Pos.x") ||
            !CheckAttribute(target, "Ship.Pos.z") ||
            (CheckAttribute(target, "Ship.HP") && stf(target.Ship.HP) <= 0.0) ||
            GetRelation(nMainCharacterIndex, other) != RELATION_ENEMY) continue;
        float dx = stf(target.Ship.Pos.x) - stf(ally.Ship.Pos.x);
        float dz = stf(target.Ship.Pos.z) - stf(ally.Ship.Pos.z);
        float distance = dx * dx + dz * dz;
        if (nearest < 0 || distance < nearestDistance) { nearest = other; nearestDistance = distance; }
    }
    if (nearest >= 0)
    {
        if (!CheckAttribute(ally, "SeaAI.Task") || sti(ally.SeaAI.Task) != AITASK_ATTACK ||
            !CheckAttribute(ally, "SeaAI.Task.Target") || sti(ally.SeaAI.Task.Target) != nearest)
            Ship_SetTaskAttack(SECONDARY_TASK, index, nearest);
    }
    else if (CheckAttribute(ally, "SeaAI.Task") && sti(ally.SeaAI.Task) == AITASK_ATTACK)
        Ship_SetTaskNone(SECONDARY_TASK, index);
}

// The debug encounter owns a separate captain and real-ship slot. Never move
// or reset GhostCapt: that character and its ship belong to the quest.
bool SeaDebugGhost_Start()
{
    if (!bSeaActive || !bSeaLoaded || bAbordageStarted || !IsEntity(&AISea) ||
        iNumShips >= MAX_SHIPS_ON_SEA || GetRealShipsCount() >= REAL_SHIPS_QUANTITY) return false;

    float x = 0.0;
    float z = 0.0;
    float angle = stf(pchar.Ship.Ang.y);
    bool found = false;
    for (int attempt = 0; attempt < 128; attempt++)
    {
        int step = (attempt + 1) / 2;
        float offset = 6.2831853 * step / 128.0;
        if (attempt % 2 == 0) offset = -offset;
        float candidate = stf(pchar.Ship.Ang.y) + offset;
        x = stf(pchar.Ship.Pos.x) + 2500.0 * sin(candidate);
        z = stf(pchar.Ship.Pos.z) + 2500.0 * cos(candidate);
        if (SendMessage(&AISea, "lff", AI_MESSAGE_TEST_SPAWN_POSITION, x, z))
        {
            angle = candidate;
            found = true;
            break;
        }
    }
    if (!found) { Log_Info("Не найдено безопасное место для Летучего Голландца"); return false; }

    int original = -1;
    int ghostIndex = GetCharacterIndex("GhostCapt");
    if (ghostIndex >= 0 && CheckAttribute(&Characters[ghostIndex], "Ship.Type"))
    {
        int candidateShip = sti(Characters[ghostIndex].Ship.Type);
        if (candidateShip >= 0 && candidateShip < REAL_SHIPS_QUANTITY &&
            CheckAttribute(&RealShips[candidateShip], "BaseType") &&
            sti(RealShips[candidateShip].BaseType) == SHIP_FLYINGDUTCHMAN) original = candidateShip;
    }
    bool temporaryOriginal = false;
    if (original < 0)
    {
        // GhostCapt is created by the quest, which may not have started yet.
        original = CreateBaseShip(SHIP_FLYINGDUTCHMAN);
        if (original < 0 || original >= REAL_SHIPS_QUANTITY) return false;
        temporaryOriginal = true;
        RealShips[original].ship.upgrades.sails = 3;
        RealShips[original].EmblemedSails.normalTex = "ships\parus_common_torn.tga";
        RealShips[original].ShipSails.SailsColor = argb(255, 60, 60, 60);
        RealShips[original].MaxCaliber = 48;
        RealShips[original].Cannon = CANNON_TYPE_CANNON_LBS48;
        RealShips[original].HP = 400000;
    }

    int serial = 0;
    if (CheckAttribute(&SeaPhonkTest, "nextGhost")) serial = sti(SeaPhonkTest.nextGhost);
    string groupID = "debug_ghost_" + serial;
    while (Group_FindGroup(groupID) >= 0) { serial++; groupID = "debug_ghost_" + serial; }
    if (Group_CreateGroup(groupID) < 0)
    {
        if (temporaryOriginal) DeleteAttribute(&RealShips[original], "");
        return false;
    }

    object fleet;
    fleet.GroupName = groupID;
    fleet.trafficFleetID = groupID;
    fleet.RealEncounterType = ENCOUNTER_TYPE_PATROL_SMALL;
    fleet.Nation = PIRATE;
    fleet.Type = "war";
    // Allocate an ordinary fantom slot, then replace only its private hull.
    fleet.trafficRoster.ship0.baseType = SHIP_BILANCETTA;
    fleet.trafficRoster.ship0.mode = "war";
    int index = SeaTraffic_AllocateCaptain(&fleet, 0);
    if (index < 0)
    {
        Group_DeleteGroup(groupID);
        if (temporaryOriginal) DeleteAttribute(&RealShips[original], "");
        return false;
    }
    ref captain = &Characters[index];
    int shipIndex = GetCharacterShipType(captain);
    ref ship = &RealShips[shipIndex];
    DeleteAttribute(ship, "");
    CopyAttributes(ship, &RealShips[original]);
    ship.index = shipIndex;
    if (temporaryOriginal) DeleteAttribute(&RealShips[original], "");
    captain.AlwaysSandbankManeuver = true;
    Group_SetXZ_AY(groupID, x, z, angle + 3.1415926);
    Group_SetType(groupID, "war");
    Sea_MountEncounterCharacter(&fleet, index, 0, index, groupID, pchar.location, false);
    if (!IsEntity(captain))
    {
        Group_DeleteGroup(groupID);
        InitCharacter(captain, index);
        DeleteAttribute(ship, "");
        string freeKey = "c" + index;
        SeaTrafficFreeSlots.(freeKey).index = index;
        aref cleared; makearef(cleared, SeaTrafficFreeSlots.(freeKey).character);
        CopyAttributes(cleared, captain);
        return false;
    }
    captain.Ship.Name = "Летучий голландец";
    captain.AlwaysEnemy = true;
    captain.name = "Ван";
    captain.lastname = "дер Декен";
    captain.PhonkTestOwner = groupID;
    string key = "c" + index;
    SeaPhonkTest.enemies.(key).index = index;
    SeaPhonkTest.enemies.(key).group = groupID;
    SeaPhonkTest.enemies.(key).shipType = shipIndex;
    SeaPhonkTest.enemies.(key).debugGhost = 1;
    SeaPhonkTest.nextGhost = serial + 1;
    SeaDebugGhost_UpdateRelations(index);
    Log_Info("Летучий Голландец появился в море");
    return true;
}

void SeaDebugGhost_UpdateRelations(int index)
{
    string key = "c" + index;
    bool changed = false;
    int nearest = -1;
    float nearestDistance = 0.0;
    for (int s = 0; s < iNumShips; s++)
    {
        int other = Ships[s];
        if (other < 0 || other >= TOTAL_CHARACTERS || other == index || !IsEntity(&Characters[other])) continue;
        int shipIndex = other;
        if (CheckAttribute(&Characters[other], "relation.UseOtherCharacter"))
        {
            int delegate = sti(Characters[other].relation.UseOtherCharacter);
            if (delegate >= 0 && delegate < TOTAL_CHARACTERS) other = delegate;
        }
        if (other == index) continue;
        string target = "c" + other;
        string relationPath = "relation." + index;
        if (!CheckAttribute(&SeaPhonkTest, "enemies." + key + ".relations." + target) ||
            SeaPhonkTest.enemies.(key).relations.(target).id != Characters[other].id)
        {
            SeaPhonkTest.enemies.(key).relations.(target).id = Characters[other].id;
            SeaPhonkTest.enemies.(key).relations.(target).index = other;
            SeaPhonkTest.enemies.(key).relations.(target).had = CheckAttribute(&Characters[other], relationPath);
            if (CheckAttribute(&Characters[other], relationPath))
                SeaPhonkTest.enemies.(key).relations.(target).previous = Characters[other].(relationPath);
        }
        string ghostRelation = "relation." + other;
        if (!CheckAttribute(&Characters[index], ghostRelation) ||
            sti(Characters[index].(ghostRelation)) != RELATION_ENEMY ||
            !CheckAttribute(&Characters[other], relationPath) ||
            sti(Characters[other].(relationPath)) != RELATION_ENEMY)
        {
            SetCharacterRelationBoth(index, other, RELATION_ENEMY);
            changed = true;
        }
        ref targetShip = &Characters[shipIndex];
        if (LAi_IsDead(targetShip) || !CheckAttribute(targetShip, "Ship.Pos.x") ||
            !CheckAttribute(targetShip, "Ship.Pos.z") ||
            (CheckAttribute(targetShip, "Ship.HP") && stf(targetShip.Ship.HP) <= 0.0)) continue;
        float dx = stf(targetShip.Ship.Pos.x) - stf(Characters[index].Ship.Pos.x);
        float dz = stf(targetShip.Ship.Pos.z) - stf(Characters[index].Ship.Pos.z);
        float distance = dx * dx + dz * dz;
        if ((nearest < 0 || distance < nearestDistance) && GetRelation(index, shipIndex) == RELATION_ENEMY)
        { nearest = shipIndex; nearestDistance = distance; }
    }
    if (changed) UpdateRelations();
    if (nearest >= 0)
    {
        if (!CheckAttribute(&Characters[index], "SeaAI.Task") ||
            sti(Characters[index].SeaAI.Task) != AITASK_ATTACK ||
            !CheckAttribute(&Characters[index], "SeaAI.Task.Target") ||
            sti(Characters[index].SeaAI.Task.Target) != nearest)
            Ship_SetTaskAttack(SECONDARY_TASK, index, nearest);
    }
    else if (CheckAttribute(&Characters[index], "SeaAI.Task") &&
             sti(Characters[index].SeaAI.Task) == AITASK_ATTACK)
        Ship_SetTaskNone(SECONDARY_TASK, index);
}

void SeaDebugGhost_RestoreRelations(int index)
{
    string key = "c" + index;
    if (!CheckAttribute(&SeaPhonkTest, "enemies." + key + ".relations")) return;
    aref relations; makearef(relations, SeaPhonkTest.enemies.(key).relations);
    for (int i = 0; i < GetAttributesNum(relations); i++)
    {
        aref entry = GetAttributeN(relations, i);
        int other = sti(entry.index);
        if (other < 0 || other >= TOTAL_CHARACTERS || Characters[other].id != entry.id) continue;
        string path = "relation." + index;
        if (!CheckAttribute(&Characters[other], path) || sti(Characters[other].(path)) != RELATION_ENEMY) continue;
        if (sti(entry.had)) Characters[other].(path) = entry.previous;
        else DeleteAttribute(&Characters[other], path);
    }
}

void SeaPhonkTest_Stop()
{
    if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestEnemies");
    if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestAllies");
    if (!CheckAttribute(&SeaPhonkTest, "enemies")) return;
    aref enemies; makearef(enemies, SeaPhonkTest.enemies);
    for (int i = 0; i < GetAttributesNum(enemies); i++)
    {
        aref entry = GetAttributeN(enemies, i);
        if (!CheckAttribute(entry, "debugGhost")) entry.stopping = 1;
    }
}

void SeaPhonkTest_ReleaseOne(int index, bool releaseSounds)
{
    string key = "c" + index;
    string path = "enemies." + key;
    if (!CheckAttribute(&SeaPhonkTest, path + ".group")) return;
    string groupID = SeaPhonkTest.enemies.(key).group;
    bool owned = SeaPhonkTest_OwnsIndex(index);
    int groupIndex = Group_FindGroup(groupID);
    bool ownedGroup = owned && groupIndex >= 0 && Group_GetCharacterIndexR(&AIGroups[groupIndex], 0) == index &&
        Group_GetCharacterIndexR(&AIGroups[groupIndex], 1) < 0;
    if (owned)
    {
        ref captain = &Characters[index];
        if (CheckAttribute(&SeaPhonkTest, path + ".debugGhost")) SeaDebugGhost_RestoreRelations(index);
        if (!CrimeSea_CanReleaseActor(captain))
        {
            if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestEnemies." + key);
            if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestAllies." + key);
            DeleteAttribute(&SeaPhonkTest, path);
            return;
        }
        if (releaseSounds && CheckAttribute(captain, "Ship.Sounds"))
        {
            aref sounds; makearef(sounds, captain.Ship.Sounds);
            Ship_ReleaseStaticSounds(sounds);
        }
        int realIndex = GetCharacterShipType(captain);
        CrimeSea_ReleaseActor(captain);
        if (!CheckAttribute(&SeaPhonkTest, path + ".debugGhost") && !CheckAttribute(&SeaPhonkTest, path + ".allied"))
            DeleteAttribute(pchar, "relation." + index);
        InitCharacter(captain, index);
        if (realIndex >= 0 && realIndex < REAL_SHIPS_QUANTITY) DeleteAttribute(&RealShips[realIndex], "");
        if (releaseSounds)
        {
            SeaTrafficFreeSlots.(key).index = index;
            aref cleared; makearef(cleared, SeaTrafficFreeSlots.(key).character);
            CopyAttributes(cleared, captain);
        }
        for (int s = iNumShips - 1; s >= 0; s--)
        {
            if (Ships[s] != index) continue;
            for (int n = s; n < iNumShips - 1; n++) Ships[n] = Ships[n + 1];
            iNumShips--;
        }
    }
    if (ownedGroup) Group_DeleteGroup(groupID);
    if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestEnemies." + key);
    if (IsEntity(&Sea)) DeleteAttribute(&Sea, "Phonk.TestAllies." + key);
    DeleteAttribute(&SeaPhonkTest, path);
}

void SeaPhonkTest_Release(bool releaseSounds)
{
    if (!CheckAttribute(&SeaPhonkTest, "enemies")) return;
    aref enemies; makearef(enemies, SeaPhonkTest.enemies);
    for (int i = GetAttributesNum(enemies) - 1; i >= 0; i--)
    {
        aref entry = GetAttributeN(enemies, i);
        if (CheckAttribute(entry, "index")) SeaPhonkTest_ReleaseOne(sti(entry.index), releaseSounds);
    }
    DeleteAttribute(&SeaPhonkTest, "enemies");
}

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
    if (CheckAttribute(&SeaPhonkTest, "enemies"))
    {
        aref enemies; makearef(enemies, SeaPhonkTest.enemies);
        for (int t = GetAttributesNum(enemies) - 1; t >= 0; t--)
        {
            aref entry = GetAttributeN(enemies, t);
            int testIndex = sti(entry.index);
            string key = GetAttributeName(entry);
            if (!SeaPhonkTest_OwnsIndex(testIndex))
            {
                if (CheckAttribute(entry, "debugGhost")) SeaDebugGhost_RestoreRelations(testIndex);
                DeleteAttribute(&Sea, "Phonk.TestEnemies." + key);
                DeleteAttribute(&Sea, "Phonk.TestAllies." + key);
                DeleteAttribute(enemies, key);
                continue;
            }
            if (!IsEntity(&Characters[testIndex]) ||
                (CheckAttribute(&Characters[testIndex], "Ship.HP") && stf(Characters[testIndex].Ship.HP) <= 0.0))
            {
                DeleteAttribute(&Sea, "Phonk.TestEnemies." + key);
                DeleteAttribute(&Sea, "Phonk.TestAllies." + key);
                entry.stopping = 1;
            }
            else if (CheckAttribute(entry, "debugGhost")) SeaDebugGhost_UpdateRelations(testIndex);
            else if (CheckAttribute(entry, "allied")) SeaPhonkTest_UpdateAlly(testIndex);
            if (CheckAttribute(entry, "stopping") && !CheckAttribute(entry, "pending"))
            {
                entry.pending = 1;
                SendMessage(&AISea, "ls", AI_MESSAGE_RELEASE_GROUP, entry.group);
            }
        }
    }
    SeaAutopilot_Tick();
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
    int testIndex = SeaPhonkTest_GroupIndex(groupID);
    if (testIndex >= 0)
    {
        string key = "c" + testIndex;
        if (!CheckAttribute(&SeaPhonkTest, "enemies." + key + ".stopping") || bAbordageStarted || sti(InterfaceStates.Launched)) return false;
        return SeaPhonkTest_OwnsIndex(testIndex) && CrimeSea_CanReleaseActor(&Characters[testIndex]);
    }
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
    if (SeaPhonkTest_GroupIndex(groupID) >= 0) return SeaTraffic_CanRelease(groupID);
    if (!SeaTraffic_CanRelease(groupID)) return false;
    ref group = Group_GetGroupByID(groupID);
    SeaTraffic_ReportPositions();
    WdmFleetSeaSaveFleet(group.trafficFleetID);
    return true;
}

void SeaTraffic_Released()
{
    string groupID = GetEventData();
    int testIndex = SeaPhonkTest_GroupIndex(groupID);
    if (testIndex >= 0) { SeaPhonkTest_ReleaseOne(testIndex, true); return; }
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
    int testIndex = SeaPhonkTest_GroupIndex(groupID);
    if (testIndex >= 0) { DeleteAttribute(&SeaPhonkTest, "enemies.c" + testIndex + ".pending"); return; }
    int g = Group_FindGroup(groupID);
    if (g < 0 || !CheckAttribute(&AIGroups[g], "trafficFleetID")) return;
    DeleteAttribute(&worldMap, "encounters." + AIGroups[g].trafficFleetID + ".trafficReleasePending");
}

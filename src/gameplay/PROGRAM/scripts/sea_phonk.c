// Local-sea mode: one transient scene flag drives ship physics, lighting and music.
string SEA_PHONK_TRACK = "MUSIC\\phonk-drive.mp3";

int SeaPhonk_PlayerDefenseType(int type)
{
    if (type < 0 || type > 2) return 0;
    return type;
}

int SeaPhonk_DefensePerTarget(int count)
{
    if (count < 1) return 1;
    if (count > 8) return 8;
    return count;
}

string SeaPhonk_MissileName(int type)
{
    if (type == 0) return "Кинжал";
    if (type == 1) return "Калибр";
    if (type == 2) return "Шторм";
    if (type == 3) return "Случайная";
    if (type == 4) return "Орешник";
    return "";
}

int SeaPhonk_NextMissileType(int type)
{
    type = type + 1;
    if (SeaPhonk_MissileName(type) == "") return 0;
    return type;
}

void SeaPhonk_DeckCameraControl(string name, string keyName, bool reevaluate)
{
    if (!reevaluate && CheckAttribute(&objControlsState, "keygroups.Sailing1Pers." + name)) return;
    int key = CI_GetKeyCode(keyName);
    aref group, item, groups;
    makearef(group, objControlsState.keygroups.Sailing1Pers);
    for (int i = 0; i < GetAttributesNum(group); i++)
    {
        item = GetAttributeN(group, i);
        if (GetAttributeName(item) != name && GetAttributeValue(item) == keyName) key = -1;
    }
    // The reverse map also contains controls in other, mutually exclusive
    // groups. Only an ungrouped control is active alongside the deck group.
    string path = "map.keys.key_" + CI_GetKeyCode(keyName);
    if (CheckAttribute(&objControlsState, path))
    {
        string other = objControlsState.(path);
        bool grouped = false;
        makearef(groups, objControlsState.keygroups);
        for (i = 0; i < GetAttributesNum(groups); i++)
        {
            group = GetAttributeN(groups, i);
            if (CheckAttribute(group, other)) grouped = true;
        }
        if (other != name && !grouped) key = -1;
    }
    int flags = 0;
    if (key < 0) flags = -1;
    CI_CreateAndSetControls("Sailing1Pers", name, key, flags, true);
}

void SeaPhonk_InitControls()
{
    SeaPhonk_DeckCameraControl("Phonk_DeckCameraUp", "KEY_Z", false);
    SeaPhonk_DeckCameraControl("Phonk_DeckCameraDown", "KEY_X", false);
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Jet"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Jet", CI_GetKeyCode("VK_CONTROL"), 0, true);
        MapControlToGroup("Phonk_Jet", "Sailing1Pers");
        MapControlToGroup("Phonk_Jet", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Laser") ||
        objControlsState.keygroups.Sailing3Pers.Phonk_Laser == "KEY_6")
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Laser", CI_GetKeyCode("KEY_7"), 0, true);
        MapControlToGroup("Phonk_Laser", "Sailing1Pers");
        MapControlToGroup("Phonk_Laser", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Nuclear") ||
        objControlsState.keygroups.Sailing3Pers.Phonk_Nuclear == "KEY_7")
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Nuclear", CI_GetKeyCode("KEY_8"), 0, true);
        MapControlToGroup("Phonk_Nuclear", "Sailing1Pers");
        MapControlToGroup("Phonk_Nuclear", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_NuclearMenu"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_NuclearMenu", CI_GetKeyCode("KEY_K"), 0, true);
        MapControlToGroup("Phonk_NuclearMenu", "Sailing1Pers");
        MapControlToGroup("Phonk_NuclearMenu", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_NuclearMode"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_NuclearMode", CI_GetKeyCode("KEY_J"), 0, true);
        MapControlToGroup("Phonk_NuclearMode", "Sailing1Pers");
        MapControlToGroup("Phonk_NuclearMode", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_NuclearPower"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_NuclearPower", CI_GetKeyCode("KEY_L"), 0, true);
        MapControlToGroup("Phonk_NuclearPower", "Sailing1Pers");
        MapControlToGroup("Phonk_NuclearPower", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Rangefinder"))
    {
        int key = CI_GetKeyCode("VK_A_POINT");
        if (CheckAttribute(&objControlsState, "map.keys.key_186")) key = -1;
        aref group, control;
        string groupName;
        for (int g = 0; g < 3; g++)
        {
            if (g == 0) groupName = "Sailing3Pers";
            if (g == 1) groupName = "Sailing1Pers";
            if (g == 2) groupName = "BattleInterfaceControls";
            makearef(group, objControlsState.keygroups.(groupName));
            for (int i = 0; i < GetAttributesNum(group); i++)
            {
                control = GetAttributeN(group, i);
                if (GetAttributeValue(control) == "VK_A_POINT") key = -1;
            }
        }
        int state = 0;
        if (key < 0) state = -1;
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Rangefinder", key, state, true);
        MapControlToGroup("Phonk_Rangefinder", "Sailing1Pers");
        MapControlToGroup("Phonk_Rangefinder", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_NuclearInterval") ||
        objControlsState.keygroups.Sailing3Pers.Phonk_NuclearInterval == "KEY_I")
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_NuclearInterval", CI_GetKeyCode("KEY_U"), 0, true);
        MapControlToGroup("Phonk_NuclearInterval", "Sailing1Pers");
        MapControlToGroup("Phonk_NuclearInterval", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Harpoon") ||
        objControlsState.keygroups.Sailing3Pers.Phonk_Harpoon == "KEY_9")
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Harpoon", CI_GetKeyCode("KEY_0"), 0, true);
        MapControlToGroup("Phonk_Harpoon", "Sailing1Pers");
        MapControlToGroup("Phonk_Harpoon", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_HarpoonRelease"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_HarpoonRelease", CI_GetKeyCode("KEY_C"), 0, true);
        MapControlToGroup("Phonk_HarpoonRelease", "Sailing1Pers");
        MapControlToGroup("Phonk_HarpoonRelease", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_HarpoonWinch"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_HarpoonWinch", CI_GetKeyCode("KEY_V"), 0, true);
        MapControlToGroup("Phonk_HarpoonWinch", "Sailing1Pers");
        MapControlToGroup("Phonk_HarpoonWinch", "BattleInterfaceControls");
    }
}

bool SeaPhonk_IsActive()
{
    if (!bSeaActive || !IsEntity(&Sea) || IsEntity(&worldMap) || bAbordageStarted) return false;
    if (!CheckAttribute(&Sea, "Phonk.Active") || sti(Sea.Phonk.Active) == 0) return false;
    if (CheckAttribute(pchar, "Ship.HP") && stf(pchar.Ship.HP) <= 0.0) return false;
    return true;
}

bool SeaPhonk_ProtectsShip(aref character)
{
    return SeaPhonk_IsActive() && CheckAttribute(character, "index") &&
        sti(character.index) == GetMainCharacterIndex() && LAi_IsImmortal(character);
}

bool SeaPhonk_HasUnlimitedAmmo(aref character)
{
    // Keep virtual rounds virtual during death/sea teardown until Reset unloads.
    return CheckAttribute(&Sea, "Phonk.Active") && sti(Sea.Phonk.Active) != 0 &&
        CheckAttribute(character, "index") && sti(character.index) == GetMainCharacterIndex();
}

void SeaPhonk_SetActive(bool active)
{
    // Reload unloads synchronously, then loads on the next native Execute.
    // Return physical rounds before entry; discard virtual rounds before exit.
    if (bSeaLoaded && IsEntity(&AISea) && IsEntity(pchar) &&
        CheckAttribute(pchar, "Ship.Cannons.Charge.Type"))
        SendMessage(&AISea, "la", AI_MESSAGE_CANNON_RELOAD, pchar);
    Sea.Phonk.Active = active;
    if (active)
    {
        Sea.Phonk.AirDefenseType = 0;
        if (CheckAttribute(pchar, "SeaPhonk.AirDefenseType"))
            Sea.Phonk.AirDefenseType = SeaPhonk_PlayerDefenseType(sti(pchar.SeaPhonk.AirDefenseType));
        Sea.Phonk.AirDefensePerTarget = 2;
        if (CheckAttribute(pchar, "SeaPhonk.AirDefensePerTarget"))
            Sea.Phonk.AirDefensePerTarget = SeaPhonk_DefensePerTarget(sti(pchar.SeaPhonk.AirDefensePerTarget));
    }
    if (active && !CheckAttribute(&Sea, "Phonk.MissileType")) Sea.Phonk.MissileType = 3;
    if (active && !CheckAttribute(&Sea, "Phonk.MissileWarhead")) Sea.Phonk.MissileWarhead = 5;
    if (active && sti(Sea.Phonk.MissileWarhead) == 3) Sea.Phonk.MissileWarhead = 1;
    if (IsEntity(pchar)) pchar.SeaPhonk.Active = active;
}

bool SeaPhonk_IsMusicMuted()
{
    return !CheckAttribute(pchar, "PhonkMusicMuted") || sti(pchar.PhonkMusicMuted) != 0;
}

bool SeaPhonk_Music(string requested)
{
    if (!SeaPhonk_IsActive())
    {
        // Retire our loop before ordinary scene/death music replaces musicID.
        if (musicName == SEA_PHONK_TRACK) SeaPhonk_Reset(false);
        return false;
    }
    if (SeaPhonk_IsMusicMuted()) return false;
    if (requested != SEA_PHONK_TRACK && requested != "") Sea.Phonk.ReturnMusic = requested;
    if (musicName == SEA_PHONK_TRACK && musicID >= 0 &&
        SendMessage(Sound, "ll", MSG_SOUND_IS_PLAYING, musicID)) return true;
    KZ|MusicEventsOff();
    InitSound();
    if (oldMusicID >= 0) StopSound(oldMusicID, 0);
    if (musicID >= 0) StopSound(musicID, 200);
    oldMusicID = -1;
    oldMusicName = musicName;
    musicName = SEA_PHONK_TRACK;
    musicID = SendMessage(Sound, "lslllllllf", MSG_SOUND_PLAY, SEA_PHONK_TRACK,
        SOUND_MP3_STEREO, VOLUME_MUSIC, false, true, false, 0, 200, 1.0);
    return true;
}

bool SeaPhonk_StopMusic(int fade)
{
    if (musicName != SEA_PHONK_TRACK) return false;
    if (musicID >= 0) StopSound(musicID, fade);
    musicID = -1;
    musicName = "";
    return true;
}

void SeaPhonk_ResumeOrdinaryMusic(string previous)
{
    if (!bSeaActive || !IsEntity(&Sea)) return;
    if (previous == "" || previous == SEA_PHONK_TRACK)
    {
        previous = "music_sea_day";
        if (Whr_IsNight()) previous = "music_sea_night";
    }
    SetMusic(previous);
}

void SeaPhonk_Reset(bool restoreMusic)
{
    SeaPhonkTest_Stop();
    string previous = "";
    if (CheckAttribute(&Sea, "Phonk.ReturnMusic")) previous = Sea.Phonk.ReturnMusic;
    if (SeaPhonk_HasUnlimitedAmmo(pchar)) SeaPhonk_SetActive(false);
    DeleteAttribute(&Sea, "Phonk");
    if (IsEntity(pchar) && CheckAttribute(pchar, "Ship.ReactiveFlight")) DeleteAttribute(pchar, "Ship.ReactiveFlight");
    DelEventHandler("frame", "SeaPhonk_Frame");
    if (restoreMusic && IsEntity(pchar)) pchar.SeaPhonk.Active = false;
    if (!SeaPhonk_StopMusic(200)) return;
    if (restoreMusic) SeaPhonk_ResumeOrdinaryMusic(previous);
}

void SeaPhonk_ToggleMusic()
{
    // Character attributes survive sea teardown and use the existing SAVE codec.
    pchar.PhonkMusicMuted = !SeaPhonk_IsMusicMuted();
    if (!SeaPhonk_IsActive()) return;
    if (SeaPhonk_IsMusicMuted())
    {
        string previous = "";
        if (CheckAttribute(&Sea, "Phonk.ReturnMusic")) previous = Sea.Phonk.ReturnMusic;
        if (SeaPhonk_StopMusic(0)) SeaPhonk_ResumeOrdinaryMusic(previous);
    }
    else SeaPhonk_Music(musicName);
}

bool SeaPhonk_Toggle()
{
    if (!bSeaActive || !IsEntity(&Sea) || IsEntity(&worldMap) || bAbordageStarted) return false;
    if (SeaPhonk_IsActive())
    {
        SeaPhonk_Reset(true);
        return true;
    }
    if (CheckAttribute(pchar, "Ship.HP") && stf(pchar.Ship.HP) <= 0.0) return false;
    // Register on entry as well: controls restored from an older save lack 6/7.
    SeaPhonk_InitControls();
    SeaPhonk_SetActive(true);
    Sea.Phonk.Weapon = 6;
    Sea.Phonk.ReturnMusic = musicName;
    SetEventHandler("frame", "SeaPhonk_Frame", 0);
    SeaPhonk_Music(SEA_PHONK_TRACK);
    return true;
}

void SeaPhonk_Frame()
{
    if (!SeaPhonk_IsActive() || LAi_IsDead(pchar))
    {
        SeaPhonk_Reset(false);
        return;
    }
    if (bSeaReloadStarted || !CheckAttribute(pchar, "Ship.HP")) return;
    // The core supplies scaled game seconds: paused menus advance neither hull
    // repair nor the contact episode. Ignore excess stall time, not real frames.
    float seconds = fHighPrecisionDeltaTime;
    if (seconds <= 0.0) return;
    if (seconds > 0.25) seconds = 0.25;
    float hp = stf(pchar.Ship.HP);
    float maximum = GetCharacterShipHP(pchar);
    if (hp <= 0.0 || maximum <= 0.0) return;
    hp = hp + maximum * 0.02 * seconds;
    if (hp > maximum) hp = maximum;
    pchar.Ship.HP = hp;
}

float SeaPhonk_CollisionDamage(ref character, float damage)
{
    if (SeaPhonk_ProtectsShip(character)) return 0.0;
    return damage;
}

bool SeaPhonk_ValidShip(aref target)
{
    if (!CheckAttribute(target, "index") || !CheckAttribute(target, "Ship.HP") ||
        !CheckAttribute(target, "Ship.Type")) return false;
    int index = sti(target.index);
    int type = sti(target.Ship.Type);
    return index >= 0 && index < TOTAL_CHARACTERS && type >= 0 && type < REAL_SHIPS_QUANTITY;
}

bool SeaPhonk_LiveShip(aref target)
{
    if (!SeaPhonk_ValidShip(target)) return false;
    return stf(target.Ship.HP) > 0.0 && !LAi_IsDead(target);
}

int SeaPhonk_HullCut()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    int fatal = GetEventData();
    int applyPartialDamage = GetEventData();
    return SeaPhonk_ApplyHullCut(target, owner, fraction, fatal, applyPartialDamage, false, 0);
}

int SeaPhonk_NuclearHullCut()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    int fatal = GetEventData();
    int applyPartialDamage = GetEventData();
    int launchEpoch = GetEventData();
    return SeaPhonk_ApplyHullCut(target, owner, fraction, fatal, applyPartialDamage, true, launchEpoch);
}

int SeaPhonk_ApplyHullCut(ref target, int owner, float fraction, int fatal, int applyPartialDamage, bool nuclear, int launchEpoch)
{
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return 0;
    // Invalid, removed and already-dead ships settle a native reconciliation.
    if (owner < -1 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return 1;
    // An existing fatal cut reconciles only after protection is disabled.
    if (SeaPhonk_ProtectsShip(target)) return 0;
    bool betrayal = false;
    if (nuclear && owner >= 0 && sti(target.index) != owner && !LAi_IsImmortal(target))
    {
        betrayal = Ship_SurrenderHit(target, owner, launchEpoch);
        SeaPhonk_RecordMissileAttack(target, owner);
    }
    if (fatal > 0)
    {
        int cause = KILL_BY_TOUCH;
        if (owner == -1) cause = KILL_BY_ISLAND;
        if (applyPartialDamage > 0) cause = KILL_BY_BALL;
        if (nuclear && applyPartialDamage > 0) Ship_ApplyProjectileStructuralHullFailure(target, owner, betrayal);
        else Ship_ApplyStructuralHullFailure(target, cause, owner);
        return 1;
    }
    // TOUCH already paid mechanical damage; restored cuts never replay it.
    if (applyPartialDamage <= 0 || sti(target.index) == owner) return 1;
    float multiplier = Ship_HullDamageMultiplier(target, owner);
    if (multiplier <= 0.0) return 1;
    float damage = GetCharacterShipHP(target) * Clampf(fraction);
    float limit = stf(target.Ship.HP) * 0.5 / multiplier;
    if (damage > limit) damage = limit;
    if (nuclear) Ship_ApplyProjectileHullHitpoints(target, damage, owner, betrayal);
    else Ship_ApplyHullHitpoints(target, damage, KILL_BY_BALL, owner);
    return 1;
}

float SeaPhonk_NuclearPower()
{
    return SeaTsunami_SampleSeverity();
}

void SeaPhonk_ClusterFlame()
{
    float x = GetEventData();
    float y = GetEventData();
    float z = GetEventData();
    int surface = GetEventData();
    // Use existing, bounded particle systems; keep water impacts distinct from
    // dust and fire on land or ship surfaces.
    Ball_CreateNavalEffect("blast_inv", x, y, z, 0.0, 0.0, 0.0, 0, 10);
    // Ship fire is attached to the hull by the native fire system. A static
    // world-space emitter would hang in the air as the ship moves away.
    if (surface == 1)
    {
        Ball_CreateNavalEffect("large_smoke", x, y, z, 0.0, 0.0, 0.0, 0, 10);
        Ball_CreateNavalEffect("FortFire", x, y, z, 0.0, 0.0, 0.0, 5, 10);
        Ball_CreateNavalEffect("GunFire", x, y, z, 0.0, 0.0, 0.0, 0, 10);
    }
}

void SeaPhonk_MissilePuff()
{
    float x = GetEventData();
    float y = GetEventData();
    float z = GetEventData();
    // The native launch burst already supplies short-lived smoke. The ship
    // fire emitter has a ten-second particle lifetime and leaves a second,
    // stationary plume at the launch position.
}

void SeaPhonk_RecordMissileAttack(ref target, int owner)
{
    // Match cannon hits: notify the encounter before damage can sink the ship.
    if (owner != GetMainCharacterIndex() || IsCompanion(target) || CheckAttribute(target, "Coastal_Captain")) return;
    if (GetNationRelation2MainCharacter(sti(target.nation)) == RELATION_ENEMY) return;
    if (WdmMilitaryParticipationHit(target, Crime_HasPlayerIntent(target))) return;
    ref commander = CrimeSea_GetGroupCommander(target);
    int targetRelation = GetRelation(sti(target.index), owner);
    int commanderRelation = GetRelation(sti(commander.index), owner);
    bool tracked = CrimeSea_IsTrackedAmbient(commander, target);
    CrimeSea_PreparePlayerBallHit(target);
    // An incident already tracking the group records a new victim but does not
    // itself change that victim's relation. Keep quest protections in the owner.
    if (tracked && targetRelation == RELATION_NEUTRAL &&
        GetRelation(sti(target.index), owner) == RELATION_NEUTRAL && !DebugAI_ProtectRelations())
    {
        CrimeSea_AmbientAttack(commander, target);
        if (GetRelation(sti(target.index), owner) == RELATION_ENEMY &&
            CrimeSea_IsTrackedShip(target) && CheckAttribute(target, "SeaAI.Group.Name"))
            Group_SetEnemyToCharacter(target.SeaAI.Group.Name, owner);
    }
    // Script relations and the native sea-AI matrix are separate owners.
    if (GetRelation(sti(target.index), owner) != targetRelation ||
        GetRelation(sti(commander.index), owner) != commanderRelation) UpdateRelations();
}

void SeaPhonk_MissileAttack()
{
    aref target = GetEventData();
    int owner = GetEventData();
    int launchEpoch = GetEventData();
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return;
    if (owner < 0 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return;
    if (sti(target.index) == owner || SeaPhonk_ProtectsShip(target) || LAi_IsImmortal(target)) return;
    Ship_SurrenderHit(target, owner, launchEpoch);
    SeaPhonk_RecordMissileAttack(target, owner);
}

void SeaPhonk_ConventionalDamage()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float power = GetEventData();
    float bombMultiplier = GetEventData();
    int warhead = GetEventData();
    int launchEpoch = GetEventData();
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return;
    if (owner < 0 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return;
    if (sti(target.index) == owner || SeaPhonk_ProtectsShip(target) || LAi_IsImmortal(target)) return;
    if (power <= 0.0 || bombMultiplier <= 0.0) return;
    if (Ship_HullDamageMultiplier(target, owner) <= 0.0) return;
    bool surrenderBetrayal = Ship_SurrenderHit(target, owner, launchEpoch);
    target.Ship.LastBallCharacter = owner;
    SeaPhonk_RecordMissileAttack(target, owner);
    // HE and legacy fragmentation both use the fragmentation crew effect.
    // Crew and rigging scale with blast falloff and configured warhead power.
    float crewPercent = 6.0;
    if (warhead == 1 || warhead == 3) crewPercent = 15.0;
    if (warhead == 4) crewPercent = 4.0;
    if (warhead == PHONK_WARHEAD_THERMOBARIC) crewPercent = 22.0;
    float effectPower = Clampf(power) * bombMultiplier / 48.0;
    Ship_ApplyCrewHitpoints(target, Ship_AmmoCrewLoss(target, crewPercent * effectPower));
    float sailDamage = Ship_SailHitDamageToSP(target, stf(Goods[GOOD_BOMBS].DamageRig) * bombMultiplier * Clampf(power));
    if (sailDamage > 0.0) MakeSailDmg(sti(target.index), sailDamage);
    // GOOD_BOMBS owns the reference HP; ordinary hull perks and kill attribution
    // remain in Ship_ApplyHullHitpoints instead of being reproduced here.
    Ship_ApplyProjectileHullHitpoints(target, stf(Goods[GOOD_BOMBS].DamageHull) * bombMultiplier * Clampf(power), owner, surrenderBetrayal);
}

void SeaPhonk_FragmentHit()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float damage = GetEventData();
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return;
    if (owner < 0 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return;
    if (sti(target.index) == owner || SeaPhonk_ProtectsShip(target) || LAi_IsImmortal(target) || damage <= 0.0) return;
    if (Ship_HullDamageMultiplier(target, owner) <= 0.0) return;
    target.Ship.LastBallCharacter = owner;
    SeaPhonk_RecordMissileAttack(target, owner);
    Ship_ApplyHullHitpoints(target, damage, KILL_BY_BALL, owner);
}

int SeaPhonk_NuclearWave()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    return SeaPhonk_ApplyNuclearWave(target, owner, fraction, false, 0);
}

int SeaPhonk_NuclearPressure()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    int launchEpoch = GetEventData();
    return SeaPhonk_ApplyNuclearWave(target, owner, fraction, true, launchEpoch);
}

int SeaPhonk_ApplyNuclearWave(ref target, int owner, float fraction, bool hasEpoch, int launchEpoch)
{
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return 0;
    if (owner < 0 || owner >= TOTAL_CHARACTERS || !SeaPhonk_ValidShip(target)) return 0;
    if (sti(target.index) == owner || SeaPhonk_ProtectsShip(target) || LAi_IsImmortal(target)) return 0;
    // A wreck can still receive physical pressure and heat; it has no HP or
    // death/reward settlement left to debit.
    if (!SeaPhonk_LiveShip(target)) return 1;
    if (Ship_HullDamageMultiplier(target, owner) <= 0.0) return 0;
    // The inner blast already paid its HP. The travelling shell only pays
    // outer hull damage; native thermal fire retains the sole burning DOT.
    if (fraction > 0.0)
    {
        bool betrayal = false;
        if (hasEpoch) betrayal = Ship_SurrenderHit(target, owner, launchEpoch);
        SeaPhonk_RecordMissileAttack(target, owner);
        Ship_ApplyProjectileHullHitpoints(target, GetCharacterShipHP(target) * Clampf(fraction), owner, betrayal);
    }
    // Damage can remove the entity/attributes; native reacquires its ID before
    // applying movement or ignition, so do not read this aref after the callback.
    return 1;
}

void SeaPhonk_Flood()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    // Native seams own cumulative ingress and its save/load projection. The sea
    // handler survives mode exit; no new water enters during a paused frame.
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return;
    if (owner < -1 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return;
    if (SeaPhonk_ProtectsShip(target)) return;
    float maximum = GetCharacterShipHP(target);
    if (maximum <= 0.0) return;
    fraction = Clampf(fraction);
    float damage = stf(target.Ship.HP) - maximum * (1.0 - fraction);
    float multiplier = Ship_HullDamageMultiplier(target, owner);
    if (fraction >= 1.0) damage = stf(target.Ship.HP) * 10.0 * multiplier;
    // Combat perks cannot leave hull above the physical flooding ceiling.
    Ship_ApplyHullHitpoints(target, damage / multiplier, KILL_BY_BALL, owner);
}

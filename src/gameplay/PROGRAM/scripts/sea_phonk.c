// Local-sea mode: one transient scene flag drives ship physics, lighting and music.
string SEA_PHONK_TRACK = "MUSIC\\phonk-drive.mp3";

void SeaPhonk_InitControls()
{
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Laser"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Laser", CI_GetKeyCode("KEY_6"), 0, true);
        MapControlToGroup("Phonk_Laser", "Sailing1Pers");
        MapControlToGroup("Phonk_Laser", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Nuclear"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Nuclear", CI_GetKeyCode("KEY_7"), 0, true);
        MapControlToGroup("Phonk_Nuclear", "Sailing1Pers");
        MapControlToGroup("Phonk_Nuclear", "BattleInterfaceControls");
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
        sti(character.index) == GetMainCharacterIndex();
}

bool SeaPhonk_Music(string requested)
{
    if (!SeaPhonk_IsActive())
    {
        // Retire our loop before ordinary scene/death music replaces musicID.
        if (musicName == SEA_PHONK_TRACK) SeaPhonk_Reset(false);
        return false;
    }
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

void SeaPhonk_Reset(bool restoreMusic)
{
    string previous = "";
    if (CheckAttribute(&Sea, "Phonk.ReturnMusic")) previous = Sea.Phonk.ReturnMusic;
    DeleteAttribute(&Sea, "Phonk");
    DelEventHandler("frame", "SeaPhonk_Frame");
    if (musicName != SEA_PHONK_TRACK) return;
    StopSound(musicID, 200);
    musicID = -1;
    musicName = "";
    if (restoreMusic && bSeaActive && IsEntity(&Sea))
    {
        if (previous == "" || previous == SEA_PHONK_TRACK)
        {
            previous = "music_sea_day";
            if (Whr_IsNight()) previous = "music_sea_night";
        }
        SetMusic(previous);
    }
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
    Sea.Phonk.Active = 1;
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

bool SeaPhonk_LiveShip(aref target)
{
    if (!CheckAttribute(target, "index") || !CheckAttribute(target, "Ship.HP") ||
        !CheckAttribute(target, "Ship.Type")) return false;
    int index = sti(target.index);
    int type = sti(target.Ship.Type);
    return index >= 0 && index < TOTAL_CHARACTERS && type >= 0 && type < REAL_SHIPS_QUANTITY &&
        stf(target.Ship.HP) > 0.0 && !LAi_IsDead(target);
}

int SeaPhonk_HullCut()
{
    aref target = GetEventData();
    int owner = GetEventData();
    float fraction = GetEventData();
    int fatal = GetEventData();
    int applyPartialDamage = GetEventData();
    if (!bSeaActive || !IsEntity(&Sea) || bSeaReloadStarted || fHighPrecisionDeltaTime <= 0.0) return 0;
    // Invalid, removed and already-dead ships settle a native reconciliation.
    if (owner < -1 || owner >= TOTAL_CHARACTERS || !SeaPhonk_LiveShip(target)) return 1;
    // An existing fatal cut reconciles only after protection is disabled.
    if (SeaPhonk_ProtectsShip(target)) return 0;
    if (fatal > 0)
    {
        int cause = KILL_BY_TOUCH;
        if (owner == -1) cause = KILL_BY_ISLAND;
        if (applyPartialDamage > 0) cause = KILL_BY_BALL;
        Ship_ApplyStructuralHullFailure(target, cause, owner);
        return 1;
    }
    // TOUCH already paid mechanical damage; restored cuts never replay it.
    if (applyPartialDamage <= 0 || sti(target.index) == owner) return 1;
    float multiplier = Ship_HullDamageMultiplier(target, owner);
    if (multiplier <= 0.0) return 1;
    float damage = GetCharacterShipHP(target) * Clampf(fraction);
    float limit = stf(target.Ship.HP) * 0.5 / multiplier;
    if (damage > limit) damage = limit;
    Ship_ApplyHullHitpoints(target, damage, KILL_BY_BALL, owner);
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

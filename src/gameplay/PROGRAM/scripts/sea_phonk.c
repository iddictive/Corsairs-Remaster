// Local-sea mode: one transient scene flag drives ship physics, lighting and music.
string SEA_PHONK_TRACK = "MUSIC\\phonk-drive.mp3";

bool SeaPhonk_IsActive()
{
    if (!bSeaActive || !IsEntity(&Sea) || IsEntity(&worldMap) || bAbordageStarted) return false;
    if (!CheckAttribute(&Sea, "Phonk.Active") || sti(Sea.Phonk.Active) == 0) return false;
    if (CheckAttribute(pchar, "Ship.HP") && stf(pchar.Ship.HP) <= 0.0) return false;
    return true;
}

bool SeaPhonk_Music(string requested)
{
    if (!SeaPhonk_IsActive()) return false;
    if (requested != SEA_PHONK_TRACK && requested != "") Sea.Phonk.ReturnMusic = requested;
    if (musicName == SEA_PHONK_TRACK) return true;
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
    Sea.Phonk.Active = 1;
    Sea.Phonk.ReturnMusic = musicName;
    SetEventHandler("frame", "SeaPhonk_Frame", 0);
    SeaPhonk_Music(SEA_PHONK_TRACK);
    return true;
}

void SeaPhonk_Frame()
{
    if (!SeaPhonk_IsActive()) SeaPhonk_Reset(false);
}

// Direct port interfaces share the ordinary town services, stock and shipyard.
object SeaPortService;

void SeaPortService_Reset()
{
    DelEventHandler("frame", "SeaPortService_Closed");
    DeleteAttribute(&SeaPortService, "");
}

bool SeaPortService_Allowed()
{
    return !LAi_boarding_process && !LAi_grp_alarmactive && !chrDisableReloadToLocation &&
        !bDisableFastReload && IsEnableFastTravel() && !LAi_IsDead(pchar) &&
        !sti(InterfaceStates.Launched) && !dialogRun;
}

string SeaPortService_Berth()
{
    if (!bSeaActive || !bCanEnterToLand || bSeaReloadStarted || bDisableMapEnter ||
        !SeaPortService_Allowed() || !Sea_LandColonyAccess()) return "";
    int island = FindIsland(sIslandID);
    if (island < 0 || !Island_isReloadEnableGlobal(sIslandID) ||
        !Island_isReloadEnableLocal(sIslandID, sIslandLocator)) return "";
    aref entries; makearef(entries, Islands[island].reload);
    for (int i = 0; i < GetAttributesNum(entries); i++)
    {
        aref entry = GetAttributeN(entries, i);
        if (entry.name == sIslandLocator && CheckAttribute(entry, "go")) return entry.go;
    }
    return "";
}

int SeaPortService_Merchant(string berth, int service)
{
    if (service < 0 || service > 1) return -1;
    int location = FindLocation(berth);
    if (location < 0 || !CheckAttribute(&Locations[location], "fastreload")) return -1;
    string town = Locations[location].fastreload;
    int colony = FindColony(town);
    if (colony < 0 || !CheckAttribute(&Colonies[colony], "nation")) return -1;
    if (service == 0 && !CheckAttribute(&Colonies[colony], "StoreNum")) return -1;
    if (CheckAttribute(&Colonies[colony], "trafficHarbour.held") && sti(Colonies[colony].trafficHarbour.held)) return -1;
    if (Colonies[colony].nation != "none")
    {
        int nation = sti(Colonies[colony].nation);
        if (nation != PIRATE && (GetNationRelation2MainCharacter(nation) == RELATION_ENEMY ||
            GetRelation2BaseNation(nation) == RELATION_ENEMY)) return -1;
    }
    string suffix = "_trader";
    if (service == 1) suffix = "_shipyarder";
    int index = GetCharacterIndex(town + suffix);
    if (index < 0) return -1;
    ref merchant = GetCharacter(index);
    if (LAi_IsDead(merchant) || !CheckAttribute(merchant, "location") ||
        !CheckAttribute(merchant, "City") || merchant.City != town) return -1;
    if (CheckAttribute(merchant, "chr_ai.disableDlg") && sti(merchant.chr_ai.disableDlg)) return -1;
    int merchantLocation = FindLocation(merchant.location);
    if (merchantLocation < 0) return -1;
    if (CheckAttribute(merchant, "location.stime") && CheckAttribute(merchant, "location.etime") &&
        !LAi_login_CheckTime(stf(merchant.location.stime), stf(merchant.location.etime))) return -1;
    // The existing port menu owns which services this town actually offers.
    if (!CheckAttribute(&objFastReloadTable, "table." + town)) return -1;
    aref services; makearef(services, objFastReloadTable.table.(town));
    for (int i = 0; i < GetAttributesNum(services); i++)
    {
        aref item = GetAttributeN(services, i);
        if (sti(item.pic) == service && sti(item.tex) == 0 &&
            FindLocation(item.location) == merchantLocation && CheckFastJump(berth, item.location)) return index;
    }
    return -1;
}

void SeaPortService_Refresh(int mainIndex, int selectedIndex)
{
    BattleInterface.Commands.PortStore.enable = false;
    BattleInterface.Commands.PortShipyard.enable = false;
    if (mainIndex != selectedIndex) return;
    string berth = SeaPortService_Berth();
    if (berth == "") return;
    BattleInterface.Commands.PortStore.enable = SeaPortService_Merchant(berth, 0) >= 0;
    BattleInterface.Commands.PortShipyard.enable = SeaPortService_Merchant(berth, 1) >= 0;
}

void SeaPortService_Start(int service)
{
    string berth = SeaPortService_Berth();
    if (berth == "") return;
    int merchant = SeaPortService_Merchant(berth, service);
    if (merchant < 0) return;
    int colony = FindColony(Locations[FindLocation(berth)].fastreload);
    if (service == 0)
    {
        if (!CheckAttribute(&Colonies[colony], "StoreNum")) return;
        pchar.PriceList.StoreManIdx = Colonies[colony].index;
        LaunchStore(sti(Colonies[colony].StoreNum));
    }
    else
    {
        DeleteAttribute(&SeaPortService, "");
        SeaPortService.berth = berth;
        SeaPortService.x = pchar.Ship.Pos.x;
        SeaPortService.z = pchar.Ship.Pos.z;
        SeaPortService.ay = pchar.Ship.Ang.y;
        for (int slot = 0; slot < COMPANION_MAX; slot++)
        {
            int index = GetCompanionIndex(pchar, slot);
            if (index < 0 || !IsEntity(&Characters[index])) continue;
            ref captain = GetCharacter(index);
            string key = "c" + index;
            SeaPortService.hulls.(key).index = index;
            SeaPortService.hulls.(key).type = captain.Ship.Type;
            SeaPortService.hulls.(key).hp = captain.Ship.HP;
            SeaPortService.hulls.(key).sp = captain.Ship.SP;
            SeaPortService.hulls.(key).cannons = captain.Ship.Cannons.Type;
            SeaPortService.hulls.(key).charge = captain.Ship.Cannons.Charge.Type;
            if (CheckAttribute(captain, "Ship.Sounds"))
            {
                aref sounds; makearef(sounds, captain.Ship.Sounds);
                aref savedSounds; makearef(savedSounds, SeaPortService.hulls.(key).sounds);
                CopyAttributes(savedSounds, sounds);
            }
        }
        if (CheckAttribute(pchar, "Ship.Autopilot"))
        {
            aref observer; makearef(observer, pchar.Ship.Autopilot);
            aref saved; makearef(saved, SeaPortService.autopilot);
            CopyAttributes(saved, observer);
        }
        LaunchShipyard(&Characters[merchant]);
        if (!sti(InterfaceStates.Launched)) DeleteAttribute(&SeaPortService, "");
    }
}

bool SeaPortService_InFleet(int index)
{
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        if (GetCompanionIndex(pchar, slot) == index) return true;
    }
    return false;
}

void SeaPortService_ForgetShip(int index)
{
    for (int i = 0; i < iNumShips; i++)
    {
        if (Ships[i] != index) continue;
        for (int j = i + 1; j < iNumShips; j++) { Ships[j - 1] = Ships[j]; }
        iNumShips--;
        break;
    }
    Group_DelCharacter(PLAYER_GROUP, Characters[index].id);
}

// Called by the ordinary interface exit owner, after the UI releases its state.
void SeaPortService_Closed()
{
    if (!CheckAttribute(&SeaPortService, "berth")) return;
    if (sti(InterfaceStates.Launched)) return;
    // Repairs retain the normal date-advance fade. Reconcile after that fader,
    // before resuming hull actions or starting a shipless return to shore.
    aref fader;
    if (GetEntity(fader, "fader"))
    {
        if (!CheckAttribute(&SeaPortService, "closing"))
        {
            SeaPortService.closing = true;
            LayerFreeze(SEA_EXECUTE, true);
            SetEventHandler("frame", "SeaPortService_Closed", 1);
        }
        return;
    }
    DelEventHandler("frame", "SeaPortService_Closed");
    object request;
    CopyAttributes(&request, &SeaPortService);
    DeleteAttribute(&SeaPortService, "");
    if (!bSeaActive || bAbordageStarted) return;
    if (CheckAttribute(&request, "closing")) LayerFreeze(SEA_EXECUTE, false);
    bool shore = GetCharacterShipType(pchar) == SHIP_NOTUSED;
    bool failed = false;
    int index;
    object retired;
    if (CheckAttribute(&request, "hulls"))
    {
        aref hulls; makearef(hulls, request.hulls);
        for (int i = 0; i < GetAttributesNum(hulls); i++)
        {
            aref before = GetAttributeN(hulls, i);
            index = sti(before.index);
            ref captain = GetCharacter(index);
            int type = GetCharacterShipType(captain);
            if (!SeaPortService_InFleet(index) || type == SHIP_NOTUSED)
            {
                if (index == GetMainCharacterIndex()) continue;
                if (!SendMessage(&AISea, "laal", AI_MESSAGE_REFRESH_SHIP, captain, &retired, sti(before.charge))) failed = true;
                else
                {
                    SeaPortService_ReleaseSounds(before);
                    DeleteAttribute(captain, "Ship.Sounds");
                    SeaPortService_ForgetShip(index);
                }
                continue;
            }
            if (type == sti(before.type) && stf(captain.Ship.HP) == stf(before.hp) &&
                stf(captain.Ship.SP) == stf(before.sp) && captain.Ship.Cannons.Type == before.cannons) continue;
            CharacterUpdateShipFromBaseShip(index);
            Event("eSwitchPerks", "l", index);
            if (!SendMessage(&AISea, "laal", AI_MESSAGE_REFRESH_SHIP, captain, &RealShips[type], sti(before.charge))) failed = true;
            else
            {
                SeaPortService_ReleaseSounds(before);
                Event(SHIP_UPDATE_PARAMETERS, "lf", index, Ship_GetSailState(captain));
                Ship_CreateStaticSounds(captain);
            }
        }
    }
    if (!shore && !failed)
    {
        // Ordinary admission owns safe start-position checks for a newly bought
        // empty-slot companion; the retained ships are never relocated.
        bool spawnOriginSet = false;
        for (int slot = 1; slot < COMPANION_MAX; slot++)
        {
            index = GetCompanionIndex(pchar, slot);
            if (index < 0 || IsEntity(&Characters[index]) || GetCharacterShipType(&Characters[index]) == SHIP_NOTUSED) continue;
            if (!spawnOriginSet)
            {
                Group_SetXZ_AY(PLAYER_GROUP, stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z), stf(pchar.Ship.Ang.y));
                spawnOriginSet = true;
            }
            Group_AddCharacter(PLAYER_GROUP, Characters[index].id);
            Ship_Add2Sea(index, true, "", false);
            Ship_SetTaskDefend(PRIMARY_TASK, index, GetMainCharacterIndex());
        }
        if (!CheckAttribute(pchar, "Ship.Autopilot") && CheckAttribute(&request, "autopilot"))
        {
            aref saved; makearef(saved, request.autopilot);
            aref observer; makearef(observer, pchar.Ship.Autopilot);
            CopyAttributes(observer, saved);
        }
        SeaCameras_UpdateCamera();
        UpdateRelations();
        RefreshBattleInterface();
        SeaAutopilot_Bind();
        return;
    }
    if (failed) Log_Info("Не удалось обновить корабль в море. Возвращаемся к причалу.");
    // Selling the final main hull leaves the captain at the actual local quay.
    // The normal landing path unloads the old scene and binds the remaining fleet.
    pchar.Ship.Pos.x = request.x;
    pchar.Ship.Pos.z = request.z;
    string mainKey = "c" + GetMainCharacterIndex();
    if (CheckAttribute(&request, "hulls." + mainKey + ".sounds") && !CheckAttribute(pchar, "Ship.Sounds"))
    {
        aref oldSounds; makearef(oldSounds, request.hulls.(mainKey).sounds);
        aref currentSounds; makearef(currentSounds, pchar.Ship.Sounds);
        CopyAttributes(currentSounds, oldSounds);
    }
    Sea_LandLoad();
}

void SeaPortService_ReleaseSounds(aref before)
{
    if (!CheckAttribute(before, "sounds")) return;
    aref sounds; makearef(sounds, before.sounds);
    Ship_ReleaseStaticSounds(sounds);
}

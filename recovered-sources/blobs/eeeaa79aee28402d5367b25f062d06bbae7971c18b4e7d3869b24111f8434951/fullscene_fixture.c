// Private case input, appended to an otherwise complete canonical seadogs.c.
// Never loaded by the installed app. Uses real initialization and gameplay owners.
object FullScene;

void FullScene_Bootstrap()
{
    FullScene.phase = "bootstrap";
    CreateEntity(&LanguageObject, "obj_strservice");
    SetMainCharacterIndex(1);
    screenscaling = BI_COMPARE_HEIGHT;
    ControlsInit(GetTargetPlatform(), false);
    Main_InitGame();
    InitGame();
    CreateColonyCommanders();
    SetNames();
    Environment.date.hour = worldMap.date.hour;
    Environment.date.min = worldMap.date.min;
    Environment.date.sec = worldMap.date.sec;
    Environment.date.year = worldMap.date.year;
    Environment.date.month = worldMap.date.month;
    Environment.date.day = worldMap.date.day;
    Environment.time = 11.0;
    // Ordinary character-select input; no player profile or SAVE is read.
    startHeroType = 3;
    NullCharacter.HeroParam.HeroType = "Merchant";
    NullCharacter.HeroParam.nation = SPAIN;
    initNewMainCharacter();
    InitPsHeros();
    RumourInit();
    ActivateTimeEvents();
    InitInteractiveInterface();
    InterfaceStates.Launched = false;
    InterfaceStates.Buttons.Resume.enable = true;
    InterfaceStates.Buttons.Save.enable = true;
    InterfaceStates.Buttons.Load.enable = true;
    // This is private initial state, before the measured journey starts.
    // All later movement/HP/money/cannon results must come from their real owners.
    ref companion = GetCharacter(NPC_GenerateCharacter("FullSceneCompanion", "officer_1", "man", "man", 5, SPAIN, -1, false));
    companion.Ship.Type = GenerateShipExt(SHIP_LUGGER, 0, companion);
    SetBaseShipData(companion);
    SetCrewQuantity(companion, GetMinCrewQuantity(companion));
    SetCompanionIndex(pchar, -1, sti(companion.index));
    FullScene.companionIndex = companion.index;
    FullScene.mainIndex = GetMainCharacterIndex();
    FullScene.phase = "initialized";
    SetEventHandler("FullSceneAction", "FullScene_Action", 0);
    SetEventHandler("FullSceneObserve", "FullScene_Observe", 0);
    SetEventHandler("CharacterUpdate", "FullScene_WitnessUpdate", 0);
    SetEventHandler(CANNON_FIRE, "FullScene_WitnessCannonFire", 0);
    FullScene.playerDischarges = 0;
    FullScene.fortDischarges = 0;
}

void FullScene_Action()
{
    string action = FullScene.action;
    string target = FullScene.target;
    string argument = FullScene.argument;
    FullScene.result = 0;
    if (action == "sea")
    {
        object login;
        login.Island = "PuertoRico";
        login.PlayerGroup.x = 1110.0;
        login.PlayerGroup.y = 0.0;
        login.PlayerGroup.z = 745.0;
        login.PlayerGroup.ay = 0.0;
        SeaLogin(&login);
        FullScene.result = 1;
    }
    if (action == "deck")
    {
        SeaCameras.Camera = "SeaDeckCamera";
        SeaDeckCamera.ThirdPerson = 1;
        SeaCameras_UpdateCamera();
        FullScene.result = 1;
    }
    if (action == "embark") FullScene.result = Port_EnterBoat();
    if (action == "disembark")
    {
        Port_TryLeaveBoat();
        FullScene.result = 1;
    }
    if (action == "select-pier") FullScene.result = Port_SelectPier(stf(pchar.PortTravel.x), stf(pchar.PortTravel.z));
    if (action == "walk")
    {
        if (!Port_IsLandHero()) return;
        LAi_SetActorType(pchar);
        LAi_ActorGoToLocator(pchar, target, argument, "", 90.0);
        FullScene.result = 1;
    }
    if (action == "player")
    {
        LAi_SetPlayerType(pchar);
        FullScene.result = 1;
    }
    if (action == "reload")
    {
        if (!Port_IsLandHero()) return;
        if (!CheckAttribute(loadedLocation, "locators.reload." + target)) return;
        aref locator;
        makearef(locator, loadedLocation.locators.reload.(target));
        float x, y, z;
        if (!GetCharacterPos(pchar, &x, &y, &z)) return;
        // Reload's caller normally owns proximity admission. Preserve it here.
        if (GetDistance2D(x, z, stf(locator.x), stf(locator.z)) > 1.8) return;
        aref reloads;
        makearef(reloads, loadedLocation.reload);
        FullScene.result = Reload(reloads, target, loadedLocation.id);
    }
    if (action == "dialogue")
    {
        int index = GetCharacterIndex(target);
        if (index < 0) return;
        FullScene.result = DialogMain(GetCharacter(index));
    }
    if (action == "follow-npc")
    {
        int following = GetCharacterIndex(target);
        if (!Port_IsLandHero() || following < 0 || !IsEntity(&Characters[following])) return;
        LAi_SetActorType(pchar);
        LAi_ActorFollow(pchar, &Characters[following], "", 90.0);
        FullScene.result = 1;
    }
    if (action == "select-guard")
    {
        for (int guardIndex = 0; guardIndex < TOTAL_CHARACTERS; guardIndex++)
        {
            ref guard = GetCharacter(guardIndex);
            if (!IsEntity(guard) || !CheckAttribute(guard, "location") || guard.location != "SanJuan_town") continue;
            if (!CheckAttribute(guard, "chr_ai.type")) continue;
            if (guard.chr_ai.type != "patrol" && guard.chr_ai.type != "guardian") continue;
            if (LAi_IsDead(guard) || CharacterCombat_ProtectionReason(guard) != "") continue;
            FullScene.guardId = guard.id;
            FullScene.guardIndex = guardIndex;
            FullScene.result = 1;
            break;
        }
    }
    if (action == "link")
    {
        // Select only a link actually offered by the original dialogue.
        if (!DialogRun || !CheckAttribute(&Dialog, "Links." + target + ".go")) return;
        Dialog.CurrentNode = Dialog.Links.(target).go;
        Event("DialogEvent");
        FullScene.result = 1;
    }
    if (action == "link-go")
    {
        if (!DialogRun || !CheckAttribute(&Dialog, "Links")) return;
        aref links;
        makearef(links, Dialog.Links);
        for (int choice = 0; choice < GetAttributesNum(links); choice++)
        {
            aref offered = GetAttributeN(links, choice);
            if (!CheckAttribute(offered, "go") || offered.go != target) continue;
            Dialog.CurrentNode = offered.go;
            Event("DialogEvent");
            FullScene.result = 1;
            break;
        }
    }
    if (action == "dialogue-exit")
    {
        DialogCancel_Exit();
        FullScene.result = !DialogRun;
    }
    if (action == "store-buy")
    {
        if (!sti(InterfaceStates.Launched)) return;
        // Select an existing table row as UI input, then use the store's own
        // quantity, price, stock, capacity and money transaction handlers.
        GameInterface.TABLE_LIST.select = sti(target);
        Event("TableSelectChange", "sl", "TABLE_LIST", sti(target));
        Event("ShowItemInfo");
        Event("ADD_BUTTON");
        Event("TransactionOK");
        FullScene.result = 1;
    }
    if (action == "buy-food")
    {
        if (!sti(InterfaceStates.Launched) || !CheckAttribute(&GameInterface, "TABLE_LIST")) return;
        aref table;
        makearef(table, GameInterface.TABLE_LIST);
        int row = -1;
        for (int entry = 0; entry < GetAttributesNum(table); entry++)
        {
            aref tableRow = GetAttributeN(table, entry);
            if (!CheckAttribute(tableRow, "index") || sti(tableRow.index) != GOOD_FOOD) continue;
            string rowName = GetAttributeName(tableRow);
            if (findsubstr(rowName, "tr", 0) != 0) continue;
            row = sti(strcut(rowName, 2, strlen(rowName) - 1));
            break;
        }
        if (row < 0) return;
        int beforeMoney = sti(pchar.money);
        int beforeFood = GetCargoGoods(pchar, GOOD_FOOD);
        GameInterface.TABLE_LIST.select = row;
        Event("TableSelectChange", "sl", "TABLE_LIST", row);
        Event("ShowItemInfo");
        Event("ADD_BUTTON");
        Event("TransactionOK");
        FullScene.transactionMoneyDelta = sti(pchar.money) - beforeMoney;
        FullScene.transactionFoodDelta = GetCargoGoods(pchar, GOOD_FOOD) - beforeFood;
        FullScene.result = sti(FullScene.transactionMoneyDelta) < 0 && sti(FullScene.transactionFoodDelta) > 0;
    }
    if (action == "interface-exit")
    {
        Event("exitCancel");
        FullScene.result = 1;
    }
    if (action == "arm-callback")
    {
        FullScene.callbackTarget = target;
        FullScene.callbackContext = "";
        FullScene.result = 1;
    }
    if (action == "charge")
    {
        if (!bSeaActive || Port_IsLandHero()) return;
        int goods = GOOD_BALLS;
        if (target == "grapes") goods = GOOD_GRAPES;
        if (target == "bombs") goods = GOOD_BOMBS;
        Ship_ChangeCharge(pchar, goods);
        FullScene.result = 1;
    }
    if (action == "sails")
    {
        float sails = stf(target);
        if (sails < 0.0 || sails > 1.0) return;
        Ship_SetSailState(GetMainCharacterIndex(), sails);
        FullScene.result = 1;
    }
    if (action == "fire")
    {
        if (!bSeaActive || Port_IsLandHero()) return;
        Ship_DoFire();
        FullScene.result = 1;
    }
    if (action == "fight")
    {
        if (!Port_IsLandHero()) return;
        LAi_SetFightMode(pchar, true);
        FullScene.result = 1;
    }
    if (action == "save")
    {
        if (DialogRun || sti(InterfaceStates.Launched)) return;
        // Native SaveEngineState supplies ordinary SeaDogs Save hooks. This
        // path is inside the private run and cannot name player state.
        SaveGameToFile("SAVE\fullscene-phase", "private fixture phase");
        FullScene.result = 1;
    }
    if (action == "load")
    {
        DeleteEntities();
        ClearEvents();
        ClearPostEvents();
        SetTimeScale(1.0);
        LoadEngineState("SAVE\fullscene-phase");
        return;
    }
    if (action == "pause") { SetTimeScale(0.0); FullScene.result = 1; }
    if (action == "resume") { SetTimeScale(1.0); FullScene.result = 1; }
    FullScene_Observe();
}

void FullScene_Observe()
{
    FullScene.seaLoaded = bSeaLoaded;
    FullScene.seaActive = bSeaActive;
    FullScene.resident = Port_IsResident();
    FullScene.landHero = Port_IsLandHero();
    FullScene.location = pchar.location;
    FullScene.loadedLocation = "";
    int location = FindLoadedLocation();
    if (location >= 0) FullScene.loadedLocation = Locations[location].id;
    FullScene.dialogue = DialogRun;
    FullScene.interface = InterfaceStates.Launched;
    aref from, into;
    makearef(from, pchar.PortTravel);
    makearef(into, FullScene.travel);
    CopyAttributes(into, from);
    FullScene.money = pchar.money;
    FullScene.heroHP = LAi_GetCharacterHP(pchar);
    FullScene.shipHP = pchar.Ship.HP;
    aref observation;
    makearef(from, pchar.quest);
    makearef(observation, FullScene.quest);
    CopyAttributes(observation, from);
    makearef(from, pchar.GenQuest);
    makearef(observation, FullScene.generatedQuest);
    CopyAttributes(observation, from);
    makearef(from, pchar.CargoQuest);
    makearef(observation, FullScene.cargoQuest);
    CopyAttributes(observation, from);
    makearef(from, pchar.items);
    makearef(observation, FullScene.items);
    CopyAttributes(observation, from);
    if (sti(InterfaceStates.Launched))
    {
        makearef(from, GameInterface.TABLE_LIST);
        makearef(observation, FullScene.storeTable);
        CopyAttributes(observation, from);
    }
    FullScene.balls = GetCargoGoods(pchar, GOOD_BALLS);
    FullScene.grapes = GetCargoGoods(pchar, GOOD_GRAPES);
    FullScene.bombs = GetCargoGoods(pchar, GOOD_BOMBS);
    if (IsEntity(pchar) && Port_IsLandHero())
    {
        float x, y, z;
        if (GetCharacterPos(pchar, &x, &y, &z))
        { FullScene.heroX = x; FullScene.heroY = y; FullScene.heroZ = z; }
    }
    if (DialogRun)
    {
        FullScene.dialogueNode = Dialog.CurrentNode;
        makearef(from, Dialog.Links);
        makearef(into, FullScene.links);
        CopyAttributes(into, from);
    }
    if (CheckAttribute(&FullScene, "guardIndex"))
    {
        ref watched = GetCharacter(sti(FullScene.guardIndex));
        FullScene.guardHP = LAi_GetCharacterHP(watched);
        FullScene.guardDead = LAi_IsDead(watched);
        FullScene.guardProtection = CharacterCombat_ProtectionReason(watched);
        makearef(from, watched.Crime);
        makearef(into, FullScene.guardCrime);
        CopyAttributes(into, from);
    }
}

void FullScene_WitnessUpdate()
{
    aref chr = GetEventData();
    float delta = GetEventData();
    if (!CheckAttribute(&FullScene, "callbackTarget") || chr.id != FullScene.callbackTarget) return;
    FullScene.callbackContext = loadedLocation.id;
    FullScene.callbackActorLocation = chr.location;
    FullScene.callbackCaptured = LAi_IsCapturedLocation;
    FullScene.callbackTemplate = chr.chr_ai.tmpl;
    if (CheckAttribute(chr, "chr_ai.tmpl.locator")) FullScene.callbackLocator = chr.chr_ai.tmpl.locator;
}

void FullScene_WitnessCannonFire()
{
    aref shooter = GetEventData();
    int index = sti(shooter.index);
    if (index == GetMainCharacterIndex())
    {
        FullScene.playerDischarges = sti(FullScene.playerDischarges) + 1;
        FullScene.lastDischargeCharge = shooter.Ship.Cannons.Charge.Type;
    }
    for (int fort = 0; fort < MAX_FORTS; fort++)
    {
        if (!CheckAttribute(&Forts[fort], "fortcmdridx") || sti(Forts[fort].fortcmdridx) != index) continue;
        FullScene.fortDischarges = sti(FullScene.fortDischarges) + 1;
        FullScene.lastFortShooter = index;
        break;
    }
}

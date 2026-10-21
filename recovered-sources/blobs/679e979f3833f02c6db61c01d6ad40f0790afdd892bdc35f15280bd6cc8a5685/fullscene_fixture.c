// Private case input, appended to an otherwise complete canonical seadogs.c.
// Never loaded by the installed app. Uses real initialization and gameplay owners.
object FullScene;

void Main()
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
    FullScene.phase = "initialized";
    SetEventHandler("FullSceneAction", "FullScene_Action", 0);
    SetEventHandler("FullSceneObserve", "FullScene_Observe", 0);
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
    if (action == "link")
    {
        // Select only a link actually offered by the original dialogue.
        if (!DialogRun || !CheckAttribute(&Dialog, "Links." + target + ".go")) return;
        Dialog.CurrentNode = Dialog.Links.(target).go;
        Event("DialogEvent");
        FullScene.result = 1;
    }
    if (action == "dialogue-exit")
    {
        DialogCancel_Exit();
        FullScene.result = !DialogRun;
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
}

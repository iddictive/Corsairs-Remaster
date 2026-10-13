// Bermuda's dream owns its fleet escrow; ordinary sea/death paths remain stock.
#event_handler("GreatRecipePulse", "GreatRecipe_Pulse");
#event_handler("GreatRecipeDeferred", "GreatRecipe_Deferred");
#event_handler("CharactersStateUpdate", "GreatRecipe_CombatCheck");
object GreatRecipeRestoreBuffer;

bool GreatRecipe_DeferredFunction(string function)
{
    return function == "GreatRecipe_Capture" || function == "GreatRecipe_FirstBoarding" ||
        function == "GreatRecipe_LaunchStorm" || function == "GreatRecipe_Return";
}

void GreatRecipe_Delay(string function, float delay)
{
    if (!GreatRecipe_DeferredFunction(function)) return;
    if (!CheckAttribute(pchar, "GreatRecipe.DeferSequence")) pchar.GreatRecipe.DeferSequence = 0;
    int sequence = sti(pchar.GreatRecipe.DeferSequence) + 1;
    pchar.GreatRecipe.DeferSequence = sequence;
    pchar.GreatRecipe.Deferred.(function) = sequence;
    PostEvent("GreatRecipeDeferred", makeint(delay * 1000.0), "sl", function, sequence);
}

void GreatRecipe_Deferred()
{
    string function = GetEventData();
    int sequence = GetEventData();
    if (!GreatRecipe_DeferredFunction(function) || !CheckAttribute(pchar, "GreatRecipe.Deferred." + function)) return;
    if (sequence != sti(pchar.GreatRecipe.Deferred.(function))) return;
    // Clear before invoking: a waiting transition may schedule itself again.
    DeleteAttribute(pchar, "GreatRecipe.Deferred." + function);
    call function("");
}

void GreatRecipe_RearmDeferred()
{
    for (int pending = 0; pending < 4; pending++)
    {
        string function = "GreatRecipe_Capture";
        if (pending == 1) function = "GreatRecipe_FirstBoarding";
        if (pending == 2) function = "GreatRecipe_LaunchStorm";
        if (pending == 3) function = "GreatRecipe_Return";
        if (CheckAttribute(pchar, "GreatRecipe.Deferred." + function)) GreatRecipe_Delay(function, 0.1);
    }
}

string GreatRecipe_Phase()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Phase")) return "";
    return pchar.GreatRecipe.Phase;
}

bool GreatRecipe_Locked()
{
    string phase = GreatRecipe_Phase();
    return phase == "whirlpool" || phase == "assault" || phase == "boarding" || phase == "capturing" || phase == "captive" ||
        phase == "duel" || phase == "storm" || phase == "returning";
}

bool GreatRecipe_Unlocked()
{
    return GreatRecipe_Phase() == "won";
}

int GreatRecipe_InitPearl(int index)
{
    if (index >= ITEMS_QUANTITY) return index;
    ref pearl = &Items[index];
    int old = -1;
    for (int item = 0; item < index; item++)
    {
        if (Items[item].id == "jewelry11") old = item;
    }
    if (old < 0) return index;
    ref original = &Items[old];
    CopyAttributes(pearl, original);
    pearl.id = "GreatRecipeRedPearl";
    pearl.index = index;
    pearl.name = "itmname_GreatRecipeRedPearl";
    pearl.describe = "itmdescr_GreatRecipeRedPearl";
    pearl.ItemType = "QUESTITEMS";
    pearl.price = 0;
    pearl.rare = 0;
    pearl.uniq = true;
    DeleteAttribute(pearl, "Generation");
    DeleteAttribute(pearl, "groupID");
    return index + 1;
}

void GreatRecipe_EnsurePearl()
{
    // Old saves restore the item catalogue; NewGame's InitItems is not replayed.
    int empty = -1;
    for (int item = 0; item < ITEMS_QUANTITY; item++)
    {
        if (CheckAttribute(&Items[item], "id") && Items[item].id == "GreatRecipeRedPearl") return;
        if (empty < 0 && CheckAttribute(&Items[item], "id") && Items[item].id == "item" + (item + 1)) empty = item;
    }
    if (empty >= 0) GreatRecipe_InitPearl(empty);
}

bool GreatRecipe_NeedsPearl()
{
    string phase = GreatRecipe_Phase();
    return (phase == "loot" || phase == "pearl_search") && GetCharacterItem(pchar, "GreatRecipeRedPearl") == 0;
}

void GreatRecipe_GrantLoot(bool full)
{
    if (full)
    {
        GiveItem2Character(pchar, "blade32");
        GiveItem2Character(pchar, "pistol4");
        AddMoneyToCharacter(pchar, 100000);
    }
    if (GetCharacterItem(pchar, "GreatRecipeRedPearl") == 0) GiveItem2Character(pchar, "GreatRecipeRedPearl");
}

bool GreatRecipe_PickupBarrel(int index)
{
    if (index != nMainCharacterIndex && !IsCompanion(GetCharacter(index))) return false;
    if (!GreatRecipe_NeedsPearl()) return true;
    bool full = !CheckAttribute(pchar, "GreatRecipe.Loot.BundleTaken");
    pchar.GreatRecipe.Loot.BundleTaken = true;
    GreatRecipe_GrantLoot(full);
    if (CheckAttribute(pchar, "GreatRecipe.Escrow"))
    {
        if (full) pchar.GreatRecipe.Loot.EscrowGrant = "full";
        else pchar.GreatRecipe.Loot.EscrowGrant = "pearl";
    }
    DeleteAttribute(pchar, "GreatRecipe.Loot.MapID");
    DeleteAttribute(pchar, "GreatRecipe.Loot.SeaPending");
    GreatRecipe_Journal("В бочке найдена красная жемчужина с клеймом Бороды. Отнести её Лаки Росину в бермудскую таверну: он поможет разобраться с рецептом.", false);
    return true;
}

void GreatRecipe_SpawnBarrel(ref captain, bool recovery)
{
    if (!bSeaActive || !isEntity(&AISeaGoods)) return;
    if (!GreatRecipe_NeedsPearl() || CheckAttribute(pchar, "GreatRecipe.Loot.SeaPending")) return;
    pchar.GreatRecipe.Loot.SeaPending = true;
    if (recovery) AISeaGoods_AddGood_Special(captain, "great_recipe_barrel", "barrel_treasure", 1200.0, 1);
    else AISeaGoods_AddGood(captain, "great_recipe_barrel", "barrel_treasure", 1200.0, 1);
}

void GreatRecipe_MarkRecoveryBarrel(string id)
{
    if (!GreatRecipe_NeedsPearl()) return;
    if (CheckAttribute(pchar, "GreatRecipe.Loot.SeaPending")) return;
    if (CheckAttribute(pchar, "GreatRecipe.Loot.MapID"))
    {
        string old = "encounters." + pchar.GreatRecipe.Loot.MapID;
        if (CheckAttribute(&worldMap, old) && !CheckAttribute(&worldMap, old + ".needDelete")) return;
        DeleteAttribute(pchar, "GreatRecipe.Loot.MapID");
    }
    string key = "encounters." + id + ".encdata";
    if (!CheckAttribute(&worldMap, key + ".RealEncounterType")) return;
    if (sti(worldMap.(key).RealEncounterType) != ENCOUNTER_TYPE_BARREL || rand(2) != 0) return;
    worldMap.(key).GreatRecipeBarrel = true;
    pchar.GreatRecipe.Loot.MapID = id;
}

bool GreatRecipe_TurnInPearl()
{
    if (GreatRecipe_Phase() != "pearl_search" || GetCharacterItem(pchar, "GreatRecipeRedPearl") == 0) return false;
    TakeItemFromCharacter(pchar, "GreatRecipeRedPearl");
    pchar.GreatRecipe.Phase = "production";
    SaveCurrentNpcQuestDateParam(pchar, "GreatRecipe.Production");
    pchar.GreatRecipe.Production.Started = true;
    for (int store = 0; store < STORE_QUANTITY; store++)
    {
        if (CheckAttribute(&Stores[store], "Colony") && Stores[store].Colony == "Pirates")
        {
            Stores[store].Goods.Airburst.Quantity = 0;
            Stores[store].GreatRecipeProduction.ThroughDay = 6;
        }
    }
    GreatRecipe_Journal("Жемчужина передана Лаки Росину. Мастерская на Бермудах готовит шрапнель: первая партия через семь игровых дней. Затем магазин получает 20–30 бомб в день, не больше 200 в наличии.", false);
    return true;
}

void GreatRecipe_EnsureProduction()
{
    if (GreatRecipe_Phase() != "won" || CheckAttribute(pchar, "GreatRecipe.Production.Started")) return;
    // Previous actual winners retain access; this seeds time, never free stock.
    pchar.GreatRecipe.Production.LegacyAccess = true;
    pchar.GreatRecipe.Production.Started = true;
    SaveCurrentNpcQuestDateParam(pchar, "GreatRecipe.Production");
}

void GreatRecipe_StoreDaily(ref store, int recoveryOffset)
{
    if (!CheckAttribute(store, "Colony") || store.Colony != "Pirates") return;
    GreatRecipe_EnsureProduction();
    if (!CheckAttribute(pchar, "GreatRecipe.Production.Started")) return;
    int day = GetNpcQuestPastDayParam(pchar, "GreatRecipe.Production") - recoveryOffset;
    bool legacy = CheckAttribute(pchar, "GreatRecipe.Production.LegacyAccess");
    if (legacy) day += 7;
    if (day < 7) return;
    int through = 6;
    if (CheckAttribute(store, "GreatRecipeProduction.ThroughDay")) through = sti(store.GreatRecipeProduction.ThroughDay);
    else if (legacy) through = 7; // admission is not a manufactured day
    if (day <= through) return;
    int count = day - through;
    if (count > 10) count = 10; // ten minimum batches necessarily reach the cap
    int quantity = sti(store.Goods.Airburst.Quantity);
    for (int batch = 0; batch < count; batch++)
    {
        quantity += 20 + rand(10);
    }
    if (quantity > 200) quantity = 200;
    store.Goods.Airburst.Quantity = quantity;
    store.GreatRecipeProduction.ThroughDay = day;
    store.Goods.Airburst.TradeType = TRADE_TYPE_AMMUNITION;
    if (GreatRecipe_Phase() == "production")
    {
        pchar.GreatRecipe.Phase = "won";
        GreatRecipe_Journal("Первая партия шрапнельных бомб готова в бермудском магазине. Выпуск идёт каждый игровой день по 20–30 бомб; запас ограничен 200. За пределами Бермуд обычной продажи нет.", true);
    }
}

void GreatRecipe_Journal(string body, bool complete)
{
    bool duplicate = false;
    if (CheckAttribute(pchar, "QuestInfo.GreatRecipe.Text"))
    {
        aref records; makearef(records, pchar.QuestInfo.GreatRecipe.Text);
        int count = GetAttributesNum(records);
        if (count > 0)
        {
            aref last = GetAttributeN(records, count - 1);
            if (CheckAttribute(last, "UserData")) duplicate = last.UserData == "@<body>" + body;
        }
    }
    if (!duplicate)
    {
        AddQuestRecord("GreatRecipe", "1");
        AddQuestUserData("GreatRecipe", "body", body);
    }
    if (complete && !sti(pchar.QuestInfo.GreatRecipe.Complete)) CloseQuestHeader("GreatRecipe");
}

bool GreatRecipe_Moonlit()
{
    // An explicit repeatable calendar window; no hidden date/time manipulation.
    return (GetTime() >= 22.0 || GetTime() < 4.0) &&
        GetDataDay() >= 12 && GetDataDay() <= 18 && !Whr_IsRain() && !Whr_IsStorm();
}

bool GreatRecipe_Window()
{
    return (GetTime() >= 22.0 || GetTime() < 4.0) && GetDataDay() >= 12 && GetDataDay() <= 18;
}

void GreatRecipe_MapCalendar()
{
    if (!IsEntity(&worldMap)) return;
    string phase = GreatRecipe_Phase();
    if (phase != "search" && phase != "lost" && !CheckAttribute(pchar, "GreatRecipe.MapCaptain")) return;
    string clock = worldMap.date.year + ":" + worldMap.date.month + ":" + worldMap.date.day + ":" + sti(worldMap.date.hour) + ":" + GreatRecipe_Phase();
    if (CheckAttribute(pchar, "GreatRecipe.MapClock") && pchar.GreatRecipe.MapClock == clock) return;
    pchar.GreatRecipe.MapClock = clock;
    bool window = phase == "search" && GreatRecipe_Moonlit();
    if (phase == "lost") window = GreatRecipe_Window() && Whr_IsStorm();
    if (!window)
    {
        if (CheckAttribute(pchar, "GreatRecipe.MapCaptain"))
        {
            Map_ReleaseQuestEncounter(pchar.GreatRecipe.MapCaptain);
            DeleteAttribute(pchar, "GreatRecipe.MapCaptain");
        }
        return;
    }
    if (CheckAttribute(pchar, "GreatRecipe.MapCaptain")) return;
    float centerX = stf(worldMap.islands.Bermudes.position.x);
    float centerZ = stf(worldMap.islands.Bermudes.position.z);
    for (int attempt = 0; attempt < 8; attempt++)
    {
        float x1 = centerX - 160.0 - attempt * 12.0;
        float z1 = centerZ - 100.0 - attempt * 8.0;
        float x2 = x1 - 80.0;
        float z2 = z1 + 50.0;
        bool clear = true;
        for (int point = 0; point < 5; point++)
        {
            float x = x1 + (x2 - x1) * point / 4.0;
            float z = z1 + (z2 - z1) * point / 4.0;
            if (!SendMessage(&worldMap, "lfff", MSG_WORLDMAP_WATER_POINT, x, z, 70.0)) clear = false;
        }
        if (!clear) continue;
        string id = "GreatRecipeDreamBeard";
        if (phase == "lost") id = "GreatRecipeRealBeard";
        ref captain = GreatRecipe_Actor(id, "RedBeard");
        if (!CheckAttribute(captain, "Ship.Type") || sti(captain.Ship.Type) == SHIP_NOTUSED)
        {
            if (phase == "search") GreatRecipe_Boss(captain);
            else { GreatRecipe_Ship(captain, SHIP_CORVETTE, "Громовая черепаха", 95); captain.Ship.HP = 900; }
        }
        captain.MapEnc.Type = "war";
        captain.name = "Красная"; captain.lastname = "Борода";
        pchar.GreatRecipe.MapCaptain = id;
        Map_CreateTraderXZ(x1, z1, x2, z2, id, 1);
        return;
    }
}

bool GreatRecipe_FleetFree()
{
    for (int slot = 1; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index >= 0 && (!GetRemovable(GetCharacter(index)) || !GetShipRemovable(GetCharacter(index)))) return false;
    }
    return true;
}

bool GreatRecipe_Accept()
{
    if (GreatRecipe_Phase() != "" || GreatRecipe_Locked()) return false;
    if (!CheckAttribute(pchar, "Ship.Type") || sti(pchar.Ship.Type) == SHIP_NOTUSED) return false;
    if (CheckAttribute(pchar, "Custody")) return false;
    pchar.GreatRecipe.Phase = "search";
    pchar.GreatRecipe.Version = 1;
    GreatRecipe_Journal("Лаки слышал о кровавом капитане и жемчуге черепахи. Проверить слухи в открытом море у Бермуд в ясную лунную ночь: с 22 до 4 часов, с 12 по 18 число. Искать странный корабль на глобальной карте и подойти к нему всей эскадрой; оставаться подальше от берегов.", false);
    return true;
}

void GreatRecipe_Copy(aref destination, aref source)
{
    DeleteAttribute(destination, "");
    CopyAttributes(destination, source);
}

void GreatRecipe_Snapshot()
{
    if (CheckAttribute(pchar, "GreatRecipe.Escrow")) return;
    aref source, destination;
    CopyAttributes(&GreatRecipeRestoreBuffer, pchar);
    DeleteAttribute(&GreatRecipeRestoreBuffer, "GreatRecipe");
    makearef(destination, pchar.GreatRecipe.Escrow.Character);
    CopyAttributes(destination, &GreatRecipeRestoreBuffer);
    DeleteAttribute(&GreatRecipeRestoreBuffer, "");
    makearef(source, pchar.Fellows.Companions);
    makearef(destination, pchar.GreatRecipe.Escrow.Companions);
    CopyAttributes(destination, source);
    pchar.GreatRecipe.Escrow.Health = LAi_GetCharacterHP(pchar);
    pchar.GreatRecipe.Escrow.Reload = chrDisableReloadToLocation;
    pchar.GreatRecipe.Escrow.FastReload = bDisableFastReload;
    pchar.GreatRecipe.Escrow.CharacterMenu = bDisableCharacterMenu;
    pchar.GreatRecipe.Escrow.Map = bQuestDisableMapEnter;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        string key = "s" + slot;
        ref captain = GetCharacter(index);
        if (index != nMainCharacterIndex)
        {
            makearef(destination, pchar.GreatRecipe.Escrow.Fleet.(key).Character);
            CopyAttributes(destination, captain);
        }
        pchar.GreatRecipe.Escrow.Fleet.(key).Index = index;
        makearef(source, captain.Ship);
        makearef(destination, pchar.GreatRecipe.Escrow.Fleet.(key).Ship);
        CopyAttributes(destination, source);
        ref realShip = GetRealShip(sti(captain.Ship.Type));
        makearef(destination, pchar.GreatRecipe.Escrow.Fleet.(key).RealShip);
        CopyAttributes(destination, realShip);
        makearef(source, captain.chr_ai);
        makearef(destination, pchar.GreatRecipe.Escrow.Fleet.(key).AI);
        CopyAttributes(destination, source);
    }
}

void GreatRecipe_ReserveShips()
{
    if ((!GreatRecipe_Locked() && GreatRecipe_Phase() != "loot") || !CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet")) return;
    aref fleet;
    makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        int type = sti(saved.Ship.Type);
        if (type >= 0 && type < REAL_SHIPS_QUANTITY) RealShips[type].lock = true;
    }
}

int GreatRecipe_Hold()
{
    int index = FindLocation("GreatRecipeHold");
    if (index >= 0) return index;
    int original = FindLocation("My_Deck");
    if (original < 0 || nLocationsNum >= MAX_LOCATIONS) return -1;
    index = nLocationsNum;
    ref hold = &Locations[index];
    CopyAttributes(hold, &Locations[original]);
    hold.id = "GreatRecipeHold";
    hold.id.label = "Трюм Красной Бороды";
    hold.index = index;
    hold.type = "quest";
    hold.DisableOfficers = true;
    hold.parent_colony = "Pirates";
    hold.islandId = "Bermudes";
    DeleteAttribute(hold, "reload");
    DeleteAttribute(hold, "box1");
    DeleteAttribute(hold, "box2");
    DeleteAttribute(hold, "box3");
    DeleteAttribute(hold, "box4");
    DeleteAttribute(hold, "items");
    DeleteAttribute(hold, "boarding");
    LAi_LocationFantomsGen(hold, false);
    LAi_LocationFightDisable(hold, true);
    nLocationsNum++;
    return index;
}

bool GreatRecipe_ShipRooms()
{
    if (GreatRecipe_Hold() < 0) return false;
    for (int room = 0; room < 3; room++)
    {
        string id = "GreatRecipeDeck";
        string original = "My_Deck_Medium";
        string label = "Палуба Красной Бороды";
        if (room == 1) { id = "GreatRecipeMess"; original = "My_Campus"; label = "Кубрик Красной Бороды"; }
        if (room == 2) { id = "GreatRecipeCabin"; original = "My_Cabin"; label = "Каюта Красной Бороды"; }
        if (FindLocation(id) >= 0) continue;
        int source = FindLocation(original);
        if (source < 0 || nLocationsNum >= MAX_LOCATIONS) return false;
        ref location = &Locations[nLocationsNum];
        CopyAttributes(location, &Locations[source]);
        location.id = id;
        location.id.label = label;
        location.index = nLocationsNum;
        location.type = "quest";
        location.DisableOfficers = true;
        location.parent_colony = "Pirates";
        DeleteAttribute(location, "fastreload");
        DeleteAttribute(location, "box1"); DeleteAttribute(location, "box2");
        DeleteAttribute(location, "box3"); DeleteAttribute(location, "box4");
        DeleteAttribute(location, "items"); DeleteAttribute(location, "boarding");
        LAi_LocationFantomsGen(location, false);
        LAi_LocationFightDisable(location, room != 0);
        nLocationsNum++;
    }
    ref hold = &Locations[FindLocation("GreatRecipeHold")];
    hold.reload.l1.name = "reload1";
    hold.reload.l1.go = "GreatRecipeDeck";
    hold.reload.l1.emerge = "reload_hold";
    hold.reload.l1.autoreload = "0";
    hold.reload.l1.label = "Палуба";
    ref deck = &Locations[FindLocation("GreatRecipeDeck")];
    deck.reload.l1.go = "GreatRecipeCabin";
    deck.reload.l2.go = "GreatRecipeMess";
    deck.reload.l3.go = "GreatRecipeHold";
    ref mess = &Locations[FindLocation("GreatRecipeMess")];
    mess.reload.l1.go = "GreatRecipeDeck";
    ref cabin = &Locations[FindLocation("GreatRecipeCabin")];
    cabin.reload.l1.go = "GreatRecipeDeck";
    cabin.reload.l1.emerge = "reload_cabin";
    return true;
}

ref GreatRecipe_Actor(string id, string model)
{
    int index = GetCharacterIndex(id);
    ref actor;
    if (index < 0) actor = GetCharacter(NPC_GenerateCharacter(id, model, "man", "man", 30, PIRATE, -1, false));
    else actor = GetCharacter(index);
    if (model == "RedBeard" && !IsEntity(actor)) actor.model = model;
    if (model == "RedBeard") actor.FaceId = 2006;
    return actor;
}

void GreatRecipe_Locks()
{
    bDisableFastReload = true;
    bQuestDisableMapEnter = true;
    // Character/ship panels stay usable for a fair duel's charge/control setup.
    chrDisableReloadToLocation = CheckAttribute(pchar, "GreatRecipe.Fight.Active");
}

void GreatRecipe_CaptiveActors()
{
    GreatRecipe_Locks();
    if (CheckAttribute(pchar, "GreatRecipe.Fight.Active"))
    {
        LAi_SetCheckMinHP(pchar, LAi_GetCharacterMaxHP(pchar) * 0.2, true, "");
        LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
        LAi_group_FightGroups("GreatRecipeFightGroup", LAI_GROUP_PLAYER, true);
    }
    for (int role = 0; role < 6; role++)
    {
        string id = "GreatRecipeBosun";
        string name = "Морган";
        string room = "GreatRecipeDeck";
        string group = "rld";
        string locator = "loc0";
        if (role == 1) { id = "GreatRecipeCook"; name = "Старый Том"; room = "GreatRecipeMess"; group = "goto"; locator = "goto1"; }
        if (role == 2) { id = "GreatRecipePrisoner"; name = "Диего"; room = "GreatRecipeHold"; group = "goto"; locator = "goto5"; }
        if (role == 3) { id = "GreatRecipeGunner"; name = "Длинный Джек"; locator = "loc6"; }
        if (role == 4) { id = "GreatRecipeMate"; name = "Ратли"; locator = "loc10"; }
        if (role == 5) { id = "GreatRecipeSurgeon"; name = "Доктор Вейн"; room = "GreatRecipeMess"; group = "goto"; locator = "goto6"; }
        ref actor = GreatRecipe_Actor(id, "pirate_1");
        actor.name = name; actor.lastname = "";
        actor.Dialog.Filename = "GreatRecipe.c";
        actor.Dialog.CurrentNode = "First time";
        actor.Dialog.TempNode = "First time";
        LAi_SetImmortal(actor, true);
        LAi_SetStayType(actor);
        LAi_group_MoveCharacter(actor, "GreatRecipeHoldGroup");
        ChangeCharacterAddressGroup(actor, room, group, locator);
    }
    ref beard = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
    beard.name = "Красная";
    beard.lastname = "Борода";
    beard.Dialog.Filename = "GreatRecipe.c";
    beard.Dialog.CurrentNode = "First time";
    beard.Dialog.TempNode = "First time";
    if (CheckAttribute(pchar, "GreatRecipe.Fight.Active") && pchar.GreatRecipe.Fight.Active == "captain")
    {
        LAi_SetWarriorTypeNoGroup(beard);
        LAi_group_MoveCharacter(beard, "GreatRecipeFightGroup");
        LAi_SetCheckMinHP(beard, 40.0, true, "");
        LAi_SetCheckMinHP(pchar, LAi_GetCharacterMaxHP(pchar) * 0.2, true, "");
        LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
        LAi_group_FightGroups("GreatRecipeFightGroup", LAI_GROUP_PLAYER, true);
        return;
    }
    LAi_SetImmortal(beard, true);
    LAi_SetStayType(beard);
    LAi_group_MoveCharacter(beard, "GreatRecipeHoldGroup");
    DeleteAttribute(beard, "AlwaysEnemy");
    ChangeCharacterAddressGroup(beard, "GreatRecipeCabin", "rld", "loc0");
}

void GreatRecipe_TaskAdvance(string task, string note)
{
    pchar.GreatRecipe.Task = task;
    pchar.GreatRecipe.Last = note;
    GreatRecipe_Journal(note, false);
}

bool GreatRecipe_IsBeard(aref actor)
{
    string id = "";
    if (CheckAttribute(actor, "id")) id = actor.id;
    // Stock boarding copies the captain to a fantom and sets this exact owner ID.
    if (CheckAttribute(actor, "CaptanId")) id = actor.CaptanId;
    return id == "GreatRecipeDreamBeard" || id == "GreatRecipeDuelBeard" || id == "GreatRecipeRealBeard";
}

int GreatRecipe_StoreSaleLimit(ref store, int good, int quantity)
{
    if (good != GOOD_AIRBURST || !CheckAttribute(store, "Colony") || store.Colony != "Pirates") return quantity;
    int room = 200 - GetStoreGoodsQuantity(store, good);
    if (room < 0) room = 0;
    if (quantity > room) quantity = room;
    return quantity;
}

float GreatRecipe_BeardBladeDamage(aref actor, aref target, float raw, float armor)
{
    if (!GreatRecipe_IsBeard(actor))
    {
        if (GreatRecipe_IsEliteCrew(actor)) raw *= 1.4;
        return raw * (1.0 - armor);
    }
    float damage = raw * 2.0 * (1.0 - armor * 0.5);
    float ceiling = LAi_GetCharacterMaxHP(target) * 0.33;
    if (damage > ceiling) damage = ceiling;
    return damage;
}

bool GreatRecipe_IsEliteCrew(aref actor)
{
    if (!CheckAttribute(actor, "GreatRecipeEliteCrew")) return false;
    if (actor.GreatRecipeEliteCrew != "GreatRecipeDreamBeard") return false;
    if (!bAbordageStarted || boarding_enemy.id != "GreatRecipeDreamBeard") return false;
    if (!CheckAttribute(actor, "chr_ai.group") || actor.chr_ai.group != LAI_GROUP_BRDENEMY) return false;
    string phase = GreatRecipe_Phase();
    return phase == "whirlpool" || phase == "assault" || phase == "boarding";
}

void GreatRecipe_BoardingFighter(ref actor, ref captain)
{
    DeleteAttribute(actor, "GreatRecipeEliteCrew");
    if (GreatRecipe_IsBeard(actor)) { GreatRecipe_BeardFighter(actor); return; }
    if (captain.id != "GreatRecipeDreamBeard") return;
    if (!CheckAttribute(actor, "chr_ai.group") || actor.chr_ai.group != LAI_GROUP_BRDENEMY) return;
    string phase = GreatRecipe_Phase();
    if (phase != "whirlpool" && phase != "assault" && phase != "boarding") return;
    actor.GreatRecipeEliteCrew = captain.id;
    float hp = LAi_GetCharacterMaxHP(actor) * 1.45;
    LAi_SetHP(actor, hp, hp);
    actor.skill.Fencing = 100;
    actor.skill.FencingLight = 100;
    actor.skill.FencingHeavy = 100;
    LAi_SetImmortal(actor, false);
}

float GreatRecipe_BeardFireDamage(aref actor, aref target, float damage)
{
    if (!GreatRecipe_IsBeard(actor))
    {
        if (GreatRecipe_IsEliteCrew(actor)) damage *= 1.4;
        return damage;
    }
    float ceiling = LAi_GetCharacterMaxHP(target) * 0.33;
    if (damage > ceiling) damage = ceiling;
    return damage;
}

void GreatRecipe_BeardFighter(ref actor)
{
    FantomMakeCoolFighter(actor, 35, 100, 100, "blade32", "", 0);
    actor.skill.Fencing = 100;
    actor.skill.FencingLight = 100;
    actor.skill.FencingHeavy = 100;
    actor.skill.Pistol = 100;
    actor.skill.Fortune = 100;
    actor.FaceId = 2006;
    LAi_SetHP(actor, 1500.0, 1500.0);
}

string GreatRecipe_Story(string id, string topic)
{
    if (id == "GreatRecipeDreamBeard")
    {
        if (topic == "scar") return "Этот глаз мне выбил береговой пушкарь. Я запомнил его порт. Когда вернулся, велел стрелять, пока у берега не стало тихо. Шрам — удобная память: в зеркале всегда видишь, кому задолжал.";
        if (topic == "mercy") return "Почему ты жив? Мёртвые все одинаковы. Живой капитан сперва врёт, потом торгуется, потом делает выбор. Мне интересно, что ты сделаешь, когда за тебя уже некому ответить.";
        if (topic == "pearl") return "Думаешь, секрет в порохе? Порох есть у всякого дурака. Важно, где рванёт скорлупа. Белый фитиль — для спешки. Красный — чтоб враг успел поверить, будто промахнулся.";
        if (topic == "crew") return "Джек остался, потому что любит пушки больше людей. Том — потому что кормить мёртвых ему ещё не приходилось. Диего помнит лицо каждого, кого я взял. А Ратли думает, что ему хватит сухарей купить весь корабль.";
        return "Берег боится красного паруса, и я не тороплюсь его разубеждать. Слабаки любят повторять, что я безумен. Так им легче объяснить, почему не смогли мне помешать. Ты тоже так думаешь? Проверь клинком — на палубе места хватит.";
    }
    if (id == "GreatRecipeCook") return "До этого я кормил людей на купеческом судне. Борода взял груз, потом спросил, умею ли я варить похлёбку. Капитана нашего он не спрашивал ни о чём. Я до сих пор ставлю лишнюю миску, когда волна тихая.";
    if (id == "GreatRecipePrisoner") return "Я чинил снасти у рыбацкого берега. Он обстрелял нас — даже не причалил за добычей. Я выплыл, а мать нет. На этом корабле мне предлагают забыть её, стать своим. Каждый день предлагают.";
    if (id == "GreatRecipeGunner") return "Я знаю звук целого борта и звук, когда он уже мёртв. Борода научился бить так, чтоб люди сами прыгали в воду. Я остаюсь у пушек, потому что боюсь, кто встанет на моё место. Не считай это доблестью.";
    if (id == "GreatRecipeMate") return "Крюк лижет капитану сапоги, Том молчит, Джек прячется за орудиями. Я хотя бы собираю людей. Они хотят уйти, но свободному человеку нужны вода, провиант и оружие. Капитан ничего не отдаст. Ты бы предпочёл умереть с чистой совестью и пустым животом?";
    if (id == "GreatRecipeSurgeon") return "Я лечил и тех, кого Борода резал, и тех, кто резал за него. У всех кровь одинаковая. Диего я перевязывал во время кражи: он не мог уйти из трюма. Скажешь боцману — он узнает, что я помогал пленнику тайком.";
    return "На берегу о нас сочиняют сказки. На корабле за каждую такую сказку кто-нибудь платит.";
}

string GreatRecipe_TaskAction(string action)
{
    if (GreatRecipe_Phase() != "captive" || CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return "Сначала разберёмся с дракой.";
    string task = pchar.GreatRecipe.Task;
    if (action == "cook" && pchar.location == "GreatRecipeMess" && task == "rations")
    { pchar.GreatRecipe.Clue.Cook = true; return "Из кладовой ушла бочка сухарей. Я запер её после ужина; ключ был у помощника капитана. А на крышке осталась синяя краска."; }
    if (action == "prisoner" && pchar.location == "GreatRecipeHold" && task == "rations")
    { pchar.GreatRecipe.Clue.Prisoner = true; return "Ратли заставил меня катить бочку в кубрик. Грозился выбросить за борт. Сухари он прячет, чтоб потом купить голоса команды."; }
    if (action == "gunner" && pchar.location == "GreatRecipeDeck" && task == "rations")
    { pchar.GreatRecipe.Clue.Gunner = true; return "Синие сапоги? Только у Ратли. Я видел его у люка после вахты. Диего тогда был прикован внизу."; }
    if (action == "surgeon" && task == "rations" && pchar.location == "GreatRecipeMess")
    { pchar.GreatRecipe.Clue.Alibi = true; return GreatRecipe_Story("GreatRecipeSurgeon", "history"); }
    if (action == "accuse" && task == "rations" && pchar.location == "GreatRecipeDeck")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Clue.Cook") || !CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") || !CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") || !CheckAttribute(pchar, "GreatRecipe.Clue.Alibi")) return "Боцман: слова пленника против слов помощника. Кто подтвердит, где был Диего? Лекарь мог его видеть. Расспроси всех, прежде чем обвинять.";
        pchar.GreatRecipe.Fight.Pending = "rations";
        return "Ратли: врёшь, крыса! Выходи на палубу, посмотрим, кто кого заставит замолчать.";
    }
    if (action == "betray" && task == "rations" && pchar.location == "GreatRecipeDeck" && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner"))
    { pchar.GreatRecipe.Betrayed = true; GreatRecipe_TaskAdvance("rigging", "Я выдал Диего боцману. Ратли прикрыл кражу; теперь велено вернуть ход заклинившему грота-шкоту. Канонир знает, где искать поломку."); return "Боцман: пленника запру. А ты к Джеку — грот опять не слушается руля."; }
    if (action == "gunner" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    { pchar.GreatRecipe.Clue.Block = true; return "Шкот цел, но блок заклинило железным штифтом. Нужен новый деревянный нагель, пропитанный маслом. У Тома масло, у Диего обрезки дуба."; }
    if (action == "cook" && task == "rigging" && pchar.location == "GreatRecipeMess" && CheckAttribute(pchar, "GreatRecipe.Clue.Block"))
    { pchar.GreatRecipe.Supply.Oil = true; return "Бери масло. Только не лей на канат — смажь ось блока. Вчера Ратли расспрашивал меня, как остановить грот незаметно."; }
    if (action == "prisoner" && task == "rigging" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Block"))
    {
        if (CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")) return "Ты выдал меня вместо Ратли. Хочешь моей помощи — заставь его признаться перед командой. Тогда поговорим.";
        pchar.GreatRecipe.Supply.Pin = true; return "Дубовый нагель лежал под бочкой; вот он. Не бей молотком: сначала ослабь шкот, потом вынь железо. Ратли хочет, чтоб нас настигли его дружки.";
    }
    if (action == "accuse" && task == "rigging" && pchar.location == "GreatRecipeDeck" && CheckAttribute(pchar, "GreatRecipe.Betrayed"))
    { pchar.GreatRecipe.Fight.Pending = "redeem"; return "Ратли: назад берёшь свои слова? Сначала попробуй отнять у меня саблю — тогда получишь признание."; }
    if (action == "repair" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Supply.Oil") || !CheckAttribute(pchar, "GreatRecipe.Supply.Pin")) return "Без нагеля и масла блок не оживёт. Возвращайся к коку и пленнику.";
        GreatRecipe_TaskAdvance("training", "Заклинивший блок заменён. Джек устроит два поединка на палубе: нужно научиться защищать орудийную прислугу. Поговорить с ним и принять вызов; поражение позволяет повторить попытку.");
        return "Джек: снасть ходит. Теперь другое дело: у пушек нужны люди, которые не побегут от первого клинка. Два моих матроса выйдут по очереди — покажи, что умеешь.";
    }
    if (action == "rigging_wrong" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    { pchar.GreatRecipe.Fight.Pending = "rigging_lesson"; return "Джек: рубить натянутый шкот? Он тебя за бортом пополам сложит. Прежде чем трогать снасти, научись стоять на ногах. Ко мне, с клинком."; }
    if (action == "drill" && task == "training" && pchar.location == "GreatRecipeDeck")
    { pchar.GreatRecipe.Fight.Pending = "training"; return "Джек: без стрельбы, до сдачи. Первый любит быстрые выпады, второй давит силой. Одинаково стоять против них — верный способ лечь."; }
    if (action == "captain_practice" && pchar.location == "GreatRecipeCabin")
    { pchar.GreatRecipe.Fight.Pending = "captain"; return "Борода: на палубу. До сдачи, не до смерти — за твой труп я уже заплатил бы слишком много хлопот. Посмотрим, держишь ли ты слово так же крепко, как клинок."; }
    if (action == "surgeon" && task == "medicine" && pchar.location == "GreatRecipeMess")
    { pchar.GreatRecipe.Clue.Medicine = true; return "Принеси чистое полотно из трюма и кипячёную воду от кока. В рану порох не сыпать, ром не лить. Диего нужен живой — он видел, кто вскрыл крюйт-камеру."; }
    if (action == "prisoner" && task == "medicine" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Medicine"))
    { pchar.GreatRecipe.Supply.Linen = true; return "Чистое полотно под койкой. Бери... Я видел Ратли у пороха. Но сперва помоги — лихорадка меня добьёт."; }
    if (action == "cook" && task == "medicine" && pchar.location == "GreatRecipeMess" && CheckAttribute(pchar, "GreatRecipe.Clue.Medicine"))
    { pchar.GreatRecipe.Supply.Water = true; return "Вот котелок воды. Уже остыла. Врач велел тебе промыть рану и стянуть края чистым полотном, а не жечь её ромом."; }
    if (action == "heal" && task == "medicine" && pchar.location == "GreatRecipeHold")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Supply.Linen") || !CheckAttribute(pchar, "GreatRecipe.Supply.Water")) return "Нужны чистая вода и полотно. Лекарь ждёт в кубрике.";
        pchar.GreatRecipe.Helped = true;
        GreatRecipe_TaskAdvance("powder", "Рана Диего перевязана. Он видел вскрытую крюйт-камеру и подменённые фитили. Нужно сверить накладную у Бороды и восстановить боезапас с канониром.");
        return "Диего: теперь верю тебе. В журнале капитана три меры пороха и сухой фитиль. Ратли подменил его промасленной паклей.";
    }
    if (action == "captain" && task == "powder" && pchar.location == "GreatRecipeCabin")
    { pchar.GreatRecipe.Clue.Manifest = true; return "Борода: три меры, сухая оболочка, фитиль с красной нитью. Не путать с белым: тот горит быстрее. Проверить с Джеком; свои пальцы мне ни к чему."; }
    if (action == "prisoner" && task == "powder" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Manifest"))
    { pchar.GreatRecipe.Supply.Fuse = true; return "Вот сухие фитили с красной нитью. Белые Ратли подкинул нарочно — снаряд рванёт ещё у борта. Скажи Джеку, что я не с ним."; }
    if (action == "powder" && task == "powder" && pchar.location == "GreatRecipeDeck")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Clue.Manifest") || !CheckAttribute(pchar, "GreatRecipe.Supply.Fuse")) return "Джек: сперва накладная у капитана и сухие фитили из трюма. На глаз заряжать не стану.";
        GreatRecipe_TaskAdvance("mutiny", "Боезапас проверен по капитанской накладной. Ратли готовит мятеж: его люди требуют открыть оружейный шкаф. Нужно решить, чью сторону принять, и остановить их на палубе.");
        return "Джек: заряды сухие. Ратли собирает своих у люка. С ним нельзя договориться — готовь клинок.";
    }
    if (action == "captain" && task == "mutiny_final" && pchar.location == "GreatRecipeCabin")
    { pchar.GreatRecipe.Clue.CaptainOrder = true; return "Борода: Ратли ещё держит носовую палубу. Своих ты уже выбрал; теперь ответь за них. Отбери у его последней тройки оружие. Или у моей — если всё ещё веришь в его свободу."; }
    if (action == "wrong_fuse" && task == "powder" && pchar.location == "GreatRecipeDeck")
    { pchar.GreatRecipe.Fight.Pending = "powder_lesson"; return "Джек вырывает фитиль из рук: белая нить! Такой разрыв перебьёт наших. Хочешь командовать пушками — сперва докажи, что способен остановиться. Выходи к носовому орудию."; }
    if ((action == "loyal" || action == "rebel") && (task == "mutiny" || task == "mutiny_final") && pchar.location == "GreatRecipeDeck")
    {
        if (task == "mutiny_final" && !CheckAttribute(pchar, "GreatRecipe.Clue.CaptainOrder")) return "Боцман: капитан вызывает в каюту. Не лезь на нос, пока не поговорил с ним.";
        pchar.GreatRecipe.Rebel = action == "rebel";
        pchar.GreatRecipe.Fight.Pending = task;
        return "На палубе уже обнажили клинки. От слов теперь толку нет.";
    }
    return "Сперва закончи дело, о котором говорил боцман.";
}

void GreatRecipe_StartFight(string unused)
{
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Fight.Pending")) return;
    if (pchar.location != "GreatRecipeDeck")
    {
        if (pchar.GreatRecipe.Fight.Pending == "captain") DoFunctionReloadToLocation("GreatRecipeDeck", "reload", "reload_cabin", "GreatRecipe_StartFight");
        return;
    }
    string task = pchar.GreatRecipe.Fight.Pending;
    DeleteAttribute(pchar, "GreatRecipe.Fight.Pending");
    pchar.GreatRecipe.Fight.Active = task;
    if (task == "captain")
    {
        ref beard = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
        GreatRecipe_BeardFighter(beard);
        LAi_SetImmortal(beard, false);
        LAi_SetWarriorTypeNoGroup(beard);
        LAi_group_MoveCharacter(beard, "GreatRecipeFightGroup");
        ChangeCharacterAddressGroup(beard, "GreatRecipeDeck", "rld", "loc9");
        LAi_SetCheckMinHP(beard, 40.0, true, "");
    }
    int opponents = 1;
    if (task == "mutiny" || task == "mutiny_final") opponents = 3;
    if (task == "mutiny" && CheckAttribute(pchar, "GreatRecipe.Helped") && (!CheckAttribute(pchar, "GreatRecipe.Betrayed") || CheckAttribute(pchar, "GreatRecipe.Reconciled"))) opponents = 2;
    pchar.GreatRecipe.Fight.Remaining = opponents;
    for (int fighter = 0; task != "captain" && fighter < opponents; fighter++)
    {
        ref enemy = GreatRecipe_Actor("GreatRecipeFighter" + fighter, "pirate_2");
        int fencing = 65;
        string blade = "blade25";
        if (task == "training" && sti(pchar.GreatRecipe.TrainingWins) > 0) { fencing = 80; blade = "blade10"; }
        if (task == "mutiny_final") { fencing = 85; blade = "blade30"; }
        FantomMakeCoolFighter(enemy, sti(pchar.rank), fencing, 55, blade, "", 30);
        enemy.name = "Головорез"; enemy.lastname = "Ратли";
        if ((task == "mutiny" || task == "mutiny_final") && sti(pchar.GreatRecipe.Rebel)) enemy.lastname = "Бороды";
        enemy.GreatRecipe.Fighter = true;
        LAi_SetImmortal(enemy, false);
        LAi_SetCurHPMax(enemy);
        LAi_SetWarriorTypeNoGroup(enemy);
        LAi_group_MoveCharacter(enemy, "GreatRecipeFightGroup");
        ChangeCharacterAddressGroup(enemy, "GreatRecipeDeck", "rld", "loc" + (11 + fighter));
        LAi_SetCheckMinHP(enemy, 10.0, true, "");
    }
    LAi_SetImmortal(pchar, false);
    LAi_SetCheckMinHP(pchar, LAi_GetCharacterMaxHP(pchar) * 0.2, true, "");
    LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
    LAi_group_FightGroups("GreatRecipeFightGroup", LAI_GROUP_PLAYER, true);
    GreatRecipe_Locks();
    LAi_SetFightMode(pchar, true);
}

void GreatRecipe_CombatCheck()
{
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (LAi_GetCharacterHP(pchar) <= LAi_GetCharacterMaxHP(pchar) * 0.2) GreatRecipe_FightFinish(false);
    else GreatRecipe_FighterDown("");
}

void GreatRecipe_FighterDown(string unused)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (pchar.GreatRecipe.Fight.Active == "captain")
    {
        ref beard = characterFromId("GreatRecipeDreamBeard");
        if (LAi_GetCharacterHP(beard) <= 40.0) GreatRecipe_FightFinish(true);
        return;
    }
    int remaining = 0;
    for (int fighter = 0; fighter < 3; fighter++)
    {
        int index = GetCharacterIndex("GreatRecipeFighter" + fighter);
        if (index < 0) continue;
        ref enemy = GetCharacter(index);
        if (enemy.location != "GreatRecipeDeck") continue;
        if (LAi_GetCharacterHP(enemy) > 10.0 && !LAi_IsDead(enemy)) { remaining++; continue; }
        LAi_RemoveCheckMinHP(enemy); LAi_SetImmortal(enemy, true); LAi_SetStayType(enemy);
        LAi_group_MoveCharacter(enemy, "GreatRecipeHoldGroup");
    }
    if (remaining == 0) GreatRecipe_FightFinish(true);
}

void GreatRecipe_FightLost(string unused) { GreatRecipe_FightFinish(false); }

void GreatRecipe_FightFinish(bool won)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (LAi_GetCharacterHP(pchar) <= LAi_GetCharacterMaxHP(pchar) * 0.2) won = false;
    string task = pchar.GreatRecipe.Fight.Active;
    GreatRecipe_AllianceRecordFight(task, won);
    DeleteAttribute(pchar, "GreatRecipe.Fight.Active");
    LAi_RemoveCheckMinHP(pchar);
    LAi_SetImmortal(pchar, true);
    LAi_SetFightMode(pchar, false);
    LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
    LAi_group_SetAlarm("GreatRecipeFightGroup", LAI_GROUP_PLAYER, 0.0);
    if (task == "captain")
    {
        ref beard = characterFromId("GreatRecipeDreamBeard");
        LAi_RemoveCheckMinHP(beard); LAi_SetImmortal(beard, true); LAi_SetStayType(beard);
        LAi_group_MoveCharacter(beard, "GreatRecipeHoldGroup");
        ChangeCharacterAddressGroup(beard, "GreatRecipeCabin", "rld", "loc0");
        if (won) pchar.GreatRecipe.CaptainRespect = true;
        if (won) GreatRecipe_Journal("Борода уступил в поединке на палубе. Я заслужил уважение его клинка; теперь нужно удержать слово и довести корабельные дела до конца. В каюте можно поговорить о том, чем закончится наша морская дуэль.", false);
        else GreatRecipe_Journal("Я уступил Бороде в поединке. Он оставил меня в живых; за новым вызовом можно прийти в каюту. Корабельные дела продолжаются.", false);
    }
    for (int fighter = 0; fighter < 3; fighter++)
    {
        int index = GetCharacterIndex("GreatRecipeFighter" + fighter);
        if (index < 0) continue;
        ref enemy = GetCharacter(index);
        LAi_RemoveCheckMinHP(enemy); LAi_SetImmortal(enemy, true); LAi_SetStayType(enemy);
        ChangeCharacterAddress(enemy, "none", "");
    }
    if (task == "captain") { }
    else if (!won)
        GreatRecipe_Journal("Команда разняла драку. Я уступил, но жив; боцман разрешает вернуться к тому же делу после перевязки.", false);
    else if (task == "rations")
        GreatRecipe_TaskAdvance("rigging", "Ратли признался в краже пайков, Диего оправдан. Следующее дело — заклинивший грота-шкот; нужно расспросить Джека на палубе.");
    else if (task == "redeem")
    {
        pchar.GreatRecipe.Reconciled = true;
        GreatRecipe_Journal("Ратли вынужден признаться. Диего оправдан перед командой; можно вернуться за дубовым нагелем в трюм.", false);
    }
    else if (task == "training")
    {
        pchar.GreatRecipe.TrainingWins = sti(pchar.GreatRecipe.TrainingWins) + 1;
        if (sti(pchar.GreatRecipe.TrainingWins) >= 2) GreatRecipe_TaskAdvance("medicine", "Оба поединка у орудий выиграны. Лекарь в кубрике просит помочь раненому Диего: узнать, что нужно для перевязки.");
        else GreatRecipe_Journal("Первый поединок у орудий выигран. Поговорить с Джеком: следующий матрос предпочитает тяжёлые рубящие удары.", false);
    }
    else if (task == "powder_lesson") GreatRecipe_Journal("Джек остановил неверную сборку заряда. Вернуться к капитанской накладной, сверить цвет фитиля и вновь поговорить с канониром.", false);
    else if (task == "rigging_lesson") GreatRecipe_Journal("Джек показал, как держаться у натянутой снасти. Достать нагель и масло, ослабить шкот перед заменой блока; затем вернуться к канониру.", false);
    else if (task == "mutiny") GreatRecipe_TaskAdvance("mutiny_final", "Первый отряд сложил оружие, но носовую палубу удерживают трое опытных матросов. Капитан вызывает в каюту: поговорить с ним, затем вернуться к боцману перед решающей схваткой.");
    else
        GreatRecipe_TaskAdvance("ready", "Мятеж на палубе закончился. Борода зовёт в каюту: теперь он согласен поставить свой рецепт на кон в честной морской дуэли.");
    LAi_SetCurHPMax(pchar);
    LAi_SetImmortal(pchar, false);
    GreatRecipe_Locks();
}

void GreatRecipe_Ship(ref captain, int type, string name, int skill)
{
    DeleteAttribute(captain, "Ship");
    FantomMakeCoolSailor(captain, type, name, CANNON_TYPE_CANNON_LBS12, skill, skill, skill);
    captain.Ship.Cannons.Charge.Type = GOOD_AIRBURST;
    // Existing AI command preserves a chosen charge while finite cargo remains.
    captain.ShipCannonChargeType = GOOD_AIRBURST;
    SetCharacterGoods(captain, GOOD_AIRBURST, 480);
    SetCharacterGoods(captain, GOOD_POWDER, 800);
    SetCharacterGoods(captain, GOOD_BALLS, 0);
    SetCharacterGoods(captain, GOOD_BOMBS, 0);
    SetCharacterGoods(captain, GOOD_GRAPES, 0);
    SetCharacterGoods(captain, GOOD_KNIPPELS, 0);
    SetShipSkill(captain, skill, skill, skill, skill, skill, skill, skill, skill, skill);
    // The stock quest sailor adds these combat cheats; the duel does not.
    DeleteAttribute(captain, "perks");
    DeleteAttribute(captain, "TmpPerks");
    GreatRecipe_BeardFighter(captain);
    LAi_SetImmortal(captain, false);
    LAi_SetCurHPMax(captain);
    captain.DontRansackCaptain = true;
    captain.AlwaysEnemy = true;
}

void GreatRecipe_Boss(ref dream)
{
    GreatRecipe_Ship(dream, SHIP_FRIGATEQUEEN, "Красный прилив", 100);
    ref flagship = GetRealShip(sti(dream.Ship.Type));
    flagship.HP = stf(flagship.HP) * 24.0;
    if (stf(flagship.SpeedRate) < 18.0) flagship.SpeedRate = 18.0;
    if (stf(flagship.TurnRate) < 51.0) flagship.TurnRate = 51.0;
    dream.Ship.HP = flagship.HP;
    dream.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS32;
    dream.Ship.WhirlpoolControl = 1.0;
    dream.Ship.GreatRecipeAirburstOnly = true;
    SetCharacterGoods(dream, GOOD_AIRBURST, 4000);
    SetCharacterGoods(dream, GOOD_POWDER, 8000);
    SetCrewQuantityFull(dream);
    dream.Ship.Crew.Exp.Sailors = GetCrewExpRate();
    dream.Ship.Crew.Exp.Cannoners = GetCrewExpRate();
    dream.Ship.Crew.Exp.Soldiers = GetCrewExpRate();
    dream.Ship.Crew.Morale = MORALE_NORMAL;
    // Stock hull defence; finite reinforced hull still takes real cannon hits.
    SetCharacterPerk(dream, "ShipDefenseProfessional");
    SetCharacterPerk(dream, "FastReload");
    SetCharacterPerk(dream, "CannonProfessional");
    SetCharacterPerk(dream, "HullDamageUp");
    SetCharacterPerk(dream, "CrewDamageUp");
    SetCharacterPerk(dream, "ShipSpeedUp");
    SetCharacterPerk(dream, "ShipTurnRateUp");
    int load = RecalculateCargoLoad(dream);
    if (sti(flagship.Capacity) < load * 2) flagship.Capacity = load * 2;
}

void GreatRecipe_Group(ref login, ref captain)
{
    Group_DeleteGroup("GreatRecipeSea");
    Group_AddCharacter("GreatRecipeSea", captain.id);
    Group_SetType("GreatRecipeSea", "pirate");
    Group_SetGroupCommander("GreatRecipeSea", captain.id);
    Group_SetXZ_AY("GreatRecipeSea", stf(login.PlayerGroup.x) + 650.0, stf(login.PlayerGroup.z), 3.14);
    Group_SetTaskAttack("GreatRecipeSea", PLAYER_GROUP);
    Group_LockTask("GreatRecipeSea");
    login.QuestGroups.GreatRecipe = "GreatRecipeSea";
}

bool GreatRecipe_LoginFree(ref login)
{
    if (!CheckAttribute(login, "QuestGroups")) return true;
    aref groups; makearef(groups, login.QuestGroups);
    for (int group = 0; group < GetAttributesNum(groups); group++)
    {
        string id = GetAttributeValue(GetAttributeN(groups, group));
        if (id != "GreatRecipeSea" && id != "Sea_GreatRecipeDreamBeard") return false;
    }
    return true;
}

bool GreatRecipe_OwnMapLogin(ref login)
{
    if (!CheckAttribute(login, "Encounters") || !CheckAttribute(pchar, "GreatRecipe.MapCaptain")) return false;
    string id = "GreatRecipeDreamBeard";
    if (GreatRecipe_Phase() == "lost") id = "GreatRecipeRealBeard";
    if (pchar.GreatRecipe.MapCaptain != id) return false;
    aref encounters; makearef(encounters, login.Encounters);
    for (int slot = 0; slot < GetAttributesNum(encounters); slot++)
    {
        aref entry = GetAttributeN(encounters, slot);
        int index = sti(entry.type);
        if (index < 0 || index >= MAX_MAP_ENCOUNTERS) continue;
        ref encounter = GetMapEncounterRef(index);
        if (CheckAttribute(encounter, "CharacterID") && encounter.CharacterID == id &&
            sti(encounter.RealEncounterType) == ENCOUNTER_TYPE_ALONE) return true;
    }
    return false;
}

void GreatRecipe_DelayBossLogin(ref login)
{
    if (CheckAttribute(login, "QuestGroups"))
    {
        aref groups; makearef(groups, login.QuestGroups);
        for (int group = GetAttributesNum(groups) - 1; group >= 0; group--)
        {
            aref entry = GetAttributeN(groups, group);
            string id = GetAttributeValue(entry);
            if (id == "GreatRecipeSea" || id == "Sea_GreatRecipeDreamBeard")
                DeleteAttribute(login, "QuestGroups." + GetAttributeName(entry));
        }
    }
    Group_SetAddressNone("GreatRecipeSea");
}

void GreatRecipe_PrepareSea(ref login)
{
    GreatRecipe_Upgrade();
    string phase = GreatRecipe_Phase();
    if (login.Island != "Bermudes" && !GreatRecipe_OwnMapLogin(login)) return;
    if (phase == "search" && GreatRecipe_Moonlit() && GreatRecipe_LoginFree(login) &&
        !CheckAttribute(pchar, "Custody") && !bAbordageStarted && !bQuestDisableMapEnter && GreatRecipe_FleetFree())
    {
        if (!GreatRecipe_ShipRooms()) return;
        GreatRecipe_Snapshot();
        pchar.GreatRecipe.Phase = "whirlpool";
        pchar.GreatRecipe.Elapsed = 0;
        pchar.GreatRecipe.Vortex.X = stf(login.PlayerGroup.x) + 245.0;
        pchar.GreatRecipe.Vortex.Z = stf(login.PlayerGroup.z);
        pchar.GreatRecipe.Vortex.Radius = 700.0;
        pchar.GreatRecipe.Vortex.Intensity = 1.0;
        pchar.GreatRecipe.Vortex.Duration = 600.0;
        pchar.GreatRecipe.Version = 2;
        pchar.GreatRecipe.Vortex.Age = 0.0;
        ref dream = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
        GreatRecipe_Boss(dream);
        GreatRecipe_Group(login, dream);
        Group_SetXZ_AY("GreatRecipeSea", stf(pchar.GreatRecipe.Vortex.X) + 60.0,
            stf(pchar.GreatRecipe.Vortex.Z), 3.14);
        Group_SetTaskNone("GreatRecipeSea");
        GreatRecipe_DelayBossLogin(login);
        GreatRecipe_Locks();
        GreatRecipe_Journal("Остров под луной распался на кольца пены. Водоворот захватил всю эскадру. Из глубины поднимаются мачты огромного корабля — держаться вместе, орудия к бою!", false);
        return;
    }
    if (phase == "whirlpool" || phase == "assault" || phase == "boarding")
    {
        int dreamIndex = GetCharacterIndex("GreatRecipeDreamBeard");
        if (dreamIndex >= 0)
        {
            ref captain = GetCharacter(dreamIndex);
            if (CheckAttribute(pchar, "GreatRecipe.UpgradeBoss") && !LAi_IsDead(captain))
            {
                float fraction = stf(captain.Ship.HP) / GetCharacterShipHP(captain);
                GreatRecipe_Boss(captain);
                captain.Ship.HP = GetCharacterShipHP(captain) * fraction;
                DeleteAttribute(pchar, "GreatRecipe.UpgradeBoss");
            }
            GreatRecipe_Group(login, captain);
            if (CheckAttribute(captain, "Ship.Pos.x"))
                Group_SetXZ_AY("GreatRecipeSea", stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z), stf(captain.Ship.Ang.y));
            if (phase == "whirlpool") { Group_SetTaskNone("GreatRecipeSea"); GreatRecipe_DelayBossLogin(login); }
            if (phase == "boarding") Group_SetTaskNone("GreatRecipeSea");
        }
        return;
    }
    if (phase == "duel")
    {
        int index = GetCharacterIndex("GreatRecipeDuelBeard");
        if (index >= 0) GreatRecipe_Group(login, GetCharacter(index));
        return;
    }
    if (phase == "storm")
    {
        login.Storm = true;
        return;
    }
    // The real captain belongs to the failed branch, never to the dream reward.
    if (phase != "lost" || CheckAttribute(pchar, "GreatRecipe.HuntTaken")) return;
    // SeaLogin creates Lightning after this hook; weather owns admission.
    if (!GreatRecipe_Window() || !Whr_IsStorm() || !Whr_IsRain()) return;
    if (CheckAttribute(login, "QuestGroups")) return;
    ref real = GreatRecipe_Actor("GreatRecipeRealBeard", "RedBeard");
    if (!CheckAttribute(pchar, "GreatRecipe.HuntActive"))
    {
        GreatRecipe_Ship(real, SHIP_CORVETTE, "Громовая черепаха", 95);
        real.Ship.HP = 900;
        real.name = "Красная";
        real.lastname = "Борода";
        pchar.GreatRecipe.HuntActive = true;
    }
    if (!LAi_IsDead(real)) GreatRecipe_Group(login, real);
}

void GreatRecipe_ClearVortex(bool immediate)
{
    if (isEntity(&Sea))
    {
        if (immediate) Sea.Whirlpool.Clear = 1;
        else Sea.Whirlpool.Start = 0;
    }
    if (CheckAttribute(pchar, "GreatRecipe.Vortex.Sound"))
    {
        StopSound(sti(pchar.GreatRecipe.Vortex.Sound), 0);
        DeleteAttribute(pchar, "GreatRecipe.Vortex.Sound");
    }
}

void GreatRecipe_RestoreLocks()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Escrow")) return;
    chrDisableReloadToLocation = sti(pchar.GreatRecipe.Escrow.Reload);
    bDisableFastReload = sti(pchar.GreatRecipe.Escrow.FastReload);
    bDisableCharacterMenu = sti(pchar.GreatRecipe.Escrow.CharacterMenu);
    bQuestDisableMapEnter = sti(pchar.GreatRecipe.Escrow.Map);
}

void GreatRecipe_StartVortex()
{
    if (!isEntity(&Sea) || !CheckAttribute(&Sea, "Whirlpool.Active")) return;
    if (sti(Sea.Whirlpool.Active)) return;
    Sea.Whirlpool.CenterX = stf(pchar.GreatRecipe.Vortex.X);
    Sea.Whirlpool.CenterZ = stf(pchar.GreatRecipe.Vortex.Z);
    Sea.Whirlpool.Radius = stf(pchar.GreatRecipe.Vortex.Radius);
    Sea.Whirlpool.Intensity = stf(pchar.GreatRecipe.Vortex.Intensity);
    Sea.Whirlpool.Duration = stf(pchar.GreatRecipe.Vortex.Duration);
    Sea.Whirlpool.ResumeAge = stf(pchar.GreatRecipe.Vortex.Age);
    aref fleet; makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        saved.Vortex.Query = 0.0;
    }
    pchar.GreatRecipe.Vortex.LastSampleAge = stf(pchar.GreatRecipe.Vortex.Age);
    Sea.Whirlpool.Start = 1;
}

void GreatRecipe_VortexDamage()
{
    if (GreatRecipe_Phase() != "whirlpool" && GreatRecipe_Phase() != "assault") return;
    if (!CheckAttribute(&Sea, "Whirlpool.Active") || !isEntity(&Sea)) return;
    aref fleet; makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        ref captain = GetCharacter(sti(saved.Index));
        if (LAi_IsDead(captain) || LAi_IsImmortal(captain) || sti(captain.Ship.Type) == SHIP_NOTUSED) continue;
        if (!CheckAttribute(saved, "Vortex.Loss")) saved.Vortex.Loss = 0.0;
        if (!CheckAttribute(saved, "Vortex.Query")) saved.Vortex.Query = 0.0;
        float fraction = 0.0;
        SendMessage(&Sea, "lae", MSG_SEA_WHIRLPOOL_CONTACT, captain, &fraction);
        float delta = fraction - stf(saved.Vortex.Query);
        saved.Vortex.Query = fraction;
        if (delta > 0.5 - stf(saved.Vortex.Loss)) delta = 0.5 - stf(saved.Vortex.Loss);
        if (delta <= 0.0) continue;
        saved.Vortex.Loss = stf(saved.Vortex.Loss) + delta;
        float damage = stf(saved.RealShip.HP) * delta;
        // Water exposure alone leaves an already damaged hull alive; gunfire does not.
        if (damage > stf(captain.Ship.HP) - 1.0) damage = stf(captain.Ship.HP) - 1.0;
        if (damage > 0.0) Ship_ApplyHullHitpoints(captain, damage, KILL_BY_TOUCH, -1);
    }
}

void GreatRecipe_Queue()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Generation")) pchar.GreatRecipe.Generation = 0;
    pchar.GreatRecipe.Generation = sti(pchar.GreatRecipe.Generation) + 1;
    PostEvent("GreatRecipePulse", 500, "l", sti(pchar.GreatRecipe.Generation));
}

void GreatRecipe_SeaReady()
{
    if (!GreatRecipe_Locked()) return;
    GreatRecipe_Locks();
    if (GreatRecipe_Phase() == "whirlpool" || GreatRecipe_Phase() == "assault")
    {
        // All live admitted ships fit inside the authored field after fleet placement.
        if (!CheckAttribute(pchar, "GreatRecipe.Vortex.Placed"))
        {
            float minX = stf(pchar.Ship.Pos.x);
            float maxX = minX;
            float minZ = stf(pchar.Ship.Pos.z);
            float maxZ = minZ;
            for (int member = 0; member < COMPANION_MAX; member++)
            {
                int memberIndex = GetCompanionIndex(pchar, member);
                if (memberIndex < 0) continue;
                ref ship = GetCharacter(memberIndex);
                if (!CheckAttribute(ship, "Ship.Pos.x")) continue;
                float sx = stf(ship.Ship.Pos.x);
                float sz = stf(ship.Ship.Pos.z);
                if (sx < minX) minX = sx;
                if (sx > maxX) maxX = sx;
                if (sz < minZ) minZ = sz;
                if (sz > maxZ) maxZ = sz;
            }
            pchar.GreatRecipe.Vortex.X = (minX + maxX) * 0.5 + 120.0;
            pchar.GreatRecipe.Vortex.Z = (minZ + maxZ) * 0.5;
            float radius = 700.0;
            for (int slot = 0; slot < COMPANION_MAX; slot++)
            {
                int index = GetCompanionIndex(pchar, slot);
                if (index < 0) continue;
                ref captain = GetCharacter(index);
                if (!CheckAttribute(captain, "Ship.Pos.x")) continue;
                float x = stf(captain.Ship.Pos.x) - stf(pchar.GreatRecipe.Vortex.X);
                float z = stf(captain.Ship.Pos.z) - stf(pchar.GreatRecipe.Vortex.Z);
                float extent = sqrt(x * x + z * z) + 240.0;
                if (extent > radius) radius = extent;
            }
            if (radius > 1600.0)
            {
                pchar.GreatRecipe.Cancelled = true;
                pchar.GreatRecipe.Won = false;
                pchar.GreatRecipe.Phase = "returning";
                GreatRecipe_Journal("Эскадра растянулась слишком далеко. Нужно собрать корабли вместе и снова проверить открытую воду у Бермуд в ясную ночь, с 22 до 4 часов, между 12-м и 18-м числами.", false);
                GreatRecipe_Delay("GreatRecipe_Return", 0.1);
                return;
            }
            pchar.GreatRecipe.Vortex.Radius = radius;
            pchar.GreatRecipe.Vortex.Placed = true;
        }
        GreatRecipe_StartVortex();
        if (CheckAttribute(pchar, "GreatRecipe.Vortex.Sound")) StopSound(sti(pchar.GreatRecipe.Vortex.Sound), 0);
        pchar.GreatRecipe.Vortex.Sound = Play3DSound("storm_sea_water", stf(pchar.GreatRecipe.Vortex.X), 0.0, stf(pchar.GreatRecipe.Vortex.Z));
    }
    if (GreatRecipe_Phase() == "storm") Whr_RequestWeather(1.0, 1.0, 28.0, 2.0, false, true);
    GreatRecipe_Queue();
}

void GreatRecipe_Capture(string unused)
{
    if (GreatRecipe_Phase() != "capturing") return;
    if (CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched) || sQuestNameAfterReload != "_")
    {
        GreatRecipe_Delay("GreatRecipe_Capture", 1.0);
        return;
    }
    GreatRecipe_ClearVortex(true);
    pchar.GreatRecipe.Phase = "captive";
    pchar.GreatRecipe.Task = "rations";
    LAi_SetCurHPMax(pchar);
    LAi_SetImmortal(pchar, false);
    Group_DeleteGroup("GreatRecipeSea");
    // The stock deck branch is a void reload operation behind a bool wrapper.
    // Its cleanup/reload owns the transition; its return value is not admission.
    if (bSeaActive && bAbordageStarted)
    {
        DoQuestReloadToLocation("GreatRecipeHold", "goto", "goto3", "");
        return;
    }
    if (!DoQuestReloadToLocation("GreatRecipeHold", "goto", "goto3", ""))
    {
        pchar.GreatRecipe.Phase = "returning";
        pchar.GreatRecipe.Won = false;
        GreatRecipe_Return("");
    }
}

void GreatRecipe_BeginCapture(bool bossBeaten)
{
    if (GreatRecipe_Phase() != "whirlpool" && GreatRecipe_Phase() != "assault" && GreatRecipe_Phase() != "boarding") return;
    pchar.GreatRecipe.BossBeaten = bossBeaten;
    pchar.GreatRecipe.Phase = "capturing";
    GreatRecipe_ClearVortex(true);
    LAi_SetImmortal(pchar, true);
    Group_SetTaskNone("GreatRecipeSea");
    GreatRecipe_Journal("Борода приказал прекратить огонь. Его люди спускают шлюпки; впереди чужой трюм и разговор с капитаном.", false);
    GreatRecipe_Delay("GreatRecipe_Capture", 1.0);
}

bool GreatRecipe_EmergeBoss()
{
    if (!bSeaActive || !bSeaLoaded || bAbordageStarted) return false;
    int index = GetCharacterIndex("GreatRecipeDreamBeard");
    if (index >= 0 && !LAi_IsDead(GetCharacter(index)))
    {
        ref captain = GetCharacter(index);
        if (!IsEntity(captain))
        {
            if (Group_FindGroup("GreatRecipeSea") < 0)
            {
                Group_AddCharacter("GreatRecipeSea", captain.id);
                Group_SetType("GreatRecipeSea", "pirate");
                Group_SetGroupCommander("GreatRecipeSea", captain.id);
                Group_SetTaskNone("GreatRecipeSea");
                Group_LockTask("GreatRecipeSea");
            }
            ref group = Group_GetGroupByID("GreatRecipeSea");
            DeleteAttribute(group, "AlreadyLoaded");
            Group_SetXZ_AY("GreatRecipeSea", stf(pchar.GreatRecipe.Vortex.X) + stf(pchar.GreatRecipe.Vortex.Radius) * 0.12,
                stf(pchar.GreatRecipe.Vortex.Z), 3.14);
            Sea_LoginGroup("GreatRecipeSea");
        }
        if (IsEntity(captain))
        {
            pchar.GreatRecipe.Emerged = true;
            DeleteAttribute(pchar, "GreatRecipe.EmergeRetries");
            return true;
        }
    }
    if (!CheckAttribute(pchar, "GreatRecipe.EmergeRetries")) pchar.GreatRecipe.EmergeRetries = 0;
    pchar.GreatRecipe.EmergeRetries = sti(pchar.GreatRecipe.EmergeRetries) + 1;
    if (index < 0 || sti(pchar.GreatRecipe.EmergeRetries) >= 10)
    {
        pchar.GreatRecipe.Cancelled = true;
        pchar.GreatRecipe.Won = false;
        pchar.GreatRecipe.Phase = "returning";
        GreatRecipe_ClearVortex(true);
        GreatRecipe_Journal("Мачты скрылись в пене, а воронка распалась. Вернуться в открытую воду у Бермуд в ясную ночь, с 22 до 4 часов между 12-м и 18-м числами. Моя эскадра останется при мне.", false);
        GreatRecipe_Delay("GreatRecipe_Return", 0.1);
    }
    return false;
}

void GreatRecipe_FirstBoarding(string unused)
{
    if (GreatRecipe_Phase() != "boarding" || bAbordageStarted) return;
    if (!bSeaActive || !bSeaLoaded || sti(InterfaceStates.Launched) || CheckAttribute(pchar, "pause"))
    {
        GreatRecipe_Delay("GreatRecipe_FirstBoarding", 0.5);
        return;
    }
    if (!GreatRecipe_EmergeBoss())
    {
        if (GreatRecipe_Phase() == "boarding") GreatRecipe_Delay("GreatRecipe_FirstBoarding", 0.5);
        return;
    }
    int index = GetCharacterIndex("GreatRecipeDreamBeard");
    pchar.GreatRecipe.BoardingStarted = true;
    LAi_SetImmortal(pchar, false);
    Sea_AbordageStartNow(SHIP_ABORDAGE, index, false, false);
}

bool GreatRecipe_ClaimHullDefeat(ref captain)
{
    if (sti(captain.index) != nMainCharacterIndex) return false;
    if (GreatRecipe_Phase() != "whirlpool" && GreatRecipe_Phase() != "assault") return false;
    // This hook runs only after real damage has reached zero: preserve the
    // wreck long enough for stock enemy-initiated boarding, not a live HP floor.
    captain.Ship.HP = 1.0;
    pchar.GreatRecipe.Phase = "boarding";
    GreatRecipe_ClearVortex(true);
    Group_SetTaskNone("GreatRecipeSea");
    LAi_SetImmortal(pchar, true);
    GreatRecipe_Journal("Корпус разбит. Люди Бороды идут на абордаж — защищать палубу; если одолею капитана, его трофей останется за мной.", false);
    GreatRecipe_Delay("GreatRecipe_FirstBoarding", 0.1);
    return true;
}

void GreatRecipe_LaunchDuel(string unused)
{
    if (GreatRecipe_Phase() != "captive" || pchar.location != "GreatRecipeCabin") return;
    if (pchar.GreatRecipe.Task != "ready" || CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    ref beard = GreatRecipe_Actor("GreatRecipeDuelBeard", "RedBeard");
    GreatRecipe_Ship(beard, SHIP_SLOOP, "Рецепт Бороды", 80);
    // Copy the same generated ship: no independent roll can bias hull/rigging.
    aref source, target;
    makearef(source, beard.Ship);
    makearef(target, pchar.Ship);
    GreatRecipe_Copy(target, source);
    pchar.Ship.Name = "Жемчуг черепахи";
    DeleteAttribute(pchar, "Ship.SeaAI");
    DeleteAttribute(pchar, "Ship.CannonMode");
    pchar.GreatRecipe.Phase = "duel";
    pchar.GreatRecipe.Elapsed = 0;
    GreatRecipe_Locks();
    LAi_SetPlayerType(pchar);
    LAi_SetImmortal(pchar, false);
    GreatRecipe_Journal("Мне достался такой же шлюп, как Бороде; людей, пушек и шрапнели поровну. Секрет жемчуга он отдаст, если я потоплю или захвачу его корабль. Мы вышли один на один.", false);
    QuestToSeaLogin_Prepare(0.0, 0.0, "Bermudes");
    GreatRecipe_Group(&questToSeaLoginer, beard);
    QuestToSeaLogin_Launch();
}

void GreatRecipe_Win()
{
    if (GreatRecipe_Phase() != "duel") return;
    GreatRecipe_AllianceNavalWin(false);
    pchar.GreatRecipe.Phase = "storm";
    pchar.GreatRecipe.Won = true;
    pchar.GreatRecipe.Elapsed = 0;
    GreatRecipe_Journal("Борода проиграл. Его секрет — шрапнельное ядро, а не жемчуг. Но море закрылось штормом: дуэльный корабль уже не спасти.", false);
    GreatRecipe_Delay("GreatRecipe_LaunchStorm", 1.0);
}

void GreatRecipe_LaunchStorm(string unused)
{
    if (GreatRecipe_Phase() != "storm") return;
    if (bAbordageStarted || CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched))
    {
        GreatRecipe_Delay("GreatRecipe_LaunchStorm", 1.0);
        return;
    }
    Group_DeleteGroup("GreatRecipeSea");
    if (bSeaActive) DeleteSeaEnvironment();
    QuestToSeaLogin_Prepare(0.0, 0.0, "Bermudes");
    questToSeaLoginer.Storm = true;
    QuestToSeaLogin_Launch();
}

bool GreatRecipe_ClaimDeath(int index)
{
    string phase = GreatRecipe_Phase();
    if ((phase == "whirlpool" || phase == "assault" || phase == "boarding") && index == GetCharacterIndex("GreatRecipeDreamBeard"))
    {
        pchar.GreatRecipe.EarlyWin = true;
        pchar.GreatRecipe.Won = true;
        pchar.GreatRecipe.Phase = "loot";
        GreatRecipe_ClearVortex(true);
        GreatRecipe_RestoreLocks();
        GreatRecipe_SpawnBarrel(GetCharacter(index), false);
        GreatRecipe_Journal("Красный прилив побеждён всей эскадрой. У его борта плавает трофейная бочка — подойти и поднять её. Если уплывёт, искать бочонок на глобальной карте. Красную жемчужину отнести Лаки Росину в бермудскую таверну.", false);
        return false;
    }
    if (phase == "duel" && index == GetCharacterIndex("GreatRecipeDuelBeard"))
    {
        GreatRecipe_Win();
        return false; // The opposing ship still sinks through normal damage.
    }
    if (index == GetCharacterIndex("GreatRecipeRealBeard") && phase == "lost" &&
        !CheckAttribute(pchar, "GreatRecipe.HuntTaken"))
    {
        pchar.GreatRecipe.HuntTaken = true;
        pchar.GreatRecipe.Phase = "pearl_search";
        GreatRecipe_SpawnBarrel(GetCharacter(index), false);
        GreatRecipe_Journal("Настоящий Борода побеждён. Поднять трофейную бочку у его корвета; если она уплывёт, искать бочонок на глобальной карте. Красную жемчужину принести Лаки Росину в бермудскую таверну.", false);
        return false;
    }
    if (index != nMainCharacterIndex || !GreatRecipe_Locked()) return false;
    if (phase == "whirlpool" || phase == "assault" || phase == "boarding")
    {
        GreatRecipe_BeginCapture(false);
        return true;
    }
    if (phase == "capturing") return true;
    if (phase == "captive")
    {
        if (CheckAttribute(pchar, "GreatRecipe.Fight.Active")) GreatRecipe_FightFinish(false);
        LAi_SetCurHPMax(pchar);
        return true;
    }
    if (phase == "returning") return true;
    pchar.GreatRecipe.Won = phase == "storm";
    pchar.GreatRecipe.Phase = "returning";
    pchar.Ship.HP = 1;
    LAi_SetCurHPMax(pchar);
    GreatRecipe_Delay("GreatRecipe_Return", 0.1);
    return true;
}

void GreatRecipe_ClaimBoarding(int index)
{
    if ((GreatRecipe_Phase() == "whirlpool" || GreatRecipe_Phase() == "assault" || GreatRecipe_Phase() == "boarding") && index == GetCharacterIndex("GreatRecipeDreamBeard"))
        GreatRecipe_ClaimDeath(index);
    if ((GreatRecipe_Phase() == "whirlpool" || GreatRecipe_Phase() == "assault" || GreatRecipe_Phase() == "boarding") && index == nMainCharacterIndex)
        GreatRecipe_BeginCapture(false);
    if (GreatRecipe_Phase() == "duel" && index == GetCharacterIndex("GreatRecipeDuelBeard"))
    {
        GreatRecipe_AllianceNavalWin(GreatRecipe_AllianceSpared());
        GreatRecipe_Win();
    }
    if (GreatRecipe_Phase() == "lost" && index == GetCharacterIndex("GreatRecipeRealBeard"))
    {
        GreatRecipe_ClaimDeath(index);
    }
}

bool GreatRecipe_RestoreFleet(bool won)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet")) return false;
    if (IsEntity(pchar)) return false;
    aref fleet, ship, ai, target, links, source;
    makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int live = 0; live < GetAttributesNum(fleet); live++)
    {
        aref owner = GetAttributeN(fleet, live);
        if (IsEntity(GetCharacter(sti(owner.Index)))) return false;
    }
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        int index = sti(saved.Index);
        ref captain = GetCharacter(index);
        if (index != nMainCharacterIndex && CheckAttribute(saved, "Character"))
        {
            makearef(source, saved.Character);
            DeleteAttribute(captain, "");
            CopyAttributes(captain, source);
        }
        makearef(ai, saved.AI);
        makearef(target, captain.chr_ai);
        GreatRecipe_Copy(target, ai);
        makearef(ship, saved.Ship);
        makearef(target, captain.Ship);
        GreatRecipe_Copy(target, ship);
        SetCharacterShipLocation(captain, "Pirates_town");
    }
    if (CheckAttribute(pchar, "GreatRecipe.Escrow.Character"))
    {
        DeleteAttribute(&GreatRecipeRestoreBuffer, "");
        makearef(source, pchar.GreatRecipe.Escrow.Character);
        makearef(target, GreatRecipeRestoreBuffer.Character);
        CopyAttributes(target, source);
        makearef(source, pchar.GreatRecipe);
        makearef(target, GreatRecipeRestoreBuffer.State);
        CopyAttributes(target, source);
        makearef(source, pchar.QuestInfo);
        makearef(target, GreatRecipeRestoreBuffer.Journal);
        CopyAttributes(target, source);
        makearef(source, pchar.SystemJournal);
        makearef(target, GreatRecipeRestoreBuffer.SystemJournal);
        CopyAttributes(target, source);
        makearef(source, pchar.location);
        makearef(target, GreatRecipeRestoreBuffer.Location);
        CopyAttributes(target, source);
        DeleteAttribute(pchar, "");
        makearef(source, GreatRecipeRestoreBuffer.Character);
        CopyAttributes(pchar, source);
        makearef(source, GreatRecipeRestoreBuffer.State);
        makearef(target, pchar.GreatRecipe);
        CopyAttributes(target, source);
        makearef(source, GreatRecipeRestoreBuffer.Journal);
        makearef(target, pchar.QuestInfo);
        GreatRecipe_Copy(target, source);
        makearef(source, GreatRecipeRestoreBuffer.SystemJournal);
        makearef(target, pchar.SystemJournal);
        GreatRecipe_Copy(target, source);
        makearef(source, GreatRecipeRestoreBuffer.Location);
        makearef(target, pchar.location);
        GreatRecipe_Copy(target, source);
        DeleteAttribute(&GreatRecipeRestoreBuffer, "");
        if (CheckAttribute(pchar, "GreatRecipe.Loot.EscrowGrant"))
        {
            GreatRecipe_GrantLoot(pchar.GreatRecipe.Loot.EscrowGrant == "full");
            DeleteAttribute(pchar, "GreatRecipe.Loot.EscrowGrant");
        }
    }
    makearef(links, pchar.GreatRecipe.Escrow.Companions);
    makearef(target, pchar.Fellows.Companions);
    GreatRecipe_Copy(target, links);
    if (won)
    {
        // The dream actors release only their own temporary ships.
        for (int actor = 0; actor < 2; actor++)
        {
            string id = "GreatRecipeDreamBeard";
            if (actor == 1) id = "GreatRecipeDuelBeard";
            int actorIndex = GetCharacterIndex(id);
            if (actorIndex >= 0) Characters[actorIndex].Ship.Type = SHIP_NOTUSED;
        }
        int rewardType = FindFirstEmptyShip();
        bool newReward = rewardType >= 0;
        if (newReward)
        {
            aref originalRealShip;
            makearef(originalRealShip, pchar.GreatRecipe.Escrow.Fleet.s0.RealShip);
            CopyAttributes(&RealShips[rewardType], originalRealShip);
            RealShips[rewardType].index = rewardType;
            RealShips[rewardType].lock = true;
            DeleteAttribute(&RealShips[rewardType], "StoreShip");
        }
        else
        {
            // A full registry still returns the reserved original ship/crew.
            // Its row may have another consumer: never rewrite that shared row.
            rewardType = sti(pchar.GreatRecipe.Escrow.Fleet.s0.Ship.Type);
        }
        pchar.Ship.Type = rewardType;
        // Original generated/unique real-ship row preserves every upgrade.
        pchar.Ship.HP = GetCharacterShipHP(pchar);
        pchar.Ship.SP = GetCharacterShipSP(pchar);
        DeleteAttribute(pchar, "Ship.Masts");
        DeleteAttribute(pchar, "Ship.Sails");
        DeleteAttribute(pchar, "Ship.Blots");
        DeleteAttribute(pchar, "Ship.Cannons.Borts");
        if (newReward) SetCrewQuantityFull(pchar);
        ref rewardShip = GetRealShip(sti(pchar.Ship.Type));
        int rewardLoad = RecalculateCargoLoad(pchar);
        if (newReward && sti(rewardShip.Capacity) < rewardLoad) rewardShip.Capacity = rewardLoad;
    }
    return true;
}

void GreatRecipe_Return(string unused)
{
    if (GreatRecipe_Phase() != "returning") return;
    GreatRecipe_ClearVortex(true);
    if (bAbordageStarted || sti(InterfaceStates.Launched) || CheckAttribute(pchar, "pause") || sQuestNameAfterReload != "_")
    {
        GreatRecipe_Delay("GreatRecipe_Return", 1.0);
        return;
    }
    Group_DeleteGroup("GreatRecipeSea");
    if (pchar.location == "Pirates_town" && CheckAttribute(pchar, "GreatRecipe.Restored")) GreatRecipe_Returned();
    else if (!DoQuestReloadToLocation("Pirates_town", "reload", "reload4", ""))
        GreatRecipe_Delay("GreatRecipe_Return", 1.0);
}

void GreatRecipe_AfterSceneUnload()
{
    DeleteAttribute(pchar, "GreatRecipe.Loot.SeaPending");
    bool seaLoot = GreatRecipe_Phase() == "loot";
    if (!seaLoot && GreatRecipe_Phase() != "returning") return;
    if (seaLoot) pchar.GreatRecipe.Phase = "returning";
    // Stock logoff detaches native characters; never replace a linked root.
    LogoffCharacter(pchar);
    aref fleet; makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        LogoffCharacter(GetCharacter(sti(saved.Index)));
    }
    if (!CheckAttribute(pchar, "GreatRecipe.Restored"))
    {
        bool won = sti(pchar.GreatRecipe.Won) && !seaLoot;
        if (!GreatRecipe_RestoreFleet(won)) return;
        LAi_SetCurHP(pchar, stf(pchar.GreatRecipe.Escrow.Health));
        GreatRecipe_RestoreLocks();
        pchar.GreatRecipe.Restored = true;
        pchar.GreatRecipe.Generation = sti(pchar.GreatRecipe.Generation) + 1;
    }
    if (seaLoot)
    {
        pchar.GreatRecipe.Phase = "pearl_search";
        DeleteAttribute(pchar, "GreatRecipe.Escrow");
        DeleteAttribute(pchar, "GreatRecipe.Restored");
    }
    for (int actor = 0; actor < 11; actor++)
    {
        string id = "GreatRecipeBosun";
        if (actor == 1) id = "GreatRecipeDreamBeard";
        if (actor == 2) id = "GreatRecipeDuelBeard";
        if (actor == 3) id = "GreatRecipeCook";
        if (actor == 4) id = "GreatRecipePrisoner";
        if (actor == 5) id = "GreatRecipeGunner";
        if (actor == 6) id = "GreatRecipeMate";
        if (actor == 7) id = "GreatRecipeSurgeon";
        if (actor > 7) id = "GreatRecipeFighter" + (actor - 8);
        int index = GetCharacterIndex(id);
        if (index >= 0) ChangeCharacterAddress(GetCharacter(index), "none", "");
    }
}

void GreatRecipe_Returned()
{
    if (GreatRecipe_Phase() != "returning" || !CheckAttribute(pchar, "GreatRecipe.Restored")) return;
    bool won = sti(pchar.GreatRecipe.Won);
    if (won) pchar.GreatRecipe.Phase = "pearl_search";
    else pchar.GreatRecipe.Phase = "lost";
    LAi_SetPlayerType(pchar);
    string body = "Кошмар кончился на Бермудах. Исходная эскадра и вещи целы. Искать Красную Бороду в открытой воде у Бермуд во время грозы, с 22 до 4 часов между 12-м и 18-м числами. Победа даст трофейную бочку: красную жемчужину нужно отнести Лаки Росину, чтобы начать изготовление шрапнельных бомб.";
    if (won) body = "Кошмар закончился на Бермудах. Моя эскадра и вещи целы, флагман полностью восстановлен. Но красная жемчужина пропала с дуэльным судном: искать трофейную бочку на глобальной карте, затем отнести жемчужину Лаки Росину в таверну. Магазин пока её не производит.";
    if (CheckAttribute(pchar, "GreatRecipe.Cancelled"))
    {
        pchar.GreatRecipe.Phase = "search";
        DeleteAttribute(pchar, "GreatRecipe.Cancelled");
    }
    else { GreatRecipe_Journal(body, false); Log_Info(body); }
    LAi_group_Delete("GreatRecipeHoldGroup");
    LAi_group_Delete("GreatRecipeFightGroup");
    DeleteAttribute(pchar, "GreatRecipe.Escrow");
    DeleteAttribute(pchar, "GreatRecipe.Restored");
    GreatRecipe_AllianceReturned();
}

void GreatRecipe_LocationLoaded(aref location)
{
    if (GreatRecipe_Phase() == "captive" && (location.id == "GreatRecipeHold" || location.id == "GreatRecipeDeck" || location.id == "GreatRecipeMess" || location.id == "GreatRecipeCabin"))
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Task")) pchar.GreatRecipe.Task = "rations";
        // Sea entities have already logged off. Keep original captain identities
        // in escrow; neither captivity nor the later duel admits their ships.
        if (!CheckAttribute(pchar, "GreatRecipe.FleetDetached"))
        {
            for (int slot = 1; slot < COMPANION_MAX; slot++)
            {
                int index = GetCompanionIndex(pchar, slot);
                if (index >= 0) RemoveCharacterCompanion(pchar, GetCharacter(index));
            }
            pchar.GreatRecipe.FleetDetached = true;
        }
        GreatRecipe_CaptiveActors();
    }
    if (GreatRecipe_Phase() == "returning")
    {
        if (location.id == "Pirates_town") GreatRecipe_Returned();
        else GreatRecipe_Delay("GreatRecipe_Return", 0.1);
    }
}

void GreatRecipe_OnLoad()
{
    GreatRecipe_EnsureProduction();
    GreatRecipe_Upgrade();
    DeleteAttribute(pchar, "GreatRecipe.Loot.SeaPending");
    GreatRecipe_AllianceOnLoad();
    GreatRecipe_RearmDeferred();
    if (!GreatRecipe_Locked()) return;
    if (GreatRecipe_Phase() == "captive") GreatRecipe_ShipRooms();
    DeleteAttribute(pchar, "GreatRecipe.Vortex.Sound");
    if (isEntity(&Sea)) Sea.Whirlpool.Clear = 1;
    if (GreatRecipe_Phase() == "returning")
    {
        GreatRecipe_Delay("GreatRecipe_Return", 0.1);
        return;
    }
    if (GreatRecipe_Phase() == "boarding" && !bAbordageStarted)
        GreatRecipe_Delay("GreatRecipe_FirstBoarding", 0.1);
    if (GreatRecipe_Phase() == "capturing") GreatRecipe_Delay("GreatRecipe_Capture", 0.1);
    int index = FindLoadedLocation();
    if (index >= 0) GreatRecipe_LocationLoaded(&Locations[index]);
    if (bSeaActive && bSeaLoaded) GreatRecipe_SeaReady();
}

void GreatRecipe_PreLoadActors()
{
    GreatRecipe_EnsurePearl();
    // Before OnLoad rebuilds location/sea entities, refresh only our saved actors.
    for (int role = 0; role < 3; role++)
    {
        string id = "GreatRecipeDreamBeard";
        if (role == 1) id = "GreatRecipeDuelBeard";
        if (role == 2) id = "GreatRecipeRealBeard";
        int index = GetCharacterIndex(id);
        if (index < 0) continue;
        ref actor = GetCharacter(index);
        if (IsEntity(actor)) LogoffCharacter(actor);
        actor.model = "RedBeard";
        actor.FaceId = 2006;
    }
}

void GreatRecipe_Upgrade()
{
    if (GreatRecipe_Phase() == "" || (CheckAttribute(pchar, "GreatRecipe.Version") && sti(pchar.GreatRecipe.Version) >= 2)) return;
    string phase = GreatRecipe_Phase();
    if (phase == "lost" && CheckAttribute(pchar, "GreatRecipe.HuntTaken"))
    {
        pchar.GreatRecipe.Phase = "pearl_search";
        GreatRecipe_Journal("Борода уже побеждён, но красной жемчужины нет. Искать трофейную бочку на глобальной карте; жемчужину отнести Лаки Росину в таверну на Бермудах.", false);
    }
    if (phase == "captive" && !CheckAttribute(pchar, "GreatRecipe.Task"))
    {
        pchar.GreatRecipe.Task = "rations";
        if (sti(pchar.GreatRecipe.Days) >= 3 || sti(pchar.GreatRecipe.Notice) >= 2) pchar.GreatRecipe.Task = "ready";
    }
    if (phase == "whirlpool" || phase == "assault")
    {
        pchar.GreatRecipe.UpgradeBoss = true;
        if (CheckAttribute(pchar, "GreatRecipe.Vortex.AppliedLoss"))
        {
            float oldLoss = stf(pchar.GreatRecipe.Vortex.AppliedLoss);
            if (oldLoss < 0.0) oldLoss = 0.0;
            if (oldLoss > 0.5) oldLoss = 0.5;
            pchar.GreatRecipe.Escrow.Fleet.s0.Vortex.Loss = oldLoss;
        }
        pchar.GreatRecipe.Vortex.Age = stf(pchar.GreatRecipe.Elapsed);
        pchar.GreatRecipe.Vortex.Radius = 700.0;
        pchar.GreatRecipe.Vortex.Duration = 600.0;
        DeleteAttribute(pchar, "GreatRecipe.Vortex.Placed");
        // Remove only the former dream's scripted immortality, using its escrow.
        aref fleet; makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
        for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
        {
            aref saved = GetAttributeN(fleet, slot);
            ref captain = GetCharacter(sti(saved.Index));
            LAi_SetImmortal(captain, CheckAttribute(saved, "AI.immortal") && sti(saved.AI.immortal));
        }
    }
    pchar.GreatRecipe.Version = 2;
}

void GreatRecipe_Pulse()
{
    int generation = GetEventData();
    if (!CheckAttribute(pchar, "GreatRecipe.Generation") || generation != sti(pchar.GreatRecipe.Generation)) return;
    if (!bSeaActive || !bSeaLoaded || !GreatRecipe_Locked()) return;
    if (bAbordageStarted || CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched))
    {
        PostEvent("GreatRecipePulse", 500, "l", generation);
        return;
    }
    string phase = GreatRecipe_Phase();
    if (phase != "whirlpool" && phase != "assault") pchar.GreatRecipe.Elapsed = stf(pchar.GreatRecipe.Elapsed) + 0.5;
    float elapsed = stf(pchar.GreatRecipe.Elapsed);
    if (phase == "whirlpool" || phase == "assault")
    {
        if (!CheckAttribute(&Sea, "Whirlpool.Age"))
        {
            PostEvent("GreatRecipePulse", 500, "l", generation);
            return;
        }
        elapsed = stf(Sea.Whirlpool.Age);
        pchar.GreatRecipe.Vortex.Age = elapsed;
        float lastSample = stf(pchar.GreatRecipe.Vortex.LastSampleAge);
        if (elapsed < stf(pchar.GreatRecipe.Vortex.Duration) && elapsed < lastSample + 1.0)
        {
            PostEvent("GreatRecipePulse", 500, "l", generation);
            return;
        }
        pchar.GreatRecipe.Vortex.LastSampleAge = elapsed;
        GreatRecipe_VortexDamage();
        // Sparse supporting foam; native water/ship response owns the vortex.
        float centerX = stf(pchar.GreatRecipe.Vortex.X);
        float centerZ = stf(pchar.GreatRecipe.Vortex.Z);
        float radius = stf(pchar.GreatRecipe.Vortex.Radius) * 0.8;
        for (int arm = 0; elapsed < 40.0 && arm < 6; arm++)
        {
            float angle = elapsed * 1.2 + arm * 1.0472;
            float spiralRadius = radius * (0.45 + arm * 0.11);
            string foam = "splash";
            if (arm % 2 == 0) foam = "splash_big";
            CreateParticleSystem(foam, centerX + cos(angle) * spiralRadius,
                0.0, centerZ + sin(angle) * spiralRadius, 0.0, 0.0, 0.0, 5);
        }
        if (phase == "whirlpool" && elapsed >= 20.0)
        {
            if (!GreatRecipe_EmergeBoss())
            {
                PostEvent("GreatRecipePulse", 500, "l", generation);
                return;
            }
            pchar.GreatRecipe.Phase = "assault";
            pchar.GreatRecipe.Elapsed = 0;
            Group_SetTaskAttack("GreatRecipeSea", PLAYER_GROUP);
            GreatRecipe_Journal("Из воронки вышел Красный прилив — быстрый бронированный фрегат Бороды. Его шрапнель рвёт паруса и людей; сам он идёт по кругу будто по тихой воде. Разворачивать эскадру, отвечать бортовым огнём!", false);
        }
    }
    if (phase == "storm")
    {
        float hp = GetCharacterShipHP(pchar) * (1.0 - elapsed / 20.0);
        if (hp < 1.0) hp = 1.0;
        if (stf(pchar.Ship.HP) > hp) pchar.Ship.HP = hp;
        if (elapsed >= 20.0)
        {
            Ship_Detonate(pchar, false, false);
            pchar.GreatRecipe.Won = true;
            pchar.GreatRecipe.Phase = "returning";
            GreatRecipe_Delay("GreatRecipe_Return", 0.5);
            return;
        }
    }
    PostEvent("GreatRecipePulse", 500, "l", generation);
}

// Bermuda's dream owns its fleet escrow; ordinary sea/death paths remain stock.
#event_handler("GreatRecipePulse", "GreatRecipe_Pulse");
#event_handler("GreatRecipeMapAttack", "GreatRecipe_MapAttack");
#event_handler("WorldMap_TrafficFrame", "GreatRecipe_MapApproach");
#event_handler("GreatRecipeDeferred", "GreatRecipe_Deferred");
#event_handler("CharactersStateUpdate", "GreatRecipe_CombatCheck");
object GreatRecipeRestoreBuffer;
string GreatRecipeMapTraceClock = "";
bool GreatRecipeMapDebug = false;
float GreatRecipeMapHuntElapsed = 0.0;

bool GreatRecipe_DeferredFunction(string function)
{
    return function == "GreatRecipe_Capture" || function == "GreatRecipe_FirstBoarding" ||
        function == "GreatRecipe_LaunchStorm" || function == "GreatRecipe_Return" ||
        function == "GreatRecipe_ShoreLand" || function == "GreatRecipe_ShoreReturn";
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
    for (int pending = 0; pending < 6; pending++)
    {
        string function = "GreatRecipe_Capture";
        if (pending == 1) function = "GreatRecipe_FirstBoarding";
        if (pending == 2) function = "GreatRecipe_LaunchStorm";
        if (pending == 3) function = "GreatRecipe_Return";
        if (pending == 4) function = "GreatRecipe_ShoreLand";
        if (pending == 5) function = "GreatRecipe_ShoreReturn";
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

string GreatRecipe_SupplyName(int supply)
{
    if (supply == 0) return "Oil";
    if (supply == 1) return "Pin";
    if (supply == 2) return "Linen";
    if (supply == 3) return "Water";
    return "Fuse";
}

int GreatRecipe_SupplyIndex(string name)
{
    return Items_FindItemIdx("GreatRecipe" + name);
}

void GreatRecipe_EnsureSupplies()
{
    for (int supply = 0; supply < 5; supply++)
    {
        string name = GreatRecipe_SupplyName(supply);
        if (GreatRecipe_SupplyIndex(name) >= 0) continue;
        for (int item = 0; item < ITEMS_QUANTITY; item++)
        {
            if (!CheckAttribute(&Items[item], "id") || Items[item].id != "item" + (item + 1)) continue;
            ref material = &Items[item];
            DeleteAttribute(material, "");
            material.id = "GreatRecipe" + name;
            material.index = item;
            material.name = "itmname_GreatRecipe" + name;
            material.describe = "itmdescr_GreatRecipe" + name;
            material.model = "pursel";
            material.picIndex = 9;
            material.picTexture = "ITEMS_1";
            if (name == "Oil" || name == "Water") { material.model = "potion"; material.picIndex = 7; }
            material.ItemType = "QUESTITEMS";
            material.Weight = 0.2;
            material.price = 0;
            material.rare = 0;
            material.uniq = true;
            material.shown = false;
            material.startLocation = ""; material.startLocator = "";
            material.useLocation = ""; material.useLocator = "";
            break;
        }
    }
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
    GreatRecipe_EnsureSupplies();
    GreatRecipe_ShoreItems();
    return index + 1;
}

void GreatRecipe_EnsurePearl()
{
    // Old saves restore the item catalogue; NewGame's InitItems is not replayed.
    int empty = -1;
    bool found = false;
    for (int item = 0; item < ITEMS_QUANTITY; item++)
    {
        if (CheckAttribute(&Items[item], "id") && Items[item].id == "GreatRecipeRedPearl") found = true;
        if (empty < 0 && CheckAttribute(&Items[item], "id") && Items[item].id == "item" + (item + 1)) empty = item;
    }
    if (!found && empty >= 0) GreatRecipe_InitPearl(empty);
    GreatRecipe_EnsureSupplies();
    GreatRecipe_ShoreItems();
}

void GreatRecipe_PreLoadItems(aref location)
{
    GreatRecipe_EnsurePearl();
    for (int supply = 0; supply < 5; supply++)
    {
        string name = GreatRecipe_SupplyName(supply);
        int index = GreatRecipe_SupplyIndex(name);
        if (index < 0) continue;
        ref material = &Items[index];
        // A completed old task is authoritative; never invent its materials,
        // treatment, journal history or alliance deeds during migration.
        bool pending = false;
        if (GreatRecipe_Phase() == "captive" && CheckAttribute(pchar, "GreatRecipe.Task") && !CheckAttribute(pchar, "GreatRecipe.Fight.Active"))
        {
            string task = pchar.GreatRecipe.Task;
            pending = (task == "rigging" && (name == "Oil" || name == "Pin")) ||
                (task == "medicine" && (name == "Linen" || name == "Water")) ||
                (task == "powder" && name == "Fuse");
            if (name == "Pin" && CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")) pending = false;
        }
        material.shown = false;
        material.useLocation = ""; material.useLocator = "";
        if (!pending) continue;
        string room = "GreatRecipeMess";
        string locator = "gr_oil";
        if (name == "Pin") { room = "GreatRecipeHold"; locator = "gr_pin"; }
        if (name == "Linen") { room = "GreatRecipeHold"; locator = "gr_linen"; }
        if (name == "Water") locator = "gr_water";
        if (name == "Fuse") locator = "gr_fuse";
        material.startLocation = room;
        material.startLocator = locator;
        if (GetCharacterItem(pchar, material.id) == 0) material.shown = true;
        if (name == "Pin" || name == "Fuse")
        {
            // The stock use action validates and consumes this actual item.
            material.useLocation = "GreatRecipeDeck";
            material.useLocator = "gr_button_rigging";
            if (name == "Fuse") material.useLocator = "gr_button_powder";
        }
    }
}

void GreatRecipe_TakeSupply(aref location, string itemId)
{
    if (GreatRecipe_Phase() != "captive") return;
    for (int supply = 0; supply < 5; supply++)
    {
        string name = GreatRecipe_SupplyName(supply);
        if (itemId != "GreatRecipe" + name || GetCharacterItem(pchar, itemId) == 0) continue;
        pchar.GreatRecipe.Supply.(name) = true;
    }
}

bool GreatRecipe_CanUseSupply(aref location, string locator, string itemId)
{
    if (itemId != "GreatRecipePin" && itemId != "GreatRecipeFuse") return true;
    bool valid = GreatRecipe_Phase() == "captive" && location.id == "GreatRecipeDeck" && !CheckAttribute(pchar, "GreatRecipe.Fight.Active");
    if (itemId == "GreatRecipePin") valid = valid && locator == "gr_button_rigging" && pchar.GreatRecipe.Task == "rigging" && CheckAttribute(pchar, "GreatRecipe.Clue.Block") && GetCharacterItem(pchar, "GreatRecipeOil") > 0;
    else valid = valid && locator == "gr_button_powder" && pchar.GreatRecipe.Task == "powder" && CheckAttribute(pchar, "GreatRecipe.Clue.Manifest");
    if (!valid) Log_Info("Сначала сверить работу с Джеком: для блока нужны нагель и масло, для заряда — капитанская накладная.");
    return valid;
}

void GreatRecipe_UseSupply(aref location, string locator, string itemId)
{
    if (itemId != "GreatRecipePin" && itemId != "GreatRecipeFuse") return;
    bool valid = GreatRecipe_CanUseSupply(location, locator, itemId);
    if (!valid)
    {
        // Stock has consumed the active item before this callback. Restore it
        // when the second material/instruction is missing, without advancing.
        GiveItem2Character(pchar, itemId);
        int unusedIndex = GreatRecipe_SupplyIndex("Pin");
        if (itemId == "GreatRecipeFuse") unusedIndex = GreatRecipe_SupplyIndex("Fuse");
        if (unusedIndex < 0) return;
        ref unused = &Items[unusedIndex];
        unused.shown = false;
        DeleteAttribute(unused, "shown.used");
        return;
    }
    if (itemId == "GreatRecipePin")
    {
        TakeItemFromCharacter(pchar, "GreatRecipeOil");
        pchar.GreatRecipe.World.BlockRepaired = true;
        GreatRecipe_TaskAdvance("medicine", "Блок снова ходит. Джек предложил тренировку у орудий, но из трюма зовут лекаря: Диего стало хуже.");
    }
    else
    {
        pchar.GreatRecipe.World.ChargeReady = true;
        GreatRecipe_TaskAdvance("shore", "У орудийного поста я заменил подложенный фитиль сухим красным и сверил заряд по накладной. Борода вызывает в каюту: ему нужна судовая книга из тайника в лунной бухте.");
    }
    int usedIndex = GreatRecipe_SupplyIndex("Pin");
    if (itemId == "GreatRecipeFuse") usedIndex = GreatRecipe_SupplyIndex("Fuse");
    if (usedIndex < 0) return;
    ref material = &Items[usedIndex];
    material.shown = false;
    material.useLocation = ""; material.useLocator = "";
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
    if (!CheckAttribute(pchar, "GreatRecipe.Loot.BundleTaken")) pchar.GreatRecipe.Loot.BundlePending = true;
    GreatRecipe_GrantLoot(false);
    if (CheckAttribute(pchar, "GreatRecipe.Escrow"))
    {
        pchar.GreatRecipe.Loot.EscrowGrant = "pearl";
    }
    DeleteAttribute(pchar, "GreatRecipe.Loot.MapID");
    DeleteAttribute(pchar, "GreatRecipe.Loot.SeaPending");
    GreatRecipe_Journal("В бочке оказалась тяжёлая красная жемчужина с клеймом Бороды. Лаки ждал вестей об этой истории.", false);
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
    GreatRecipe_Journal("Лаки передал жемчужину мастерам. На первую партию им нужна неделя; продавать будут в бермудском магазине, небольшими партиями.", false);
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
        GreatRecipe_Journal("Бермудские мастера наладили изготовление шрапнели. Первая партия поступила в магазин.", true);
        GreatRecipe_FinalRewards();
    }
}

void GreatRecipe_RewardRepair()
{
    if (GreatRecipe_Phase() != "won" || !CheckAttribute(pchar, "GreatRecipe.Rewards.RepairPending") ||
        (CheckAttribute(pchar, "GreatRecipe.Cancelled") && sti(pchar.GreatRecipe.Cancelled)) ||
        CheckAttribute(pchar, "GreatRecipe.Escrow") || bSeaActive || IsEntity(pchar) ||
        GetCharacterShipType(pchar) == SHIP_NOTUSED) return;
    pchar.Ship.HP = GetCharacterShipHP(pchar);
    pchar.Ship.SP = GetCharacterShipSP(pchar);
    DeleteAttribute(pchar, "Ship.Masts");
    DeleteAttribute(pchar, "Ship.Sails");
    Ship_ResetHullDamage(pchar);
    DeleteAttribute(pchar, "Ship.Cannons.Borts");
    DeleteAttribute(pchar, "GreatRecipe.Rewards.RepairPending");
    pchar.GreatRecipe.Rewards.RepairPaid = true;
}

void GreatRecipe_FinalRewards()
{
    if (GreatRecipe_Phase() != "won" || CheckAttribute(pchar, "GreatRecipe.Escrow") ||
        (CheckAttribute(pchar, "GreatRecipe.Cancelled") && sti(pchar.GreatRecipe.Cancelled))) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Rewards.Paid"))
    {
        int authority = 250;
        bool honest = !CheckAttribute(pchar, "GreatRecipe.Betrayed") || CheckAttribute(pchar, "GreatRecipe.Reconciled");
        if (honest && CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") &&
            CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && CheckAttribute(pchar, "GreatRecipe.Clue.Alibi")) authority += 100;
        if (CheckAttribute(pchar, "GreatRecipe.Helped"))
        {
            authority += 50;
            if (honest) ChangeCharacterReputation(pchar, 2);
        }
        if (CheckAttribute(pchar, "GreatRecipe.CaptainRespect")) authority += 50;
        AddCharacterExpToSkill(pchar, "Leadership", authority);
        AddCharacterExpToSkill(pchar, "Sailing", 200);
        int lessons = 0;
        if (CheckAttribute(pchar, "GreatRecipe.TrainingWins")) lessons = sti(pchar.GreatRecipe.TrainingWins);
        if (lessons > 2) lessons = 2;
        if (lessons > 0) AddCharacterExpToSkill(pchar, "Fencing", lessons * 50);
        if (CheckAttribute(pchar, "GreatRecipe.World.BlockRepaired")) AddCharacterExpToSkill(pchar, "Repair", 100);
        if (CheckAttribute(pchar, "GreatRecipe.World.ChargeReady")) AddCharacterExpToSkill(pchar, "Cannons", 100);
        if (CheckAttribute(pchar, "GreatRecipe.Loot.BundlePending"))
        {
            GiveItem2Character(pchar, "blade32");
            GiveItem2Character(pchar, "pistol4");
            AddMoneyToCharacter(pchar, 100000);
            pchar.GreatRecipe.Loot.BundleTaken = true;
            DeleteAttribute(pchar, "GreatRecipe.Loot.BundlePending");
        }
        if (CheckAttribute(pchar, "GreatRecipe.Rewards.ShorePending"))
        {
            GiveItem2Character(pchar, "potion2"); GiveItem2Character(pchar, "potion2");
            GiveItem2Character(pchar, "jewelry5");
            DeleteAttribute(pchar, "GreatRecipe.Rewards.ShorePending");
        }
        pchar.GreatRecipe.Rewards.Paid = true;
    }
    GreatRecipe_RewardRepair();
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

float GreatRecipe_MapRouteX() { return stf(worldMap.islands.Bermudes.position.x) - 300.0; }
float GreatRecipe_MapRouteStart() { return stf(worldMap.islands.Bermudes.position.z) - 100.0; }
float GreatRecipe_MapRouteEnd() { return stf(worldMap.islands.Bermudes.position.z) - 60.0; }

bool GreatRecipe_MapWestPoint(float x, float z)
{
    float west = GreatRecipe_MapRouteX();
    float start = GreatRecipe_MapRouteStart();
    return abs(x - west) <= 40.0 && z >= start - 40.0 && z <= start + 80.0 &&
        (SendMessage(&worldMap, "lfff", MSG_WORLDMAP_NAVIGABLE_POINT, x, z, 40.0) != 0);
}

bool GreatRecipe_MapGuide(string id)
{
    return id == "GreatRecipeGuide" && CheckAttribute(&worldMap, "encounters." + id + ".GreatRecipeGuide") &&
        CheckAttribute(&worldMap, "encounters." + id + ".authoredPatrol.loaded") && sti(worldMap.encounters.GreatRecipeGuide.authoredPatrol.loaded);
}

bool GreatRecipe_MapImportBlocked(string id)
{
    if (id == "GreatRecipeGuide" && CheckAttribute(&worldMap, "encounters." + id + ".GreatRecipeGuide")) return true;
    string path = "encounters." + id + ".sinking.source";
    return CheckAttribute(&worldMap, path) && worldMap.(path) == "GreatRecipeGuide";
}

void GreatRecipe_MapClearGuide()
{
    if (CheckAttribute(&worldMap, "encounters.GreatRecipeGuide"))
    {
        if (IsEntity(&worldMap)) worldMap.retireEncounter = "GreatRecipeGuide";
        DeleteAttribute(&worldMap, "encounters.GreatRecipeGuide");
    }
    DeleteAttribute(&worldMap, "GreatRecipeZoneAttack");
    DeleteAttribute(&worldMap, "GreatRecipeGuideRestoreAttempt");
    GreatRecipeMapHuntElapsed = 0.0;
}

void GreatRecipe_MapBuildGuide()
{
    worldMap.encounters.GreatRecipeGuide.type = "Merchant";
    worldMap.encounters.GreatRecipeGuide.modelName = Nations[PIRATE].worldMapShip + "_bark";
    worldMap.encounters.GreatRecipeGuide.GreatRecipeGuide = true;
    worldMap.encounters.GreatRecipeGuide.qID = "GreatRecipeGuide";
    worldMap.encounters.GreatRecipeGuide.x = GreatRecipe_MapRouteX();
    worldMap.encounters.GreatRecipeGuide.z = GreatRecipe_MapRouteStart();
    worldMap.encounters.GreatRecipeGuide.gotoX = worldMap.encounters.GreatRecipeGuide.x;
    worldMap.encounters.GreatRecipeGuide.gotoZ = GreatRecipe_MapRouteEnd();
    worldMap.encounters.GreatRecipeGuide.gotoRad = 2.0;
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.returnX = worldMap.encounters.GreatRecipeGuide.x;
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.returnZ = worldMap.encounters.GreatRecipeGuide.z;
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.radius = 2.0;
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.nightKey = GreatRecipe_MapNightKey();
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.loaded = false;
    worldMap.encounters.GreatRecipeGuide.speed = 0.0;
    worldMap.encounters.GreatRecipeGuide.kMaxSpeed = 0.6;
    worldMap.encounters.GreatRecipeGuide.isEnableKill = false;
    worldMap.encounters.GreatRecipeGuide.isEnemy = false;
    worldMap.encounters.GreatRecipeGuide.killMe = false;
    worldMap.encounters.GreatRecipeGuide.brnAlpha = 1.0;
}

string GreatRecipe_MapNightKey()
{
    int day = GetDataDay();
    if (GetTime() < 4.0) day--;
    return GetDataYear() + ":" + GetDataMonth() + ":" + day;
}

bool GreatRecipe_MapSameNight()
{
    return CheckAttribute(&worldMap, "encounters.GreatRecipeGuide.authoredPatrol.nightKey") &&
        worldMap.encounters.GreatRecipeGuide.authoredPatrol.nightKey == GreatRecipe_MapNightKey();
}

void GreatRecipe_MapPatrolWindow()
{
    if (GreatRecipe_Phase() != "search" || !GreatRecipe_Moonlit()) { GreatRecipe_MapClearGuide(); return; }
    if (!GreatRecipe_MapSameNight())
    { GreatRecipe_MapClearGuide(); GreatRecipe_MapBuildGuide(); }
    if (GreatRecipe_MapGuide("GreatRecipeGuide")) return;
    if (CheckAttribute(&worldMap, "GreatRecipeGuideRestoreAttempt")) return;
    worldMap.GreatRecipeGuideRestoreAttempt = true;
    worldMap.restoreEncounter = "GreatRecipeGuide";
    if (!GreatRecipe_MapGuide("GreatRecipeGuide") && GreatRecipeMapDebug) Trace("GreatRecipe map: native patrol admission failed");
}

void GreatRecipe_MapSeed()
{
    // Called after the stock off-map collector, before native restoration.
    // Same-night saved position/route survives; daylight never restores a ghost.
    DeleteAttribute(&worldMap, "GreatRecipeZoneAttack");
    DeleteAttribute(&worldMap, "GreatRecipeGuideBlocked");
    DeleteAttribute(&worldMap, "GreatRecipeGuideRestoreAttempt");
    GreatRecipeMapHuntElapsed = 0.0;
    if (GreatRecipe_Phase() != "search") { GreatRecipe_MapClearGuide(); return; }
    aref encounters; makearef(encounters, worldMap.encounters);
    for (int slot = GetAttributesNum(encounters) - 1; slot >= 0; slot--)
    {
        aref old = GetAttributeN(encounters, slot);
        if (CheckAttribute(old, "quest.chrID") && old.quest.chrID == "GreatRecipeDreamBeard")
        {
            DeleteAttribute(old, "quest.event");
            wdmDeleteLoginEncounter(GetAttributeName(old));
        }
    }
    if (CheckAttribute(pchar, "GreatRecipe.MapCaptain") && pchar.GreatRecipe.MapCaptain == "GreatRecipeDreamBeard") DeleteAttribute(pchar, "GreatRecipe.MapCaptain");
    if (CheckAttribute(pchar, "GreatRecipe.MapReissue.ID") && pchar.GreatRecipe.MapReissue.ID == "GreatRecipeDreamBeard") DeleteAttribute(pchar, "GreatRecipe.MapReissue");
    Map_ReleaseQuestEncounter("GreatRecipeDreamBeard");
    if (!GreatRecipe_Moonlit()) { GreatRecipe_MapClearGuide(); return; }
    if (!GreatRecipe_MapSameNight())
    { GreatRecipe_MapClearGuide(); GreatRecipe_MapBuildGuide(); }
    worldMap.encounters.GreatRecipeGuide.authoredPatrol.loaded = false;
    worldMap.encounters.GreatRecipeGuide.brnAlpha = 1.0;
}

bool GreatRecipe_MapNearGuide()
{
    if (!GreatRecipe_MapGuide("GreatRecipeGuide")) return false;
    float dx = stf(worldMap.playerShipX) - stf(worldMap.encounters.GreatRecipeGuide.x);
    float dz = stf(worldMap.playerShipZ) - stf(worldMap.encounters.GreatRecipeGuide.z);
    return dx * dx + dz * dz <= 10000.0 &&
        (SendMessage(&worldMap, "lfff", MSG_WORLDMAP_NAVIGABLE_POINT, stf(worldMap.playerShipX), stf(worldMap.playerShipZ), 40.0) != 0);
}

bool GreatRecipe_MapGuideSelected(int index)
{
    if (index < 0 || !wdmSetCurrentShipData(index)) return false;
    return GreatRecipe_MapImportBlocked(worldMap.encounter.id);
}

void GreatRecipe_MapApproach()
{
    // Native TrafficFrame follows the player's movement/coordinate publication.
    // Calendar throttling must never throttle physical approach.
    if (GreatRecipe_Phase() != "search" || !IsEntity(&worldMap) || wdmLockReload || sti(InterfaceStates.Launched) ||
        CheckAttribute(pchar, "pause") || !CheckAttribute(&worldMap, "date.hourPerSec") || stf(worldMap.date.hourPerSec) <= 0.0) return;
    GreatRecipe_MapHunt(makefloat(GetDeltaTime()) * 0.001);
    GreatRecipe_QueueMapAttack();
}

bool GreatRecipe_MapPrey(aref encounter)
{
    if (!WdmTrafficIsOrdinary(encounter) || CheckAttribute(encounter, "qID") || CheckAttribute(encounter, "encdata.quest") ||
        CheckAttribute(encounter, "sinking") || (CheckAttribute(encounter, "trafficRetired") && sti(encounter.trafficRetired)) ||
        (CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea)) || !CheckAttribute(encounter, "encdata.trafficRoster.count") ||
        !CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z")) return false;
    string id = GetAttributeName(encounter);
    for (int companion = 0; companion < COMPANION_MAX; companion++)
    {
        int index = GetCompanionIndex(pchar, companion);
        if (index < 0) continue;
        ref captain = GetCharacter(index);
        if (CheckAttribute(captain, "trafficFleetID") && captain.trafficFleetID == id) return false;
    }
    aref roster; makearef(roster, encounter.encdata.trafficRoster);
    int count = sti(roster.count);
    if (count <= 0 || count >= GetAttributesNum(roster)) return false;
    bool alive = false;
    for (int slot = 0; slot < count; slot++)
    {
        string key = "ship" + slot;
        if (!CheckAttribute(roster, key)) return false;
        aref hull; makearef(hull, roster.(key));
        if (CheckAttribute(hull, "seaLoaded") && sti(hull.seaLoaded)) return false;
        if (!CheckAttribute(hull, "dead") || !sti(hull.dead)) alive = true;
    }
    return alive;
}

void GreatRecipe_MapHunt(float delta)
{
    if (!GreatRecipe_Moonlit() || !GreatRecipe_MapGuide("GreatRecipeGuide")) return;
    GreatRecipeMapHuntElapsed = GreatRecipeMapHuntElapsed + delta;
    if (GreatRecipeMapHuntElapsed < 0.25) return;
    GreatRecipeMapHuntElapsed = 0.0;
    aref encounters; makearef(encounters, worldMap.encounters);
    for (int target = 0; target < GetAttributesNum(encounters); target++)
    {
        aref encounter = GetAttributeN(encounters, target);
        if (!CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z")) continue;
        float dx = stf(encounter.x) - stf(worldMap.encounters.GreatRecipeGuide.x);
        float dz = stf(encounter.z) - stf(worldMap.encounters.GreatRecipeGuide.z);
        if (dx * dx + dz * dz > 2500.0 || !GreatRecipe_MapPrey(encounter)) continue;
        aref fleet; makearef(fleet, encounter.encdata);
        aref roster; makearef(roster, fleet.trafficRoster);
        for (int slot = 0; slot < sti(roster.count); slot++)
        {
            aref hull;
            string key = "ship" + slot; makearef(hull, roster.(key));
            if (CheckAttribute(hull, "dead") && sti(hull.dead)) continue;
            hull.dead = true; hull.hp = 0.0; hull.trafficLoss = "sunk";
            if (CheckAttribute(hull, "Ship.HP")) hull.Ship.HP = 0.0;
            WdmTrafficLoseCargo(hull);
        }
        WdmTrafficRefreshRosterCounts(fleet);
        encounter.trafficPower = WdmTrafficRosterPower(fleet);
        encounter.trafficRetired = true;
        encounter.sinking.age = 0.0; encounter.sinking.duration = 1.0;
        encounter.sinking.source = "GreatRecipeGuide";
        encounter.needDelete = "GreatRecipe sunk";
        worldMap.encounters.GreatRecipeGuide.authoredPatrol.attackTarget = GetAttributeName(encounter);
        worldMap.deleteUpdate = "";
        if (GreatRecipeMapDebug) Trace("GreatRecipe map: ordinary fleet sunk " + GetAttributeName(encounter));
        return;
    }
}

float GreatRecipe_MapReissueSeconds()
{
    float hours = GetPastTime("day", sti(pchar.GreatRecipe.MapReissue.Year), sti(pchar.GreatRecipe.MapReissue.Month), sti(pchar.GreatRecipe.MapReissue.Day), 0.0,
        sti(worldMap.date.year), sti(worldMap.date.month), sti(worldMap.date.day), 0.0) * 24.0 + stf(worldMap.date.hour) - stf(pchar.GreatRecipe.MapReissue.Hour);
    return (stf(pchar.GreatRecipe.MapReissue.Hours) - hours) / stf(worldMap.date.hourPerSec);
}

string GreatRecipe_FirstBattleBlock()
{
    if (CheckAttribute(pchar, "Custody")) return "custody";
    if (bAbordageStarted) return "boarding";
    if (bQuestDisableMapEnter) return "map locked";
    if (!GreatRecipe_FleetFree()) return "protected companion";
    return "";
}

bool GreatRecipe_MapAttackEligible()
{
    if (GreatRecipe_Phase() != "search" || !IsEntity(&worldMap) || wdmLockReload || sti(InterfaceStates.Launched) ||
        CheckAttribute(pchar, "pause") || !CheckAttribute(&worldMap, "date.hourPerSec") || stf(worldMap.date.hourPerSec) <= 0.0 ||
        !CheckAttribute(&worldMap, "playerShipX") || !CheckAttribute(&worldMap, "playerShipZ")) return false;
    if (CheckAttribute(pchar, "GreatRecipe.MapCaptain") && pchar.GreatRecipe.MapCaptain != "GreatRecipeDreamBeard") return false;
    if (!GreatRecipe_Moonlit()) return false;
    return GreatRecipe_MapNearGuide();
}

void GreatRecipe_QueueMapAttack()
{
    if (CheckAttribute(&worldMap, "GreatRecipeZoneAttack") || !GreatRecipe_MapAttackEligible()) return;
    if (GreatRecipe_FirstBattleBlock() != "" || CheckAttribute(&worldMap, "GreatRecipeGuideBlocked")) return;
    if (!GreatRecipe_ShipRooms())
    {
        worldMap.GreatRecipeGuideBlocked = true;
        Trace("GreatRecipe map: approach blocked room templates/capacity");
        return;
    }
    worldMap.GreatRecipeZoneAttack = true;
    PostEvent("GreatRecipeMapAttack", 1);
}

void GreatRecipe_MapAttack()
{
    if (!CheckAttribute(&worldMap, "GreatRecipeZoneAttack")) return;
    if (!GreatRecipe_MapAttackEligible() || GreatRecipe_FirstBattleBlock() != "")
    { DeleteAttribute(&worldMap, "GreatRecipeZoneAttack"); Trace("GreatRecipe map: zone dispatch cancelled"); return; }
    Trace("GreatRecipe map: zone attack x=" + worldMap.playerShipX + " z=" + worldMap.playerShipZ);
    SendMessage(&worldMap, "l", MSG_WORLDMAP_ENTER_SEA_DIRECT);
}

void GreatRecipe_MapAdmission(ref login)
{
    Trace("GreatRecipe map: admission phase=" + GreatRecipe_Phase() + " time=" + GetTime() +
        " queued=" + CheckAttribute(&worldMap, "GreatRecipeZoneAttack"));
    DeleteAttribute(login, "GreatRecipeOffshore");
    DeleteAttribute(login, "GreatRecipeMapAttack");
    if (GreatRecipe_Phase() != "search" || !IsEntity(&worldMap) || !GreatRecipe_Moonlit() ||
        !CheckAttribute(&worldMap, "playerShipX") || !CheckAttribute(&worldMap, "playerShipZ")) return;
    if (!CheckAttribute(&worldMap, "GreatRecipeZoneAttack") || !GreatRecipe_MapNearGuide()) return;
    login.GreatRecipeOffshore = true;
    login.GreatRecipeMapAttack = "GreatRecipeDreamBeard";
    Trace("GreatRecipe map: admission confirmed time=" + GetTime());
    DeleteAttribute(&worldMap, "GreatRecipeZoneAttack");
    // Keep the guide if PrepareSea rejects a foreign quest/fleet. The next map
    // restores it at the same position; advancing phase removes it on creation.
}
bool GreatRecipe_MapCreateWest(string id, float seconds)
{
    // Native creation clamps TTL below three seconds; never extend an expiring marker.
    if (seconds < 3.0) { Trace("GreatRecipe map: expired ttl=" + seconds); return false; }
    float x = GreatRecipe_MapRouteX();
    float z = GreatRecipe_MapRouteStart();
    for (int point = 0; point < 5; point++)
    {
        if (!GreatRecipe_MapWestPoint(x, z + point * 10.0))
        { Trace("GreatRecipe map: navigation rejected x=" + x + " z=" + (z + point * 10.0)); return false; }
        if (!SendMessage(&worldMap, "lfff", MSG_WORLDMAP_WATER_POINT, x, z + point * 10.0, 40.0))
        { Trace("GreatRecipe map: occupied x=" + x + " z=" + (z + point * 10.0)); return false; }
    }
    int slot = -1;
    if (!GenerateMapEncounter_Alone(id, &slot))
    { Trace("GreatRecipe map: encounter allocation rejected captain=" + id + " slot=" + slot); return false; }
    string model = wdmEncounterModelName(slot);
    // Same stock creation owner, with exact remaining native seconds rather than a new day.
    bool created = SendMessage(&worldMap, "lsffffff", MSG_WORLDMAP_CREATEENC_MER_XZ, model, x, z, x, z + 40.0, 0.2, seconds);
    Trace("GreatRecipe map: create=" + created + " captain=" + id + " model=" + model + " x=" + x + " z=" + z + " ttl=" + seconds);
    if (created)
    {
        ref encounter = GetMapEncounterRef(slot);
        WdmCopyEncounterData(encounter, worldMap.EncounterID1);
        string path = "encounters." + worldMap.EncounterID1;
        worldMap.(path).quest.event = "Map_TraderSucces";
        worldMap.(path).quest.chrID = id;
        pchar.GreatRecipe.MapCaptain = id;
    }
    ManualReleaseMapEncounter(slot);
    wdmReconcileQuestShipCounter();
    return created;
}

string GreatRecipe_MapTraceValue(aref data, string path)
{
    if (!CheckAttribute(data, path)) return "missing";
    return data.(path);
}

void GreatRecipe_MapCalendar()
{
    if (!IsEntity(&worldMap) || wdmLockReload) return;
    GreatRecipe_MapPatrolWindow();
    string phase = GreatRecipe_Phase();
    // Hourly file-only evidence for the actual player route; no quest state or UI changes.
    if (GreatRecipeMapDebug)
    {
    string traceClock = worldMap.date.year + ":" + worldMap.date.month + ":" + worldMap.date.day + ":" + sti(worldMap.date.hour) + ":" + phase;
    if (GreatRecipeMapTraceClock != traceClock)
    {
        GreatRecipeMapTraceClock = traceClock;
        string captainID = "none";
        if (CheckAttribute(pchar, "GreatRecipe.MapCaptain")) captainID = pchar.GreatRecipe.MapCaptain;
        Trace("GreatRecipe map: clock=" + traceClock + " time=" + GetTime() + " locked=" + GreatRecipe_Locked() +
            " rain=" + Whr_IsRain() + " storm=" + Whr_IsStorm() + " captain=" + captainID +
            " playerX=" + GreatRecipe_MapTraceValue(&worldMap, "playerShipX") + " playerZ=" + GreatRecipe_MapTraceValue(&worldMap, "playerShipZ") +
            " hoursPerSec=" + GreatRecipe_MapTraceValue(&worldMap, "date.hourPerSec") + " previousClock=" + GreatRecipe_MapTraceValue(pchar, "GreatRecipe.MapClock"));
    }
    }
    if (GreatRecipe_Locked() || phase == "search") return;
    if (!CheckAttribute(&worldMap, "date.hourPerSec") || stf(worldMap.date.hourPerSec) <= 0.0) return;
    if (CheckAttribute(pchar, "GreatRecipe.MapCaptain") &&
        pchar.GreatRecipe.MapCaptain != "GreatRecipeDreamBeard" && pchar.GreatRecipe.MapCaptain != "GreatRecipeRealBeard") return;
    if (phase != "search" && phase != "lost" && !CheckAttribute(pchar, "GreatRecipe.MapCaptain")) return;
    string clock = worldMap.date.year + ":" + worldMap.date.month + ":" + worldMap.date.day + ":" + sti(worldMap.date.hour) + ":" + GreatRecipe_Phase();
    if (phase != "search" && CheckAttribute(pchar, "GreatRecipe.MapReissue")) clock = clock + ":retry:" + makeint(stf(worldMap.date.hour) * 60.0);
    if (CheckAttribute(pchar, "GreatRecipe.MapClock") && pchar.GreatRecipe.MapClock == clock) return;
    pchar.GreatRecipe.MapClock = clock;
    if (CheckAttribute(pchar, "GreatRecipe.MapCaptain"))
    {
        // A ship first sighted at night remains reachable for its native day
        // lifetime; dawn does not erase it while the player is approaching.
        aref encounters; makearef(encounters, worldMap.encounters);
        for (int active = 0; active < GetAttributesNum(encounters); active++)
        {
            aref encounter = GetAttributeN(encounters, active);
            if (CheckAttribute(encounter, "quest.chrID") && encounter.quest.chrID == pchar.GreatRecipe.MapCaptain &&
                !CheckAttribute(encounter, "needDelete") && (phase == "search" || phase == "lost"))
            {
                if (!CheckAttribute(encounter, "liveTime")) { Trace("GreatRecipe map: retained marker without ttl"); return; }
                Trace("GreatRecipe map: existing x=" + GreatRecipe_MapTraceValue(encounter, "x") + " z=" + GreatRecipe_MapTraceValue(encounter, "z") +
                    " gotoX=" + GreatRecipe_MapTraceValue(encounter, "gotoX") + " gotoZ=" + GreatRecipe_MapTraceValue(encounter, "gotoZ") + " ttl=" + encounter.liveTime);
                if (stf(encounter.liveTime) < 3.0) return;
                float west = GreatRecipe_MapRouteX();
                float end = GreatRecipe_MapRouteEnd();
                bool safe = CheckAttribute(encounter, "x") && CheckAttribute(encounter, "z") && CheckAttribute(encounter, "gotoX") && CheckAttribute(encounter, "gotoZ");
                if (safe) safe = GreatRecipe_MapWestPoint(stf(encounter.x), stf(encounter.z)) &&
                    abs(stf(encounter.gotoX) - west) < 0.1 && abs(stf(encounter.gotoZ) - end) < 0.1;
                if (safe) return;
                pchar.GreatRecipe.MapReissue.ID = pchar.GreatRecipe.MapCaptain;
                pchar.GreatRecipe.MapReissue.Hours = stf(encounter.liveTime) * stf(worldMap.date.hourPerSec);
                pchar.GreatRecipe.MapReissue.Year = worldMap.date.year;
                pchar.GreatRecipe.MapReissue.Month = worldMap.date.month;
                pchar.GreatRecipe.MapReissue.Day = worldMap.date.day;
                pchar.GreatRecipe.MapReissue.Hour = worldMap.date.hour;
                // Release owns deletion/counter reconciliation. Suppress only this owned success event.
                DeleteAttribute(encounter, "quest.event");
                break;
            }
        }
        Trace("GreatRecipe map: releasing missing or unsafe marker captain=" + pchar.GreatRecipe.MapCaptain);
        Map_ReleaseQuestEncounter(pchar.GreatRecipe.MapCaptain);
        DeleteAttribute(pchar, "GreatRecipe.MapCaptain");
    }
    if (CheckAttribute(pchar, "GreatRecipe.MapReissue"))
    {
        string retryID = pchar.GreatRecipe.MapReissue.ID;
        string expected = "GreatRecipeDreamBeard";
        if (phase == "lost") expected = "GreatRecipeRealBeard";
        if (retryID != expected || (phase != "search" && phase != "lost")) { DeleteAttribute(pchar, "GreatRecipe.MapReissue"); return; }
        float left = GreatRecipe_MapReissueSeconds();
        if (left < 3.0 || GreatRecipe_MapCreateWest(retryID, left)) DeleteAttribute(pchar, "GreatRecipe.MapReissue");
        return;
    }
    bool window = phase == "lost" && GreatRecipe_Window() && Whr_IsStorm();
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
        string id = "GreatRecipeRealBeard";
        ref captain = GreatRecipe_Actor(id, "RedBeard");
        if (!CheckAttribute(captain, "Ship.Type") || sti(captain.Ship.Type) == SHIP_NOTUSED)
        {
            GreatRecipe_Ship(captain, SHIP_CORVETTE, "Громовая черепаха", 95); captain.Ship.HP = 900;
        }
        captain.MapEnc.Type = "war";
        captain.name = "Красная"; captain.lastname = "Борода";
        GreatRecipe_MapCreateWest(id, 24.0 / stf(worldMap.date.hourPerSec));
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
    GreatRecipe_Journal("Лаки потерял брата и слышал о корабле с красными парусами. Рыбаки видели что-то у Бермуд, далеко от берега, в ясную ночь при полной луне. Их рассказы расходятся даже в том, был ли это остров.", false);
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
    deck.reload.l1.disable = !GreatRecipe_AudienceAllowed();
    deck.reload.l2.go = "GreatRecipeMess";
    deck.reload.l3.go = "GreatRecipeHold";
    ref mess = &Locations[FindLocation("GreatRecipeMess")];
    mess.reload.l1.go = "GreatRecipeDeck";
    ref cabin = &Locations[FindLocation("GreatRecipeCabin")];
    cabin.reload.l1.go = "GreatRecipeDeck";
    cabin.reload.l1.emerge = "reload_cabin";
    // Supplemental GMs register real item-group labels through the stock model
    // loader. Refresh existing saved clones as well as newly created rooms.
    for (int supplies = 0; supplies < 3; supplies++)
    {
        string supplyRoom = "GreatRecipeHold";
        if (supplies == 1) supplyRoom = "GreatRecipeMess";
        if (supplies == 2) supplyRoom = "GreatRecipeDeck";
        ref supplyLocation = &Locations[FindLocation(supplyRoom)];
        supplyLocation.models.always.greatRecipeItems = "great_recipe_items";
        DeleteAttribute(supplyLocation, "models.always.greatRecipeItems.tech");
        supplyLocation.locators_radius.item = 0.8;
    }
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

string GreatRecipe_CrewModel(int role)
{
    if (role == 0) return "pirate_6";
    if (role == 1) return "barmen_1";
    if (role == 2) return "prison_2";
    if (role == 3) return "pirate_3";
    if (role == 4) return "pirate_4";
    return "trader_7";
}

bool GreatRecipe_AudienceAllowed()
{
    if (GreatRecipe_Phase() != "captive") return false;
    if (CheckAttribute(pchar, "GreatRecipe.AudienceGranted")) return sti(pchar.GreatRecipe.AudienceGranted);
    if (!CheckAttribute(pchar, "GreatRecipe.Task")) return false;
    // Existing advanced saves already reached the captain's part of the story.
    string task = pchar.GreatRecipe.Task;
    return task != "rations" && task != "rigging";
}

void GreatRecipe_UpdateAudience()
{
    int deck = FindLocation("GreatRecipeDeck");
    if (deck < 0) return;
    Locations[deck].reload.l1.disable = !GreatRecipe_AudienceAllowed();
    if (IsEntity(&Locations[deck])) LocatorReloadEnterDisable("GreatRecipeDeck", "reload_cabin", !GreatRecipe_AudienceAllowed());
}

bool GreatRecipe_EarnAudience()
{
    if (GreatRecipe_AudienceAllowed()) return true;
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Arrival.Complete")) return false;
    bool earned = CheckAttribute(pchar, "GreatRecipe.World.BlockRepaired");
    if (pchar.GreatRecipe.Task == "rigging" && (!CheckAttribute(pchar, "GreatRecipe.Betrayed") || CheckAttribute(pchar, "GreatRecipe.Reconciled"))) earned = true;
    if (!earned) return false;
    pchar.GreatRecipe.AudienceGranted = true;
    GreatRecipe_UpdateAudience();
    return true;
}

bool GreatRecipe_DrillAllowed()
{
    if (GreatRecipe_Phase() != "captive" || GreatRecipe_ShoreActive() || CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return false;
    string task = pchar.GreatRecipe.Task;
    return sti(pchar.GreatRecipe.TrainingWins) < 2 &&
        (CheckAttribute(pchar, "GreatRecipe.World.BlockRepaired") || (task != "rations" && task != "rigging"));
}

bool GreatRecipe_IntroPending()
{
    return GreatRecipe_Phase() == "captive" && CheckAttribute(pchar, "GreatRecipe.Task") && pchar.GreatRecipe.Task == "rations" &&
        !CheckAttribute(pchar, "GreatRecipe.Arrival.Complete") && !CheckAttribute(pchar, "GreatRecipe.Fight.Active") && !GreatRecipe_ShoreActive();
}

void GreatRecipe_Arrived(string unused)
{
    if (!GreatRecipe_IntroPending() || pchar.location != "GreatRecipeHold") return;
    if (!CheckAttribute(pchar, "GreatRecipe.Arrival.Stage")) pchar.GreatRecipe.Arrival.Stage = "captain";
    GreatRecipe_ArrivalClass();
    if (pchar.GreatRecipe.Arrival.Stage == "leaving") { GreatRecipe_CaptainLeaves(""); return; }
    if (pchar.GreatRecipe.Arrival.Stage == "captain")
    {
        ref captain = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
        ChangeCharacterAddressGroup(captain, "GreatRecipeHold", "goto", "goto2");
        captain.Dialog.CurrentNode = "ArrivalBeard";
        if (CheckAttribute(pchar, "GreatRecipe.Arrival.Resume")) captain.Dialog.CurrentNode = pchar.GreatRecipe.Arrival.Resume;
        LAi_SetActorType(captain);
        LAi_ActorDialogNow(captain, pchar, "", -1);
        return;
    }
    ref bosun = GreatRecipe_Actor("GreatRecipeBosun", GreatRecipe_CrewModel(0));
    ChangeCharacterAddressGroup(bosun, "GreatRecipeHold", "goto", "goto4");
    bosun.Dialog.CurrentNode = "Arrival";
    if (CheckAttribute(pchar, "GreatRecipe.Arrival.Resume")) bosun.Dialog.CurrentNode = pchar.GreatRecipe.Arrival.Resume;
    LAi_SetActorType(bosun);
    LAi_ActorDialogNow(bosun, pchar, "", -1);
}

void GreatRecipe_CaptainLeaves(string unused)
{
    if (!GreatRecipe_IntroPending() || pchar.location != "GreatRecipeHold" ||
        !CheckAttribute(pchar, "GreatRecipe.Arrival.Stage") || pchar.GreatRecipe.Arrival.Stage != "leaving") return;
    ref captain = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
    LAi_SetStayType(captain);
    ChangeCharacterAddressGroup(captain, "GreatRecipeCabin", "rld", "loc0");
    captain.Dialog.CurrentNode = "First time";
    pchar.GreatRecipe.Arrival.Stage = "bosun";
    DeleteAttribute(pchar, "GreatRecipe.Arrival.Resume");
    GreatRecipe_UpdateAudience();
    GreatRecipe_Arrived("");
}

int GreatRecipe_ArrivalClass()
{
    if (CheckAttribute(pchar, "GreatRecipe.Arrival.Class")) return sti(pchar.GreatRecipe.Arrival.Class);
    int shipClass = 0;
    if (CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet.s0.RealShip.Class")) shipClass = sti(pchar.GreatRecipe.Escrow.Fleet.s0.RealShip.Class);
    else if (CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet.s0.RealShip.BaseType"))
    {
        int base = sti(pchar.GreatRecipe.Escrow.Fleet.s0.RealShip.BaseType);
        if (base >= 0 && base < SHIP_TYPES_QUANTITY && CheckAttribute(&ShipsTypes[base], "Class")) shipClass = sti(ShipsTypes[base].Class);
    }
    if (shipClass < 1 || shipClass > 7) shipClass = 0;
    pchar.GreatRecipe.Arrival.Class = shipClass;
    return shipClass;
}

string GreatRecipe_ArrivalTaunt()
{
    int shipClass = GreatRecipe_ArrivalClass();
    if (shipClass == 1) return "Корабль первого ранга, целая эскадра — а капитан валяется у моих ног. Я бы плюнул тебе в лицо, да слюны жалко. Без крепостных стен на воде ты кто?";
    if (shipClass == 2) return "С таким кораблём ты, небось, привык, что люди уступают дорогу. Здесь дорогу выбираю я. Твои пушки остались там, а ты — здесь.";
    if (shipClass == 3) return "Привёл добрый корабль, спору нет. Только командовал им так, словно море обязано подвинуться. Подвинулось — под тебя.";
    if (shipClass == 4) return "Твой корабль мог драться. Значит, дело не в досках и не в пушках. Капитан подвёл — и теперь лежит в чужом трюме.";
    if (shipClass == 5) return "На таком корабле надо знать, где отступить. Ты решил показать зубы. Ну что, капитан, все ещё на месте?";
    if (shipClass == 6) return "Смелости у тебя больше, чем бортов и пушек. На море это быстро лечится. Сегодня тебе повезло: лекарство досталось кораблю.";
    if (shipClass == 7) return "На этой скорлупке у тебя и шанса не было. Не сердись: иной раз маленькому капитану полезно увидеть большой корабль изнутри.";
    return "Вот и встретились без пушек между нами. На моей палубе твой капитанский голос ничего не стоит. Посмотрим, чего стоишь ты сам.";
}

string GreatRecipe_NamedOpponent(string task)
{
    if (task == "captain") return "GreatRecipeDreamBeard";
    if (task == "rations" || task == "redeem") return "GreatRecipeMate";
    if (task == "rigging_lesson" || task == "powder_lesson") return "GreatRecipeGunner";
    return "";
}

string GreatRecipe_FightArm(ref actor, string slot, float floor, bool hero, bool recover)
{
    int sequence = sti(pchar.GreatRecipe.Fight.Sequence);
    string name = "GreatRecipeFightYield_" + sequence + "_" + slot;
    bool yielded = false;
    if (recover)
    {
        yielded = CheckAttribute(actor, "GreatRecipe.YieldSequence") && sti(actor.GreatRecipe.YieldSequence) == sequence;
        if (!CheckAttribute(actor, "chr_ai.hpchecker") && LAi_IsImmortal(actor) &&
            (hero || CheckAttribute(actor, "GreatRecipe.FightSequence") || CheckAttribute(actor, "GreatRecipe.Fighter") ||
             (actor.id == "GreatRecipeDreamBeard" && actor.location == "GreatRecipeDeck"))) yielded = true;
    }
    pchar.GreatRecipe.Fight.Callbacks.(name).Sequence = sequence;
    pchar.GreatRecipe.Fight.Callbacks.(name).Index = actor.index;
    pchar.GreatRecipe.Fight.Callbacks.(name).Hero = hero;
    pchar.GreatRecipe.Fight.Callbacks.(name).Floor = floor;
    pchar.GreatRecipe.Fight.Callbacks.(name).Yielded = yielded;
    actor.GreatRecipe.FightSequence = sequence;
    if (yielded) { actor.GreatRecipe.YieldSequence = sequence; LAi_QuestDelay(name, 0.0); }
    else LAi_SetCheckMinHP(actor, floor, true, name);
    return name;
}

void GreatRecipe_FightTerminal(string name)
{
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Fight.Active") ||
        !CheckAttribute(pchar, "GreatRecipe.Fight.Callbacks." + name)) return;
    aref callback;
    makearef(callback, pchar.GreatRecipe.Fight.Callbacks.(name));
    int sequence = sti(pchar.GreatRecipe.Fight.Sequence);
    if (sti(callback.Sequence) != sequence) return;
    int index = sti(callback.Index);
    if (index < 0 || index >= MAX_CHARACTERS) return;
    ref actor = GetCharacter(index);
    if (!CheckAttribute(actor, "GreatRecipe.FightSequence") || sti(actor.GreatRecipe.FightSequence) != sequence) return;
    // Stock damage has already clamped HP and removed the checker. The marker
    // survives subsequent regeneration, SAVE and either handler ordering.
    if (sti(callback.Hero) || (!CheckAttribute(pchar, "chr_ai.hpchecker") && LAi_IsImmortal(pchar)))
    { GreatRecipe_FightFinish(false); return; }
    if (sti(callback.Yielded))
    {
        // A recovered yield still needs the aggregate terminal decision.
        actor.GreatRecipe.YieldSequence = sequence;
    }
    else
    {
        callback.Yielded = true;
        actor.GreatRecipe.YieldSequence = sequence;
        LAi_SetImmortal(actor, true); LAi_SetStayType(actor);
        LAi_group_MoveCharacter(actor, "GreatRecipeHoldGroup");
    }
    aref callbacks;
    makearef(callbacks, pchar.GreatRecipe.Fight.Callbacks);
    for (int entry = 0; entry < GetAttributesNum(callbacks); entry++)
    {
        aref pending = GetAttributeN(callbacks, entry);
        if (!sti(pending.Hero) && !sti(pending.Yielded)) return;
    }
    GreatRecipe_FightFinish(true);
}

void GreatRecipe_RearmFight()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Sequence")) pchar.GreatRecipe.Fight.Sequence = 1;
    GreatRecipe_FightArm(pchar, "hero", LAi_GetCharacterMaxHP(pchar) * 0.2, true, true);
    string task = pchar.GreatRecipe.Fight.Active;
    string named = GreatRecipe_NamedOpponent(task);
    if (named != "")
    {
        int index = GetCharacterIndex(named);
        if (index < 0) { GreatRecipe_FightFinish(false); return; }
        ref actor = GetCharacter(index);
        // Earlier builds fought a generic stand-in. Recover its real surrender
        // state before replacing it with the named character.
        if (task != "captain" && !CheckAttribute(actor, "GreatRecipe.FightSequence"))
        {
            int old = GetCharacterIndex("GreatRecipeFighter0");
            if (old >= 0)
            {
                ref prior = GetCharacter(old);
                if (prior.location == "GreatRecipeDeck" && !CheckAttribute(prior, "chr_ai.hpchecker") && LAi_IsImmortal(prior))
                    actor.GreatRecipe.YieldSequence = pchar.GreatRecipe.Fight.Sequence;
            }
        }
        float floor = 10.0;
        if (task == "captain") floor = 40.0;
        GreatRecipe_FightArm(actor, "named", floor, false, true);
        return;
    }
    for (int fighter = 0; fighter < 3; fighter++)
    {
        int enemyIndex = GetCharacterIndex("GreatRecipeFighter" + fighter);
        if (enemyIndex < 0) continue;
        ref enemy = GetCharacter(enemyIndex);
        if (enemy.location == "GreatRecipeDeck") GreatRecipe_FightArm(enemy, "f" + fighter, 10.0, false, true);
    }
}

void GreatRecipe_Locks()
{
    bDisableFastReload = true;
    bQuestDisableMapEnter = true;
    // Character/ship panels stay usable for a fair duel's charge/control setup.
    chrDisableReloadToLocation = CheckAttribute(pchar, "GreatRecipe.Fight.Active");
}

void GreatRecipe_MateFighter(ref mate)
{
    if (!CheckAttribute(mate, "GreatRecipe.MateDuel"))
    {
        FantomMakeCoolFighter(mate, sti(pchar.rank), 65, 55, "blade25", "", 30);
        LAi_SetCurHPMax(mate);
        mate.GreatRecipe.MateDuel = true;
    }
    LAi_SetImmortal(mate, false);
    LAi_SetWarriorTypeNoGroup(mate);
    LAi_group_MoveCharacter(mate, "GreatRecipeFightGroup");
}

void GreatRecipe_CaptiveActors()
{
    GreatRecipe_RearmFight();
    GreatRecipe_Locks();
    if (CheckAttribute(pchar, "GreatRecipe.Fight.Active"))
    {
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
        ref actor = GreatRecipe_Actor(id, GreatRecipe_CrewModel(role));
        if (!IsEntity(actor)) { actor.model = GreatRecipe_CrewModel(role); FaceMaker(actor); }
        if (role == 2 && !CheckAttribute(actor, "GreatRecipe.Wounded") && !CheckAttribute(pchar, "GreatRecipe.Helped") &&
            (pchar.GreatRecipe.Task == "rations" || pchar.GreatRecipe.Task == "rigging" || pchar.GreatRecipe.Task == "training" || pchar.GreatRecipe.Task == "medicine"))
        {
            actor.GreatRecipe.Wounded = true;
            LAi_SetCurHP(actor, LAi_GetCharacterMaxHP(actor) * 0.4);
        }
        actor.name = name; actor.lastname = "";
        actor.Dialog.Filename = "GreatRecipe.c";
        actor.Dialog.CurrentNode = "First time";
        actor.Dialog.TempNode = "First time";
        if (CheckAttribute(pchar, "GreatRecipe.Fight.Active") && id == GreatRecipe_NamedOpponent(pchar.GreatRecipe.Fight.Active))
        {
            // A saved duel from the former generic-opponent path keeps the task,
            // but only its named opponent remains on deck.
            for (int fighter = 0; fighter < 3; fighter++)
            {
                int old = GetCharacterIndex("GreatRecipeFighter" + fighter);
                if (old < 0) continue;
                ref extra = GetCharacter(old);
                LAi_RemoveCheckMinHP(extra); LAi_SetImmortal(extra, true); LAi_SetStayType(extra);
                LAi_group_MoveCharacter(extra, "GreatRecipeHoldGroup");
                ChangeCharacterAddress(extra, "none", "");
            }
            if (!CheckAttribute(actor, "GreatRecipe.MateDuel")) GreatRecipe_MateFighter(actor);
            if (!CheckAttribute(actor, "GreatRecipe.YieldSequence") || sti(actor.GreatRecipe.YieldSequence) != sti(pchar.GreatRecipe.Fight.Sequence))
            {
                LAi_SetImmortal(actor, false); LAi_SetWarriorTypeNoGroup(actor);
                LAi_group_MoveCharacter(actor, "GreatRecipeFightGroup");
            }
            if (actor.location != "GreatRecipeDeck") ChangeCharacterAddressGroup(actor, "GreatRecipeDeck", "rld", locator);
            continue;
        }
        LAi_SetImmortal(actor, true);
        LAi_SetStayType(actor);
        LAi_group_MoveCharacter(actor, "GreatRecipeHoldGroup");
        if (role == 0 && GreatRecipe_IntroPending())
        {
            room = "GreatRecipeHold"; group = "goto"; locator = "goto4";
            actor.Dialog.CurrentNode = "Arrival";
            if (CheckAttribute(pchar, "GreatRecipe.Arrival.Resume")) actor.Dialog.CurrentNode = pchar.GreatRecipe.Arrival.Resume;
        }
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
        if (!CheckAttribute(beard, "GreatRecipe.YieldSequence") || sti(beard.GreatRecipe.YieldSequence) != sti(pchar.GreatRecipe.Fight.Sequence)) LAi_SetWarriorTypeNoGroup(beard);
        LAi_group_MoveCharacter(beard, "GreatRecipeFightGroup");
        LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
        LAi_group_FightGroups("GreatRecipeFightGroup", LAI_GROUP_PLAYER, true);
        return;
    }
    LAi_SetImmortal(beard, true);
    LAi_SetStayType(beard);
    LAi_group_MoveCharacter(beard, "GreatRecipeHoldGroup");
    DeleteAttribute(beard, "AlwaysEnemy");
    if (GreatRecipe_IntroPending() &&
        (!CheckAttribute(pchar, "GreatRecipe.Arrival.Stage") || pchar.GreatRecipe.Arrival.Stage == "captain"))
        ChangeCharacterAddressGroup(beard, "GreatRecipeHold", "goto", "goto2");
    else ChangeCharacterAddressGroup(beard, "GreatRecipeCabin", "rld", "loc0");
    GreatRecipe_UpdateAudience();
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
    if (GreatRecipe_IntroPending()) return "Сначала выслушай капитана и Моргана.";
    string task = pchar.GreatRecipe.Task;
    if (action == "cook" && pchar.location == "GreatRecipeMess" && task == "rations")
    { pchar.GreatRecipe.Clue.Cook = true; return "Из кладовой ушла бочка сухарей. Я запер её после ужина; ключ был у помощника капитана. А на крышке осталась синяя краска."; }
    if (action == "prisoner" && pchar.location == "GreatRecipeHold" && task == "rations")
    { pchar.GreatRecipe.Clue.Prisoner = true; return "До ужина Ратли велел мне перекатить бочку в кладовую. Потом меня приковали в трюме; после ужина я никуда не выходил. Ночью слышал, как он спускал сухари через люк: прячет их, чтоб купить голоса команды."; }
    if (action == "gunner" && pchar.location == "GreatRecipeDeck" && task == "rations")
    { pchar.GreatRecipe.Clue.Gunner = true; return "Синяя краска на сапогах? У Ратли: он красил ими люк. После ужина я видел его с бочкой у люка. Диего тогда был прикован внизу; доктор спускался к нему."; }
    if (action == "surgeon" && task == "rations" && pchar.location == "GreatRecipeMess")
    { pchar.GreatRecipe.Clue.Alibi = true; return GreatRecipe_Story("GreatRecipeSurgeon", "history"); }
    if (action == "accuse" && task == "rations" && pchar.location == "GreatRecipeDeck")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Clue.Cook") || !CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") || !CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") || !CheckAttribute(pchar, "GreatRecipe.Clue.Alibi")) return "Боцман: слова пленника против слов помощника. Кто подтвердит, где был Диего? Лекарь мог его видеть. Расспроси всех, прежде чем обвинять.";
        pchar.GreatRecipe.Fight.Pending = "rations";
        return "Ратли: врёшь, крыса! Выходи на палубу, посмотрим, кто кого заставит замолчать.";
    }
    if (action == "betray" && task == "rations" && pchar.location == "GreatRecipeDeck" && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner"))
    { pchar.GreatRecipe.Betrayed = true; GreatRecipe_TaskAdvance("rigging", "Я выдал Диего боцману. Ратли остался в стороне. Теперь на корабле заклинило грота-шкот; Джек недоволен тем, как с ним обращались."); return "Боцман: пленника запру. А ты к Джеку — грот опять не слушается руля."; }
    if (action == "gunner" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    { pchar.GreatRecipe.Clue.Block = true; return "Шкот цел, но блок заклинило железным штифтом. Нужен новый деревянный нагель, пропитанный маслом. У Тома масло, у Диего обрезки дуба."; }
    if (action == "cook" && task == "rigging" && pchar.location == "GreatRecipeMess" && CheckAttribute(pchar, "GreatRecipe.Clue.Block"))
    { return "Бутылка масла стоит возле переборки у камбуза. Забери её сам. Не лей на канат — смажь ось блока. Вчера Ратли расспрашивал меня, как остановить грот незаметно."; }
    if (action == "prisoner" && task == "rigging" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Block"))
    {
        if (CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")) return "Ты выдал меня вместо Ратли. Хочешь моей помощи — заставь его признаться перед командой. Тогда поговорим.";
        return "Дубовый нагель в свёртке у бочек в трюме; забери его. Не бей молотком: сначала ослабь шкот, потом вынь железо. Ратли хочет, чтоб нас настигли его дружки.";
    }
    if (action == "accuse" && task == "rigging" && pchar.location == "GreatRecipeDeck" && CheckAttribute(pchar, "GreatRecipe.Betrayed"))
    {
        if (CheckAttribute(pchar, "GreatRecipe.Reconciled")) return "Ратли уже признался перед командой. Диего оправдан; повторной дракой здесь ничего не изменишь.";
        pchar.GreatRecipe.Fight.Pending = "redeem"; return "Ратли: назад берёшь свои слова? Сначала попробуй отнять у меня саблю — тогда получишь признание.";
    }
    if (action == "repair" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    {
        if (GetCharacterItem(pchar, "GreatRecipeOil") == 0 || GetCharacterItem(pchar, "GreatRecipePin") == 0) return "Без настоящих нагеля и масла блок не оживёт. Забери бутылку в кубрике и свёрток в трюме.";
        return "Джек: верно. Блок справа от грот-мачты. Подойди к нему и поставь нагель; масло тоже уйдёт в работу. Разговором снасть не починишь.";
    }
    if (action == "rigging_wrong" && task == "rigging" && pchar.location == "GreatRecipeDeck")
    {
        if (CheckAttribute(pchar, "GreatRecipe.Lesson.RiggingDone")) return "Джек: мы это уже выяснили. Снасть всё ещё заклинена; нагель и масло пойдут в блок, не в наши головы.";
        pchar.GreatRecipe.Fight.Pending = "rigging_lesson";
        return "Джек перехватывает мою руку: под этим шкотом работают люди. Хочешь рубить, не слушая их? Тогда сначала пройди через меня. До сдачи — но клинок держи крепче.";
    }
    if (action == "drill" && GreatRecipe_DrillAllowed() && pchar.location == "GreatRecipeDeck")
    {
        if (sti(pchar.GreatRecipe.TrainingWins) >= 2) return "Оба матроса уже уступили. Джек не станет выпускать их снова ради той же проверки.";
        pchar.GreatRecipe.Fight.Pending = "training"; return "Джек: без стрельбы, до сдачи. Первый любит быстрые выпады, второй давит силой. Одинаково стоять против них — верный способ лечь.";
    }
    if (action == "captain_practice" && pchar.location == "GreatRecipeCabin")
    {
        if (!GreatRecipe_AudienceAllowed()) return "Сначала получи слово боцмана. Капитан ещё не принял твой вызов.";
        pchar.GreatRecipe.Fight.Pending = "captain"; return "Борода: на палубу. До сдачи, не до смерти — за твой труп я уже заплатил бы слишком много хлопот. Посмотрим, держишь ли ты слово так же крепко, как клинок.";
    }
    if (action == "surgeon" && task == "medicine" && pchar.location == "GreatRecipeMess")
    { pchar.GreatRecipe.Clue.Medicine = true; return "Принеси чистое полотно из трюма и кипячёную воду от кока. В рану порох не сыпать, ром не лить. Диего нужен живой — он видел, кто вскрыл крюйт-камеру."; }
    if (action == "prisoner" && task == "medicine" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Medicine"))
    { return "Чистое полотно в свёртке возле моей койки. Забери... Я видел Ратли у пороха. Но сперва помоги — лихорадка меня добьёт."; }
    if (action == "cook" && task == "medicine" && pchar.location == "GreatRecipeMess" && CheckAttribute(pchar, "GreatRecipe.Clue.Medicine"))
    { return "Бутылка кипячёной воды стоит у прохода в кубрике. Уже остыла. Забери и промой рану: чистое полотно лучше рома."; }
    if (action == "heal" && task == "medicine" && pchar.location == "GreatRecipeHold")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Clue.Medicine") || GetCharacterItem(pchar, "GreatRecipeLinen") == 0 || GetCharacterItem(pchar, "GreatRecipeWater") == 0) return "Нужны чистая вода и полотно в твоём инвентаре. Лекарь в кубрике объяснит, где их взять.";
        TakeItemFromCharacter(pchar, "GreatRecipeLinen");
        TakeItemFromCharacter(pchar, "GreatRecipeWater");
        ref prisoner = characterFromId("GreatRecipePrisoner");
        prisoner.GreatRecipe.Wounded = false;
        LAi_SetCurHPMax(prisoner);
        pchar.GreatRecipe.Helped = true;
        GreatRecipe_TaskAdvance("powder", "Диего перевязан. Он утверждает, что Ратли возился в крюйт-камере и подменил фитили. Если это правда, следующий залп может погубить своих.");
        return "Диего: рана больше не горит... Теперь верю тебе. Но послушай: в журнале капитана три меры пороха и сухой фитиль. Ратли подменил его промасленной паклей. Следующий залп может убить своих.";
    }
    if (action == "captain" && task == "powder" && pchar.location == "GreatRecipeCabin")
    { pchar.GreatRecipe.Clue.Manifest = true; return "Борода: три меры, сухая оболочка, фитиль с красной нитью. Не путать с белым: тот горит быстрее. Проверить с Джеком; свои пальцы мне ни к чему."; }
    if (action == "prisoner" && task == "powder" && pchar.location == "GreatRecipeHold" && CheckAttribute(pchar, "GreatRecipe.Clue.Manifest"))
    { return "Я спрятал сухие фитили в свёртке у стола в кубрике. Там красная нить. Белые Ратли подкинул нарочно — снаряд рванёт ещё у борта. Скажи Джеку, что я не с ним."; }
    if (action == "powder" && task == "powder" && pchar.location == "GreatRecipeDeck")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Clue.Manifest") || GetCharacterItem(pchar, "GreatRecipeFuse") == 0) return "Джек: сперва накладная у капитана и сухие фитили из кубрика. На глаз заряжать не стану.";
        return "Джек: пост у орудия по левому борту. Подойди и поставь сухой красный фитиль по накладной. Затем поговорим о людях Ратли.";
    }
    if (action == "captain" && task == "mutiny_final" && pchar.location == "GreatRecipeCabin")
    {
        pchar.GreatRecipe.Clue.CaptainOrder = true;
        if (sti(pchar.GreatRecipe.Rebel)) return "Борода: мою первую вахту ты разоружил, но нос ещё держат трое верных мне матросов. Свободу обещал Ратли — а кровь за неё проливаешь ты. Решил идти с ним? Тогда отвечай за своих до конца.";
        return "Борода: Ратли ещё держит носовую палубу. Своих ты уже выбрал; теперь ответь за них. Отбери у его последней тройки оружие.";
    }
    if (action == "wrong_fuse" && task == "powder" && pchar.location == "GreatRecipeDeck")
    {
        if (CheckAttribute(pchar, "GreatRecipe.Lesson.PowderDone")) return "Джек: хватит испытывать моё терпение. Сухой красный фитиль, как в накладной. Пост у орудия ждёт.";
        pchar.GreatRecipe.Fight.Pending = "powder_lesson"; return "Джек вырывает белый фитиль: этот рванёт у нашего борта. Моих людей ты в расход не пустишь. Всё ещё считаешь, что знаешь лучше? На палубу, до сдачи.";
    }
    if ((action == "loyal" || action == "rebel") && (task == "mutiny" || task == "mutiny_final") && pchar.location == "GreatRecipeDeck")
    {
        if (task == "mutiny_final" && !CheckAttribute(pchar, "GreatRecipe.Clue.CaptainOrder")) return "Боцман: капитан вызывает в каюту. Не лезь на нос, пока не поговорил с ним.";
        if (CheckAttribute(pchar, "GreatRecipe.MutinySide") || task == "mutiny_final")
        {
            if (sti(pchar.GreatRecipe.Rebel) != (action == "rebel")) return "Свою сторону ты уже выбрал. Твои люди ждут на палубе.";
        }
        else pchar.GreatRecipe.Rebel = action == "rebel";
        pchar.GreatRecipe.MutinySide = true;
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
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Sequence")) pchar.GreatRecipe.Fight.Sequence = 0;
    pchar.GreatRecipe.Fight.Sequence = sti(pchar.GreatRecipe.Fight.Sequence) + 1;
    DeleteAttribute(pchar, "GreatRecipe.Fight.Callbacks");
    pchar.GreatRecipe.Fight.Active = task;
    if (task == "captain")
    {
        ref beard = GreatRecipe_Actor("GreatRecipeDreamBeard", "RedBeard");
        GreatRecipe_BeardFighter(beard);
        LAi_SetImmortal(beard, false);
        LAi_SetWarriorTypeNoGroup(beard);
        LAi_group_MoveCharacter(beard, "GreatRecipeFightGroup");
        ChangeCharacterAddressGroup(beard, "GreatRecipeDeck", "rld", "loc9");
        DeleteAttribute(beard, "GreatRecipe.YieldSequence");
        GreatRecipe_FightArm(beard, "named", 40.0, false, false);
    }
    if (task == "rations" || task == "redeem")
    {
        ref mate = GreatRecipe_Actor("GreatRecipeMate", GreatRecipe_CrewModel(4));
        GreatRecipe_MateFighter(mate);
        LAi_SetCurHPMax(mate);
        ChangeCharacterAddressGroup(mate, "GreatRecipeDeck", "rld", "loc10");
        DeleteAttribute(mate, "GreatRecipe.YieldSequence");
        GreatRecipe_FightArm(mate, "named", 10.0, false, false);
    }
    if (task == "rigging_lesson" || task == "powder_lesson")
    {
        ref jack = GreatRecipe_Actor("GreatRecipeGunner", GreatRecipe_CrewModel(3));
        GreatRecipe_MateFighter(jack);
        LAi_SetCurHPMax(jack);
        ChangeCharacterAddressGroup(jack, "GreatRecipeDeck", "rld", "loc6");
        DeleteAttribute(jack, "GreatRecipe.YieldSequence");
        GreatRecipe_FightArm(jack, "named", 10.0, false, false);
    }
    int opponents = 1;
    if (task == "mutiny" || task == "mutiny_final") opponents = 3;
    if (task == "mutiny" && CheckAttribute(pchar, "GreatRecipe.Helped") && (!CheckAttribute(pchar, "GreatRecipe.Betrayed") || CheckAttribute(pchar, "GreatRecipe.Reconciled"))) opponents = 2;
    pchar.GreatRecipe.Fight.Remaining = opponents;
    for (int fighter = 0; GreatRecipe_NamedOpponent(task) == "" && fighter < opponents; fighter++)
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
        DeleteAttribute(enemy, "GreatRecipe.YieldSequence");
        GreatRecipe_FightArm(enemy, "f" + fighter, 10.0, false, false);
    }
    LAi_SetImmortal(pchar, false);
    DeleteAttribute(pchar, "GreatRecipe.YieldSequence");
    GreatRecipe_FightArm(pchar, "hero", LAi_GetCharacterMaxHP(pchar) * 0.2, true, false);
    LAi_group_SetRelation("GreatRecipeFightGroup", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
    LAi_group_FightGroups("GreatRecipeFightGroup", LAI_GROUP_PLAYER, true);
    GreatRecipe_Locks();
    LAi_SetFightMode(pchar, true);
}

void GreatRecipe_CombatCheck()
{
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    GreatRecipe_FighterDown("");
}

void GreatRecipe_FighterDown(string unused)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Callbacks")) return;
    aref callbacks;
    makearef(callbacks, pchar.GreatRecipe.Fight.Callbacks);
    for (int entry = 0; entry < GetAttributesNum(callbacks); entry++)
    {
        aref callback = GetAttributeN(callbacks, entry);
        int index = sti(callback.Index);
        if (index < 0 || index >= MAX_CHARACTERS) continue;
        ref actor = GetCharacter(index);
        if (!CheckAttribute(actor, "chr_ai.hpchecker") && LAi_IsImmortal(actor))
        {
            string name = GetAttributeName(callback);
            GreatRecipe_FightTerminal(name);
            return;
        }
    }
}

void GreatRecipe_FightLost(string unused) { GreatRecipe_FightFinish(false); }

void GreatRecipe_FightFinish(bool won)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    if (LAi_GetCharacterHP(pchar) <= LAi_GetCharacterMaxHP(pchar) * 0.2) won = false;
    string task = pchar.GreatRecipe.Fight.Active;
    GreatRecipe_AllianceRecordFight(task, won);
    DeleteAttribute(pchar, "GreatRecipe.Fight.Active");
    DeleteAttribute(pchar, "GreatRecipe.Fight.Pending");
    DeleteAttribute(pchar, "GreatRecipe.Fight.Callbacks");
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
        if (won) GreatRecipe_Journal("Борода уступил в поединке. После боя он смотрел на меня иначе. Посмотрим, чего стоит это уважение.", false);
        else GreatRecipe_Journal("Борода разоружил меня и оставил в живых. По его словам, следующий вызов он примет.", false);
    }
    if (task == "rations" || task == "redeem")
    {
        ref mate = characterFromId("GreatRecipeMate");
        LAi_RemoveCheckMinHP(mate); LAi_SetImmortal(mate, true); LAi_SetStayType(mate);
        LAi_SetCurHPMax(mate);
        DeleteAttribute(mate, "AlwaysEnemy");
        LAi_group_MoveCharacter(mate, "GreatRecipeHoldGroup");
        ChangeCharacterAddressGroup(mate, "GreatRecipeDeck", "rld", "loc10");
    }
    if (task == "rigging_lesson" || task == "powder_lesson")
    {
        ref jack = characterFromId("GreatRecipeGunner");
        LAi_RemoveCheckMinHP(jack); LAi_SetImmortal(jack, true); LAi_SetStayType(jack);
        LAi_SetCurHPMax(jack); DeleteAttribute(jack, "AlwaysEnemy");
        LAi_group_MoveCharacter(jack, "GreatRecipeHoldGroup");
        ChangeCharacterAddressGroup(jack, "GreatRecipeDeck", "rld", "loc6");
        string lesson = "Rigging";
        if (task == "powder_lesson") lesson = "Powder";
        string lessonDone = lesson + "Done";
        string lessonWon = lesson + "Won";
        pchar.GreatRecipe.Lesson.(lessonDone) = true;
        pchar.GreatRecipe.Lesson.(lessonWon) = won;
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
        GreatRecipe_TaskAdvance("rigging", "Ратли признался в краже. Диего оправдан, но на палубе неспокойно: грота-шкот заклинило, и Джек подозревает новую пакость.");
    else if (task == "redeem")
    {
        pchar.GreatRecipe.Reconciled = true;
        GreatRecipe_Journal("Ратли признался перед командой. Диего больше не считают вором; теперь с ним можно говорить без этой лжи между нами.", false);
    }
    else if (task == "training")
    {
        pchar.GreatRecipe.TrainingWins = sti(pchar.GreatRecipe.TrainingWins) + 1;
        if (sti(pchar.GreatRecipe.TrainingWins) >= 2) GreatRecipe_Journal("Оба поединка позади. Джек признал, что я могу прикрыть его людей у орудий.", false);
        else GreatRecipe_Journal("Первый матрос уступил. Джек обещал ещё одного противника, с другой манерой боя.", false);
    }
    else if (task == "powder_lesson") GreatRecipe_Journal("Джек остановил меня у заряда. Я не разобрался в его устройстве и едва не погубил орудийную прислугу.", false);
    else if (task == "rigging_lesson") GreatRecipe_Journal("Джек показал, чем кончается спешка у натянутой снасти. Блок всё ещё неисправен.", false);
    else if (task == "mutiny")
    {
        string side = "Ратли";
        if (sti(pchar.GreatRecipe.Rebel)) side = "Бороды";
        GreatRecipe_TaskAdvance("mutiny_final", "Первый отряд сложил оружие, но носовую палубу удерживают трое опытных матросов " + side + ". Борода требует объяснений.");
    }
    else
        GreatRecipe_TaskAdvance("ready", "Мятеж на палубе закончился. Борода зовёт в каюту: теперь он согласен поставить свой рецепт на кон в честной морской дуэли.");
    LAi_SetCurHPMax(pchar);
    LAi_SetImmortal(pchar, false);
    GreatRecipe_Locks();
    GreatRecipe_UpdateAudience();
}

void GreatRecipe_BeardPerks(ref captain)
{
    if (!GreatRecipe_IsBeard(captain)) return;
    DeleteAttribute(captain, "perks.list.Brander");
    DeleteAttribute(captain, "TmpPerks.Brander");
    if (captain.id != "GreatRecipeDreamBeard" && captain.id != "GreatRecipeRealBeard") return;
    float oldMax = LAi_GetCharacterMaxHP(captain);
    float fraction = 0.0;
    if (oldMax > 0.0) fraction = LAi_GetCharacterHP(captain) / oldMax;
    aref catalogue;
    makearef(catalogue, ChrPerksList.list);
    for (int entry = 0; entry < GetAttributesNum(catalogue); entry++)
    {
        string perk = GetAttributeName(GetAttributeN(catalogue, entry));
        if (perk != "Brander") captain.perks.list.(perk) = true;
    }
    // HPPlus is derived by the stock getter; preserve damage when granting it.
    LAi_SetCurHP(captain, LAi_GetCharacterMaxHP(captain) * fraction);
    SetEnergyToCharacter(captain);
    SetShipSkill(captain, 100, 100, 100, 100, 100, 100, 100, 100, 100);
}

void GreatRecipe_BeardSeaTasks()
{
    if (!bSeaActive || !bSeaLoaded || bAbordageStarted) return;
    for (int ship = 0; ship < iNumShips; ship++)
    {
        ref captain = GetCharacter(Ships[ship]);
        if (!GreatRecipe_IsBeard(captain) || LAi_IsDead(captain) ||
            !CheckAttribute(captain, "SeaAI.Task") || sti(captain.SeaAI.Task) != AITASK_BRANDER) continue;
        int target = -1;
        if (CheckAttribute(captain, "SeaAI.Task.Target")) target = sti(captain.SeaAI.Task.Target);
        bool available = false;
        for (int other = 0; other < iNumShips; other++)
        {
            if (Ships[other] == target && target != sti(captain.index) && target >= 0 && target < MAX_CHARACTERS)
            {
                ref victim = GetCharacter(target);
                available = !LAi_IsDead(victim) && CheckAttribute(victim, "Ship.HP") && stf(victim.Ship.HP) > 0.0;
            }
        }
        if (available) Ship_SetTaskAttack(SECONDARY_TASK, sti(captain.index), target);
        else Ship_SetTaskDrift(SECONDARY_TASK, sti(captain.index));
    }
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
    GreatRecipe_BeardPerks(captain);
    LAi_SetImmortal(captain, false);
    LAi_SetCurHPMax(captain);
    captain.DontRansackCaptain = true;
    captain.AlwaysEnemy = true;
}

void GreatRecipe_Boss(ref dream)
{
    GreatRecipe_Ship(dream, SHIP_FRIGATEQUEEN, "Красный прилив", 100);
    ref flagship = GetRealShip(sti(dream.Ship.Type));
    flagship.HP = stf(flagship.HP) * 4.0;
    dream.Ship.GreatRecipeHullScale = 4;
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
    if (CheckAttribute(login, "residentResume") && IsEntity(captain) && Group_FindGroup("GreatRecipeSea") >= 0)
    {
        ref group = Group_GetGroupByID("GreatRecipeSea");
        if (Group_GetGroupCommanderIndexR(group) == sti(captain.index))
        {
            login.QuestGroups.GreatRecipe = "GreatRecipeSea";
            return;
        }
    }
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
    bool firstBattle = phase == "whirlpool" || phase == "assault" || phase == "boarding";
    bool admitted = CheckAttribute(login, "GreatRecipeMapAttack") && login.GreatRecipeMapAttack == "GreatRecipeDreamBeard" &&
        CheckAttribute(login, "GreatRecipeOffshore") && sti(login.GreatRecipeOffshore);
    Trace("GreatRecipe sea: prepare phase=" + phase + " time=" + GetTime() + " admitted=" + admitted + " island=" + login.Island);
    if (login.Island != "Bermudes" && !admitted && !GreatRecipe_OwnMapLogin(login) && !firstBattle) return;
    if (phase == "search" && admitted)
    {
        DeleteAttribute(login, "GreatRecipeMapAttack");
        DeleteAttribute(login, "GreatRecipeOffshore");
        string blocked = GreatRecipe_FirstBattleBlock();
        if (!GreatRecipe_LoginFree(login)) blocked = "foreign quest group";
        if (blocked != "") { Trace("GreatRecipe sea: first battle blocked " + blocked); return; }
        if (!GreatRecipe_ShipRooms()) { Trace("GreatRecipe sea: first battle blocked room templates/capacity"); return; }
        GreatRecipe_Snapshot();
        // This offshore encounter needs room below the full funnel. Use the
        // stock island-free sea mode; coordinates, geography and weather stay.
        login.Island = "";
        login.GreatRecipeOpenSea = true;
        pchar.GreatRecipe.Phase = "whirlpool";
        Trace("GreatRecipe sea: first battle started time=" + GetTime());
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
        GreatRecipe_Journal("Тёмный силуэт распался на кольца пены. Воронка захватила эскадру; среди воды поднимаются чужие мачты.", false);
        return;
    }
    if (phase == "whirlpool" || phase == "assault" || phase == "boarding")
    {
        login.Island = "";
        login.GreatRecipeOpenSea = true;
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
            if (phase == "boarding") Ship_SetTaskAbordage(SECONDARY_TASK, dreamIndex, nMainCharacterIndex);
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
    int boss = GetCharacterIndex("GreatRecipeDreamBeard");
    if (boss >= 0)
    {
        ref captain = GetCharacter(boss);
        if (CheckAttribute(captain, "Ship.SeaEmergence")) captain.Ship.SeaEmergence.Clear = 1;
        DeleteAttribute(captain, "SeaAI.PreferTaskTarget");
        DeleteAttribute(captain, "GreatRecipeBoardingApproach");
        DeleteAttribute(captain, "Ship_SetTaskAbordage");
    }
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

void GreatRecipe_PlaceVortex()
{
    if (!GreatRecipe_Locked()) return;
    GreatRecipe_Locks();
    if (GreatRecipe_Phase() == "whirlpool" || GreatRecipe_Phase() == "assault")
    {
        // Fit the fixed field around the live stock column; ships stay where
        // sea login placed them, with the current running along their heading.
        if (!CheckAttribute(pchar, "GreatRecipe.Vortex.Placed"))
        {
            bool known = CheckAttribute(pchar, "Ship.Pos.x") && CheckAttribute(pchar, "Ship.Pos.z") && CheckAttribute(pchar, "Ship.Ang.y");
            float heading = 0.0;
            float heroX = 0.0;
            float heroZ = 0.0;
            if (known)
            {
                heading = stf(pchar.Ship.Ang.y);
                heroX = stf(pchar.Ship.Pos.x);
                heroZ = stf(pchar.Ship.Pos.z);
            }
            float forwardX = sin(heading);
            float forwardZ = cos(heading);
            float minAlong = 0.0;
            float maxAlong = 0.0;
            float minAcross = 0.0;
            float maxAcross = 0.0;
            for (int slot = 0; slot < COMPANION_MAX; slot++)
            {
                int index = GetCompanionIndex(pchar, slot);
                if (index < 0) continue;
                ref captain = GetCharacter(index);
                if (!CheckAttribute(captain, "Ship.Pos.x") || !CheckAttribute(captain, "Ship.Pos.z")) { known = false; continue; }
                float x = stf(captain.Ship.Pos.x) - heroX;
                float z = stf(captain.Ship.Pos.z) - heroZ;
                float along = x * forwardX + z * forwardZ;
                float across = x * forwardZ - z * forwardX;
                if (along < minAlong) minAlong = along;
                if (along > maxAlong) maxAlong = along;
                if (across < minAcross) minAcross = across;
                if (across > maxAcross) maxAcross = across;
            }
            float span = maxAlong - minAlong;
            float halfWidth = (maxAcross - minAcross) * 0.5;
            float radius = 1300.0;
            if (span * 1.5 > radius) radius = span * 1.5;
            if ((240.0 + halfWidth) / 0.185 > radius) radius = (240.0 + halfWidth) / 0.185;
            float offset = sqrt(0.815 * 0.815 * radius * radius - span * span * 0.25);
            float midAlong = (minAlong + maxAlong) * 0.5;
            float midAcross = (minAcross + maxAcross) * 0.5 - offset;
            float centerX = heroX + midAlong * forwardX + midAcross * forwardZ;
            float centerZ = heroZ + midAlong * forwardZ - midAcross * forwardX;
            for (int member = 0; member < COMPANION_MAX; member++)
            {
                int memberIndex = GetCompanionIndex(pchar, member);
                if (memberIndex < 0) continue;
                ref admitted = GetCharacter(memberIndex);
                if (!CheckAttribute(admitted, "Ship.Pos.x") || !CheckAttribute(admitted, "Ship.Pos.z")) continue;
                float dx = stf(admitted.Ship.Pos.x) - centerX;
                float dz = stf(admitted.Ship.Pos.z) - centerZ;
                float distance = sqrt(dx * dx + dz * dz);
                float q = distance / radius;
                if (distance + 240.0 > radius || q < 0.68 || q > 0.88) known = false;
            }
            if (!known || radius > 1600.0)
            {
                pchar.GreatRecipe.Cancelled = true;
                pchar.GreatRecipe.Won = false;
                pchar.GreatRecipe.Phase = "returning";
                GreatRecipe_Journal("Эскадра растянулась, и странный силуэт пропал из виду. Если искать его снова, лучше держать корабли рядом.", false);
                GreatRecipe_Delay("GreatRecipe_Return", 0.1);
                return;
            }
            pchar.GreatRecipe.Vortex.X = centerX;
            pchar.GreatRecipe.Vortex.Z = centerZ;
            pchar.GreatRecipe.Vortex.Radius = radius;
            pchar.GreatRecipe.Vortex.Placed = true;
        }
        GreatRecipe_StartVortex();
        if (CheckAttribute(pchar, "GreatRecipe.Vortex.Sound")) StopSound(sti(pchar.GreatRecipe.Vortex.Sound), 0);
        pchar.GreatRecipe.Vortex.Sound = Play3DSound("storm_sea_water", stf(pchar.GreatRecipe.Vortex.X), 0.0, stf(pchar.GreatRecipe.Vortex.Z));
    }
}

void GreatRecipe_SeaReady()
{
    GreatRecipe_BeardSeaTasks();
    if (!GreatRecipe_Locked()) return;
    GreatRecipe_Locks();
    // FirstInit precedes native position readback. The first normal pulse
    // places a fresh field from live positions; saved fields keep their center.
    if (CheckAttribute(pchar, "GreatRecipe.Vortex.Placed")) GreatRecipe_PlaceVortex();
    if (GreatRecipe_Phase() == "storm") Whr_RequestWeather(1.0, 1.0, 28.0, 2.0, false, true);
    GreatRecipe_Queue();
    if (GreatRecipe_Phase() == "assault") GreatRecipe_AssaultTactics(true);
}

void GreatRecipe_RecoverHero()
{
    if (!LAi_IsDead(pchar)) return;
    // Stock land death sets this lock before GameOver. HP alone does not
    // release it or revive the dead native entity; the location reload does.
    InterfaceStates.Launched = false;
    Dead_DelLoginedCharacter(pchar);
    LAi_SetCurHPMax(pchar);
    LAi_SetPlayerType(pchar);
}

void GreatRecipe_Capture(string unused)
{
    if (GreatRecipe_Phase() != "capturing") return;
    GreatRecipe_RecoverHero();
    if (CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched) || sQuestNameAfterReload != "_")
    {
        GreatRecipe_Delay("GreatRecipe_Capture", 1.0);
        return;
    }
    GreatRecipe_ClearVortex(true);
    pchar.GreatRecipe.Phase = "captive";
    pchar.GreatRecipe.Task = "rations";
    DeleteAttribute(pchar, "GreatRecipe.Arrival.Complete");
    pchar.GreatRecipe.Arrival.Stage = "captain";
    DeleteAttribute(pchar, "GreatRecipe.Arrival.Resume");
    GreatRecipe_ArrivalClass();
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
    GreatRecipe_RecoverHero();
    GreatRecipe_ClearVortex(true);
    LAi_SetImmortal(pchar, true);
    Group_SetTaskNone("GreatRecipeSea");
    string note = "Борода приказал прекратить огонь. От его борта идут шлюпки.";
    pchar.GreatRecipe.Arrival.Cause = "seized";
    if (bAbordageStarted) { pchar.GreatRecipe.Arrival.Cause = "boarding"; note = "Я проиграл бой на палубе. Борода приказал взять меня живым и увести в трюм."; }
    else if (stf(pchar.Ship.HP) <= 0.0) { pchar.GreatRecipe.Arrival.Cause = "sinking"; note = "Флагман пошёл ко дну. Матросы Бороды вытащили меня из воды и связали."; }
    GreatRecipe_Journal(note, false);
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
            if (GreatRecipe_Phase() == "whirlpool" && !CheckAttribute(pchar, "GreatRecipe.Emerged") && !CheckAttribute(captain, "Ship.SeaEmergence"))
            {
                float radialX = stf(pchar.Ship.Pos.x) - stf(pchar.GreatRecipe.Vortex.X);
                float radialZ = stf(pchar.Ship.Pos.z) - stf(pchar.GreatRecipe.Vortex.Z);
                float angle = atan2(radialZ, radialX);
                float ring = stf(pchar.GreatRecipe.Vortex.Radius) * 0.78;
                angle = angle + 350.0 / ring;
                Group_SetXZ_AY("GreatRecipeSea", stf(pchar.GreatRecipe.Vortex.X) + cos(angle) * ring,
                    stf(pchar.GreatRecipe.Vortex.Z) + sin(angle) * ring, atan2(-sin(angle), cos(angle)));
                captain.Ship.SeaEmergence.Generation = sti(pchar.GreatRecipe.Generation);
                captain.Ship.SeaEmergence.Age = 0.0;
                captain.Ship.SeaEmergence.Duration = 6.35;
                captain.Ship.SeaEmergence.Active = 1;
            }
            else
            {
                if (CheckAttribute(captain, "Ship.Pos.x") && CheckAttribute(captain, "Ship.Pos.z") && CheckAttribute(captain, "Ship.Ang.y"))
                    Group_SetXZ_AY("GreatRecipeSea", stf(captain.Ship.Pos.x), stf(captain.Ship.Pos.z), stf(captain.Ship.Ang.y));
            }
            Sea_LoginGroup("GreatRecipeSea");
        }
        if (IsEntity(captain))
        {
            pchar.GreatRecipe.Emerged = true;
            DeleteAttribute(pchar, "GreatRecipe.EmergeRetries");
            if (CheckAttribute(captain, "Ship.SeaEmergence.Active") && sti(captain.Ship.SeaEmergence.Active)) return false;
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
        GreatRecipe_Journal("Мачты скрылись в пене, воронка распалась. Корабли целы. Рассказ рыбаков остаётся единственной зацепкой.", false);
        GreatRecipe_Delay("GreatRecipe_Return", 0.1);
    }
    return false;
}

void GreatRecipe_BoardingApproach()
{
    if (GreatRecipe_Phase() != "assault" || bAbordageStarted || bSeaReloadStarted) return;
    if (stf(pchar.Ship.HP) <= 0.0) { GreatRecipe_BeginCapture(false); return; }
    int index = GetCharacterIndex("GreatRecipeDreamBeard");
    if (index < 0) return;
    ref captain = GetCharacter(index);
    if (!IsEntity(captain) || LAi_IsDead(captain) || stf(captain.Ship.HP) <= 0.0) return;
    if (GreatRecipe_AssaultEscort(index) >= 0)
    {
        DeleteAttribute(captain, "GreatRecipeBoardingApproach");
        DeleteAttribute(captain, "Ship_SetTaskAbordage");
        return;
    }
    bool vulnerable = GetHullPercent(pchar) <= 30.0 || GetSailPercent(pchar) <= 12.0;
    if (!vulnerable && CheckAttribute(pchar, "Ship.Masts"))
    {
        aref masts; makearef(masts, pchar.Ship.Masts);
        int total = 0;
        int standing = 0;
        for (int mast = 0; mast < GetAttributesNum(masts); mast++)
        {
            aref part = GetAttributeN(masts, mast);
            int number = BI_MastRepairMainNumber(GetAttributeName(part));
            if (number <= 0 || number >= 100) continue;
            total++;
            if (stf(GetAttributeValue(part)) < 1.0) standing++;
        }
        vulnerable = total > 0 && standing == 0;
    }
    if (!vulnerable)
    {
        if (CheckAttribute(captain, "GreatRecipeBoardingApproach"))
        {
            DeleteAttribute(captain, "GreatRecipeBoardingApproach");
            DeleteAttribute(captain, "Ship_SetTaskAbordage");
            Ship_SetTaskAttack(SECONDARY_TASK, index, nMainCharacterIndex);
        }
        return;
    }
    captain.GreatRecipeBoardingApproach = true;
    captain.Ship_SetTaskAbordage = true;
    Ship_SetTaskAbordage(SECONDARY_TASK, index, nMainCharacterIndex);
    // Reuse stock grappling reach/angle/speed; steering remains native and
    // the player may escape or win the actual deck fight.
    if (!CheckAttribute(captain, "SeaAI.Update.Ships")) return;
    aref ships; makearef(ships, captain.SeaAI.Update.Ships);
    float grappling = stf(captain.TmpSkill.Grappling);
    float reach = Ship_CalcMaxAbordageDistance(grappling, Ship_CalcGrapplingPerk(captain));
    float distance = Ship_GetDistance2D(captain, pchar) -
        (stf(captain.Ship.BoxSize.x) + stf(pchar.Ship.BoxSize.x)) * 0.5;
    if (distance >= reach) return;
    for (int ship = 0; ship < GetAttributesNum(ships); ship++)
    {
        aref target = GetAttributeN(ships, ship);
        if (sti(target.idx) != nMainCharacterIndex) continue;
        if (abs(stf(target.d_ay)) >= Ship_CalcMaxAbordageAngle(grappling) ||
            abs(stf(target.RelativeSpeed)) >= Ship_CalcMaxAbordageSpeed(grappling)) return;
        pchar.GreatRecipe.Phase = "boarding";
        GreatRecipe_Journal("Красный прилив сблизился с повреждённым флагманом. Люди Бороды бросают абордажные крючья.", false);
        GreatRecipe_FirstBoarding("");
        return;
    }
}

bool GreatRecipe_AssaultTargetEligible(int target, int boss)
{
    if (target < 0 || target >= MAX_CHARACTERS || target == boss) return false;
    bool live = false;
    for (int ship = 0; ship < iNumShips; ship++) { if (Ships[ship] == target) live = true; }
    if (!live) return false;
    ref actor = GetCharacter(target);
    if (!IsEntity(actor) || LAi_IsDead(actor) || !CheckAttribute(actor, "Ship.HP") || stf(actor.Ship.HP) <= 0.0) return false;
    if (CheckAttribute(actor, "Ship.Sink") || CheckAttribute(actor, "SeaSurrender") || CheckAttribute(actor, "Surrendered")) return false;
    return GetRelation(boss, target) == RELATION_ENEMY;
}

float GreatRecipe_AssaultOpportunity(ref captain, ref target)
{
    float distance = Ship_GetDistance2D(captain, target);
    if (distance < 1.0) distance = 1.0;
    float dx = stf(target.Ship.Pos.x) - stf(captain.Ship.Pos.x);
    float dz = stf(target.Ship.Pos.z) - stf(captain.Ship.Pos.z);
    float angle = stf(captain.Ship.Ang.y);
    float lateral = abs(dx * cos(angle) - dz * sin(angle)) / distance;
    float reach = 0.0;
    float ready = 0.0;
    if (CheckAttribute(captain, "Ship.Cannons.Borts"))
    {
        aref borts; makearef(borts, captain.Ship.Cannons.Borts);
        for (int bort = 0; bort < GetAttributesNum(borts); bort++)
        {
            aref guns = GetAttributeN(borts, bort);
            string side = GetAttributeName(guns);
            if (side != "cannonl" && side != "cannonr") continue;
            if (CheckAttribute(guns, "MaxFireDistance") && stf(guns.MaxFireDistance) > reach) reach = stf(guns.MaxFireDistance);
            if (CheckAttribute(guns, "LoadedCannons") && stf(guns.LoadedCannons) > ready) ready = stf(guns.LoadedCannons);
        }
    }
    float score = 1000.0 / (100.0 + distance);
    if (reach > 0.0 && distance <= reach) score += 20.0 + lateral * 20.0;
    if (ready > 0.0 && reach > 0.0 && distance <= reach) score += 20.0 * lateral;
    score += (100.0 - GetHullPercent(target)) * 0.2;
    float incoming = abs(dx * cos(stf(target.Ship.Ang.y)) - dz * sin(stf(target.Ship.Ang.y))) / distance;
    score += incoming * GetCannonsNum(target) * 0.05;
    return score;
}

int GreatRecipe_AssaultEscort(int boss)
{
    int best = -1;
    float score = -1.0;
    ref captain = GetCharacter(boss);
    for (int slot = 1; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index == nMainCharacterIndex || !GreatRecipe_AssaultTargetEligible(index, boss)) continue;
        float opportunity = GreatRecipe_AssaultOpportunity(captain, GetCharacter(index));
        if (opportunity > score) { best = index; score = opportunity; }
    }
    return best;
}

void GreatRecipe_AssaultTactics(bool immediate)
{
    if (GreatRecipe_Phase() != "assault" || bAbordageStarted || !bSeaActive || !bSeaLoaded) return;
    if (!CheckAttribute(&Sea, "Whirlpool.Age")) return;
    int boss = GetCharacterIndex("GreatRecipeDreamBeard");
    if (boss < 0) return;
    ref captain = GetCharacter(boss);
    if (!IsEntity(captain) || LAi_IsDead(captain) || stf(captain.Ship.HP) <= 0.0) return;
    int selected = GreatRecipe_AssaultEscort(boss);
    bool escorts = selected >= 0;
    if (!escorts && GreatRecipe_AssaultTargetEligible(nMainCharacterIndex, boss)) selected = nMainCharacterIndex;
    if (selected < 0) { DeleteAttribute(captain, "SeaAI.PreferTaskTarget"); return; }
    float age = stf(Sea.Whirlpool.Age);
    int prior = -1;
    if (CheckAttribute(pchar, "GreatRecipe.Tactics.Target")) prior = sti(pchar.GreatRecipe.Tactics.Target);
    bool valid = GreatRecipe_AssaultTargetEligible(prior, boss) && (!escorts || prior != nMainCharacterIndex);
    if (!immediate && valid && CheckAttribute(pchar, "GreatRecipe.Tactics.Checked") && age < stf(pchar.GreatRecipe.Tactics.Checked) + 2.0) selected = prior;
    else
    {
        pchar.GreatRecipe.Tactics.Checked = age;
        if (valid && selected != prior)
        {
            float oldScore = GreatRecipe_AssaultOpportunity(captain, GetCharacter(prior));
            float newScore = GreatRecipe_AssaultOpportunity(captain, GetCharacter(selected));
            if ((CheckAttribute(pchar, "GreatRecipe.Tactics.Since") && age < stf(pchar.GreatRecipe.Tactics.Since) + 6.0) || newScore < oldScore * 1.25) selected = prior;
        }
    }
    if (selected != prior || !CheckAttribute(pchar, "GreatRecipe.Tactics.Since")) pchar.GreatRecipe.Tactics.Since = age;
    pchar.GreatRecipe.Tactics.Target = selected;
    captain.SeaAI.PreferTaskTarget = true;
    if (escorts)
    {
        DeleteAttribute(captain, "GreatRecipeBoardingApproach"); DeleteAttribute(captain, "Ship_SetTaskAbordage");
    }
    if (!CheckAttribute(captain, "GreatRecipeBoardingApproach") &&
        (!CheckAttribute(captain, "SeaAI.Task") || sti(captain.SeaAI.Task) != AITASK_ATTACK || !CheckAttribute(captain, "SeaAI.Task.Target") || sti(captain.SeaAI.Task.Target) != selected))
        Ship_SetTaskAttack(SECONDARY_TASK, boss, selected);
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
    if (stf(pchar.Ship.HP) <= 1.0)
    {
        // Older saves preserved a destroyed hull at one HP for forced boarding.
        // Recover that real loss without attempting to board the wreck.
        GreatRecipe_BeginCapture(false);
        return;
    }
    pchar.GreatRecipe.BoardingStarted = true;
    LAi_SetImmortal(pchar, false);
    Sea_AbordageStartNow(SHIP_ABORDAGE, index, false, false);
}

bool GreatRecipe_ClaimHullDefeat(ref captain)
{
    if (sti(captain.index) != nMainCharacterIndex) return false;
    if (GreatRecipe_Phase() != "whirlpool" && GreatRecipe_Phase() != "assault") return false;
    // Only actual lethal hull damage reaches this hook. Rescue is the loss
    // continuation; boarding belongs to an afloat, nearby damaged flagship.
    captain.Ship.HP = 0.0;
    GreatRecipe_BeginCapture(false);
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
    GreatRecipe_Journal("Борода проиграл. Теперь я знаю, что скрывается внутри его жемчуга. Над морем сгущаются тучи.", false);
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
    if (GreatRecipe_ShoreClaimDeath(index)) return true;
    string phase = GreatRecipe_Phase();
    if ((phase == "whirlpool" || phase == "assault" || phase == "boarding") && index == GetCharacterIndex("GreatRecipeDreamBeard"))
    {
        pchar.GreatRecipe.EarlyWin = true;
        pchar.GreatRecipe.Won = true;
        pchar.GreatRecipe.Phase = "loot";
        GreatRecipe_ClearVortex(true);
        GreatRecipe_RestoreLocks();
        GreatRecipe_SpawnBarrel(GetCharacter(index), false);
        GreatRecipe_Journal("Красный прилив побеждён. Среди обломков у его борта заметна бочка с капитанским клеймом.", false);
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
        GreatRecipe_Journal("Борода побеждён. У его корабля осталась бочка с капитанским клеймом; течение уже уносит обломки.", false);
        return false;
    }
    if (index != nMainCharacterIndex || !GreatRecipe_Locked()) return false;
    bool heroDead = LAi_IsDead(pchar);
    GreatRecipe_RecoverHero();
    if (phase == "whirlpool" || phase == "assault" || phase == "boarding")
    {
        GreatRecipe_BeginCapture(false);
        return true;
    }
    if (phase == "capturing") { GreatRecipe_RecoverHero(); return true; }
    if (phase == "captive")
    {
        if (CheckAttribute(pchar, "GreatRecipe.Fight.Active")) GreatRecipe_FightFinish(false);
        LAi_SetCurHPMax(pchar);
        if (heroDead && sQuestNameAfterReload == "_")
            DoQuestReloadToLocation("GreatRecipeHold", "goto", "goto3", "");
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
        pchar.GreatRecipe.Rewards.RepairPending = true;
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
    GreatRecipe_RewardRepair();
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
        GreatRecipe_ShoreCleanup();
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
    string body = "Я очнулся на Бермудах. Эскадра и вещи при мне, но история Бороды кажется слишком настоящей для сна. Лаки всё ещё ждёт вестей.";
    if (won) body = "Я очнулся на Бермудах. Мои корабли и вещи снова при мне, но красная жемчужина осталась в море. Обломки Бороды ещё может принести течением.";
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
    GreatRecipe_ShoreLocationLoaded(location);
    if (GreatRecipe_ShoreActive()) return;
    if (GreatRecipe_Phase() == "captive" && (location.id == "GreatRecipeHold" || location.id == "GreatRecipeDeck" || location.id == "GreatRecipeMess" || location.id == "GreatRecipeCabin"))
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Task")) pchar.GreatRecipe.Task = "rations";
        if (pchar.GreatRecipe.Task != "rations") pchar.GreatRecipe.Arrival.Complete = true;
        if (pchar.GreatRecipe.Task == "training") pchar.GreatRecipe.Task = "medicine";
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
        if (location.id == "GreatRecipeHold" && !CheckAttribute(pchar, "GreatRecipe.Arrival.Complete")) GreatRecipe_Arrived("");
    }
    if (GreatRecipe_Phase() == "returning")
    {
        if (location.id == "Pirates_town") GreatRecipe_Returned();
        else GreatRecipe_Delay("GreatRecipe_Return", 0.1);
    }
}

void GreatRecipe_OnLoad()
{
    DeleteAttribute(&worldMap, "GreatRecipeZoneAttack");
    DeleteAttribute(&wdmLoginToSea, "GreatRecipeMapAttack");
    DeleteAttribute(&wdmLoginToSea, "GreatRecipeOffshore");
    if (GreatRecipe_Phase() == "search") DeleteAttribute(pchar, "GreatRecipe.MapClock");
    GreatRecipe_FinalRewards();
    GreatRecipe_EnsureProduction();
    GreatRecipe_Upgrade();
    DeleteAttribute(pchar, "GreatRecipe.Loot.SeaPending");
    GreatRecipe_AllianceOnLoad();
    GreatRecipe_ShoreOnLoad();
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
    if (GreatRecipe_Phase() == "capturing")
    {
        GreatRecipe_RecoverHero();
        GreatRecipe_Delay("GreatRecipe_Capture", 0.1);
    }
    int index = FindLoadedLocation();
    if (index >= 0) GreatRecipe_LocationLoaded(&Locations[index]);
    if (bSeaActive && bSeaLoaded) GreatRecipe_SeaReady();
}

void GreatRecipe_PreLoadActors()
{
    GreatRecipe_RewardRepair();
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
    for (int crew = 0; crew < 6; crew++)
    {
        string crewId = "GreatRecipeBosun";
        if (crew == 1) crewId = "GreatRecipeCook";
        if (crew == 2) crewId = "GreatRecipePrisoner";
        if (crew == 3) crewId = "GreatRecipeGunner";
        if (crew == 4) crewId = "GreatRecipeMate";
        if (crew == 5) crewId = "GreatRecipeSurgeon";
        int crewIndex = GetCharacterIndex(crewId);
        if (crewIndex < 0) continue;
        ref crewActor = GetCharacter(crewIndex);
        if (IsEntity(crewActor)) LogoffCharacter(crewActor);
        crewActor.model = GreatRecipe_CrewModel(crew);
        FaceMaker(crewActor);
    }
}

void GreatRecipe_Upgrade()
{
    for (int role = 0; role < 3; role++)
    {
        string perkOwner = "GreatRecipeDreamBeard";
        if (role == 1) perkOwner = "GreatRecipeRealBeard";
        if (role == 2) perkOwner = "GreatRecipeDuelBeard";
        int actorIndex = GetCharacterIndex(perkOwner);
        if (actorIndex >= 0)
        {
            ref actor = GetCharacter(actorIndex);
            if (!LAi_IsDead(actor)) GreatRecipe_BeardPerks(actor);
            if (actor.id == "GreatRecipeDreamBeard" && GreatRecipe_Phase() != "assault") DeleteAttribute(actor, "SeaAI.PreferTaskTarget");
        }
    }
    string current = GreatRecipe_Phase();
    if ((current == "search" || current == "whirlpool" || current == "assault" || current == "boarding") &&
        CheckAttribute(pchar, "GreatRecipe.Version") && sti(pchar.GreatRecipe.Version) >= 2)
    {
        int boss = GetCharacterIndex("GreatRecipeDreamBeard");
        if (boss >= 0)
        {
            ref bossCaptain = GetCharacter(boss);
            if (CheckAttribute(bossCaptain, "Ship.GreatRecipeAirburstOnly") && !LAi_IsDead(bossCaptain))
            {
                float scale = 24.0;
                if (CheckAttribute(bossCaptain, "Ship.GreatRecipeHullScale")) scale = stf(bossCaptain.Ship.GreatRecipeHullScale);
                if (scale == 24.0 || scale == 8.0)
                {
                    // Normalize both saved max/current HP by the same factor;
                    // never heal damage or touch cargo, crew, armor or duels.
                    ref ship = GetRealShip(sti(bossCaptain.Ship.Type));
                    ship.HP = stf(ship.HP) * 4.0 / scale;
                    bossCaptain.Ship.HP = stf(bossCaptain.Ship.HP) * 4.0 / scale;
                    bossCaptain.Ship.GreatRecipeHullScale = 4;
                }
            }
        }
    }
    if (GreatRecipe_Phase() == "" || (CheckAttribute(pchar, "GreatRecipe.Version") && sti(pchar.GreatRecipe.Version) >= 2)) return;
    string phase = GreatRecipe_Phase();
    if (phase == "lost" && CheckAttribute(pchar, "GreatRecipe.HuntTaken"))
    {
        pchar.GreatRecipe.Phase = "pearl_search";
        GreatRecipe_Journal("Борода побеждён, но жемчужины у меня нет. На воде по-прежнему попадаются обломки и бочонки с его клеймом.", false);
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
        if (!CheckAttribute(pchar, "GreatRecipe.Vortex.Placed")) GreatRecipe_PlaceVortex();
        if (GreatRecipe_Phase() != phase) return;
        if (phase == "assault") { GreatRecipe_AssaultTactics(false); GreatRecipe_BoardingApproach(); }
        if (GreatRecipe_Phase() != phase) return;
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
            GreatRecipe_AssaultTactics(true);
            GreatRecipe_Journal("Из воронки вышел Красный прилив. Борода идёт по кругу будто по тихой воде; его железные жемчужины рвутся за бортом.", false);
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

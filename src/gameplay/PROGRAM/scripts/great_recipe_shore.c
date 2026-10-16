// A captive excursion; fleet escrow and the recipe reward remain core-owned.
#event_handler("CharactersStateUpdate", "GreatRecipe_ShoreCheck");

bool GreatRecipe_ShoreActive()
{
    return GreatRecipe_Phase() == "captive" && CheckAttribute(pchar, "GreatRecipe.Shore.Active") && sti(pchar.GreatRecipe.Shore.Active);
}

void GreatRecipe_ShoreTerminal(string name)
{
    if (!GreatRecipe_ShoreActive() || CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending") ||
        !GreatRecipe_ShoreLocation(pchar.location) || !CheckAttribute(pchar, "GreatRecipe.Shore.Sequence")) return;
    int sequence = sti(pchar.GreatRecipe.Shore.Sequence);
    string prefix = "GreatRecipeShoreYield_" + sequence + "_";
    if (name == prefix + "hero")
    {
        if (!CheckAttribute(pchar, "GreatRecipeShoreSequence") || sti(pchar.GreatRecipeShoreSequence) != sequence) return;
        pchar.GreatRecipe.Shore.Yield.hero = true;
        GreatRecipe_ShoreRetreat(true);
        return;
    }
    if (name != prefix + "helper" || !sti(pchar.GreatRecipe.Shore.Help) || CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear")) return;
    int index = GetCharacterIndex("GreatRecipeShoreBeard");
    if (index < 0) return;
    ref helper = GetCharacter(index);
    if (helper.location != "GreatRecipeVault" || CheckAttribute(helper, "GreatRecipeShoreWithdrawn") ||
        !CheckAttribute(helper, "GreatRecipeShoreSequence") || sti(helper.GreatRecipeShoreSequence) != sequence) return;
    pchar.GreatRecipe.Shore.Yield.helper = true;
    helper.GreatRecipeShoreWithdrawn = true;
    LAi_RemoveCheckMinHP(helper); LAi_SetStayType(helper); LAi_SetImmortal(helper, true);
    ChangeCharacterAddressGroup(helper, "GreatRecipeBay", "goto", "goto10");
    GreatRecipe_Journal("Борода ранен и отходит к шлюпке. Дальше придётся справиться самому; проход обратно в бухту остаётся открыт.", false);
}

bool GreatRecipe_ShoreArm(ref actor, string role, float floor)
{
    int sequence = sti(pchar.GreatRecipe.Shore.Sequence);
    bool owned = CheckAttribute(actor, "GreatRecipeShoreSequence") && sti(actor.GreatRecipeShoreSequence) == sequence;
    bool recover = role == "hero" || (actor.location == "GreatRecipeVault" && !CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear"));
    bool yielded = CheckAttribute(pchar, "GreatRecipe.Shore.Yield." + role);
    if (recover && (owned || CheckAttribute(pchar, "GreatRecipe.Shore.LegacyThresholds")) &&
        !CheckAttribute(actor, "chr_ai.hpchecker") && LAi_IsImmortal(actor)) yielded = true;
    if (recover && LAi_GetCharacterHP(actor) <= floor) yielded = true;
    actor.GreatRecipeShoreSequence = sequence;
    string name = "GreatRecipeShoreYield_" + sequence + "_" + role;
    if (yielded) { GreatRecipe_ShoreTerminal(name); return false; }
    LAi_SetImmortal(actor, false);
    LAi_SetCheckMinHP(actor, floor, true, name);
    return true;
}

bool GreatRecipe_ShoreLocation(string id)
{
    return id == "GreatRecipeBay" || id == "GreatRecipeVault";
}

bool GreatRecipe_ShoreMoonScene()
{
    return GreatRecipe_ShoreActive() && pchar.location == "GreatRecipeBay";
}

bool GreatRecipe_ShoreHelpAllowed()
{
    return CheckAttribute(pchar, "GreatRecipe.CaptainRespect") && sti(pchar.GreatRecipe.CaptainRespect);
}

void GreatRecipe_ShoreItems()
{
    for (int kind = 0; kind < 2; kind++)
    {
        string id = "GreatRecipeCacheKey";
        if (kind == 1) id = "GreatRecipeCacheLedger";
        if (Items_FindItemIdx(id) >= 0) continue;
        for (int slot = 0; slot < ITEMS_QUANTITY; slot++)
        {
            if (Items[slot].id != "item" + (slot + 1)) continue;
            ref item = &Items[slot];
            DeleteAttribute(item, "");
            item.id = id; item.index = slot;
            item.name = "itmname_" + id; item.describe = "itmdescr_" + id;
            item.model = "pursel"; item.picIndex = 9; item.picTexture = "ITEMS_1";
            item.ItemType = "QUESTITEMS"; item.Weight = 0.2; item.price = 0;
            item.rare = 0; item.uniq = true; item.shown = false;
            item.startLocation = ""; item.startLocator = "";
            item.useLocation = ""; item.useLocator = "";
            break;
        }
    }
}

bool GreatRecipe_ShoreRooms()
{
    if (FindLocation("Shore3") < 0 || FindLocation("Bermudes_Dungeon") < 0) return false;
    for (int room = 0; room < 2; room++)
    {
        string id = "GreatRecipeBay"; string original = "Shore3";
        string label = "Лунная бухта";
        if (room == 1) { id = "GreatRecipeVault"; original = "Bermudes_Dungeon"; label = "Склеп старой команды"; }
        if (FindLocation(id) >= 0) continue;
        if (nLocationsNum >= MAX_LOCATIONS) return false;
        ref place = &Locations[nLocationsNum];
        CopyAttributes(place, &Locations[FindLocation(original)]);
        place.id = id; place.id.label = label; place.index = nLocationsNum;
        place.parent_colony = "Pirates"; place.DisableOfficers = true;
        DeleteAttribute(place, "worldmap"); DeleteAttribute(place, "fastreload");
        DeleteAttribute(place, "townsack"); DeleteAttribute(place, "items");
        DeleteAttribute(place, "reload"); DeleteAttribute(place, "boarding");
        DeleteAttribute(place, "QuestlockWeather");
        // Preset only. hours/minutes would change the player's actual clock.
        place.QuestlockWeather = "23 Hour";
        for (int box = 1; box <= 4; box++) DeleteAttribute(place, "box" + box);
        LAi_LocationFantomsGen(place, false);
        LAi_LocationFightDisable(place, false);
        nLocationsNum++;
    }
    ref bay = &Locations[FindLocation("GreatRecipeBay")];
    DeleteAttribute(bay, "monsters");
    LAi_LocationDisableMonstersGen(bay.id, true);
    bay.reload.vault.name = "reload2_back"; bay.reload.vault.go = "GreatRecipeVault";
    bay.reload.vault.emerge = "reload2"; bay.reload.vault.autoreload = "0";
    bay.reload.vault.label = "Склеп";
    bay.reload.boat.name = "boat"; bay.reload.boat.go = "GreatRecipeDeck";
    bay.reload.boat.emerge = "reload_hold"; bay.reload.boat.autoreload = "0";
    bay.reload.boat.label = "Вернуться на корабль";
    ref vault = &Locations[FindLocation("GreatRecipeVault")];
    DeleteAttribute(vault, "monsters");
    LAi_LocationDisableMonstersGen(vault.id, true);
    vault.reload.bay.name = "reload2_back"; vault.reload.bay.go = "GreatRecipeBay";
    vault.reload.bay.emerge = "reload2"; vault.reload.bay.autoreload = "1";
    vault.reload.bay.label = "Бухта";
    return true;
}

// Called before the stock quest-box owner: our clones never generate random loot.
bool GreatRecipe_ShoreQuestBox(ref place, string box)
{
    if (!GreatRecipe_ShoreLocation(place.id)) return false;
    if (!CheckAttribute(place, box)) place.(box) = "";
    place.(box).opened = true;
    return true;
}

void GreatRecipe_ShoreSeed()
{
    ref bay = &Locations[FindLocation("GreatRecipeBay")];
    ref vault = &Locations[FindLocation("GreatRecipeVault")];
    if (CheckAttribute(pchar, "GreatRecipe.Shore.Seeded"))
    {
        // Only the untouched, still locked former reward bundle has provenance.
        if (!CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear") && !CheckAttribute(pchar, "GreatRecipe.Shore.RewardDeferred") &&
            CheckAttribute(vault, "box2.QuestClosed") && sti(vault.box2.QuestClosed) &&
            CheckAttribute(vault, "box2.items.potion2") && sti(vault.box2.items.potion2) == 2 &&
            CheckAttribute(vault, "box2.items.jewelry5") && sti(vault.box2.items.jewelry5) == 1)
        {
            DeleteAttribute(vault, "box2.items.potion2"); DeleteAttribute(vault, "box2.items.jewelry5");
            pchar.GreatRecipe.Shore.RewardDeferred = true;
        }
        return;
    }
    bay.box1.items.GreatRecipeCacheKey = 1;
    vault.box2.items.GreatRecipeCacheLedger = 1;
    vault.box2.QuestClosed = true;
    pchar.GreatRecipe.Shore.RewardDeferred = true;
    pchar.GreatRecipe.Shore.Seeded = true;
}

void GreatRecipe_ShoreStart(bool help)
{
    if (GreatRecipe_Phase() != "captive" || pchar.GreatRecipe.Task != "shore" || GreatRecipe_ShoreActive()) return;
    if (CheckAttribute(pchar, "GreatRecipe.Fight.Active")) return;
    GreatRecipe_ShoreItems();
    if (!GreatRecipe_ShoreRooms()) { Log_Info("Шлюпка пока не готова к высадке."); return; }
    GreatRecipe_ShoreSeed();
    pchar.GreatRecipe.Shore.Help = help && GreatRecipe_ShoreHelpAllowed();
    if (!CheckAttribute(pchar, "GreatRecipe.Shore.Sequence")) pchar.GreatRecipe.Shore.Sequence = 0;
    pchar.GreatRecipe.Shore.Sequence = sti(pchar.GreatRecipe.Shore.Sequence) + 1;
    DeleteAttribute(pchar, "GreatRecipe.Shore.Yield");
    DeleteAttribute(pchar, "GreatRecipe.Shore.LegacyThresholds");
    pchar.GreatRecipe.Shore.Active = true;
    DeleteAttribute(pchar, "GreatRecipe.Shore.ReturnPending");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreReturn");
    GreatRecipe_Journal("Борода хочет вернуть судовую книгу своей прежней команды из берегового тайника. Шлюпочник останется в бухте, пока я ищу проход.", false);
    GreatRecipe_Delay("GreatRecipe_ShoreLand", 0.1);
}

void GreatRecipe_ShoreLand(string unused)
{
    if (!GreatRecipe_ShoreActive() || CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) return;
    if (GreatRecipe_ShoreLocation(pchar.location)) return;
    if (bAbordageStarted || CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched) || sQuestNameAfterReload != "_" || CheckAttribute(pchar, "GreatRecipe.Fight.Active"))
    { GreatRecipe_Delay("GreatRecipe_ShoreLand", 0.5); return; }
    if (!DoQuestReloadToLocation("GreatRecipeBay", "reload", "boat", ""))
        GreatRecipe_Delay("GreatRecipe_ShoreLand", 0.5);
}

void GreatRecipe_ShoreHideEnemies()
{
    for (int fighter = 0; fighter < 2; fighter++)
    {
        int index = GetCharacterIndex("GreatRecipeBone" + fighter);
        if (index < 0) continue;
        ref enemy = GetCharacter(index);
        if (IsEntity(enemy)) LogoffCharacter(enemy);
        ChangeCharacterAddress(enemy, "none", "");
    }
    LAi_group_SetRelation("GreatRecipeBones", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
    LAi_group_SetRelation("GreatRecipeBones", "GreatRecipeShoreFriends", LAI_GROUP_NEITRAL);
    LAi_group_SetAlarm("GreatRecipeBones", LAI_GROUP_PLAYER, 0.0);
}

void GreatRecipe_ShoreEnemies(string room)
{
    if (room != "GreatRecipeVault" || CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear")) return;
    int rank = sti(pchar.rank);
    if (rank < 1) rank = 1;
    if (rank > 25) rank = 25;
    int fencing = 25 + rank;
    float hp = LAi_GetCharacterMaxHP(pchar) * 0.55;
    if (hp < 35.0) hp = 35.0;
    if (hp > 220.0) hp = 220.0;
    for (int member = 0; member < 2; member++)
    {
        int number = member;
        string id = "GreatRecipeBone" + number;
        int index = GetCharacterIndex(id);
        ref enemy;
        if (index < 0)
        {
            index = NPC_GenerateCharacter(id, "Skel" + (1 + number % 3), "skeleton", "skeleton", rank, PIRATE, -1, false);
            enemy = GetCharacter(index);
            FantomMakeCoolFighter(enemy, rank, fencing, 0, "blade1", "", 0);
            DeleteAttribute(enemy, "items.pistol1");
            LAi_SetHP(enemy, hp, hp);
            enemy.GreatRecipeShoreBone = true;
        }
        else enemy = GetCharacter(index);
        if (LAi_IsDead(enemy) || LAi_GetCharacterHP(enemy) <= 0.0) continue;
        LAi_SetImmortal(enemy, false); LAi_SetWarriorTypeNoGroup(enemy);
        LAi_group_MoveCharacter(enemy, "GreatRecipeBones");
        string group = "monsters"; string locator = "monster9";
        if (member == 1) locator = "monster13";
        if (enemy.location != room || !IsEntity(enemy)) ChangeCharacterAddressGroup(enemy, room, group, locator);
    }
    LAi_group_SetRelation("GreatRecipeBones", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
    LAi_group_FightGroups("GreatRecipeBones", LAI_GROUP_PLAYER, true);
}

void GreatRecipe_ShoreFriends(string room)
{
    ref boatman = GreatRecipe_Actor("GreatRecipeBoatman", "pirate_1");
    boatman.name = "Сэм"; boatman.lastname = "";
    boatman.Dialog.Filename = "GreatRecipeShore.c";
    boatman.Dialog.CurrentNode = "First time"; boatman.Dialog.TempNode = "First time";
    LAi_SetImmortal(boatman, true); LAi_SetStayType(boatman);
    LAi_group_MoveCharacter(boatman, "GreatRecipeShoreFriends");
    ChangeCharacterAddressGroup(boatman, "GreatRecipeBay", "goto", "goto5");
    if (!sti(pchar.GreatRecipe.Shore.Help)) return;
    ref captain = GreatRecipe_Actor("GreatRecipeShoreBeard", "RedBeard");
    captain.name = "Красная"; captain.lastname = "Борода";
    captain.Dialog.Filename = "GreatRecipeShore.c";
    captain.Dialog.CurrentNode = "First time"; captain.Dialog.TempNode = "First time";
    if (!CheckAttribute(captain, "GreatRecipeShoreReady"))
    { GreatRecipe_BeardFighter(captain); captain.GreatRecipeShoreReady = true; }
    if (LAi_IsDead(captain)) { ChangeCharacterAddress(captain, "none", ""); return; }
    if (CheckAttribute(captain, "GreatRecipeShoreWithdrawn"))
    {
        LAi_SetStayType(captain); LAi_SetImmortal(captain, true);
        ChangeCharacterAddressGroup(captain, "GreatRecipeBay", "goto", "goto10");
        return;
    }
    LAi_group_MoveCharacter(captain, "GreatRecipeShoreFriends");
    if (room == "GreatRecipeVault" && !CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear"))
    {
        if (!GreatRecipe_ShoreArm(captain, "helper", 40.0)) return;
        LAi_SetWarriorTypeNoGroup(captain);
        LAi_group_SetRelation("GreatRecipeBones", "GreatRecipeShoreFriends", LAI_GROUP_ENEMY);
        LAi_group_FightGroups("GreatRecipeBones", "GreatRecipeShoreFriends", true);
    }
    else { LAi_SetStayType(captain); LAi_SetImmortal(captain, true); }
    if (room == "GreatRecipeVault")
    {
        if (captain.location != room || !IsEntity(captain)) ChangeCharacterAddressGroup(captain, room, "monsters", "monster30");
    }
    else ChangeCharacterAddressGroup(captain, "GreatRecipeBay", "goto", "goto10");
}

float GreatRecipe_ShoreBladeDamage(aref actor, aref target, float damage)
{
    if (!GreatRecipe_ShoreActive() || !CheckAttribute(actor, "GreatRecipeShoreBone") || sti(target.index) != nMainCharacterIndex) return damage;
    float cap = LAi_GetCharacterMaxHP(pchar) * 0.18;
    if (damage > cap) damage = cap;
    return damage;
}

void GreatRecipe_ShoreLocationLoaded(aref place)
{
    if (!GreatRecipe_ShoreActive()) return;
    if (place.id == "GreatRecipeDeck")
    {
        // An active save on deck can be a not-yet-started landing, not a return.
        if (CheckAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand") && !CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) return;
        GreatRecipe_ShoreReturned(); return;
    }
    if (!GreatRecipe_ShoreLocation(place.id)) return;
    if (CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) { GreatRecipe_Delay("GreatRecipe_ShoreReturn", 0.1); return; }
    if (!CheckAttribute(pchar, "GreatRecipe.Shore.Sequence"))
    { pchar.GreatRecipe.Shore.Sequence = 1; pchar.GreatRecipe.Shore.LegacyThresholds = true; }
    GreatRecipe_Locks();
    chrDisableReloadToLocation = false;
    if (!GreatRecipe_ShoreArm(pchar, "hero", LAi_GetCharacterMaxHP(pchar) * 0.2)) return;
    LAi_SetPlayerType(pchar);
    GreatRecipe_ShoreFriends(place.id);
    DeleteAttribute(pchar, "GreatRecipe.Shore.LegacyThresholds");
    if (place.id == "GreatRecipeBay")
    {
        LocatorReloadEnterDisable("GreatRecipeBay", "reload2_back", GetCharacterItem(pchar, "GreatRecipeCacheKey") < 1 && !CheckAttribute(pchar, "GreatRecipe.Shore.KeyFound"));
    }
    GreatRecipe_ShoreEnemies(place.id);
    GreatRecipe_ShoreCheck();
}

void GreatRecipe_ShoreCheck()
{
    if (!GreatRecipe_ShoreActive() || !GreatRecipe_ShoreLocation(pchar.location) || CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Shore.Sequence")) return;
    string prefix = "GreatRecipeShoreYield_" + pchar.GreatRecipe.Shore.Sequence + "_";
    if (LAi_GetCharacterHP(pchar) <= LAi_GetCharacterMaxHP(pchar) * 0.2 ||
        (!CheckAttribute(pchar, "chr_ai.hpchecker") && LAi_IsImmortal(pchar) && CheckAttribute(pchar, "GreatRecipeShoreSequence") &&
         sti(pchar.GreatRecipeShoreSequence) == sti(pchar.GreatRecipe.Shore.Sequence)))
    { GreatRecipe_ShoreTerminal(prefix + "hero"); return; }
    int helperIndex = GetCharacterIndex("GreatRecipeShoreBeard");
    if (helperIndex >= 0 && !CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear"))
    {
        ref helper = GetCharacter(helperIndex);
        if (helper.location == "GreatRecipeVault" && !CheckAttribute(helper, "GreatRecipeShoreWithdrawn") &&
            (LAi_GetCharacterHP(helper) <= 40.0 || (!CheckAttribute(helper, "chr_ai.hpchecker") && LAi_IsImmortal(helper) &&
             CheckAttribute(helper, "GreatRecipeShoreSequence") && sti(helper.GreatRecipeShoreSequence) == sti(pchar.GreatRecipe.Shore.Sequence))))
            GreatRecipe_ShoreTerminal(prefix + "helper");
    }
    string room = pchar.location;
    if (room == "GreatRecipeVault" && !CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear"))
    {
        for (int member = 0; member < 2; member++)
        {
            int index = GetCharacterIndex("GreatRecipeBone" + member);
            if (index < 0) return;
            ref enemy = GetCharacter(index);
            if (!LAi_IsDead(enemy) && LAi_GetCharacterHP(enemy) > 0.0) return;
        }
        pchar.GreatRecipe.Shore.VaultClear = true;
        ref place = &Locations[FindLocation(room)];
        DeleteAttribute(place, "box2.QuestClosed");
        GreatRecipe_Journal("Скелеты больше не двигаются. Старый тайник теперь можно осмотреть.", false);
        int captainIndex = GetCharacterIndex("GreatRecipeShoreBeard");
        if (captainIndex >= 0) { ref captain = GetCharacter(captainIndex); LAi_SetStayType(captain); LAi_SetImmortal(captain, true); }
    }
    if (room == "GreatRecipeBay" && GetCharacterItem(pchar, "GreatRecipeCacheKey") > 0)
    {
        pchar.GreatRecipe.Shore.KeyFound = true;
        LocatorReloadEnterDisable(room, "reload2_back", false);
    }
    if (room == "GreatRecipeVault" && GetCharacterItem(pchar, "GreatRecipeCacheLedger") > 0 && !CheckAttribute(pchar, "GreatRecipe.Shore.Recovered"))
    {
        pchar.GreatRecipe.Shore.Recovered = true;
        GreatRecipe_Journal("В книге — невыплаченные доли и подпись Ратли под последней выдачей пороха. Вот почему Борода хотел её вернуть.", false);
    }
}

void GreatRecipe_ShoreRetreat(bool defeated)
{
    if (!GreatRecipe_ShoreActive() || CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) return;
    pchar.GreatRecipe.Shore.ReturnPending = true;
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreReturn");
    LAi_RemoveCheckMinHP(pchar); LAi_SetImmortal(pchar, true); LAi_SetFightMode(pchar, false);
    if (defeated)
    {
        float hp = LAi_GetCharacterMaxHP(pchar) * 0.3;
        if (LAi_GetCharacterHP(pchar) < hp) LAi_SetCurHP(pchar, hp);
        GreatRecipe_Journal("Шлюпочник вытащил меня из-под клинков. Тайник остался на берегу; после перевязки можно попробовать снова.", false);
    }
    GreatRecipe_Delay("GreatRecipe_ShoreReturn", 0.1);
}

void GreatRecipe_ShoreReturn(string unused)
{
    if (!GreatRecipe_ShoreActive() || !CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending")) return;
    if (bAbordageStarted || CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched) || sQuestNameAfterReload != "_")
    { GreatRecipe_Delay("GreatRecipe_ShoreReturn", 0.5); return; }
    if (pchar.location == "GreatRecipeDeck") { GreatRecipe_ShoreReturned(); return; }
    if (!DoQuestReloadToLocation("GreatRecipeDeck", "reload", "reload_hold", ""))
        GreatRecipe_Delay("GreatRecipe_ShoreReturn", 0.5);
}

void GreatRecipe_ShoreReturned()
{
    if (!GreatRecipe_ShoreActive()) return;
    DeleteAttribute(pchar, "GreatRecipe.Shore.Active");
    DeleteAttribute(pchar, "GreatRecipe.Shore.ReturnPending");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreReturn");
    LAi_RemoveCheckMinHP(pchar); LAi_SetImmortal(pchar, true);
    GreatRecipe_ShoreHideEnemies();
    // A retreat is a fresh combat attempt, never a victory or an inventory reset.
    for (int fighter = 0; fighter < 2; fighter++)
    {
        if (CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear")) continue;
        int index = GetCharacterIndex("GreatRecipeBone" + fighter);
        if (index >= 0) { ref enemy = GetCharacter(index); LAi_SetCurHPMax(enemy); }
    }
    for (int role = 0; role < 2; role++)
    {
        string id = "GreatRecipeBoatman"; if (role == 1) id = "GreatRecipeShoreBeard";
        int actorIndex = GetCharacterIndex(id);
        if (actorIndex >= 0) { ref actor = GetCharacter(actorIndex); LAi_RemoveCheckMinHP(actor); if (IsEntity(actor)) LogoffCharacter(actor); ChangeCharacterAddress(actor, "none", ""); }
    }
    GreatRecipe_CaptiveActors();
}

bool GreatRecipe_ShoreClaimDeath(int index)
{
    if (index != nMainCharacterIndex || !GreatRecipe_ShoreActive()) return false;
    GreatRecipe_ShoreRetreat(true);
    return true;
}

bool GreatRecipe_ShoreHandOver()
{
    if (GreatRecipe_Phase() != "captive" || pchar.GreatRecipe.Task != "shore" || GreatRecipe_ShoreActive()) return false;
    if (!CheckAttribute(pchar, "GreatRecipe.Shore.VaultClear") || GetCharacterItem(pchar, "GreatRecipeCacheLedger") < 1) return false;
    TakeItemFromCharacter(pchar, "GreatRecipeCacheLedger");
    TakeItemFromCharacter(pchar, "GreatRecipeCacheKey");
    pchar.GreatRecipe.Shore.Delivered = true;
    if (CheckAttribute(pchar, "GreatRecipe.Shore.RewardDeferred")) pchar.GreatRecipe.Rewards.ShorePending = true;
    GreatRecipe_TaskAdvance("mutiny", "Борода забрал книгу и вызвал Ратли. Спор о долях перекинулся на палубу.");
    return true;
}

void GreatRecipe_ShoreOnLoad()
{
    if (GreatRecipe_Phase() != "captive" || !CheckAttribute(pchar, "GreatRecipe.Task") || pchar.GreatRecipe.Task != "shore") return;
    GreatRecipe_ShoreItems();
    if (!GreatRecipe_ShoreRooms()) return;
    if (!GreatRecipe_ShoreActive()) return;
    if (CheckAttribute(pchar, "GreatRecipe.Shore.ReturnPending"))
    {
        DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand");
        GreatRecipe_Delay("GreatRecipe_ShoreReturn", 0.1);
    }
    else if (!GreatRecipe_ShoreLocation(pchar.location) &&
        (pchar.location != "GreatRecipeDeck" || CheckAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand")))
        GreatRecipe_Delay("GreatRecipe_ShoreLand", 0.1);
}

void GreatRecipe_ShoreCleanup()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Shore")) return;
    GreatRecipe_ShoreHideEnemies();
    for (int role = 0; role < 2; role++)
    {
        string id = "GreatRecipeBoatman"; if (role == 1) id = "GreatRecipeShoreBeard";
        int index = GetCharacterIndex(id);
        if (index >= 0) { ref actor = GetCharacter(index); LAi_RemoveCheckMinHP(actor); if (IsEntity(actor)) LogoffCharacter(actor); ChangeCharacterAddress(actor, "none", ""); }
    }
    DeleteAttribute(pchar, "GreatRecipe.Shore.Active");
    DeleteAttribute(pchar, "GreatRecipe.Shore.ReturnPending");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreLand");
    DeleteAttribute(pchar, "GreatRecipe.Deferred.GreatRecipe_ShoreReturn");
    LAi_group_Delete("GreatRecipeBones"); LAi_group_Delete("GreatRecipeShoreFriends");
}

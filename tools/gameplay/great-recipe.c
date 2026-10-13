// Bermuda's dream owns its fleet escrow; ordinary sea/death paths remain stock.
#event_handler("GreatRecipePulse", "GreatRecipe_Pulse");

string GreatRecipe_Phase()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Phase")) return "";
    return pchar.GreatRecipe.Phase;
}

bool GreatRecipe_Locked()
{
    string phase = GreatRecipe_Phase();
    return phase == "whirlpool" || phase == "assault" || phase == "captive" ||
        phase == "duel" || phase == "storm" || phase == "returning";
}

bool GreatRecipe_Unlocked()
{
    return GreatRecipe_Phase() == "won";
}

void GreatRecipe_Journal(string body, bool complete)
{
    if (complete) Journal_CloseSystemEntry("GreatRecipe", "GreatRecipe", body);
    else Journal_SetSystemEntry("GreatRecipe", "GreatRecipe", body);
}

bool GreatRecipe_Moonlit()
{
    // An explicit repeatable calendar window; no hidden date/time manipulation.
    return (GetTime() >= 22.0 || GetTime() < 4.0) &&
        GetDataDay() >= 12 && GetDataDay() <= 18 && !Whr_IsRain() && !Whr_IsStorm();
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
    GreatRecipe_Journal("Лаки Росин просит найти жемчуг черепахи — секрет шрапнельных бомб Красной Бороды. Искать остров нужно в море у Бермуд при ясной луне: с 22 до 4 часов, с 12 по 18 число. Лаки предупреждает: назад можно не вернуться.", false);
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
    if (!GreatRecipe_Locked() || !CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet")) return;
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

ref GreatRecipe_Actor(string id, string model)
{
    int index = GetCharacterIndex(id);
    ref actor;
    if (index < 0) actor = GetCharacter(NPC_GenerateCharacter(id, model, "man", "man", 30, PIRATE, -1, false));
    else actor = GetCharacter(index);
    return actor;
}

void GreatRecipe_Locks()
{
    bDisableFastReload = true;
    bQuestDisableMapEnter = true;
    // Character/ship panels stay usable for a fair duel's charge/control setup.
    chrDisableReloadToLocation = GreatRecipe_Phase() == "captive";
}

void GreatRecipe_CaptiveActors()
{
    GreatRecipe_Locks();
    ref bosun = GreatRecipe_Actor("GreatRecipeBosun", "pirate_1");
    bosun.name = "Морган";
    bosun.lastname = "Крюк";
    bosun.Dialog.Filename = "GreatRecipe.c";
    bosun.Dialog.CurrentNode = "First time";
    bosun.Dialog.TempNode = "First time";
    LAi_SetImmortal(bosun, true);
    LAi_SetStayType(bosun);
    LAi_group_MoveCharacter(bosun, "GreatRecipeHoldGroup");
    ChangeCharacterAddressGroup(bosun, "GreatRecipeHold", "goto", "goto1");
    ref beard = GreatRecipe_Actor("GreatRecipeDreamBeard", "pirate_6");
    beard.name = "Красная";
    beard.lastname = "Борода";
    beard.Dialog.Filename = "GreatRecipe.c";
    beard.Dialog.CurrentNode = "First time";
    beard.Dialog.TempNode = "First time";
    LAi_SetImmortal(beard, true);
    LAi_SetStayType(beard);
    LAi_group_MoveCharacter(beard, "GreatRecipeHoldGroup");
    DeleteAttribute(beard, "AlwaysEnemy");
    if (sti(pchar.GreatRecipe.Days) >= 3 || sti(pchar.GreatRecipe.Notice) >= 2)
        ChangeCharacterAddressGroup(beard, "GreatRecipeHold", "goto", "goto2");
    else ChangeCharacterAddress(beard, "none", "");
}

void GreatRecipe_Chore(string action)
{
    if (GreatRecipe_Phase() != "captive" || pchar.location != "GreatRecipeHold") return;
    if (action != "scrub" && action != "help" && action != "defy") return;
    pchar.GreatRecipe.Days = sti(pchar.GreatRecipe.Days) + 1;
    if (action == "scrub") pchar.GreatRecipe.Last = "День ушёл на палубу. Боцман ворчит, но начинает доверять тебе.";
    if (action == "help")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Helped")) pchar.GreatRecipe.Notice = sti(pchar.GreatRecipe.Notice) + 1;
        pchar.GreatRecipe.Helped = true;
        pchar.GreatRecipe.Last = "Ты помог раненому пленнику и вывел его из-под удара. Команда это запомнила.";
    }
    if (action == "defy")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Defied")) pchar.GreatRecipe.Notice = sti(pchar.GreatRecipe.Notice) + 1;
        pchar.GreatRecipe.Defied = true;
        pchar.GreatRecipe.Last = "Ты сорвал крепление орудия и отказался назвать виноватого. Борода слышал, как ты разговаривал с боцманом.";
    }
    WaitDate("", 0, 0, 1, 0, 0);
    GreatRecipe_CaptiveActors();
    string result = pchar.GreatRecipe.Last + " В трюме прошло " + pchar.GreatRecipe.Days + " дней.";
    if (sti(pchar.GreatRecipe.Days) >= 3 || sti(pchar.GreatRecipe.Notice) >= 2) result += " Борода спустился в трюм и зовёт тебя к себе.";
    GreatRecipe_Journal(result, false);
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
    LAi_SetImmortal(captain, false);
    LAi_SetCurHPMax(captain);
    captain.DontRansackCaptain = true;
    captain.AlwaysEnemy = true;
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

void GreatRecipe_PrepareSea(ref login)
{
    string phase = GreatRecipe_Phase();
    if (login.Island != "Bermudes") return;
    if (phase == "search" && GreatRecipe_Moonlit() && !CheckAttribute(login, "QuestGroups") &&
        !CheckAttribute(pchar, "Custody") && !bAbordageStarted && !bQuestDisableMapEnter && GreatRecipe_FleetFree())
    {
        if (GreatRecipe_Hold() < 0) return;
        GreatRecipe_Snapshot();
        // Scripted capture protects the player/captains; the equal duel does not.
        for (int slot = 0; slot < COMPANION_MAX; slot++)
        {
            int captainIndex = GetCompanionIndex(pchar, slot);
            if (captainIndex >= 0 && captainIndex != nMainCharacterIndex) LAi_SetImmortal(GetCharacter(captainIndex), true);
        }
        pchar.GreatRecipe.Phase = "whirlpool";
        pchar.GreatRecipe.Elapsed = 0;
        pchar.GreatRecipe.Vortex.X = stf(login.PlayerGroup.x) + 35.0;
        pchar.GreatRecipe.Vortex.Z = stf(login.PlayerGroup.z);
        pchar.GreatRecipe.Vortex.Radius = 90.0;
        pchar.GreatRecipe.Vortex.Intensity = 1.0;
        pchar.GreatRecipe.Vortex.Duration = 24.0;
        pchar.GreatRecipe.Vortex.AppliedLoss = 0.0;
        ref dream = GreatRecipe_Actor("GreatRecipeDreamBeard", "pirate_6");
        GreatRecipe_Ship(dream, SHIP_CORVETTE, "Красный прилив", 90);
        dream.Ship.Cannons.Charge.Type = GOOD_BOMBS;
        dream.ShipCannonChargeType = GOOD_BOMBS;
        SetCharacterGoods(dream, GOOD_BOMBS, 600);
        GreatRecipe_Group(login, dream);
        Group_SetTaskNone("GreatRecipeSea");
        GreatRecipe_Locks();
        GreatRecipe_Journal("Остров под луной распался на кольца пены. Вместо берега — водоворот. За ним показался красный корабль.", false);
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
    if (!Whr_IsStorm() || !Whr_IsRain() || !isEntity(&Lightning)) return;
    if (CheckAttribute(login, "QuestGroups")) return;
    ref real = GreatRecipe_Actor("GreatRecipeRealBeard", "pirate_6");
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

void GreatRecipe_StartVortex()
{
    if (!isEntity(&Sea) || !CheckAttribute(&Sea, "Whirlpool.Active")) return;
    if (sti(Sea.Whirlpool.Active)) return;
    Sea.Whirlpool.CenterX = stf(pchar.GreatRecipe.Vortex.X);
    Sea.Whirlpool.CenterZ = stf(pchar.GreatRecipe.Vortex.Z);
    Sea.Whirlpool.Radius = stf(pchar.GreatRecipe.Vortex.Radius);
    Sea.Whirlpool.Intensity = stf(pchar.GreatRecipe.Vortex.Intensity);
    Sea.Whirlpool.Duration = stf(pchar.GreatRecipe.Vortex.Duration);
    Sea.Whirlpool.ResumeAge = stf(pchar.GreatRecipe.Elapsed);
    pchar.GreatRecipe.Vortex.LastQuery = 0.0;
    pchar.GreatRecipe.Vortex.LastSampleAge = stf(pchar.GreatRecipe.Elapsed);
    Sea.Whirlpool.Start = 1;
}

void GreatRecipe_VortexDamage()
{
    if (GreatRecipe_Phase() != "whirlpool") return;
    if (!CheckAttribute(&Sea, "Whirlpool.Active") || !isEntity(&Sea)) return;
    if (LAi_IsDead(pchar) || LAi_IsImmortal(pchar) || sti(pchar.Ship.Type) == SHIP_NOTUSED) return;
    float fraction = 0.0;
    SendMessage(&Sea, "lae", MSG_SEA_WHIRLPOOL_CONTACT, pchar, &fraction);
    float previous = stf(pchar.GreatRecipe.Vortex.LastQuery);
    pchar.GreatRecipe.Vortex.LastQuery = fraction;
    float delta = fraction - previous;
    if (delta <= 0.0) return;
    float applied = stf(pchar.GreatRecipe.Vortex.AppliedLoss);
    if (delta > 0.5 - applied) delta = 0.5 - applied;
    if (delta <= 0.0) return;
    pchar.GreatRecipe.Vortex.AppliedLoss = applied + delta;
    float damage = stf(pchar.GreatRecipe.Escrow.Fleet.s0.RealShip.HP) * delta;
    // The dream's current cannot kill a damaged starting hull before capture.
    if (damage > stf(pchar.Ship.HP) - 1.0) damage = stf(pchar.Ship.HP) - 1.0;
    if (damage > 0.0) Ship_ApplyHullHitpoints(pchar, damage, KILL_BY_TOUCH, -1);
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
    if (GreatRecipe_Phase() == "whirlpool")
    {
        GreatRecipe_StartVortex();
        if (CheckAttribute(pchar, "GreatRecipe.Vortex.Sound")) StopSound(sti(pchar.GreatRecipe.Vortex.Sound), 0);
        pchar.GreatRecipe.Vortex.Sound = Play3DSound("storm_sea_water", stf(pchar.GreatRecipe.Vortex.X), 0.0, stf(pchar.GreatRecipe.Vortex.Z));
    }
    if (GreatRecipe_Phase() == "storm") Whr_RequestWeather(1.0, 1.0, 28.0, 2.0, false, true);
    GreatRecipe_Queue();
}

void GreatRecipe_Capture(string unused)
{
    if (GreatRecipe_Phase() != "assault") return;
    if (CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched))
    {
        DoQuestFunctionDelay("GreatRecipe_Capture", 1.0);
        return;
    }
    GreatRecipe_ClearVortex(true);
    pchar.GreatRecipe.Phase = "captive";
    pchar.GreatRecipe.Days = 0;
    pchar.GreatRecipe.Notice = 0;
    Group_DeleteGroup("GreatRecipeSea");
    if (!DoQuestReloadToLocation("GreatRecipeHold", "goto", "goto3", ""))
    {
        pchar.GreatRecipe.Phase = "returning";
        pchar.GreatRecipe.Won = false;
        GreatRecipe_Return("");
    }
}

void GreatRecipe_LaunchDuel(string unused)
{
    if (GreatRecipe_Phase() != "captive" || pchar.location != "GreatRecipeHold") return;
    if (sti(pchar.GreatRecipe.Days) < 3 && sti(pchar.GreatRecipe.Notice) < 2) return;
    ref beard = GreatRecipe_Actor("GreatRecipeDuelBeard", "pirate_6");
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
    pchar.GreatRecipe.Phase = "storm";
    pchar.GreatRecipe.Won = true;
    pchar.GreatRecipe.Elapsed = 0;
    GreatRecipe_Journal("Борода проиграл. Его секрет — шрапнельное ядро, а не жемчуг. Но море закрылось штормом: дуэльный корабль уже не спасти.", false);
    DoQuestFunctionDelay("GreatRecipe_LaunchStorm", 1.0);
}

void GreatRecipe_LaunchStorm(string unused)
{
    if (GreatRecipe_Phase() != "storm") return;
    if (bAbordageStarted || CheckAttribute(pchar, "pause") || sti(InterfaceStates.Launched))
    {
        DoQuestFunctionDelay("GreatRecipe_LaunchStorm", 1.0);
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
    if (phase == "duel" && index == GetCharacterIndex("GreatRecipeDuelBeard"))
    {
        GreatRecipe_Win();
        return false; // The opposing ship still sinks through normal damage.
    }
    if (index == GetCharacterIndex("GreatRecipeRealBeard") && phase == "lost" &&
        !CheckAttribute(pchar, "GreatRecipe.HuntTaken"))
    {
        pchar.GreatRecipe.HuntTaken = true;
        AddCharacterGoodsSimple(pchar, GOOD_AIRBURST, 80);
        GreatRecipe_Journal("Корвет настоящего Бороды потоплен в грозе. Из воды подняты 80 шрапнельных бомб. Рецепта нет; бермудский магазин по-прежнему закрыт.", true);
        return false;
    }
    if (index != nMainCharacterIndex || !GreatRecipe_Locked()) return false;
    if (phase == "returning") return true;
    pchar.GreatRecipe.Won = phase == "storm";
    pchar.GreatRecipe.Phase = "returning";
    pchar.Ship.HP = 1;
    LAi_SetCurHPMax(pchar);
    DoQuestFunctionDelay("GreatRecipe_Return", 0.1);
    return true;
}

void GreatRecipe_ClaimBoarding(int index)
{
    if (GreatRecipe_Phase() == "duel" && index == GetCharacterIndex("GreatRecipeDuelBeard")) GreatRecipe_Win();
    if (GreatRecipe_Phase() == "lost" && index == GetCharacterIndex("GreatRecipeRealBeard"))
    {
        pchar.GreatRecipe.HuntTaken = true;
        GreatRecipe_Journal("Корабль настоящего Бороды взят абордажем. Шрапнель можно забрать из трюма. Сам капитан рецепта не выдал.", true);
    }
}

bool GreatRecipe_RestoreFleet(bool won)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Escrow.Fleet")) return false;
    aref fleet, ship, ai, target, links;
    makearef(fleet, pchar.GreatRecipe.Escrow.Fleet);
    for (int slot = 0; slot < GetAttributesNum(fleet); slot++)
    {
        aref saved = GetAttributeN(fleet, slot);
        int index = sti(saved.Index);
        ref captain = GetCharacter(index);
        makearef(ai, saved.AI);
        makearef(target, captain.chr_ai);
        GreatRecipe_Copy(target, ai);
        if (won && index != nMainCharacterIndex)
        {
            captain.Ship.Type = SHIP_NOTUSED;
            continue;
        }
        makearef(ship, saved.Ship);
        makearef(target, captain.Ship);
        GreatRecipe_Copy(target, ship);
        SetCharacterShipLocation(captain, "Pirates_town");
    }
    if (!won)
    {
        makearef(links, pchar.GreatRecipe.Escrow.Companions);
        makearef(target, pchar.Fellows.Companions);
        GreatRecipe_Copy(target, links);
    }
    else
    {
        // Release only authored dream actors; their rows make room for the gift.
        for (int actor = 0; actor < 2; actor++)
        {
            string id = "GreatRecipeDreamBeard";
            if (actor == 1) id = "GreatRecipeDuelBeard";
            int actorIndex = GetCharacterIndex(id);
            if (actorIndex >= 0) Characters[actorIndex].Ship.Type = SHIP_NOTUSED;
        }
        int rewardType = FindFirstEmptyShip();
        if (rewardType < 0)
        {
            EmptyAllFantomShips();
            rewardType = FindFirstEmptyShip();
        }
        if (rewardType < 0) return false;
        aref originalRealShip;
        makearef(originalRealShip, pchar.GreatRecipe.Escrow.Fleet.s0.RealShip);
        CopyAttributes(&RealShips[rewardType], originalRealShip);
        RealShips[rewardType].index = rewardType;
        RealShips[rewardType].lock = true;
        DeleteAttribute(&RealShips[rewardType], "StoreShip");
        pchar.Ship.Type = rewardType;
        DeleteAttribute(pchar, "Fellows.Companions");
        pchar.Fellows.Companions.id0 = nMainCharacterIndex;
        // Original generated/unique real-ship row preserves every upgrade.
        pchar.Ship.HP = GetCharacterShipHP(pchar);
        pchar.Ship.SP = GetCharacterShipSP(pchar);
        DeleteAttribute(pchar, "Ship.Masts");
        DeleteAttribute(pchar, "Ship.Sails");
        DeleteAttribute(pchar, "Ship.Blots");
        DeleteAttribute(pchar, "Ship.Cannons.Borts");
        SetCrewQuantityFull(pchar);
        ref rewardShip = GetRealShip(sti(pchar.Ship.Type));
        int rewardLoad = RecalculateCargoLoad(pchar) + GetGoodWeightByType(GOOD_AIRBURST, 240);
        if (sti(rewardShip.Capacity) < rewardLoad) rewardShip.Capacity = rewardLoad;
        AddCharacterGoodsSimple(pchar, GOOD_AIRBURST, 240);
    }
    return true;
}

void GreatRecipe_Return(string unused)
{
    if (GreatRecipe_Phase() != "returning") return;
    GreatRecipe_ClearVortex(true);
    if (!CheckAttribute(pchar, "GreatRecipe.Restored"))
    {
        bool won = sti(pchar.GreatRecipe.Won);
        if (!GreatRecipe_RestoreFleet(won)) return;
        LAi_SetCurHP(pchar, stf(pchar.GreatRecipe.Escrow.Health));
        chrDisableReloadToLocation = sti(pchar.GreatRecipe.Escrow.Reload);
        bDisableFastReload = sti(pchar.GreatRecipe.Escrow.FastReload);
        bDisableCharacterMenu = sti(pchar.GreatRecipe.Escrow.CharacterMenu);
        bQuestDisableMapEnter = sti(pchar.GreatRecipe.Escrow.Map);
        pchar.GreatRecipe.Restored = true;
        pchar.GreatRecipe.Generation = sti(pchar.GreatRecipe.Generation) + 1;
    }
    Group_DeleteGroup("GreatRecipeSea");
    for (int actor = 0; actor < 3; actor++)
    {
        string id = "GreatRecipeBosun";
        if (actor == 1) id = "GreatRecipeDreamBeard";
        if (actor == 2) id = "GreatRecipeDuelBeard";
        int index = GetCharacterIndex(id);
        if (index >= 0) ChangeCharacterAddress(GetCharacter(index), "none", "");
    }
    if (pchar.location == "Pirates_town") GreatRecipe_Returned();
    else if (!DoQuestReloadToLocation("Pirates_town", "reload", "reload4", ""))
        DoQuestFunctionDelay("GreatRecipe_Return", 1.0);
}

void GreatRecipe_Returned()
{
    if (GreatRecipe_Phase() != "returning" || !CheckAttribute(pchar, "GreatRecipe.Restored")) return;
    bool won = sti(pchar.GreatRecipe.Won);
    if (won) pchar.GreatRecipe.Phase = "won";
    else pchar.GreatRecipe.Phase = "lost";
    LAi_SetPlayerType(pchar);
    string body = "Кошмар кончился на Бермудах. Исходная эскадра и вещи целы, но рецепт не добыт. В грозу у Бермуд можно найти настоящую Красную Бороду и взять её шрапнель. Магазинный доступ закрыт.";
    if (won) body = "На Бермудах тебя встречают как легенду. Выдан корабль с характеристиками прежнего флагмана, полностью восстановленный, и 240 шрапнельных бомб. В бермудском магазине теперь есть небольшой возобновляемый запас; за пределами Бермуд шрапнель — контрабанда.";
    GreatRecipe_Journal(body, true);
    Log_Info(body);
    LAi_group_Delete("GreatRecipeHoldGroup");
    DeleteAttribute(pchar, "GreatRecipe.Escrow");
    DeleteAttribute(pchar, "GreatRecipe.Restored");
}

void GreatRecipe_LocationLoaded(aref location)
{
    if (location.id == "GreatRecipeHold" && GreatRecipe_Phase() == "captive") GreatRecipe_CaptiveActors();
    if (GreatRecipe_Phase() == "returning")
    {
        if (location.id == "Pirates_town") GreatRecipe_Returned();
        else DoQuestFunctionDelay("GreatRecipe_Return", 0.1);
    }
}

void GreatRecipe_OnLoad()
{
    if (!GreatRecipe_Locked()) return;
    DeleteAttribute(pchar, "GreatRecipe.Vortex.Sound");
    if (isEntity(&Sea)) Sea.Whirlpool.Clear = 1;
    if (GreatRecipe_Phase() == "returning")
    {
        DoQuestFunctionDelay("GreatRecipe_Return", 0.1);
        return;
    }
    int index = FindLoadedLocation();
    if (index >= 0) GreatRecipe_LocationLoaded(&Locations[index]);
    if (bSeaActive && bSeaLoaded) GreatRecipe_SeaReady();
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
    if (phase != "whirlpool") pchar.GreatRecipe.Elapsed = stf(pchar.GreatRecipe.Elapsed) + 0.5;
    float elapsed = stf(pchar.GreatRecipe.Elapsed);
    if (phase == "whirlpool")
    {
        if (!CheckAttribute(&Sea, "Whirlpool.Age")) return;
        elapsed = stf(Sea.Whirlpool.Age);
        pchar.GreatRecipe.Elapsed = elapsed;
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
        for (int arm = 0; arm < 6; arm++)
        {
            float angle = elapsed * 1.2 + arm * 1.0472;
            float spiralRadius = radius * (0.45 + arm * 0.11);
            string foam = "splash";
            if (arm % 2 == 0) foam = "splash_big";
            CreateParticleSystem(foam, centerX + cos(angle) * spiralRadius,
                0.0, centerZ + sin(angle) * spiralRadius, 0.0, 0.0, 0.0, 5);
        }
        if (elapsed >= stf(pchar.GreatRecipe.Vortex.Duration))
        {
            pchar.GreatRecipe.Phase = "assault";
            GreatRecipe_ClearVortex(false);
            LAi_SetImmortal(pchar, true);
            pchar.GreatRecipe.Elapsed = 0;
            Group_SetTaskAttack("GreatRecipeSea", PLAYER_GROUP);
            GreatRecipe_Journal("Корабль вырвался из водоворота. Красная Борода атакует эскадру и готовится к абордажу.", false);
        }
    }
    if (phase == "assault" && elapsed >= 8.0)
    {
        for (int slot = COMPANION_MAX - 1; slot > 0; slot--)
        {
            int index = GetCompanionIndex(pchar, slot);
            if (index < 0) continue;
            ref captain = GetCharacter(index);
            Ship_Detonate(captain, false, false);
            captain.Ship.HP = 0;
            // Sink the visible ship without ordinary captain/crew-debt death.
            SendMessage(&AISea, "la", AI_MESSAGE_CHARACTER_DEAD, captain);
            RemoveCharacterCompanion(pchar, captain);
            captain.Ship.Type = SHIP_NOTUSED;
        }
        Ship_Detonate(pchar, false, false);
        float assaultHP = GetCharacterShipHP(pchar) * 0.08;
        if (stf(pchar.Ship.HP) > assaultHP) pchar.Ship.HP = assaultHP;
        DoQuestFunctionDelay("GreatRecipe_Capture", 2.0);
        return;
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
            DoQuestFunctionDelay("GreatRecipe_Return", 0.5);
            return;
        }
    }
    PostEvent("GreatRecipePulse", 500, "l", generation);
}

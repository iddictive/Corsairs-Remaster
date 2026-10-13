// The optional earned alliance owns no ordinary quest outcome or ammunition stock.
#event_handler("GreatRecipeAllianceMercy", "GreatRecipe_AllianceMercyTick");
// Script globals use the normal SAVE codec; no quest tree is created for new games.
string GreatRecipe_AlliancePirateLandState = "";
string GreatRecipe_AlliancePirateLowerLandState = "";

string GreatRecipe_AllianceStage()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Stage")) return "";
    return pchar.GreatRecipe.Alliance.Stage;
}

bool GreatRecipe_Allied() { return GreatRecipe_AllianceStage() == "king"; }

void GreatRecipe_AllianceRecordFight(string task, bool won)
{
    if (!won || (task != "mutiny" && task != "mutiny_final")) return;
    if (sti(pchar.GreatRecipe.Rebel)) pchar.GreatRecipe.Alliance.BrokeLoyalty = true;
    else pchar.GreatRecipe.Alliance.(task) = true;
}

bool GreatRecipe_AllianceEligible()
{
    if (CheckAttribute(pchar, "GreatRecipe.EarlyWin")) return false;
    if (!CheckAttribute(pchar, "GreatRecipe.CaptainRespect") || !CheckAttribute(pchar, "GreatRecipe.Helped")) return false;
    if (CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")) return false;
    if (sti(pchar.GreatRecipe.Rebel) || CheckAttribute(pchar, "GreatRecipe.Alliance.BrokeLoyalty")) return false;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.mutiny") || !CheckAttribute(pchar, "GreatRecipe.Alliance.mutiny_final")) return false;
    if (sti(pchar.GreatRecipe.TrainingWins) < 2) return false;
    return CheckAttribute(pchar, "GreatRecipe.Supply.Oil") && CheckAttribute(pchar, "GreatRecipe.Supply.Pin") &&
        CheckAttribute(pchar, "GreatRecipe.Clue.Manifest") && CheckAttribute(pchar, "GreatRecipe.Supply.Fuse");
}

bool GreatRecipe_AlliancePledge()
{
    if (GreatRecipe_Phase() != "captive" || pchar.GreatRecipe.Task != "ready" || !GreatRecipe_AllianceEligible()) return false;
    if (GreatRecipe_AllianceStage() != "") return false;
    pchar.GreatRecipe.Alliance.Stage = "pledged";
    GreatRecipe_Journal("Борода уважает мой клинок и дела на корабле. Я обещал принять его сдачу, если выиграю честную морскую дуэль. Нужно взять шлюп на абордаж и одолеть капитана в каюте; потопленный корабль не оставит нам переговоров.", false);
    return true;
}

void GreatRecipe_AllianceArmCaptain(ref actor, ref captain)
{
    if (GreatRecipe_Phase() != "duel" || GreatRecipe_AllianceStage() != "pledged") return;
    if (!GreatRecipe_AllianceEligible() || captain.id != "GreatRecipeDuelBeard") return;
    if (!CheckAttribute(actor, "CaptanId") || actor.CaptanId != captain.id) return;
    int location = FindLocation(actor.location);
    if (location < 0 || !CheckAttribute(&Locations[location], "CabinType")) return;
    pchar.GreatRecipe.Alliance.CabinCaptain = actor.index;
    LAi_SetCheckMinHP(actor, 40.0, true, "");
    GreatRecipe_AllianceStartMercyTimer();
}

void GreatRecipe_AllianceStartMercyTimer()
{
    pchar.GreatRecipe.Alliance.MercyGeneration = sti(pchar.GreatRecipe.Alliance.MercyGeneration) + 1;
    PostEvent("GreatRecipeAllianceMercy", 500, "ll", sti(pchar.GreatRecipe.Alliance.CabinCaptain), sti(pchar.GreatRecipe.Alliance.MercyGeneration));
}

void GreatRecipe_AllianceMercyTick()
{
    int index = GetEventData();
    int generation = GetEventData();
    if (generation != sti(pchar.GreatRecipe.Alliance.MercyGeneration)) return;
    if (GreatRecipe_Phase() != "duel" || GreatRecipe_AllianceStage() != "pledged") return;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.CabinCaptain") || index != sti(pchar.GreatRecipe.Alliance.CabinCaptain)) return;
    if (index < 0 || index >= MAX_CHARACTERS) return;
    ref actor = GetCharacter(index);
    if (!bAbordageStarted || actor.location != pchar.location || LAi_IsDead(actor)) return;
    if (!CheckAttribute(actor, "CaptanId") || actor.CaptanId != "GreatRecipeDuelBeard") return;
    if (CheckAttribute(pchar, "GreatRecipe.Alliance.Spared")) return;
    if (!CheckAttribute(pchar, "pause")) GreatRecipe_AlliancePulse();
    if (GreatRecipe_AllianceStage() == "pledged" && !CheckAttribute(pchar, "GreatRecipe.Alliance.Spared"))
        PostEvent("GreatRecipeAllianceMercy", 500, "ll", index, generation);
}

void GreatRecipe_AlliancePulse()
{
    if (GreatRecipe_Phase() != "duel" || GreatRecipe_AllianceStage() != "pledged" || !bAbordageStarted) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.CabinCaptain") || CheckAttribute(pchar, "GreatRecipe.Alliance.Spared")) return;
    int index = sti(pchar.GreatRecipe.Alliance.CabinCaptain);
    if (index < 0 || index >= MAX_CHARACTERS) return;
    ref actor = GetCharacter(index);
    if (!CheckAttribute(actor, "CaptanId") || actor.CaptanId != "GreatRecipeDuelBeard" || actor.location != pchar.location) return;
    if (LAi_GetCharacterHP(actor) > 40.0 || LAi_IsDead(actor)) return;
    if (CheckAttribute(pchar, "GreatRecipe.Alliance.MercyDialog") || sti(InterfaceStates.Launched)) return;
    pchar.GreatRecipe.Alliance.MercyDialog = true;
    LAi_RemoveCheckMinHP(actor);
    LAi_SetImmortal(actor, true);
    LAi_SetFightMode(pchar, false);
    LAi_group_SetRelation(LAI_GROUP_BRDENEMY, LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
    LAi_group_SetAlarm(LAI_GROUP_BRDENEMY, LAI_GROUP_PLAYER, 0.0);
    LAi_SetActorType(actor);
    actor.Dialog.Filename = "GreatRecipeAlliance.c";
    actor.Dialog.CurrentNode = "Mercy";
    pchar.GenQuest.QuestAboardCabinDialogIdx = index;
    LAi_ActorDialogNow(actor, pchar, "", -1);
}

bool GreatRecipe_AllianceMercy(ref actor, bool spare)
{
    if (GreatRecipe_Phase() != "duel" || GreatRecipe_AllianceStage() != "pledged" || !bAbordageStarted) return false;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.CabinCaptain") || sti(actor.index) != sti(pchar.GreatRecipe.Alliance.CabinCaptain)) return false;
    if (LAi_IsDead(actor) || LAi_GetCharacterHP(actor) > 40.0) return false;
    if (!spare)
    {
        pchar.GreatRecipe.Alliance.Stage = "declined";
        DeleteAttribute(pchar, "GreatRecipe.Alliance.MercyDialog");
        QuestAboardCabinDialogExitWithBattleNoParam();
        return true;
    }
    pchar.GreatRecipe.Alliance.Spared = true;
    GreatRecipe_Journal("Борода сдался. В обычной форме раздела добычи нужно отпустить капитана с командой: только свободный человек сможет заключить наш мир. Взятие в плен или убийство закончит дело обычной победой.", false);
    // Stock surrender owns the captured event, live captain and boarding cleanup.
    QuestAboardCabinDialogSurrender();
    return true;
}

bool GreatRecipe_AllianceSpared()
{
    return GreatRecipe_AllianceStage() == "pledged" && CheckAttribute(pchar, "GreatRecipe.Alliance.Spared");
}

bool GreatRecipe_AllianceReleaseCaptain(ref actor)
{
    if (GreatRecipe_Phase() != "duel" || actor.id != "GreatRecipeDuelBeard") return false;
    if (!GreatRecipe_AllianceSpared() || !GreatRecipe_AllianceEligible()) return false;
    if (LAi_IsDead(actor) || LAi_GetCharacterHP(actor) <= 0.0) return false;
    pchar.GreatRecipe.Alliance.Released = true;
    return true;
}

bool GreatRecipe_AllianceKeepCaptain(ref actor)
{
    if (actor.id != "GreatRecipeDuelBeard" || LAi_IsDead(actor) || LAi_GetCharacterHP(actor) <= 0.0) return false;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Spared") || !CheckAttribute(pchar, "GreatRecipe.Alliance.Released") ||
        !CheckAttribute(pchar, "GreatRecipe.Alliance.NavalWin")) return false;
    return GreatRecipe_AllianceStage() == "parley" || GreatRecipe_Allied();
}

void GreatRecipe_AllianceNavalWin(bool alive)
{
    if (GreatRecipe_AllianceStage() != "pledged") return;
    if (!alive || !GreatRecipe_AllianceSpared() || !CheckAttribute(pchar, "GreatRecipe.Alliance.Released") || !GreatRecipe_AllianceEligible())
    {
        pchar.GreatRecipe.Alliance.Stage = "declined";
        return;
    }
    pchar.GreatRecipe.Alliance.Stage = "parley";
    pchar.GreatRecipe.Alliance.NavalWin = true;
    GreatRecipe_Journal("Шлюп взят, Борода сложил клинок, и я сохранил ему жизнь. На Бермудах он согласен говорить в резиденции. Но Лаки и Джекмен не простят его по одному моему слову: нужны освобождение пленников, возмещение семьям и договор о прекращении нападений.", false);
}

void GreatRecipe_AllianceReturned()
{
    if (GreatRecipe_AllianceStage() != "parley" && !GreatRecipe_Allied()) return;
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.NavalWin") || !CheckAttribute(pchar, "GreatRecipe.Alliance.Spared") ||
        !CheckAttribute(pchar, "GreatRecipe.Alliance.Released")) return;
    ref beard = GreatRecipe_Actor("GreatRecipeKingBeard", "RedBeard");
    beard.name = "Красная"; beard.lastname = "Борода";
    beard.nation = PIRATE; beard.FaceId = 2006;
    beard.Dialog.Filename = "GreatRecipeAlliance.c";
    if (!CheckAttribute(beard, "Dialog.CurrentNode")) beard.Dialog.CurrentNode = "First time";
    LAi_SetImmortal(beard, true);
    LAi_SetLoginTime(beard, 0.0, 24.0);
    LAi_group_MoveCharacter(beard, "GreatRecipeAllianceGuests");
    DeleteAttribute(beard, "AlwaysEnemy");
    if (!GreatRecipe_Allied())
    {
        LAi_SetStayType(beard);
        ChangeCharacterAddressGroup(beard, "Pirates_townhall", "goto", "goto6");
    }
}

bool GreatRecipe_AllianceFreedomProof()
{
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.PrisonerFreed")) return false;
    int index = GetCharacterIndex("GreatRecipePrisoner");
    if (index < 0) return false;
    ref prisoner = GetCharacter(index);
    return !LAi_IsDead(prisoner) && !CheckAttribute(prisoner, "prisoned") && prisoner.location == "Pirates_town" &&
        prisoner.chr_ai.group == "GreatRecipeAllianceGuests";
}

bool GreatRecipe_AllianceConcessions()
{
    return GreatRecipe_AllianceFreedomProof() && CheckAttribute(pchar, "GreatRecipe.Alliance.RecipeShared");
}

bool GreatRecipe_AllianceDecline()
{
    if (GreatRecipe_AllianceStage() != "parley") return false;
    pchar.GreatRecipe.Alliance.Stage = "declined";
    DeleteAttribute(pchar, "GreatRecipe.Alliance.SeatPending");
    int index = GetCharacterIndex("GreatRecipeKingBeard");
    if (index >= 0) ChangeCharacterAddress(GetCharacter(index), "none", "");
    string body = "Я отказался ручаться за Бороду и принимать пиратский союз. Короны и особых отношений не будет. Обычное поручение Лаки и изготовление шрапнели остаются в силе.";
    if (CheckAttribute(pchar, "GreatRecipe.Alliance.PrisonerFreed")) body = body + " Диего остаётся свободным.";
    if (CheckAttribute(pchar, "GreatRecipe.Alliance.RecipeShared")) body = body + " Уже переданный рецепт Борода не отзывает.";
    GreatRecipe_Journal(body, false);
    return true;
}

string GreatRecipe_AllianceAction(string actor, string action)
{
    if (GreatRecipe_AllianceStage() != "parley") return "Этот разговор уже закончен.";
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Spared") || !CheckAttribute(pchar, "GreatRecipe.Alliance.Released") ||
        !CheckAttribute(pchar, "GreatRecipe.Alliance.NavalWin"))
        return "Сначала нужно выиграть бой и сохранить капитану жизнь.";
    if (actor == "GreatRecipeKingBeard" && action == "terms")
    {
        int prisonerIndex = GetCharacterIndex("GreatRecipePrisoner");
        if (prisonerIndex < 0) return "Сперва найди Диего. Не стану просить берег поверить в свободу человека, которого никто не видел.";
        ref prisoner = GetCharacter(prisonerIndex);
        if (LAi_IsDead(prisoner)) return "Диего погиб. После этого ни Лаки, ни Джекмен не примут наш уговор. Можно прекратить эти переговоры; обычный поиск рецепта остаётся твоим делом.";
        if (!GreatRecipe_AllianceFreedomProof())
        {
            DeleteAttribute(prisoner, "prisoned");
            LAi_SetStayType(prisoner);
            LAi_group_MoveCharacter(prisoner, "GreatRecipeAllianceGuests");
            prisoner.Dialog.Filename = "GreatRecipeAlliance.c";
            prisoner.Dialog.CurrentNode = "Freed";
            ChangeCharacterAddressGroup(prisoner, "Pirates_town", "goto", "goto1");
            if (prisoner.location != "Pirates_town" || CheckAttribute(prisoner, "prisoned")) return "Диего должен выйти на берег. До этого никакого совета и никакой короны.";
            pchar.GreatRecipe.Alliance.PrisonerFreed = true;
        }
        if (!CheckAttribute(pchar, "GreatRecipe.Alliance.RecipeShared"))
        {
            if (GreatRecipe_Phase() == "pearl_search" && GetCharacterItem(pchar, "GreatRecipeRedPearl") == 0) GreatRecipe_GrantLoot(false);
            if (GetCharacterItem(pchar, "GreatRecipeRedPearl") == 0 && !CheckAttribute(pchar, "GreatRecipe.Production.Started")) return "Рецепт нужно передать мастерам, а не обещать с высоты кресла. Вернёмся к разговору, когда жемчужина будет у тебя или у Лаки.";
            pchar.GreatRecipe.Alliance.RecipeShared = true;
        }
        pchar.GreatRecipe.Alliance.Terms = true;
        GreatRecipe_Journal("Борода освободил Диего: тот уже на берегу в пиратском поселении. Красная жемчужина и секрет заряда переданы для бермудской мастерской; отнести жемчужину Лаки в таверну. Но рецепт и победы не прощают убийств: Лаки требует возмещения семьям, а Джекмен — гарантии безопасности берега, прежде чем допустить Бороду к общему флоту.", false);
        return "Диего уже свободен, можешь найти его на берегу. Вот жемчужина: пусть Лаки передаст секрет своим мастерам. Я умею вести бой, а бермудцы умеют строить и торговать. Вместе будет флот, с которым считаются. Только Лаки сперва потребует ответа за своих, а Джекмен — за берег. Поручился за меня — внесёшь пятьдесят тысяч семьям и ты.";
    }
    if (actor == "Pirates_tavernkeeper" && action == "restitution")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Terms") || !GreatRecipe_AllianceConcessions()) return "Пусть Диего сперва выйдет свободным на берег, а рецепт будет передан нашим мастерам. Я не стану менять чужую жизнь на обещание будущей выгоды.";
        if (CheckAttribute(pchar, "GreatRecipe.Alliance.Restitution")) return "Твои пятьдесят тысяч уже распределены между семьями. Я засвидетельствую уговор перед Джекменом.";
        if (sti(pchar.money) < 50000) return "Пятьдесят тысяч от тебя — помощь семьям, за которых ты поручился. Пока денег нет, я не стану свидетельствовать перед Джекменом. Вернёшься с ними — договорим.";
        AddMoneyToCharacter(pchar, -50000);
        pchar.GreatRecipe.Alliance.Restitution = true;
        GreatRecipe_Journal("Диего свободен, рецепт передан, пятьдесят тысяч получил Лаки для бермудских семей. Лаки не простил убийство брата, но готов засвидетельствовать мир ради живых. Теперь говорить с Джекменом: он решит, как подчинить силу Бороды общим интересам, сохранив управление поселением и свои договоры.", false);
        return "Диего я видел. Рецепт ты принёс, деньги семьям переданы. Моего брата это не вернёт, и другом Бороды я не стану. Но свободные люди и работающая мастерская лучше новой войны. Я засвидетельствую мир. Джекмену скажи: за следующий выстрел по нашему берегу отвечать будете оба.";
    }
    if (actor == "Jackman" && action == "treaty")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Terms") || !GreatRecipe_AllianceConcessions() || !CheckAttribute(pchar, "GreatRecipe.Alliance.Restitution")) return "Сначала свобода Диего, рецепт для мастерской и возмещение семьям через Лаки. Флот нам нужен, но ещё одного хозяина, который кормит нас обещаниями, я здесь не потерплю.";
        pchar.GreatRecipe.Alliance.Treaty = true;
        GreatRecipe_Journal("Джекмен предложил Бороде власть морского вождя: его доказанная сила и переданный рецепт усилят общий флот, а не станут новой угрозой Бермудам. Управление поселением, дипломатия и прежние поручения остаются у Джекмена. Я, победивший и отпустивший Бороду, должен стать поручителем мира и обещать не захватывать пиратские поселения; последний разговор — с Бородой.", false);
        return "Силу его флота мы знаем. Теперь у наших мастеров есть его рецепт, а Диего свободен и семьи получили помощь. Лучше поставить такую силу во главе общих кораблей, чем снова ждать красных парусов у своего берега. Пусть зовётся королём пиратов — морским вождём, не хозяином наших кошельков. Управление, договоры и мои поручения остаются у меня. Ты победил его и отпустил: станешь поручителем. И поклянешься не брать наши поселения силой.";
    }
    if (actor == "GreatRecipeKingBeard" && action == "friendship")
    {
        if (!CheckAttribute(pchar, "GreatRecipe.Alliance.Treaty") || !GreatRecipe_AllianceConcessions() || !CheckAttribute(pchar, "GreatRecipe.Alliance.Restitution")) return "Без свободного Диего, переданного рецепта и слова Лаки с Джекменом никакой короны не будет. Дружбу ты заслужил в бою, но мир за других не подпишешь.";
        pchar.GreatRecipe.Alliance.Stage = "king";
        pchar.GreatRecipe.Alliance.Version = 1;
        pchar.GreatRecipe.Alliance.SeatPending = true;
        GreatRecipe_AllianceRefreshLand();
        GreatRecipe_Journal("Красная Борода стал моим близким другом и королём пиратов — вождём общего флота. Он получил эту власть за доказанную морскую силу и переданный рецепт после освобождения Диего, возмещения семьям и соглашения с Джекменом. Джекмен остаётся рядом, управляет поселением и ведёт прежние дела. Под пиратским флагом нас принимают как своих, под иным — нейтрально. Я обещал не захватывать пиратские поселения. Изготовление жемчуга осталось у мастеров: семь дней после сдачи Лаки, партии по двадцать–тридцать бомб и не больше двухсот в запасе.", false);
        return "Лаки меня не простит. Джекмен не даст править берегом. И правильно. За общий флот отвечу я, за поселение и договоры — он; за наше слово — оба с тобой. Ты мог меня убить и отпустил. Такой друг мне дороже короны. Джекмен, подвинься к креслу: спорить будем рядом, не через пушечный порт.";
    }
    return "Сперва договоримся с теми, кто заплатил за нашу войну.";
}

void GreatRecipe_AllianceSeat(string unused)
{
    if (!CheckAttribute(pchar, "GreatRecipe.Alliance.SeatPending")) return;
    DeleteAttribute(pchar, "GreatRecipe.Alliance.SeatPending");
    if (!GreatRecipe_Allied() || pchar.location != "Pirates_townhall") return;
    // The ordinary loader applies both poses before its character login.
    DoQuestReloadToLocation("Pirates_townhall", "reload", "reload1", "");
}

// Called before LoginCharactersInLocation: alter only ordinary residence poses.
void GreatRecipe_AllianceResidence(ref location)
{
    if (location.id != "Pirates_townhall") return;
    GreatRecipe_AllianceReturned();
    if (GreatRecipe_AllianceStage() != "parley" && !GreatRecipe_Allied()) return;
    int kingIndex = GetCharacterIndex("GreatRecipeKingBeard");
    int jackIndex = GetCharacterIndex("Jackman");
    if (kingIndex < 0 || jackIndex < 0) return;
    ref king = GetCharacter(kingIndex);
    ref jack = GetCharacter(jackIndex);
    if (CheckAttribute(pchar, "questTemp.piratesLine") && pchar.questTemp.piratesLine == "KillLoy_toJackman")
    {
        ChangeCharacterAddress(king, "none", "");
        return;
    }
    if (!CheckAttribute(location, "locators.sit.sit1") || !CheckAttribute(location, "locators.goto.goto6")) return;
    bool moved = CheckAttribute(jack, "GreatRecipeAlliancePose");
    bool ordinary = jack.location == location.id && !IsCompanion(jack) &&
        ((jack.chr_ai.type == "huber" && jack.location.group == "sit" && jack.location.locator == "sit1") ||
         (moved && jack.chr_ai.type == "huberstay" && jack.location.group == "goto" && jack.location.locator == "goto6"));
    if (!ordinary)
    {
        // An authored actor, companion or off-site quest owns Jackman's state.
        ChangeCharacterAddress(king, "none", "");
        return;
    }
    for (int i = 0; i < MAX_CHARACTERS; i++)
    {
        ref other = GetCharacter(i);
        if (i == kingIndex || i == jackIndex) continue;
        bool seatTaken = other.location.group == "sit" && other.location.locator == "sit1";
        bool neighborTaken = other.location.group == "goto" && other.location.locator == "goto6";
        if (other.location == location.id && (seatTaken || neighborTaken))
        { ChangeCharacterAddress(king, "none", ""); return; }
    }
    if (!GreatRecipe_Allied()) return;
    jack.GreatRecipeAlliancePose = true;
    ChangeCharacterAddressGroup(jack, location.id, "goto", "goto6");
    LAi_SetHuberStayTypeNoGroup(jack);
    jack.chr_ai.type.main_group = "goto";
    jack.chr_ai.type.main_locator = "goto6";
    jack.chr_ai.type.map_group = "";
    jack.chr_ai.type.map_locator = "";
    jack.chr_ai.type.win_group = "";
    jack.chr_ai.type.win_locator = "";
    LAi_SetHuberType(king);
    LAi_group_MoveCharacter(king, "GreatRecipeAllianceGuests");
    ChangeCharacterAddressGroup(king, location.id, "sit", "sit1");
}

int GreatRecipe_AllianceRelation(int nation, int ordinary)
{
    if (!GreatRecipe_Allied() || nation != PIRATE) return ordinary;
    // The flown flag is pchar.nation; BaseNation/patent is another contract.
    if (sti(pchar.nation) == PIRATE) return RELATION_FRIEND;
    return RELATION_NEUTRAL;
}

bool GreatRecipe_AllianceAmbientActor(ref actor)
{
    if (sti(actor.nation) != PIRATE) return false;
    if (CheckAttribute(actor, "AlwaysEnemy") || CheckAttribute(actor, "QuestShip")) return false;
    if (CheckAttribute(actor, "quest"))
    {
        aref quest; makearef(quest, actor.quest);
        if (GetAttributesNum(quest) > 0 || GetAttributeValue(quest) != "") return false;
    }
    if (CheckAttribute(actor, "SeaAI.Group.Name"))
    {
        string group = actor.SeaAI.Group.Name;
        if (Group_isTaskLock(group)) return false;
    }
    return true;
}

int GreatRecipe_AllianceActorRelation(ref actor, int ordinary)
{
    if (!GreatRecipe_Allied() || !GreatRecipe_AllianceAmbientActor(actor)) return ordinary;
    // Authored actor relations are evaluated by GetRelation before this fallback.
    return GreatRecipe_AllianceRelation(PIRATE, ordinary);
}

// Only Sea_Login's ordinary cache-write owner may admit this marker.
void GreatRecipe_AllianceRegisterPair(int index, int ordinary)
{
    if (index < 0 || index >= MAX_CHARACTERS) return;
    ref actor = GetCharacter(index);
    DeleteAttribute(actor, "GreatRecipeAllianceAmbientPair");
    if (!CheckAttribute(actor, "RealEncounterType") || !GreatRecipe_AllianceAmbientActor(actor)) return;
    actor.GreatRecipeAllianceAmbientPair = true;
    actor.GreatRecipeAllianceAmbientPair.Ordinary = ordinary;
}

// SetCharacterRelation is the authoritative explicit-write boundary.
void GreatRecipe_AllianceRelationWritten(int first, int second)
{
    int index = -1;
    if (first == nMainCharacterIndex) index = second;
    if (second == nMainCharacterIndex) index = first;
    if (index < 0 || index >= MAX_CHARACTERS) return;
    DeleteAttribute(GetCharacter(index), "GreatRecipeAllianceAmbientPair");
}

bool GreatRecipe_AllianceHasPair(ref actor)
{
    return GreatRecipe_Allied() && CheckAttribute(actor, "GreatRecipeAllianceAmbientPair") && GreatRecipe_AllianceAmbientActor(actor);
}

int GreatRecipe_AlliancePairRelation(ref actor)
{
    return GreatRecipe_AllianceActorRelation(actor, sti(actor.GreatRecipeAllianceAmbientPair.Ordinary));
}

string GreatRecipe_AllianceLandRelation(string group1, string group2, string ordinary)
{
    string pirateGroup = "";
    if ((group1 == "PIRATE_CITIZENS" || group1 == "Pirate_citizens") && group2 == LAI_GROUP_PLAYER) pirateGroup = group1;
    if ((group2 == "PIRATE_CITIZENS" || group2 == "Pirate_citizens") && group1 == LAI_GROUP_PLAYER) pirateGroup = group2;
    if (pirateGroup == "") return ordinary;
    if (pirateGroup == "PIRATE_CITIZENS") GreatRecipe_AlliancePirateLandState = ordinary;
    else GreatRecipe_AlliancePirateLowerLandState = ordinary;
    if (!GreatRecipe_Allied() || ordinary == LAI_GROUP_ENEMY) return ordinary;
    if (sti(pchar.nation) == PIRATE) return LAI_GROUP_FRIEND;
    return LAI_GROUP_NEITRAL;
}

void GreatRecipe_AllianceRefreshLand()
{
    if (!GreatRecipe_Allied() || !IsEntity(&LAi_grp_relations)) return;
    // SetRelation resets alarm internally. Never erase a live attack or story.
    if (LAi_group_GetPlayerAlarm() > 0.0 || LAi_group_IsActivePlayerAlarm()) return;
    if (GreatRecipe_AlliancePirateLandState == LAI_GROUP_NEITRAL || GreatRecipe_AlliancePirateLandState == LAI_GROUP_FRIEND)
        LAi_group_SetRelation("PIRATE_CITIZENS", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
    if (GreatRecipe_AlliancePirateLowerLandState == LAI_GROUP_NEITRAL || GreatRecipe_AlliancePirateLowerLandState == LAI_GROUP_FRIEND)
        LAi_group_SetRelation("Pirate_citizens", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
}

bool GreatRecipe_AllianceProtects(string city)
{
    if (!GreatRecipe_Allied()) return false;
    if (city == "Pirates") return true;
    int index = FindColony(city);
    if (index < 0) return false;
    return sti(Colonies[index].nation) == PIRATE;
}

bool GreatRecipe_AllianceRefuseCapture(string city)
{
    if (!GreatRecipe_AllianceProtects(city)) return false;
    Log_Info("Борода — мой друг. Я дал слово не захватывать пиратские поселения.");
    return true;
}

void GreatRecipe_AllianceOnLoad()
{
    // Prior saves without proven choices stay ordinary; never fabricate respect.
    if (GreatRecipe_AllianceStage() == "") return;
    GreatRecipe_AllianceReturned();
    GreatRecipe_AllianceRefreshLand();
    if (GreatRecipe_AllianceStage() == "pledged" && !CheckAttribute(pchar, "GreatRecipe.Alliance.Spared"))
    {
        DeleteAttribute(pchar, "GreatRecipe.Alliance.MercyDialog");
        if (CheckAttribute(pchar, "GreatRecipe.Alliance.CabinCaptain"))
            GreatRecipe_AllianceStartMercyTimer();
    }
}

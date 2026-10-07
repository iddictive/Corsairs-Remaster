
// Emergency main-mast repair. Ship.Masts is the existing persisted damage owner.
object BI_MastRepairQuote;
#event_handler("BI_MastRepairCommit", "BI_MastRepairCommit");

bool BI_MastRepairSafe()
{
    return bSeaActive && !bSeaReloadStarted && !bDeckBoatStarted &&
        !bDisableMapEnter && !bStorm && !bTornado && !LAi_IsDead(pchar) &&
        CheckEnemyCompanionDistance2GoAway(false) && GetCrewQuantity(pchar) > 0;
}

int BI_MastRepairMainNumber(string name)
{
    if (strlen(name) < 5 || strcut(name, 0, 3) != "mast") return 0;
    return sti(strcut(name, 4, strlen(name) - 1));
}

int BI_MastRepairHours(ref captain)
{
    float skill = GetSummonSkillFromName(captain, SKILL_REPAIR) / 100.0;
    if (skill < 0.0) skill = 0.0;
    if (skill > 1.0) skill = 1.0;
    float crew = GetCrewQuantity(captain) / makefloat(GetOptCrewQuantity(captain));
    if (crew < 0.0) crew = 0.0;
    if (crew > 1.0) crew = 1.0;
    int learned = 0;
    int available = 0;
    aref perks, perk;
    makearef(perks, ChrPerksList.list);
    for (int i = 0; i < GetAttributesNum(perks); i++)
    {
        perk = GetAttributeN(perks, i);
        if (!CheckAttribute(perk, "OfficerType") || perk.OfficerType != "carpenter") continue;
        available++;
        if (CheckOfficersPerk(captain, GetAttributeName(perk))) learned++;
    }
    float mastery = 1.0;
    if (available > 0) mastery = learned / makefloat(available);
    float efficiency = skill * (0.5 + 0.5 * mastery) * (0.5 + 0.5 * crew);
    float duration = 72.0 - 64.0 * efficiency;
    int hours = makeint(duration);
    if (hours < duration) hours++;
    return hours;
}

// Same quote owner for menu eligibility, confirmation and commit-time validation.
bool BI_MastRepairPlan(ref captain, ref plan)
{
    DeleteAttribute(plan, "");
    if (!CheckAttribute(captain, "Ship.Masts") || GetOptCrewQuantity(captain) <= 0) return false;
    aref masts, mast;
    makearef(masts, captain.Ship.Masts);
    int total = 0;
    int standing = 0;
    string damage = "";
    for (int i = 0; i < GetAttributesNum(masts); i++)
    {
        mast = GetAttributeN(masts, i);
        int number = BI_MastRepairMainNumber(GetAttributeName(mast));
        damage += GetAttributeName(mast) + ":" + GetAttributeValue(mast) + ";";
        if (number <= 0 || number >= 100) continue;
        total++;
        if (stf(GetAttributeValue(mast)) < 1.0) standing++;
    }
    int limit = total * 3 / 5;
    int count = limit - standing;
    if (count <= 0) return false;
    float perMast = 100.0 * GetHullPPP(captain) / total;
    if (perMast <= 0.0) return false;
    int affordable = makeint(GetRepairGoods(true, captain) / perMast);
    if (count > affordable) count = affordable;
    if (count <= 0) return false;
    plan.captain = captain.index;
    plan.ship = captain.Ship.Type;
    plan.damage = damage;
    plan.count = count;
    plan.total = total;
    plan.standing = standing;
    plan.materials = count * perMast;
    plan.hours = BI_MastRepairHours(captain);
    return true;
}

void BI_MastRepairRefresh(int mainIndex, int selectedIndex)
{
    object plan;
    BattleInterface.Commands.MastRepair.enable = selectedIndex == mainIndex &&
        BI_MastRepairSafe() && BI_MastRepairPlan(pchar, &plan);
}

string BI_MastRepairText()
{
    float credit = 0.0;
    if (CheckAttribute(pchar, "RepairMaterials.forHull")) credit = stf(pchar.RepairMaterials.forHull);
    float needed = stf(BI_MastRepairQuote.materials) - credit;
    if (needed < 0.0) needed = 0.0;
    int boards = makeint(needed);
    if (boards < needed) boards++;
    int hours = sti(BI_MastRepairQuote.hours);
    string time = hours + " ч.";
    if (hours == 72) time = "3 дня";
    return "Поднять мачты: " + BI_MastRepairQuote.count + " (будет " +
        (sti(BI_MastRepairQuote.standing) + sti(BI_MastRepairQuote.count)) + " из " +
        BI_MastRepairQuote.total + ")." + NewStr() + "Доски: " + boards + ". Время: " + time +
        NewStr() + "Время будет пропущено. В море доступно до 60% мачт.";
}

void BI_MastRepairCancel()
{
    DeleteAttribute(&BI_MastRepairQuote, "");
}

void BI_MastRepairOpen()
{
    if (sti(InterfaceStates.Launched)) return;
    BI_MastRepairCancel();
    if (!BI_MastRepairSafe() || !BI_MastRepairPlan(pchar, &BI_MastRepairQuote)) return;
    LaunchLeaveBattleScreen();
    if (!sti(InterfaceStates.Launched)) BI_MastRepairCancel();
}

void BI_MastRepairAccept()
{
    if (!CheckAttribute(&BI_MastRepairQuote, "captain")) return;
    BI_MastRepairQuote.accepted = true;
    PostEvent("BI_MastRepairCommit", 100);
}

void BI_MastRepairRestore(ref captain, int count)
{
    object repaired;
    aref masts, mast, part;
    makearef(masts, captain.Ship.Masts);
    for (int i = 0; i < GetAttributesNum(masts) && count > 0; i++)
    {
        mast = GetAttributeN(masts, i);
        int number = BI_MastRepairMainNumber(GetAttributeName(mast));
        if (number <= 0 || number >= 100 || stf(GetAttributeValue(mast)) < 1.0) continue;
        for (int j = 0; j < GetAttributesNum(masts); j++)
        {
            part = GetAttributeN(masts, j);
            int child = BI_MastRepairMainNumber(GetAttributeName(part));
            if (child == number || child / 100 == number)
            {
                string name = GetAttributeName(part);
                masts.(name) = 0.0;
                repaired.(name) = true;
            }
        }
        count--;
    }
    // Rehang only the rig belonging to the raised masts; other sail damage stays.
    aref sails, yard, sail;
    makearef(sails, captain.Ship.Sails);
    for (i = 0; i < GetAttributesNum(sails); i++)
    {
        yard = GetAttributeN(sails, i);
        for (j = 0; j < GetAttributesNum(yard); j++)
        {
            sail = GetAttributeN(yard, j);
            if (!CheckAttribute(sail, "mastFall")) continue;
            if (!CheckAttribute(&repaired, sail.mastFall)) continue;
            sail.dmg = 0.0;
            sail.hc = 0;
            sail.hd = 0;
            DeleteAttribute(sail, "mastFall");
        }
    }
    captain.Ship.SP = CalculateShipSP(captain);
}

void BI_MastRepairCommit()
{
    if (!CheckAttribute(&BI_MastRepairQuote, "accepted")) return;
    object quoted;
    CopyAttributes(&quoted, &BI_MastRepairQuote);
    BI_MastRepairCancel();
    object current;
    if (sti(InterfaceStates.Launched) || !BI_MastRepairSafe() ||
        !BI_MastRepairPlan(pchar, &current)) return;
    if (quoted.captain != current.captain || quoted.ship != current.ship ||
        quoted.damage != current.damage || quoted.count != current.count ||
        stf(quoted.materials) != stf(current.materials) || quoted.hours != current.hours)
    {
        Log_SetStringToLog("Условия ремонта изменились. Откройте ремонт мачт ещё раз.");
        return;
    }
    object before;
    aref ship;
    makearef(ship, pchar.Ship);
    CopyAttributes(&before, ship);
    BI_MastRepairRestore(pchar, sti(quoted.count));
    int restored = SendMessage(pchar, "l", MSG_SHIP_REPAIR_MASTS);
    if (restored != sti(quoted.count))
    {
        aref saved, target;
        makearef(saved, before.Masts);
        makearef(target, pchar.Ship.Masts);
        CopyAttributes(target, saved);
        DeleteAttribute(pchar, "Ship.Sails");
        if (CheckAttribute(&before, "Sails"))
        {
            makearef(saved, before.Sails);
            makearef(target, pchar.Ship.Sails);
            CopyAttributes(target, saved);
        }
        pchar.Ship.SP = before.SP;
        Log_SetStringToLog("Не удалось поднять мачты. Доски и время сохранены.");
        return;
    }
    RemoveRepairGoods(true, pchar, stf(quoted.materials));
    WaitDate("", 0, 0, 0, sti(quoted.hours), 0);
    Sea_ReconcileCabinSleepEnvironment();
    RefreshBattleInterface();
    Log_SetStringToLog("Мачты подняты: " + restored + ". Ремонт занял " + quoted.hours + " ч.");
}

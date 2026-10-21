"""Compose the native captain journal into already-patched gameplay outputs."""

from __future__ import annotations

import hashlib


PATHS = (
    "PROGRAM/quests/quests.c",
    "PROGRAM/scripts/custody.c",
    "PROGRAM/scripts/Crew.c",
    "PROGRAM/sea_ai/AIShip.c",
    "PROGRAM/QuestBook/QuestBook_New.txt",
)

BASE_SHA256 = {
    "PROGRAM/quests/quests.c": "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
    "PROGRAM/scripts/custody.c": "26dffe1eaefa640f0f489db839bcc8f4ee486a9e35216771ddcf5693a802a558",
    "PROGRAM/scripts/Crew.c": "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
    "PROGRAM/sea_ai/AIShip.c": "96b1eaef43f818f1b5d5e25f89c2fae3ef74286d7d37102d65c3ea4057cfe187",
    "PROGRAM/QuestBook/QuestBook_New.txt": "381bd67437fcabb49abca3df8f5253b3113dec685d628d01933bf5b0610cc349",
}

QUEST_HELPER = r'''
// Codex: one stable, silent journal row per systemic consequence owner.
void Journal_SetSystemEntry(string idQuest, string logName, string body)
{
	if (!CheckAttribute(pchar, "QuestInfo." + idQuest)) SetQuestHeaderEx(idQuest, logName);
	pchar.QuestInfo.(idQuest).Complete = false;
	pchar.QuestInfo.(idQuest).LogName = logName;
	pchar.QuestInfo.(idQuest).Text.l0 = "@" + logName + "@" + GetQuestBookData() + "@1";
	DeleteAttribute(pchar, "QuestInfo." + idQuest + ".Text.l0.UserData");
	AddQuestUserData(idQuest, "body", body);
}
'''

CUSTODY_HELPERS = r'''
string Custody_JournalItemName(string itemId)
{
	int itemIndex = GetItemIndex(itemId);
	if (itemIndex < 0) return itemId;
	return GetConvertStr(Items[itemIndex].name, "ItemsDescribe.txt");
}

string Custody_JournalEscrow()
{
	string result = "";
	if (CheckAttribute(pchar, "Custody.Escrow.Blade")) result += Custody_JournalItemName(pchar.Custody.Escrow.Blade) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Gun")) result += Custody_JournalItemName(pchar.Custody.Escrow.Gun) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Cirass")) result += Custody_JournalItemName(pchar.Custody.Escrow.Cirass) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Spyglass")) result += Custody_JournalItemName(pchar.Custody.Escrow.Spyglass) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Bullet")) result += "пули: " + pchar.Custody.Escrow.Bullet + "; ";
	if (CheckAttribute(pchar, "Custody.Escrow.GunPowder")) result += "порох: " + pchar.Custody.Escrow.GunPowder + "; ";
	if (result == "") return "ничего";
	return result;
}

void Custody_UpdateJournal()
{
	if (!Custody_IsActive()) return;
	string city = XI_ConvertString("Colony" + pchar.Custody.City);
	string body = "Под стражей в " + city + ". Осталось: " + FindRussianDaysString(sti(pchar.Custody.DaysRemaining)) + ". Штраф уплачен: " + FindRussianMoneyString(sti(pchar.Custody.FinePaid)) + ". Эскадра находится в порту " + city + ". Изъято на хранение: " + Custody_JournalEscrow();
	if (CheckAttribute(pchar, "Custody.LastResult")) body += NewStr() + "Последний день: " + pchar.Custody.LastResult;
	Journal_SetSystemEntry("SystemCustody", "SystemCustody", body);
}

void Custody_CloseJournal(string destination, int standing)
{
	string outcome = "срок отбыт";
	if (standing <= 20) outcome = "освобождение с бесчестьем";
	string body = "Заключение завершено: " + outcome + ". Итоговое положение среди заключённых: " + standing + " из 100. Штраф был уплачен: " + FindRussianMoneyString(sti(pchar.Custody.FinePaid)) + ". Вещи возвращены. Эскадра переведена в " + destination + ".";
	Journal_SetSystemEntry("SystemCustody", "SystemCustody", body);
}
'''

DEBT_HELPER = r'''
void CrewDebt_UpdateJournal()
{
	if (!CheckAttribute(pchar, "CrewDebt.Version")) return;
	string body = "";
	int openCount = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || sti(debtEntry.Amount) <= 0) continue;
		openCount++;
		string hiring = "кредитор остаётся в команде";
		if (debtEntry.Status == "dismissed") hiring = "кредитор уволен";
		body += CrewDebt_GetEntryCreditor(debtEntry) + ": " + FindRussianMoneyString(sti(debtEntry.Amount)) + ", период " + debtEntry.Period + ", " + hiring + "." + NewStr();
	}
	if (openCount == 0) body = "Долги перед командой закрыты.";
	int months = 0;
	if (CheckAttribute(pchar, "CrewDebt.MissedMonths")) months = sti(pchar.CrewDebt.MissedMonths);
	body += "Просроченных месяцев: " + months + ". ";
	bool blocked = (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0) || (months >= 3 && CheckAttribute(pchar, "CrewPayment") && sti(pchar.CrewPayment) > 0);
	if (blocked) body += "Найм матросов закрыт.";
	else body += "Найм матросов доступен.";
	Journal_SetSystemEntry("SystemCrewDebt", "SystemCrewDebt", body);
}
'''

QUEST_DATA = r'''

#QUEST SystemCustody
Арест и заключение
#TEXT 1
@<body>

#QUEST SystemCrewDebt
Долги перед командой
#TEXT 1
@<body>

#QUEST SystemPublicCrime
Известные властям преступления
#TEXT 1
@<body>
'''


def _replace_once(data: bytes, old: str, new: str, path: str) -> bytes:
    old_b = old.replace("\n", "\r\n").encode()
    new_b = new.replace("\n", "\r\n").encode()
    if data.count(old_b) != 1:
        raise ValueError(f"{path}: expected one anchor, found {data.count(old_b)}")
    return data.replace(old_b, new_b, 1)


def transform_outputs(outputs: dict[str, bytes]) -> dict[str, bytes]:
    """Return deterministic journal-enabled copies of the five native outputs."""
    missing = [path for path in PATHS if path not in outputs]
    if missing:
        raise KeyError(f"missing journal inputs: {', '.join(missing)}")
    result = dict(outputs)
    for path in PATHS:
        digest = hashlib.sha256(result[path]).hexdigest()
        if digest != BASE_SHA256[path]:
            raise ValueError(f"{path}: unsupported base sha256 {digest}")

    path = "PROGRAM/quests/quests.c"
    result[path] = _replace_once(result[path], "void AddQuestUserDataForTitle(string idQuest, string strID, string strData)", QUEST_HELPER + "\nvoid AddQuestUserDataForTitle(string idQuest, string strID, string strData)", path)

    path = "PROGRAM/scripts/custody.c"
    result[path] = _replace_once(result[path], "bool Custody_IsCurrentJail(aref loc)", CUSTODY_HELPERS + "\nbool Custody_IsCurrentJail(aref loc)", path)
    result[path] = _replace_once(result[path], "\tCustody_TakeEscrow();\n\tCustody_ApplyLocks();", "\tCustody_TakeEscrow();\n\tCustody_UpdateJournal();\n\tCustody_ApplyLocks();", path)
    result[path] = _replace_once(result[path], "\tpchar.Custody.DaysRemaining = sti(pchar.Custody.DaysRemaining) - 1;\n\tWaitDate", "\tpchar.Custody.DaysRemaining = sti(pchar.Custody.DaysRemaining) - 1;\n\tCustody_UpdateJournal();\n\tWaitDate", path)
    result[path] = _replace_once(result[path], "\tCustody_ApplyLocks();\n\tint nation = sti(pchar.Custody.Nation);", "\tCustody_ApplyLocks();\n\tCustody_UpdateJournal();\n\tint nation = sti(pchar.Custody.Nation);", path)
    result[path] = _replace_once(result[path], "\tpchar.CustodyHistory.LastFine = pchar.Custody.FinePaid;", "\tpchar.CustodyHistory.LastFine = pchar.Custody.FinePaid;\n\tCustody_CloseJournal(destination, standing);", path)
    result[path] = _replace_once(result[path], "\tif (hunter > 0) ChangeCharacterHunterScore(pchar, NationShortName(nation) + \"hunter\", -hunter);", "\tif (hunter > 0) ChangeCharacterHunterScore(pchar, NationShortName(nation) + \"hunter\", -hunter);\n\tJournal_SetSystemEntry(\"SystemPublicCrime\", \"SystemPublicCrime\", \"После отбытия срока розыск державы \" + XI_ConvertString(GetNationNameByType(nation)) + \" снят.\");", path)

    path = "PROGRAM/scripts/Crew.c"
    result[path] = _replace_once(result[path], "void CrewDebt_RebuildProjection()", DEBT_HELPER + "\nvoid CrewDebt_RebuildProjection()", path)
    result[path] = _replace_once(result[path], "\t\tpchar.Partition.MonthPart.Hero = currentHero;\n\t}\n}\n\nvoid CrewDebt_Ensure", "\t\tpchar.Partition.MonthPart.Hero = currentHero;\n\t}\n\tCrewDebt_UpdateJournal();\n}\n\nvoid CrewDebt_Ensure", path)

    path = "PROGRAM/sea_ai/AIShip.c"
    result[path] = _replace_once(result[path], "\tint newHunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + \"hunter\", hunter);\n\tCrime_ApplyPatentConsequence(nation);", "\tint newHunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + \"hunter\", hunter);\n\tCrime_ApplyPatentConsequence(nation);\n\tstring crimeBody = \"Власти державы \" + XI_ConvertString(GetNationNameByType(nation)) + \" установили личность капитана. Репутация: -\" + repLoss + \"; розыск: +\" + hunter + \" (теперь \" + newHunter + \").\";\n\tif (newHunter >= 10) crimeBody += \" Отношения с державой стали враждебными.\";\n\tJournal_SetSystemEntry(\"SystemPublicCrime\", \"SystemPublicCrime\", crimeBody);", path)

    path = "PROGRAM/QuestBook/QuestBook_New.txt"
    result[path] += QUEST_DATA.replace("\n", "\r\n").encode()
    return result


UPDATED_SHA256 = {
    "PROGRAM/quests/quests.c": "1c1d67ff5496a857f8011cc652d4b4853f8a7d3f4c59d589f24af42a18396317",
    "PROGRAM/scripts/custody.c": "3bf751a37c1eb0487afc79908e27ddf77ab4be712da7266f0ae2ef579e8a16f8",
    "PROGRAM/scripts/Crew.c": "31bfc02822166b3daae6092385004a1f1e55d2bb1a59c9c7e7817f1041b5448e",
    "PROGRAM/sea_ai/AIShip.c": "9ba052d5f1cd240b5adf3bacd9eda0314572ff404c66efef25b3cd35ca66d815",
    "PROGRAM/QuestBook/QuestBook_New.txt": "0b44d01978f7bc69d1fecb95138a1e66bcf194295b6ae18915cd3272a860f3cf",
}

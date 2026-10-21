#!/usr/bin/env python3
"""Install reversible officer auto-equipment and cabin-chest resupply."""

from __future__ import annotations

import argparse
import sys

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, transform


FOOD_OLD = '''// boal food for crew 20.01.2004 -->
void DailyEatCrewUpdate()   // сюда пихаю всё что в 1 день
{
    ref mainCh = GetMainCharacter();'''

FOOD_NEW = '''// Автоснабжение офицеров из личного рундука в каюте.
// Отсутствующий флаг означает выключенный режим, поэтому старые сохранения совместимы.
bool OfficerSupply_IsEnabled(ref officer)
{
    if (!CheckAttribute(officer, "OfficerSupply.Enabled")) return false;
    return sti(officer.OfficerSupply.Enabled) == true;
}

void OfficerSupply_EnsureTargets(ref officer)
{
    if (CheckAttribute(officer, "OfficerSupply.TargetBullet")) return;

    int ammo = 8 + rand(6);
    officer.OfficerSupply.TargetBullet = ammo;
    officer.OfficerSupply.TargetGunPowder = ammo;
    officer.OfficerSupply.TargetPotion1 = 2 + rand(2);
    officer.OfficerSupply.TargetPotion2 = 1 + rand(1);
    officer.OfficerSupply.TargetPotion3 = 1;
    officer.OfficerSupply.TargetPotion4 = rand(1);
}

bool OfficerSupply_CanUseGun(ref officer, ref item)
{
    bool musketeer = false;
    if (CheckAttribute(officer, "CanTakeMushket"))
    {
        if (CheckAttribute(officer, "IsMushketer")) musketeer = true;
    }

    bool musket = isMushket(item.id);
    if (musketeer != musket) return false;
    if (item.id == "mushket2x2" && officer.id != "OffMushketer") return false;
    if (item.id == "mushket_H2" && !IsCharacterPerkOn(officer, "GunProfessional")) return false;
    if (!CheckAttribute(item, "chargeQ")) return false;

    int charges = sti(item.chargeQ);
    if (charges < 2) return true;
    if (charges < 4) return IsCharacterPerkOn(officer, "Gunman");
    return IsCharacterPerkOn(officer, "GunProfessional");
}

float OfficerSupply_ItemScore(ref officer, ref item, string groupID)
{
    float result = -1.0;
    float skill;
    float weight;
    float speed;
    float accuracy;

    if (!CheckAttribute(item, "price")) return result;
    if (sti(item.price) <= 0) return result;
    if (CheckAttribute(item, "quest")) return result;

    if (groupID == BLADE_ITEM_TYPE)
    {
        if (CheckAttribute(officer, "isMusketer")) return result;
        if (CheckAttribute(officer, "model.animation"))
        {
            if (HasSubStr(officer.model.animation, "mushketer")) return result;
        }
        if (!CheckAttribute(item, "FencingType")) return result;
        if (!CheckAttribute(item, "dmg_min")) return result;
        if (!CheckAttribute(item, "dmg_max")) return result;
        if (!CheckAttribute(item, "Weight")) return result;
        weight = stf(item.Weight);
        if (weight <= 0.0) return result;
        skill = GetCharacterSkill(officer, item.FencingType);
        return (stf(item.dmg_min) * 3.0 + stf(item.dmg_max) * skill / SKILL_MAX) / GetEnergyBladeDrain(weight);
    }

    if (groupID == GUN_ITEM_TYPE)
    {
        if (!OfficerSupply_CanUseGun(officer, item)) return result;
        if (!CheckAttribute(item, "dmg_min")) return result;
        if (!CheckAttribute(item, "dmg_max")) return result;
        accuracy = 50.0;
        if (CheckAttribute(item, "accuracy")) accuracy = 50.0 + stf(item.accuracy);
        speed = 1.0;
        if (CheckAttribute(item, "chargespeed")) speed = stf(item.chargespeed);
        if (speed <= 0.0) speed = 1.0;
        return ((stf(item.dmg_min) + stf(item.dmg_max)) * accuracy / speed) + stf(item.chargeQ) * 25.0;
    }

    if (groupID == CIRASS_ITEM_TYPE)
    {
        if (!IsCharacterPerkOn(officer, "Ciras")) return result;
        if (!CheckAttribute(item, "CirassLevel")) return result;
        if (stf(item.CirassLevel) <= 0.0) return result;
        weight = 0.0;
        if (CheckAttribute(item, "Weight")) weight = stf(item.Weight);
        return stf(item.CirassLevel) * 1000.0 - weight;
    }
    return result;
}

string OfficerSupply_FindBestItem(ref officer, aref chest, string groupID)
{
    string bestID = "";
    float bestScore = -1.0;
    float score;
    ref item;

    for (int i = 0; i < TOTAL_ITEMS; i++)
    {
        item = &Items[i];
        if (!CheckAttribute(item, "id")) continue;
        if (!CheckAttribute(item, "groupID")) continue;
        if (item.groupID != groupID) continue;
        if (GetCharacterItem(officer, item.id) < 1 && GetCharacterItem(chest, item.id) < 1) continue;

        score = OfficerSupply_ItemScore(officer, item, groupID);
        if (score <= bestScore) continue;
        if (GetCharacterItem(officer, item.id) < 1)
        {
            if (CheckAttribute(item, "Weight"))
            {
                if (GetItemsWeight(officer) + stf(item.Weight) > GetMaxItemsWeight(officer)) continue;
            }
        }
        bestScore = score;
        bestID = item.id;
    }
    return bestID;
}

int OfficerSupply_FillEquipment(ref officer, aref chest, string groupID)
{
    string equipped = GetCharacterEquipByGroup(officer, groupID);
    if (equipped != "" && equipped != "unarmed")
    {
        if (GetCharacterItem(officer, equipped) > 0) return 0;
    }
    if (groupID == BLADE_ITEM_TYPE && CheckAttribute(officer, "DontChangeBlade")) return 0;
    if (groupID == GUN_ITEM_TYPE && CheckAttribute(officer, "DontChangeGun")) return 0;

    string itemID = OfficerSupply_FindBestItem(officer, chest, groupID);
    if (itemID == "") return 0;

    int moved = 0;
    if (GetCharacterItem(officer, itemID) < 1)
    {
        if (!TakeNItems(officer, itemID, 1)) return 0;
        TakeNItems(chest, itemID, -1);
        moved = 1;
    }
    EquipCharacterByItem(officer, itemID);
    return moved;
}

int OfficerSupply_FillItem(ref officer, aref chest, string itemID, int target)
{
    int need = target - GetCharacterItem(officer, itemID);
    int available = GetCharacterItem(chest, itemID);
    int moved = 0;
    aref item;

    if (need < 1 || available < 1) return 0;
    if (need > available) need = available;
    if (Items_FindItem(itemID, &item) < 0) return 0;

    while (need > 0)
    {
        if (CheckAttribute(item, "Weight"))
        {
            if (GetItemsWeight(officer) + stf(item.Weight) > GetMaxItemsWeight(officer)) break;
        }
        if (!TakeNItems(officer, itemID, 1)) break;
        TakeNItems(chest, itemID, -1);
        moved++;
        need--;
    }
    return moved;
}

int OfficerSupply_RefillOfficer(ref officer, aref chest)
{
    if (!OfficerSupply_IsEnabled(officer)) return 0;
    if (!GetRemovable(officer)) return 0;
    if (CheckAttribute(officer, "prisoned"))
    {
        if (sti(officer.prisoned) == true) return 0;
    }

    OfficerSupply_EnsureTargets(officer);
    int moved = OfficerSupply_FillEquipment(officer, chest, BLADE_ITEM_TYPE);
    moved += OfficerSupply_FillEquipment(officer, chest, GUN_ITEM_TYPE);

    string gunID = GetCharacterEquipByGroup(officer, GUN_ITEM_TYPE);
    if (gunID != "")
    {
        moved += OfficerSupply_FillItem(officer, chest, "bullet", sti(officer.OfficerSupply.TargetBullet));
        moved += OfficerSupply_FillItem(officer, chest, "GunPowder", sti(officer.OfficerSupply.TargetGunPowder));
    }
    moved += OfficerSupply_FillItem(officer, chest, "potion1", sti(officer.OfficerSupply.TargetPotion1));
    moved += OfficerSupply_FillItem(officer, chest, "potion2", sti(officer.OfficerSupply.TargetPotion2));
    moved += OfficerSupply_FillItem(officer, chest, "potion3", sti(officer.OfficerSupply.TargetPotion3));
    moved += OfficerSupply_FillItem(officer, chest, "potion4", sti(officer.OfficerSupply.TargetPotion4));
    moved += OfficerSupply_FillEquipment(officer, chest, CIRASS_ITEM_TYPE);
    return moved;
}

bool OfficerSupply_CanRefillNow()
{
    if (dialogRun) return false;
    if (LAi_grp_alarmactive) return false;
    if (LAi_IsFightMode(pchar)) return false;
    if (Get_My_Cabin() == "") return false;
    if (pchar.location != Get_My_Cabin()) return false;
    return true;
}

int OfficerSupply_RefillAllFromChest(aref chest, bool showLog)
{
    if (!OfficerSupply_CanRefillNow()) return 0;
    int moved = 0;
    int idx;
    ref officer;

    for (int i = 0; i < GetPassengersQuantity(pchar); i++)
    {
        idx = GetPassenger(pchar, i);
        if (idx < 1) continue;
        officer = &Characters[idx];
        moved += OfficerSupply_RefillOfficer(officer, chest);
    }
    if (showLog && moved > 0)
    {
        Log_Info("Автоснабжение офицеров: передано предметов — " + moved + ".");
    }
    return moved;
}

int OfficerSupply_RefillFromCabin(bool showLog)
{
    if (!OfficerSupply_CanRefillNow()) return 0;
    int locationIndex = FindLocation(Get_My_Cabin());
    if (locationIndex < 0) return 0;
    if (!CheckAttribute(&Locations[locationIndex], "box1.items")) return 0;

    aref chest;
    makearef(chest, Locations[locationIndex].box1);
    return OfficerSupply_RefillAllFromChest(chest, showLog);
}

void OfficerSupply_AfterDialog()
{
    if (!CheckAttribute(pchar, "questTemp.OfficerSupplyIdx")) return;
    int idx = sti(pchar.questTemp.OfficerSupplyIdx);
    DeleteAttribute(pchar, "questTemp.OfficerSupplyIdx");
    if (!OfficerSupply_CanRefillNow()) return;

    int locationIndex = FindLocation(Get_My_Cabin());
    if (locationIndex < 0) return;
    if (!CheckAttribute(&Locations[locationIndex], "box1.items")) return;

    aref chest;
    makearef(chest, Locations[locationIndex].box1);
    int moved = OfficerSupply_RefillOfficer(GetCharacter(idx), chest);
    if (moved > 0)
    {
        Log_Info("Автоснабжение офицера: передано предметов — " + moved + ".");
    }
}

// boal food for crew 20.01.2004 -->
void DailyEatCrewUpdate()   // сюда пихаю всё что в 1 день
{
    ref mainCh = GetMainCharacter();
    OfficerSupply_RefillFromCabin(false);'''


ITEMSBOX_OLD = '''\t\tmakearef(arChest, itemsRef);
\t\tbBoxUsed = true;
\t}'''

ITEMSBOX_NEW = '''\t\tmakearef(arChest, itemsRef);
\t\tbBoxUsed = true;
\t\tif (pchar.location == Get_My_Cabin() && CheckAttribute(pchar, "boxname"))
\t\t{
\t\t\tif (pchar.boxname == "box1")
\t\t\t{
\t\t\t\tOfficerSupply_RefillAllFromChest(arChest, true);
\t\t\t}
\t\t}
\t}'''


DIALOG_LINK_OLD = '''\t\t\tif (isOfficer(Npchar)) // проверка для кампуса, где слотовые офы
\t\t\t{
\t\t\t\tLink.l2 = "Слушай мой приказ!";
\t\t\t\tLink.l2.go = "stay_follow";
\t\t\t}

           \t\t// boal отчёт о корабле'''

DIALOG_LINK_NEW = '''\t\t\tif (isOfficer(Npchar)) // проверка для кампуса, где слотовые офы
\t\t\t{
\t\t\t\tLink.l2 = "Слушай мой приказ!";
\t\t\t\tLink.l2.go = "stay_follow";
\t\t\t}

\t\t\tif (GetRemovable(NPChar))
\t\t\t{
\t\t\t\tbool supplyAllowed = true;
\t\t\t\tif (CheckAttribute(NPChar, "prisoned"))
\t\t\t\t{
\t\t\t\t\tif (sti(NPChar.prisoned) == true) supplyAllowed = false;
\t\t\t\t}
\t\t\t\tif (supplyAllowed && OfficerSupply_IsEnabled(NPChar))
\t\t\t\t{
\t\t\t\t\tLink.l3 = "Больше не бери снаряжение из корабельного сундука.";
\t\t\t\t\tLink.l3.go = "OfficerSupply_Disable";
\t\t\t\t}
\t\t\t\tif (supplyAllowed && !OfficerSupply_IsEnabled(NPChar))
\t\t\t\t{
\t\t\t\t\tLink.l3 = "Бери нужное снаряжение из корабельного сундука и пополняй припасы.";
\t\t\t\t\tLink.l3.go = "OfficerSupply_Enable";
\t\t\t\t}
\t\t\t}

           \t\t// boal отчёт о корабле'''


DIALOG_CASE_OLD = '''\t\tcase "TransferGoodsEnable":
\t\t\tif(sti(PChar.Ship.Type) == SHIP_NOTUSED)'''

DIALOG_CASE_NEW = '''\t\tcase "OfficerSupply_Enable":
\t\t\tNPChar.OfficerSupply.Enabled = true;
\t\t\tOfficerSupply_EnsureTargets(NPChar);
\t\t\tpchar.questTemp.OfficerSupplyIdx = NPChar.index;
\t\t\tAddDialogExitQuestFunction("OfficerSupply_AfterDialog");
\t\t\tdialog.text = "Есть, капитан. Подберу недостающее оружие и броню, а патроны и лекарства буду пополнять из рундука в каюте.";
\t\t\tLink.l1 = "Вольно.";
\t\t\tLink.l1.go = "Exit";
\t\tbreak;

\t\tcase "OfficerSupply_Disable":
\t\t\tDeleteAttribute(NPChar, "OfficerSupply.Enabled");
\t\t\tdialog.text = "Понял. То, что уже выдано, оставлю при себе, но больше ничего из рундука брать не буду.";
\t\t\tLink.l1 = "Вольно.";
\t\t\tLink.l1.go = "Exit";
\t\tbreak;

\t\tcase "TransferGoodsEnable":
\t\t\tif(sti(PChar.Ship.Type) == SHIP_NOTUSED)'''


PATCH = PatchSet(
    TARGET_ROOT / ".codex-officer-auto-supply" / "20260914-v1",
    (
        FilePatch(
            "PROGRAM/scripts/food.c",
            "f0182ada3df1e7cff1a1ac151c752165754d18a66071fa922ff9af6beb7ee61b",
            "e4278e7829d4d1f8a004e6c46dd3f6c0f7becebf320c697122f9c967c2c36d41",
            ((FOOD_OLD, FOOD_NEW),),
        ),
        FilePatch(
            "PROGRAM/interface/itemsbox.c",
            "e43de5980a70c5065fd58d35a9108965ee30acf2d85743fc90cc409448a15e12",
            "5b35060ae02e96089d9e20991897c2309c838eb2926ef4f67be0a533fabfc029",
            ((ITEMSBOX_OLD, ITEMSBOX_NEW),),
        ),
        FilePatch(
            "PROGRAM/dialogs/russian/Enc_Officer_dialog.c",
            "c93d62854004bf888a3716c90ccb9150be74577ad5bae067442b7e940c263e99",
            "41a35682f6d8d6bd8720095b7b4dffa6d6f2bd55bfda0dc79a21a41d7c919c27",
            ((DIALOG_LINK_OLD, DIALOG_LINK_NEW), (DIALOG_CASE_OLD, DIALOG_CASE_NEW)),
        ),
    ),
)


def verify() -> int:
    state, rows = PATCH.overall_state(TARGET_ROOT)
    if state not in {"original", "patched"}:
        PATCH.print_status(TARGET_ROOT)
        return 1

    candidates: dict[str, str] = {}
    for spec, item_state in rows:
        source = (TARGET_ROOT / spec.relative_path).read_bytes()
        candidate = transform(source, spec) if item_state == "original" else source
        candidates[spec.relative_path] = candidate.decode("utf-8").replace("\r\n", "\n")

    food = candidates["PROGRAM/scripts/food.c"]
    itemsbox = candidates["PROGRAM/interface/itemsbox.c"]
    dialog = candidates["PROGRAM/dialogs/russian/Enc_Officer_dialog.c"]
    character_utility = (
        TARGET_ROOT / "PROGRAM/characters/characterUtilite.c"
    ).read_text(encoding="utf-8").replace("\r\n", "\n")

    required_food = (
        'bool OfficerSupply_IsEnabled(ref officer)',
        'if (sti(item.price) <= 0) return result;',
        'if (equipped != "" && equipped != "unarmed")',
        'if (GetCharacterItem(officer, equipped) > 0) return 0;',
        'int ammo = 8 + rand(6);',
        'OfficerSupply_RefillFromCabin(false);',
    )
    for marker in required_food:
        if food.count(marker) != 1:
            raise RuntimeError(f"auto-supply marker is missing or duplicated: {marker}")

    if food.count('OfficerSupply_FillItem(officer, chest, "bullet"') != 1:
        raise RuntimeError("bullet target is not filled exactly once")
    if food.count('OfficerSupply_FillItem(officer, chest, "GunPowder"') != 1:
        raise RuntimeError("gunpowder target is not filled exactly once")
    if 'OfficerSupply_FillItem(officer, chest, "Gold"' in food:
        raise RuntimeError("money must not enter auto-supply")
    equipment_transfer = '''if (!TakeNItems(officer, itemID, 1)) return 0;
        TakeNItems(chest, itemID, -1);'''
    item_transfer = '''if (!TakeNItems(officer, itemID, 1)) break;
        TakeNItems(chest, itemID, -1);'''
    if equipment_transfer not in food or item_transfer not in food:
        raise RuntimeError("chest subtraction must follow a successful officer add")
    if 'if (pchar.location != Get_My_Cabin()) return false;' not in food:
        raise RuntimeError("safe cabin boundary is missing")
    if 'if (dialogRun) return false;' not in food:
        raise RuntimeError("dialogue guard is missing")
    if 'if (LAi_grp_alarmactive) return false;' not in food:
        raise RuntimeError("active-alarm guard is missing")
    if 'if (LAi_IsFightMode(pchar)) return false;' not in food:
        raise RuntimeError("combat guard is missing")
    if itemsbox.count('OfficerSupply_RefillAllFromChest(arChest, true);') != 1:
        raise RuntimeError("cabin chest trigger is missing or duplicated")
    if 'if (pchar.boxname == "box1")' not in itemsbox:
        raise RuntimeError("opened cabin-box identity guard is missing")
    if 'arChest.money = GetCharacterItem(arChest, "Gold");' not in itemsbox:
        raise RuntimeError("runtime no longer proves GetCharacterItem accepts a chest aref")
    if dialog.count('Link.l3.go = "OfficerSupply_Enable";') != 1:
        raise RuntimeError("enable control is missing")
    if dialog.count('Link.l3.go = "OfficerSupply_Disable";') != 1:
        raise RuntimeError("disable control is missing")
    if dialog.count('AddDialogExitQuestFunction("OfficerSupply_AfterDialog");') != 1:
        raise RuntimeError("post-dialogue refill handoff is missing")

    take_start = character_utility.find(
        "bool TakeNItems(ref _refCharacter, string itemName, int n)"
    )
    take_end = character_utility.find("int GetCharacterCurrentIsland", take_start)
    if take_start < 0 or take_end < 0:
        raise RuntimeError("TakeNItems runtime contract is unavailable")
    take_function = character_utility[take_start:take_end]
    if 'if(q <= 0 && GetChrItemQuantity(_refCharacter) >= MAX_ITEM_TAKE)' not in take_function:
        raise RuntimeError("TakeNItems no longer rejects a full unique-item inventory")
    if "return false;" not in take_function or "return true;" not in take_function:
        raise RuntimeError("TakeNItems success/failure return contract changed")

    print("officer auto-supply patch: verified")
    print("state: old saves default off; manual equipment preserved; cabin-only refill")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    args = parser.parse_args()
    try:
        if args.action == "status":
            return PATCH.print_status(TARGET_ROOT)
        if args.action == "verify":
            return verify()
        if args.action == "apply":
            print(PATCH.apply(TARGET_ROOT))
        else:
            print(PATCH.revert(TARGET_ROOT))
        return PATCH.print_status(TARGET_ROOT)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

"""Compose the Metal squad-reserve and officer-upgrade gameplay layer."""

from __future__ import annotations

import hashlib


BASE = {
    "PROGRAM/scripts/food.c": "3a178dd249cb75713bd92d2e94a30c3c3dcc5daaf1777bdbbc6f492febcabf8e",
    "PROGRAM/ITEMS/itemLogic.c": "aab9f649883ae523289c865b3cde41afaff76c87c072acb9a5f15a8259651c03",
}

UPDATED = {
    "PROGRAM/scripts/food.c": "3a2756ae55d9db463b59a500b297f37eb9d70efff5f008bf08608bb8290eb78d",
    "PROGRAM/ITEMS/itemLogic.c": "59d52202cd76b4bd1c9e9e25a1db531ec1bd2bb55b231ac0d1c1b36528ddd853",
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def replace_once(data: bytes, old: str, new: str, path: str) -> bytes:
    old_bytes = old.replace("\n", "\r\n").encode("utf-8")
    new_bytes = new.replace("\n", "\r\n").encode("utf-8")
    count = data.count(old_bytes)
    if count != 1:
        raise ValueError(f"{path}: expected one anchor, got {count}: {old[:72]!r}")
    return data.replace(old_bytes, new_bytes, 1)


RESERVE = r'''
// Count and spend from the same active removable squad cargo owner.
// Travelling companions are deliberately excluded: their provisions stay aboard
// their own ship while they are outside the squad.
int SquadReserve_Goods(ref rChar, int goodID, bool isCompanionTraveler)
{
    if (isCompanionTraveler || !GetShipRemovableEx(rChar)) return GetCargoGoods(rChar, goodID);
    return GetSquadronGoods(pchar, goodID);
}

void SquadReserve_Remove(ref rChar, int goodID, int quantity, bool isCompanionTraveler)
{
    if (quantity < 1) return;
    if (isCompanionTraveler || !GetShipRemovableEx(rChar))
    {
        RemoveCharacterGoodsSelf(rChar, goodID, quantity);
        return;
    }
    // GetSquadronGoods excludes ShipRemovable=false companions, whereas
    // RemoveCharacterGoods does not. Mirror the former's donor set exactly.
    int available = GetCargoGoods(pchar, goodID);
    int take = quantity;
    if (take > available) take = available;
    if (take > 0) RemoveCharacterGoodsSelf(pchar, goodID, take);
    quantity = quantity - take;
    ref donor;
    int index;
    for (int slot = 1; slot < COMPANION_MAX; slot++)
    {
        if (quantity < 1) return;
        index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        donor = GetCharacter(index);
        if (!GetRemovable(donor) || !GetShipRemovableEx(donor)) continue;
        available = GetCargoGoods(donor, goodID);
        take = quantity;
        if (take > available) take = available;
        if (take > 0) RemoveCharacterGoodsSelf(donor, goodID, take);
        quantity = quantity - take;
    }
}

'''


EQUIPMENT = r'''bool OfficerSupply_ItemIsReturnable(string itemID)
{
    aref item;
    if (itemID == "" || itemID == "unarmed") return false;
    if (Items_FindItem(itemID, &item) < 0) return false;
    if (!CheckAttribute(item, "price")) return false;
    if (sti(item.price) <= 0) return false;
    if (CheckAttribute(item, "quest")) return false;
    return true;
}

bool OfficerSupply_IsEquipmentLocked(ref officer, string groupID, string itemID)
{
    if (CheckAttribute(officer, "HoldEquip")) return true;
    if (groupID == BLADE_ITEM_TYPE && CheckAttribute(officer, "DontChangeBlade")) return true;
    if (groupID == GUN_ITEM_TYPE && CheckAttribute(officer, "DontChangeGun")) return true;
    if (!OfficerSupply_ItemIsReturnable(itemID) && itemID != "" && itemID != "unarmed") return true;
    return false;
}

bool OfficerSupply_ReturnEquipment(ref officer, aref chest, string itemID)
{
    if (!OfficerSupply_ItemIsReturnable(itemID)) return false;
    if (GetCharacterItem(officer, itemID) < 1) return false;
    if (!TakeNItems(chest, itemID, 1)) return false;
    TakeNItems(officer, itemID, -1);
    return true;
}

'''


FILL_EQUIPMENT_OLD = r'''int OfficerSupply_FillEquipment(ref officer, aref chest, string groupID)
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
    if (GetCharacterEquipByGroup(officer, groupID) != itemID)
    {
        if (moved > 0)
        {
            TakeNItems(chest, itemID, 1);
            TakeNItems(officer, itemID, -1);
        }
        return 0;
    }
    return moved;
}'''


FILL_EQUIPMENT_NEW = r'''int OfficerSupply_FillEquipment(ref officer, aref chest, string groupID)
{
    string equipped = GetCharacterEquipByGroup(officer, groupID);
    if (OfficerSupply_IsEquipmentLocked(officer, groupID, equipped)) return 0;

    string itemID = OfficerSupply_FindBestItem(officer, chest, groupID);
    if (itemID == "" || itemID == equipped) return 0;

    float equippedScore = -1.0;
    aref equippedItem;
    if (equipped != "" && GetCharacterItem(officer, equipped) > 0)
    {
        if (Items_FindItem(equipped, &equippedItem) >= 0)
        {
            equippedScore = OfficerSupply_ItemScore(officer, equippedItem, groupID);
        }
    }

    aref item;
    if (Items_FindItem(itemID, &item) < 0) return 0;
    if (OfficerSupply_ItemScore(officer, item, groupID) <= equippedScore) return 0;

    int moved = 0;
    if (GetCharacterItem(officer, itemID) < 1)
    {
        if (!TakeNItems(officer, itemID, 1)) return 0;
        TakeNItems(chest, itemID, -1);
        moved = 1;
    }
    EquipCharacterByItem(officer, itemID);
    if (GetCharacterEquipByGroup(officer, groupID) != itemID)
    {
        if (moved > 0)
        {
            TakeNItems(chest, itemID, 1);
            TakeNItems(officer, itemID, -1);
        }
        return 0;
    }

    // Return only the displaced, ordinary item; other officer inventory stays
    // untouched so the automation cannot clean out a player's manual loadout.
    if (equipped != "" && equipped != itemID)
    {
        OfficerSupply_ReturnEquipment(officer, chest, equipped);
    }
    return moved;
}'''


def _patch_daily_food(data: bytes) -> bytes:
    start = data.index(b"void DailyEatCrewUpdateForShip(ref rChar, bool IsCompanionTraveler)")
    # This is the last function in food.c in the reviewed input.
    end = len(data)
    daily = data[start:end]
    replacements = {
        b"GetCargoGoods(rChar, GOOD_MEDICAMENT)": b"SquadReserve_Goods(rChar, GOOD_MEDICAMENT, IsCompanionTraveler)",
        b"RemoveCharacterGoodsSelf(rChar, GOOD_MEDICAMENT, cn);": b"SquadReserve_Remove(rChar, GOOD_MEDICAMENT, cn, IsCompanionTraveler);",
        b"GetCargoGoods(rChar, GOOD_RUM)": b"SquadReserve_Goods(rChar, GOOD_RUM, IsCompanionTraveler)",
        b"RemoveCharacterGoodsSelf(rChar, GOOD_RUM, iCrewQty);": b"SquadReserve_Remove(rChar, GOOD_RUM, iCrewQty, IsCompanionTraveler);",
        b"GetCargoGoods(rChar, GOOD_FOOD)": b"SquadReserve_Goods(rChar, GOOD_FOOD, IsCompanionTraveler)",
        b"RemoveCharacterGoodsSelf(rChar, GOOD_FOOD, iCrewQty);": b"SquadReserve_Remove(rChar, GOOD_FOOD, iCrewQty, IsCompanionTraveler);",
    }
    for old, new in replacements.items():
        if old not in daily:
            raise ValueError(f"food.c: missing daily reserve consumer {old!r}")
        daily = daily.replace(old, new)
    return data[:start] + daily + data[end:]


def transform(outputs: dict[str, bytes]) -> dict[str, bytes]:
    """Apply the layer to suite outputs without mutating archived source."""
    result = dict(outputs)
    for path, expected in BASE.items():
        if path not in result:
            raise ValueError(f"squad supply layer: missing {path}")
        if digest(result[path]) != expected:
            raise ValueError(
                f"squad supply layer: unreviewed input {path}: {digest(result[path])}"
            )

    food = result["PROGRAM/scripts/food.c"]
    food = replace_once(food, FILL_EQUIPMENT_OLD, EQUIPMENT + FILL_EQUIPMENT_NEW,
                        "PROGRAM/scripts/food.c")
    food = replace_once(food,
                        "// boal food for crew 20.01.2004 -->\nvoid DailyEatCrewUpdate()",
                        RESERVE + "// boal food for crew 20.01.2004 -->\nvoid DailyEatCrewUpdate()",
                        "PROGRAM/scripts/food.c")
    result["PROGRAM/scripts/food.c"] = _patch_daily_food(food)

    item_logic = result["PROGRAM/ITEMS/itemLogic.c"]
    result["PROGRAM/ITEMS/itemLogic.c"] = replace_once(
        item_logic,
        "\tBox_OnLoadLocation(activeLocation);",
        "\tBox_OnLoadLocation(activeLocation);\n\t// The current cabin's box1 is now materialized; refill without opening its UI.\n\tOfficerSupply_RefillFromCabin(false);",
        "PROGRAM/ITEMS/itemLogic.c",
    )
    return result

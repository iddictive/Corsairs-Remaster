#!/usr/bin/env python3
"""Static falsifier for the Metal squad reserve and officer equipment layer."""

from __future__ import annotations

from pathlib import Path

import patch_gameplay_suite as suite
import patch_squad_common_supply as layer


def main() -> int:
    source = suite.NATIVE_ROOT
    inputs = {path: (source / path).read_bytes() for path in layer.BASE}
    outputs = layer.transform(inputs)
    food = outputs["PROGRAM/scripts/food.c"].decode("utf-8")
    item_logic = outputs["PROGRAM/ITEMS/itemLogic.c"].decode("utf-8")

    required_food = (
        "int SquadReserve_Goods(ref rChar, int goodID, bool isCompanionTraveler)",
        "return GetSquadronGoods(pchar, goodID);",
        "RemoveCharacterGoods(pchar, goodID, quantity);",
        "if (isCompanionTraveler) return GetCargoGoods(rChar, goodID);",
        "bool OfficerSupply_IsEquipmentLocked(ref officer, string groupID, string itemID)",
        "if (CheckAttribute(officer, \"HoldEquip\")) return true;",
        "itemID != \"unarmed\"",
        "if (CheckAttribute(item, \"quest\")) return false;",
        "if (sti(item.price) <= 0) return false;",
        "OfficerSupply_ReturnEquipment(officer, chest, equipped);",
        "OfficerSupply_ItemScore(officer, item, groupID) <= equippedScore",
    )
    for marker in required_food:
        if marker not in food:
            raise RuntimeError(f"missing squad supply safety marker: {marker}")

    if food.count("SquadReserve_Remove(rChar, GOOD_RUM") != 2:
        raise RuntimeError("rum consumption did not use the shared reserve exactly twice")
    if food.count("SquadReserve_Remove(rChar, GOOD_FOOD") != 2:
        raise RuntimeError("food consumption did not use the shared reserve exactly twice")
    if food.count("SquadReserve_Remove(rChar, GOOD_MEDICAMENT") != 2:
        raise RuntimeError("medicine consumption did not use the shared reserve exactly twice")
    if "OfficerSupply_RefillFromCabin(false);" not in item_logic:
        raise RuntimeError("cabin entry does not trigger automatic refill")
    if item_logic.index("Box_OnLoadLocation(activeLocation);") > item_logic.index(
        "OfficerSupply_RefillFromCabin(false);"
    ):
        raise RuntimeError("refill runs before the cabin chest is materialized")

    print("Squad common supply probe: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Static falsifier for the Metal squad reserve and officer equipment layer."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

import patch_gameplay_suite as suite
import patch_squad_common_supply as layer


VM_MAIN = r'''
 InterfaceStates.Launched = false;
 makeref(pchar, Characters[0]);
 nMainCharacterIndex = 0;
 pchar.id = "ReserveHero"; pchar.index = 0;
 pchar.Fellows.Companions.id1 = 1;
 pchar.Fellows.Companions.id2 = 2;
 pchar.Fellows.Companions.id3 = 3;
 Characters[1].id = "LockedShip"; Characters[1].index = 1;
 Characters[1].ShipRemovable = false;
 Characters[2].id = "Donor"; Characters[2].index = 2;
 Characters[3].id = "QuestShip"; Characters[3].index = 3;
 Characters[3].NonRemovable = true;
 for (int ship = 0; ship < 5; ship++) Characters[ship].Ship.Type = 0;
 Goods[GOOD_FOOD].name = "food";
 Goods[GOOD_RUM].name = "rum";
 Goods[GOOD_MEDICAMENT].name = "medicament";
 int good;
 string name;
 for (int row = 0; row < 3; row++)
 {
   good = GOOD_FOOD;
   if (row == 1) good = GOOD_RUM;
   if (row == 2) good = GOOD_MEDICAMENT;
   name = Goods[good].name;
   pchar.Ship.Cargo.Goods.(name) = 2;
   Characters[1].Ship.Cargo.Goods.(name) = 50;
   Characters[2].Ship.Cargo.Goods.(name) = 8;
   Characters[3].Ship.Cargo.Goods.(name) = 70;
   if (SquadReserve_Goods(pchar, good, false) != 10) Trace("PROBE_FAILED reserve count");
   SquadReserve_Remove(pchar, good, 6, false);
   if (GetCargoGoods(pchar, good) != 0 || GetCargoGoods(&Characters[2], good) != 4) Trace("PROBE_FAILED reserve spending");
   if (GetCargoGoods(&Characters[1], good) != 50 || GetCargoGoods(&Characters[3], good) != 70) Trace("PROBE_FAILED protected donor");
   SquadReserve_Remove(pchar, good, 0, false);
   if (SquadReserve_Goods(pchar, good, false) != 4) Trace("PROBE_FAILED zero spending");
   SquadReserve_Remove(&Characters[1], good, 3, false);
   if (GetCargoGoods(&Characters[1], good) != 47 || SquadReserve_Goods(pchar, good, false) != 4) Trace("PROBE_FAILED locked consumer isolation");
   Characters[4].Ship.Cargo.Goods.(name) = 9;
   SquadReserve_Remove(&Characters[4], good, 2, true);
   if (GetCargoGoods(&Characters[4], good) != 7 || SquadReserve_Goods(pchar, good, false) != 4) Trace("PROBE_FAILED traveller isolation");
 }
 Items[0].id = "probe_blade"; Items[0].price = 10;
 Items[1].id = "probe_quest"; Items[1].price = 10; Items[1].quest = true;
 Items[2].id = "probe_free"; Items[2].price = 0;
 Characters[2].items.probe_blade = 2;
 Characters[2].items.probe_quest = 1;
 Characters[2].items.probe_free = 1;
 aref chest;
 makearef(chest, Locations[0].box1);
 if (!OfficerSupply_ReturnEquipment(&Characters[2], chest, "probe_blade")) Trace("PROBE_FAILED return ordinary");
 if (GetCharacterItem(&Characters[2], "probe_blade") != 1 || GetCharacterItem(chest, "probe_blade") != 1) Trace("PROBE_FAILED item conservation");
 if (OfficerSupply_ReturnEquipment(&Characters[2], chest, "probe_quest") || OfficerSupply_ReturnEquipment(&Characters[2], chest, "probe_free")) Trace("PROBE_FAILED protected item");
 if (OfficerSupply_IsEnabled(&Characters[2])) Trace("PROBE_FAILED old save enabled");
 Characters[2].HoldEquip = true;
 if (!OfficerSupply_IsEquipmentLocked(&Characters[2], BLADE_ITEM_TYPE, "probe_blade")) Trace("PROBE_FAILED manual lock");
 Trace("SQUAD_STATE_PROBE_FINISHED");
 ExitProgram();
'''


def verify_vm(outputs, negative):
    # Reuse the existing isolated Metal script VM boundary. Only cargo-weight
    # recalculation is stubbed; reserve enumeration and item transfers are real.
    import custody_life_probe as vm
    root = Path(tempfile.mkdtemp(prefix="corsairs-squad-probe."))
    try:
        shutil.copytree(vm.RUNTIME / "PROGRAM", root / "PROGRAM")
        (root / "RESOURCE").symlink_to(vm.RUNTIME / "RESOURCE", target_is_directory=True)
        for path, data in outputs.items():
            (root / path).write_bytes(data)
        (root / "engine.ini").write_text("full_screen = 0\nwindow_borders = 1\nscreen_x = 320\nscreen_y = 240\nrun_in_background = 1\nSteam = 0\nprogram_directory = PROGRAM\nprogram_bin_directory = PROGRAM_BIN\nrun = seadogs.c\nmax_fps = 30\n[script]\nruntimelog = 1\ntracefiles = 1\n[compatibility]\ntarget_version = teho\n")
        vm.replace_body(root, "characters/characterUtilite.c", "RecalculateCargoLoad", "return 0;")
        vm.replace_body(root, "seadogs.c", "Main", VM_MAIN)
        if negative:
            vm.replace_body(root, "scripts/food.c", "SquadReserve_Remove", "RemoveCharacterGoods(pchar, goodID, quantity);")
        log = vm.run_engine(root)
        assert "SQUAD_STATE_PROBE_FINISHED" in log, log
        if negative:
            assert "PROBE_FAILED protected donor" in log, log
        else:
            assert "PROBE_FAILED" not in log, log
        print("Squad VM: PASS (" + ("unsafe donor mutation rejected" if negative else "3 commodities, conservation, locked/travelling exclusions, item return, old-save/manual locks") + ")")
    finally:
        subprocess.run(["/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp", str(root.resolve())], check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vm", action="store_true")
    parser.add_argument("--negative", action="store_true")
    args = parser.parse_args()
    source = suite.NATIVE_ROOT
    inputs = {path: (source / path).read_bytes() for path in layer.BASE}
    outputs = layer.transform(inputs)
    food = outputs["PROGRAM/scripts/food.c"].decode("utf-8")
    item_logic = outputs["PROGRAM/ITEMS/itemLogic.c"].decode("utf-8")

    required_food = (
        "int SquadReserve_Goods(ref rChar, int goodID, bool isCompanionTraveler)",
        "return GetSquadronGoods(pchar, goodID);",
        "if (!GetRemovable(donor) || !GetShipRemovableEx(donor)) continue;",
        "if (isCompanionTraveler || !GetShipRemovableEx(rChar)) return GetCargoGoods(rChar, goodID);",
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

    for path, data in outputs.items():
        assert layer.digest(data) == layer.UPDATED[path], path
        bad = dict(inputs)
        bad[path] += b"unreviewed input"
        try:
            layer.transform(bad)
        except ValueError:
            pass
        else:
            raise AssertionError(f"accepted unknown input: {path}")

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
    if args.vm or args.negative:
        verify_vm(outputs, args.negative)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

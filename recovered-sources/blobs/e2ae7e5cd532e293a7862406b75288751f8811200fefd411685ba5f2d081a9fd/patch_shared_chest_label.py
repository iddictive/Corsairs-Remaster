#!/usr/bin/env python3
"""Mark and use only box1 in the player's loaded cabin as the shared chest."""
import argparse
import subprocess
from pathlib import Path

from runtime_script_patch import atomic_write, sha256

PROJECT = Path(__file__).resolve().parents[1]
RELATIVE = "PROGRAM/interface/itemsbox.c"
BASE_SHA256 = "5b35060ae02e96089d9e20991897c2309c838eb2926ef4f67be0a533fabfc029"
V1_SHA256 = "906a5fd0422633105730f13e8523e91e3a39bd8a79b8744ac866acee28d01659"
V2_SHA256 = "ab9a1e002bbece472d644dd75d7e9937f359da72292c19289d759b62dc38389f"
V3_SHA256 = "0f97a1253c871e55c429d95313182de2e2dfe72423d844593272ea4f08bb61e8"

INIT = "void InitInterface_RS(string iniName, ref itemsRef, string faceID)"
HELPER = '''bool OfficerSupply_IsSharedChestInterface()
{
	if (sFaceID != "") return false;
	if (LAi_boarding_process) return false;
	if (!CheckAttribute(pchar, "boxname")) return false;
	if (pchar.boxname != "box1") return false;
	int locationIndex = FindLoadedLocation();
	if (locationIndex < 0) return false;
	if (!CheckAttribute(&Locations[locationIndex], "CabinType")) return false;
	return true;
}

'''
HELPER_NEW = '''bool OfficerSupply_IsSharedChestInterface()
{
	if (sFaceID != "") return false;
	if (LAi_boarding_process) return false;
	int locationIndex = FindLoadedLocation();
	if (locationIndex < 0) return false;
	if (locationIndex != FindLocation(Get_My_Cabin())) return false;
	if (GetAttributeName(arChest) != "box1") return false;
	return true;
}

'''

SUPPLY_OLD = '''		if (pchar.location == Get_My_Cabin() && CheckAttribute(pchar, "boxname"))
		{
			if (pchar.boxname == "box1")
			{
				OfficerSupply_RefillAllFromChest(arChest, true);
			}
		}'''
SUPPLY_NEW = '''		if (OfficerSupply_IsSharedChestInterface())
		{
			OfficerSupply_RefillAllFromChest(arChest, true);
		}'''

CAPTION = '\tSetFormatedText("MAIN_CAPTION", XI_ConvertString(sGetInterfaceTypeStr("titleExchangeItems", "titleItemsBox", "titleDeadItems","titleBarrel")));'
LABEL_V1 = CAPTION + '''
	if (sGetInterfaceType() == INTERFACETYPE_CHEST && pchar.location == Get_My_Cabin())
	{
		if (CheckAttribute(pchar, "boxname"))
		{
			if (pchar.boxname == "box1") SetFormatedText("MAIN_CAPTION", "Общий сундук");
		}
	}'''
LABEL_V2 = CAPTION + '''
	if (sGetInterfaceType() == INTERFACETYPE_CHEST && FindLoadedLocation() == FindLocation(Get_My_Cabin()))
	{
		if (CheckAttribute(pchar, "boxname"))
		{
			if (pchar.boxname == "box1")
			{
				SetFormatedText("MAIN_CAPTION", "Общий сундук");
				SetFormatedText("TABSTR_3", "Общий сундук");
			}
		}
	}'''
LABEL_NEW = CAPTION + '''
	if (OfficerSupply_IsSharedChestInterface())
	{
		SetFormatedText("MAIN_CAPTION", "Общий сундук");
		SetFormatedText("TABSTR_3", "Общий сундук");
	}'''


def encoded(text, newline):
    return text.replace("\n", newline.decode()).encode("utf-8")


def transform(source):
    digest = sha256(source)
    if digest not in {BASE_SHA256, V1_SHA256, V2_SHA256, V3_SHA256}:
        raise RuntimeError("unknown itemsbox revision; review before applying")
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    if digest != V3_SHA256:
        label = LABEL_V2 if digest == V2_SHA256 else LABEL_V1 if digest == V1_SHA256 else CAPTION
        for old, new in ((INIT, HELPER + INIT), (SUPPLY_OLD, SUPPLY_NEW), (label, LABEL_NEW)):
            old_bytes, new_bytes = encoded(old, newline), encoded(new, newline)
            if result.count(old_bytes) != 1:
                raise RuntimeError("expected itemsbox block is not unique")
            result = result.replace(old_bytes, new_bytes, 1)
    old_helper, new_helper = encoded(HELPER, newline), encoded(HELPER_NEW, newline)
    if result.count(old_helper) != 1:
        raise RuntimeError("expected obsolete shared-chest helper is not unique")
    result = result.replace(old_helper, new_helper, 1)
    return result


def is_applied(source):
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    return (source.count(encoded(HELPER_NEW, newline)) == 1
            and source.count(encoded("if (OfficerSupply_IsSharedChestInterface())", newline)) == 2
            and source.count(encoded(HELPER, newline)) == 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"))
    parser.add_argument("--runtime", choices=("native-storm", "native-metal"), default="native-storm")
    args = parser.parse_args()
    root = PROJECT / "experiments" / args.runtime / ".cache"
    path = root / "runtime" / RELATIVE
    current = path.read_bytes()
    if is_applied(current):
        print("Shared chest marker: already applied")
        return
    candidate = transform(current)
    if not is_applied(candidate):
        raise RuntimeError("generated shared chest marker is incomplete")
    if args.action == "apply":
        processes = subprocess.run(["ps", "-axo", "comm="], capture_output=True, text=True, check=True)
        if any(line.endswith(("/native-engine", "/metal-engine")) for line in processes.stdout.splitlines()):
            raise RuntimeError("close the game before applying the chest marker")
        if path.read_bytes() != current:
            raise RuntimeError("itemsbox changed concurrently")
        atomic_write(path, candidate)
        if path.read_bytes() != candidate:
            raise RuntimeError("shared chest marker write did not persist")
    print("Shared chest marker: " + ("applied" if args.action == "apply" else "prepared"))


if __name__ == "__main__":
    main()

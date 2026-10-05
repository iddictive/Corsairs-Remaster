#!/usr/bin/env python3
"""Source-only encounter UI layer; no filesystem/runtime mutation in prepare.

Compose after the gameplay layers. Native integration must provide message
31131 (MSG_WORLDMAP_ENTER_SEA_DIRECT): clear every isSelect, then emit
ExitFromWorldMap without ExitFromMap/enemy selection. 31130 stays targeted.
The worldmap owner supplies WdmTrafficRefresh, trafficPlayerPower and
WdmTrafficFleetPower(ref fleet, int rank); outer trafficCondition stays separate.
"""

from __future__ import annotations

import hashlib
from pathlib import Path


BASES = {
    "PROGRAM/interface/map.c": "1132a781a0c41f4b05e2de11179a37412cb61e24b76931dd45558c282c3d1286",
    "PROGRAM/battle_interface/WmInterface.c": "214260e2258e51bf54b642737e6c95413efb7e46708407410aadd0119ed3a245",
    "PROGRAM/battle_interface/loginterface.c": "88a187aec46bf600ff6c239eb2d358b8155775a006f70d085917fa7bc79a849c",
    "RESOURCE/INI/interfaces/map.ini": "112a4943f67b1d85d6e9fad17dacbdc0d40f338a058af8e4373a48f2a0d5f608",
}
HELPER_SHA256 = "27b697ce0738aaf8fb30172348acda2bb06f2a5e8adf4e30e84182962fa2e5aa"


def enc(text: str) -> bytes:
    return text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8")


def replace(data: bytes, old: str, new: str, count: int = 1) -> bytes:
    anchor = enc(old)
    found = data.count(anchor)
    if found != count:
        raise RuntimeError(f"fleet UI anchor mismatch ({found} != {count}): {old[:90]!r}")
    return data.replace(anchor, enc(new))


def replace_function(data: bytes, start: str, end: str, base: str, new: str) -> bytes:
    begin, finish = enc(start), enc(end)
    if data.count(begin) != 1 or data.count(finish) != 1:
        raise RuntimeError(f"fleet UI function boundary mismatch: {start}")
    first, last = data.index(begin), data.index(finish)
    if first >= last or hashlib.sha256(data[first:last]).hexdigest() != base:
        raise RuntimeError(f"fleet UI function body mismatch: {start}")
    return data[:first] + enc(new) + data[last:]


def prepare_map(data: bytes) -> bytes:
    data = replace(data, "void InitInterface(string iniName)\n{", """void InitInterface(string iniName)
{
	sQuestSeaCharId = "";
	isSkipable = false;""")
    data = replace_function(data, "void wdmRecalcReloadToSea()", "void DoPostExit()",
                            "666c7fb7afbc2ada334ae9f2d33fd8d0a3c519624d15f0757b389c36aec862af",
                            "void wdmRecalcReloadToSea()\n{\n\tWdmFleetUIInfo();\n}\n\n")
    data = replace(data, 'if(isSkipable == true)', 'if(isSkipable == true && GetSelectable("B_CANCEL"))')
    data = replace(data, """	else
	{
		IDoExit(RC_INTERFACE_MAP_EXIT);
		wdmReloadToSea();
	}""", """	else
	{
		if (sQuestSeaCharId != "") wdmEnterSeaQuest(sQuestSeaCharId);
		IDoExit(RC_INTERFACE_MAP_EXIT);
		wdmReloadToSea();
	}""")
    data = replace(data, 'case "B_OK":\n\t\t\tif(comName=="activate" || comName=="click")',
                   'case "B_OK":\n\t\t\tif(GetSelectable("B_OK") && (comName=="activate" || comName=="click"))')
    data = replace(data, 'case "B_CANCEL":\n\t\t\tif(comName=="activate" || comName=="click")',
                   'case "B_CANCEL":\n\t\t\tif(GetSelectable("B_CANCEL") && (comName=="activate" || comName=="click"))')
    data = replace(data, '\t\tcase "B_OK":', """		case "B_SEA":
			if (GetSelectable("B_SEA") && !bFleetUIQuest && (comName=="activate" || comName=="click"))
			{
				IDoExit(RC_INTERFACE_MAP_EXIT);
				SendMessage(&worldMap,"l",MSG_WORLDMAP_ENTER_SEA_DIRECT);
			}
		break;

		case "B_OK":""")
    data = replace(data, 'SetCurrentNode("BTN_OK")', 'SetCurrentNode("B_OK")')
    data = replace(data, 'SetSelectable("BTN_CANCEL",true)', 'SetSelectable("B_CANCEL",true)')
    data = replace(data, 'SetSelectable("BTN_CANCEL",false)', 'SetSelectable("B_CANCEL",false)', 3)
    data = replace(data, '\tpchar.space_press = 0;', """	// Story, storm and island actions retain their authored transition.
	bool plainSea = !bFleetUIQuest && sti(worldMap.encounter_type) != 0 && sti(worldMap.encounter_type) != 4;
	SetSelectable("B_SEA", plainSea);
	if (!plainSea) SetNodeUsing("B_SEA", false);
	if (plainSea) SetCurrentNode("B_SEA");
	pchar.space_press = 0;""")
    data = replace(data, 'EI_CreateFrame("BORDERS", 245,154,555,330);',
                   'EI_CreateFrame("BORDERS", 145,94,655,250);')
    helper = Path(__file__).with_name("gameplay") / "fleet-encounter-ui.c"
    source = helper.read_bytes()
    if hashlib.sha256(source).hexdigest() != HELPER_SHA256:
        raise RuntimeError("unreviewed fleet encounter UI helper")
    return data + enc("\n\n") + enc(source.decode("utf-8"))


def prepare_wm(data: bytes) -> bytes:
    data = replace(data, """case "EnterToSea":
		SendMessage(&worldMap,"l",MSG_WORLDMAP_LAUNCH_EXIT_TO_SEA);""", """case "EnterToSea":
		SendMessage(&worldMap,"l",MSG_WORLDMAP_ENTER_SEA_DIRECT);""")
    # Both producers (possible commands/current action) choose plain sea by
    # default around fleets. Keep island/storm paths and dead-character guard.
    for action in ("EnterToShip", "EnterToAttack", "EnterToEnemy"):
        data = replace(data, f'Log_SetActiveAction("{action}");', 'Log_SetActiveAction("EnterToSea");', 2)
    for line in ('BattleInterface.Commands.EnterToShip.enable\t= true;',
                 'BattleInterface.Commands.EnterToAttack.enable = true;',
                 'BattleInterface.Commands.EnterToEnemy.enable = true;'):
        data = replace(data, line, line + '\n\t\tBattleInterface.Commands.EnterToSea.enable = true;')
    data = replace(data, '//Log_SetActiveAction("EnterToIsland");\n\t\tLog_SetActiveAction("EnterToSea");  //boal',
                   'Log_SetActiveAction("EnterToIsland");', 2)
    data = replace(data, '//Log_SetActiveAction("EnterToIsland");\n\t\t\tLog_SetActiveAction("EnterToSea");  //boal',
                   'Log_SetActiveAction("EnterToIsland");', 2)
    # The baseline assigns these two icon pairs to the opposite commands.
    data = replace(data, 'BattleInterface.Commands.EnterToShip.picNum\t\t= 1;', 'BattleInterface.Commands.EnterToSea.picNum\t\t= 1;')
    data = replace(data, 'BattleInterface.Commands.EnterToShip.selPicNum\t= 9;', 'BattleInterface.Commands.EnterToSea.selPicNum\t= 9;')
    data = replace(data, 'BattleInterface.Commands.EnterToSea.picNum\t\t= 4;', 'BattleInterface.Commands.EnterToShip.picNum\t\t= 4;')
    data = replace(data, 'BattleInterface.Commands.EnterToSea.selPicNum\t= 12;', 'BattleInterface.Commands.EnterToShip.selPicNum\t= 12;')
    for action, label in (("EnterToShip", "Сблизиться"), ("EnterToAttack", "Атаковать"), ("EnterToEnemy", "Преследовать")):
        # Whitespace differs per command; pin the full known note assignment.
        tabs = '\t\t\t' if action == "EnterToAttack" else '\t\t'
        data = replace(data, f'BattleInterface.Commands.{action}.note{tabs}= LanguageConvertString(idLngFile, "worldmap_sea");',
                       f'BattleInterface.Commands.{action}.note{tabs}= "{label}";')
    return data


def prepare_log(data: bytes) -> bytes:
    return replace(data, """case "EnterToSea":
			bEC = true;
			SendMessage(&worldMap,"l",MSG_WORLDMAP_LAUNCH_EXIT_TO_SEA);""", """case "EnterToSea":
			bEC = true;
			SendMessage(&worldMap,"l",MSG_WORLDMAP_ENTER_SEA_DIRECT);""")


def prepare_ini(data: bytes) -> bytes:
    data = replace(data, 'item = 90,FORMATEDTEXT,INFO_TEXT_QUESTION\n', '')
    data = replace(data, 'item = 100,TEXTBUTTON2,B_OK', 'item = 90,SCROLLER,INFO_SCROLL\nitem = 100,TEXTBUTTON2,B_SEA\nitem = 100,TEXTBUTTON2,B_OK')
    data = replace(data, 'start = B_OK', 'start = B_SEA')
    for old, new in (('position = 240,119,560,469', 'position = 140,65,660,540'),
                     ('position = 240,341,560,469', 'position = 140,250,660,540'),
                     ('position = 241,144,559,341', 'position = 141,90,659,250'),
                     ('position = 251,121,548,147', 'position = 151,67,648,91'),
                     ('position = 268,432,398,464', 'position = 316,495,484,527'),
                     ('position = 402,432,532,464', 'position = 490,495,648,527')):
        data = replace(data, old, new)
    data = replace(data, '[INFO_TEXT]\nposition = 242,344,558,425\nfontScale = 0.9\nlineSpace = 13', """[INFO_TEXT]
command = click,select:INFO_TEXT
command = upstep
command = downstep
command = speedup
command = speeddown
command = deactivate,select:B_OK
position = 155,262,625,479
scrollerName = INFO_SCROLL
fontScale = 0.9
lineSpace = 16""")
    data = replace(data, 'command = rightstep,select:B_CANCEL\nposition = 316,495,484,527',
                   'command = leftstep,select:B_SEA\ncommand = rightstep,select:B_CANCEL\ncommand = upstep,select:INFO_TEXT\nposition = 316,495,484,527')
    data = replace(data, 'command = leftstep,select:B_OK\nposition = 490,495,648,527',
                   'command = leftstep,select:B_OK\ncommand = rightstep,select:B_SEA\ncommand = upstep,select:INFO_TEXT\nposition = 490,495,648,527')
    return data + enc("""
[B_SEA]
bBreakCommand
command = deactivate,event:exitCancel
command = activate
command = click
command = leftstep,select:B_CANCEL
command = rightstep,select:B_OK
command = upstep,select:INFO_TEXT
position = 152,495,310,527
string = worldmap_sea
glowoffset = 0,0

[INFO_SCROLL]
command = click
command = upstep
command = downstep
command = deactivate,select:B_OK
position = 628,262,645,479
ownedControl = INFO_TEXT
""")


PREPARERS = {
    "PROGRAM/interface/map.c": prepare_map,
    "PROGRAM/battle_interface/WmInterface.c": prepare_wm,
    "PROGRAM/battle_interface/loginterface.c": prepare_log,
    "RESOURCE/INI/interfaces/map.ini": prepare_ini,
}
PATHS = tuple(PREPARERS)


def prepare(relative: str, data: bytes) -> bytes:
    if relative not in PREPARERS:
        return data
    # Parent pins the complete prior-composed input and final output hashes.
    # This layer owns exact local anchors, not another layer's whole-file hash.
    return PREPARERS[relative](data)

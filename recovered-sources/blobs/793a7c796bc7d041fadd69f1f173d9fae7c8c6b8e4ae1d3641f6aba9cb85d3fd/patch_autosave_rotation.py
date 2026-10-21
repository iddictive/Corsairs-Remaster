#!/usr/bin/env python3
"""Install rotating autosaves and decouple boarding from the generic save event."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


PATCH = PatchSet(
    TARGET_ROOT / ".codex-autosave-fixes" / "20260913-rotation-v1",
    (
        FilePatch(
            "PROGRAM/interface/interface.c",
            "eeb4fef17da0ba1f801be27d36bf51ee464940029159676d7fc6efbf381a8bfd",
            "4a0d377586c1e7cab50fe4653b6c8dd17f9a25b633d3b8260366b2e6db94df24",
            (
                (
                    '#event_handler("evntOptionsBreak","procOptionsBreak");\n',
                    '#event_handler("evntOptionsBreak","procOptionsBreak");\n'
                    '#event_handler("frame","AutoSaveProcessFrame");\n',
                ),
                (
                '''void MakeAutoSave()
{
\tif( InterfaceStates.Launched != 0 ) {return;}

\taref arScrShoter;
\tif( !GetEntity(&arScrShoter,"scrshoter") ) {
\t\tSetEventHandler("makescrshot","AutoSaveContinue",0);
\t\tCreateScreenShoter();
\t\tPostEvent("makescrshot",1);
\t} else { return; }
}

void AutoSaveContinue()
{
\tDelEventHandler("makescrshot","AutoSaveContinue");

\tstring curSave = PlayerProfile.name + " AutoSave";
\tSendMessage(&GameInterface,"ls",MSG_INTERFACE_DELETE_SAVE_FILE,curSave);
\tstring sSaveDescriber = GetSaveDataString(GetCurLocationName());

\tSetEventHandler("evntSave","SaveGame",1);
\tPostEvent("evntSave",0,"ss", "SAVE\\"+PlayerProfile.name+"\\"+curSave, sSaveDescriber);
}
''',
                '''int gAutoSaveElapsed = 0;

bool TryMakeAutoSave()
{
\tif( InterfaceStates.Launched != 0 ) {return false;}

\taref arScrShoter;
\tif( GetEntity(&arScrShoter,"scrshoter") ) {return false;}

\tSetEventHandler("makescrshot","AutoSaveContinue",0);
\tCreateScreenShoter();
\tPostEvent("makescrshot",1);
\treturn true;
}

void MakeAutoSave()
{
\tTryMakeAutoSave();
}

void AutoSaveProcessFrame()
{
\tif( !CheckAttribute(&InterfaceStates,"EnabledAutoSaveMode") || sti(InterfaceStates.EnabledAutoSaveMode) == 0 )
\t{
\t\tgAutoSaveElapsed = 0;
\t\treturn;
\t}
\tif( !CheckAttribute(&InterfaceStates,"Buttons.Save.enable") ) {return;}
\tif( InterfaceStates.Launched != 0 ) {return;}
\tif( CheckAttribute(pchar,"pause") && sti(pchar.pause) != 0 ) {return;}

\tgAutoSaveElapsed = gAutoSaveElapsed + iRealDeltaTime;
\tif( gAutoSaveElapsed < 300000 ) {return;}
\tif( !CheckSaveGameEnabled() ) {return;}
\tif( TryMakeAutoSave() ) {gAutoSaveElapsed = 0;}
}

void AutoSaveContinue()
{
\tDelEventHandler("makescrshot","AutoSaveContinue");
\tgAutoSaveElapsed = 0;

\tint AutoSaveIndex = 1;
\tif( CheckAttribute(&PlayerProfile,"AutoSaveIndex") )
\t{
\t\tAutoSaveIndex = sti(PlayerProfile.AutoSaveIndex);
\t\tif(AutoSaveIndex >= 12) {AutoSaveIndex = 1;}
\t\telse {AutoSaveIndex++;}
\t}
\tPlayerProfile.AutoSaveIndex = AutoSaveIndex;

\tstring curSave = PlayerProfile.name + " AutoSave " + AutoSaveIndex;
\tstring sSaveDescriber = GetSaveDataString(GetCurLocationName());

\tSetEventHandler("evntSave","SaveAutoGame",1);
\tPostEvent("evntSave",0,"ss", "SAVE\\"+PlayerProfile.name+"\\"+curSave, sSaveDescriber);
}
''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/sea_ai/AIAbordage.c",
            "1979c2856871429365ba058431c32c5b29ddedec0262e47cf354609376030480",
            "a1c8d1c2c14851469ad147a3ea77d93236f85871bf7cdc17751206d2cd737831",
            (
                (
                    '''\t\t\t\t\t\t//MakeAutoSaveAndGoOnAbord(); //eddy. чтобы глюков не було.
\t\t\t\t\t\tMakeAutoSave();
\t\t\t\t\t\tSetEventHandler("evntSave","Continue_Sea_AbordageLoadPre", 0);''',
                    '''\t\t\t\t\t\tSetEventHandler("evntAutoSaveFinished","Continue_Sea_AbordageLoadPre", 0);
\t\t\t\t\t\tif(!TryMakeAutoSave())
\t\t\t\t\t\t{
\t\t\t\t\t\t\tDelEventHandler("evntAutoSaveFinished","Continue_Sea_AbordageLoadPre");
\t\t\t\t\t\t\tContinue_Sea_AbordageLoad();
\t\t\t\t\t\t}''',
                ),
                (
                    '\tDelEventHandler("evntSave","Continue_Sea_AbordageLoadPre");',
                    '\tDelEventHandler("evntAutoSaveFinished","Continue_Sea_AbordageLoadPre");',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/seadogs.c",
            "1f762b84e39c2d7878b6f8999de246867b2882b4e6f83c8a0a190c0047315b84",
            "1ca8cdd17dcc0269fa10f59b9d87639f07101a731a8a2a5dc8cee32762a36942",
            ((
                '''void SaveGame()
{
\tstring saveName = GetEventData();
\tstring saveData = GetEventData();
\tDelEventHandler("evntSave","SaveGame");

\taref arTmp;
\tif( !GetEntity(arTmp,"fader") )
\t{
\t\t//implemet interface
\t\t//LaunchQuickSaveMenu();
\t\tpchar.version_number = 104;
\t\tSaveEngineState(saveName);
\t\tISetSaveData(saveName,saveData);
\t}

\t//hide interface
\tPostEvent("DoInfoShower",1,"sl","save game",false);
\tDeleteEntitiesByType("scrshoter");
\tHideQuickSaveMenu();
}
''',
                '''void SaveGameToFile(string saveName, string saveData)
{
\taref arTmp;
\tif( !GetEntity(arTmp,"fader") )
\t{
\t\t//implemet interface
\t\t//LaunchQuickSaveMenu();
\t\tpchar.version_number = 104;
\t\tSaveEngineState(saveName);
\t\tISetSaveData(saveName,saveData);
\t}

\t//hide interface
\tPostEvent("DoInfoShower",1,"sl","save game",false);
\tDeleteEntitiesByType("scrshoter");
\tHideQuickSaveMenu();
}

void SaveGame()
{
\tstring saveName = GetEventData();
\tstring saveData = GetEventData();
\tDelEventHandler("evntSave","SaveGame");
\tSaveGameToFile(saveName, saveData);
}

void SaveAutoGame()
{
\tstring saveName = GetEventData();
\tstring saveData = GetEventData();
\tDelEventHandler("evntSave","SaveAutoGame");
\tSaveGameToFile(saveName, saveData);
\tPostEvent("evntAutoSaveFinished",0);
}
''',
            ),),
        ),
    ),
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return PATCH.print_status(root)
        result = PATCH.apply(root) if args.action == "apply" else PATCH.revert(root)
        print(result)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

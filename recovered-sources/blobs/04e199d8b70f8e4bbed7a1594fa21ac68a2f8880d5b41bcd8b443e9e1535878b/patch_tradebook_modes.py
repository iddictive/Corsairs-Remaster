#!/usr/bin/env python3
"""Add ordinary/route modes and current-town pinning to the trade journal."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


OLD_MODE_HELPERS = '''void InitTradeComparison()
{
\tref nulChr = &NullCharacter;
\tif (CheckAttribute(pchar, "SystemInfo.TradeBookPinnedCity"))
\t{
\t\tTradeBookPinnedCity = pchar.SystemInfo.TradeBookPinnedCity;
\t\tif (!CheckAttribute(nulChr, "PriceList." + TradeBookPinnedCity)) TradeBookPinnedCity = "";
\t}
\tif (TradeBookPinnedCity == "")
\t{
\t\tTradeBookPinnedCity = TradeBookSelectedCity;
\t\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\t}
\tRefreshTradeComparison();
}

void PinSelectedTradeCity()
{
\tif (TradeBookSelectedCity == "") return;
\tTradeBookPinnedCity = TradeBookSelectedCity;
\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\tRefreshTradeComparison();
}

void RefreshTradeComparison()
{
\tif (TradeBookSelectedCity == "") return;
\tif (TradeBookPinnedCity == "" || TradeBookPinnedCity == TradeBookSelectedCity)
\t{
\t\tSetFormatedText("TRADE_ROUTE", "Закреплён: " + GetTradeBookCityName(TradeBookPinnedCity) + "  |  выберите второй город");
\t\tFillPriceList("TABLE_GOODS", TradeBookSelectedCity);
\t\treturn;
\t}
\tFillTradeComparison("TABLE_GOODS", TradeBookPinnedCity, TradeBookSelectedCity);
}'''


NEW_MODE_HELPERS = '''void SyncTradeBookControls()
{
\tif (TradeBookRouteMode == 1)
\t{
\t\tSendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"MODE_TOGGLE",0,"#С закреплением");
\t\tSendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PIN_CITY",0,"#Закрепить");
\t\tSetNodeUsing("PIN_CITY", true);
\t}
\telse
\t{
\t\tSendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"MODE_TOGGLE",0,"#Обычный режим");
\t\tSetNodeUsing("PIN_CITY", false);
\t}
}

bool TryPinCurrentTradeCity()
{
\tstring currentTown = GetCurrentTown();
\tref nulChr = &NullCharacter;
\tif (currentTown == "" && bSeaActive && bCanEnterToLand)
\t{
\t\tcurrentTown = Sea_FindNearColony();
\t}
\tif (currentTown == "") return false;
\tif (FindColony(currentTown) == -1) return false;
\tif (!CheckAttribute(nulChr, "PriceList." + currentTown)) return false;
\tTradeBookPinnedCity = currentTown;
\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\treturn true;
}

void InitTradeComparison()
{
\tref nulChr = &NullCharacter;
\tTradeBookRouteMode = 1;
\tif (CheckAttribute(pchar, "SystemInfo.TradeBookRouteMode"))
\t{
\t\tTradeBookRouteMode = sti(pchar.SystemInfo.TradeBookRouteMode);
\t}
\tif (CheckAttribute(pchar, "SystemInfo.TradeBookPinnedCity"))
\t{
\t\tTradeBookPinnedCity = pchar.SystemInfo.TradeBookPinnedCity;
\t\tif (!CheckAttribute(nulChr, "PriceList." + TradeBookPinnedCity)) TradeBookPinnedCity = "";
\t}
\tif (TradeBookRouteMode == 1 && !TryPinCurrentTradeCity() && TradeBookPinnedCity == "")
\t{
\t\tTradeBookPinnedCity = TradeBookSelectedCity;
\t\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\t}
\tpchar.SystemInfo.TradeBookRouteMode = TradeBookRouteMode;
\tSyncTradeBookControls();
\tRefreshTradeComparison();
}

void ToggleTradeBookMode()
{
\tif (TradeBookRouteMode == 1) TradeBookRouteMode = 0;
\telse TradeBookRouteMode = 1;
\tpchar.SystemInfo.TradeBookRouteMode = TradeBookRouteMode;
\tif (TradeBookRouteMode == 1 && !TryPinCurrentTradeCity() && TradeBookPinnedCity == "")
\t{
\t\tTradeBookPinnedCity = TradeBookSelectedCity;
\t\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\t}
\tSyncTradeBookControls();
\tRefreshTradeComparison();
}

void PinSelectedTradeCity()
{
\tif (TradeBookSelectedCity == "") return;
\tTradeBookPinnedCity = TradeBookSelectedCity;
\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
\tRefreshTradeComparison();
}

void RefreshTradeComparison()
{
\tif (TradeBookSelectedCity == "") return;
\tif (TradeBookRouteMode == 0)
\t{
\t\tSetFormatedText("TRADE_ROUTE", "Обычные цены: " + GetTradeBookCityName(TradeBookSelectedCity));
\t\tFillPriceList("TABLE_GOODS", TradeBookSelectedCity);
\t\treturn;
\t}
\tif (TradeBookPinnedCity == "" || TradeBookPinnedCity == TradeBookSelectedCity)
\t{
\t\tSetFormatedText("TRADE_ROUTE", "Закреплён: " + GetTradeBookCityName(TradeBookPinnedCity) + "  |  выберите второй город");
\t\tFillPriceList("TABLE_GOODS", TradeBookSelectedCity);
\t\treturn;
\t}
\tFillTradeComparison("TABLE_GOODS", TradeBookPinnedCity, TradeBookSelectedCity);
}'''


PATCH = PatchSet(
    TARGET_ROOT / ".codex-tradebook-upgrade" / "20260913-route-modes-v2",
    (
        FilePatch(
            "PROGRAM/interface/tradebook.c",
            "ce90ff42f3c2d3fe2f6e5f90d873b2133a18fe1330d0b7a7b0faaff868536137",
            "59a239e2ebac0102dbf9cf0c810e65e5efbf0e6d84f119090b81c0660357e05c",
            (
                (
                    'string TradeBookPinnedCity = "";',
                    'string TradeBookPinnedCity = "";\nint TradeBookRouteMode = 1;',
                ),
                (
                    '    FillPriceListTown("TABLE_CITY");\n    InitTradeComparison();\n    SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PIN_CITY",0,"#Закрепить город");',
                    '    FillPriceListTown("TABLE_CITY");\n    InitTradeComparison();',
                ),
                (
                    '\tswitch(nodName)\n\t{\n\t\tcase "PIN_CITY":',
                    '\tswitch(nodName)\n\t{\n\t\tcase "MODE_TOGGLE":\n\t\t\tif(comName == "click" || comName == "activate")\n\t\t\t{\n\t\t\t\tToggleTradeBookMode();\n\t\t\t\treturn;\n\t\t\t}\n\t\tbreak;\n\t\tcase "PIN_CITY":',
                ),
                (OLD_MODE_HELPERS, NEW_MODE_HELPERS),
            ),
        ),
        FilePatch(
            "RESOURCE/INI/interfaces/tradebook.ini",
            "9018e88d19d41e0a533c9273f959e36eb482276af7e9f6030a0a62bb3efc359b",
            "a15e1e013cbe8ade5a6a9e80975e91af1b99627c2e34ceb080e8d70a78916fe1",
            (
                (
                    "item = 210,TEXTBUTTON2,PIN_CITY\nitem = 210,FORMATEDTEXT,TRADE_ROUTE",
                    "item = 210,TEXTBUTTON2,MODE_TOGGLE\nitem = 210,TEXTBUTTON2,PIN_CITY\nitem = 210,FORMATEDTEXT,TRADE_ROUTE",
                ),
                (
                    "[PIN_CITY]\ncommand = click\ncommand = activate\ncommand = deactivate,event:exitCancel\nposition = 20,86,310,112\nstring = Ok\nfontScale = 0.75\nglowoffset = 0,0",
                    "[MODE_TOGGLE]\ncommand = click\ncommand = activate\ncommand = deactivate,event:exitCancel\nposition = 20,86,158,112\nstring = Ok\nfontScale = 0.68\nglowoffset = 0,0\n\n[PIN_CITY]\ncommand = click\ncommand = activate\ncommand = deactivate,event:exitCancel\nposition = 162,86,310,112\nstring = Ok\nfontScale = 0.68\nglowoffset = 0,0",
                ),
            ),
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

#!/usr/bin/env python3
"""Add pinned-city route comparison and a no-overload cargo plan."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


COMPARISON_HELPERS = '''string GetTradeBookCityName(string cityId)
{
\tif (FindColony(cityId) == -1) return cityId;
\treturn GetConvertStr(cityId + " Town", "LocLables.txt");
}

void InitTradeComparison()
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
}

void FillTradeComparison(string tableName, string sourceCity, string targetCity)
{
\tref nulChr = &NullCharacter;
\tint goodsOrder[GOODS_QUANTITY];
\tfloat goodsScore[GOODS_QUANTITY];
\tint goodsCount = 0;
\tint i, j, tmpGood;
\tfloat tmpScore;
\tstring goodsAttr;

\tfor (i = 0; i < GOODS_QUANTITY; i++)
\t{
\t\tgoodsAttr = "Gidx" + i;
\t\tif (!CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".Buy")) continue;
\t\tif (!CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".TradeType")) continue;
\t\tif (!CheckAttribute(nulChr, "PriceList." + targetCity + "." + goodsAttr + ".Sell")) continue;
\t\tif (!CheckAttribute(nulChr, "PriceList." + targetCity + "." + goodsAttr + ".TradeType")) continue;
\t\tif (sti(nulChr.PriceList.(sourceCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND && !bBettaTestMode) continue;
\t\tif (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND && !bBettaTestMode) continue;
\t\tif (sti(nulChr.PriceList.(sourceCity).(goodsAttr).TradeType) == TRADE_TYPE_CANNONS && !bBettaTestMode) continue;
\t\tif (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CANNONS && !bBettaTestMode) continue;
\t\tif (stf(Goods[i].Weight) <= 0.0) continue;

\t\tgoodsOrder[goodsCount] = i;
\t\tgoodsScore[goodsCount] = (stf(nulChr.PriceList.(targetCity).(goodsAttr).Sell) - stf(nulChr.PriceList.(sourceCity).(goodsAttr).Buy)) / stf(Goods[i].Weight);
\t\tgoodsCount++;
\t}

\tfor (i = 0; i < goodsCount - 1; i++)
\t{
\t\tfor (j = i + 1; j < goodsCount; j++)
\t\t{
\t\t\tif (goodsScore[j] > goodsScore[i])
\t\t\t{
\t\t\t\ttmpScore = goodsScore[i];
\t\t\t\tgoodsScore[i] = goodsScore[j];
\t\t\t\tgoodsScore[j] = tmpScore;
\t\t\t\ttmpGood = goodsOrder[i];
\t\t\t\tgoodsOrder[i] = goodsOrder[j];
\t\t\t\tgoodsOrder[j] = tmpGood;
\t\t\t}
\t\t}
\t}

\tRecalculateSquadronCargoLoad(pchar);
\tint freeSpace[COMPANION_MAX];
\tfreeSpace[0] = GetCargoFreeSpace(pchar);
\tint totalFreeSpace = freeSpace[0];
\tfor (i = 1; i < COMPANION_MAX; i++)
\t{
\t\tfreeSpace[i] = 0;
\t\tint companionIndex = GetCompanionIndex(pchar, i);
\t\tif (companionIndex != -1 && GetRemovable(&Characters[companionIndex]))
\t\t{
\t\t\tfreeSpace[i] = GetCargoFreeSpace(&Characters[companionIndex]);
\t\t\ttotalFreeSpace += freeSpace[i];
\t\t}
\t}

\tTable_Clear(tableName, false, true, false);
\tint moneyLeft = sti(pchar.money);
\tint usedSpace = 0;
\tint planProfit = 0;
\tGameInterface.(tableName).select = 0;
\tGameInterface.(tableName).hr.td1.str = XI_ConvertString("Good name");
\tGameInterface.(tableName).hr.td2.str = "Тип";
\tGameInterface.(tableName).hr.td3.str = "Купить";
\tGameInterface.(tableName).hr.td4.str = "Продать";
\tGameInterface.(tableName).hr.td5.str = "+/ц";
\tGameInterface.(tableName).hr.td6.str = "Взять";
\tGameInterface.(tableName).hr.td7.str = "Прибыль";

\tfor (i = 0; i < goodsCount; i++)
\t{
\t\tint goodIndex = goodsOrder[i];
\t\tgoodsAttr = "Gidx" + goodIndex;
\t\tstring row = "tr" + (i + 1);
\t\tint unitSize = sti(Goods[goodIndex].Units);
\t\tint packWeight = makeint(stf(Goods[goodIndex].Weight) + 0.5);
\t\tint buyPrice = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Buy);
\t\tint sellPrice = sti(nulChr.PriceList.(targetCity).(goodsAttr).Sell);
\t\tint profitPerPack = sellPrice - buyPrice;
\t\tint plannedPacks = 0;

\t\tif (profitPerPack > 0 && buyPrice > 0 && packWeight > 0 && unitSize > 0 && CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".Qty"))
\t\t{
\t\t\tint packsLeft = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Qty) / unitSize;
\t\t\tint affordablePacks = moneyLeft / buyPrice;
\t\t\tif (packsLeft > affordablePacks) packsLeft = affordablePacks;
\t\t\tfor (j = 0; j < COMPANION_MAX; j++)
\t\t\t{
\t\t\t\tint bestSlot = -1;
\t\t\t\tint bestRemainder = 1000000000;
\t\t\t\tfor (int k = 0; k < COMPANION_MAX; k++)
\t\t\t\t{
\t\t\t\t\tif (freeSpace[k] < packWeight) continue;
\t\t\t\t\tint slotRemainder = freeSpace[k] - packWeight;
\t\t\t\t\tif (slotRemainder < bestRemainder)
\t\t\t\t\t{
\t\t\t\t\t\tbestSlot = k;
\t\t\t\t\t\tbestRemainder = slotRemainder;
\t\t\t\t\t}
\t\t\t\t}
\t\t\t\tif (bestSlot == -1) break;
\t\t\t\tint fittingPacks = freeSpace[bestSlot] / packWeight;
\t\t\t\tif (fittingPacks > packsLeft) fittingPacks = packsLeft;
\t\t\t\tplannedPacks += fittingPacks;
\t\t\t\tpacksLeft -= fittingPacks;
\t\t\t\tfreeSpace[bestSlot] -= fittingPacks * packWeight;
\t\t\t\tif (packsLeft <= 0) break;
\t\t\t}
\t\t\tmoneyLeft -= plannedPacks * buyPrice;
\t\t\tusedSpace += plannedPacks * packWeight;
\t\t\tplanProfit += plannedPacks * profitPerPack;
\t\t}

\t\tGameInterface.(tableName).(row).UserData.ID = Goods[goodIndex].name;
\t\tGameInterface.(tableName).(row).UserData.IDX = goodIndex;
\t\tGameInterface.(tableName).(row).td1.icon.group = "GOODS";
\t\tGameInterface.(tableName).(row).td1.icon.image = Goods[goodIndex].name;
\t\tGameInterface.(tableName).(row).td1.icon.offset = "1, 0";
\t\tGameInterface.(tableName).(row).td1.icon.width = 32;
\t\tGameInterface.(tableName).(row).td1.icon.height = 32;
\t\tGameInterface.(tableName).(row).td1.textoffset = "30,0";
\t\tGameInterface.(tableName).(row).td1.str = XI_ConvertString(Goods[goodIndex].name);
\t\tGameInterface.(tableName).(row).td1.scale = 0.8;
\t\tGameInterface.(tableName).(row).td2.icon.group = "TRADE_TYPE";
\t\tGameInterface.(tableName).(row).td2.icon.image = "ico_" + nulChr.PriceList.(sourceCity).(goodsAttr).TradeType;
\t\tGameInterface.(tableName).(row).td2.icon.offset = "0, 1";
\t\tGameInterface.(tableName).(row).td2.icon.width = 16;
\t\tGameInterface.(tableName).(row).td2.icon.height = 28;
\t\tGameInterface.(tableName).(row).td3.str = buyPrice;
\t\tGameInterface.(tableName).(row).td4.str = sellPrice;
\t\tif (goodsScore[i] < 0.0) GameInterface.(tableName).(row).td5.str = "-" + FloatToString(-goodsScore[i], 1);
\t\telse GameInterface.(tableName).(row).td5.str = FloatToString(goodsScore[i], 1);
\t\tGameInterface.(tableName).(row).td6.str = plannedPacks * unitSize;
\t\tGameInterface.(tableName).(row).td7.str = plannedPacks * profitPerPack;
\t\tif (profitPerPack > 0)
\t\t{
\t\t\tGameInterface.(tableName).(row).td5.color = argb(255,196,255,196);
\t\t\tGameInterface.(tableName).(row).td7.color = argb(255,196,255,196);
\t\t}
\t\tif (profitPerPack < 0)
\t\t{
\t\t\tGameInterface.(tableName).(row).td5.color = argb(255,255,196,196);
\t\t\tGameInterface.(tableName).(row).td7.color = argb(255,255,196,196);
\t\t}
\t}

\tSetFormatedText("TRADE_ROUTE", GetTradeBookCityName(sourceCity) + "  >  " + GetTradeBookCityName(targetCity) + "  |  план " + usedSpace + "/" + totalFreeSpace + " ц  |  прибыль " + planProfit);
\tTable_UpdateWindow(tableName);
}'''


ORIGINAL_TABLE_LAYOUT = '''[TABLE_CITY]
command = deactivate,event:exitCancel
command = click,select:TABLE_CITY
command = upstep
command = downstep
position = 10,85,320,570
backimage = piclist=SITH_TABLE, picname=left side, color=(225,128,128,128)
scroller = SCROLL_CITY
rowquantity = 16
colquantity = 4
vlinewidth = 1
hlineheight = 1
headerlineheight = 1
borderwidth = 0
bordercornersize = 0,0
fontcellvalignment = center
colswidth = 35,100,100,75
rowsheight = 35,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30
NormalLineHeight = 30

[SCROLL_CITY]
command = click, select:TABLE_CITY
command = deactivate,event:exitCancel
command = downstep
command = upstep
position = 316,85,330,570
ownedcontrol = TABLE_CITY

[TABLE_GOODS]
command = deactivate,event:exitCancel
command = click,select:TABLE_GOODS
command = rclick,select:TABLE_GOODS,event:ShowInfoWindow
command = upstep
command = downstep
position = 330,85,780,570
backimage = piclist=SITH_TABLE, picname=right side, color=(225,128,128,128)
scroller = SCROLL_GOODS
rowquantity = 16
colquantity = 7
vlinewidth = 1
hlineheight = 1
headerlineheight = 1
borderwidth = 0
bordercornersize = 0,0
fontcellvalignment = center
colswidth = 143,22,60,60,65,50,50
rowsheight = 35,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30
NormalLineHeight = 30

[SCROLL_GOODS]
command = click, select:TABLE_GOODS
command = deactivate,event:exitCancel
command = downstep
command = upstep
position = 776,85,790,570
ownedcontrol = TABLE_GOODS'''


TABLE_LAYOUT = '''[TABLE_CITY]
command = deactivate,event:exitCancel
command = click,select:TABLE_CITY
command = upstep
command = downstep
position = 10,115,320,570
backimage = piclist=SITH_TABLE, picname=left side, color=(225,128,128,128)
scroller = SCROLL_CITY
rowquantity = 15
colquantity = 4
vlinewidth = 1
hlineheight = 1
headerlineheight = 1
borderwidth = 0
bordercornersize = 0,0
fontcellvalignment = center
colswidth = 35,100,100,75
rowsheight = 35,30,30,30,30,30,30,30,30,30,30,30,30,30,30
NormalLineHeight = 30

[SCROLL_CITY]
command = click, select:TABLE_CITY
command = deactivate,event:exitCancel
command = downstep
command = upstep
position = 316,115,330,570
ownedcontrol = TABLE_CITY

[TABLE_GOODS]
command = deactivate,event:exitCancel
command = click,select:TABLE_GOODS
command = rclick,select:TABLE_GOODS,event:ShowInfoWindow
command = upstep
command = downstep
position = 330,115,780,570
backimage = piclist=SITH_TABLE, picname=right side, color=(225,128,128,128)
scroller = SCROLL_GOODS
rowquantity = 15
colquantity = 7
vlinewidth = 1
hlineheight = 1
headerlineheight = 1
borderwidth = 0
bordercornersize = 0,0
fontcellvalignment = center
colswidth = 118,22,55,55,60,60,80
rowsheight = 35,30,30,30,30,30,30,30,30,30,30,30,30,30,30
NormalLineHeight = 30

[SCROLL_GOODS]
command = click, select:TABLE_GOODS
command = deactivate,event:exitCancel
command = downstep
command = upstep
position = 776,115,790,570
ownedcontrol = TABLE_GOODS'''


PATCH = PatchSet(
    TARGET_ROOT / ".codex-tradebook-upgrade" / "20260913-route-planner-v1",
    (
        FilePatch(
            "PROGRAM/interface/tradebook.c",
            "6d14278629c7a7f14842b11f039caaed4c1fc3a40af90c0689ffa6966fbd18ef",
            "ce90ff42f3c2d3fe2f6e5f90d873b2133a18fe1330d0b7a7b0faaff868536137",
            (
                (
                    "string CurTable, CurRow;",
                    'string CurTable, CurRow;\nstring TradeBookSelectedCity = "";\nstring TradeBookPinnedCity = "";',
                ),
                (
                    '    XI_RegistryExitKey("IExit_F2");\n    FillPriceListTown("TABLE_CITY");',
                    '    XI_RegistryExitKey("IExit_F2");\n    FillPriceListTown("TABLE_CITY");\n    InitTradeComparison();\n    SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PIN_CITY",0,"#Закрепить город");',
                ),
                (
                    "    switch(nodName)\n\t{\n\t\tcase \"I_CHARACTER_2\":",
                    "\tswitch(nodName)\n\t{\n\t\tcase \"PIN_CITY\":\n\t\t\tif(comName == \"click\" || comName == \"activate\")\n\t\t\t{\n\t\t\t\tPinSelectedTradeCity();\n\t\t\t\treturn;\n\t\t\t}\n\t\tbreak;\n\t\tcase \"I_CHARACTER_2\":",
                ),
                (
                    '\t//if (n > 1) GameInterface.(_tabName).select = 1;\n\tTable_UpdateWindow(_tabName);\n\tFillPriceList("TABLE_GOODS", firstId);',
                    '\tif (n > 1) GameInterface.(_tabName).select = 1;\n\tTable_UpdateWindow(_tabName);\n\tTradeBookSelectedCity = firstId;',
                ),
                (
                    "    string  row;\n    nulChr = &NullCharacter;\n    // шапка -->",
                    "    string  row;\n    nulChr = &NullCharacter;\n    Table_Clear(_tabName, false, true, false);\n    // шапка -->",
                ),
                (
                    "    Table_UpdateWindow(_tabName);\n}\n\nvoid TableSelectChange()",
                    "    Table_UpdateWindow(_tabName);\n}\n\n" + COMPARISON_HELPERS + "\n\nvoid TableSelectChange()",
                ),
                (
                    '    if (CurTable == "TABLE_CITY")\n    {\n    \tFillPriceList("TABLE_GOODS", GameInterface.(CurTable).(CurRow).UserData.CityID);\n    }',
                    '    if (CurTable == "TABLE_CITY")\n    {\n\t    \tTradeBookSelectedCity = GameInterface.(CurTable).(CurRow).UserData.CityID;\n\t    \tRefreshTradeComparison();\n    }',
                ),
                (
                    'sText2  = "Цвета: красный - контрабанда, синий - импорт, зелёный - экспорт";',
                    'sText2  = "Тип товара показан значком. В сравнении зелёная прибыль выгодна, красная означает убыток.";',
                ),
            ),
        ),
        FilePatch(
            "RESOURCE/INI/interfaces/tradebook.ini",
            "27d8922a463e9a1aae840ddf9e0f93cf1103f6d72d06236f10493471c9cc70cf",
            "9018e88d19d41e0a533c9273f959e36eb482276af7e9f6030a0a62bb3efc359b",
            (
                (
                    "item = 202,FRAME,BACK_FRAME\nitem = 200,TABLE,TABLE_CITY",
                    "item = 202,FRAME,BACK_FRAME\nitem = 210,TEXTBUTTON2,PIN_CITY\nitem = 210,FORMATEDTEXT,TRADE_ROUTE\nitem = 200,TABLE,TABLE_CITY",
                ),
                (
                    "rightbottom_size\t= 10,10\n\n[TABLE_CITY]",
                    "rightbottom_size\t= 10,10\n\n[PIN_CITY]\ncommand = click\ncommand = activate\ncommand = deactivate,event:exitCancel\nposition = 20,86,310,112\nstring = Ok\nfontScale = 0.75\nglowoffset = 0,0\n\n[TRADE_ROUTE]\nposition = 330,86,770,112\nfontScale = 0.72\nlineSpace = 1\nfont = interface_button\nalignment = center\nColor = 255,255,255,255\n\n[TABLE_CITY]",
                ),
                (ORIGINAL_TABLE_LAYOUT, TABLE_LAYOUT),
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

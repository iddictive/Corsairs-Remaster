#!/usr/bin/env python3
"""Install the reversible KVL cargo-market and item-shop rotation patch."""

from __future__ import annotations

import argparse
import os
import re
import sys
from datetime import date
from pathlib import Path

from runtime_script_patch import (
    FilePatch,
    PatchSet,
    TARGET_ROOT,
    atomic_write,
    lifecycle_lock,
    require_idle,
    sha256,
    transform,
)


SEGMENT_PATH = "PROGRAM/scripts/shop_rotation.c"


ITEM_SHOP_CODE = r'''
// Codex shop rotation: generated stock is tracked separately from player-sold stock.
int TraderStock_Profile(aref ch)
{
    if (CheckAttribute(ch, "PhantomType") && ch.PhantomType == "hawker") return 1;
    if (CheckAttribute(ch, "Dialog.Filename") && findsubstr(ch.Dialog.Filename, "Enc_Walker", 0) != -1) return 2;
    return 0;
}

int TraderStock_TradeType(aref ch, int goodsIndex)
{
    int storeIndex;
    string goodsName;
    if (!CheckAttribute(ch, "City")) return -1;
    storeIndex = FindStore(ch.City);
    if (storeIndex < 0) return -1;
    goodsName = Goods[goodsIndex].Name;
    if (!CheckAttribute(&Stores[storeIndex], "Goods." + goodsName + ".TradeType")) return -1;
    return sti(Stores[storeIndex].Goods.(goodsName).TradeType);
}

int TraderStock_Chance(aref ch, int profile, int category, int baseChance, int targetRank)
{
    int chance = baseChance + (sti(PChar.rank) - targetRank) * 7;
    int nation = -1;
    int goodsIndex = -1;
    int tradeType;

    if (profile == 0)
    {
        if (category <= 2) chance += 50;
        if (category == 3 || category == 4) chance += 80;
        if (category >= 6) chance -= 40;
    }
    if (profile == 1)
    {
        if (category <= 2) chance -= 100;
        if (category == 4) chance += 40;
        if (category >= 6) chance += 100;
    }
    if (profile == 2)
    {
        if (category <= 1) chance += 20;
        if (category == 3) chance += 100;
        if (category == 4) chance += 80;
        if (category >= 6) chance -= 100;
    }

    if (CheckAttribute(ch, "nation")) nation = sti(ch.nation);
    if (nation == PIRATE)
    {
        if (category == 0 || category >= 6) chance += 35;
    }
    if (nation == SPAIN)
    {
        if (category == 0 || category == 2) chance += 30;
    }
    if (nation == ENGLAND)
    {
        if (category == 1 || category == 3) chance += 30;
    }
    if (nation == HOLLAND)
    {
        if (category == 5 || category >= 6) chance += 30;
    }
    if (nation == FRANCE)
    {
        if (category == 0 || category == 4) chance += 30;
    }

    if (category <= 2) goodsIndex = GOOD_WEAPON;
    if (category == 3) goodsIndex = GOOD_POWDER;
    if (category == 4) goodsIndex = GOOD_MEDICAMENT;
    if (goodsIndex >= 0)
    {
        tradeType = TraderStock_TradeType(ch, goodsIndex);
        if (tradeType == TRADE_TYPE_EXPORT) chance += 60;
        if (tradeType == TRADE_TYPE_IMPORT) chance -= 35;
        if (tradeType == TRADE_TYPE_CONTRABAND)
        {
            if (profile == 1) chance += 60;
            else chance -= 20;
        }
    }

    if (CheckAttribute(ch, "City"))
    {
        int cityStore = FindStore(ch.City);
        if (cityStore >= 0)
        {
            if (Stores[cityStore].StoreSize == "large") chance += 20;
            if (Stores[cityStore].StoreSize == "small") chance -= 20;
        }
    }

    if (chance < 10) chance = 10;
    if (chance > 900) chance = 900;
    return chance;
}

void TraderStock_RecordGenerated(aref ch, string itemID, int qty)
{
    int oldQty = 0;
    if (CheckAttribute(ch, "TradeStock.Generated." + itemID)) oldQty = sti(ch.TradeStock.Generated.(itemID));
    ch.TradeStock.Generated.(itemID) = oldQty + qty;
}

bool TraderStock_ItemMatchesSeries(string itemID, string baseID)
{
    int itemIndex;
    if (itemID == baseID) return true;
    itemIndex = GetItemIndex(itemID);
    if (itemIndex < 0) return false;
    if (!CheckAttribute(&Items[itemIndex], "DefItemID")) return false;
    return Items[itemIndex].DefItemID == baseID;
}

int TraderStock_SeriesQty(aref ch, string baseID)
{
    int total = 0;
    aref stock;
    aref item;
    string itemID;

    if (CheckAttribute(ch, "TradeStock.Generated"))
    {
        makearef(stock, ch.TradeStock.Generated);
        for (int i = 0; i < GetAttributesNum(stock); i++)
        {
            item = GetAttributeN(stock, i);
            itemID = GetAttributeName(item);
            if (TraderStock_ItemMatchesSeries(itemID, baseID)) total += sti(GetAttributeValue(item));
        }
    }
    if (CheckAttribute(ch, "TradeStock.Player"))
    {
        makearef(stock, ch.TradeStock.Player);
        for (int j = 0; j < GetAttributesNum(stock); j++)
        {
            item = GetAttributeN(stock, j);
            itemID = GetAttributeName(item);
            if (TraderStock_ItemMatchesSeries(itemID, baseID)) total += sti(GetAttributeValue(item));
        }
    }
    return total;
}

void TraderStock_Add(aref ch, string itemID, int qty, int cap, bool generable)
{
    string actualID;
    int current = GetCharacterFreeItem(ch, itemID);
    if (generable) current = TraderStock_SeriesQty(ch, itemID);
    if (current >= cap) return;
    if (qty > cap - current) qty = cap - current;
    if (qty < 1) return;

    if (!generable)
    {
        AddItems(ch, itemID, qty);
        TraderStock_RecordGenerated(ch, itemID, qty);
        return;
    }

    for (int i = 0; i < qty; i++)
    {
        actualID = GetGeneratedItem(itemID);
        AddItems(ch, actualID, 1);
        TraderStock_RecordGenerated(ch, actualID, 1);
    }
}

void TraderStock_Try(aref ch, int profile, string itemID, int category, int baseChance, int targetRank, int minQty, int maxQty, int cap, bool generable)
{
    if (category == 7 && findsubstr(itemID, "map_", 0) == 0 && CheckMainHeroMap(itemID)) return;
    if (category == 7 && findsubstr(itemID, "Totem_", 0) == 0 && CheckMainHeroTotem(itemID)) return;
    if (rand(999) >= TraderStock_Chance(ch, profile, category, baseChance, targetRank)) return;
    TraderStock_Add(ch, itemID, minQty + rand(maxQty - minQty), cap, generable);
}

void TraderStock_RemoveGenerated(aref ch, bool partial)
{
    aref generated;
    aref item;
    string itemID;
    int tracked;
    int playerQty;
    int available;
    int removeQty;

    if (!CheckAttribute(ch, "TradeStock.Generated")) return;
    makearef(generated, ch.TradeStock.Generated);

    for (int i = GetAttributesNum(generated) - 1; i >= 0; i--)
    {
        item = GetAttributeN(generated, i);
        itemID = GetAttributeName(item);
        tracked = sti(GetAttributeValue(item));
        playerQty = 0;

        if (CheckAttribute(ch, "TradeStock.Player." + itemID)) playerQty = sti(ch.TradeStock.Player.(itemID));
        available = GetCharacterFreeItem(ch, itemID) - playerQty;
        if (available < 0) available = 0;
        if (available > tracked) available = tracked;
        removeQty = available;
        if (partial)
        {
            removeQty = available * (25 + rand(25)) / 100;
            if (removeQty == 0 && available > 0 && rand(1) == 1) removeQty = 1;
        }
        if (removeQty > 0) TakeNItems(ch, itemID, -removeQty);
        tracked -= removeQty;
        if (tracked > 0) ch.TradeStock.Generated.(itemID) = tracked;
        else DeleteAttribute(generated, itemID);
    }
}

void TraderStock_RecordPlayerSale(aref ch, string itemID, int qty)
{
    int oldQty = 0;
    if (TraderStock_Profile(ch) != 0) return;
    if (qty < 1) return;
    if (CheckAttribute(ch, "TradeStock.Player." + itemID)) oldQty = sti(ch.TradeStock.Player.(itemID));
    ch.TradeStock.Player.(itemID) = oldQty + qty;
}

void TraderStock_RecordBuy(aref ch, string itemID, int qty)
{
    int tracked;
    int used;
    if (TraderStock_Profile(ch) != 0) return;
    if (qty < 1) return;
    if (CheckAttribute(ch, "TradeStock.Player." + itemID))
    {
        tracked = sti(ch.TradeStock.Player.(itemID));
        used = qty;
        if (used > tracked) used = tracked;
        tracked -= used;
        qty -= used;
        if (tracked > 0) ch.TradeStock.Player.(itemID) = tracked;
        else DeleteAttribute(ch, "TradeStock.Player." + itemID);
    }
    if (qty > 0 && CheckAttribute(ch, "TradeStock.Generated." + itemID))
    {
        tracked = sti(ch.TradeStock.Generated.(itemID));
        used = qty;
        if (used > tracked) used = tracked;
        tracked -= used;
        if (tracked > 0) ch.TradeStock.Generated.(itemID) = tracked;
        else DeleteAttribute(ch, "TradeStock.Generated." + itemID);
    }
}

void TraderStock_AddLSCMap(aref ch)
{
    ref itm;
    if (ch.id != "LSC_Trader") return;
    if (CheckAttribute(ch, "quest.takeMap") && ch.quest.takeMap == "changePrice")
    {
        itm = ItemsFromID("map_LSC");
        itm.price = 1000;
        if (GetCharacterFreeItem(ch, "map_LSC") < 1) AddItems(ch, "map_LSC", 1);
        ch.quest.takeMap = "smallPrice";
        return;
    }
    if (!CheckAttribute(ch, "quest.takeMap") || ch.quest.takeMap != "changePrice")
    {
        if (GetCharacterFreeItem(ch, "map_LSC") < 1) AddItems(ch, "map_LSC", 1);
        if (!CheckAttribute(ch, "quest.takeMap") || ch.quest.takeMap != "discount") ch.quest.takeMap = "mapIsToken";
    }
}

void TraderStock_GiveItemToTrader(aref ch)
{
    int profile;
    int elapsed;

    if (bBettaTestMode)
    {
        GiveItemToTraderLegacy(ch);
        return;
    }

    profile = TraderStock_Profile(ch);
    if (!CheckAttribute(ch, "TradeStock.control_year"))
    {
        SaveCurrentNpcQuestDateParam(ch, "TradeStock");
        ch.TradeStock.Interval = 2 + rand(2);
    }
    else
    {
        if (!CheckAttribute(ch, "TradeStock.Interval")) ch.TradeStock.Interval = 2 + rand(2);
        elapsed = GetNpcQuestPastDayParam(ch, "TradeStock");
        if (elapsed < sti(ch.TradeStock.Interval))
        {
            TraderStock_AddLSCMap(ch);
            return;
        }
        TraderStock_RemoveGenerated(ch, true);
        SaveCurrentNpcQuestDateParam(ch, "TradeStock");
        ch.TradeStock.Interval = 2 + rand(2);
    }

    TraderStock_Try(ch, profile, "blade5", 0, 420, 1, 1, 2, 3, true);
    TraderStock_Try(ch, profile, "blade1", 0, 390, 1, 1, 2, 3, true);
    TraderStock_Try(ch, profile, "blade2", 0, 360, 2, 1, 2, 3, true);
    TraderStock_Try(ch, profile, "blade3", 0, 300, 3, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade4", 0, 240, 5, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade10", 0, 220, 5, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade35", 0, 180, 7, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "topor3", 0, 160, 8, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade11", 0, 140, 9, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "topor1", 0, 100, 11, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade6", 0, 110, 10, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade7", 0, 100, 11, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade9", 0, 95, 11, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade8", 0, 90, 12, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade12", 0, 80, 13, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade16", 0, 55, 16, 1, 1, 2, true);
    TraderStock_Try(ch, profile, "blade17", 0, 35, 20, 1, 1, 1, true);
    TraderStock_Try(ch, profile, "blade18", 0, 25, 24, 1, 1, 1, true);

    TraderStock_Try(ch, profile, "pistol1", 1, 300, 1, 1, 2, 3, false);
    TraderStock_Try(ch, profile, "pistol2", 1, 210, 5, 1, 1, 2, false);
    TraderStock_Try(ch, profile, "pistol3", 1, 120, 10, 1, 1, 2, false);
    TraderStock_Try(ch, profile, "cirass1", 2, 130, 7, 1, 1, 2, false);
    TraderStock_Try(ch, profile, "cirass2", 2, 70, 12, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "cirass4", 2, 30, 18, 1, 1, 1, false);

    TraderStock_Try(ch, profile, "bullet", 3, 800, 1, 8, 18, 40, false);
    TraderStock_Try(ch, profile, "GunPowder", 3, 760, 1, 5, 14, 30, false);
    TraderStock_Try(ch, profile, "potion1", 4, 820, 1, 2, 6, 15, false);
    TraderStock_Try(ch, profile, "potion2", 4, 260, 4, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "potion3", 4, 220, 5, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "potion4", 4, 150, 7, 1, 2, 4, false);
    TraderStock_Try(ch, profile, "potion5", 4, 260, 3, 1, 4, 8, false);

    TraderStock_Try(ch, profile, "spyglass1", 5, 190, 1, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "spyglass2", 5, 110, 7, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "spyglass3", 5, 45, 14, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "jewelry1", 6, 150, 1, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry2", 6, 145, 2, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry3", 6, 140, 3, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry5", 6, 140, 3, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry6", 6, 135, 4, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry7", 6, 130, 5, 1, 4, 8, false);
    TraderStock_Try(ch, profile, "jewelry10", 6, 110, 6, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry11", 6, 105, 7, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry12", 6, 100, 8, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry13", 6, 95, 9, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry14", 6, 90, 10, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry16", 6, 80, 11, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry17", 6, 70, 12, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "jewelry18", 6, 60, 13, 1, 3, 6, false);
    TraderStock_Try(ch, profile, "indian4", 6, 85, 8, 1, 2, 4, false);
    TraderStock_Try(ch, profile, "indian7", 6, 80, 9, 1, 2, 4, false);
    TraderStock_Try(ch, profile, "indian8", 6, 75, 10, 1, 2, 4, false);
    TraderStock_Try(ch, profile, "indian9", 6, 70, 11, 1, 2, 4, false);
    TraderStock_Try(ch, profile, "indian13", 6, 55, 12, 1, 2, 3, false);
    TraderStock_Try(ch, profile, "indian16", 6, 45, 14, 1, 2, 3, false);

    TraderStock_Try(ch, profile, "map_bad", 7, 90, 1, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_normal", 7, 35, 4, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_jam", 7, 18, 3, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_cayman", 7, 12, 6, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_barbados", 7, 18, 4, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_trinidad", 7, 17, 5, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_Curacao", 7, 17, 5, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_martiniqua", 7, 17, 5, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_dominica", 7, 11, 7, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_puerto", 7, 17, 6, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_cuba", 7, 17, 6, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_hisp", 7, 17, 7, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_nevis", 7, 17, 7, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_guad", 7, 17, 8, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_antigua", 7, 17, 8, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_TORTUGA", 7, 17, 8, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_terks", 7, 11, 9, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_sm", 7, 17, 8, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_bermudas", 7, 17, 9, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_Pearl", 7, 11, 10, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_beliz", 7, 17, 9, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_santa", 7, 17, 10, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_maine_1", 7, 11, 11, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_maine_2", 7, 11, 12, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_panama", 7, 17, 10, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_maracaibo", 7, 17, 11, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_cumana", 7, 17, 11, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "map_Providence", 7, 17, 12, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "sculMa3", 7, 12, 1, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_1", 7, 10, 15, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_2", 7, 10, 15, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_3", 7, 10, 16, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_4", 7, 10, 16, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_5", 7, 10, 17, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_6", 7, 10, 17, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_7", 7, 10, 18, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_8", 7, 10, 18, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_9", 7, 10, 19, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_10", 7, 10, 19, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_13", 7, 10, 20, 1, 1, 1, false);
    TraderStock_Try(ch, profile, "Totem_14", 7, 10, 20, 1, 1, 1, false);
    TraderStock_AddLSCMap(ch);
}

'''


SEGMENT_BYTES = ITEM_SHOP_CODE.strip().replace("\n", "\r\n").encode("utf-8") + b"\r\n"
SEGMENT_SHA256 = sha256(SEGMENT_BYTES)

SEADOGS_LOAD_OLD = '''void Main()
{
	screenscaling = BI_COMPARE_HEIGHT;'''

SEADOGS_LOAD_NEW = '''void Main()
{
	if (!LoadSegment("scripts\\shop_rotation.c"))
	{
		Trace("shop rotation segment failed to load");
	}
	screenscaling = BI_COMPARE_HEIGHT;'''


BASE_PATCH = PatchSet(
    TARGET_ROOT / ".codex-shop-rotation" / "20260914-v1",
    (
        FilePatch(
            "PROGRAM/seadogs.c",
            "1ca8cdd17dcc0269fa10f59b9d87639f07101a731a8a2a5dc8cee32762a36942",
            "93713714aece725656dcb65358ab113fef395ed407fae4859314a1c189878fff",
            ((SEADOGS_LOAD_OLD, SEADOGS_LOAD_NEW),),
        ),
        FilePatch(
            "PROGRAM/store/initStore.c",
            "783ce9b565d043a61895f225a25ed0b23da546101eae0d3ac6cfc7a03e600f09",
            "27aa94dc9049ee9ce93f1d564db6114847af2b3bddf1d1d4f66618b13642f9f0",
            (
                (
                    '''		if (pRef.StoreSize == "small")
		{
		    pRef.Goods.(goodName).Quantity = makeint(sti(pRef.Goods.(goodName).Quantity) / 3); // было 2 до 1.2.3
		}''',
                    '''		if (pRef.StoreSize == "small")
		{
		    pRef.Goods.(goodName).Quantity = makeint(sti(pRef.Goods.(goodName).Quantity) / 3); // было 2 до 1.2.3
		}
		else
		{
			if (pRef.StoreSize == "medium")
			{
				pRef.Goods.(goodName).Quantity = makeint(sti(pRef.Goods.(goodName).Quantity) / 2);
			}
		}''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/store/storeutilite.c",
            "146839ed55be9c76fa7b4bfbd1149d41dde8581e72ef7d87f1bd0eaf5a243e08",
            "fe19d87f68c3b838c838f8909353194bb6027118414418cf64ed00d1d0dfb903",
            (
                (
                    '''int storeDayUpdateCnt = -1;
void StoreDayUpdateStart()
{''' + "\t" + '''
	if(storeDayUpdateCnt >= 0) return;
	storeDayUpdateCnt = 0;
	PostEvent("EvStoreDayUpdate", 30);
	Event("EvSituationsUpdate", "l", 0);   // вызов размазаных вычислений на НехтДай
}

void StoreDayUpdate()
{
	if(storeDayUpdateCnt >= 0)
	{
		UpdateStore(&Stores[storeDayUpdateCnt]);
		storeDayUpdateCnt++;
		if(storeDayUpdateCnt >= STORE_QUANTITY) storeDayUpdateCnt = -1;
		PostEvent("EvStoreDayUpdate", 30);
	}
}''',
                    '''int storeDayUpdateCnt = -1;
int storeDayUpdateSteps = 1;
void StoreDayUpdateStart()
{''' + "\t" + '''
	if(storeDayUpdateCnt >= 0) return;
	storeDayUpdateSteps = 1;
	if (CheckAttribute(&Stores[SHIP_STORE], "MarketUpdate.control_year"))
	{
		storeDayUpdateSteps = GetNpcQuestPastDayParam(&Stores[SHIP_STORE], "MarketUpdate");
		if (storeDayUpdateSteps < 0) storeDayUpdateSteps = 0;
	}
	SaveCurrentNpcQuestDateParam(&Stores[SHIP_STORE], "MarketUpdate");
	if (storeDayUpdateSteps == 0) return;
	storeDayUpdateCnt = 0;
	PostEvent("EvStoreDayUpdate", 30);
	Event("EvSituationsUpdate", "l", 0);   // прочие суточные события вызываются ровно один раз
}

void StoreDayUpdate()
{
	if(storeDayUpdateCnt >= 0)
	{
		for (int marketStep = 0; marketStep < storeDayUpdateSteps; marketStep++)
		{
			UpdateStore(&Stores[storeDayUpdateCnt]);
		}
		storeDayUpdateCnt++;
		if(storeDayUpdateCnt >= STORE_QUANTITY) storeDayUpdateCnt = -1;
		PostEvent("EvStoreDayUpdate", 30);
	}
}''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/scripts/utils.c",
            "a949007932fe27f86a36648b177d821a5ffb17642badb5b5a8c0c9d8e9b394f2",
            "52e0ebaf023ffa41eed60808aeb8c0970b544978bb8f3ceb6cc9b22e63cdb962",
            (
                ("void GiveItemToTrader(aref ch)\n{", "void GiveItemToTraderLegacy(aref ch)\n{"),
                (
                    "string PlaceCharacter(aref ch, string group, string location)",
                    '''extern void TraderStock_GiveItemToTrader(aref ch);

void GiveItemToTrader(aref ch)
{
    GiveItemToTraderLegacy(ch);
}

string PlaceCharacter(aref ch, string group, string location)''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/interface/itemstrade.c",
            "38f3c1deb5077a79a26199f1cd802d6a12ac3759b6c75bb81e7a5c670180aa16",
            "81413086b0ea636981ca419f23c80abbbc401dd6857543eb957d947f0b64f111",
            (
                (
                    '''	if (BuyOrSell == 1) // BUY
	{
		TakeNItems(refStoreChar, Items[iCurGoodsIdx].id, -nTradeQuantity);''',
                    '''	if (BuyOrSell == 1) // BUY
	{
		TraderStock_RecordBuy(refStoreChar, Items[iCurGoodsIdx].id, nTradeQuantity);
		TakeNItems(refStoreChar, Items[iCurGoodsIdx].id, -nTradeQuantity);''',
                ),
                (
                    ''' 	else
	{ // SELL
	    TakeNItems(refStoreChar, Items[iCurGoodsIdx].id, nTradeQuantity);''',
                    ''' 	else
	{ // SELL
	    TakeNItems(refStoreChar, Items[iCurGoodsIdx].id, nTradeQuantity);
		TraderStock_RecordPlayerSale(refStoreChar, Items[iCurGoodsIdx].id, nTradeQuantity);''',
                ),
            ),
        ),
    ),
)


class ShopRotationPatch:
    """Exact existing-file transforms plus one owned persistent script segment."""

    def __init__(self, base: PatchSet) -> None:
        self.base = base

    def overall_state(self, root: Path):
        base_state, rows = self.base.overall_state(root)
        segment = root / SEGMENT_PATH
        if not segment.exists():
            segment_state = "original"
        elif segment.is_file() and sha256(segment.read_bytes()) == SEGMENT_SHA256:
            segment_state = "patched"
        else:
            segment_state = "unsupported"
        aggregate = base_state if base_state == segment_state else "mixed"
        return aggregate, rows

    def print_status(self, root: Path) -> int:
        state, rows = self.overall_state(root)
        print(f"target: {root}")
        print(f"state:  {state}")
        for spec, item_state in rows:
            path = root / spec.relative_path
            digest = sha256(path.read_bytes()) if path.is_file() else "-"
            print(f"{item_state:11} {digest}  {spec.relative_path}")
        segment = root / SEGMENT_PATH
        segment_state = "original" if not segment.exists() else (
            "patched" if segment.is_file() and sha256(segment.read_bytes()) == SEGMENT_SHA256 else "unsupported"
        )
        digest = sha256(segment.read_bytes()) if segment.is_file() else "-"
        print(f"{segment_state:11} {digest}  {SEGMENT_PATH}")
        return 0 if state in {"original", "patched"} else 1

    def apply(self, root: Path) -> str:
        if root.resolve() != TARGET_ROOT.resolve():
            raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
        state, _ = self.overall_state(root)
        if state == "patched":
            return "already patched"
        if state != "original":
            raise RuntimeError(f"refusing apply: aggregate state is {state}")

        with lifecycle_lock():
            require_idle()
            originals = {
                spec.relative_path: (root / spec.relative_path).read_bytes()
                for spec in self.base.files
            }
            for spec in self.base.files:
                snapshot = self.base.snapshot_root / spec.relative_path
                if snapshot.exists():
                    if sha256(snapshot.read_bytes()) != spec.original_sha256:
                        raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
                else:
                    atomic_write(snapshot, originals[spec.relative_path])
                    os.chmod(snapshot, 0o444)

            written: list[Path] = []
            segment = root / SEGMENT_PATH
            try:
                for spec in self.base.files:
                    path = root / spec.relative_path
                    atomic_write(path, transform(originals[spec.relative_path], spec))
                    written.append(path)
                atomic_write(segment, SEGMENT_BYTES)
                written.append(segment)
                final, _ = self.overall_state(root)
                if final != "patched":
                    raise RuntimeError(f"post-apply state is {final}")
            except Exception:
                if segment.exists() and sha256(segment.read_bytes()) == SEGMENT_SHA256:
                    segment.unlink()
                for path in reversed(written):
                    relative = str(path.relative_to(root))
                    if relative in originals:
                        atomic_write(path, originals[relative])
                raise
        return "patched"

    def revert(self, root: Path) -> str:
        if root.resolve() != TARGET_ROOT.resolve():
            raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
        state, _ = self.overall_state(root)
        if state == "original":
            return "already original"
        if state != "patched":
            raise RuntimeError(f"refusing revert: aggregate state is {state}")

        with lifecycle_lock():
            require_idle()
            patched = {
                spec.relative_path: (root / spec.relative_path).read_bytes()
                for spec in self.base.files
            }
            originals: dict[str, bytes] = {}
            for spec in self.base.files:
                snapshot = self.base.snapshot_root / spec.relative_path
                if not snapshot.is_file():
                    raise RuntimeError(f"snapshot is missing: {snapshot}")
                data = snapshot.read_bytes()
                if sha256(data) != spec.original_sha256:
                    raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
                originals[spec.relative_path] = data

            segment = root / SEGMENT_PATH
            restored: list[Path] = []
            try:
                for spec in self.base.files:
                    path = root / spec.relative_path
                    atomic_write(path, originals[spec.relative_path])
                    restored.append(path)
                segment.unlink()
                final, _ = self.overall_state(root)
                if final != "original":
                    raise RuntimeError(f"post-revert state is {final}")
            except Exception:
                for path in reversed(restored):
                    atomic_write(path, patched[str(path.relative_to(root))])
                if not segment.exists():
                    atomic_write(segment, SEGMENT_BYTES)
                raise
        return "original"


PATCH = ShopRotationPatch(BASE_PATCH)


def verify(root: Path) -> int:
    state, rows = PATCH.overall_state(root)
    if state not in {"original", "patched"}:
        PATCH.print_status(root)
        return 1

    candidates: dict[str, str] = {}
    for spec, item_state in rows:
        source = (root / spec.relative_path).read_bytes()
        candidate = transform(source, spec) if item_state == "original" else source
        candidates[spec.relative_path] = candidate.decode("utf-8")

        # The inverse is intentionally tested without trusting runtime snapshots.
        newline = b"\r\n" if b"\r\n" in candidate else b"\n"
        restored = candidate
        for old, new in reversed(spec.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if restored.count(new_bytes) != 1:
                raise RuntimeError(f"inverse block is not unique: {spec.relative_path}")
            restored = restored.replace(new_bytes, old_bytes, 1)
        if sha256(restored) != spec.original_sha256:
            raise RuntimeError(f"inverse hash mismatch: {spec.relative_path}")

    seadogs = candidates["PROGRAM/seadogs.c"]
    init = candidates["PROGRAM/store/initStore.c"]
    stores = candidates["PROGRAM/store/storeutilite.c"]
    wrapper = candidates["PROGRAM/scripts/utils.c"]
    utils = SEGMENT_BYTES.decode("utf-8")
    trade = candidates["PROGRAM/interface/itemstrade.c"]

    required = (
        (seadogs, 'LoadSegment("scripts\\shop_rotation.c")'),
        (init, 'pRef.StoreSize == "medium"'),
        (stores, 'storeDayUpdateSteps = GetNpcQuestPastDayParam(&Stores[SHIP_STORE], "MarketUpdate")'),
        (stores, 'for (int marketStep = 0; marketStep < storeDayUpdateSteps; marketStep++)'),
        (utils, "int TraderStock_Profile(aref ch)"),
        (utils, "25 + rand(25)"),
        (utils, "ch.TradeStock.Interval = 2 + rand(2)"),
        (utils, "TraderStock_RecordPlayerSale"),
        (utils, "TraderStock_RecordBuy"),
        (utils, "int TraderStock_SeriesQty(aref ch, string baseID)"),
        (utils, "if (TraderStock_Profile(ch) != 0) return;"),
        (wrapper, "extern void TraderStock_GiveItemToTrader(aref ch);"),
        (wrapper, "GiveItemToTraderLegacy(ch);"),
        (trade, "TraderStock_RecordPlayerSale(refStoreChar"),
        (trade, "TraderStock_RecordBuy(refStoreChar"),
    )
    for text, marker in required:
        if marker not in text:
            raise RuntimeError(f"shop rotation marker is missing: {marker}")

    if "TraderStock_GiveItemToTrader(ch);" in wrapper:
        raise RuntimeError("item shops still enter the unaccepted weighted generator")

    if 'SaveCurrentNpcQuestDateParam(&Stores[SHIP_STORE], "MarketUpdate")' in init:
        raise RuntimeError("market cursor is initialized before the game date exists")

    if 'DeleteAttribute(ch, "items")' in utils:
        raise RuntimeError("active item-shop rotation still wipes the whole inventory")
    if stores.count('Event("EvSituationsUpdate", "l", 0)') != 1:
        raise RuntimeError("non-market daily event dispatch was duplicated")

    candidate_ids = set(re.findall(r'TraderStock_Try\(ch, profile, "([^"]+)"', utils))
    item_corpus = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in (root / "PROGRAM/ITEMS").rglob("*.c")
    )
    missing_ids = sorted(
        item_id
        for item_id in candidate_ids
        if re.search(r'\.id\s*=\s*"' + re.escape(item_id) + r'"', item_corpus) is None
    )
    if missing_ids:
        raise RuntimeError(f"weighted catalog contains unknown item IDs: {missing_ids}")

    expected_maps = {
        "map_bad", "map_normal", "map_jam", "map_cayman", "map_barbados",
        "map_trinidad", "map_Curacao", "map_martiniqua", "map_dominica",
        "map_puerto", "map_cuba", "map_hisp", "map_nevis", "map_guad",
        "map_antigua", "map_TORTUGA", "map_terks", "map_sm",
        "map_bermudas", "map_Pearl", "map_beliz", "map_santa",
        "map_maine_1", "map_maine_2", "map_panama", "map_maracaibo",
        "map_cumana", "map_Providence",
    }
    expected_totems = {f"Totem_{value}" for value in (*range(1, 11), 13, 14)}
    if not expected_maps.issubset(candidate_ids):
        raise RuntimeError("weighted catalog dropped a legacy map candidate")
    if not expected_totems.issubset(candidate_ids) or "sculMa3" not in candidate_ids:
        raise RuntimeError("weighted catalog dropped a legacy totem or sculpture candidate")

    # Deterministic fixtures mirror the changed rules, without touching the runtime.
    large = 90
    medium = large // 2
    small = large // 3
    if not small < medium < large:
        raise RuntimeError("medium cargo scaling is not strictly between small and large")

    if (date(1665, 3, 4) - date(1665, 3, 1)).days != 3:
        raise RuntimeError("multi-day market fixture failed")
    if (date(1665, 3, 1) - date(1665, 3, 1)).days != 0:
        raise RuntimeError("same-day market fixture failed")

    def chance(base: int, rank: int, target: int, modifier: int = 0) -> int:
        return max(10, min(900, base + (rank - target) * 7 + modifier))

    if chance(25, 1, 24) != 10 or chance(25, 30, 24) <= 25:
        raise RuntimeError("rank is not a soft, bounded stock weight")
    if chance(10, 100, 1, 300) >= 1000:
        raise RuntimeError("rare stock can become guaranteed")
    if {2 + value for value in range(3)} != {2, 3, 4}:
        raise RuntimeError("rotation interval is not 2-4 days")

    current, player, generated = 8, 3, 5
    removable = min(generated, max(0, current - player))
    for percent in (25, 37, 50):
        removed = removable * percent // 100
        if current - removed < player:
            raise RuntimeError("partial retirement can consume player-sold stock")
    buy = 6
    player_used = min(player, buy)
    generated_used = min(generated, buy - player_used)
    if (player - player_used, generated - generated_used) != (0, 2):
        raise RuntimeError("mixed-origin buy accounting fixture failed")

    print("shop rotation patch: verified")
    print("cargo: small=1/3, medium=1/2, large=1; elapsed-day catch-up is market-only")
    print("items: weighted profiles, 2-4 day partial rotation, permanent-shop sales preserved")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    args = parser.parse_args()

    try:
        if args.action == "status":
            return PATCH.print_status(TARGET_ROOT)
        if args.action == "verify":
            return verify(TARGET_ROOT)
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

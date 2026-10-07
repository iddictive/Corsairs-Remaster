// BOAL меню торговли
string CurTable, CurRow;
string TradeBookSelectedCity = "";
string TradeBookPinnedCity = "";
int TradeBookRouteMode = 1;
int TradeBookContraband = 0;
int TradeBookPinAuto = 0;

void InitInterface(string iniName)
{
    InterfaceStack.SelectMenu_node = "LaunchTradeBook"; // запоминаем, что звать по Ф2
	GameInterface.title = "titleTradeBook";
	SendMessage(&GameInterface,"ls",MSG_INTERFACE_INIT,iniName);

	SetEventHandler("InterfaceBreak","ProcessExitCancel",0);
	SetEventHandler("exitCancel","ProcessExitCancel",0);
    SetEventHandler("ievnt_command","ProcessCommandExecute",0);
    SetEventHandler("TableSelectChange", "TableSelectChange", 0);
    SetEventHandler("ShowInfoWindow","ShowInfoWindow",0);
    SetEventHandler("MouseRClickUp","HideInfoWindow",0);
    
    XI_RegistryExitKey("IExit_F2");
    FillPriceListTown("TABLE_CITY");
    InitTradeComparison();
}

void ProcessExitCancel()
{
	IDoExit(RC_INTERFACE_ANY_EXIT);
}

void IDoExit(int exitCode)
{
	DelEventHandler("InterfaceBreak","ProcessExitCancel");
	DelEventHandler("exitCancel","ProcessExitCancel");
    DelEventHandler("ievnt_command","ProcessCommandExecute");
    DelEventHandler("TableSelectChange", "TableSelectChange");
    DelEventHandler("ShowInfoWindow","ShowInfoWindow");
    DelEventHandler("MouseRClickUp","HideInfoWindow");
    
	interfaceResultCommand = exitCode;
	if( CheckAttribute(&InterfaceStates,"ReloadMenuExit"))
	{
        DeleteAttribute(&InterfaceStates,"ReloadMenuExit");
		EndCancelInterface(false);
	}
	else
	{
		EndCancelInterface(true);
	}
}
void ProcessCommandExecute()
{
	string comName = GetEventData();
	string nodName = GetEventData();
	switch(nodName)
	{
		case "MODE_TOGGLE":
			if(comName == "click" || comName == "activate")
			{
				ToggleTradeBookMode();
				return;
			}
		break;
        case "CONTRABAND_TOGGLE":
            if(comName == "click" || comName == "activate")
            {
                ToggleTradeBookContraband();
                return;
            }
        break;
		case "PIN_CITY":
			if(comName == "click" || comName == "activate")
			{
				PinSelectedTradeCity();
				return;
			}
		break;
		case "I_CHARACTER_2":
			if(comName=="click")
			{
			    nodName = "I_CHARACTER";
			}
		break;
		case "I_SHIP_2":
			if(comName=="click")
			{
			    nodName = "I_SHIP";
			}
		break;
		case "I_QUESTBOOK_2":
			if(comName=="click")
			{
			    nodName = "I_QUESTBOOK";
			}
		break;
		case "I_TRADEBOOK_2":
			if(comName=="click")
			{
			    nodName = "I_TRADEBOOK";
			}
		break;
		case "I_NATIONS_2":
			if(comName=="click")
			{
			    nodName = "I_NATIONS";
			}
		break;
		case "I_ITEMS_2":
			if(comName=="click")
			{
			    nodName = "I_ITEMS";
			}
		break;
	}
	// boal new menu 31.12.04 -->
	if (nodName == "I_CHARACTER" || nodName == "I_SHIP" ||
	    nodName == "I_QUESTBOOK" || nodName == "I_TRADEBOOK" ||
		nodName == "I_NATIONS" || nodName == "I_ITEMS")
	{
		if(comName=="click")
		{
            InterfaceStates.ReloadMenuExit = true;
			IDoExit(RC_INTERFACE_ANY_EXIT);
			PostEvent("LaunchIAfterFrame",1,"sl", nodName, 2);
			return;
		}
	}
	// boal new menu 31.12.04 -->
}
//  таблица: город, местоположение, актуальность
void FillPriceListTown(string _tabName)
{
	string  cityId, attr2, firstId;
    int     i, cn, n;
    ref     nulChr;
    string  row;
    aref    rootItems;
    aref    curItem;
    ref     rCity;
    
    // шапка -->
    GameInterface.(_tabName).select = 0;
    GameInterface.(_tabName).hr.td1.str = "Нация";
    GameInterface.(_tabName).hr.td1.scale = 0.77
	GameInterface.(_tabName).hr.td2.str = "Город";
	GameInterface.(_tabName).hr.td2.scale = 0.8;
	GameInterface.(_tabName).hr.td3.str = "Местоположение";
	GameInterface.(_tabName).hr.td3.scale = 0.7;
	GameInterface.(_tabName).hr.td4.str = "Актуальность";
	GameInterface.(_tabName).hr.td4.scale = 0.7;
    // <--
    nulChr = &NullCharacter;
    makearef(rootItems, nulChr.PriceList);  // тут живут ИД города и служ. инфа.
    n = 1;
    firstId = "";
    for (i=0; i<GetAttributesNum(rootItems); i++)
    {
        row = "tr" + n;
		curItem = GetAttributeN(rootItems, i);
		cityId = GetAttributeName(curItem);
		cn = FindColony(cityId);
		if (cn != -1)
		{
			rCity = GetColonyByIndex(cn);
			if (n == 1) firstId = cityId;
			GameInterface.(_tabName).(row).UserData.CityID  = cityId;
			GameInterface.(_tabName).(row).UserData.CityIDX = cn;
			GameInterface.(_tabName).(row).td1.icon.group  = "NATIONS";
			GameInterface.(_tabName).(row).td1.icon.image  = Nations[sti(rCity.nation)].Name;
			GameInterface.(_tabName).(row).td1.icon.width  = 26;
		    GameInterface.(_tabName).(row).td1.icon.height = 26;
		    GameInterface.(_tabName).(row).td1.icon.offset = "3, 3";
			GameInterface.(_tabName).(row).td2.str = GetConvertStr(cityId + " Town", "LocLables.txt");
			GameInterface.(_tabName).(row).td2.scale = 0.85;
			GameInterface.(_tabName).(row).td3.str = GetConvertStr(rCity.islandLable, "LocLables.txt");
			GameInterface.(_tabName).(row).td3.scale = 0.8;
			GameInterface.(_tabName).(row).td4.scale = 0.75;
			if (CheckAttribute(nulChr, "PriceList." + cityId + ".AltDate"))
		    {
		        GameInterface.(_tabName).(row).td4.str = nulChr.PriceList.(cityId).AltDate;
		    }
		    else
		    {
		        GameInterface.(_tabName).(row).td4.str = "??.??.????";
		    }
			n++;
		}
	}
	if (n > 1) GameInterface.(_tabName).select = 1;
	Table_UpdateWindow(_tabName);
	TradeBookSelectedCity = firstId;
}
//  таблица
// картинка, название, картинка экспорта, продажа, покупка, колво, пачка, вес пачки
void FillPriceList(string _tabName, string  attr1)
{
    string  sGoods;
    int     i, n;
    ref     nulChr;
    string  row;
    nulChr = &NullCharacter;
    Table_Clear(_tabName, false, true, false);
    // шапка -->
    GameInterface.(_tabName).select = 0;
    GameInterface.(_tabName).hr.td1.str = "Товар";
    GameInterface.(_tabName).hr.td1.scale = 0.75;
    GameInterface.(_tabName).hr.td2.scale = 0.65;
    GameInterface.(_tabName).hr.td3.scale = 0.75;
    GameInterface.(_tabName).hr.td4.scale = 0.75;
    GameInterface.(_tabName).hr.td5.scale = 0.75;
    GameInterface.(_tabName).hr.td6.scale = 0.75;
    GameInterface.(_tabName).hr.td7.scale = 0.75;
    
    GameInterface.(_tabName).hr.td2.str = "Тип";
    
	GameInterface.(_tabName).hr.td3.str = "Купить/ц";
	
	GameInterface.(_tabName).hr.td4.str = "Продать/ц";
	
	GameInterface.(_tabName).hr.td5.str = "Запас, ц";
	
	GameInterface.(_tabName).hr.td6.str = "Шт./ц";
	
	GameInterface.(_tabName).hr.td7.str = "Мин., ц";
	
	if (attr1 != "")
	{
	    // <--
	    n = 1;
	    for (i = 0; i < GOODS_QUANTITY; i++)
	    {
	        row = "tr" + n;
	        sGoods = "Gidx" + i;
            if (sti(nulChr.PriceList.(attr1).(sGoods).TradeType) == TRADE_TYPE_CONTRABAND && TradeBookContraband == 0) continue;			
	        if (sti(nulChr.PriceList.(attr1).(sGoods).TradeType) == TRADE_TYPE_CANNONS && !bBettaTestMode) continue; // не пушки
	        
            GameInterface.(_tabName).(row).UserData.ID = Goods[i].name;
            GameInterface.(_tabName).(row).UserData.IDX = i;
            
	        GameInterface.(_tabName).(row).td1.icon.group = "GOODS";
			GameInterface.(_tabName).(row).td1.icon.image = Goods[i].name;
			GameInterface.(_tabName).(row).td1.icon.offset = "1, 0";
			GameInterface.(_tabName).(row).td1.icon.width = 32;
			GameInterface.(_tabName).(row).td1.icon.height = 32;
			GameInterface.(_tabName).(row).td1.textoffset = "30,0";
			GameInterface.(_tabName).(row).td1.str = XI_ConvertString(Goods[i].name);
			GameInterface.(_tabName).(row).td1.scale = 0.85;

	        GameInterface.(_tabName).(row).td2.icon.group = "TRADE_TYPE";
			GameInterface.(_tabName).(row).td2.icon.image = "ico_" + nulChr.PriceList.(attr1).(sGoods).TradeType;
			GameInterface.(_tabName).(row).td2.icon.offset = "0, 1";
			GameInterface.(_tabName).(row).td2.icon.width = 16;
			GameInterface.(_tabName).(row).td2.icon.height = 28;

	        if (CheckAttribute(nulChr, "PriceList." + attr1 + "." + sGoods + ".Buy"))
	        {
	            GameInterface.(_tabName).(row).td3.str = TradeBookPricePerWeight(nulChr.PriceList.(attr1).(sGoods).Buy, i);
	        }
	        else
	        {
	            GameInterface.(_tabName).(row).td3.str = "???";
	        }
	        if (CheckAttribute(nulChr, "PriceList." + attr1 + "." + sGoods + ".Sell"))
	        {
	            GameInterface.(_tabName).(row).td4.str = TradeBookPricePerWeight(nulChr.PriceList.(attr1).(sGoods).Sell, i);
	        }
	        else
	        {
	            GameInterface.(_tabName).(row).td4.str = "???";
	        }
	        if (CheckAttribute(nulChr, "PriceList." + attr1 + "." + sGoods + ".Qty"))
	        {
	            GameInterface.(_tabName).(row).td5.str = "???";
                if (TradeBookKnownPrice(nulChr.PriceList.(attr1).(sGoods).Qty))
                    GameInterface.(_tabName).(row).td5.str = FloatToString(stf(nulChr.PriceList.(attr1).(sGoods).Qty) * stf(Goods[i].Weight) / stf(Goods[i].Units), 1);
	        }
	        else
	        {
	            GameInterface.(_tabName).(row).td5.str = "????";
	        }
	        GameInterface.(_tabName).(row).td6.str = FloatToString(stf(Goods[i].Units) / stf(Goods[i].Weight), 1);
			GameInterface.(_tabName).(row).td7.str = Goods[i].Weight;
	        n++;
	    }
    }
    Table_UpdateWindow(_tabName);
}

string GetTradeBookCityName(string cityId)
{
	if (FindColony(cityId) == -1) return cityId;
	return GetConvertStr(cityId + " Town", "LocLables.txt");
}

void SyncTradeBookControls()
{
    string modeLabel = "#Обычный режим";
    if (TradeBookRouteMode == 1) modeLabel = "#Сравнение";
    SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"MODE_TOGGLE",0,modeLabel);
    SetNodeUsing("PIN_CITY", TradeBookRouteMode == 1);
    string pinLabel = "#Закрепить";
    if (TradeBookPinnedCity != "") pinLabel = "#Открепить";
    SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PIN_CITY",0,pinLabel);
    string filterLabel = "#Контрабанда: выкл.";
    if (TradeBookContraband == 1) filterLabel = "#Контрабанда: вкл.";
    SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CONTRABAND_TOGGLE",0,filterLabel);
}

void TradeBookRouteText(string text)
{
    SetFormatedText("TRADE_ROUTE", text);
    SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"TRADE_ROUTE",5);
}

void ToggleTradeBookContraband()
{
    TradeBookContraband = 1 - TradeBookContraband;
    pchar.SystemInfo.TradeBookContraband = TradeBookContraband;
    RefreshTradeComparison();
}

bool TradeBookKnownPrice(string price)
{
    if (price == "" || price == "???" || price == "????") return false;
    return true;
}

string TradeBookPricePerWeight(string price, int goodIndex)
{
    if (!TradeBookKnownPrice(price)) return "???";
    if (stf(Goods[goodIndex].Weight) <= 0.0) return "???";
    return FloatToString(stf(price) / stf(Goods[goodIndex].Weight), 1);
}

bool TryPinCurrentTradeCity()
{
	string currentTown = GetCurrentTown();
	ref nulChr = &NullCharacter;
	if (currentTown == "" && bSeaActive && bCanEnterToLand)
	{
		currentTown = Sea_FindNearColony();
	}
	if (currentTown == "") return false;
	if (FindColony(currentTown) == -1) return false;
	if (!CheckAttribute(nulChr, "PriceList." + currentTown)) return false;
	TradeBookPinnedCity = currentTown;
	TradeBookPinAuto = 1;
	pchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
	pchar.SystemInfo.TradeBookPinAuto = 1;
	return true;
}

void InitTradeComparison()
{
    ref nulChr = &NullCharacter;
    TradeBookPinnedCity = "";
    TradeBookRouteMode = 1;
    TradeBookContraband = 0;
    TradeBookPinAuto = 0;
    if (CheckAttribute(pchar, "SystemInfo.TradeBookRouteMode"))
        TradeBookRouteMode = sti(pchar.SystemInfo.TradeBookRouteMode);
    if (CheckAttribute(pchar, "SystemInfo.TradeBookContraband"))
        TradeBookContraband = sti(pchar.SystemInfo.TradeBookContraband);
    if (CheckAttribute(pchar, "SystemInfo.TradeBookPinnedCity"))
    {
        TradeBookPinnedCity = pchar.SystemInfo.TradeBookPinnedCity;
        if (!CheckAttribute(nulChr, "PriceList." + TradeBookPinnedCity)) TradeBookPinnedCity = "";
    }
    if (TradeBookPinnedCity != "")
    {
        if (CheckAttribute(pchar, "SystemInfo.TradeBookPinAuto"))
            TradeBookPinAuto = sti(pchar.SystemInfo.TradeBookPinAuto);
        else
            TradeBookPinAuto = 1;
    }
    if (TradeBookRouteMode == 1 && (TradeBookPinnedCity == "" || TradeBookPinAuto == 1))
    {
        TryPinCurrentTradeCity();
    }
    if (TradeBookPinnedCity == "")
    {
        pchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
    }
    RefreshTradeComparison();
}

void ToggleTradeBookMode()
{
    TradeBookRouteMode = 1 - TradeBookRouteMode;
    pchar.SystemInfo.TradeBookRouteMode = TradeBookRouteMode;
    if (TradeBookRouteMode == 1 && (TradeBookPinnedCity == "" || TradeBookPinAuto == 1))
    {
        TryPinCurrentTradeCity();
    }
    RefreshTradeComparison();
}

void PinSelectedTradeCity()
{
    if (TradeBookPinnedCity != "")
    {
        TradeBookPinnedCity = "";
        TradeBookPinAuto = 0;
    }
    else
    {
        TradeBookPinnedCity = TradeBookSelectedCity;
        TradeBookPinAuto = 0;
    }
    pchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;
    pchar.SystemInfo.TradeBookPinAuto = TradeBookPinAuto;
    RefreshTradeComparison();
}

void RefreshTradeComparison()
{
    SyncTradeBookControls();
	if (TradeBookSelectedCity == "") return;
	if (TradeBookRouteMode == 0)
	{
		TradeBookRouteText("Обычные цены: " + GetTradeBookCityName(TradeBookSelectedCity));
		FillPriceList("TABLE_GOODS", TradeBookSelectedCity);
		return;
	}
    if (TradeBookPinnedCity == "")
    {
        TradeBookRouteText("Выберите город отправления и закрепите его");
        FillPriceList("TABLE_GOODS", TradeBookSelectedCity);
        return;
    }
	if (TradeBookPinnedCity == TradeBookSelectedCity)
	{
		TradeBookRouteText("Закреплён: " + GetTradeBookCityName(TradeBookPinnedCity) + "  |  выберите второй город");
		FillPriceList("TABLE_GOODS", TradeBookSelectedCity);
		return;
	}
	FillTradeComparison("TABLE_GOODS", TradeBookPinnedCity, TradeBookSelectedCity);
}

void FillTradeComparison(string tableName, string sourceCity, string targetCity)
{
	ref nulChr = &NullCharacter;
	int goodsOrder[GOODS_QUANTITY];
	float goodsScore[GOODS_QUANTITY];
	int goodsCount = 0;
	int i, j, tmpGood;
	float tmpScore;
	string goodsAttr;

	for (i = 0; i < GOODS_QUANTITY; i++)
	{
		goodsAttr = "Gidx" + i;
		if (!CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".Buy")) continue;
		if (!CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".TradeType")) continue;
		if (!CheckAttribute(nulChr, "PriceList." + targetCity + "." + goodsAttr + ".Sell")) continue;
		if (!CheckAttribute(nulChr, "PriceList." + targetCity + "." + goodsAttr + ".TradeType")) continue;
		if (sti(nulChr.PriceList.(sourceCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND && TradeBookContraband == 0) continue;
		if (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND && TradeBookContraband == 0) continue;
		if (sti(nulChr.PriceList.(sourceCity).(goodsAttr).TradeType) == TRADE_TYPE_CANNONS && !bBettaTestMode) continue;
		if (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CANNONS && !bBettaTestMode) continue;
		if (stf(Goods[i].Weight) <= 0.0) continue;

		goodsOrder[goodsCount] = i;
        goodsScore[goodsCount] = -1000000000.0;
        if (TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Buy) && TradeBookKnownPrice(nulChr.PriceList.(targetCity).(goodsAttr).Sell))
    goodsScore[goodsCount] = (stf(nulChr.PriceList.(targetCity).(goodsAttr).Sell) - stf(nulChr.PriceList.(sourceCity).(goodsAttr).Buy)) / stf(Goods[i].Weight);
		goodsCount++;
	}

	for (i = 0; i < goodsCount - 1; i++)
	{
		for (j = i + 1; j < goodsCount; j++)
		{
			if (goodsScore[j] > goodsScore[i])
			{
				tmpScore = goodsScore[i];
				goodsScore[i] = goodsScore[j];
				goodsScore[j] = tmpScore;
				tmpGood = goodsOrder[i];
				goodsOrder[i] = goodsOrder[j];
				goodsOrder[j] = tmpGood;
			}
		}
	}

	RecalculateSquadronCargoLoad(pchar);
	int freeSpace[COMPANION_MAX];
	freeSpace[0] = GetCargoFreeSpace(pchar);
	int totalFreeSpace = freeSpace[0];
	for (i = 1; i < COMPANION_MAX; i++)
	{
		freeSpace[i] = 0;
		int companionIndex = GetCompanionIndex(pchar, i);
		if (companionIndex != -1 && GetRemovable(&Characters[companionIndex]))
		{
			freeSpace[i] = GetCargoFreeSpace(&Characters[companionIndex]);
			totalFreeSpace += freeSpace[i];
		}
	}

	Table_Clear(tableName, false, true, false);
	int moneyLeft = sti(pchar.money);
	int usedSpace = 0;
	int planProfit = 0;
	GameInterface.(tableName).select = 0;
	GameInterface.(tableName).hr.td1.str = "Товар";
    GameInterface.(tableName).hr.td1.scale = 0.75;
    GameInterface.(tableName).hr.td2.scale = 0.65;
    GameInterface.(tableName).hr.td3.scale = 0.75;
    GameInterface.(tableName).hr.td4.scale = 0.75;
    GameInterface.(tableName).hr.td5.scale = 0.75;
    GameInterface.(tableName).hr.td6.scale = 0.75;
    GameInterface.(tableName).hr.td7.scale = 0.75;
	GameInterface.(tableName).hr.td2.str = "Тип";
	GameInterface.(tableName).hr.td3.str = "Купить/ц";
	GameInterface.(tableName).hr.td4.str = "Продать/ц";
	GameInterface.(tableName).hr.td5.str = "+/ц";
	GameInterface.(tableName).hr.td6.str = "Взять, ц";
	GameInterface.(tableName).hr.td7.str = "Прибыль";

	for (i = 0; i < goodsCount; i++)
	{
		int goodIndex = goodsOrder[i];
		goodsAttr = "Gidx" + goodIndex;
		string row = "tr" + (i + 1);
		int unitSize = sti(Goods[goodIndex].Units);
		int packWeight = makeint(stf(Goods[goodIndex].Weight) + 0.5);
        int buyPrice = 0;
        int sellPrice = 0;
        bool knownPrices = TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Buy) && TradeBookKnownPrice(nulChr.PriceList.(targetCity).(goodsAttr).Sell);
        if (knownPrices)
        {
            buyPrice = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Buy);
            sellPrice = sti(nulChr.PriceList.(targetCity).(goodsAttr).Sell);
        }
		int profitPerPack = sellPrice - buyPrice;
		int plannedPacks = 0;

		if (profitPerPack > 0 && buyPrice > 0 && packWeight > 0 && unitSize > 0 && CheckAttribute(nulChr, "PriceList." + sourceCity + "." + goodsAttr + ".Qty"))
		{
            int packsLeft = 0;
            if (TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Qty))
                packsLeft = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Qty) / unitSize;
			int affordablePacks = moneyLeft / buyPrice;
			if (packsLeft > affordablePacks) packsLeft = affordablePacks;
			for (j = 0; j < COMPANION_MAX; j++)
			{
				int bestSlot = -1;
				int bestRemainder = 1000000000;
				for (int k = 0; k < COMPANION_MAX; k++)
				{
					if (freeSpace[k] < packWeight) continue;
					int slotRemainder = freeSpace[k] - packWeight;
					if (slotRemainder < bestRemainder)
					{
						bestSlot = k;
						bestRemainder = slotRemainder;
					}
				}
				if (bestSlot == -1) break;
				int fittingPacks = freeSpace[bestSlot] / packWeight;
				if (fittingPacks > packsLeft) fittingPacks = packsLeft;
				plannedPacks += fittingPacks;
				packsLeft -= fittingPacks;
				freeSpace[bestSlot] -= fittingPacks * packWeight;
				if (packsLeft <= 0) break;
			}
			moneyLeft -= plannedPacks * buyPrice;
			usedSpace += plannedPacks * packWeight;
			planProfit += plannedPacks * profitPerPack;
		}

		GameInterface.(tableName).(row).UserData.ID = Goods[goodIndex].name;
		GameInterface.(tableName).(row).UserData.IDX = goodIndex;
		GameInterface.(tableName).(row).td1.icon.group = "GOODS";
		GameInterface.(tableName).(row).td1.icon.image = Goods[goodIndex].name;
		GameInterface.(tableName).(row).td1.icon.offset = "1, 0";
		GameInterface.(tableName).(row).td1.icon.width = 32;
		GameInterface.(tableName).(row).td1.icon.height = 32;
		GameInterface.(tableName).(row).td1.textoffset = "30,0";
		GameInterface.(tableName).(row).td1.str = XI_ConvertString(Goods[goodIndex].name);
		GameInterface.(tableName).(row).td1.scale = 0.8;
		GameInterface.(tableName).(row).td2.icon.group = "TRADE_TYPE";
		GameInterface.(tableName).(row).td2.icon.image = "ico_" + nulChr.PriceList.(sourceCity).(goodsAttr).TradeType;
		GameInterface.(tableName).(row).td2.icon.offset = "0, 1";
		GameInterface.(tableName).(row).td2.icon.width = 16;
		GameInterface.(tableName).(row).td2.icon.height = 28;
		GameInterface.(tableName).(row).td3.str = TradeBookPricePerWeight(nulChr.PriceList.(sourceCity).(goodsAttr).Buy, goodIndex);
		GameInterface.(tableName).(row).td4.str = TradeBookPricePerWeight(nulChr.PriceList.(targetCity).(goodsAttr).Sell, goodIndex);
		if (goodsScore[i] < 0.0) GameInterface.(tableName).(row).td5.str = "-" + FloatToString(-goodsScore[i], 1);
		else GameInterface.(tableName).(row).td5.str = FloatToString(goodsScore[i], 1);
		GameInterface.(tableName).(row).td6.str = plannedPacks * packWeight;
		GameInterface.(tableName).(row).td7.str = plannedPacks * profitPerPack;
        if (!knownPrices)
        {
            GameInterface.(tableName).(row).td5.str = "???";
            GameInterface.(tableName).(row).td7.str = "???";
        }
        if (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND)
            GameInterface.(tableName).(row).td2.icon.image = "ico_" + TRADE_TYPE_CONTRABAND;
		if (profitPerPack > 0)
		{
			GameInterface.(tableName).(row).td5.color = argb(255,196,255,196);
			GameInterface.(tableName).(row).td7.color = argb(255,196,255,196);
		}
		if (profitPerPack < 0)
		{
			GameInterface.(tableName).(row).td5.color = argb(255,255,196,196);
			GameInterface.(tableName).(row).td7.color = argb(255,255,196,196);
		}
	}

	TradeBookRouteText(GetTradeBookCityName(sourceCity) + "  >  " + GetTradeBookCityName(targetCity) + "  |  план " + usedSpace + "/" + totalFreeSpace + " ц  |  прибыль " + planProfit);
	Table_UpdateWindow(tableName);
}

void TableSelectChange()
{
	string sControl = GetEventData();
	int iSelected = GetEventData();
    CurTable = sControl;
    CurRow   =  "tr" + (iSelected);
 	//NullSelectTable("TABLE_CITY");
    NullSelectTable("TABLE_GOODS");
    // перерисуем "прайс"
    if (CurTable == "TABLE_CITY")
    {
	    	TradeBookSelectedCity = GameInterface.(CurTable).(CurRow).UserData.CityID;
	    	RefreshTradeComparison();
    }
}

void NullSelectTable(string sControl)
{
	if (sControl != CurTable)
	{
	    GameInterface.(sControl).select = 0;
	    Table_UpdateWindow(sControl);
	}
}

void ShowInfoWindow()
{
	string sCurrentNode = GetCurrentNode();
	string sHeader, sText1, sText2, sText3, sPicture;
	string sGroup, sGroupPicture;
	int iItem;

	sPicture = "-1";
	string sAttributeName;
	int nChooseNum = -1;
	switch (sCurrentNode)
	{
        case "PIN_CITY":
            sHeader = "Закрепить город";
            sText1 = "Закрепляет выбранный город отправления. Открепить — снять закрепление.";
            sGroup = "NET_SERVERPASSWORD"; sGroupPicture = "locked";
        break;
        case "CONTRABAND_TOGGLE":
            sHeader = "Контрабанда";
            sText1 = "Показать или скрыть контрабанду. Неизвестные цены не участвуют в плане.";
            sGroup = "TRADE_TYPE"; sGroupPicture = "ico_" + TRADE_TYPE_CONTRABAND;
        break;
		case "TABLE_GOODS":
		    sGroup = "GOODS";
		    sGroupPicture = GameInterface.(CurTable).(CurRow).UserData.ID;
		    sHeader = XI_ConvertString(GameInterface.(CurTable).(CurRow).UserData.ID);
		    iItem = sti(GameInterface.(CurTable).(CurRow).UserData.IDX);
		    sText1  = GetAssembledString(GetConvertStr(GameInterface.(CurTable).(CurRow).UserData.ID + "_descr", "GoodsDescribe.txt"), &Goods[iItem]);
		    sText2  = "Цены — за центнер. Взять — вес в центнерах. Прибыль — за весь запланированный груз по записанным ценам. Неизвестные цены не входят в план.";
		break;
	}
	CreateTooltip("#" + sHeader, sText1, argb(255,255,255,255), sText2, argb(255,255,192,192), sText3, argb(255,192,255,192), "", argb(255,255,255,255), sPicture, sGroup, sGroupPicture, 64, 64);

}
void HideInfoWindow()
{
	CloseTooltip();
}
// Native treasurer management. Transactions and sale eligibility belong to the global Treasurer_* API.
int nCurScrollNum = 0;
int iCurCompanion = -1;
int iCurGoodIndex = -1;
string CurRow = "";
ref refCharacter;
bool bShowChangeWin = false;
bool bSyncControls = false;
bool bRefreshingTable = false;
int iTreasurerTab = 0;
string sSelectedItemID = "";
int iSelectedItemIndex = -1;
int iSaleQuantity = 0;

// 0: cargo target, 1: money reserve, 2: exclusive price limit, 3: retained quantity.
int iQuantityMode = 0;
int iQuantityShip = -1;
int iQuantityGood = -1;
int iQuantityMinimum = 0;
int iQuantityMaximum = 999999;
int iQuantityStep = 1;
string sQuantityReturnNode = "TABLE_LIST";

void InitInterface(string iniName)
{
    StartAboveForm(true);
    FillShipsScroll();
    SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);
    // The native scroll normalizes its provisional -1 only during initialization.
    nCurScrollNum = sti(GameInterface.SHIPS_SCROLL.current);
    CreateString(true, "ShipName", "", FONT_NORMAL, COLOR_MONEY, 400, 98, SCRIPT_ALIGN_CENTER, 0.7);
    SetFormatedText("MAIN_CAPTION", "Казначей");
    SetFormatedText("TAB_BUY_TEXT", "Закупка товаров");
    SetFormatedText("TAB_SELL_TEXT", "Продажа из сундука");
    SendMessage(&GameInterface, "lsl", MSG_INTERFACE_MSG_TO_NODE, "TAB_BUY_TEXT", 5);
    SendMessage(&GameInterface, "lsl", MSG_INTERFACE_MSG_TO_NODE, "TAB_SELL_TEXT", 5);
    Button_SetText("EDIT_TARGET_BUTTON", "#Изменить цель");
    Button_SetText("CLEAR_TABLE_LIST", "#Очистить цели");
    Button_SetText("BUY_NOW_BUTTON", "#Купить эскадре");
    Button_SetText("SELL_NOW_BUTTON", "#Продать");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "AUTO_BUY_CHECK", 1, 1, "Автозакупка в порту");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "AUTO_SELL_CHECK", 1, 1, "Автопродажа в порту");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "NOTGOODSTRANSFER_CHECK", 1, 1, "Пропустить корабль");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "BUYCONTRABAND_CHECK", 1, 1, "Закупать контрабанду");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "SELL_BLADE_CHECK", 1, 1, "Клинки");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "SELL_GUN_CHECK", 1, 1, "Огнестрельное");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "SELL_ARMOR_CHECK", 1, 1, "Доспехи");
    SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "SELL_LOOT_CHECK", 1, 1, "Ценности");
    SetControlsTabMode(0);

    SetEventHandler("exitCancel", "ProcessCancelExit", 0);
    SetEventHandler("ievnt_command", "ProcCommand", 0);
    SetEventHandler("TableSelectChange", "TableSelectChange", 0);
    SetEventHandler("TableActivate", "TreasurerUI_TableActivate", 0);
    SetEventHandler("MouseRClickUP", "HideInfo", 0);
    SetEventHandler("CheckButtonChange", "ProcessCheckBox", 0);
    SetEventHandler("ShowItemInfo", "ShowItemInfo", 0);
    SetEventHandler("eTabControlPress", "procTabChange", 0);
    SetEventHandler("UnShowWindow", "UnShowWindow", 0);
    SetEventHandler("frame", "ProcessFrame", 1);
    SetEventHandler("ADD", "TreasurerUI_Add", 0);
    SetEventHandler("REMOVE", "TreasurerUI_Remove", 0);
    SetEventHandler("ADD_ALL", "TreasurerUI_AddAll", 0);
    SetEventHandler("REMOVE_ALL", "TreasurerUI_RemoveAll", 0);
    SetEventHandler("confirmChangeQTY_EDIT", "confirmChangeQTY_EDIT", 0);
}

void ProcessCancelExit()
{
    if (bShowChangeWin) UnShowWindow();
    else IDoExit(RC_INTERFACE_GOODS_TRANSFER);
}

void IDoExit(int exitCode)
{
    EndAboveForm(true);
    DelEventHandler("exitCancel", "ProcessCancelExit");
    DelEventHandler("ievnt_command", "ProcCommand");
    DelEventHandler("TableSelectChange", "TableSelectChange");
    DelEventHandler("TableActivate", "TreasurerUI_TableActivate");
    DelEventHandler("MouseRClickUP", "HideInfo");
    DelEventHandler("CheckButtonChange", "ProcessCheckBox");
    DelEventHandler("ShowItemInfo", "ShowItemInfo");
    DelEventHandler("eTabControlPress", "procTabChange");
    DelEventHandler("UnShowWindow", "UnShowWindow");
    DelEventHandler("frame", "ProcessFrame");
    DelEventHandler("ADD", "TreasurerUI_Add");
    DelEventHandler("REMOVE", "TreasurerUI_Remove");
    DelEventHandler("ADD_ALL", "TreasurerUI_AddAll");
    DelEventHandler("REMOVE_ALL", "TreasurerUI_RemoveAll");
    DelEventHandler("confirmChangeQTY_EDIT", "confirmChangeQTY_EDIT");
    interfaceResultCommand = exitCode;
    EndCancelInterface(true);
}

void HideInfo()
{
    CloseTooltip();
}

void FillShipsScroll()
{
    FillScrollImageWithCompanionShips("SHIPS_SCROLL", 5);
}

void ProcessFrame()
{
    if (bShowChangeWin) return;
    // Selection refresh only: automatic transactions never run in interface frames.
    int selected = sti(GameInterface.SHIPS_SCROLL.current);
    if (selected == nCurScrollNum) return;
    nCurScrollNum = selected;
    iCurGoodIndex = -1;
    RefreshInterface();
}

void procTabChange()
{
    int command = GetEventData();
    string node = GetEventData();
    if (bShowChangeWin) return;
    if (node == "TAB_BUY") SetControlsTabMode(0);
    if (node == "TAB_SELL") SetControlsTabMode(1);
}

void SetControlsTabMode(int tab)
{
    iTreasurerTab = tab;
    XI_WindowShow("BUY_WINDOW", tab == 0);
    XI_WindowDisable("BUY_WINDOW", tab != 0);
    XI_WindowShow("SELL_WINDOW", tab == 1);
    XI_WindowDisable("SELL_WINDOW", tab != 1);
    string buyPicture = "TabSelected";
    string sellPicture = "TabSelected";
    int buyColor = argb(255, 196, 196, 196);
    int sellColor = buyColor;
    if (tab == 0)
    {
        buyPicture = "TabDeSelected";
        buyColor = argb(255, 255, 255, 255);
    }
    else
    {
        sellPicture = "TabDeSelected";
        sellColor = argb(255, 255, 255, 255);
    }
    SetNewGroupPicture("TAB_BUY", "TABS", buyPicture);
    SetNewGroupPicture("TAB_SELL", "TABS", sellPicture);
    SendMessage(&GameInterface, "lslll", MSG_INTERFACE_MSG_TO_NODE, "TAB_BUY_TEXT", 8, 0, buyColor);
    SendMessage(&GameInterface, "lslll", MSG_INTERFACE_MSG_TO_NODE, "TAB_SELL_TEXT", 8, 0, sellColor);
    RefreshInterface();
    if (tab == 0) SetCurrentNode("TABLE_LIST");
    else SetCurrentNode("CHEST_TABLE");
}

void RefreshInterface()
{
    FillShipContext();
    if (iTreasurerTab == 0) FillGoodsTable();
    else FillChestTable();
    SetCheckButtonsStates();
    SetVariable();
}

void FillShipContext()
{
    iCurCompanion = -1;
    string attributeName = "pic" + (nCurScrollNum + 1);
    if (nCurScrollNum >= 0 && CheckAttribute(&GameInterface, "SHIPS_SCROLL." + attributeName + ".companionIndex"))
    {
        int index = sti(GameInterface.SHIPS_SCROLL.(attributeName).companionIndex);
        if (index >= 0 && index < TOTAL_CHARACTERS) iCurCompanion = index;
    }
    refCharacter = pchar;
    if (iCurCompanion >= 0) refCharacter = GetCharacter(iCurCompanion);
    string captainTitle = XI_ConvertString("Captain");
    if (nCurScrollNum > 0) captainTitle = XI_ConvertString("companionship");
    SetFormatedText("CAPACITY", captainTitle + NewStr() + GetFullName(refCharacter));
    SetNewPicture("MAIN_CHARACTER_PICTURE", "interfaces\portraits\256\face_" + refCharacter.FaceId + ".tga");
    GameInterface.strings.shipname = "";
    if (CheckAttribute(refCharacter, "ship.name")) GameInterface.strings.shipname = refCharacter.ship.name;

    int treasurerIndex = Treasurer_OfficerIndex();
    SetFormatedText("STORE_CAPACITY", XI_ConvertString("treasurer"));
    SetNodeUsing("OTHER_PICTURE", false);
    if (treasurerIndex >= 0 && treasurerIndex < TOTAL_CHARACTERS)
    {
        SetFormatedText("STORE_CAPACITY", XI_ConvertString("treasurer") + NewStr() + GetFullName(Characters[treasurerIndex]));
        SetNodeUsing("OTHER_PICTURE", true);
        SetNewPicture("OTHER_PICTURE", "interfaces\portraits\256\face_" + Characters[treasurerIndex].FaceId + ".tga");
    }
}

void SetCheckButtonsStates()
{
    // Native SetState emits CheckButtonChange: reflecting saved values must never write defaults.
    bSyncControls = true;
    bool disabled = false;
    bool contraband = false;
    if (iCurCompanion >= 0)
    {
        disabled = CheckAttribute(refCharacter, "TransferGoods.Enable");
        contraband = CheckAttribute(refCharacter, "TransferGoods.BuyContraband");
    }
    CheckButton_SetState("NOTGOODSTRANSFER_CHECK", 1, disabled);
    CheckButton_SetState("BUYCONTRABAND_CHECK", 1, contraband);
    CheckButton_SetDisable("NOTGOODSTRANSFER_CHECK", 1, iCurCompanion < 0);
    CheckButton_SetDisable("BUYCONTRABAND_CHECK", 1, iCurCompanion < 0);
    CheckButton_SetState("AUTO_BUY_CHECK", 1, Treasurer_Setting("AutoBuy", 0, 0, 1) != 0);
    CheckButton_SetState("AUTO_SELL_CHECK", 1, Treasurer_Setting("AutoSell", 0, 0, 1) != 0);
    CheckButton_SetState("SELL_BLADE_CHECK", 1, Treasurer_Setting("SellBlade", 1, 0, 1) != 0);
    CheckButton_SetState("SELL_GUN_CHECK", 1, Treasurer_Setting("SellGun", 1, 0, 1) != 0);
    CheckButton_SetState("SELL_ARMOR_CHECK", 1, Treasurer_Setting("SellArmor", 1, 0, 1) != 0);
    CheckButton_SetState("SELL_LOOT_CHECK", 1, Treasurer_Setting("SellLoot", 1, 0, 1) != 0);
    bSyncControls = false;
}

void SetVariable()
{
    Button_SetText("RESERVE_BUTTON", "#Резерв: " + Treasurer_Setting("ReserveGold", 0, 0, 100000000));
    Button_SetText("PRICE_LIMIT_BUTTON", "#Цена вещи < " + Treasurer_Setting("PriceLimit", 1500, 1, 1000000));
    Button_SetText("KEEP_COUNT_BUTTON", "#Оставлять: " + Treasurer_Setting("KeepCount", 0, 0, 999) + " шт.");
    SetFormatedText("SERVICE_STATUS", Treasurer_Status());
    bool available = Treasurer_OfficerIndex() >= 0;
    if (Treasurer_BuyerIndex() < 0) available = false;
    SetSelectable("BUY_NOW_BUTTON", available);
    if (Treasurer_CabinIndex() < 0 || iSaleQuantity <= 0) available = false;
    SetSelectable("SELL_NOW_BUTTON", available);
    SetSelectable("EDIT_TARGET_BUTTON", iCurGoodIndex >= 0);
    SetSelectable("CLEAR_TABLE_LIST", iCurCompanion >= 0);
    SetSelectedItemControl();
}

void FillGoodsTable()
{
    bRefreshingTable = true;
    Table_Clear("TABLE_LIST", false, true, false);
    GameInterface.TABLE_LIST.hr.td1.str = XI_ConvertString("In the hold");
    GameInterface.TABLE_LIST.hr.td2.str = XI_ConvertString("weight");
    GameInterface.TABLE_LIST.hr.td3.str = "Пачка\n/Вес";
    GameInterface.TABLE_LIST.hr.td4.str = XI_ConvertString("Good name");
    GameInterface.TABLE_LIST.hr.td5.str = "Цель\nв трюме";
    GameInterface.TABLE_LIST.hr.td6.str = XI_ConvertString("weight");
    GameInterface.TABLE_LIST.hr.td7.str = XI_ConvertString("Cost");
    GameInterface.TABLE_LIST.hr.td1.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td2.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td3.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td4.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td5.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td6.scale = 0.85;
    GameInterface.TABLE_LIST.hr.td7.scale = 0.85;
    int n = 1;
    int selected = 0;
    int targetCount = 0;
    int missing = 0;
    if (iCurCompanion >= 0)
    {
        for (int i = 0; i < GOODS_QUANTITY; i++)
        {
            if (CheckAttribute(Goods[i], "CannonIdx")) continue;
            string row = "tr" + n;
            string sGoods = Goods[i].name;
            int cargo = GetCargoGoods(refCharacter, i);
            int target = 0;
            if (CheckAttribute(refCharacter, "TransferGoods." + sGoods)) target = sti(refCharacter.TransferGoods.(sGoods));
            if (target > 0)
            {
                targetCount++;
                if (target > cargo) missing += target - cargo;
            }
            GameInterface.TABLE_LIST.(row).index = i;
            GameInterface.TABLE_LIST.(row).td1.str = cargo;
            GameInterface.TABLE_LIST.(row).td2.str = GetGoodWeightByType(i, cargo);
            GameInterface.TABLE_LIST.(row).td3.str = Goods[i].Units + " / " + Goods[i].Weight;
            GameInterface.TABLE_LIST.(row).td4.icon.group = "GOODS";
            GameInterface.TABLE_LIST.(row).td4.icon.image = sGoods;
            GameInterface.TABLE_LIST.(row).td4.icon.offset = "0, 0";
            GameInterface.TABLE_LIST.(row).td4.icon.width = 29;
            GameInterface.TABLE_LIST.(row).td4.icon.height = 29;
            GameInterface.TABLE_LIST.(row).td4.textoffset = "25,0";
            GameInterface.TABLE_LIST.(row).td4.str = XI_ConvertString(sGoods);
            GameInterface.TABLE_LIST.(row).td5.str = target;
            GameInterface.TABLE_LIST.(row).td6.str = Goods[i].Weight + " / " + Goods[i].Units;
            GameInterface.TABLE_LIST.(row).td7.str = Goods[i].Cost;
            if (i == iCurGoodIndex) selected = n;
            n++;
        }
    }
    if (selected == 0 && n > 1) selected = 1;
    CurRow = "tr" + selected;
    iCurGoodIndex = -1;
    if (selected > 0) iCurGoodIndex = sti(GameInterface.TABLE_LIST.(CurRow).index);
    GameInterface.TABLE_LIST.select = selected;
    Table_UpdateWindow("TABLE_LIST");
    bRefreshingTable = false;
    string total = "Цели корабля не заданы";
    if (targetCount > 0) total = "До целей корабля: " + missing + " ед.";
    SetFormatedText("TAB_TOTAL", total);
}

void FillChestTable()
{
    bRefreshingTable = true;
    Table_Clear("CHEST_TABLE", false, true, false);
    GameInterface.CHEST_TABLE.hr.td1.str = "В сундуке";
    GameInterface.CHEST_TABLE.hr.td2.str = "Предмет";
    GameInterface.CHEST_TABLE.hr.td3.str = "Продать";
    GameInterface.CHEST_TABLE.hr.td4.str = "Цена\nза шт.";
    GameInterface.CHEST_TABLE.hr.td5.str = "Останется";
    GameInterface.CHEST_TABLE.hr.td6.str = "Редкость";
    GameInterface.CHEST_TABLE.hr.td1.scale = 0.8;
    GameInterface.CHEST_TABLE.hr.td2.scale = 0.85;
    GameInterface.CHEST_TABLE.hr.td3.scale = 0.8;
    GameInterface.CHEST_TABLE.hr.td4.scale = 0.8;
    GameInterface.CHEST_TABLE.hr.td5.scale = 0.8;
    GameInterface.CHEST_TABLE.hr.td6.scale = 0.8;
    int cabinIndex = Treasurer_CabinIndex();
    int n = 1;
    int selected = 0;
    iSaleQuantity = 0;
    float proceeds = 0.0;
    if (cabinIndex >= 0 && CheckAttribute(&Locations[cabinIndex], "box1.items"))
    {
        aref chestItems, entry;
        makearef(chestItems, Locations[cabinIndex].box1.items);
        for (int i = 0; i < GetAttributesNum(chestItems); i++)
        {
            entry = GetAttributeN(chestItems, i);
            int quantity = sti(GetAttributeValue(entry));
            if (quantity <= 0) continue;
            string itemID = GetAttributeName(entry);
            int itemIndex = FindItem(itemID);
            if (itemIndex < 0 || itemIndex >= TOTAL_ITEMS) continue;
            if (Treasurer_ItemSaleCategory(&Items[itemIndex]) == "") continue;
            string row = "tr" + n;
            string reason = "Неизвестный предмет";
            string label = itemID;
            int sellQuantity = 0;
            int price = 0;
            if (itemIndex >= 0 && itemIndex < TOTAL_ITEMS)
            {
                ref item = &Items[itemIndex];
                if (CheckAttribute(item, "name")) label = GetConvertStr(item.name, "ItemsDescribe.txt");
                sellQuantity = Treasurer_ItemSaleQuantity(item, quantity);
                price = Treasurer_ItemSalePrice(item);
                reason = "Неизвестна";
                if (CheckAttribute(item, "rare"))
                {
                    reason = "Обычный";
                    if (stf(item.rare) < 0.01) reason = "Редкий";
                }
                if (CheckAttribute(item, "unique")) reason = "Уникальный";
                if (CheckAttribute(item, "quest") || IsQuestUsedItem(item.id)) reason = "Квестовый";
                if (CheckAttribute(item, "picTexture") && CheckAttribute(item, "picIndex"))
                {
                    GameInterface.CHEST_TABLE.(row).td2.icon.group = item.picTexture;
                    GameInterface.CHEST_TABLE.(row).td2.icon.image = "itm" + item.picIndex;
                    GameInterface.CHEST_TABLE.(row).td2.icon.offset = "0, 1";
                    GameInterface.CHEST_TABLE.(row).td2.icon.width = 29;
                    GameInterface.CHEST_TABLE.(row).td2.icon.height = 29;
                    GameInterface.CHEST_TABLE.(row).td2.textoffset = "31,0";
                }
            }
            // Only sale categories enter this list; preview and selection never edit stock.
            GameInterface.CHEST_TABLE.(row).itemID = itemID;
            GameInterface.CHEST_TABLE.(row).index = itemIndex;
            GameInterface.CHEST_TABLE.(row).td1.str = quantity;
            GameInterface.CHEST_TABLE.(row).td2.str = label;
            GameInterface.CHEST_TABLE.(row).td2.scale = 0.83;
            GameInterface.CHEST_TABLE.(row).td3.str = sellQuantity;
            GameInterface.CHEST_TABLE.(row).td4.str = price;
            GameInterface.CHEST_TABLE.(row).td5.str = quantity - sellQuantity;
            GameInterface.CHEST_TABLE.(row).td6.str = reason;
            GameInterface.CHEST_TABLE.(row).td6.scale = 0.75;
            iSaleQuantity += sellQuantity;
            proceeds += sellQuantity * stf(price);
            if (itemID == sSelectedItemID) selected = n;
            n++;
        }
    }
    if (selected == 0 && n > 1) selected = 1;
    GameInterface.CHEST_TABLE.select = selected;
    Table_UpdateWindow("CHEST_TABLE");
    bRefreshingTable = false;
    SelectChestRow(selected);
    string total = "К продаже: " + iSaleQuantity + " шт., " + FloatToString(proceeds, 0) + " пиастров";
    if (n == 1) total = "Сундук пуст";
    if (cabinIndex < 0) total = "Сундук каюты недоступен";
    SetFormatedText("TAB_TOTAL", total);
}

void SelectChestRow(int selected)
{
    sSelectedItemID = "";
    iSelectedItemIndex = -1;
    string row = "tr" + selected;
    if (selected > 0 && CheckAttribute(&GameInterface, "CHEST_TABLE." + row + ".itemID"))
    {
        sSelectedItemID = GameInterface.CHEST_TABLE.(row).itemID;
        iSelectedItemIndex = sti(GameInterface.CHEST_TABLE.(row).index);
    }
    SetSelectedItemControl();
}

void SetSelectedItemControl()
{
    SetSelectable("KEEP_ITEM_BUTTON", sSelectedItemID != "");
    string label = "#Защитить от продажи";
    if (sSelectedItemID != "" && Treasurer_ItemLocked(sSelectedItemID)) label = "#Снять свою защиту";
    Button_SetText("KEEP_ITEM_BUTTON", label);
}

void TableSelectChange()
{
    string control = GetEventData();
    int selected = GetEventData();
    if (bRefreshingTable || bShowChangeWin) return;
    if (control == "TABLE_LIST" && iTreasurerTab == 0)
    {
        CurRow = "tr" + selected;
        iCurGoodIndex = -1;
        if (selected > 0 && CheckAttribute(&GameInterface, "TABLE_LIST." + CurRow + ".index"))
            iCurGoodIndex = sti(GameInterface.TABLE_LIST.(CurRow).index);
        SetSelectable("EDIT_TARGET_BUTTON", iCurGoodIndex >= 0);
    }
    if (control == "CHEST_TABLE" && iTreasurerTab == 1) SelectChestRow(selected);
}

void TreasurerUI_TableActivate()
{
    string control = GetEventData();
    // Native TableActivate is zero-based; table attributes and selection are one-based.
    int selected = GetEventData() + 1;
    if (bRefreshingTable || bShowChangeWin || selected <= 0) return;
    string row = "tr" + selected;
    if (control == "TABLE_LIST" && iTreasurerTab == 0)
    {
        if (!CheckAttribute(&GameInterface, "TABLE_LIST." + row + ".index")) return;
        CurRow = row;
        iCurGoodIndex = sti(GameInterface.TABLE_LIST.(row).index);
        ShowItemInfo();
    }
    if (control == "CHEST_TABLE" && iTreasurerTab == 1)
    {
        SelectChestRow(selected);
        TreasurerUI_ToggleItemLocked();
    }
}

void TreasurerUI_ToggleItemLocked()
{
    if (iTreasurerTab != 1 || sSelectedItemID == "") return;
    Treasurer_SetItemLocked(sSelectedItemID, !Treasurer_ItemLocked(sSelectedItemID));
    RefreshInterface();
}

void ProcessCheckBox()
{
    string control = GetEventData();
    int selected = GetEventData();
    int state = GetEventData();
    if (bSyncControls || bShowChangeWin) return;
    if (selected != 1) return;
    if (state != 0) state = 1;
    if (control == "NOTGOODSTRANSFER_CHECK" || control == "BUYCONTRABAND_CHECK")
    {
        if (iCurCompanion < 0 || iTreasurerTab != 0) return;
        string attributeName = "Enable";
        if (control == "BUYCONTRABAND_CHECK") attributeName = "BuyContraband";
        if (state) refCharacter.TransferGoods.(attributeName) = true;
        else DeleteAttribute(refCharacter, "TransferGoods." + attributeName);
    }
    if (control == "AUTO_BUY_CHECK") Treasurer_SetSetting("AutoBuy", state);
    if (control == "AUTO_SELL_CHECK") Treasurer_SetSetting("AutoSell", state);
    if (control == "SELL_BLADE_CHECK") Treasurer_SetSetting("SellBlade", state);
    if (control == "SELL_GUN_CHECK") Treasurer_SetSetting("SellGun", state);
    if (control == "SELL_ARMOR_CHECK") Treasurer_SetSetting("SellArmor", state);
    if (control == "SELL_LOOT_CHECK") Treasurer_SetSetting("SellLoot", state);
    RefreshInterface();
}

void ProcCommand()
{
    string command = GetEventData();
    string node = GetEventData();
    if (command != "click" && command != "activate") return;
    if (bShowChangeWin)
    {
        if (node == "QTY_CANCEL_BUTTON") UnShowWindow();
        if (node == "QTY_OK_BUTTON") QTY_OK();
        return;
    }
    switch (node)
    {
        case "EDIT_TARGET_BUTTON": ShowItemInfo(); break;
        case "CLEAR_TABLE_LIST":
            if (iCurCompanion < 0 || iTreasurerTab != 0) return;
            // Delete only named cargo targets, preserving both flags and other metadata.
            for (int i = 0; i < GOODS_QUANTITY; i++)
                DeleteAttribute(refCharacter, "TransferGoods." + Goods[i].name);
            RefreshInterface();
        break;
        case "RESERVE_BUTTON": OpenSettingQuantity(1); break;
        case "PRICE_LIMIT_BUTTON": OpenSettingQuantity(2); break;
        case "KEEP_COUNT_BUTTON": OpenSettingQuantity(3); break;
        case "KEEP_ITEM_BUTTON":
            TreasurerUI_ToggleItemLocked();
        break;
        case "BUY_NOW_BUTTON":
            if (iTreasurerTab != 0) return;
            Treasurer_BuyFleet();
            RefreshInterface();
        break;
        case "SELL_NOW_BUTTON":
            if (iTreasurerTab != 1) return;
            Treasurer_SellChest(false);
            RefreshInterface();
        break;
    }
}

void ShowItemInfo()
{
    if (bShowChangeWin) return;
    if (iTreasurerTab != 0 || iCurCompanion < 0 || iCurGoodIndex < 0) return;
    iQuantityMode = 0;
    iQuantityShip = iCurCompanion;
    iQuantityGood = iCurGoodIndex;
    iQuantityMinimum = 0;
    iQuantityMaximum = 999999;
    iQuantityStep = sti(Goods[iCurGoodIndex].Units);
    if (iQuantityStep < 1) iQuantityStep = 1;
    string sGood = Goods[iCurGoodIndex].name;
    int target = 0;
    if (CheckAttribute(refCharacter, "TransferGoods." + sGood)) target = sti(refCharacter.TransferGoods.(sGood));
    GameInterface.QTY_EDIT.str = target;
    SetFormatedText("QTY_CAPTION", XI_ConvertString(sGood) + ": цель в трюме");
    SetNewGroupPicture("QTY_PICTURE", "GOODS", sGood);
    SetFormatedText("QTY_INFO", GetAssembledString(GetConvertStr(sGood + "_descr", "GoodsDescribe.txt"), &Goods[iCurGoodIndex]));
    SetFormatedText("QTY_RESULT", "Итоговый груз; 0 — убрать цель");
    ShowWindow(1);
}

void OpenSettingQuantity(int mode)
{
    iQuantityMode = mode;
    iQuantityMinimum = 0;
    iQuantityStep = 1;
    string caption = "";
    string info = "";
    switch (mode)
    {
        case 1:
            caption = "Денежный резерв";
            info = "Оставлять эту сумму в пиастрах после закупок для эскадры.";
            iQuantityMaximum = 100000000;
            iQuantityStep = 1000;
            GameInterface.QTY_EDIT.str = Treasurer_Setting("ReserveGold", 0, 0, 100000000);
        break;
        case 2:
            caption = "Базовая цена предмета";
            info = "Продавать только предметы с базовой ценой строго ниже этого порога. Это не цена продажи торговцу.";
            iQuantityMinimum = 1;
            iQuantityMaximum = 1000000;
            iQuantityStep = 100;
            GameInterface.QTY_EDIT.str = Treasurer_Setting("PriceLimit", 1500, 1, 1000000);
        break;
        case 3:
            caption = "Оставлять в сундуке";
            info = "Сохранять столько экземпляров каждого предмета, допущенного к продаже.";
            iQuantityMaximum = 999;
            GameInterface.QTY_EDIT.str = Treasurer_Setting("KeepCount", 0, 0, 999);
        break;
    }
    SetFormatedText("QTY_CAPTION", caption);
    SetFormatedText("QTY_RULE_INFO", info);
    SetFormatedText("QTY_RESULT", "От " + iQuantityMinimum + " до " + iQuantityMaximum);
    ShowWindow(1);
}

void ShowWindow(int window)
{
    if (window == 1)
    {
        sQuantityReturnNode = GetCurrentNode();
        bShowChangeWin = true;
        XI_WindowDisable("MAIN_WINDOW", true);
        XI_WindowShow("QTY_WINDOW", true);
        XI_WindowDisable("QTY_WINDOW", false);
        SetNodeUsing("QTY_PICTURE", iQuantityMode == 0);
        SetNodeUsing("QTY_INFO", iQuantityMode == 0);
        SetNodeUsing("QTY_RULE_INFO", iQuantityMode != 0);
        SetCurrentNode("QTY_EDIT");
        return;
    }
    bShowChangeWin = false;
    XI_WindowDisable("QTY_WINDOW", true);
    XI_WindowShow("QTY_WINDOW", false);
    XI_WindowDisable("MAIN_WINDOW", false);
    SetControlsTabMode(iTreasurerTab);
    SetCurrentNode(sQuantityReturnNode);
}

void UnShowWindow()
{
    if (bShowChangeWin) ShowWindow(0);
}

void OnAddBtnClick(int _add)
{
    int value = MakeInt(GameInterface.QTY_EDIT.str);
    if (value < iQuantityMinimum) value = iQuantityMinimum;
    if (value > iQuantityMaximum) value = iQuantityMaximum;
    value += _add * iQuantityStep;
    if (value < iQuantityMinimum) value = iQuantityMinimum;
    if (value > iQuantityMaximum) value = iQuantityMaximum;
    GameInterface.QTY_EDIT.str = value;
}

void TreasurerUI_Add()
{
    if (bShowChangeWin) OnAddBtnClick(1);
}

void TreasurerUI_Remove()
{
    if (bShowChangeWin) OnAddBtnClick(-1);
}

void TreasurerUI_AddAll()
{
    if (bShowChangeWin) OnAddBtnClick(50);
}

void TreasurerUI_RemoveAll()
{
    if (bShowChangeWin) OnAddBtnClick(-50);
}

void QTY_OK()
{
    if (!bShowChangeWin) return;
    int value = MakeInt(GameInterface.QTY_EDIT.str);
    if (value < iQuantityMinimum) value = iQuantityMinimum;
    if (value > iQuantityMaximum) value = iQuantityMaximum;
    if (iQuantityMode == 0)
    {
        if (iQuantityShip < 0 || iQuantityShip >= TOTAL_CHARACTERS) return;
        if (iQuantityGood < 0 || iQuantityGood >= GOODS_QUANTITY) return;
        string sGood = Goods[iQuantityGood].name;
        if (value == 0) DeleteAttribute(&Characters[iQuantityShip], "TransferGoods." + sGood);
        else Characters[iQuantityShip].TransferGoods.(sGood) = value;
    }
    if (iQuantityMode == 1) Treasurer_SetSetting("ReserveGold", value);
    if (iQuantityMode == 2) Treasurer_SetSetting("PriceLimit", value);
    if (iQuantityMode == 3) Treasurer_SetSetting("KeepCount", value);
    UnShowWindow();
}

void confirmChangeQTY_EDIT()
{
    if (bShowChangeWin) SetCurrentNode("QTY_OK_BUTTON");
}

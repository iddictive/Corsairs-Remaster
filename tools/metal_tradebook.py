"""Exact-hash Metal trade journal layer; archived gameplay remains read-only."""
import hashlib

SCRIPT = "PROGRAM/interface/tradebook.c"
LAYOUT = "RESOURCE/INI/interfaces/tradebook.ini"
BASE = {
    SCRIPT: "59a239e2ebac0102dbf9cf0c810e65e5efbf0e6d84f119090b81c0660357e05c",
    LAYOUT: "a15e1e013cbe8ade5a6a9e80975e91af1b99627c2e34ceb080e8d70a78916fe1",
}
PREVIOUS = {
    SCRIPT: "e4572beabccda273daca89bfee8270ac633c6a1b3ffbdd08a12b46e33a2340c6",
    LAYOUT: "2cba66d380e5de28ed5ffc00464747c4d40aac09b0cee3425da2d6e1132190b9",
}

CONTROLS = '''void SyncTradeBookControls()
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

'''

INIT = '''void InitTradeComparison()
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

'''


def replace(text, old, new, count=1):
    if text.count(old) != count:
        raise RuntimeError(f"tradebook anchor mismatch: {old[:90]!r}")
    return text.replace(old, new)


def script(text):
    text = replace(text, 'int TradeBookRouteMode = 1;', 'int TradeBookRouteMode = 1;\nint TradeBookContraband = 0;\nint TradeBookPinAuto = 0;')
    text = replace(text, '\t\tcase "PIN_CITY":', '''        case "CONTRABAND_TOGGLE":
            if(comName == "click" || comName == "activate")
            {
                ToggleTradeBookContraband();
                return;
            }
        break;
\t\tcase "PIN_CITY":''')
    start, end = text.index('void SyncTradeBookControls()'), text.index('bool TryPinCurrentTradeCity()')
    text = text[:start] + CONTROLS + text[end:]
    start, end = text.index('void InitTradeComparison()'), text.index('void RefreshTradeComparison()')
    text = text[:start] + INIT + text[end:]
    text = replace(text, '\tTradeBookPinnedCity = currentTown;\n\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;', '\tTradeBookPinnedCity = currentTown;\n\tTradeBookPinAuto = 1;\n\tpchar.SystemInfo.TradeBookPinnedCity = TradeBookPinnedCity;\n\tpchar.SystemInfo.TradeBookPinAuto = 1;')
    text = replace(text, 'void RefreshTradeComparison()\n{', 'void RefreshTradeComparison()\n{\n    SyncTradeBookControls();')
    text = replace(text, '\tif (TradeBookPinnedCity == "" || TradeBookPinnedCity == TradeBookSelectedCity)', '''    if (TradeBookPinnedCity == "")
    {
        SetFormatedText("TRADE_ROUTE", "Выберите город отправления и закрепите его");
        FillPriceList("TABLE_GOODS", TradeBookSelectedCity);
        return;
    }
\tif (TradeBookPinnedCity == TradeBookSelectedCity)''')
    # Ordinary prices remain an inventory view, with explicit per-centner labels.
    text = replace(text, 'XI_ConvertString("Price sell")', '"Купить/ц"')
    text = replace(text, 'XI_ConvertString("Price buy")', '"Продать/ц"')
    text = replace(text, 'XI_ConvertString("In the store")', '"Запас, ц"')
    text = replace(text, '"Пачка"', '"Шт./ц"')
    text = replace(text, '"Вес пачки"', '"Мин., ц"')
    for side, col in [('Buy', 3), ('Sell', 4)]:
        text = replace(text, f'GameInterface.(_tabName).(row).td{col}.str = nulChr.PriceList.(attr1).(sGoods).{side};', f'GameInterface.(_tabName).(row).td{col}.str = TradeBookPricePerWeight(nulChr.PriceList.(attr1).(sGoods).{side}, i);')
    text = replace(text, 'GameInterface.(_tabName).(row).td5.str = nulChr.PriceList.(attr1).(sGoods).Qty;', '''GameInterface.(_tabName).(row).td5.str = "???";
                if (TradeBookKnownPrice(nulChr.PriceList.(attr1).(sGoods).Qty))
                    GameInterface.(_tabName).(row).td5.str = FloatToString(stf(nulChr.PriceList.(attr1).(sGoods).Qty) * stf(Goods[i].Weight) / stf(Goods[i].Units), 1);''')
    text = replace(text, 'GameInterface.(_tabName).(row).td6.str = Goods[i].Units;', 'GameInterface.(_tabName).(row).td6.str = FloatToString(stf(Goods[i].Units) / stf(Goods[i].Weight), 1);')
    text = replace(text, 'sGoods = "Gidx" + i;', 'sGoods = "Gidx" + i;\n            if (sti(nulChr.PriceList.(attr1).(sGoods).TradeType) == TRADE_TYPE_CONTRABAND && TradeBookContraband == 0) continue;')
    text = replace(text, '== TRADE_TYPE_CONTRABAND && !bBettaTestMode', '== TRADE_TYPE_CONTRABAND && TradeBookContraband == 0', 2)
    score = '\t\tgoodsScore[goodsCount] = (stf(nulChr.PriceList.(targetCity).(goodsAttr).Sell) - stf(nulChr.PriceList.(sourceCity).(goodsAttr).Buy)) / stf(Goods[i].Weight);'
    text = replace(text, score, '''        goodsScore[goodsCount] = -1000000000.0;
        if (TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Buy) && TradeBookKnownPrice(nulChr.PriceList.(targetCity).(goodsAttr).Sell))
    ''' + score.strip())
    text = replace(text, '"Купить";', '"Купить/ц";')
    text = replace(text, '"Продать";', '"Продать/ц";')
    text = replace(text, '"Взять";', '"Взять, ц";')
    text = replace(text, '\t\tint buyPrice = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Buy);\n\t\tint sellPrice = sti(nulChr.PriceList.(targetCity).(goodsAttr).Sell);', '''        int buyPrice = 0;
        int sellPrice = 0;
        bool knownPrices = TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Buy) && TradeBookKnownPrice(nulChr.PriceList.(targetCity).(goodsAttr).Sell);
        if (knownPrices)
        {
            buyPrice = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Buy);
            sellPrice = sti(nulChr.PriceList.(targetCity).(goodsAttr).Sell);
        }''')
    text = replace(text, '\t\t\tint packsLeft = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Qty) / unitSize;', '''            int packsLeft = 0;
            if (TradeBookKnownPrice(nulChr.PriceList.(sourceCity).(goodsAttr).Qty))
                packsLeft = sti(nulChr.PriceList.(sourceCity).(goodsAttr).Qty) / unitSize;''')
    text = replace(text, 'GameInterface.(tableName).(row).td3.str = buyPrice;', 'GameInterface.(tableName).(row).td3.str = TradeBookPricePerWeight(nulChr.PriceList.(sourceCity).(goodsAttr).Buy, goodIndex);')
    text = replace(text, 'GameInterface.(tableName).(row).td4.str = sellPrice;', 'GameInterface.(tableName).(row).td4.str = TradeBookPricePerWeight(nulChr.PriceList.(targetCity).(goodsAttr).Sell, goodIndex);')
    text = replace(text, 'plannedPacks * unitSize;', 'plannedPacks * packWeight;')
    text = replace(text, '\t\tif (profitPerPack > 0)\n', '''        if (!knownPrices)
        {
            GameInterface.(tableName).(row).td5.str = "???";
            GameInterface.(tableName).(row).td7.str = "???";
        }
        if (sti(nulChr.PriceList.(targetCity).(goodsAttr).TradeType) == TRADE_TYPE_CONTRABAND)
            GameInterface.(tableName).(row).td2.icon.image = "ico_" + TRADE_TYPE_CONTRABAND;
\t\tif (profitPerPack > 0)
''')
    text = replace(text, 'Тип товара показан значком. В сравнении зелёная прибыль выгодна, красная означает убыток.', 'Цены — за центнер. Взять — вес в центнерах. Прибыль — за весь запланированный груз по записанным ценам. Неизвестные цены не входят в план.')
    text = replace(text, '\t\tcase "TABLE_GOODS":\n', '''        case "PIN_CITY":
            sHeader = "Закрепить город";
            sText1 = "Закрепляет выбранный город отправления. Открепить — снять закрепление.";
            sGroup = "NET_SERVERPASSWORD"; sGroupPicture = "locked";
        break;
        case "CONTRABAND_TOGGLE":
            sHeader = "Контрабанда";
            sText1 = "Показать или скрыть контрабанду. Неизвестные цены не участвуют в плане.";
            sGroup = "TRADE_TYPE"; sGroupPicture = "ico_" + TRADE_TYPE_CONTRABAND;
        break;
\t\tcase "TABLE_GOODS":
''')
    # Table_Clear removes header scales: set them explicitly in both modes.
    for owner in ("_tabName", "tableName"):
        anchor = f'GameInterface.({owner}).hr.td1.str = XI_ConvertString("Good name");'
        scales = "\n".join(f'    GameInterface.({owner}).hr.td{col}.scale = {0.65 if col == 2 else 0.75};' for col in range(1, 8))
        text = replace(text, anchor, f'GameInterface.({owner}).hr.td1.str = "Товар";\n' + scales)
    # Remove the ordinary-mode overrides so both modes share identical metrics.
    for col in range(1, 8):
        old_scale = "0.85" if col == 2 else "0.8"
        text = replace(text, f'GameInterface.(_tabName).hr.td{col}.scale = {old_scale};', '')
    # The formatted-text node resets alignment when its content changes.
    text = text.replace('SetFormatedText("TRADE_ROUTE", ', 'TradeBookRouteText(')
    text = replace(text, 'TradeBookRouteText(text);', 'SetFormatedText("TRADE_ROUTE", text);')
    return text


def layout(text):
    text = replace(text, 'item = 210,TEXTBUTTON2,PIN_CITY', 'item = 210,TEXTBUTTON2,PIN_CITY\nitem = 210,TEXTBUTTON2,CONTRABAND_TOGGLE')
    text = replace(text, 'position = 20,86,158,112', 'position = 20,86,110,112')
    text = replace(text, 'lineSpace = 1', 'lineSpace = 12\nvalignment = 1')
    start, end = text.index('[PIN_CITY]'), text.index('[TRADE_ROUTE]')
    text = text[:start] + '''[PIN_CITY]
command = click
command = activate
command = rclick,select:PIN_CITY,event:ShowInfoWindow
command = deactivate,event:exitCancel
position = 114,86,202,112
string = Ok
fontScale = 0.68
glowoffset = 0,0

[CONTRABAND_TOGGLE]
command = click
command = activate
command = rclick,select:CONTRABAND_TOGGLE,event:ShowInfoWindow
command = deactivate,event:exitCancel
position = 206,86,316,112
string = Ok
fontScale = 0.55
glowoffset = 0,0

''' + text[end:]
    return text


def prepare(relative, data):
    if hashlib.sha256(data).hexdigest() != BASE[relative]:
        raise RuntimeError(f"unrecognized trade journal baseline: {relative}")
    encoding = 'utf-8' if relative == SCRIPT else 'cp1251'
    text = data.decode(encoding).replace('\r\n', '\n')
    result = script(text) if relative == SCRIPT else layout(text)
    return result.replace('\n', '\r\n').encode(encoding)

// One owner for treasurer preview, manual orders and automatic port service.
#event_handler("TreasurerPortService", "Treasurer_PortService");
int Treasurer_PortGeneration = 0;
bool Treasurer_ServiceBatch = false;
int Treasurer_ServiceMinutes = 0;

bool Treasurer_BeginService()
{
    if (Treasurer_ServiceBatch) return false;
    Treasurer_ServiceBatch = true;
    Treasurer_ServiceMinutes = 0;
    return true;
}

void Treasurer_ChargeTime(int minutes)
{
    if (Treasurer_ServiceBatch) Treasurer_ServiceMinutes += minutes;
    else WaitDate("", 0, 0, 0, minutes / 60, minutes % 60);
}

void Treasurer_EndService(bool owner)
{
    if (!owner) return;
    int minutes = Treasurer_ServiceMinutes;
    Treasurer_ServiceBatch = false;
    Treasurer_ServiceMinutes = 0;
    if (minutes > 0) WaitDate("", 0, 0, 0, minutes / 60, minutes % 60);
}

int Treasurer_OfficerIndex()
{
    if (!CheckAttribute(pchar, "Fellows.Passengers.treasurer")) return -1;
    int index = sti(pchar.Fellows.Passengers.treasurer);
    if (index < 0 || index >= TOTAL_CHARACTERS) return -1;
    ref officer = GetCharacter(index);
    if (LAi_IsDead(officer) || !GetRemovable(officer)) return -1;
    if (CheckAttribute(officer, "prisoned") && sti(officer.prisoned)) return -1;
    return index;
}

int Treasurer_Setting(string key, int fallback, int minimum, int maximum)
{
    int value = fallback;
    if (CheckAttribute(pchar, "Treasury." + key)) value = sti(pchar.Treasury.(key));
    if (value < minimum) value = minimum;
    if (value > maximum) value = maximum;
    return value;
}

void Treasurer_SetSetting(string key, int value)
{
    pchar.Treasury.(key) = value;
}

int Treasurer_CabinIndex()
{
    if (!CheckAttribute(pchar, "Ship.Type") || sti(pchar.Ship.Type) == SHIP_NOTUSED) return -1;
    int index = FindLocation(Get_My_Cabin());
    if (index < 0 || !CheckAttribute(&Locations[index], "box1.items")) return -1;
    return index;
}

// Remote port service must obey the same store door, NPC and quest restrictions.
bool Treasurer_StoreDoorOpen(ref buyer)
{
    int storeLocation = FindLocation(buyer.location);
    if (storeLocation < 0 || !CheckAttribute(&Locations[storeLocation], "type")) return false;
    if (Locations[storeLocation].type != "store") return false;
    int town = FindLocation(buyer.City + "_town");
    if (town < 0 || !CheckAttribute(&Locations[town], "reload")) return false;
    aref doors;
    makearef(doors, Locations[town].reload);
    for (int index = 0; index < GetAttributesNum(doors); index++)
    {
        aref door = GetAttributeN(doors, index);
        if (!CheckAttribute(door, "go") || !CheckAttribute(door, "name")) continue;
        if (door.go == buyer.location) return chrCheckReload(&Locations[town], door.name);
    }
    return false;
}

string Treasurer_ServiceReason()
{
    if (Treasurer_OfficerIndex() < 0) return "Назначьте казначея на флагман.";
    if (!CheckAttribute(pchar, "Ship.Type") || sti(pchar.Ship.Type) == SHIP_NOTUSED) return "Нужен корабль.";
    if (bSeaActive || bAbordageStarted) return "Торговля доступна в колонии.";
    if (LAi_grp_alarmactive || LAi_grp_playeralarm > 0 || LAi_IsFightMode(pchar)) return "Торговля недоступна во время боя или тревоги.";
    if (CheckAttribute(pchar, "questTemp.CapBloodLine") && sti(pchar.questTemp.CapBloodLine)) return "Торговля пока недоступна по сюжету.";
    int location = FindLocation(pchar.location);
    if (location < 0 || !CheckAttribute(&Locations[location], "fastreload")) return "Нужна стоянка в колонии.";
    if (CheckAttribute(&Locations[location], "boarding") || IsLocationCaptured(pchar.location)) return "Торговля недоступна во время захвата.";
    return "";
}

int Treasurer_BuyerIndex()
{
    if (Treasurer_ServiceReason() != "") return -1;
    string city = GetCurrentTown();
    if (city == "" || FindStore(city) < 0) return -1;
    ref buyer;
    for (int index = 0; index < TOTAL_CHARACTERS; index++)
    {
        buyer = GetCharacter(index);
        if (!CheckAttribute(buyer, "Dialog.Filename") || buyer.Dialog.Filename != "Common_Store.c") continue;
        if (!CheckAttribute(buyer, "City") || buyer.City != city) continue;
        if (!CheckAttribute(buyer, "location") || LAi_IsDead(buyer)) continue;
        if (!CheckAttribute(buyer, "nation")) continue;
        if (GetNationRelation2MainCharacter(sti(buyer.nation)) == RELATION_ENEMY) continue;
        if (CheckAttribute(buyer, "angry")) continue;
        if (CheckFreeServiceForNPC(buyer, "Store") != -1) continue;
        if (!CheckAttribute(buyer, "Dialog.CurrentNode")) continue;
        string node = buyer.Dialog.CurrentNode;
        if (node != "First time" && node != "first time" && node != "Second time" && node != "second time" && node != "market") continue;
        if (!Treasurer_StoreDoorOpen(buyer)) continue;
        if (CheckAttribute(buyer, "location.stime") && CheckAttribute(buyer, "location.etime"))
        {
            if (!LAi_login_CheckTime(stf(buyer.location.stime), stf(buyer.location.etime))) continue;
        }
        return index;
    }
    return -1;
}

bool Treasurer_ItemLocked(string itemID)
{
    return CheckAttribute(pchar, "Treasury.KeepItems." + itemID);
}

void Treasurer_SetItemLocked(string itemID, bool locked)
{
    if (locked) pchar.Treasury.KeepItems.(itemID) = true;
    else DeleteAttribute(pchar, "Treasury.KeepItems." + itemID);
}

bool Treasurer_ItemNeededBy(ref officer, ref item)
{
    string equipped = GetCharacterEquipByGroup(officer, item.groupID);
    if (equipped == item.id) return true;
    if (OfficerSupply_IsEquipmentLocked(officer, item.groupID, equipped)) return false;
    float score = OfficerSupply_ItemScore(officer, item, item.groupID);
    if (score < 0.0) return false;
    if (equipped == "" || equipped == "unarmed") return true;
    aref current;
    if (Items_FindItem(equipped, &current) < 0) return true;
    return score > OfficerSupply_ItemScore(officer, current, item.groupID);
}

bool Treasurer_ItemNeeded(ref item)
{
    if (Treasurer_ItemNeededBy(pchar, item)) return true;
    int index;
    ref officer;
    for (int slot = 0; slot < GetPassengersQuantity(pchar); slot++)
    {
        index = GetPassenger(pchar, slot);
        if (index < 0) continue;
        officer = GetCharacter(index);
        if (!GetRemovable(officer) || LAi_IsDead(officer)) continue;
        if (CheckAttribute(officer, "prisoned") && sti(officer.prisoned)) continue;
        if (Treasurer_ItemNeededBy(officer, item)) return true;
    }
    for (int ship = 1; ship < COMPANION_MAX; ship++)
    {
        index = GetCompanionIndex(pchar, ship);
        if (index < 0) continue;
        officer = GetCharacter(index);
        if (!FleetService_IsShip(officer)) continue;
        if (Treasurer_ItemNeededBy(officer, item)) return true;
    }
    return false;
}

string Treasurer_ItemSaleCategory(ref item)
{
    if (!CheckAttribute(item, "id")) return "";
    if (CheckAttribute(item, "groupID"))
    {
        if (item.groupID == BLADE_ITEM_TYPE) return "SellBlade";
        if (item.groupID == GUN_ITEM_TYPE) return "SellGun";
        if (item.groupID == CIRASS_ITEM_TYPE) return "SellArmor";
        return "";
    }
    // The ordinary valuables/minerals families in initItems; amulets and supplies stay separate.
    if (findsubstr(item.id, "jewelry", 0) == 0 || findsubstr(item.id, "mineral", 0) == 0) return "SellLoot";
    return "";
}

string Treasurer_ItemSaleReason(ref item)
{
    if (!CheckAttribute(item, "id")) return "Неизвестная вещь";
    if (Treasurer_ItemLocked(item.id)) return "Сохранять";
    if (CheckAttribute(item, "quest") || CheckAttribute(item, "unique")) return "Квестовая вещь";
    if (CheckAttribute(item, "ItemType"))
    {
        if (item.ItemType == "QUESTITEMS" || item.ItemType == "MAP" || item.ItemType == "BOOK") return "Квест / карта / книга";
    }
    string category = Treasurer_ItemSaleCategory(item);
    if (category == "" || item.id == "unarmed" || item.id == "CaptainBook") return "Не для автосбыта";
    if (!CheckAttribute(item, "price") || sti(item.price) <= 0 || IsQuestUsedItem(item.id)) return "Квестовая вещь";
    if (!CheckAttribute(item, "rare")) return "Редкость неизвестна";
    if (stf(item.rare) < 0.01) return "Редкая вещь";
    if (category != "SellLoot" && Treasurer_ItemNeeded(item)) return "Нужно вам / офицеру";
    if (!Treasurer_Setting(category, 1, 0, 1)) return "Категория отключена";
    if (sti(item.price) >= Treasurer_Setting("PriceLimit", 1500, 1, 1000000)) return "Дороже предела";
    return "";
}

int Treasurer_ItemSaleQuantity(ref item, int quantity)
{
    if (Treasurer_ItemSaleReason(item) != "") return 0;
    quantity -= Treasurer_Setting("KeepCount", 1, 0, 999);
    if (quantity < 0) quantity = 0;
    if (quantity > 1000000) quantity = 1000000;
    return quantity;
}

int Treasurer_ItemSalePrice(ref item)
{
    if (!CheckAttribute(item, "price")) return 0;
    // Ordinary itemstrade PRICE_TYPE_SELL, including the existing perk bonus.
    float modifier = 0.75 + GetSummonSkillFromNameToOld(pchar, SKILL_COMMERCE) * 0.019;
    if (CheckOfficersPerk(pchar, "AdvancedCommerce")) modifier += 0.05;
    return makeint(sti(item.price) * modifier);
}

void Treasurer_Report(string text)
{
    pchar.Treasury.Last.city = GetCurrentTown();
    pchar.Treasury.Last.text = text;
}

string Treasurer_Status()
{
    string reason = Treasurer_ServiceReason();
    if (reason != "") return reason;
    if (Treasurer_BuyerIndex() < 0) return "Магазин закрыт, занят или торговец недоступен.";
    if (CheckAttribute(pchar, "Treasury.Last.city") && pchar.Treasury.Last.city == GetCurrentTown()) return pchar.Treasury.Last.text;
    return "Торговец доступен.";
}

int Treasurer_SellChest(bool automatic)
{
    int buyerIndex = Treasurer_BuyerIndex();
    if (buyerIndex < 0) return 0;
    int cabin = Treasurer_CabinIndex();
    if (cabin < 0) { if (!automatic) Treasurer_Report("Рундук пуст или недоступен."); return 0; }
    ref buyer = GetCharacter(buyerIndex);
    aref chest, entries;
    makearef(chest, Locations[cabin].box1);
    makearef(entries, Locations[cabin].box1.items);
    int proceeds = 0;
    int sold = 0;
    ref item;
    for (int index = GetAttributesNum(entries) - 1; index >= 0; index--)
    {
        aref entry = GetAttributeN(entries, index);
        string itemID = GetAttributeName(entry);
        aref found;
        int itemIndex = Items_FindItem(itemID, &found);
        if (itemIndex < 0) continue;
        item = &Items[itemIndex];
        int quantity = Treasurer_ItemSaleQuantity(item, sti(GetAttributeValue(entry)));
        int price = Treasurer_ItemSalePrice(item);
        if (quantity < 1 || price < 1) continue;
        // Keep every quantity/money sum inside the script VM's signed integers.
        int affordable = makeint((2000000000.0 - sti(pchar.Money) - proceeds) / price);
        if (quantity > affordable) quantity = affordable;
        int buyerQuantity = GetCharacterItem(buyer, itemID);
        if (quantity < 1 || buyerQuantity > 2000000000 - quantity) continue;
        if (!TakeNItems(buyer, itemID, quantity)) continue;
        if (GetCharacterItem(buyer, itemID) != buyerQuantity + quantity) continue;
        if (!TakeNItems(chest, itemID, -quantity))
        {
            TakeNItems(buyer, itemID, -quantity);
            continue;
        }
        TraderStock_RecordPlayerSale(buyer, itemID, quantity);
        sold += quantity;
        proceeds += price * quantity;
    }
    if (proceeds > 0)
    {
        AddMoneyToCharacter(pchar, proceeds);
        Statistic_AddValue(pchar, "Money_get", proceeds);
        AddCharacterExpToSkill(&Characters[Treasurer_OfficerIndex()], "Commerce", proceeds / 1000.0);
        Treasurer_Report("Продано из рундука: " + sold + " шт., выручено " + FindRussianMoneyString(proceeds) + ".");
        Treasurer_ChargeTime(5);
    }
    else { if (!automatic) Treasurer_Report("Нет вещей для продажи по выбранным правилам."); }
    return proceeds;
}

// One purchase ledger for the store button and automatic fleet orders.
int Treasurer_TradeShip(ref captain, ref store, bool sellExcess)
{
    if (Treasurer_OfficerIndex() < 0 || !FleetService_IsShip(captain)) return 0;
    if (CheckAttribute(captain, "TransferGoods.Enable")) return 0;
    int spent = 0;
    int earned = 0;
    ref good;
    for (int index = 0; index < GOODS_QUANTITY; index++)
    {
        good = &Goods[index];
        string name = good.name;
        if (CheckAttribute(good, "CannonIdx")) continue;
        if (!CheckAttribute(captain, "TransferGoods." + name)) continue;
        int target = sti(captain.TransferGoods.(name));
        if (target < 0 || target > 999999) continue;
        int current = GetCargoGoods(captain, index);
        int units = sti(good.Units);
        float weight = stf(good.Weight);
        if (units < 1 || weight <= 0.0) continue;
        int tradeType = TRADE_TYPE_NORMAL;
        if (CheckAttribute(store, "goods." + name + ".tradetype")) tradeType = sti(store.goods.(name).tradetype);
        // Legacy explicit excess-sale policy stays manual, never a port default.
        if (sellExcess && CheckAttribute(captain, "TransferGoods.SellRestriction") && current > target)
        {
            if (tradeType == TRADE_TYPE_CONTRABAND || tradeType == TRADE_TYPE_CANNONS) continue;
            int salePrice = GetStoreGoodsPrice(store, index, PRICE_TYPE_SELL, pchar, 1);
            int saleQuantity = current - target;
            int saleCost = makeint(salePrice * 1.0 * saleQuantity / units);
            if (saleCost <= 0 || saleCost > 2000000000 - sti(pchar.Money)) continue;
            if (!RemoveCharacterGoodsSelf(captain, index, saleQuantity)) continue;
            AddStoreGoods(store, index, saleQuantity);
            AddMoneyToCharacter(pchar, saleCost);
            earned += saleCost;
            Treasurer_ChargeTime(1);
            continue;
        }
        if (current >= target) continue;
        if (tradeType == TRADE_TYPE_CANNONS) continue;
        if (tradeType == TRADE_TYPE_CONTRABAND && !CheckAttribute(captain, "TransferGoods.BuyContraband")) continue;
        int packPrice = GetStoreGoodsPrice(store, index, PRICE_TYPE_BUY, pchar, 1);
        if (packPrice < 1) continue;
        int quantity = target - current;
        int stock = GetStoreGoodsQuantity(store, index);
        if (quantity > stock) quantity = stock;
        int room = GetGoodQuantityByWeight(index, GetCargoFreeSpace(captain));
        if (quantity > room) quantity = room;
        int budget = sti(pchar.Money) - Treasurer_Setting("ReserveGold", 0, 0, 100000000);
        if (budget <= 0) continue;
        int affordable = makeint(budget * 1.0 * units / packPrice);
        if (quantity > affordable) quantity = affordable;
        if (quantity < 1) continue;
        // Round payment upwards: buying part of a cheap ammunition pack is not free.
        float exactCost = packPrice * 1.0 * quantity / units;
        int cost = makeint(exactCost);
        if (cost < exactCost) cost++;
        if (cost < 1 || cost > budget) continue;
        AddCharacterGoodsSimple(captain, index, quantity);
        if (GetCargoGoods(captain, index) != current + quantity)
        {
            SetCharacterGoods(captain, index, current);
            continue;
        }
        RemoveStoreGoods(store, index, quantity);
        AddMoneyToCharacter(pchar, -cost);
        spent += cost;
        Treasurer_ChargeTime(1);
    }
    if (spent > 0 || earned > 0)
    {
        AddCharacterExpToSkill(&Characters[Treasurer_OfficerIndex()], "Commerce", makeint((spent + earned) / 800.0) + 2);
    }
    return spent;
}

int Treasurer_BuyFleet()
{
    int buyer = Treasurer_BuyerIndex();
    if (buyer < 0) return 0;
    int storeIndex = FindStore(Characters[buyer].City);
    if (storeIndex < 0) return 0;
    bool batch = Treasurer_BeginService();
    int spent = 0;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int captain = GetCompanionIndex(pchar, slot);
        if (captain < 0) continue;
        spent += Treasurer_TradeShip(&Characters[captain], &Stores[storeIndex], false);
    }
    Treasurer_EndService(batch);
    if (spent > 0) Treasurer_Report("Закуплено для эскадры на " + FindRussianMoneyString(spent) + ".");
    else Treasurer_Report("Заказы выполнены либо не хватает товара, места или свободных денег.");
    return spent;
}

void Treasurer_OnLocationLoaded(ref location)
{
    Treasurer_PortGeneration++;
    // Save rehydration is not a new arrival; old saves opt in only through the UI.
    if (actLoadFlag != 0 || bAbordageStarted) return;
    if (!CheckAttribute(location, "type") || CheckAttribute(location, "boarding")) return;
    if (location.type != "port" && location.type != "town" && location.type != "store") return;
    if (!Treasurer_Setting("AutoBuy", 0, 0, 1) && !Treasurer_Setting("AutoSell", 0, 0, 1)) return;
    PostEvent("TreasurerPortService", 500, "lsl", Treasurer_PortGeneration, location.id, 0);
}

void Treasurer_PortService()
{
    int generation = GetEventData();
    string locationID = GetEventData();
    int retries = GetEventData();
    if (generation != Treasurer_PortGeneration || locationID != pchar.location || actLoadFlag != 0) return;
    if (dialogRun || sti(InterfaceStates.Launched))
    {
        if (retries < 15) PostEvent("TreasurerPortService", 2000, "lsl", generation, locationID, retries + 1);
        return;
    }
    if (Treasurer_BuyerIndex() < 0) return;
    bool batch = Treasurer_BeginService();
    int proceeds = 0;
    int spent = 0;
    if (Treasurer_Setting("AutoSell", 0, 0, 1)) proceeds = Treasurer_SellChest(true);
    if (Treasurer_Setting("AutoBuy", 0, 0, 1)) spent = Treasurer_BuyFleet();
    Treasurer_EndService(batch);
    if (proceeds > 0 || spent > 0)
    {
        Treasurer_Report("Казначей: продажа " + proceeds + ", закупки " + spent + " пиастров.");
        Log_Info(pchar.Treasury.Last.text);
    }
}

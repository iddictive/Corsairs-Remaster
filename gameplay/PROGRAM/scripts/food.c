#define I_MIN_MORALE	10

// boal -->
#define FOOD_BY_CREW       10.0
#define FOOD_BY_SLAVES     20.0
#define FOOD_BY_PASSENGERS 10.0
#define RUM_BY_CREW        20.0
// boal <--

//bool bInterfaceFood = false;
//int iFoodQuantity = 0;

// вернёт число дней на сколько есть еда на всех кораблях
int CalculateFood()
{
	int chrIndex;
	int iCrewQuantity = 0;
	float foodNeeded = 0;
	int iSlavesQuantity = 0;
	int iFoodQuantity = 0;

	for (int i=0; i<COMPANION_MAX; i++)
	{
		chrIndex = GetCompanionIndex(pchar, i);
		if (chrIndex != -1)
		{
            if (!GetRemovable(&characters[chrIndex])) continue;
			if (characters[chrIndex].ship.type != SHIP_NOTUSED)
			{
				iCrewQuantity   = iCrewQuantity   + sti(characters[chrIndex].ship.crew.quantity);
				iFoodQuantity   = iFoodQuantity   + GetCargoGoods(&characters[chrIndex], GOOD_FOOD);
				iSlavesQuantity = iSlavesQuantity + GetCargoGoods(&characters[chrIndex], GOOD_SLAVES);
			}
		}
	}
	int iPassQuantity = GetPassengersQuantity(pchar);

	foodNeeded = makefloat(iCrewQuantity/FOOD_BY_CREW + iPassQuantity/FOOD_BY_PASSENGERS + iSlavesQuantity/FOOD_BY_SLAVES);

	if (foodNeeded < 1)
	{
		foodNeeded = 1;
	}

	iFoodQuantity = makeint(iFoodQuantity/foodNeeded + 0.2);

	return iFoodQuantity;
}

// еды на одном корабле
int CalculateShipFood(ref _chr)
{
	int iCrewQuantity = 0;
	float foodNeeded = 0;
	int iSlavesQuantity = 0;
	int iFoodQuantity = 0;
	int iPassQuantity = 0;

	iCrewQuantity   =  sti(_chr.ship.crew.quantity);
	iFoodQuantity   =  GetCargoGoods(_chr, GOOD_FOOD);
	iSlavesQuantity =  GetCargoGoods(_chr, GOOD_SLAVES);
	if (_chr.id == pchar.id) 
	{
		iPassQuantity = GetPassengersQuantity(pchar);
	}

	foodNeeded = makefloat(iCrewQuantity/FOOD_BY_CREW + iPassQuantity/FOOD_BY_PASSENGERS + iSlavesQuantity/FOOD_BY_SLAVES);

	if (foodNeeded < 1)
	{
		foodNeeded = 1;
	}

	iFoodQuantity = makeint(iFoodQuantity/foodNeeded + 0.2);

	return iFoodQuantity;
}

// Warship 11.07.09 Вернёт кол-во дней, на сколько хватит рому на одном корабле
int CalculateShipRum(ref _character)
{
	int crewQuantity = GetCrewQuantity(_character);
	int rumQuantity = GetCargoGoods(_character, GOOD_RUM);
	float rumNeeded = makefloat((crewQuantity + 5.1) / RUM_BY_CREW); // Сколько жрут за день
	
	if(rumNeeded < 1.0) rumNeeded = 1.0;	
	rumQuantity = makeint(rumQuantity/rumNeeded + 0.2);		
	return rumQuantity;
}

// Ugeen  29.10.10 вернёт число дней на сколько есть рому на всех кораблях
int CalculateRum()
{
	int chrIndex;
	int iCrewQuantity = 0;
	int iRumCount = 0;
	float RumNeeded = 0;

	for (int i=0; i<COMPANION_MAX; i++)
	{
		chrIndex = GetCompanionIndex(pchar, i);
		if (chrIndex != -1)
		{
            if (!GetRemovable(&characters[chrIndex])) continue;
			if (characters[chrIndex].ship.type != SHIP_NOTUSED)
			{
				iCrewQuantity += GetCrewQuantity(&characters[chrIndex]);
				iRumCount     += makeint(GetCargoGoods(&characters[chrIndex], GOOD_RUM));							
			}
		}
	}
	RumNeeded = makefloat(iCrewQuantity/RUM_BY_CREW);

	if (RumNeeded < 1.0)
	{
		RumNeeded = 1.0;
	}

	return makeint(iRumCount/RumNeeded + 0.2);
}

// boal 21.04.04 крысы на корабле -->
void DailyRatsEatGoodsUpdate(ref chref)
{
	if(GetCharacterItem(chref, "indian11")) return; // проверка крысиного бога
    int iGoods = GOOD_POWDER + rand(GOOD_OIL - GOOD_POWDER);
    int iQuantity = GetCargoGoods(chref, iGoods);
    int iSeaGoods = LanguageOpenFile("ShipEatGood.txt");
    if (iQuantity > 60 && rand(4) != 2) // шанс не жрать, если весь спектр
    {
        float fSkill = GetSummonSkillFromNameToOld(chref, SKILL_REPAIR) + GetSummonSkillFromNameToOld(chref,SKILL_FORTUNE);
        
        iQuantity = 1+ rand(makeint(iQuantity / (10+fSkill)));
        RemoveCharacterGoodsSelf(chref, iGoods, iQuantity);
        //PlaySound("Notebook_1");
        Log_SetStringToLog(RandSwear() + " Крысы на корабле " +
                           chref.Ship.Name + LinkRandPhrase(" испортили ", " повредили ", " уничтожили ") +
                           iQuantity + " шт. " + LanguageConvertString(iSeaGoods, "seg_" + Goods[iGoods].Name));

        Statistic_AddValue(pchar, "RatsEatGoods", iQuantity);
                
		if (iQuantity > 400) iQuantity = 400;
		
		AddCharacterExpToSkill(chref, SKILL_REPAIR, iQuantity);
        AddCharacterExpToSkill(chref, SKILL_FORTUNE, iQuantity/10);
    }
    LanguageCloseFile(iSeaGoods);
}

// Автоснабжение офицеров из личного рундука в каюте.
// Отсутствующий флаг означает выключенный режим, поэтому старые сохранения совместимы.
bool OfficerSupply_IsEnabled(ref officer)
{
    if (!CheckAttribute(officer, "OfficerSupply.Enabled")) return false;
    return sti(officer.OfficerSupply.Enabled) == true;
}

void OfficerSupply_EnsureTargets(ref officer)
{
    if (CheckAttribute(officer, "OfficerSupply.TargetBullet")) return;

    int ammo = 8 + rand(6);
    officer.OfficerSupply.TargetBullet = ammo;
    officer.OfficerSupply.TargetGunPowder = ammo;
    officer.OfficerSupply.TargetPotion1 = 2 + rand(2);
    officer.OfficerSupply.TargetPotion2 = 1 + rand(1);
    officer.OfficerSupply.TargetPotion3 = 1;
    officer.OfficerSupply.TargetPotion4 = rand(1);
}

bool OfficerSupply_CanUseGun(ref officer, ref item)
{
    bool musketeer = false;
    if (CheckAttribute(officer, "CanTakeMushket"))
    {
        if (CheckAttribute(officer, "IsMushketer")) musketeer = true;
    }

    bool musket = isMushket(item.id);
    if (officer.id == "OffMushketer") return item.id == "mushket2x2";
    if (musketeer != musket) return false;
    if (item.id == "mushket2x2" && officer.id != "OffMushketer") return false;
    if (item.id == "mushket_H2" && !IsCharacterPerkOn(officer, "GunProfessional")) return false;
    if (!CheckAttribute(item, "chargeQ")) return false;

    int charges = sti(item.chargeQ);
    if (charges < 2) return true;
    if (charges < 4) return IsCharacterPerkOn(officer, "Gunman");
    return IsCharacterPerkOn(officer, "GunProfessional");
}

float OfficerSupply_ItemScore(ref officer, ref item, string groupID)
{
    float result = -1.0;
    float skill;
    float weight;
    float speed;
    float accuracy;

    if (!CheckAttribute(item, "price")) return result;
    if (sti(item.price) <= 0) return result;
    if (CheckAttribute(item, "quest")) return result;

    if (groupID == BLADE_ITEM_TYPE)
    {
        if (CheckAttribute(officer, "isMusketer")) return result;
        if (CheckAttribute(officer, "model.animation"))
        {
            if (HasSubStr(officer.model.animation, "mushketer")) return result;
        }
        if (!CheckAttribute(item, "FencingType")) return result;
        if (!CheckAttribute(item, "dmg_min")) return result;
        if (!CheckAttribute(item, "dmg_max")) return result;
        if (!CheckAttribute(item, "Weight")) return result;
        weight = stf(item.Weight);
        if (weight <= 0.0) return result;
        skill = GetCharacterSkill(officer, item.FencingType);
        return (stf(item.dmg_min) * 3.0 + stf(item.dmg_max) * skill / SKILL_MAX) / GetEnergyBladeDrain(weight);
    }

    if (groupID == GUN_ITEM_TYPE)
    {
        if (!OfficerSupply_CanUseGun(officer, item)) return result;
        if (!CheckAttribute(item, "dmg_min")) return result;
        if (!CheckAttribute(item, "dmg_max")) return result;
        accuracy = 50.0;
        if (CheckAttribute(item, "accuracy")) accuracy = 50.0 + stf(item.accuracy);
        speed = 1.0;
        if (CheckAttribute(item, "chargespeed")) speed = stf(item.chargespeed);
        if (speed <= 0.0) speed = 1.0;
        return ((stf(item.dmg_min) + stf(item.dmg_max)) * accuracy / speed) + stf(item.chargeQ) * 25.0;
    }

    if (groupID == CIRASS_ITEM_TYPE)
    {
        if (!IsCharacterPerkOn(officer, "Ciras")) return result;
        if (!CheckAttribute(item, "CirassLevel")) return result;
        if (stf(item.CirassLevel) <= 0.0) return result;
        weight = 0.0;
        if (CheckAttribute(item, "Weight")) weight = stf(item.Weight);
        return stf(item.CirassLevel) * 1000.0 - weight;
    }
    return result;
}

string OfficerSupply_FindBestItem(ref officer, aref chest, string groupID)
{
    string bestID = "";
    float bestScore = -1.0;
    float score;
    ref item;

    for (int i = 0; i < TOTAL_ITEMS; i++)
    {
        item = &Items[i];
        if (!CheckAttribute(item, "id")) continue;
        if (!CheckAttribute(item, "groupID")) continue;
        if (item.groupID != groupID) continue;
        if (GetCharacterItem(officer, item.id) < 1 && GetCharacterItem(chest, item.id) < 1) continue;

        score = OfficerSupply_ItemScore(officer, item, groupID);
        if (score <= bestScore) continue;
        if (GetCharacterItem(officer, item.id) < 1)
        {
            if (CheckAttribute(item, "Weight"))
            {
                if (GetItemsWeight(officer) + stf(item.Weight) > GetMaxItemsWeight(officer)) continue;
            }
        }
        bestScore = score;
        bestID = item.id;
    }
    return bestID;
}

bool OfficerSupply_ItemIsReturnable(string itemID)
{
    aref item;
    if (itemID == "" || itemID == "unarmed") return false;
    if (Items_FindItem(itemID, &item) < 0) return false;
    if (!CheckAttribute(item, "price")) return false;
    if (sti(item.price) <= 0) return false;
    if (CheckAttribute(item, "quest")) return false;
    return true;
}

bool OfficerSupply_IsEquipmentLocked(ref officer, string groupID, string itemID)
{
    if (CheckAttribute(officer, "HoldEquip")) return true;
    if (groupID == BLADE_ITEM_TYPE && CheckAttribute(officer, "DontChangeBlade")) return true;
    if (groupID == GUN_ITEM_TYPE && CheckAttribute(officer, "DontChangeGun")) return true;
    if (!OfficerSupply_ItemIsReturnable(itemID) && itemID != "" && itemID != "unarmed") return true;
    return false;
}

bool OfficerSupply_ReturnEquipment(ref officer, aref chest, string itemID)
{
    if (!OfficerSupply_ItemIsReturnable(itemID)) return false;
    if (GetCharacterItem(officer, itemID) < 1) return false;
    if (!TakeNItems(chest, itemID, 1)) return false;
    TakeNItems(officer, itemID, -1);
    return true;
}

int OfficerSupply_FillEquipment(ref officer, aref chest, string groupID)
{
    string equipped = GetCharacterEquipByGroup(officer, groupID);
    if (OfficerSupply_IsEquipmentLocked(officer, groupID, equipped)) return 0;

    string itemID = OfficerSupply_FindBestItem(officer, chest, groupID);
    if (itemID == "" || itemID == equipped) return 0;

    float equippedScore = -1.0;
    aref equippedItem;
    if (equipped != "" && GetCharacterItem(officer, equipped) > 0)
    {
        if (Items_FindItem(equipped, &equippedItem) >= 0)
        {
            equippedScore = OfficerSupply_ItemScore(officer, equippedItem, groupID);
        }
    }

    aref item;
    if (Items_FindItem(itemID, &item) < 0) return 0;
    if (OfficerSupply_ItemScore(officer, item, groupID) <= equippedScore) return 0;

    int moved = 0;
    if (GetCharacterItem(officer, itemID) < 1)
    {
        if (!TakeNItems(officer, itemID, 1)) return 0;
        TakeNItems(chest, itemID, -1);
        moved = 1;
    }
    EquipCharacterByItem(officer, itemID);
    if (GetCharacterEquipByGroup(officer, groupID) != itemID)
    {
        if (moved > 0)
        {
            TakeNItems(chest, itemID, 1);
            TakeNItems(officer, itemID, -1);
        }
        return 0;
    }

    // Return only the displaced, ordinary item; other officer inventory stays
    // untouched so the automation cannot clean out a player's manual loadout.
    if (equipped != "" && equipped != itemID)
    {
        OfficerSupply_ReturnEquipment(officer, chest, equipped);
    }
    return moved;
}

int OfficerSupply_FillItem(ref officer, aref chest, string itemID, int target)
{
    int need = target - GetCharacterItem(officer, itemID);
    int available = GetCharacterItem(chest, itemID);
    int moved = 0;
    aref item;

    if (need < 1 || available < 1) return 0;
    if (need > available) need = available;
    if (Items_FindItem(itemID, &item) < 0) return 0;

    while (need > 0)
    {
        if (CheckAttribute(item, "Weight"))
        {
            if (GetItemsWeight(officer) + stf(item.Weight) > GetMaxItemsWeight(officer)) break;
        }
        if (!TakeNItems(officer, itemID, 1)) break;
        TakeNItems(chest, itemID, -1);
        moved++;
        need--;
    }
    return moved;
}

int OfficerSupply_RefillOfficer(ref officer, aref chest)
{
    if (!OfficerSupply_IsEnabled(officer)) return 0;
    if (!GetRemovable(officer)) return 0;
    if (CheckAttribute(officer, "prisoned"))
    {
        if (sti(officer.prisoned) == true) return 0;
    }

    OfficerSupply_EnsureTargets(officer);
    int moved = OfficerSupply_FillEquipment(officer, chest, BLADE_ITEM_TYPE);
    moved += OfficerSupply_FillEquipment(officer, chest, GUN_ITEM_TYPE);

    string gunID = GetCharacterEquipByGroup(officer, GUN_ITEM_TYPE);
    if (gunID != "")
    {
        moved += OfficerSupply_FillItem(officer, chest, "bullet", sti(officer.OfficerSupply.TargetBullet));
        moved += OfficerSupply_FillItem(officer, chest, "GunPowder", sti(officer.OfficerSupply.TargetGunPowder));
    }
    moved += OfficerSupply_FillItem(officer, chest, "potion1", sti(officer.OfficerSupply.TargetPotion1));
    moved += OfficerSupply_FillItem(officer, chest, "potion2", sti(officer.OfficerSupply.TargetPotion2));
    moved += OfficerSupply_FillItem(officer, chest, "potion3", sti(officer.OfficerSupply.TargetPotion3));
    moved += OfficerSupply_FillItem(officer, chest, "potion4", sti(officer.OfficerSupply.TargetPotion4));
    moved += OfficerSupply_FillEquipment(officer, chest, CIRASS_ITEM_TYPE);
    return moved;
}

bool OfficerSupply_CanRefillNow()
{
    if (dialogRun) return false;
    if (LAi_grp_alarmactive) return false;
    if (LAi_IsFightMode(pchar)) return false;
    if (Get_My_Cabin() == "") return false;
    return true;
}

void LaunchCabinChest()
{
    string cabinID = Get_My_Cabin();
    if (cabinID == "") return;
    int locIdx = FindLocation(cabinID);
    if (locIdx < 0) return;
    if (CheckAttribute(&Locations[locIdx], "box1"))
    {
        aref chestRef;
        makearef(chestRef, Locations[locIdx].box1);
        if (GetAttributesNum(chestRef) == 0) Locations[locIdx].box1.Money = 0;
        LaunchItemsBox(&chestRef);
    }
}

int OfficerSupply_RefillAllFromChest(aref chest, bool showLog)
{
    if (!OfficerSupply_CanRefillNow()) return 0;
    int moved = 0;
    int idx;
    ref officer;

    for (int i = 0; i < GetPassengersQuantity(pchar); i++)
    {
        idx = GetPassenger(pchar, i);
        if (idx < 1) continue;
        officer = &Characters[idx];
        moved += OfficerSupply_RefillOfficer(officer, chest);
    }
    if (showLog && moved > 0)
    {
        Log_Info("Автоснабжение офицеров: передано предметов — " + moved + ".");
    }
    return moved;
}

int OfficerSupply_RefillFromCabin(bool showLog)
{
    if (!OfficerSupply_CanRefillNow()) return 0;
    int locationIndex = FindLocation(Get_My_Cabin());
    if (locationIndex < 0) return 0;
    if (!CheckAttribute(&Locations[locationIndex], "box1.items")) return 0;

    aref chest;
    makearef(chest, Locations[locationIndex].box1);
    return OfficerSupply_RefillAllFromChest(chest, showLog);
}

void OfficerSupply_AfterDialog()
{
    if (!CheckAttribute(pchar, "questTemp.OfficerSupplyIdx")) return;
    int idx = sti(pchar.questTemp.OfficerSupplyIdx);
    DeleteAttribute(pchar, "questTemp.OfficerSupplyIdx");
    if (!OfficerSupply_CanRefillNow()) return;

    int locationIndex = FindLocation(Get_My_Cabin());
    if (locationIndex < 0) return;
    if (!CheckAttribute(&Locations[locationIndex], "box1.items")) return;

    aref chest;
    makearef(chest, Locations[locationIndex].box1);
    int moved = OfficerSupply_RefillOfficer(GetCharacter(idx), chest);
    if (moved > 0)
    {
        Log_Info("Автоснабжение офицера: передано предметов — " + moved + ".");
    }
}


// Count and spend from the same active removable squad cargo owner.
// Travelling companions are deliberately excluded: their provisions stay aboard
// their own ship while they are outside the squad.
int SquadReserve_Goods(ref rChar, int goodID, bool isCompanionTraveler)
{
    if (isCompanionTraveler || !GetShipRemovableEx(rChar)) return GetCargoGoods(rChar, goodID);
    return GetSquadronGoods(pchar, goodID);
}

void SquadReserve_Remove(ref rChar, int goodID, int quantity, bool isCompanionTraveler)
{
    if (quantity < 1) return;
    if (isCompanionTraveler || !GetShipRemovableEx(rChar))
    {
        RemoveCharacterGoodsSelf(rChar, goodID, quantity);
        return;
    }
    // GetSquadronGoods excludes ShipRemovable=false companions, whereas
    // RemoveCharacterGoods does not. Mirror the former's donor set exactly.
    int available = GetCargoGoods(pchar, goodID);
    int take = quantity;
    if (take > available) take = available;
    if (take > 0) RemoveCharacterGoodsSelf(pchar, goodID, take);
    quantity = quantity - take;
    ref donor;
    int index;
    for (int slot = 1; slot < COMPANION_MAX; slot++)
    {
        if (quantity < 1) return;
        index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        donor = GetCharacter(index);
        if (!GetRemovable(donor) || !GetShipRemovableEx(donor)) continue;
        available = GetCargoGoods(donor, goodID);
        take = quantity;
        if (take > available) take = available;
        if (take > 0) RemoveCharacterGoodsSelf(donor, goodID, take);
        quantity = quantity - take;
    }
}

// boal food for crew 20.01.2004 -->
void DailyEatCrewUpdate()   // сюда пихаю всё что в 1 день
{
    ref mainCh = GetMainCharacter();
    OfficerSupply_RefillFromCabin(false);
    int i, cn, crew, morale;
    ref chref;
    int nMoraleDecreaseQ;
    
    // to_do
    // boal 030804 Начисление денег верфям -->
    //DailyShipyardMoneyUpdate();
    // boal 030804 Начисление денег верфям <--
    mainCh.questTemp.abordage = 0; // fix квест потопить пирата второй абордаж

    //таможня
    //if(IsCharacterPerkOn(mainCh, "CustomsHouse"))
    //{
    //    AddGoverGoods();
    //}

    SetNewDayHealth(); // здоровье за день
    // >>>>>======== квест Аззи, подсчет контрольныйх сумм по неуязвимости =================
    AzzyCheckSumControl();
    // <<<<<======== квест Аззи, подсчет контрольныйх сумм по неуязвимости =================
	// ОЗГи -->
	//SetPortShoreEnter(mainCh);
    DeleteAttribute(mainCh, "GenQuest.Hunter2Pause");  // boal бойня в форте кончилась - ОЗГи вернулись
    // ОЗГи <--

    //  уже не нужно SetAllHabitueToNew(); // сменить всех пьяниц в тавернах

	////////////////      ЕДА     /////////////////
	if (bNoEatNoRats) return; // betatest
    if (sti(mainCh.Ship.Type) == SHIP_NOTUSED ) return;

	// снижение лояльности от долга 02.02.08 -->
	if (CheckAttribute(pchar, "CrewPayment")) // Долг
	{
        cn = makeint(pchar.CrewPayment);
        if (cn > 32000) cn = 32000;
		if (rand(cn) > 1000)
		{
            morale = 5 + CheckOfficersPerk(pchar, "IronWill");   // true = 1
			for (i = 0; i<GetPassengersQuantity(pchar); i++)
			{   // любой пассажир у кого есть пристрастие может свалить
				cn = GetPassenger(pchar, i);
				if (cn != -1)
				{
		            chref = &Characters[cn];
					if (CheckAttribute(chref, "loyality") && !CheckAttribute(chref, "OfficerWantToGo.DontGo") && rand(morale) == 2)
					{
		    			chref.loyality = makeint(chref.loyality) - 1;
					}
				}
			}
		}
	}
	// снижение лояльности от долга 02.02.08 <--
	
	for(i=0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(mainCh,i);
		if( cn>=0 )
		{
			chref = GetCharacter(cn);

			if (!GetRemovable(chref)) continue;

			// RATS -->
			DailyRatsEatGoodsUpdate(chref);
			// RATS <--
			DailyEatCrewUpdateForShip(chref, false);
			
		}
	}
}

// boal 20.01.2004 <--

// Warship. Вынес в отдельный метод
void DailyEatCrewUpdateForShip(ref rChar, bool IsCompanionTraveler) // IsCompanionTraveler - спец флаг для компаньонов-путешественников
{
	int iCrewQty = GetCrewQuantity(rChar);
	int cn, morale, nMoraleDecreaseQ, iDeadCrew;
	if(iCrewQty < 1 && GetCargoGoods(rChar, GOOD_SLAVES) < 1) return;
	if(!CheckAttribute(rChar, "Ship.Crew.Morale"))
	{
		rChar.Ship.Crew.Morale = 50;
	}
	// расчёт медицины -->
	if(rand(4) == 2)
	{
		// матросы
		cn = iCrewQty / 10;
		if(cn > 30) cn = 30;
		cn = rand(cn)+1;
		if(iCrewQty < cn) cn = iCrewQty;
		if(cn > 0)
		{
			if(SquadReserve_Goods(rChar, GOOD_MEDICAMENT, IsCompanionTraveler) < 1)
			{
				if(!IsCompanionTraveler) Log_Info("На корабле " + rChar.Ship.Name + " от болезней умерло " + FindRussianSailorString(cn, "No"));
				iCrewQty = iCrewQty - cn;
				Statistic_AddValue(pchar, "Sailors_dead", cn);
				rChar.Ship.Crew.Quantity = iCrewQty;
				// мораль в минус
				morale = sti(rChar.Ship.Crew.Morale);
				
				if(CheckOfficersPerk(rChar, "IronWill")) cn /= 1.5;
				
				AddCrewMorale(rChar, -makeint(cn / 2)); // до 15 пунктов за раз
			}
			else
			{
				if(CheckShipSituationDaily_GenQuest(rChar) == 2) cn = cn * 2;
				if(CheckShipSituationDaily_GenQuest(rChar) == 3) cn = cn * 3;
				
				SquadReserve_Remove(rChar, GOOD_MEDICAMENT, cn, IsCompanionTraveler);
				if(SquadReserve_Goods(rChar, GOOD_MEDICAMENT, IsCompanionTraveler) < 16)
				{
					if(!IsCompanionTraveler) Log_Info("На корабле " + rChar.Ship.Name + " осталось мало медикаментов");
				}
			}
		}
		// рабы
		cn = GetCargoGoods(rChar, GOOD_SLAVES) / 10;
		if(cn > 30) cn = 30;
		cn = rand(cn)+1;
		if(GetCargoGoods(rChar, GOOD_SLAVES) < cn) cn = GetCargoGoods(rChar, GOOD_SLAVES);
		if(cn > 0)
		{
			if(SquadReserve_Goods(rChar, GOOD_MEDICAMENT, IsCompanionTraveler) < 1)
			{
				if(!IsCompanionTraveler) Log_Info("На корабле " + rChar.Ship.Name + " от болезней умерло " + FindRussianSlavesString(cn, "No"));
				RemoveCharacterGoodsSelf(rChar, GOOD_SLAVES, cn);
			}
			else
			{
				cn /= 3;
				SquadReserve_Remove(rChar, GOOD_MEDICAMENT, cn, IsCompanionTraveler);
			}
		}
		// повторный контроль
		if(iCrewQty < 1 && GetCargoGoods(rChar, GOOD_SLAVES) < 1) return;
	}
	// расчёт медицины <--
	
	iCrewQty = makeint((iCrewQty+5.1) / RUM_BY_CREW); // eat ratio
	//if (rChar == 0) rChar = 1;
	if(iCrewQty > 0)
	{
		if(SquadReserve_Goods(rChar, GOOD_RUM, IsCompanionTraveler) >= iCrewQty)
		{
			SquadReserve_Remove(rChar, GOOD_RUM, iCrewQty, IsCompanionTraveler);
			// проверка на остатки
			cn = makeint(SquadReserve_Goods(rChar, GOOD_RUM, IsCompanionTraveler) / iCrewQty);
			if (cn < 1)
			{
				if(!IsCompanionTraveler) 
				{
					Log_Info("На корабле " + rChar.Ship.Name + " весь ром выпит");
					PlaySound("Notebook_1"); // Hokkins: произведем звук, когда ром закончится.
				}
			}
			// поднимем мораль
			if(CheckShipSituationDaily_GenQuest(rChar) == 1) AddCrewMorale(rChar, 2);
		}
		else
		{
			iCrewQty = SquadReserve_Goods(rChar, GOOD_RUM, IsCompanionTraveler);
			SquadReserve_Remove(rChar, GOOD_RUM, iCrewQty, IsCompanionTraveler);
		}
	}
	iCrewQty = GetCrewQuantity(rChar);
	// рассчет перегруза команды на мораль -->
	if(iCrewQty > GetOptCrewQuantity(rChar))
	{
		AddCrewMorale(rChar, -(1+rand(3)));
	} 
	// рассчет перегруза команды на мораль <--
	
	// расчёт долга на мораль
	if(iCrewQty > 0 && CheckAttribute(PChar, "CrewPayment"))
	{
		cn = makeint(PChar.CrewPayment);
		if(cn > 32000) cn = 32000;
		if(rand(cn) > 1000)
		{
			AddCrewMorale(rChar, -1);
			cn = 5 + CheckOfficersPerk(PChar, "IronWill");  // перк у ГГ
			if(i > 0 && rand(cn) == 2 && !CheckAttribute(rChar, "OfficerWantToGo.DontGo"))
			{
				rChar.loyality = sti(rChar.loyality) - 1;
			}
		}
	}
	// расчёт еды после Рома
	iCrewQty = makeint((iCrewQty+5.1) / FOOD_BY_CREW + GetPassengersQuantity(rChar) / FOOD_BY_PASSENGERS); // eat ratio
	iCrewQty = iCrewQty + makeint((GetCargoGoods(rChar, GOOD_SLAVES)+6)/ FOOD_BY_SLAVES);  // учет рабов
	if(iCrewQty == 0) iCrewQty = 1;
	if(SquadReserve_Goods(rChar, GOOD_FOOD, IsCompanionTraveler) >= iCrewQty)
	{
		SquadReserve_Remove(rChar, GOOD_FOOD, iCrewQty, IsCompanionTraveler);
		// проверка на остатки
		cn = makeint(SquadReserve_Goods(rChar, GOOD_FOOD, IsCompanionTraveler) / iCrewQty);
		if (cn < 4)
		{
			if(!IsCompanionTraveler)
			{
				Log_Info("На корабле " + rChar.Ship.Name + " продовольствия осталось на " + FindRussianDaysString(cn));
				Log_Info("Нужно срочно пополнить запасы!");
				PlaySound("Notebook_1");
			}
		}
		// возможный бунт рабов
		if (sti(rChar.index) == GetMainCharacterIndex() && GetCargoGoods(rChar, GOOD_SLAVES) > (GetCrewQuantity(rChar)*1.5 + sti(rChar.Ship.Crew.Morale)))
		{
			nMoraleDecreaseQ = 12 - GetSummonSkillFromNameToOld(rChar, SKILL_LEADERSHIP);
			if(CheckOfficersPerk(rChar, "IronWill")) nMoraleDecreaseQ /= 2;
			if(rand(2) == 1 && nMoraleDecreaseQ > rand(10))
			{
				if(IsEntity(worldMap))
				{
					rChar.GenQuest.SlavesMunity = true;
					Log_Info("Рабы подняли восстание!");
					MunityOnShip("SlavesMunity");
				}
			}
		}
	}
	else
	{
		iCrewQty = SquadReserve_Goods(rChar, GOOD_FOOD, IsCompanionTraveler);
		SquadReserve_Remove(rChar, GOOD_FOOD, iCrewQty, IsCompanionTraveler);
		PlaySound("Notebook_1");
		
		if(!IsCompanionTraveler) Log_Info("На корабле " + rChar.Ship.Name + " матросы голодают. Мораль команды падает!");
		
		if(sti(rChar.index) == GetMainCharacterIndex())
		{
			AddCharacterHealth(PChar, -1);
		}
		
		cn = GetCrewQuantity(rChar);
		if(cn > 1)
		{
			iDeadCrew = makeint(cn/10 +0.5);
			rChar.Ship.Crew.Quantity = cn - iDeadCrew;
			Statistic_AddValue(pchar, "Sailors_dead", iDeadCrew);
			if(!IsCompanionTraveler) Log_Info("Матросы умирают от голода");
		}
		cn = GetCargoGoods(rChar, GOOD_SLAVES);
		if(cn > 0)
		{
			RemoveCharacterGoodsSelf(rChar, GOOD_SLAVES, makeint(cn/5 + 0.5));
			if(!IsCompanionTraveler) Log_Info("Рабы умирают от голода");
		}
		morale = sti(rChar.Ship.Crew.Morale);
		
		nMoraleDecreaseQ = 12 - GetSummonSkillFromNameToOld(rChar, SKILL_LEADERSHIP);
		if(CheckOfficersPerk(rChar, "IronWill")) nMoraleDecreaseQ /= 2;
		rChar.Ship.Crew.Morale = morale - nMoraleDecreaseQ;
		if(sti(rChar.Ship.Crew.Morale) < MORALE_MIN) rChar.Ship.Crew.Morale = MORALE_MIN;  
	}
	
	if(sti(rChar.index) == GetMainCharacterIndex())
	{
		if(sti(rChar.Ship.Crew.Morale) <= MORALE_MIN)
		{
			//int locidx = FindLocation(rChar.location); // не используется
			if(IsEntity(worldMap) && GetCrewQuantity(rChar) > 0)
			{
				Log_Info("Бунт на корабле " + rChar.Ship.Name + "!!!! ");
				MunityOnShip("ShipMunity");
			}
		}
	}
	else
	{
		if(GetShipRemovable(rChar) && !CheckAttribute(rChar, "OfficerWantToGo.DontGo") && !IsCompanionTraveler) // ПГГ, квестовые оффы и компаньоны-путешественники не бунтуют
		{
			if(sti(rChar.Ship.Crew.Morale) <= MORALE_MIN || sti(rChar.loyality) <= 0) // допуск, что лояльность есть у всех офов
			{
				if(GetCrewQuantity(rChar) > 0)
				{
					Log_Info("Бунт на корабле " + rChar.Ship.Name + "!!!! ");
					Log_SetStringToLog("Корабль выходит из эскадры");
					Statistic_AddValue(PChar, "ShipMunity", 1);
					RemoveCharacterCompanion(PChar, rChar);
					//fix  ПГГ
					if(!CheckAttribute(rChar, "PGGAi"))
					{
						rChar.LifeDay = 0; // стереть при выходе
					}
					else
					{
						rChar.PGGAi.IsPGG = true;
						rChar.RebirthPhantom = true;
						rChar.PGGAi.location.town = PGG_FindRandomTownByNation(sti(rChar.nation));
						rChar.Dialog.FileName = "PGG_Dialog.c";
						rChar.Dialog.CurrentNode = "Second Time";
						PGG_ChangeRelation2MainCharacter(rChar, -20);
					}
					rChar.location = ""; // нафиг, нафиг..а то в таверне появлялся...
					rChar.location.group = "";
					rChar.location.locator = "";
				}
			}
		}
	}
}

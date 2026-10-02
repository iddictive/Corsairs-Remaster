#define REAL_SHIPS_QUANTITY 900

object	RealShips[REAL_SHIPS_QUANTITY];
#define SAILSCOLOR_PRICE_RATE    0.1 
#define SAILSGERALD_PRICE_RATE   0.2 

#define SAIL_COST_PERCENT 10
#define HULL_COST_PERCENT 20

ref GetRealShip(int iType) 
{ 
	if(iType >= REAL_SHIPS_QUANTITY)
	{
		trace ("--- Wrong Ship Index. iType is " + iType);
		Log_TestInfo("--- Wrong Ship Index. iType is " + iType);
		return &ShipsTypes[SHIP_TYPES_QUANTITY + 1]; // для отлова
	}
	return &RealShips[iType]; 
}

// isLock это признак ворованности, определяет цену на верфи
// 1 - ворованный, цена копеешная; 0 - честно купленный, можно перепродать
int GenerateShip(int iBaseType, bool isLock)
{
	int iShip = CreateBaseShip(iBaseType);

	if (iShip == -1)
	{
		return SHIP_NOTUSED;
	}
	
	ref rRealShip = GetRealShip(iShip);
	ref rBaseShip = GetShipByType(sti(rRealShip.BaseType));
    // boal 26/05/06 изменим
    rRealShip.ship.upgrades.hull  = 1 + rand(2);  //признак корабля теперь
	rRealShip.ship.upgrades.sails = 1 + rand(2);  // только визуальная разница
	
	if (!CheckAttribute(rRealShip, "isFort"))
	{
	    rRealShip.SpeedRate	       = stf(rRealShip.SpeedRate) + frandSmall(stf(rRealShip.SpeedRate) / 5.0) - stf(rRealShip.SpeedRate) / 10.0;
	    rRealShip.TurnRate         = stf(rRealShip.TurnRate) + frandSmall(stf(rRealShip.TurnRate) / 5.0) - stf(rRealShip.TurnRate) / 10.0;
	    //rRealShip.Price            = sti(rRealShip.Price) + rand(makeint(sti(rRealShip.Price)/2)) - makeint(sti(rRealShip.Price)/4);
	    rRealShip.HP               = sti(rRealShip.HP) + rand(makeint(sti(rRealShip.HP)/5)) - makeint(sti(rRealShip.HP)/10);
	    rRealShip.WindAgainstSpeed = stf(rRealShip.WindAgainstSpeed) + frandSmall(stf(rRealShip.WindAgainstSpeed)/5.0) - stf(rRealShip.WindAgainstSpeed)/10.0;
	}
    rRealShip.Capacity        = sti(rRealShip.Capacity) + rand(makeint(sti(rRealShip.Capacity)/4)) - makeint(sti(rRealShip.Capacity)/8);
    rRealShip.OptCrew         = makeint(sti(rRealShip.OptCrew) + rand(makeint(sti(rRealShip.OptCrew)/3)) - makeint(sti(rRealShip.OptCrew)/6));
    rRealShip.MaxCrew         = makeint(sti(rRealShip.OptCrew) * 1.25 + 0.5);  // 25% перегруза
    rRealShip.MinCrew         = makeint(sti(rRealShip.MinCrew) + rand(makeint(sti(rRealShip.MinCrew)/3)) - makeint(sti(rRealShip.MinCrew)/6));

	rRealShip.Weight		  = sti(rRealShip.Weight) + rand(sti(rRealShip.Weight)/20) - rand(sti(rRealShip.Weight)/20);
	
	// to_do del -->
	rRealShip.BoardingCrew    = 0;
	rRealShip.GunnerCrew      = 0;
	rRealShip.CannonerCrew    = 0;
	rRealShip.SailorCrew      = sti(rRealShip.OptCrew);
    // to_do del <--
    
	int iDiffWeight			= sti(rRealShip.Weight) - sti(rBaseShip.Weight);
	int iDiffCapacity		= sti(rRealShip.Capacity) - sti(rBaseShip.Capacity);
	int iDiffMaxCrew		= sti(rRealShip.MaxCrew) - sti(rBaseShip.MaxCrew);
	int iDiffMinCrew		= sti(rRealShip.MinCrew) - sti(rBaseShip.MinCrew);
	float fDiffSpeedRate	= stf(rRealShip.SpeedRate) - stf(rBaseShip.SpeedRate);
	int iDiffTurnRate		= sti(rRealShip.TurnRate) - sti(rBaseShip.TurnRate);
	int iDiffHP	    		= sti(rRealShip.HP) - sti(rBaseShip.HP);

	rRealShip.Price	= (iDiffWeight + iDiffCapacity + iDiffMaxCrew*2 + iDiffMinCrew + fDiffSpeedRate*2 + iDiffTurnRate*2 + iDiffHP)*5 + sti(rRealShip.Price);

	if (sti(rRealShip.Price) <= 0) rRealShip.Price = 100;
	
	rRealShip.Stolen = isLock;  // ворованность

	return iShip;
}

// -> added by ugeen 25.01.09 (на основе GenerateShip(.....)) - рандомизируем кол-во стволов на борту
// 31.01.09 - добавил бонусы к корабельным статам если кол-во орудий на борту < базового;
// основной метод генерации корабля
int GenerateShipExt(int iBaseType, bool isLock, ref chr)
{
	string  attr, sCity;
	int 	i;
	aref 	refShip;
	float	Кdckyrd = 1.0;
	bool	isShipyard = false; 
	
	int iShip = CreateBaseShip(iBaseType);

	if (iShip == -1)
	{
		return SHIP_NOTUSED;
	}
	
	ref rRealShip = GetRealShip(iShip);
	ref rBaseShip = GetShipByType(sti(rRealShip.BaseType));
	// boal 26/05/06 изменим
	rRealShip.ship.upgrades.sails = 1 + rand(2);  // только визуальная разница
	
	// --->>> ZhilyaevDm 06.03.23
	// национальные хуллы + установка хулла от количества у модели
	if (rRealShip.isNationalHull == true && CheckAttribute(chr, "nation"))
	{
		switch (sti(chr.nation))
		{
			case 0: rRealShip.ship.upgrades.hull = 1;			break;	//Английская расцветка
			case 1: rRealShip.ship.upgrades.hull = 2;			break;	//Французская расцветка
			case 2: rRealShip.ship.upgrades.hull = 3;			break;	//Испанская расцветка
			case 3: rRealShip.ship.upgrades.hull = 4;			break;	//Голландская расцветка
			case 4: rRealShip.ship.upgrades.hull = 1 + rand(3);	break;	//Пиратскаие корабли имеют рандомные цвета
		}
	}
	else
		rRealShip.ship.upgrades.hull = 1 + rand(sti(rRealShip.HullsAmount) - 1); //для кораблей без нац хулов под их заданное кол-во
	// <<<--- ZhilyaevDm

	// ugeen --> если кораблик генерится на верфи, разброс статов более узкий
	if (CheckAttribute(chr, "City")) 
	{
		sCity = chr.City; 
		if(CheckCharacterID(chr, sCity + "_shipyarder"))
		{
			Кdckyrd 	= 0.8;
			isShipyard 	= true;
		}
	}	
	// ugeen

	rRealShip.Bonus_Capacity = 0;
	rRealShip.Bonus_HP = 0;
	rRealShip.Bonus_SpeedRate = 0.0;
	rRealShip.Bonus_TurnRate = 0.0;

	if (!CheckAttribute(rRealShip, "isFort"))
	{
		int   iCannonDiff 		= rand(sti(rRealShip.CannonDiff));	// рандомим кол-во стволов
		int   iDiffClass  		= 5 - makeint(sti(rRealShip.Class)/2);
		
		if(CheckAttribute(rRealShip, "QuestShip") || isShipyard) iCannonDiff = 0;
		
		if(rRealShip.BaseName == "FrigateQueen") iCannonDiff = 2; // делаем фрегат "Королева" 48-и пушечным
		
		// собственно сам рэндом стволов
		makearef(refShip, chr.Ship);
		ResetShipCannonsDamages(chr);
		
		// куда-нить запишем максимально возможное кол-во орудий (потом нужно будет, если захотим проапгрейдиться на этот счет)
		rRealShip.CannonsQuantityMax = sti(rRealShip.CannonsQuantity);		
		// принципиальный момент ! нужно нулить обязательно левые стволы ! 	
		for (i = 0; i < sti(rRealShip.rcannon); i++)
		{
			attr = "c" + i;
			rRealShip.Cannons.Borts.cannonr.damages.(attr) = 1.0; 
			rRealShip.Cannons.Borts.cannonl.damages.(attr) = 1.0; 
			rRealShip.Cannons.Borts.rcannon.damages.(attr) = 1.0; 
			rRealShip.Cannons.Borts.lcannon.damages.(attr) = 1.0; 
			
			refShip.Cannons.Borts.cannonr.damages.(attr) = 1.0; 
			refShip.Cannons.Borts.cannonl.damages.(attr) = 1.0; 
			refShip.Cannons.Borts.rcannon.damages.(attr) = 1.0; 
			refShip.Cannons.Borts.lcannon.damages.(attr) = 1.0; 
		}
		
		rRealShip.rcannon = sti(rRealShip.rcannon) - iCannonDiff;				
		rRealShip.lcannon = sti(rRealShip.lcannon) - iCannonDiff;
		
		rRealShip.cannonr = sti(rRealShip.rcannon);
		rRealShip.cannonl = sti(rRealShip.lcannon);
		rRealShip.cannonf = sti(rRealShip.fcannon);
		rRealShip.cannonb = sti(rRealShip.bcannon);
/*
		refShip.cannonr = sti(rRealShip.rcannon);
		refShip.cannonl = sti(rRealShip.lcannon);
		refShip.cannonf = sti(rRealShip.fcannon);
		refShip.cannonb = sti(rRealShip.bcannon);
*/		
		rRealShip.Cannons = sti(rRealShip.CannonsQuantityMax) - iCannonDiff * 2;
		rRealShip.CannonsQuantity = sti(rRealShip.Cannons);
		
		//а теперь вертаем стволы обратно в потребном нам количестве		
		for (i = 0; i < sti(rRealShip.rcannon); i++)
		{
			attr = "c" + i;
			rRealShip.Cannons.borts.cannonr.damages.(attr) = 0.0; 
			rRealShip.Cannons.borts.cannonl.damages.(attr) = 0.0; 
			rRealShip.Cannons.borts.rcannon.damages.(attr) = 0.0; 
			rRealShip.Cannons.borts.lcannon.damages.(attr) = 0.0; 
			
			refShip.Cannons.borts.cannonr.damages.(attr) = 0.0; 
			refShip.Cannons.borts.cannonl.damages.(attr) = 0.0; 
			refShip.Cannons.borts.rcannon.damages.(attr) = 0.0; 
			refShip.Cannons.borts.lcannon.damages.(attr) = 0.0; 
		}	

		for (i = 0; i < sti(rRealShip.fcannon); i++)
		{
			attr = "c" + i;
			rRealShip.Cannons.borts.cannonf.damages.(attr) = 0.0; 			
			rRealShip.Cannons.borts.fcannon.damages.(attr) = 0.0; 			
			refShip.Cannons.borts.cannonf.damages.(attr) = 0.0; 
			refShip.Cannons.borts.fcannon.damages.(attr) = 0.0; 
		}
		
		for (i = 0; i < sti(rRealShip.bcannon); i++)
		{
			attr = "c" + i;
			rRealShip.Cannons.borts.cannonb.damages.(attr) = 0.0; 			
			rRealShip.Cannons.borts.bcannon.damages.(attr) = 0.0; 			
			refShip.Cannons.borts.cannonb.damages.(attr) = 0.0; 
			refShip.Cannons.borts.bcannon.damages.(attr) = 0.0; 		
		}
		
		refShip.Cannons.Borts.cannonl = sti(rRealShip.lcannon);
		refShip.Cannons.Borts.cannonr = sti(rRealShip.rcannon);
		refShip.Cannons.Borts.cannonf = sti(rRealShip.fcannon);
		refShip.Cannons.Borts.cannonb = sti(rRealShip.bcannon);
		
		refShip.Cannons.Borts.lcannon = sti(rRealShip.lcannon);
		refShip.Cannons.Borts.rcannon = sti(rRealShip.rcannon);
		refShip.Cannons.Borts.fcannon = sti(rRealShip.fcannon);
		refShip.Cannons.Borts.bcannon = sti(rRealShip.bcannon);
		
		refShip.Cannons = sti(rRealShip.Cannons);
		// <-- рэндом стволов

		if(!CheckAttribute(rRealShip, "QuestShip"))
		{	
			rRealShip.Bonus_Capacity 	= makeint((sti(rRealShip.Capacity)*iCannonDiff)/(15 * iDiffClass));
			rRealShip.Bonus_HP 			= makeint((sti(rRealShip.HP)*iCannonDiff)/(15 * iDiffClass));  
			rRealShip.Bonus_SpeedRate   = (stf(rRealShip.SpeedRate)*iCannonDiff)/(15 * iDiffClass);
			rRealShip.Bonus_TurnRate    = (stf(rRealShip.TurnRate)*iCannonDiff)/(15 * iDiffClass);
		
			rRealShip.SpeedRate	   		= stf(rRealShip.SpeedRate) + Кdckyrd * (frandSmall(stf(rRealShip.SpeedRate) / 5.0) - stf(rRealShip.SpeedRate) / 10.0);
			rRealShip.TurnRate         	= stf(rRealShip.TurnRate) + Кdckyrd * (frandSmall(stf(rRealShip.TurnRate) / 5.0) - stf(rRealShip.TurnRate) / 10.0);
			//rRealShip.Price            = sti(rRealShip.Price) + rand(makeint(sti(rRealShip.Price)/2)) - makeint(sti(rRealShip.Price)/4);
			rRealShip.HP               	= sti(rRealShip.HP) + makeint(Кdckyrd * (rand(makeint(sti(rRealShip.HP)/5)) - makeint(sti(rRealShip.HP)/10)));
			rRealShip.WindAgainstSpeed 	= stf(rRealShip.WindAgainstSpeed) + Кdckyrd * (frandSmall(stf(rRealShip.WindAgainstSpeed)/5.0) - stf(rRealShip.WindAgainstSpeed)/10.0);
		}	
	}
	
	if(!CheckAttribute(rRealShip, "QuestShip"))
	{
		rRealShip.Capacity        = sti(rRealShip.Capacity) + makeint(Кdckyrd * (rand(makeint(sti(rRealShip.Capacity)/4)) - makeint(sti(rRealShip.Capacity)/8)));
		rRealShip.OptCrew         = makeint(sti(rRealShip.OptCrew) + Кdckyrd * (rand(makeint(sti(rRealShip.OptCrew)/3)) - makeint(sti(rRealShip.OptCrew)/6)));
		rRealShip.MaxCrew         = makeint(sti(rRealShip.OptCrew) * 1.25 + 0.5);  // 25% перегруза
		rRealShip.MinCrew         = makeint(sti(rRealShip.MinCrew) + Кdckyrd * (rand(makeint(sti(rRealShip.MinCrew)/3)) - makeint(sti(rRealShip.MinCrew)/6)));
		rRealShip.Weight		  = sti(rRealShip.Weight) + makeint(Кdckyrd * (rand(sti(rRealShip.Weight)/20) - rand(sti(rRealShip.Weight)/20)));
	}	
	
	// to_do del -->
	rRealShip.BoardingCrew    = 0;
	rRealShip.GunnerCrew      = 0;
	rRealShip.CannonerCrew    = 0;
	rRealShip.SailorCrew      = sti(rRealShip.OptCrew);
	// to_do del <--
	
	if(!CheckAttribute(rRealShip, "QuestShip"))
	{
		rRealShip.Capacity  = sti(rRealShip.Capacity) + sti(rRealShip.Bonus_Capacity);
		rRealShip.HP        = sti(rRealShip.HP) + sti(rRealShip.Bonus_HP);
		rRealShip.SpeedRate = stf(rRealShip.SpeedRate) + stf(rRealShip.Bonus_SpeedRate);
		rRealShip.TurnRate  = stf(rRealShip.TurnRate) + stf(rRealShip.Bonus_TurnRate);
	}	
    
	int iDiffWeight			= sti(rRealShip.Weight) - sti(rBaseShip.Weight);
	int iDiffCapacity		= sti(rRealShip.Capacity) - sti(rBaseShip.Capacity);
	int iDiffMaxCrew		= sti(rRealShip.MaxCrew) - sti(rBaseShip.MaxCrew);
	int iDiffMinCrew		= sti(rRealShip.MinCrew) - sti(rBaseShip.MinCrew);
	float fDiffSpeedRate	= stf(rRealShip.SpeedRate) - stf(rBaseShip.SpeedRate);
	int iDiffTurnRate		= sti(rRealShip.TurnRate) - sti(rBaseShip.TurnRate);
	int iDiffHP	    		= sti(rRealShip.HP) - sti(rBaseShip.HP);

	rRealShip.Price	= (iDiffWeight + iDiffCapacity + iDiffMaxCrew*2 + iDiffMinCrew + fDiffSpeedRate*2 + iDiffTurnRate*2 + iDiffHP)*5 + sti(rRealShip.Price);

	if (sti(rRealShip.Price) <= 0) rRealShip.Price = 100;
	
	rRealShip.Stolen = isLock;  // ворованность

	return iShip;
}
// -> ugeen

// ---> >> ZhilyaevDm 27.10.22
// дополнительный метод выборки кораблей для сторонних генераторов 
// 
int GetShipTypeNational(int iClassMin, int iClassMax, string sShipType, int iNation)
{
	int iShips[50];
	int i, iShipsNum;
	iShipsNum = 0;

	aref aNation;	//	логика привязки к нации
	string sAttr;	//
	bool bOk;		//

	for (i = SHIP_TARTANE; i <= SHIP_MANOWAR; i++)  //энкаунтеры только до мановара, квестовые корабли отдельно
	{
		object rShip = GetShipByType(i);
		if (!checkAttribute(rship, "class"))
		{
			trace("bad ship is: " + rship.name);
		}
		int iClass = MakeInt(rShip.Class);
		int iRate = MakeInt(rShip.Rate);

		if (iClass > iClassMin) { continue; }
		if (iClass < iClassMax) { continue; }
		if (sti(rShip.CanEncounter) != true) { continue; }
		if (sti(rShip.Type.(sShipType)) != true) { continue; }

		// --->>> логика привязки к нации
		bOk = false;
		if (CheckAttribute(rShip, "nation"))
		{
			makearef(aNation, rShip.nation);
			int q = GetAttributesNum(aNation);
			for (int j = 0; j < q; j++)
			{
				sAttr = GetAttributeName(GetAttributeN(aNation, j));
				if (GetNationTypeByName(sAttr) == iNation && rShip.nation.(sAttr) == true) bOk = true;
			}
		}
		if (!bOk) { continue; }
		// логика привязки к нации <<<---

		iShips[iShipsNum] = i;
		iShipsNum++;
	}
	if (iShipsNum == 0)
	{
		return INVALID_SHIP_TYPE;
	}

	int iBaseShipType = iShips[rand(iShipsNum - 1)];

	return iBaseShipType;
}
// <<<--- ZhilyaevDm 27.10.22

int CreateBaseShip(int iBaseType)
{
	int iShip = FindFirstEmptyShip();

	if(iShip == -1)
	{
        trace("Can not find empty ship slot");
		return SHIP_NOTUSED;
	}

	ref rBaseShip = GetShipByType(iBaseType);
	ref rRealShip = GetRealShip(iShip);
	DeleteAttribute(rRealShip, "");
	CopyAttributes(rRealShip, rBaseShip);
    rRealShip.index = iShip;
    rRealShip.BaseType = iBaseType;
    if (!CheckAttribute(rRealShip, "ship.upgrades.hull")) // если задано в базе, то квестовый тип, не перебивать
    {
    	rRealShip.ship.upgrades.hull = 1;  //признак корабля теперь
    }
	rRealShip.ship.upgrades.sails = 1;
	
    rRealShip.BaseName = rRealShip.name; // запоминалка для нужд, тк далее идет "странное"
	if (rRealShip.name != "Fort" && rRealShip.name != "Boat") // не знаю зачем :(
	{
		rRealShip.name = rRealShip.name + "1"; // без этого вылет, но почему!!!!!!!!!!!!!!
		// выяснил - папка с моделью имеет на конце 1, вот и ответ
	}
    rRealShip.Stolen = 0;  // ворованность - честный
    
	if (iArcadeSails == 0) // момент инерции ниже для тактики
	{
	    rRealShip.InertiaAccelerationY = stf(rRealShip.InertiaAccelerationY) / 2.0;
	}
    
	return iShip;
}

void ResetShipCannonsDamages(ref refCharacter)
{
	aref refShip;
	
	makearef(refShip, refCharacter.Ship);
	refShip.Cannons.Borts = "";

	if (CheckAttribute(refShip, "Cannons.Borts.cannonf.damages")) { DeleteAttribute(refShip, "Cannons.Borts.cannonf.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.cannonb.damages")) { DeleteAttribute(refShip, "Cannons.Borts.cannonb.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.cannonl.damages")) { DeleteAttribute(refShip, "Cannons.Borts.cannonl.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.cannonr.damages")) { DeleteAttribute(refShip, "Cannons.Borts.cannonr.damages"); }		
		
	if (CheckAttribute(refShip, "Cannons.Borts.fcannon.damages")) { DeleteAttribute(refShip, "Cannons.Borts.fcannon.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.bcannon.damages")) { DeleteAttribute(refShip, "Cannons.Borts.bcannon.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.lcannon.damages")) { DeleteAttribute(refShip, "Cannons.Borts.lcannon.damages"); }
	if (CheckAttribute(refShip, "Cannons.Borts.rcannon.damages")) { DeleteAttribute(refShip, "Cannons.Borts.rcannon.damages"); }
}

// Генерация корабля для верфи
int GenerateStoreShip(int iBaseType)
{
	int iShip = GenerateShip(iBaseType, 0); // честный 

	if (iShip == -1)
	{
		return SHIP_NOTUSED;
	}
	
	ref rRealShip = GetRealShip(iShip);
    rRealShip.StoreShip = true;  // корабль на верфи, трется отдельным методом
    
	return iShip;	
}

// -> ugeen 25.01.09 -> для верфи
int GenerateStoreShipExt(int iBaseType, ref chr)
{
	int iShip = GenerateShipExt(iBaseType, 0, chr); // честный 

	if (iShip == -1)
	{
		return SHIP_NOTUSED;
	}
	
	ref rRealShip = GetRealShip(iShip);
	rRealShip.StoreShip = true;  // корабль на верфи, трется отдельным методом
    
	return iShip;	
}
// <- ugeen

// переработка метода 26.05.06 boal
// идея: трем все корабли, где нет НПС-владельца, все галки пофиг
void EmptyAllFantomShips()
{
	int  i;
	ref  chr;
	// оптимальный код - два прохода, а не вложенный цикл. В начале всем сносим флаг удаляемости, потом проставляем, где есть НПС
	for (i = 0; i < REAL_SHIPS_QUANTITY; i++)
	{
        DeleteAttribute(&RealShips[i], "lock");
	}
	// лодку ставим, тк он имеет индекс НПС за пределами MAX_CHARACTERS
	//RealShips[sti(Characters[BOAT_CHARACTER].ship.type)].lock = true;
	// теперь НПС-кэпы
	for (i = 0; i < MAX_CHARACTERS; i++)
	{
        chr = &Characters[i];
        if (CheckAttribute(chr, "ship.type") && chr.ship.type != SHIP_NOTUSED)
        {
            RealShips[sti(chr.ship.type)].lock = true;
        }
        // очистка для верфи
		if (CheckAttribute(chr, "shipyard"))
		{
		    FreeShipFromShipyard(chr);
		}
	}
	for (i = FANTOM_CHARACTERS; i < TOTAL_CHARACTERS; i++) // фантомы в море
	{
        chr = &Characters[i];
        if (CheckAttribute(chr, "ship.type") && chr.ship.type != SHIP_NOTUSED)
        {
            RealShips[sti(chr.ship.type)].lock = true;
        }
	}
	// теперь сборка мусора
	for (i = 0; i < REAL_SHIPS_QUANTITY; i++)
	{
        if (!CheckAttribute(&RealShips[i], "lock") && !CheckAttribute(&RealShips[i], "StoreShip"))  // на верфи корабли не трем
        {
			DeleteAttribute(&RealShips[i], "");
		}
	}
}

int GetRealShipsCount()
{ // метод для отладки
	int result = 0;

	for (int i= 0; i< REAL_SHIPS_QUANTITY; i++)
	{
		if (checkAttribute(&RealShips[i], "name"))
		{
			result++;
		}
	}

	return result;
}

// поиск пустого слота
int FindFirstEmptyShip()
{
	for(int i = 0; i < REAL_SHIPS_QUANTITY; i++)
	{
		if(!checkAttribute(&RealShips[i], "name"))
		{
			return i;
		}
	}

	return -1;
}


/*
float SpeedByHullUpgrade(aref refCharacter)
{
	int iHull = sti(refCharacter.ship.upgrades.hull) - 1;

	float fResult = 1.0 - (0.05 * iHull);

	return fResult;
}


float SpeedBySailsUpgrade(aref refCharacter)
{
	int iSails = sti(refCharacter.ship.upgrades.sails) - 1;
	iSails = iSails * 5;
	float fResult = 1.0 + (0.01 * iSails);

	return fResult;
}
*/

float FindShipSpeed(aref refCharacter)
{
	if(!CheckAttribute(refCharacter, "Ship.type"))
	{
		trace("Character " + refCharacter.id + " have no ship!");
		return 0.0;
	}
	int nShipType = sti(refCharacter.ship.type);
	if(nShipType == SHIP_NOTUSED)
	{
		trace("Character " + refCharacter.id + " have invalid ship!");
		return 1.0;
	}
	ref rShip = GetRealShip(nShipType);
	
	float	fMaxSpeedZ = stf(rShip.SpeedRate);  // базовая скорость
	float fShipHp = stf(refCharacter.ship.hp);
	float fShipMaxHp = stf(rShip.HP);
	float fSpeedFromHp = fShipHp/fShipMaxHp;
	fSpeedFromHp = 0.8 + fSpeedFromHp*0.3;

	if(fSpeedFromHp > 1.0) // некий запас перед снижением скорости
	{
		fSpeedFromHp = 1.0;
	}
	
	fMaxSpeedZ = fMaxSpeedZ * AIShip_isPerksUse(CheckCharacterPerk(refCharacter, "MapMaker"), 1.0, 1.05);
	
	float fSailsDamage = stf(refCharacter.ship.sp);

	float	fTRFromSailDamage = 1.0;
	if (fSailsDamage < 90.0)
	{
		fTRFromSailDamage = Bring2Range(0.1, 1.0, 0.1, 90.0, fSailsDamage);
	}

	float fTRFromShipState = fSpeedFromHp * fTRFromSailDamage;
	
	float	fLoad = Clampf(stf(refCharacter.Ship.Cargo.Load) / stf(rShip.Capacity));
	float	fTRFromWeight = Clampf(1.03 - stf(rShip.SpeedDependWeight) * fLoad);
	float   fTRFromSkill = SpeedBySkill(refCharacter);
	// от команды
	float fCrewMax = stf(rShip.MaxCrew);
	float fCrewOpt = stf(rShip.OptCrew);
	float fCrewCur = stf(refCharacter.Ship.Crew.Quantity);
	if (fCrewCur > fCrewMax)
	{
		fCrewCur = fCrewMax;
	}
	float  fTRFromPeople;
	fTRFromPeople = 0.85 + ((GetCrewExp(refCharacter, "Sailors") * fCrewCur) / (fCrewOpt * GetCrewExpRate())) * 0.15;
	if (fTRFromPeople > 1) fTRFromPeople = 1;
	 
	fMaxSpeedZ = fMaxSpeedZ * fTRFromWeight * fTRFromSkill * fTRFromShipState * fTRFromPeople;

	return fMaxSpeedZ;
}

float SpeedBySkill(aref refCharacter)
{
	float fSpeedPerk;

	float fSkill = GetSummonSkillFromNameToOld(refCharacter, SKILL_SAILING);
	float fTRFromSkill = 0.7 + (0.03 *  fSkill);
		
    fSpeedPerk = AIShip_isPerksUse(CheckOfficersPerk(refCharacter, "ShipSpeedUp"), 1.0, 1.15);   //slib
    fSpeedPerk = AIShip_isPerksUse(CheckOfficersPerk(refCharacter, "SailingProfessional"), fSpeedPerk, 1.20);
	
	return fTRFromSKill*fSpeedPerk;
}

float FindShipTurnRate(aref refCharacter)
{
	if(!CheckAttribute(refCharacter, "Ship.type"))
	{
		trace("Character " + refCharacter.id + " have no ship!");
		return 0.0;
	}
	int nShipType = sti(refCharacter.ship.type);
	if(nShipType == SHIP_NOTUSED)
	{
		trace("Character " + refCharacter.id + " have invalid ship!");
		return 1.0;
	}
	ref rShip = GetRealShip(nShipType);

	float fShipHp = stf(refCharacter.ship.hp);
	float fShipMaxHp = stf(rShip.HP);
	float fSpeedFromHp = fShipHp/fShipMaxHp;
	fSpeedFromHp = 0.7 + fSpeedFromHp * 0.3;

	float fLoad = Clampf(stf(refCharacter.Ship.Cargo.Load) / stf(rShip.Capacity));
	float fTRFromWeight;
	if (iArcadeSails == 1)
	{
		fTRFromWeight = Clampf(1.03 - stf(rShip.TurnDependWeight) * fLoad);
	}
	else
	{
		fTRFromWeight = Clampf(1.03 - 2.0*stf(rShip.TurnDependWeight) * fLoad); // влияние веса больше
	}
	// не реализовано, всегда 1 float fTRFromSpeed = 1.0 - stf(rShip.TurnDependSpeed);
	// от команды
	//float fCrewMin = stf(rShip.MinCrew);
	float fCrewMax = stf(rShip.MaxCrew);
	float fCrewOpt = stf(rShip.OptCrew);
	float fCrewCur = stf(refCharacter.Ship.Crew.Quantity);
	if (fCrewCur > fCrewMax)
	{
		fCrewCur = fCrewMax;
	}
	float  fTRFromPeople;
	if (iArcadeSails == 1)
	{
		fTRFromPeople = 0.3 + ((GetCrewExp(refCharacter, "Sailors") * fCrewCur) / (fCrewOpt * GetCrewExpRate())) * 0.7;
	}
	else
	{
		fTRFromPeople = 0.05 + ((GetCrewExp(refCharacter, "Sailors") * fCrewCur) / (fCrewOpt * GetCrewExpRate())) * 0.95;
	}
	if (fTRFromPeople > 1) fTRFromPeople = 1;
	
	float fTRFromSKill = TurnBySkill(refCharacter);

	float fTRFromSailDamage = Bring2Range(0.05, 1.0, 0.1, 100.0, stf(refCharacter.ship.sp));

	float fTurn = fTRFromWeight * fTRFromSkill * fTRFromPeople * fTRFromSailDamage * fSpeedFromHp;

	//Log_info(refCharacter.id + "  " + fTurn);
	return fTurn;	
}

float TurnBySkill(aref refCharacter)
{
	float fSkill = GetSummonSkillFromNameToOld(refCharacter, SKILL_SAILING);
	float fTRFromSKill;

    if (iArcadeSails == 1)
	{
		fTRFromSKill = 0.5 + (0.05 * fSkill);
	}
	else
	{
		fTRFromSKill = 0.3 + (0.07 * fSkill);
	}
	
    float fSpeedPerk = AIShip_isPerksUse(CheckOfficersPerk(refCharacter, "ShipTurnRateUp"), 1.0, 1.15);   //slib
    fSpeedPerk = AIShip_isPerksUse(CheckOfficersPerk(refCharacter, "SailingProfessional"), fSpeedPerk, 1.20);
    float fFastTurn180 = AIShip_isPerksUse(CheckOfficersPerk(refCharacter, "Turn180"), 1.0, 4.0);
    
	return fTRFromSKill*fSpeedPerk*fFastTurn180;
}

int GetCannonsNum(aref chr)
{
	int nShipType = sti(chr.ship.type);
    ref refBaseShip = GetRealShip(nShipType);
	
	int iCannons = GetBortIntactCannonsNum(chr, "fcannon", sti(refBaseShip.fcannon)) + GetBortIntactCannonsNum(chr, "bcannon", sti(refBaseShip.bcannon)) + GetBortIntactCannonsNum(chr, "lcannon", sti(refBaseShip.lcannon)) + GetBortIntactCannonsNum(chr, "rcannon", sti(refBaseShip.rcannon));
		
	return iCannons;
}

// ugeen  02.02.09 
int GetCannonsNumMax(aref chr)
{
	int nShipType = sti(chr.ship.type);
	ref refBaseShip = GetRealShip(nShipType);

	int iCannons = sti(refBaseShip.CannonsQuantityMax);
		
	return iCannons;
}
// ugeen

// boal
int GetBortCannonsQty(aref chr, string  sType)
{
	int nShipType = sti(chr.ship.type);
    ref refBaseShip = GetRealShip(nShipType);

	int iCannons = GetBortIntactCannonsNum(chr, sType, sti(refBaseShip.(sType)));

	return iCannons;
}

int GetBortCannonsMaxQty(aref chr, string  sType)
{
	int nShipType = sti(chr.ship.type);
    ref refBaseShip = GetRealShip(nShipType);

	int iCannons = sti(refBaseShip.(sType));

	return iCannons;
}

int GetBortCannonsBaseQty(ref chr, string sBort)
{
	int nShipType = sti(chr.ship.type);
	ref rRealShip = GetRealShip(nShipType);
	ref rBaseShip = GetShipByType(sti(rRealShip.BaseType));
	
	int iCannons = sti(rBaseShip.(sBort));
	
	return iCannons;
}

///////////////////////  ВЕРФИ //////////////////////
void SetShipyardStore(ref NPChar)
{
    int    iTest_ship, i;
	string attrName;
	
	if (CheckAttribute(NPChar, "shipyard")) return; // ещё есть корабли с того раза
	
	SaveCurrentNpcQuestDateParam(npchar, "shipyardDate"); // дата заполнения верфи
    
    if (bBettaTestMode)
    {
        for (i = 1; i <=SHIP_TYPES_QUANTITY; i++)
        {
            attrName = "ship" + i;
			FillShipParamShipyard(NPChar, GenerateStoreShipExt(i-1, NPChar), attrName);
        }
        return;
    }

    FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARKAS, NPChar), "ship1");
	FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_TARTANE, NPChar), "ship1");
    
	iTest_ship = rand(7);
	if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LIUTO, NPChar), "ship2");
	if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_WAR_TARTANE, NPChar), "ship2");
	if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BILANCETTA, NPChar), "ship2");
	if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LUGGER, NPChar), "ship2");
	if (iTest_ship == 5) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SLOOP, NPChar), "ship2");

	if (sti(PChar.rank) > 1) 
	{
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LUGGER, NPChar), "ship3");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SLOOP, NPChar), "ship3");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SLOOP_W, NPChar), "ship3");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LUGGER_W, NPChar), "ship3");

		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SLOOP, NPChar), "ship4");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LUGGER, NPChar), "ship4");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CUTTER, NPChar), "ship4");
	}
    if (sti(PChar.rank) > 2)
    {
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SCHOONER, NPChar), "ship5");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARQUE, NPChar), "ship5");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARQUE_W, NPChar), "ship5");
	
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SCHOONER, NPChar), "ship6");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SCHOONER_W, NPChar), "ship6");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARQUE, NPChar), "ship6");
	}
	if (sti(PChar.rank) > 3)
    {
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SCHOONER, NPChar), "ship8");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SCHOONER_W, NPChar), "ship8");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIG_L, NPChar), "ship8");
	
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_FLEUT, NPChar), "ship9");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARQUE_W, NPChar), "ship9");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_XEBEK_L, NPChar), "ship9");
	
		iTest_ship = rand(7);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_FLEUT, NPChar), "ship10");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARQUE, NPChar), "ship10");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BARKENTINE, NPChar), "ship10");		
	}
	if (sti(PChar.rank) > 5)
    {
		iTest_ship = rand(10);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIG, NPChar), "ship11");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_FLEUT, NPChar), "ship11");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_XEBEK_L, NPChar), "ship11");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON_L, NPChar), "ship11");
		if (iTest_ship == 5) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CARAVEL, NPChar), "ship11");
	
		iTest_ship = rand(10);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIG, NPChar), "ship12");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON, NPChar), "ship12");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CARAVEL, NPChar), "ship12");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIG_L, NPChar), "ship12");
		if (iTest_ship == 5) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CORVETTE_L, NPChar), "ship12");

		iTest_ship = rand(10);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIG_L, NPChar), "ship13");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON_L, NPChar), "ship13");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_BRIGANTINE, NPChar), "ship13");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_SHNYAVA, NPChar), "ship13");
		if (iTest_ship == 5) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CARAVEL_W, NPChar), "ship13");
    }
    if (sti(PChar.rank) > 8)
    {
		iTest_ship = rand(50);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON_L, NPChar), "ship14");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CORVETTE, NPChar), "ship14");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON, NPChar), "ship14");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CARACCA, NPChar), "ship14");
	
		iTest_ship = rand(70);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON_H, NPChar), "ship15");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_PINNACE, NPChar), "ship15");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_EASTINDIAMAN, NPChar), "ship15");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_POLACRE, NPChar), "ship15");
    }
    
    if (sti(PChar.rank) > 12)
    {	
		iTest_ship = rand(70);
		if (iTest_ship == 1) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_FRIGATE, NPChar), "ship16");
		if (iTest_ship == 2) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_PINNACE, NPChar), "ship16");
		if (iTest_ship == 3) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON_H, NPChar), "ship16");
		if (iTest_ship == 4) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_CORVETTE, NPChar), "ship16");
		if (iTest_ship == 5) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_POLACRE, NPChar), "ship16");

		if (iTest_ship == 6) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_FRIGATE_H, NPChar), "ship16");
		if (iTest_ship == 7) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_GALEON, NPChar), "ship16");
		if (iTest_ship == 8) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_EASTINDIAMAN, NPChar), "ship16");
		if (iTest_ship == 9) FillShipParamShipyard(NPChar, GenerateStoreShipExt(SHIP_LINESHIP, NPChar), "ship16");	
    }    
}

void FreeShipFromShipyard(ref NPChar)
{
	if (GetNpcQuestPastDayParam(NPChar, "shipyardDate") > (3 + rand(6)))
	{
        aref   arDest, arImt;
		string sAttr;
		int    iNum, i, iShip;
		makearef(arDest, NPChar.shipyard);
		iNum = GetAttributesNum(arDest);
		
		for (i = 0; i < iNum; i++)
		{
	    	arImt = GetAttributeN(arDest, i);
	    	iShip = sti(arImt.Type); //GetAttributeValue(arImt));
	    	DeleteAttribute(&RealShips[iShip], "StoreShip"); // можно тереть
	    }
		
        DeleteAttribute(NPChar, "shipyard");
	}
}

void RemoveCannonsFromBortShipyard(ref chr, string sBort)
{
//	int     maxQty = GetBortCannonsMaxQty(chr, sBort);
	int     maxQty = GetBortCannonsBaseQty(chr, sBort);
	int     i;
	string  attr;
	string  sBort_real = sBort;

		
	if(sBort == "rcannon") sBort_real = "cannonr";
	if(sBort == "lcannon") sBort_real = "cannonl";
	if(sBort == "fcannon") sBort_real = "cannonf";
	if(sBort == "bcannon") sBort_real = "cannonb";
	
	// нулим кол-во пушек на борту
	for (i = 0; i < maxQty; i++)
	{
		attr = "c" + i;
		// поломана на 100%, не палит, те нет её
		chr.Ship.Cannons.borts.(sBort).damages.(attr) = 1.0;  
		chr.Ship.Cannons.borts.(sBort_real).damages.(attr) = 1.0; 
	}
	
	RecalculateCargoLoad(chr);  // пересчет, тк пушки снялись
}

void RemoveAllCannonsShipyardShip(ref NPChar)
{
    RemoveCannonsFromBortShipyard(NPChar, "fcannon");
    RemoveCannonsFromBortShipyard(NPChar, "bcannon");
    RemoveCannonsFromBortShipyard(NPChar, "rcannon");
    RemoveCannonsFromBortShipyard(NPChar, "lcannon");
}

void FillShipParamShipyard(ref NPChar, int _iType, string _sShipNum)
{
    aref    arTo, arFrom;
    
	DeleteAttribute(NPChar, "ship");
    NPChar.Ship.Type = _iType;
    
	SetRandomNameToShip(NPChar);

    SetBaseShipData(NPChar);
    SetCrewQuantity(NPChar, 0);
    if (sti(NPChar.Ship.Cannons.Type) != CANNON_TYPE_NONECANNON)
    {
    	NPChar.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS3; // to_do переделать на продажу без орудий вообще
    }
	RemoveAllCannonsShipyardShip(NPChar);	//нулим пушки .. сами покупайте какие надо
    DeleteAttribute(NPChar, "Ship.Cargo");  //пустой трюм
    SetGoodsInitNull(NPChar);
    RecalculateCargoLoad(NPChar);
    
    FillShipyardShipBack(NPChar, _sShipNum);
}

void FillShipyardShip(ref NPChar, string _sShipNum)
{
    aref    arTo, arFrom;

	DeleteAttribute(NPChar, "ship");

    makearef(arFrom,   NPChar.shipyard.(_sShipNum));
    NPChar.Ship = "";
	makearef(arTo, NPChar.Ship);
	CopyAttributes(arTo, arFrom);
}

void FillShipyardShipBack(ref NPChar, string _sShipNum)
{
    aref    arTo, arFrom;

  	DeleteAttribute(NPChar, "shipyard." + _sShipNum);
    NPChar.shipyard.(_sShipNum) = "";
    makearef(arTo,   NPChar.shipyard.(_sShipNum));
	makearef(arFrom, NPChar.Ship);
	CopyAttributes(arTo, arFrom);
}

int GetSailsTuningPrice(ref _chr, ref _shipyard, float _rate)
{
	int st  = GetCharacterShipType(_chr);
	ref shref;
	
	if (st==SHIP_NOTUSED) return 0;
	shref = GetRealShip(st);

	return makeint(stf(_shipyard.ShipCostRate) * sti(shref.Price) * _rate / 10) * 10; // кратно 10
}

// Warship 26.07.09 Вынес сюда
// int st - GetCharacterShipType(_character)
// ref _shipyard - референс на верфиста
int GetShipPriceByType(int st, ref _shipyard)
{
	if(st==SHIP_NOTUSED) return 0;
	ref shref = GetRealShip(st);

	return makeint(stf(_shipyard.ShipCostRate) * sti(shref.Price));
}

// Метод вернёт цену продажи корабля персонажа с учетом всех скилов (для верфи)
int GetShipSellPrice(ref _chr, ref _shipyard)
{
	int st = GetCharacterShipType(_chr);
	int price = GetShipPriceByType(st, _shipyard);
	price = makeint(price - 1.5*GetSailRepairCost(st, GetSailDamagePercent(_chr), _shipyard));
	price = makeint(price - 1.5*GetHullRepairCost(st, GetHullDamagePercent(_chr), _shipyard));

	float nCommerce   = GetSummonSkillFromNameToOld(GetMainCharacter(), SKILL_COMMERCE) + 0.001;

	if(CheckOfficersPerk(pchar,"AdvancedCommerce"))	{ nCommerce += 4; }
	else
	{
		if(CheckOfficersPerk(pchar,"BasicCommerce"))	{ nCommerce += 2; }
	}

	price = price - price / (nCommerce*10.5);

	ref rRealShip = GetRealShip(st);

	if (sti(rRealShip.Stolen) == true) //проверка на ворованный
	{
    	price = makeint(price/3);
    }
    if (price < 0 && sti(_chr.Ship.Type) != SHIP_NOTUSED)
	{
	   price = 0;
	}

	return price;
}

// Метод вернёт цену покупки корабля персонажа с учетом всех скилов (для верфи)
int GetShipBuyPrice(int iType, ref _shipyard)
{
	// boal учет скилов торговли 22.01.2004 -->
	float nCommerce   = GetSummonSkillFromNameToOld(GetMainCharacter(), SKILL_COMMERCE);

    if(CheckOfficersPerk(pchar,"AdvancedCommerce"))	{ nCommerce += 4; }
	else
	{
		if(CheckOfficersPerk(pchar,"BasicCommerce"))	{ nCommerce += 2; }
	}

    return makeint(GetShipPriceByType(iType, _shipyard) + GetShipPriceByType(iType, _shipyard)/(nCommerce*10));
    // boal 22.01.2004 <--
}

int GetSailRepairCost(int shipType, int repairPercent, ref _shipyard)
{
	int SailRepairCoeff = 1;
	int shipPrice = GetShipPriceByType(shipType, _shipyard);
	if(shipPrice<=0) return 0;
	ref shref = GetRealShip(shipType);
	if(CheckAttribute(shref,"Tuning.GhostShip")) SailRepairCoeff = 3;
	float scost = SailRepairCoeff * (shipPrice*SAIL_COST_PERCENT+99.0)/100.0;
	return makeint((scost*repairPercent+99.0)/100.0);
}

int GetHullRepairCost(int shipType,int repairPercent, ref _shipyard)
{
	int HullRepairCoeff = 1;
	int shipPrice = GetShipPriceByType(shipType, _shipyard);
	if(shipPrice<=0) return 0;
	ref shref = GetRealShip(shipType);
	if(CheckAttribute(shref,"Tuning.GhostShip")) HullRepairCoeff = 3;
	float scost = HullRepairCoeff * (shipPrice*HULL_COST_PERCENT+99.0)/100.0;
	return makeint((scost*repairPercent+99.0)/100.0);
}

int GetSailDamagePercent(ref _chr)
{
	return 100 - MakeInt(GetSailPercent(_chr));
}

int GetHullDamagePercent(ref _chr)
{
	return 100 - MakeInt(GetHullPercent(_chr));
}
///////////////////////////////////////////////////////////////   ВЕРФИ  <--
// палуба от типа корабля
string GetShip_deck(ref chr, bool map2sea)
{
	string ret, add;

	int    iType = sti(chr.Ship.Type);
	if (map2sea)
	{
	    ret = "Ship_deck_";
	}
	else
	{
	    ret = "Deck_Near_Ship_";
	}
	add = "Low"; // если нет типа
	if (iType != SHIP_NOTUSED)
	{
        ref rRealShip = GetRealShip(iType);
        
		add = rRealShip.DeckType;
	}
	
	return ret + add;
}

void MakeCloneShipDeck(ref chr, bool map2sea)
{
	ref rOrg, rClone;
	int iOrg, iClone;
    string locId, toLocId;
    
    locId = GetShip_deck(chr, map2sea);
    if (map2sea)
	{
	    toLocId = "Ship_deck";
	}
	else
	{
	    toLocId = "Deck_Near_Ship";
	}
	
	iOrg = FindLocation(locId);
	iClone = FindLocation(toLocId);

	makeref(rOrg, Locations[iOrg]);
	makeref(rClone, Locations[iClone]);

	DeleteAttribute(rClone, "");
	CopyAttributes(rClone, rOrg);
	rClone.id = toLocId;
	rClone.index = iClone;
}

void MakeCloneFortBoarding(string fromLocId)
{
	ref rOrg, rClone;
	int iOrg, iClone;
    string toLocId;
    
    toLocId = "BOARDING_FORT";

	iOrg = FindLocation(fromLocId);
	iClone = FindLocation(toLocId);

	makeref(rOrg, Locations[iOrg]);
	makeref(rClone, Locations[iClone]);

	DeleteAttribute(rClone, "");
	CopyAttributes(rClone, rOrg);
	rClone.id = toLocId;
	rClone.index = iClone;
	
    rClone.type = "fort_attack";
	LAi_LocationFantomsGen(rClone, false);
	DeleteAttribute(rClone, "reload");
	//это подкручивание другого файла локаторов. там нет goto, soldiers и пр. есть rld и дополн.сундуки
	if (rClone.models.always.locators == "fortV_locators") 
	{
		rClone.models.always.locators = "fortV_lAttack";
		rClone.models.always.fortV = "fortV_attack";	
	}
	else
	{
		rClone.models.always.locators = "fortVRight_lAttack";
		rClone.models.always.fortV = "fortVRight_attack";
	}
	rClone.boarding.locatorNum = 25;
	rClone.boarding = "false";
	rClone.boarding.nextdeck = "Boarding_fortyard"; // Jason
	rClone.image = "loading\Firefort.tga";
}

int GetPortManPrice(int Price, ref NPChar)
{
    if (!CheckAttribute(NPChar, "PortManPrice"))
	{
		NPChar.PortManPrice = (0.06 + frnd()*0.1);
	}
	return makeint(Price * stf(NPChar.PortManPrice) /10 ) * 10;
}

// ugeen --> альтернативный расчёт стоянки корабля в ПУ
int GetPortManPriceExt(ref NPChar, ref chref)
{
	float fLeadership = 1.5 - GetSummonSkillFromName(pchar, SKILL_LEADERSHIP)/120.0; // учитываем авторитет
	float fCommerce = 1.5 - GetSummonSkillFromName(pchar, SKILL_COMMERCE)/120.0; // учитываем торговлю
	float fRelation = 1.0; // учитываем  - родная нация или нет
	
	if(sti(NPChar.nation) == GetBaseHeroNation()) fRelation = 0.5; // если нация родная- снижаем цену в два раза
	int price = makeint(200 * MOD_SKILL_ENEMY_RATE * 1.56 * sqr(7.7 - sti(RealShips[sti(chref.Ship.Type)].Class)) * (1 + 0.5 * GetNationRelation2MainCharacter(sti(NPChar.nation))) * fRelation * fLeadership * fCommerce);
	price = price + sti(chref.Ship.Crew.Quantity) * 10; // LEO: за каждого матроса платим по 10 золота
	return price;
}
//<-- альтернативный расчёт стоянки корабля в ПУ

// Warship 31.05.09 Установка на корабль персонажа определённого паруса
// Путь относительно папки Textures
bool SetShipSailsFromFile(ref _char, String _sailTexturePath)
{
	ref realShip;
	int shipType = sti(_char.Ship.Type);

	if(shipType == SHIP_NOTUSED) // Есть ли корабль вообще
	{
		return false;
	}

	realShip = GetRealShip(shipType);
	
	realShip.EmblemedSails.normalTex = _sailTexturePath;
	
	return true;
}

string GetLocalShipAttrib(ref ship, string attrib) 
{
	if (CheckAttribute(ship, attrib)) return ship.(attrib);
	return ""; 
}

bool CheckShipAttribute(ref ship, string attrib)
{
	if(CheckAttribute(ship, attrib)) return true;
	return false;
}

string GetBaseShipParamFromType(int iType, string _param)
{
	object rBaseShip = GetShipByType(iType);
	return rBaseShip.(_param);
}

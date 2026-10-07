
#event_handler("evntChrPerkDelay","procChrPerkDelay");

object ChrPerksList;

extern void extrnInitPerks();

void InitPerks()
{
	if( LoadSegment("interface\perks\perks_init.c") )
	{
		extrnInitPerks();
		UnloadSegment("interface\perks\perks_init.c");
	}
}

bool CheckCharacterPerk(ref chref, string perkName)
{
	if( CheckAttribute(chref,"perks.list."+perkName) ) return true;
	return false;
}

bool SetCharacterPerk(ref chref, string perkName)
{
	chref.perks.list.(perkName) = true;
	// разовые применения при назначении -->
	switch (perkName)
	{
		case "HPPlus":
			LAi_SetHP(chref, LAi_GetCharacterMaxHP(chref), LAi_GetCharacterMaxHP(chref));
			return true; // нужен рефреш
		break;
		
		case "EnergyPlus":
			SetEnergyToCharacter(chref);
			return true;	
		break;
		
		case "Grus":
			return true;
		break;
		
		case "WildCaribbean":	// EvgAnat - дикие Карибы, получение перка
			SetWildCaribbean();
		break;
	}
	// разовые применения при назначении <--
	return false; // рефреш не нужен
}

void ActivateCharacterPerk(ref chref, string perkName)
{
	if( !CheckAttribute(&ChrPerksList,"list."+perkName) )
	{
		trace("Invalid perk name - " + perkName);
		return;
	}

	int timeDelay = 0;
	int timeDuration = 0;

	if( CheckAttribute(&ChrPerksList,"list."+perkName+".TimeDuration") )
	{	timeDelay = sti(ChrPerksList.list.(perkName).TimeDuration);
		timeDuration = timeDelay;
	}
	if( CheckAttribute(&ChrPerksList,"list."+perkName+".TimeDelay") )
	{	timeDelay = sti(ChrPerksList.list.(perkName).TimeDelay);
		if(timeDuration<=0)	{timeDuration=timeDelay;}
	}
    // boal fix
    // иначе после применения давался ГГ
	
    int cn;
    if (!CheckCharacterPerk(chref, perkName))
    {

        cn = GetOfficersPerkUsingIdx(chref, perkName);
        if (cn != -1)
        chref = GetCharacter(cn);
    }
    // <--
	chref.perks.list.(perkName).delay = timeDelay;
	chref.perks.list.(perkName).active = timeDuration;

	//if(sti(chref.index) == nMainCharacterIndex)
	if (sti(chref.index) == nMainCharacterIndex || isOfficerInShip(chref, false)) // наследие перка от офа boal 30.06.06
	{
		AddPerkToActiveList(perkName);
	}

	if(timeDelay>0) PostEvent("evntChrPerkDelay",1000,"sl",perkName,sti(chref.index));
	Event("eSwitchPerks","l",sti(chref.index));
	// fix boal всегда для ГГ
	Event("eSwitchPerks","l", GetMainCharacterIndex());
}

bool GetCharacterPerkUsing(ref chref, string perkName)
{   // можно ли пользовать умение (задержки нет)
	if( !CheckAttribute(chref,"perks.list."+perkName) ) return false;
	if( CheckAttribute(chref,"perks.list."+perkName+".delay") ) return false;
	return true;
}

bool GetOfficersPerkUsing(ref chref, string perkName)
{ // boal препишем внутрянку под новых офов, че в к3 не было? не ведаю.
	string  sOfficerType;	
	ref   offc;	   
	bool  ok = false; // boal fix 25.03.05 проверка на запрет
	bool  okDelay = true;
	int   cn;
	if (GetCharacterPerkUsing(chref, perkName)) {return true;} // босс отдельно
	if (!CheckAttribute(chref,"perks.list."+perkName))  // у гг нет перка вообще, тогда смотрим офов, иначе выход
	{
		for(int i=1; i<=6; i++)
		{
			sOfficerType = GetOfficerTypeByNum(i);
			if (CheckAttribute(&ChrPerksList, "list." + perkName + ".OfficerType") && ChrPerksList.list.(perkName).OfficerType == sOfficerType)
			{
				cn = sti(chref.Fellows.Passengers.(sOfficerType));
				if(cn<0) {continue;}
				offc = &Characters[cn];
				if (CheckAttribute(offc, "perks.list."+perkName) )          ok = true;
				if (CheckAttribute(offc, "perks.list."+perkName+".delay") ) okDelay = false;
				//if (GetCharacterPerkUsing(chref, perkName) return true;
			}
		}
	}
	return (ok) && (okDelay);
}

// boal
int GetOfficersPerkUsingIdx(ref chref, string perkName)
{
	string  sOfficerType;
	int     cn;
	if (GetCharacterPerkUsing(chref, perkName)) {return sti(chref.index);} // босс отдельно
	for(int i=1; i<=6; i++)
	{
		sOfficerType = GetOfficerTypeByNum(i);
		if (CheckAttribute(&ChrPerksList, "list." + perkName + ".OfficerType") && ChrPerksList.list.(perkName).OfficerType == sOfficerType)
		{
			cn = sti(chref.Fellows.Passengers.(sOfficerType));
			if(cn<0) {continue;}
			if (GetCharacterPerkUsing(&Characters[cn], perkName) ) {return cn;}
		}
	}
	return -1;
}

bool IsCharacterPerkOn(ref chref, string perkName)
{
	aref arRoot, arBase;
	makearef(arRoot,chref.perks.list);
	if( !CheckAttribute(arRoot,perkName) ) return false;
	makearef(arBase,ChrPerksList.list.(perkName));
	if( CheckAttribute(arBase,"TimeDuration") || CheckAttribute(arBase,"TimeDelay") ) {
		return CheckAttribute(arRoot,perkName+".active");
	}
	return true;
}

void CharacterPerkOff(ref chref, string perkName)
{
	if (perkName == "Turn180")
	{
		chref.Tmp.SpeedRecall = 0; // чтоб манёвр применить
	}
	DeleteAttribute(chref,"perks.list."+perkName+".active");
	Event("eSwitchPerks","l",sti(chref.index));	 
	// fix boal всегда для ГГ
	Event("eSwitchPerks","l", GetMainCharacterIndex());
	if (sti(chref.index) == nMainCharacterIndex || isOfficerInShip(chref, false)) // наследие перка от офа boal 30.06.06
	{
		DelPerkFromActiveList(perkName);
	}
}

bool CheckOfficersPerk(ref chref, string perkName)
{ // активность перка в данный момент, для временных - режим активности, а не задержки
	bool ret = CheckOfficersPerkWOSelf(chref, perkName);

	if (ret) return true;
	// самого НПС
	if(IsCharacterPerkOn(chref,perkName) == true)
	{
		return true;
	}
	return false;
}

bool CheckOfficersPerkWOSelf(ref chref, string perkName)
{
	int     iOfficer = -1;
	string  sOfficerType;
	if(chref.id == pchar.id)
	{
		// boal новая схема наследования. switch не наш метод
		if (CheckAttribute(&ChrPerksList, "list." + perkName + ".OfficerType"))
		{
			sOfficerType = ChrPerksList.list.(perkName).OfficerType;
			iOfficer = sti(pchar.Fellows.Passengers.(sOfficerType));
		}
	}

	if(iOfficer != -1)
	{
		if(IsCharacterPerkOn(GetCharacter(iOfficer), perkName) == true )
		{
			return true;
		}
	}
	return false;
}
// нигде не юзан
bool CheckCompanionsPerk(ref chref, string perkName)
{
	int i,cn;
	for(i=0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(chref,i);
		if(cn>=0)
		{	if( IsCharacterPerkOn(GetCharacter(cn),perkName) )	{return true;}
		}
	}
	return false; // fix by boal
}

void procChrPerkDelay()
{
	string perkName = GetEventData();
	int chrIdx = GetEventData();

	aref arPerk;
	makearef(arPerk,Characters[chrIdx].perks.list.(perkName));
	if( !CheckAttribute(arPerk,"delay") ) return;
	int delay = sti(arPerk.delay);
 	// фикс в каюте, палубе, абордаже
 	bool ok;
 	ok = (!bAbordageStarted) && (!bSeaReloadStarted);
 	if (ok || perkName == "Rush")
 	{
		delay--;
	}

	if( CheckAttribute(arPerk,"active") )
	{
		int iActive = sti(arPerk.active)-1;
		if( iActive>0 )	{arPerk.active = iActive;}
		else
		{
			CharacterPerkOff(GetCharacter(chrIdx),perkName);
		}
	}

	if( delay<=0 )
	{	DeleteAttribute(&Characters[chrIdx],"perks.list."+perkName+".delay");
		DeleteAttribute(&Characters[chrIdx],"perks.list."+perkName+".active");
		PostEvent("evntPerkAgainUsable",1);
	}
	else
	{
		Characters[chrIdx].perks.list.(perkName).delay = delay;
		PostEvent("evntChrPerkDelay",1000,"sl",perkName,chrIdx);
	}
}

void EnableUsingAbility(ref chref,string perkName)
{
    // boal fix
    // иначе после применения давался ГГ
	int cn;
    if (!CheckCharacterPerk(chref, perkName))
    {

        cn = GetOfficersPerkUsingIdx(chref, perkName);
        if (cn != -1)
        chref = GetCharacter(cn);
    }
    // <--
    
	Event("evntChrPerkDelay","sl",perkName, sti(chref.index));
}

void PerkLoad()
{
	// Saves restore the old registry; rebuild definitions, preserving learned perks.
	InitPerks();
//	int iRDTSC = RDTSC_B();
	string locName = pchar.location;
	aref arPerksRoot,arPerk;
	int i,j,n,tmpi;

	for(i=0; i<MAX_CHARACTERS; i++)
	{
		if(Characters[i].location == locName)
		{
			makearef(arPerksRoot,Characters[i].perks.list);
			n = GetAttributesNum(arPerksRoot);
			for(j=0; j<n; j++)
			{
				arPerk = GetAttributeN(arPerksRoot,j);
				if( CheckAttribute(arPerk,"delay") )
				{
					tmpi = sti(arPerk.delay);
					if( tmpi>0 )
					{
						PostEvent("evntChrPerkDelay",1000,"sl",GetAttributeName(arPerk),i);
					}
				}
			}
		}
	}

//	trace("TIME!!! PerkLoad() = " + RDTSC_E(iRDTSC));
}
// boal под новые слоты -->
void ClearActiveChrPerks(ref chref)
{
	int i,cn;
    ref offc; // boal
    string  sOfficerType;	
    
    if (bAbordageStarted || bSeaReloadStarted) return; 
	
	ClearActive(chref); // босс отдельно
	if (CheckAttribute(chref, "Fellows.Passengers")) // не у всех есть
	{
		for (i=1; i<=6; i++)
		{
			sOfficerType = GetOfficerTypeByNum(i);
			cn = sti(chref.Fellows.Passengers.(sOfficerType));
			if(cn<0) {continue;}
			offc = &Characters[cn];
			ClearActive(offc);
		}
	}
}

void ClearActive(ref offic)
{
	aref arPerksRoot, arPerk;
	int i,n;
	
	makearef(arPerksRoot, offic.perks.list);
		
	n = GetAttributesNum(arPerksRoot);
	for (i=0; i<n; i++)
	{
		arPerk = GetAttributeN(arPerksRoot,i);

		if (CheckAttribute(arPerk, "delay"))
        {
			DeleteAttribute(arPerk,"delay");
		}
		if (CheckAttribute(arPerk,"active"))
        {
			DeleteAttribute(arPerk,"active");
		}
	}
}
// был баг К3, этот перк не работал вовсе
void AcceptWindCatcherPerk(ref refCharacter)
{
    int  nShipType;
    ref  refBaseShip, refRealShip;

	nShipType = GetCharacterShipType(refCharacter);
	if (nShipType==SHIP_NOTUSED) return;

	refRealShip = GetRealShip(nShipType);
	refBaseShip = GetShipByType(sti(refRealShip.BaseType));
		
	if (CheckOfficersPerk(refCharacter, "WindCatcher"))
	{
		refRealShip.InertiaAccelerationX	= stf(refBaseShip.InertiaAccelerationX) + stf(refBaseShip.InertiaAccelerationX) / 10.0;
		refRealShip.InertiaAccelerationY	= stf(refBaseShip.InertiaAccelerationY) + stf(refBaseShip.InertiaAccelerationY) / 10.0;
		refRealShip.InertiaAccelerationZ	= stf(refBaseShip.InertiaAccelerationZ) + stf(refBaseShip.InertiaAccelerationZ) / 10.0;
		// потмоу что перк помогает только быстрее набирать скорость, нет торможения
	}
	else
	{   // вернём, если перк снят
	    refRealShip.InertiaAccelerationX	= stf(refBaseShip.InertiaAccelerationX);
		refRealShip.InertiaAccelerationY	= stf(refBaseShip.InertiaAccelerationY);
		refRealShip.InertiaAccelerationZ	= stf(refBaseShip.InertiaAccelerationZ);
	}
	if (iArcadeSails == 0) // момент инерции ниже для тактики
	{
	    refRealShip.InertiaAccelerationY = stf(refRealShip.InertiaAccelerationY) / 2.0;
	}
}

// EvgAnat - переключение опции быстрого нахождения предметов -->
void SetHawkEye(bool isEnabled)
{
	ref itm;
	int itemN;
	if(isEnabled)
	{
		for (itemN=0; itemN<ITEMS_QUANTITY; itemN++)
		{
			makeref(itm,Items[itemN]);
			if(CheckAttribute(itm, "model"))
			{
				switch(itm.model)
				{
					case "emerald":		itm.model = "emerald-new";		break;
					case "diamond":		itm.model = "diamond-new";		break;
					case "ruby":		itm.model = "ruby-new";			break;
					case "sapphire":	itm.model = "sapphire-new";		break;
					case "EmeraldA":	itm.model = "EmeraldA-new";		break;
					case "DiamondA":	itm.model = "DiamondA-new";		break;
					case "RubyA":		itm.model = "RubyA-new";		break;
					case "SapphireA":	itm.model = "SapphireA-new";	break;
					case "keygrand":	itm.model = "keygrand-new";		break;
					case "purse":		itm.model = "purse-new";		break;
					case "pursel":		itm.model = "pursel-new";		break;
					case "pursem":		itm.model = "pursem-new";		break;
					case "stonemap":	itm.model = "stonemap-new";		break;
					case "RingJew":		itm.model = "RingJew-new";		break;
					case "RingSim":		itm.model = "RingSim-new";		break;
					case "RingSimGold":	itm.model = "RingSimGold-new";	break;
				}
			}
		}
	}
	else
	{
		for (itemN=0; itemN<ITEMS_QUANTITY; itemN++)
		{
			makeref(itm,Items[itemN]);
			if(CheckAttribute(itm, "model"))
			{
				switch(itm.model)
				{
					case "emerald-new":		itm.model = "emerald";		break;
					case "diamond-new":		itm.model = "diamond";		break;
					case "ruby-new":		itm.model = "ruby";			break;
					case "sapphire-new":	itm.model = "sapphire";		break;
					case "EmeraldA-new":	itm.model = "EmeraldA";		break;
					case "DiamondA-new":	itm.model = "DiamondA";		break;
					case "RubyA-new":		itm.model = "RubyA";		break;
					case "SapphireA-new":	itm.model = "SapphireA";	break;
					case "keygrand-new":	itm.model = "keygrand";		break;
					case "purse-new":		itm.model = "purse";		break;
					case "pursel-new":		itm.model = "pursel";		break;
					case "pursem-new":		itm.model = "pursem";		break;
					case "stonemap-new":	itm.model = "stonemap";		break;
					case "RingJew-new":		itm.model = "RingJew";		break;
					case "RingSim-new":		itm.model = "RingSim";		break;
					case "RingSimGold-new":	itm.model = "RingSimGold";	break;
				}
			}
		}
	}
}
// EvgAnat - переключение опции быстрого нахождения предметов <--

// EvgAnat - дикие Карибы, возвращение сфер во все локации
void SetWildCaribbean()
{
	//QuestionsInit();
	//SeaQuestionsInit();
	int n;
	n = FindLocation("SentJons_TownCave");
	SetSphereToLocation(n, "DungeonDuffer1");
	n = FindLocation("Antigua_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("Barbados_Cave");
	SetSphereToLocation(n, "DungeonDuffer2");
	n = FindLocation("Beliz_Cave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("Beliz_Cave_2");
	SetSphereToLocation(n, "DungeonDuffer2");
	n = FindLocation("Bermudes_Dungeon");
	SetSphereToLocation(n, "DungeonVault1");
	n = FindLocation("Bermudes_Cavern");
	SetSphereToLocation(n, "cavernLow1");
	n = FindLocation("Caiman_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("Caracas_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("Cartahena_Cave");
	SetSphereToLocation(n, "cavernMedium2");
	n = FindLocation("Cumana_Cave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("Curacao_Cave");
	SetSphereToLocation(n, "cavernMedium2");
	n = FindLocation("Dominica_Grot");
	SetSphereToLocation(n, "grotto2");
	n = FindLocation("Guadeloupe_Cave");
	SetSphereToLocation(n, "cavernBig1");
	n = FindLocation("Jamaica_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("Maracaibo_Cave");
	SetSphereToLocation(n, "cavernMedium2");
	n = FindLocation("FortFrance_Dungeon");
	SetSphereToLocation(n, "DungeonVault2");
	n = FindLocation("Charles_Grot");
	SetSphereToLocation(n, "grotto2");
	n = FindLocation("Panama_Cave");
	SetSphereToLocation(n, "DungeonDuffer2");
	n = FindLocation("Pearl_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("PortoBello_Cave");
	SetSphereToLocation(n, "DungeonDuffer2");
	n = FindLocation("SanJuan_Cave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("SantaCatalina_Cave");
	SetSphereToLocation(n, "cavernMedium2");
	n = FindLocation("SantaCatalina_Grot");
	SetSphereToLocation(n, "grotto2");
	n = FindLocation("SantaCatalina_PearlCave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("Cuba_Grot");
	SetSphereToLocation(n, "grotto2");
	n = FindLocation("Hispaniola_Cave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("Marigo_Cave");
	SetSphereToLocation(n, "cavernMedium2");
	n = FindLocation("Tenotchitlan_Cave");
	SetSphereToLocation(n, "cavernMedium1");
	n = FindLocation("Terks_Grot");
	SetSphereToLocation(n, "grotto1");
	n = FindLocation("Tortuga_Cave");
	SetSphereToLocation(n, "DungeonVault1");
	n = FindLocation("Tortuga_Grot");
	SetSphereToLocation(n, "grotto2");
	n = FindLocation("Trinidad_Grot");
	SetSphereToLocation(n, "grotto2");
}
// EvgAnat - дикие Карибы, возвращение сфер в конкретную локацию
void SetSphereToLocation(int n, string sLoc)
{
	string sRays = sLoc+"_rays";
	string sLocators = sLoc+"_locatorsdk";
	if(n == FindLocation("SantaCatalina_PearlCave"))
	{
		sLocators = "CavernMedium1_lSCPearlsdk";
		sRays = "cavernMedium1_rays";
	}
	Locations[n].models.always.locators = sLocators;
	Locations[n].locators_radius.item.duhi1 = 1.3;
	Locations[n].models.always.rays = sRays;
	Locations[n].models.always.rays.level = 49;
	Locations[n].models.always.rays.tech = "LocVisRays";
	Locations[n].models.always.rays.uvslide.u1 = -0.03;
	Locations[n].models.always.rays.uvslide.v1 = -0.03;
}

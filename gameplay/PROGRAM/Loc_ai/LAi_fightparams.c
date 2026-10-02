/*

	Возможные типы атаки attackType:
	"fast" быстрая атака
	"force" обычная
	"round" круговая
	"break" пробивающая блок
	"feint" атака после финта


*/

//--------------------------------------------------------------------------------
//Blade parameters
//--------------------------------------------------------------------------------

//Расчитать повреждение для персонажа
float LAi_CalcDamageForBlade(aref attack, aref enemy, string attackType, bool isBlocked)
{
	//Расчитываем повреждение от сабли
	float min = 10.0;
	float max = 10.0;
	
	if(CheckAttribute(attack, "chr_ai.dmgbldmin"))
	{
		min = stf(attack.chr_ai.dmgbldmin);
	}
	
	if(CheckAttribute(attack, "chr_ai.dmgbldmax"))
	{
		max = stf(attack.chr_ai.dmgbldmax);
	}

	// evganat - пасха
	if(CheckAttribute(attack, "easter.damage"))
	{
		string sItem = GetCharacterEquipByGroup(attack, BLADE_ITEM_TYPE);
		if(CheckAttribute(attack, "easter.damage."+sItem))
		{
			switch(attack.easter.damage.(sItem))
			{
				case "1":	min += (min+max)*0.125;		max += (min+max)*0.125;		break;
				case "2":	min += (min+max)*0.25;		max += (min+max)*0.25;		break;
			}
		}
	}
	
	float bladeDmg = min + (max - min)*frandSmall(LAi_GetCharacterFightLevel(attack));
	//Коэфициент в зависимости от скилов
	float aSkill = LAi_GetCharacterFightLevel(attack);
	float eSkill = LAi_GetCharacterFightLevel(enemy);
	
    if(aSkill < eSkill)
	{
		bladeDmg = bladeDmg * (1.0 + 0.7 * (aSkill - eSkill));
	}
	
	// Warship 27.08.09 Для сильных противников
	// Если долбить совсем сильных (хардкорные абордажи), то шансов взять шип будет меньше
	if(sti(enemy.Rank) > 50)
	{
		bladeDmg = bladeDmg * 45 / sti(enemy.Rank);
	}
	
	if(CheckAttribute(loadedLocation, "CabinType") && sti(enemy.index) == GetMainCharacterIndex())
	{
		bladeDmg = bladeDmg * (1.0 + stf(attack.rank)/100);
	}
	
	//Коэфициент в зависимости от удара
	float kAttackDmg = 1.0;
	
	// TO_DO оптимизация на ветку параметров
	switch(attackType)
	{
		case "fast":
			if(isBlocked)
			{
				kAttackDmg = 0.0;
			}else{
				kAttackDmg = 0.7;
			}
			break;
		case "force":
			if(isBlocked)
			{
				kAttackDmg = 0.0;
			}else{
				kAttackDmg = 1.0;
			}
			break;
		case "round":
			if(isBlocked)
			{
				kAttackDmg = 0.0;
			}else{
				kAttackDmg = 0.6;
			}
			if(CheckCharacterPerk(attack, "BladeDancer"))
			{
				kAttackDmg = kAttackDmg * 1.3;
			}
			break;
		case "break":
			if(isBlocked)
			{
				kAttackDmg = 1.0;
			}else{
				kAttackDmg = 3.0;
			}
		break;
		
		case "feintc":  // фикс после изучения ядра //Атакующие продолжение финта
			if(isBlocked)
			{
				kAttackDmg = 0.0;
			}else{
				kAttackDmg = 0.8;
			}
		break;
		
		case "feint":
			if(isBlocked)
			{
				kAttackDmg = 0.0;
			}else{
				kAttackDmg = 0.5;
			}
		break;
	}
	
	if(kAttackDmg > 0)  // оптимизация boal
	{
		//Результирующий демедж
		float dmg = bladeDmg * kAttackDmg;
		
		if(CheckCharacterPerk(attack, "HardHitter"))  
		{
			if(CheckAttribute(enemy, "chr_ai.energy"))
			{
				enemy.chr_ai.energy = (stf(enemy.chr_ai.energy) * 0.9); //fix
			}
		}
		
		if(MOD_SKILL_ENEMY_RATE < 5 && sti(enemy.index) == GetMainCharacterIndex())	
		{
			dmg = dmg * (4.0 + MOD_SKILL_ENEMY_RATE) / 10.0;
		}
		
		if (attack.model.animation == "mushketer" && IsMainCharacter(attack))
		{
			dmg = dmg / 3.0;
		}
		
		return dmg;
	}
	
	return 0.0;
}

//Расчитать полученный опыт при ударе саблей
float LAi_CalcExperienceForBlade(aref attack, aref enemy, string attackType, bool isBlocked, float dmg)
{
	//Вычисляем полученый опыт
	float ra = 1.0;
	float re = 1.0;
	if(CheckAttribute(attack, "rank"))
	{
		ra = stf(attack.rank);
	}
	if(CheckAttribute(enemy, "rank"))
	{
		re = stf(enemy.rank);
	}
	if(ra < 1.0) ra = 1.0;
	if(re < 1.0) re = 1.0;
	dmg = dmg*((1.0 + re*0.5)/(1.0 + ra*0.5));

	switch(attackType)
	{
		case "break":
			if(isBlocked)
			{
				dmg = dmg*1.1;
			}else{
				dmg = dmg*1.2;
			}
		break;
		case "feint":
			dmg = dmg*1.5;
		break;
		case "feintc":
			dmg = dmg*1.5;
		break;
	}
	if (stf(enemy.chr_ai.hp) < dmg)
	{
       dmg = stf(enemy.chr_ai.hp);
	}
	return dmg;
}

//Энергия, необходимая для запуска действия
float LAi_CalcUseEnergyForBlade(aref character, string actionType)
{
	float energy = 0.0;
	
	switch(actionType)
	{
		case "fast":
			energy = 10.0;
		break;
		case "force":
			energy = 7.0;
		break;
		case "round":
			energy = 14.0;
		break;
		case "break":
			energy = 25.0;
		break;
		//case "feint":
		//	energy = 7.0;
		//break;
		//case "parry": 
		//	energy = 20.0;
		//break;
		case "hit_parry":  // boal fix эту энергию тратит не атакующий, а атакуемый в анимации fgt_hit_parry
			energy = 20.0;
		break;
		case "feintc":
			energy = 7.0; // расход при успехе финта
		break;
	}
	
	if(energy > 0)  // оптимизация
	{
		float fSkill = LAi_GetCharacterFightLevel(character);  // stf(character.skill.fencing) - не так это далеют!!
		fSkill = (1.0 - (0.3 * fSkill));
		energy = energy * fSkill * LAi_GetBladeEnergyType(character);  // энергоемкость от веса
	}
	
	return energy;
}

float Lai_UpdateEnergyPerDltTime(aref chr, float curEnergy, float dltTime)
{
	float fMultiplier = 1.6666667;

	if(CheckCharacterPerk(chr, "Energaiser")) // скрытый перк боссов и ГГ
	{
		fMultiplier = fMultiplier * 1.5;
	}
	if(CheckCharacterPerk(chr, "Tireless")) 
	{
		fMultiplier = fMultiplier * 1.15;
	}
    // честно все всем
	/*
	if(chr.id == pchar.id || chr.chr_ai.group == LAI_GROUP_PLAYER)
	{
		fMultiplier = fMultiplier * (1.0 + (0.025 * MOD_SKILL_ENEMY_RATE));
	}
	*/
	float fEnergy;
	fEnergy = curEnergy + dltTime * fMultiplier; 

	return fEnergy;
}


//--------------------------------------------------------------------------------
//Gun parameters
//--------------------------------------------------------------------------------

//Расчитаем вероятность попадания
float LAi_GunCalcProbability(aref attack, float kDist)
{
	//Если близко, то попадём точно
	if(kDist >= 0.9) return 1.0;
	//Расчитаем вероятность на конце отрезка
	float pmin = 0.3;
 	if(CheckAttribute(attack, "chr_ai.accuracy")) // boal это меткость самого пистолета, а не скил!
	{
		pmin = stf(attack.chr_ai.accuracy);
	}
 	//Применим разброс от скила
	// boal -->
	float aSkill = LAi_GetCharacterGunLevel(attack);
	// boal <--

	pmin = pmin + 0.3*aSkill;

	//Вероятность попадания в текущей позиции
	float p = pmin + (1.0 - pmin)*(kDist/0.9);
 	//Учесть абилити
	if(IsCharacterPerkOn(attack, "GunProfessional"))
	{
		p = p + 0.25;
	}else{
		if(IsCharacterPerkOn(attack, "Gunman"))
		{
			p = p + 0.1;
		}
	}
	// путь будет больше 1 - тогда 100% попал
	return p;
}

//Получить повреждение от пистолета
float LAi_GunCalcDamage(aref attack, aref enemy)
{
	//Расчитываем повреждение
	float min = 10.0;
	float max = 10.0;
	if(CheckAttribute(attack, "chr_ai.dmggunmin"))
	{
		min = stf(attack.chr_ai.dmggunmin);
	}
	if(CheckAttribute(attack, "chr_ai.dmggunmax"))
	{
		max = stf(attack.chr_ai.dmggunmax);
	}
	//Учитываем скилы
	float aSkill = LAi_GetCharacterGunLevel(attack);
	float eSkill = LAi_GetCharacterLuckLevel(enemy); // good luck
	
	float dmg = min + (max - min)*frandSmall(aSkill);
	//Модифицировать повреждение от pistol с учетом скилов
    if(aSkill < eSkill)
	{
		dmg = dmg * (1.0 + 0.7 * (aSkill - eSkill));
	}
	// упрощение игры новичкам 
	/*if (MOD_SKILL_ENEMY_RATE == 1 && CheckAttribute(enemy, "chr_ai.group"))	
	{
		if (enemy.chr_ai.group == LAI_GROUP_PLAYER) 
		{
			dmg = dmg * 0.5;
		}
	}   */
	if (MOD_SKILL_ENEMY_RATE < 5 && sti(enemy.index) == GetMainCharacterIndex())	
	{
		dmg = dmg * (4.0 + MOD_SKILL_ENEMY_RATE) / 10.0;
	}
	return dmg;
}

// Один выстрел не может снять главному герою больше 49% максимального здоровья.
float LAi_LimitMainCharacterFirearmDamage(aref enemy, float damage)
{
	if(sti(enemy.index) != GetMainCharacterIndex()) return damage;
	float maxDamage = MakeFloat(MakeInt(LAi_GetCharacterMaxHP(enemy) * 0.49));
	if(damage > maxDamage) return maxDamage;
	return damage;
}

//Расчитать полученный опыт при попадании из пистолета
float LAi_GunCalcExperience(aref attack, aref enemy, float dmg)
{
    //Вычисляем полученый опыт
	float ra = 1.0;
	float re = 1.0;
	if(CheckAttribute(attack, "rank"))
	{
		ra = stf(attack.rank);
	}
	if(CheckAttribute(enemy, "rank"))
	{
		re = stf(enemy.rank);
	}
	if(ra < 1.0) ra = 1.0;
	if(re < 1.0) re = 1.0;
	dmg = dmg*((1.0 + re*0.5)/(1.0 + ra*0.5));
	if (stf(enemy.chr_ai.hp) < dmg)
	{
       dmg = stf(enemy.chr_ai.hp);
	}
    return dmg;
}

//Расчитаем текущую скорость перезарядки пистолета
float LAi_GunReloadSpeed(aref chr)
{
	//Получим текущее состояние скорости зарядки
	float charge_dlt = LAI_DEFAULT_DLTCHRG;
	if(CheckAttribute(chr, "chr_ai.charge_dlt"))
	{
		charge_dlt = stf(chr.chr_ai.charge_dlt);
	}
	//Модифицируем скилом
	// boal -->
	//float skill = LAi_GetCharacterFightLevel(chr);
	float skill = LAi_GetCharacterGunLevel(chr);
	// boal <--

	charge_dlt = charge_dlt*(1.0 + 0.3*skill);//boal
	//УчтЈм абилити
	if(IsCharacterPerkOn(chr, "GunProfessional"))
	{
		charge_dlt = charge_dlt*1.25;
	}else{
		if(IsCharacterPerkOn(chr, "Gunman"))
		{
			charge_dlt = charge_dlt*1.1;
		}
	}
	return charge_dlt;
}

//--------------------------------------------------------------------------------
//All
//--------------------------------------------------------------------------------

/*float LAi_CalcDeadExp(aref attack, aref enemy)
{
	//Вычисляем полученый опыт
	float ra = 1.0;
	float re = 1.0;
	if(CheckAttribute(attack, "rank"))
	{
		ra = stf(attack.rank);
	}
	if(CheckAttribute(enemy, "rank"))
	{
		re = stf(enemy.rank);
	}
	if(ra < 1.0) ra = 1.0;
	if(re < 1.0) re = 1.0;
	float dmg = (0.5 + 4.0*LAi_GetCharacterFightLevel(enemy))*LAi_GetCharacterMaxHP(enemy);
	dmg = dmg*((1.0 + re*0.5)/(1.0 + ra*0.5));
	return dmg*0.5;
} */

//--------------------------------------------------------------------------------
//Calculate total
//--------------------------------------------------------------------------------

//Начисление повреждений при атаке мечём
void LAi_ApplyCharacterAttackDamage(aref attack, aref enemy, string attackType, bool isBlocked)
{
	//Если неубиваемый, то нетрогаем его
	if(CheckAttribute(enemy, "chr_ai.immortal"))
	{
		if(sti(enemy.chr_ai.immortal) != 0)
		{
			return;
		}
	}
	//Вычисляем повреждение
	float dmg = LAi_CalcDamageForBlade(attack, enemy, attackType, isBlocked);
	float critical = 0.0;
	
	// ГПК 1.2.3
	if(IsCharacterPerkOn(attack, "SwordplayProfessional"))
	{
		if(rand(100) <= 15)
		{
			critical = 1.0;//LAi_GetCharacterMaxHP(enemy)*0.30;
		}
	}else{
		if(IsCharacterPerkOn(attack, "CriticalHit"))
		{
			if(rand(100) <= 5)
			{
				critical = 1.0;//LAi_GetCharacterMaxHP(enemy)*0.20;
			}
		}
	}

	float kDmg = 1.0;
	if(IsCharacterPerkOn(attack, "Rush"))
	{
		kDmg = 3.0;
	}
	float kDmgRush = 1.0;
	if(IsCharacterPerkOn(enemy, "Rush"))
	{
		kDmgRush = 0.5;
	}
	dmg = dmg*kDmg*kDmgRush;
	//Аттака своей группы
	bool noExp = false;
	if(CheckAttribute(attack, "chr_ai.group") && CheckAttribute(enemy, "chr_ai.group"))
	{
		if(attack.chr_ai.group == enemy.chr_ai.group)
		{
			dmg = 0.0;
			critical = 0.0;
			noExp = true;
		}
	}
	if(isBlocked || CheckAttribute(enemy, "cirassId"))// если кираса или блок, то критикал невозможен
	{
		critical = 0;
	}
	if(critical > 0.0)
	{
        AddCharacterExpToSkill(attack, SKILL_FORTUNE, 5);
		if(sti(attack.index) == GetMainCharacterIndex())
		{
			Log_SetStringToLog(XI_ConvertString("Critical Hit"));
			//Log_TestInfo(" " + critical + " хитов");
		}
	}
	kDmg = 1.0;
	if(IsCharacterPerkOn(enemy, "BasicDefense")) kDmg = 0.9;
	if(IsCharacterPerkOn(enemy, "AdvancedDefense")) kDmg = 0.8;
	if(IsCharacterPerkOn(enemy, "SwordplayProfessional")) kDmg = 0.7;

	// ГПК 1.2.3
	dmg = dmg*kDmg;
	dmg = dmg *(1 + critical);//dmg + critical;
	if(CheckAttribute(enemy, "cirassId"))
	{
		dmg = dmg * (1.0 - stf(Items[sti(enemy.cirassId)].CirassLevel));
	}
	if(dmg > 0.0)
	{
		//Наносим повреждение
		LAi_ApplyCharacterDamage(enemy, MakeInt(dmg + 0.5));
		// Preserve lethal attribution before the synchronous death event.
		if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);
		//проверим на отравление
		MakePoisonAttackCheckSex(enemy, attack);
	}
	//Есть ли оружие у цели
	bool isSetBalde = (CheckAttribute(enemy, "equip.blade") == true);//(SendMessage(enemy, "ls", MSG_CHARACTER_EX_MSG, "IsSetBalde") != 0);
	//Начисляем опыта
	float exp = LAi_CalcExperienceForBlade(attack, enemy, attackType, isBlocked, dmg);
	/*if(LAi_grp_alarmactive == false)
	{
		if(CheckAttribute(pchar, "sneak.success"))
		{
			if(sti(pchar.sneak.success) == 1)
			{
				pchar.sneak.success = 0;
			}
		}
	} */
	if(LAi_IsDead(enemy))
	{
		//Начислим за убийство
		//exp = exp + LAi_CalcDeadExp(attack, enemy);
		//exp = LAi_GetCharacterMaxHP(enemy) * 10;
		//noExp = false;
		//DoCharacterKilledStatistics(sti(attack.index), sti(enemy.index));
		/*if(!isSetBalde)
		{
			//LAi_ChangeReputation(attack, -3);
		}*/
		// boal  check skill -->
		float ra = 1.0;
	    float re = 1.0;
	    if(CheckAttribute(attack, "rank"))
	    {
	       ra = stf(attack.rank);
	    }
	    if(CheckAttribute(enemy, "rank"))
	    {
	       re = stf(enemy.rank);
	    }
        AddCharacterExpToSkill(attack, LAi_GetBladeFencingType(attack), makefloat(10.0 + ((1 + re) / (1+ra))*6.5));
        AddCharacterExpToSkill(attack, SKILL_DEFENCE, 1);
        AddCharacterExpToSkill(attack, SKILL_FORTUNE, 1);
        AddCharacterExpToSkill(attack, SKILL_LEADERSHIP, 1);
        // boal <--
        // boal statistic info 17.12.2003 -->
        Statistic_KillChar(attack, enemy, "_s");
        // boal statistic info 17.12.2003 <--
        LAi_SetResultOfDeath(attack, enemy, isSetBalde);
	}
	if(!isSetBalde)
	{
		exp = 0.0;
	}
	
	if (!noExp)
    {
        //AddCharacterExp(attack, MakeInt(exp*0.5 + 0.5));
        AddCharacterExpToSkill(attack, LAi_GetBladeFencingType(attack), Makefloat(exp*0.2));
    }
	
}
//boal 19.09.05 -->
void LAi_SetResultOfDeath(ref attack, ref enemy, bool isSetBalde)
{
	bool playerOwnedKiller = sti(attack.index) == GetMainCharacterIndex() || IsOfficer(attack) || IsCompanion(attack);
	if (!playerOwnedKiller) return;
	enemy.Killer.Index = attack.index;
	if (Crime_HasPlayerIntent(enemy))
	{
		int crimeSeverity = 2;
		if (CrewDebt_IsNamedCreditor(enemy)) crimeSeverity = 3;
		CrimeLand_RecordDeath(attack, enemy, crimeSeverity);
		return;
	}
	// Preserve legacy quest/enemy behavior only for the player actor.
	if (sti(attack.index) != GetMainCharacterIndex()) return;
	if (GetRelation2BaseNation(sti(enemy.nation)) == RELATION_ENEMY)
	{
		if (!isSetBalde)
		{
			LAi_ChangeReputation(attack, -1);
			if (rand(1) && CheckAttribute(enemy, "City"))
				ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 1);
		}
	}
	else
	{
		if (CheckAttribute(enemy, "City"))
			ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 2);
	}
	if (CheckAttribute(enemy, "City"))
	{
		if (GetSummonSkillFromName(attack, SKILL_SNEAK) < rand(140))
			SetNationRelation2MainCharacter(sti(enemy.nation), RELATION_ENEMY);
	}
}
// boal <--

//Начисление повреждений при попадании
void LAi_ApplyCharacterFireDamage(aref attack, aref enemy, float kDist)
{
	Lai_CharacterChangeEnergy(attack, -4); // жрем энергию за выстрел boal 20/06/06
	//Если неубиваемый, то нетрогаем его
	if(CheckAttribute(enemy, "chr_ai.immortal"))
	{
		if(sti(enemy.chr_ai.immortal) != 0)
		{
			return;
		}
	}
	//Вероятность поподания
	float p = LAi_GunCalcProbability(attack, kDist);
	//Если промахнулись, то выйдем
	if(rand(10000) > p*10000) return;	  
	// boal брон работает всегда, а не токо в блоке 23.05.2004 -->
	if(CheckAttribute(enemy, "cirassId"))
	{
        if(rand(1000) < stf(Items[sti(enemy.cirassId)].CirassLevel)*500) return;
	}
	// boal 23.05.2004 <--
	//Начисляем повреждение
	float damage = LAi_GunCalcDamage(attack, enemy);
	damage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);

	//Аттака своей группы
	bool noExp = false;
	if(CheckAttribute(attack, "chr_ai.group"))
	{
		if(CheckAttribute(enemy, "chr_ai.group"))
		{
			if(attack.chr_ai.group == enemy.chr_ai.group)
			{
				damage = 0.0;
				noExp = true;
			}
		}
	}
	if(damage > 0.0)
	{
		LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));	
		// Preserve lethal attribution before the synchronous death event.
		if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);
	}
	//Есть ли оружие у цели
	bool isSetBalde = (CheckAttribute(enemy, "equip.blade") == true);//(SendMessage(enemy, "ls", MSG_CHARACTER_EX_MSG, "IsSetBalde") != 0);
	//Начисляем опыт
	float exp = LAi_GunCalcExperience(attack, enemy, damage);
	/*if(LAi_grp_alarmactive == false)
	{
		if(CheckAttribute(pchar, "sneak.success"))
		{
			if(sti(pchar.sneak.success) == 1)
			{
				pchar.sneak.success = 0;
			}
		}
	}  */
	if(LAi_IsDead(enemy))
	{
		//Начислим за убийство
		//exp = exp + LAi_CalcDeadExp(attack, enemy);
		//exp = LAi_GetCharacterMaxHP(enemy) * 10;
		//noExp = false;
		//if(!isSetBalde)
		//{
			//LAi_ChangeReputation(attack, -3);
		//	exp = 0.0;
		//}
		//DoCharacterKilledStatistics(sti(attack.index), sti(enemy.index));
		// boal skill -->
		float ra = 1.0;
	    float re = 1.0;
	    if(CheckAttribute(attack, "rank"))
	    {
	       ra = stf(attack.rank);
	    }
	    if(CheckAttribute(enemy, "rank"))
	    {
	       re = stf(enemy.rank);
	    }
        AddCharacterExpToSkill(attack, SKILL_PISTOL, makefloat(15.0 + ((1 + re) / (1+ra))*12.0));
        AddCharacterExpToSkill(attack, SKILL_DEFENCE, 1);
        AddCharacterExpToSkill(attack, SKILL_FORTUNE, 2);
        AddCharacterExpToSkill(attack, SKILL_LEADERSHIP, 1);
		// boal skill <--
		// boal statistic info 17.12.2003 -->
        Statistic_KillChar(attack, enemy, "_g");
        // boal statistic info 17.12.2003 <--
        
        //Начислим за убийство
		/*exp = exp + */
        //LAi_CalcDeadExp(attack, enemy); // начисляем только за удар и смерть
  		LAi_SetResultOfDeath(attack, enemy, isSetBalde);
	}
	if(!isSetBalde)
	{
		//LAi_ChangeReputation(attack, -1);
		exp = 0.0;
	}
	if(!noExp)
    {
        AddCharacterExpToSkill(attack, SKILL_PISTOL, Makefloat(exp*0.85));
    }
}

//--------------------------------------------------------------------------------
//Параметры NPC
//--------------------------------------------------------------------------------

float npc_return_tmp;
bool npc_return_tmpb;

//Атаки
//Скорость нарастания вероятности атаки в секунду  p > 0
#event_handler("NPC_Event_GetAttackActive","LAi_NPC_GetAttackActive");
float LAi_NPC_GetAttackActive()
{
	aref chr = GetEventData();
	float level = LAi_GetCharacterFightLevel(chr);
	npc_return_tmp = 0.3 + level*0.7;
	npc_return_tmp = npc_return_tmp + 0.1;
	return npc_return_tmp;
}

//Вес выбора удара "fast", 0 - никогда не выбирать
#event_handler("NPC_Event_GetAttackWeightFast","LAi_NPC_GetAttackWeightFast");
float LAi_NPC_GetAttackWeightFast()
{
	aref chr = GetEventData();
	npc_return_tmp = 20.0;
	npc_return_tmp = npc_return_tmp * (0.8 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Вес выбора удара "force", 0 - никогда не выбирать
#event_handler("NPC_Event_GetAttackWeightForce","LAi_NPC_GetAttackWeightForce");
float LAi_NPC_GetAttackWeightForce()
{
	aref chr = GetEventData();
	npc_return_tmp = 50.0;
	npc_return_tmp = npc_return_tmp * (0.8 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Вес выбора удара "round", 0 - никогда не выбирать, если врагов <3 то удар не выбирается
#event_handler("NPC_Event_GetAttackWeightRound","LAi_NPC_GetAttackWeightRound");
float LAi_NPC_GetAttackWeightRound()
{
	aref chr = GetEventData();
	npc_return_tmp = 20.0;
	npc_return_tmp = npc_return_tmp * (0.8 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Вес выбора удара "break", 0 - никогда не выбирать
#event_handler("NPC_Event_GetAttackWeightBreak","LAi_NPC_GetAttackWeightBreak");
float LAi_NPC_GetAttackWeightBreak()
{
	aref chr = GetEventData();
	npc_return_tmp = 20.0;
	npc_return_tmp = npc_return_tmp * (0.6 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Вес выбора удара "feint", 0 - никогда не выбирать
#event_handler("NPC_Event_GetAttackWeightFeint","LAi_NPC_GetAttackWeightFeint");
float LAi_NPC_GetAttackWeightFeint()
{
	aref chr = GetEventData();
	npc_return_tmp = 10.0; //30 boal fix
	npc_return_tmp = npc_return_tmp * (0.6 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Прараметры защиты
//Вероятность желания защитится - кубик с такой вероятностью кидается 2 раза в секунду
#event_handler("NPC_Event_GetDefenceActive","LAi_NPC_GetAttackDefence");
float LAi_NPC_GetAttackDefence()
{
	aref chr = GetEventData();
	float level = LAi_GetCharacterFightLevel(chr);
	//npc_return_tmp = 0.05 + level*0.4;
	npc_return_tmp = 0.3 + level*0.35;
	/*if (!iArcadeFencingAI)	// to_do
	{
		npc_return_tmp = npc_return_tmp + 0.2;
	}*/
	return npc_return_tmp;
}

// boal 20.01.08 коммент - забавно, что спустя два года, понал как и что с вероятностями. Они все приводятся к 0-1 от веса общей суммы, то есть фактически умножение на сложность или цифры распределяют сумму по другим акшенам, а не усиливают этот
// Экшены идут парами - все атаки и защита (блок + пари)
//Вес выбора блока, 0 - никогда не выбирать
#event_handler("NPC_Event_GetDefenceWeightBlock","LAi_NPC_GetDefenceWeightBlock");
float LAi_NPC_GetDefenceWeightBlock()
{
	aref chr = GetEventData();
	npc_return_tmp = 80.0;
	npc_return_tmp = npc_return_tmp * (0.5 + (0.05 * MOD_SKILL_ENEMY_RATE));   // boal
	return npc_return_tmp;
}

//Вес выбора  паррирования, 0 - никогда не выбирать
//кубик с такой вероятностью кидается 2 раза в секунду
#event_handler("NPC_Event_GetDefenceWeightParry","LAi_NPC_GetDefenceWeightParry");
float LAi_NPC_GetDefenceWeightParry()
{
	aref chr = GetEventData();
	npc_return_tmp = 20.0; // 40 boal
	npc_return_tmp = npc_return_tmp * (0.6 + (0.1 * MOD_SKILL_ENEMY_RATE));
	return npc_return_tmp;
}

//Разрешён ли отскок
#event_handler("NPC_Event_EnableRecoil","LAi_NPC_EnableRecoil");
bool LAi_NPC_EnableRecoil()
{
	aref chr = GetEventData();
	npc_return_tmpb = true;
	return npc_return_tmpb;
}


//Параметры стрельбы
//Вероятность желания выстрелить - кубик с такой вероятностью кидается 2 раза в секунду
#event_handler("NPC_Event_GetFireActive","LAi_NPC_GetFireActive");
float LAi_NPC_GetFireActive()
{
	aref chr = GetEventData();
	float level = LAi_GetCharacterGunLevel(chr);
	npc_return_tmp = 0.001 + level*0.06;
	// boal наши офицеры пулят из всех стволов -->
	if (chr.chr_ai.group == LAI_GROUP_PLAYER)
	{
        npc_return_tmp = 0.38 + npc_return_tmp;
	}
	else
	{
	// boal наши офицеры пулят из всех стволов <--
		if (CheckAttribute(chr, "SuperShooter"))
		{
			npc_return_tmp = npc_return_tmp + 0.4 * MOD_SKILL_ENEMY_RATE / 10.0;
		}
		else
		{
			npc_return_tmp = npc_return_tmp + 0.1 * MOD_SKILL_ENEMY_RATE / 10.0;
		}
	}
	//if (npc_return_tmp > 0.5) npc_return_tmp = 0.5;
	
	return npc_return_tmp;
}

//Разрешён ли выстрел
#event_handler("NPC_Event_EnableFire","LAi_NPC_EnableFire");
bool LAi_NPC_EnableFire()
{
	aref chr = GetEventData();
	return   true;
	/*float level = LAi_GetCharacterGunLevel(chr);
	npc_return_tmpb = false;
	if(!iArcadeFencingAI)
	{
		level = level + 0.02;
	}
	else
	{
		level = level - 0.01;
	}
	if(level > 0.1) npc_return_tmpb = true;
	return npc_return_tmpb;    */
}

//Разрещён ли временный перевыбор цели среди ближних - опрашивается постоянно
#event_handler("NPC_Event_AdaptiveTargetSelect","LAi_NPC_AdaptiveTargetSelect");
bool LAi_NPC_AdaptiveTargetSelect()
{
	aref chr = GetEventData();
	if(chr.chr_ai.type == LAI_TYPE_ACTOR)
	{
		npc_return_tmpb = false;
	}else{
		npc_return_tmpb = true;
	}
	return npc_return_tmpb;
}


//--------------------------------------------------------------------------------
//Work
//--------------------------------------------------------------------------------

#event_handler("Location_CharacterSGFire","LAi_Location_CharacterSGFire");
void LAi_Location_CharacterSGFire()
{
	aref attack = GetEventData();
	aref enemy = GetEventData();
	float kDmg = GetEventData();
	if(LAi_IsDead(enemy)) return;
	//Реакция груп на атаку
	LAi_group_Attack(attack, enemy);
	//AddCharacterExp(attack, 100*kDmg);
	//Наносим повреждение
	float damage = MakeFloat((5 + rand(5))*kDmg);
	damage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);
	LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));
	// Preserve SG-fire attribution and run the same crime/death consequence path.
	if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
	//Проверим на смерть
	LAi_CheckKillCharacter(enemy);
	if (LAi_IsDead(enemy)) LAi_SetResultOfDeath(attack, enemy, CheckAttribute(enemy, "equip.blade"));
}

#event_handler("ChrAttackAction", "LAi_ChrAttackAction");
bool LAi_ChrAttackAction()
{
	aref attack = GetEventData();
	string attackType = GetEventData();
	float curEnergy = Lai_CharacterGetEnergy(attack);
	float needEnergy = LAi_CalcUseEnergyForBlade(attack, attackType);
	if(curEnergy >= needEnergy)
	{
		npc_return_tmpb = true;
	}
	else
	{
		npc_return_tmpb = false;
	}
	return npc_return_tmpb;
}

#event_handler("ChrFgtActApply", "LAi_ChrFightActionApply");
void LAi_ChrFightActionApply()
{
	aref attack = GetEventData();
	string attackType = GetEventData();
	float needEnergy = LAi_CalcUseEnergyForBlade(attack, attackType);
	Lai_CharacterChangeEnergy(attack, -needEnergy);
}

//Получить относительную затрачиваемую энергию
#event_handler("NPC_Event_GetActionEnergy","LAi_NPC_GetActionEnergy");
float LAi_NPC_GetActionEnergy()
{
	aref chr = GetEventData();
	string act = GetEventData();
	npc_return_tmp = LAi_CalcUseEnergyForBlade(chr, act) / LAi_GetCharacterMaxEnergy(chr);  // boal
	return npc_return_tmp;
}

//Необходимо вернуть максимально быстро нормализованое значение жизни
#event_handler("NpcEvtHP", "LAi_NPC_EvtGetHP");
float LAi_NPC_EvtGetHP()
{
	aref chr = GetEventData();
	npc_return_tmp = LAi_GetCharacterRelHP(chr);
	return npc_return_tmp;
}


//Необходимо вернуть максимально быстро нормализованое значение энергии
#event_handler("NpcEvtEny", "LAi_NPC_EvtGetEny");
float LAi_NPC_EvtGetEny()
{
	aref chr = GetEventData();
	npc_return_tmp = LAi_GetCharacterRelEnergy(chr);
	return npc_return_tmp;
}

//Вероятность стана после удара, опрашивается единожды при загрузке персонажа
#event_handler("NPC_Event_StunChance","LAi_NPC_StunChance");
int LAi_NPC_StunChance()
{
	aref chr = GetEventData();
	int npc_return_tmpi = 100;
	if(CheckCharacterPerk(chr, "SwordplayProfessional"))
	{
		npc_return_tmpi = 40;
	}
	else 
	{
		if(CheckCharacterPerk(chr, "AdvancedDefense"))
		{
			npc_return_tmpi = 70;
		}
		else
		{	
			if(CheckCharacterPerk(chr, "BasicDefense"))
			{
				npc_return_tmpi = 80;
			}
			else
			{
				npc_return_tmpi = 90;
			}
		}	
	}		
	return npc_return_tmpi;
}

// EvgAnat - требование энергии для отскока -->
bool bIsRecoilEnableWithoutEnergy = true; // можно ли выполнять отскоки при недостатке энергии (это не влияет на расход энергии)

#event_handler("ChrCheckEnergy", "LAi_Chr_CheckEnergy");
bool LAi_Chr_CheckEnergy()
{
	aref chr = GetEventData();
	string action = GetEventData(); // "recoil" - отскок назад, "strafe_l" и "strafe_r" - отскоки влево и вправо
	bool res = false;
	float needEnergy = 0.0;
	switch(action)
	{
		case "recoil":		needEnergy = 2.0;	break;
		case "strafe_l":	needEnergy = 2.0;	break;
		case "strafe_r":	needEnergy = 2.0;	break;
	}
	if(CheckAttribute(chr, "exhausted"))
		DeleteAttribute(chr, "exhausted");
	if(CheckAttribute(chr, "cirassId"))
	{
		needEnergy += stf(Items[sti(chr.cirassId)].Weight)*0.1;
	}
	if (stf(chr.chr_ai.energy) >= needEnergy)
	{	
		res = true;
		Lai_CharacterChangeEnergy(chr, -needEnergy);
	}
	else
	{
		if (bIsRecoilEnableWithoutEnergy)
		{
			res = true;
			Lai_CharacterChangeEnergy(chr, -needEnergy);
			chr.exhausted = true;
		}
	}
	return res;
}
// EvgAnat - требование энергии для отскока <--

// EvgAnat - включено ли уклонение от выстрела для нпс -->
#event_handler("NPC_IsDodgeEnabled", "LAi_Chr_IsDodgeEnabled");
bool LAi_Chr_IsDodgeEnabled()
{
	return true;
}
// EvgAnat - включено ли уклонение от выстрела для нпс <--

// EvgAnat - уклонение от выстрела -->
#event_handler("Check_ChrHitFire", "LAi_Chr_CheckHitFire");
int LAi_Chr_CheckHitFire() // 0 - не попал, 1 - попал
{
	aref shooter = GetEventData(); // стрелок
	aref target = GetEventData(); // цель
	bool isRecoil = GetEventData(); // находится ли цель в окне уклонения 
	float kDist = GetEventData(); // коэффициент дальности, равный 1-d/25; k(0)=1; k(10)=0.6; k(25)=0
	int res = 1;
	float r = Random();
	float chance = 1.0;
	if (CheckAttribute(target, "exhausted"))
		chance = 0.5;
	if (isRecoil && r <= chance)
	{
		res = 0;
		if (shooter.index == GetMainCharacterIndex())
			Log_SetStringToLog("Мазила!");
	}
	return res;
}
// EvgAnat - уклонение от выстрела <--

// EvgAnat - уклонение от атаки -->
#event_handler("Check_ChrHitAttack", "LAi_Chr_CheckHitAttack");
bool LAi_Chr_CheckHitAttack() // попала ли атака
{
	aref attack = GetEventData();
	aref enemy = GetEventData();
	bool isRecoil = GetEventData(); // находится ли цель в окне уклонения
	bool res = true;
	float r = Random();
	float chance = 1.0;
	if (CheckAttribute(enemy, "exhausted"))
		chance = 0.5;
	if (isRecoil && r <= chance)
		res = false;	
	return res;
}
// EvgAnat - уклонение от атаки <--

// EvgAnat - вероятность желания уклониться от выстрела у нпс -->
#event_handler("NPC_IsDodge", "LAi_NPC_IsDodge");
bool LAi_NPC_IsDodge() // true - уклоняется, false - не уклоняется
{
	aref chr = GetEventData();
	float r = Random();
	bool res = false;
	//Log_SetStringToLog("1. " + GetFullName(chr) + " ищет в себе желание уклониться")
	if (CheckAttribute(chr, "canDodge") || chr.chr_ai.type == LAI_TYPE_OFFICER)
	{
		//Log_SetStringToLog("2. " + GetFullName(chr) + " умеет уклоняться");
		if (r <= 0.25)
		{
			//Log_SetStringToLog("3. " + GetFullName(chr) + " желает уклониться");
			res = true;
		}
	}
	return res;
}
// EvgAnat - вероятность желания уклониться от выстрела у нпс <--

// EvgAnat - дальность отскока и стрейфа -->
#event_handler("GetCharacterRecoilDistance", "LAi_GetRecoilDistance");
float LAi_GetRecoilDistance()
{
	aref chr = GetEventData();
	string aType = GetEventData();
	float res = 2.0;
	switch(aType)
	{
		case "recoil":
			res = 3.0; // по умолчанию 2.0
		break;
		case "strafe":
			res = 10.0; // по умолчанию 15.0
		break;
	}
	return res;
}
// EvgAnat - дальность отскока и стрейфа <--

//Rosarak. Для отложенного возврата запрета на обнажение оружия
void FightDisable()
{
	LAi_LocationFightDisable(&Locations[FindLocation(TEV.noFight.loc)], true);
	DeleteAttribute(&TEV, "noFight");
	FightDisable_Forced();
}

//Rosarak. Оружие в ножны без возможности задоджить выстрелом или анимацией
void FightDisable_Forced()
{
	if(SendMessage(pchar, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", false))
		LAi_MethodDelay("FightDisable_Forced", 0.1);
}

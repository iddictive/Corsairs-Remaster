

//Объект, описывающий отнощение одной группы к другой
object LAi_grp_relations;
float LAi_grp_playeralarm = 0;
bool LAi_grp_alarmactive = false;

//------------------------------------------------------------------------------------------
//Инициализация
//------------------------------------------------------------------------------------------

//Инициализация
bool LAi_group_Init()
{
probeGroupBuilds++; probeGroupLive=true; return true;
}

//Освобождение ресурсов
void LAi_group_Release()
{
	DeleteAttribute(&LAi_grp_relations, "quests");
	LAi_group_ClearAllTargets();
	//  boal fix 291105 - это тормоза, тк мусора много, но убирать низя, проблемы с запоминанием обид
	SendMessage(&LAi_grp_relations, "s", "SaveData");
	//Удаляем объект групп
	DeleteClass(&LAi_grp_relations);
}

void LAi_group_SaveInfo()
{
	SendMessage(&LAi_grp_relations, "s", "SaveData");
}

//------------------------------------------------------------------------------------------
//Регистрация новой группы, установка общих параметров
//------------------------------------------------------------------------------------------

//Регистрация новой группы
void LAi_group_Register(string groupName)
{
	SendMessage(&LAi_grp_relations, "ss", "RegistryGroup", groupName);
	LAi_group_SetLookRadius(groupName, LAI_GROUP_DEF_LOOK);
	LAi_group_SetHearRadius(groupName, LAI_GROUP_DEF_HEAR);
	LAi_group_SetSayRadius(groupName, LAI_GROUP_DEF_SAY);
}

void LAi_group_Delete(string groupName)
{
	SendMessage(&LAi_grp_relations, "ss", "ReleaseGroup", groupName);
}


//Установить для группы радиус видимости
void LAi_group_SetLookRadius(string groupName, float radius)
{
	SendMessage(&LAi_grp_relations, "ssf", "SetGroupLook", groupName, radius);
}

//Установить для группы радиус слышимости
void LAi_group_SetHearRadius(string groupName, float radius)
{
	SendMessage(&LAi_grp_relations, "ssf", "SetGroupHear", groupName, radius);
}

//Установить для группы радиус сообщения
void LAi_group_SetSayRadius(string groupName, float radius)
{
	SendMessage(&LAi_grp_relations, "ssf", "SetGroupSay", groupName, radius);
}

//Установить для группы приоритет
void LAi_group_SetPriority(string groupName, int priority)
{
	SendMessage(&LAi_grp_relations, "ssl", "SetGroupPriority", groupName, priority);
}

//Установить уровень тревоги 0..1
void LAi_group_SetAlarm(string group1, string group2, float level)
{
	SendMessage(&LAi_grp_relations, "sssf", "SetAlarm", group1, group2, level);
}

//Установить скорость убывания тревоги ед/сек
void LAi_group_SetAlarmDown(string group1, string group2, float down)
{
	SendMessage(&LAi_grp_relations, "sssf", "SetAlarmDown", group1, group2, down);
}


//------------------------------------------------------------------------------------------
//Перемещение персонажа в группы
//------------------------------------------------------------------------------------------

//Переместить персонажа в новую группу
void LAi_group_MoveCharacter(aref chr, string groupName)
{
	chr.chr_ai.group = groupName;
	SendMessage(&LAi_grp_relations, "sis", "MoveChr", chr, groupName);	
}

//------------------------------------------------------------------------------------------
//Отношения групп
//------------------------------------------------------------------------------------------

//Установить отношения между группами
void LAi_group_SetRelation(string group1, string group2, string curRelation)
{
    curRelation = GreatRecipe_AllianceLandRelation(group1, group2, curRelation);
	if (group1 != group2) // boal fix
	{
		SendMessage(&LAi_grp_relations, "ssss", "SetRelation", group1, group2, curRelation);
	}
}

//Установить отношения между группами при активации тревоги и после
void LAi_group_SetAlarmReaction(string group1, string group2, string actRelation, string relRelation)
{
    if (group1 != group2) // boal fix
    {
		SendMessage(&LAi_grp_relations, "sssss", "SetAlarmReaction", group1, group2, actRelation, relRelation);
	}
}

//Натравить друг на друга 2 группы
void LAi_group_FightGroups(string group1, string group2, bool asignWarriarType)
{
    if (group1 != group2) // boal fix
    {
		LAi_group_FightGroupsEx(group1, group2, asignWarriarType, -1, -1, true, false);
	}
}

//Натравить друг на друга 2 группы, с возможной установкой командиров
void LAi_group_FightGroupsEx(string group1, string group2, bool asignWarriarType, int commander1, int commander2, bool isStay, bool isDialog)
{
    if (group1 == group2) // boal fix
    {
        return;
    }
	//Укажем что обновление надо провести немедленно
	SendMessage(&LAi_grp_relations, "s", "ResetWaveTime");
	//Установим войнов типы персонажем
	if(asignWarriarType)
	{
		for(int i = 0; i < LAi_numloginedcharacters; i++)
		{
			int idx = LAi_loginedcharacters[i];
			if(idx >= 0)
			{
				//Пропустим главного персонажа или офицера
				if(idx == nMainCharacterIndex) continue;
				for(int j = 1; j < 4; j++)
				{
					if(GetOfficersIndex(pchar, j) == idx) break;
				}
				if(j < 4) continue;
				//Проверяем попадание персонажа в группу
				ref chr = &Characters[idx];
				if(CheckAttribute(chr, "chr_ai.group"))
				{
					bool selected = false;
					bool isOne;
					if(chr.chr_ai.group == group1)
					{
						selected = true;
						isOne = true;
					}else{
						if(chr.chr_ai.group == group2)
						{
							selected = true;
							isOne = false;
						}
					}
					if(selected)
					{
						LAi_SetWarriorTypeNoGroup(chr);
						if(CheckAttribute(chr, "walker")) // затесавшиеся офицеры-ходоки с типом citizen
						{
							TEV.walkers = TEV.walkers + chr.id + ",";
							TEV.walkers.count = sti(TEV.walkers.count) + 1;
							RemoveCharacterEquip(chr, GUN_ITEM_TYPE); // чтобы не агрил стражу во время драки с ОЗГ
							if(group1 != LAI_GROUP_PLAYER) LAi_group_SetCheck(group1, "Officers_Citizen_Type");
							else LAi_group_SetCheck(group2, "Officers_Citizen_Type");
						}
						else // default
						{
							LAi_warrior_SetStay(chr, isStay);
							LAi_warrior_DialogEnable(chr, isDialog);
						}
						if(isOne)
						{
							if(commander1 >= 0)
							{
								LAi_warrior_SetCommander(chr, &Characters[commander1]);
							}
							LAi_group_MoveCharacter(chr, group1);
						}else{
							if(commander2 >= 0)
							{
								LAi_warrior_SetCommander(chr, &Characters[commander2]);
							}
							LAi_group_MoveCharacter(chr, group2);
						}
					}
				}
			}
		}
	}
	//Настроим группы
	LAi_group_SetRelation(group1, group2, LAI_GROUP_ENEMY);
	LAi_group_SetHearRadius(group1, 40000000.0);
	LAi_group_SetSayRadius(group1, 40000000.0);
	LAi_group_SetHearRadius(group2, 40000000.0);	
	LAi_group_SetSayRadius(group2, 40000000.0);
}

void LAi_group_NotFightPlayerVSGroup(string group2)
{
	LAi_group_SetHearRadius(LAI_GROUP_PLAYER, LAI_GROUP_PLA_HEAR);
	LAi_group_SetSayRadius(LAI_GROUP_PLAYER, LAI_GROUP_PLA_SAY);
	LAi_group_SetHearRadius(group2, LAI_GROUP_DEF_HEAR);	
	LAi_group_SetSayRadius(group2, LAI_GROUP_DEF_SAY);
}

//------------------------------------------------------------------------------------------
//Цели
//------------------------------------------------------------------------------------------

//Выбрать цель для данного персонажа
int LAi_group_GetTarget(aref chr)
{
	int index = -1;
	if(SendMessage(&LAi_grp_relations, "sie", "GetTrg", chr, &index) == 0)
	{
		index = -1;
	}
	else
	{
	    if (!LAi_group_IsEnemy(chr, &characters[index]))  // boal fix 19.08.06 ищет любых вооружённых НПС,  а нужно врагов
		{
		    index = -1;
		}
	}
	/*        // to_do del
	if(index != -1)
	{
		if(characters[index].id == pchar.id)
		{
			if(LAi_grp_alarmactive != true)
			{
				index = -1;
			}
		}
	}  */

	return index;
}

//Проверить цель
bool LAi_group_ValidateTarget(aref chr, aref trg)
{	
	return (SendMessage(&LAi_grp_relations, "sii", "VldTrg", chr, trg) != 0);
}

//Враг ли данный персонаж
bool LAi_group_IsEnemy(aref chr, aref trg)
{
	return (SendMessage(&LAi_grp_relations, "sii", "IsEnemy", chr, trg) != 0);
}

//------------------------------------------------------------------------------------------
//Реакция на атаку
//------------------------------------------------------------------------------------------

//Реакция групп на атаку attack->hit
bool LAi_group_Attack(aref attack, aref hit)
{
	if(attack.chr_ai.group == hit.chr_ai.group)
	{
		//Своих игнорируем
		return false;
	}
	//Натравим друг на друга
	SendMessage(&LAi_grp_relations, "sss", "Attack", attack.chr_ai.group, hit.chr_ai.group);
	//Добавляем врага
	SendMessage(&LAi_grp_relations, "siif", "AddTarget", hit, attack, 10.0 + rand(10));
	
	// Dolphin -> генератор "Охотник на торговцев"
	/*if(hit.chr_ai.group == "HOTP_HunterTrader_LandGroup" && !CheckAttribute(pchar, "HOTP.GenQuests.HunterTrader.quest.sound"))
	{
		PlaySound("Voice\Russian\EvilPirates01.wav");
		pchar.HOTP.GenQuests.HunterTrader.quest.sound = true;
	}*/

	// Французский сюжет, нападение без диалога на парней в пещере
	if(hit.chr_ai.group == "FL1_s33_group") pchar.MainQuest = "";
	// <-
	
	return true;
}
//eddy.Можно пользоваться этим методом для непостоянного стравливания. Для групп горожан юзать только его!
bool LAi_group_AttackGroup(string attack, string hit)
{
	//Натравим друг на друга
	SendMessage(&LAi_grp_relations, "sss", "Attack", attack, hit);
	
	return true;
}

//Обновить список врагов персонажа
void LAi_group_UpdateTargets(aref chr)
{
	SendMessage(&LAi_grp_relations, "si", "UpdChrTrg", chr);
}

//------------------------------------------------------------------------------------------
//Утилитные
//------------------------------------------------------------------------------------------

//Удалить все цели из списков
void LAi_group_ClearAllTargets()
{
	SendMessage(&LAi_grp_relations, "s", "ClearAllTargets");
}

//Получить значение тревоги для игрока
float LAi_group_GetPlayerAlarm()
{
	return LAi_grp_playeralarm;
}

//Активна ли тревога для игрока
bool LAi_group_IsActivePlayerAlarm()
{
	return LAi_grp_alarmactive;
}

//Установить квест на убивание группы
void LAi_group_SetCheck(string groupName, string quest)
{
	//Найдём свободное поле
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(quests);
	string atr = "q";
	for(int i = 0; i < num + 10; i++)
	{
		atr = "q" + i;
		if(!CheckAttribute(quests, atr)) break;
	}
	//Заполним
	quests.(atr).group = groupName;
	quests.(atr).quest = quest;
}

// Warship 18.08.09 Проверка убийства группы с вызовом функции
void LAi_group_SetCheckFunction(String _groupName, String _function)
{
	//Найдём свободное поле
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(quests);
	String atr = "q";
	
	for(int i = 0; i < num + 10; i++)
	{
		atr = "q" + i;
		if(!CheckAttribute(quests, atr)) break;
	}
	
	//Заполним
	quests.(atr).group = _groupName;
	quests.(atr).function = _function;
}

//Установить событие на убивание группы
void LAi_group_SetCheckEvent(string groupName)
{
	//Найдём свободное поле
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(quests);
	string atr = "q";
	for(int i = 0; i < num + 10; i++)
	{
		atr = "q" + i;
		if(!CheckAttribute(quests, atr)) break;
	}
	//Заполним
	quests.(atr).group = groupName;
}


//Убрать квест на убивание группы
void LAi_group_RemoveCheck(string groupName)
{
	//Найдём свободное поле
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(quests);
	for(int i = 0; i < num; i++)
	{
		aref atr = GetAttributeN(&quests, i);
		if(atr.group == groupName)
		{
			//Удалим
			DeleteAttribute(&quests, GetAttributeName(GetAttributeN(&quests, i)));
			i = -1;
			num = GetAttributesNum(&quests);
		}		
	}
}

//Проверить квест на убивание группы
void LAi_group_CheckGroupQuest(aref chr)
{
	//Получим ветку с квестами
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(&quests);
	String function;
	
	if(num <= 0) return;
	//Проверим остальных персонажей в данной группе
	if(!CheckAttribute(chr, "chr_ai.group"))
	{
		Trace("CheckGroupQuest: Character <" + chr.id + "> can't have field chr_ai.group");
		return;
	}
	string group = chr.chr_ai.group;
	int chridx = sti(chr.index);
	for(int i = 0; i < LAi_numloginedcharacters; i++)
	{
		int idx = LAi_loginedcharacters[i];
		if(idx >= 0)
		{
			if(idx != chridx)
			{
				ref ch = &Characters[idx];
				if(!CheckAttribute(ch, "chr_ai.group")) ch.chr_ai.group = LAI_DEFAULT_GROUP;
				if(group == ch.chr_ai.group)
				{
					//Есть ещё живые в этой группе
					return;
				}
			}
		}
	}
	//Последний из магикан...
	for(i = 0; i < num; i++)
	{
		aref atr = GetAttributeN(&quests, i);
		if(atr.group == group)
		{
			//Нашли квест, запустим его на исполнение
			if(CheckAttribute(atr, "quest"))
			{
				LAi_QuestDelay(atr.quest, 0.000001);
			}
			else
			{
				// Warship 18.08.09 Вызов функции
				if(CheckAttribute(atr, "function"))
				{
					function = atr.function;
					call function(function); // В параметре - название самой функции
				}
				else
				{
				PostEvent("LAi_event_GroupKill", 1, "s", group);
			}
			}
			string atname = GetAttributeName(GetAttributeN(&quests, i));
			//Теперь удалим
			DeleteAttribute(&quests, atname);
			i = -1;
			num = GetAttributesNum(&quests);
		}
	}
}


//------------------------------------------------------------------------------------------
//Ответная реакция на запросы
//------------------------------------------------------------------------------------------

//Обновление аларма, вызывается на каждом кадре
#event_handler("CharacterGroup_UpdateAlarm", "LAi_group_UpdateAlarm");
void LAi_group_UpdateAlarm()
{
	LAi_grp_playeralarm = GetEventData();
	LAi_grp_alarmactive = GetEventData();

	/*if(CheckAttribute(pchar, "sneak.success"))
	{
		if(sti(pchar.sneak.success) == 1)
		{
			LAi_grp_playeralarm = 0.0;
			LAi_grp_alarmactive = false;
		}
	}  */

	Sound_OnAlarm(LAi_grp_alarmactive);
}
// boal 14.02.06 меню дебугера
// Hokkins: Обновленная верстка (25.07.2022)
string totalInfo;
int idLngFile = -1;
int remInt = 0;
string textLine = "***********************************"

object DebugMenu;
int DebugMenuCount = 0;
int DebugMenuGroup = 0;
bool PhonkMissileSettings = false;
int PhonkMissileSection = 0;

float PhonkMissile_Multiplier(string path)
{
	float value = 1.0;
	if (CheckAttribute(pchar,path)) value = stf(pchar.(path));
	if (value < 0.25) value = 0.25;
	if (value > 4.0) value = 4.0;
	return value;
}

void PhonkMissile_ListRefresh()
{
	int selected = sti(Sea.Phonk.MissileType) + 1;
	if (CheckAttribute(&GameInterface,"PHONK_MISSILES.select")) selected = sti(GameInterface.PHONK_MISSILES.select);
	int top = 0;
	if (CheckAttribute(&GameInterface,"PHONK_MISSILES.top")) top = sti(GameInterface.PHONK_MISSILES.top);
	DeleteAttribute(&GameInterface,"PHONK_MISSILES");
	int type = 0;
	string name = SeaPhonk_MissileName(type);
	while (name != "")
	{
		string row = "tr" + (type + 1);
		if (type == sti(Sea.Phonk.MissileType)) name += " [выбрана]";
		GameInterface.PHONK_MISSILES.(row).td1.str = name;
		type = type + 1;
		name = SeaPhonk_MissileName(type);
	}
	if (selected < 1 || selected > type) selected = 1;
	if (top < 0) top = 0;
	if (selected <= top) top = selected - 1;
	if (selected > top + 4) top = selected - 4;
	GameInterface.PHONK_MISSILES.select = selected;
	GameInterface.PHONK_MISSILES.top = top;
	Table_UpdateWindow("PHONK_MISSILES");
}

void PhonkMissile_ListSelect(int type)
{
	if (!PhonkMissileSettings || PhonkMissileSection != 0 || type < 0 || SeaPhonk_MissileName(type) == "") return;
	Sea.Phonk.MissileType = type;
	Sea.Phonk.Weapon = 7;
	PhonkMissile_Refresh();
}

void PhonkMissile_TableClick()
{
	string node = GetEventData();
	int row = GetEventData();
	if (node == "PHONK_MISSILES") PhonkMissile_ListSelect(row);
}

void PhonkMissile_TableActivate()
{
	string node = GetEventData();
	int row = GetEventData();
	if (node == "PHONK_MISSILES") PhonkMissile_ListSelect(row);
}

void PhonkMissile_Refresh()
{
	string label;
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY0",0,"#Ракета");
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY1",0,"#Боеголовка");
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY2",0,"#ПВО");
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY3",0,"#Корабли");
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY4",0,"#ПВО корабля");
	for (int i = 0; i < 8; i++) SetNodeUsing("SELECTED" + i, i == PhonkMissileSection);
	bool nuclear = sti(Sea.Phonk.MissileWarhead) == 0;
	bool cluster = sti(Sea.Phonk.MissileWarhead) == 2;
	for (i = 0; i < 13; i++)
	{
		bool visible = (PhonkMissileSection == 1 && (i < 5 || ((nuclear || cluster) && i < 7) || (nuclear && i < 9) || i == 11)) || (PhonkMissileSection == 2 && (i < 7 || i == 8 || i == 9 || i == 10 || i == 11)) || (PhonkMissileSection == 3 && (i < 5 || i == 6 || i == 8 || i == 9)) || (PhonkMissileSection == 4 && (i == 0 || i == 1 || i == 2 || i == 4 || i == 6 || i == 8 || i == 9 || i == 10));
		if (PhonkMissileSection == 1 && !nuclear && sti(Sea.Phonk.MissileWarhead) != 5 && i >= 7 && i <= 10) visible = true;
		if (PhonkMissileSection == 2 && (i == 3 || i == 5)) visible = false;
		SetNodeUsing("ACTION" + i, visible);
		SetSelectable("ACTION" + i, visible);
		if (PhonkMissileSection == 2 && (i == 2 || i == 4 || i == 6)) SetSelectable("ACTION" + i, false);
		if (PhonkMissileSection == 3 && (i == 4 || i == 6)) SetSelectable("ACTION" + i, false);
		if (PhonkMissileSection == 4 && (i == 2 || i == 4 || i == 6)) SetSelectable("ACTION" + i, false);
	}
	bool missileList = PhonkMissileSection == 0;
	SetNodeUsing("PHONK_MISSILES",missileList); SetSelectable("PHONK_MISSILES",missileList);
	SetNodeUsing("PHONK_MISSILES_SCROLL",missileList); SetSelectable("PHONK_MISSILES_SCROLL",missileList);
	for (i = 0; i < 8; i++) { bool shown = missileList && (sti(Sea.Phonk.MissileType) != 3 || i < 3 || i == 7); SetNodeUsing("PHONK_CONTROL" + i,shown); SetSelectable("PHONK_CONTROL" + i,shown); }
	for (i = 0; i < 3; i++) { bool showSlide = PhonkMissileSection == 2 || PhonkMissileSection == 4 || (PhonkMissileSection == 3 && (i == 1 || i == 2)); SetNodeUsing("PHONK_SLIDE" + i, showSlide); SetSelectable("PHONK_SLIDE" + i, showSlide); }
	if (PhonkMissileSection >= 2)
	{
		// Reset before changing the scale so an unchanged index cannot leave the pointer at its old position.
		for (i = 0; i < 3; i++) SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE" + i,3,0.0);
		SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE0",1,760);
		if (PhonkMissileSection == 3)
		{
			SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE1",1,15);
			SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE2",1,195);
		}
		else
		{
			SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE1",1,495);
			SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE2",1,31);
		}
	}
	if (PhonkMissileSection == 0)
	{
		SetFormatedText("SECTION_TITLE","Ракета");
		PhonkMissile_ListRefresh();
		if (sti(Sea.Phonk.MissileHoming) != 0) label = "Наведение: вкл"; else label = "Наведение: выкл";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL0",0,"#" + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL1",0,"#Интервал -");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL2",0,"#Интервал +");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL3",0,"#Скорость -");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL4",0,"#Скорость +");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL5",0,"#Ускорение -");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL6",0,"#Ускорение +");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"PHONK_CONTROL7",0,"#Дальность: " + sti(Sea.Phonk.MissileRange) + " м");
		label = "Выбрана: " + SeaPhonk_MissileName(sti(Sea.Phonk.MissileType));
		string missilePath = "SeaPhonk.Missile" + sti(Sea.Phonk.MissileType) + ".";
		if (sti(Sea.Phonk.MissileType) != 3) label += NewStr() + "Скорость: x" + FloatToString(PhonkMissile_Multiplier(missilePath + "Speed"), 2) + "   Ускорение: x" + FloatToString(PhonkMissile_Multiplier(missilePath + "Acceleration"), 2);
		label += NewStr() + "Интервал: " + FloatToString(stf(Sea.Phonk.NuclearInterval), 1) + " с (при мгновенной перезарядке)";
	}
	else if (PhonkMissileSection == 1)
	{
		SetFormatedText("SECTION_TITLE","Боеголовка");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION0",0,"#Ядерная");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION1",0,"#Осколочно-фугасная");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION2",0,"#Кассетная");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION3",0,"#Зажигательная");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION4",0,"#Термобарическая");
		if (sti(Sea.Phonk.MissileWarhead) == 5) label = "Случайная [выбрана]"; else label = "Случайная";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION11",0,"#" + label);
		if (cluster) label = "Суббоеприпасы"; else label = "Боеголовки";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION5",0,"#" + label + " -");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION6",0,"#" + label + " +");
		if (nuclear) label = "Мощность"; else label = "Урон x";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION7",0,"#" + label + " -");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION8",0,"#" + label + " +");
		if (!nuclear) {
			SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION9",0,"#Радиус x -");
			SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION10",0,"#Радиус x +");
		}
		label = "Выбрана: ";
		switch (sti(Sea.Phonk.MissileWarhead))
		{
			case 0: label += "ядерная" + NewStr() + "Мощный взрыв и волна поражают большую область."; break;
			case 1: label += "осколочно-фугасная" + NewStr() + "Взрыв и осколки поражают корпус, экипаж и паруса."; break;
			case 2: label += "кассетная" + NewStr() + "Раскрывается в воздухе и накрывает область суббоеприпасами."; break;
			case 3: label += "осколочно-фугасная" + NewStr() + "Взрыв и осколки поражают корпус, экипаж и паруса."; break;
			case 4: label += "зажигательная" + NewStr() + "Взрыв может поджечь поражённые корабли."; break;
			case 5: label += "случайная" + NewStr() + "Случайная неядерная боеголовка при каждом пуске."; break;
			case PHONK_WARHEAD_THERMOBARIC: label += "термобарическая" + NewStr() + "Объёмный взрыв поражает экипаж и может поджечь корабль."; break;
		}
		if (nuclear) label += NewStr() + "Боеголовки: " + sti(Sea.Phonk.NuclearWarheads) + "   Уровень мощности: " + sti(Sea.Phonk.NuclearPowerLimit);
		if (cluster) label += NewStr() + "Суббоеприпасы: " + sti(Sea.Phonk.ClusterCount);
		if (!nuclear && sti(Sea.Phonk.MissileWarhead) != 5) {
			string setting = "SeaPhonk.Warhead" + sti(Sea.Phonk.MissileWarhead);
			string powerPath = setting + ".Power";
			string radiusPath = setting + ".Radius";
			float power = 1.0; float radius = 1.0;
			if (CheckAttribute(pchar,powerPath)) power = stf(pchar.(powerPath));
			if (CheckAttribute(pchar,radiusPath)) radius = stf(pchar.(radiusPath));
			if (power < 0.5) power = 0.5; if (power > 2.0) power = 2.0;
			if (radius < 0.5) radius = 0.5; if (radius > 2.0) radius = 2.0;
			label += NewStr() + "Урон: x" + FloatToString(power, 2) + "   Радиус: x" + FloatToString(radius, 2);
		}
	}
	else if (PhonkMissileSection == 2)
	{
		SetFormatedText("SECTION_TITLE","ПВО");
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseAvailable") || sti(Sea.Phonk.AirDefenseAvailable) == 0) label = "ПВО: недоступна";
		else if (sti(Sea.Phonk.AirDefenseEnabled) != 0) label = "ПВО: включена"; else label = "ПВО: выключена";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION0",0,"#" + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION1",0,"#Создать врага");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION8",0,"#Создать союзника");
		if (sti(Sea.Phonk.AirDefenseEconomy) != 0) label = "Режим: экономный"; else label = "Режим: полный";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION9",0,"#" + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION2",0,"#Дальность: " + sti(Sea.Phonk.AirDefenseRange) + " м");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION4",0,"#Интервал: " + FloatToString(stf(Sea.Phonk.AirDefenseInterval), 2) + " с");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION6",0,"#Одновременно в воздухе: " + sti(Sea.Phonk.AirDefenseMax));
		label = "Панцирь";
		if (sti(Sea.Phonk.AirDefenseType) == 1) label = "Patriot";
		if (sti(Sea.Phonk.AirDefenseType) == 2) label = "Панцирь + Patriot";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION10",0,"#Перехватчик: " + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION11",0,"#На одну ракету: " + sti(Sea.Phonk.AirDefensePerTarget));
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE0",3,(stf(Sea.Phonk.AirDefenseRange)-400.0)/7600.0 + 0.000001);
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE1",3,(stf(Sea.Phonk.AirDefenseInterval)-0.05)/4.95 + 0.000001);
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE2",3,(stf(Sea.Phonk.AirDefenseMax)-1.0)/31.0 + 0.000001);
		label = "Интервал между пусками: " + FloatToString(stf(Sea.Phonk.AirDefenseInterval), 2) + " с" + NewStr() + "Одновременно в воздухе: " + sti(Sea.Phonk.AirDefenseMax);
	}
	else if (PhonkMissileSection == 3)
	{
		SetFormatedText("SECTION_TITLE","Создание корабля");
		if (sti(pchar.SeaPhonk.Spawn.Custom)) label = "Вооружение: своё"; else label = "Вооружение: случайное";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION0",0,"#" + label);
		label = "случайный";
		switch (sti(pchar.SeaPhonk.Spawn.ShipClass)) { case 1: label = "биланчета"; break; case 2: label = "корвет"; break; case 3: label = "фрегат"; break; case 4: label = "линкор"; break; case 5: label = "мановар"; break; }
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION1",0,"#Корабль: " + label);
		label = SeaPhonk_MissileName(sti(pchar.SeaPhonk.Spawn.MissileType));
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION2",0,"#Ракета: " + label);
		label = "случайная"; if (sti(pchar.SeaPhonk.Spawn.Warhead) == 1) label = "фугасная"; if (sti(pchar.SeaPhonk.Spawn.Warhead) == 2) label = "кассетная"; if (sti(pchar.SeaPhonk.Spawn.Warhead) == 4) label = "зажигательная"; if (sti(pchar.SeaPhonk.Spawn.Warhead) == PHONK_WARHEAD_THERMOBARIC) label = "термобарическая";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION3",0,"#БЧ: " + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION4",0,"#Ракет в залпе: " + sti(pchar.SeaPhonk.Spawn.Volley));
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION6",0,"#Перерыв: " + FloatToString(stf(pchar.SeaPhonk.Spawn.FireInterval), 1) + " с");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION8",0,"#Создать врага");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION9",0,"#Создать союзника");
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE1",3,(stf(pchar.SeaPhonk.Spawn.Volley)-1.0)/15.0 + 0.000001);
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE2",3,(stf(pchar.SeaPhonk.Spawn.FireInterval)-0.5)/19.5 + 0.000001);
		label = "Настройки применяются к следующему кораблю.";
	}
	else
	{
		SetFormatedText("SECTION_TITLE","ПВО создаваемого корабля");
		if (sti(pchar.SeaPhonk.Spawn.DefenseEnabled)) label = "ПВО: включена"; else label = "ПВО: выключена";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION0",0,"#" + label);
		if (sti(pchar.SeaPhonk.Spawn.DefenseCustom)) label = "ПВО: своё"; else label = "ПВО: случайное";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION1",0,"#" + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION2",0,"#Дальность: " + sti(pchar.SeaPhonk.Spawn.DefenseRange) + " м");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION4",0,"#Интервал: " + FloatToString(stf(pchar.SeaPhonk.Spawn.DefenseInterval), 2) + " с");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION6",0,"#В воздухе: " + sti(pchar.SeaPhonk.Spawn.DefenseMax));
		label = "случайный"; if (sti(pchar.SeaPhonk.Spawn.AirDefenseType) == 0) label = "Панцирь"; if (sti(pchar.SeaPhonk.Spawn.AirDefenseType) == 1) label = "Patriot";
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION10",0,"#Перехватчик: " + label);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION8",0,"#Создать врага");
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"ACTION9",0,"#Создать союзника");
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE0",3,(stf(pchar.SeaPhonk.Spawn.DefenseRange)-400.0)/7600.0 + 0.000001);
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE1",3,(stf(pchar.SeaPhonk.Spawn.DefenseInterval)-0.05)/4.95 + 0.000001);
		SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"PHONK_SLIDE2",3,(stf(pchar.SeaPhonk.Spawn.DefenseMax)-1.0)/31.0 + 0.000001);
		if (sti(pchar.SeaPhonk.Spawn.DefenseCustom)) label = "ПВО: свои настройки."; else label = "ПВО: случайные параметры.";
	}
	SetFormatedText("DEBUGER_INFO", label);
}

void PhonkMissile_Command(string node)
{
	if (node == "CATEGORY0" || node == "CATEGORY1" || node == "CATEGORY2" || node == "CATEGORY3" || node == "CATEGORY4")
	{
		if (node == "CATEGORY0") PhonkMissileSection = 0;
		if (node == "CATEGORY1") PhonkMissileSection = 1;
		if (node == "CATEGORY2") PhonkMissileSection = 2;
		if (node == "CATEGORY3") PhonkMissileSection = 3;
		if (node == "CATEGORY4") PhonkMissileSection = 4;
	}
	else if (PhonkMissileSection == 0)
	{
		if (node == "ACTION6" || node == "ACTION7" || node == "ACTION8" || node == "ACTION9")
		{
			string missilePath = "SeaPhonk.Missile" + sti(Sea.Phonk.MissileType) + ".";
			if (node == "ACTION6" || node == "ACTION7") missilePath += "Speed"; else missilePath += "Acceleration";
			float multiplier = PhonkMissile_Multiplier(missilePath);
			if (node == "ACTION6" || node == "ACTION8") multiplier -= 0.05; else multiplier += 0.05;
			if (multiplier < 0.25) multiplier = 0.25;
			if (multiplier > 4.0) multiplier = 4.0;
			pchar.(missilePath) = multiplier;
			PhonkMissile_Refresh();
			return;
		}
		switch (node)
	{
		case "ACTION2": Sea.Phonk.MissileHoming = !sti(Sea.Phonk.MissileHoming); break;
		case "ACTION3": Sea.Phonk.NuclearInterval = stf(Sea.Phonk.NuclearInterval) - 0.1; if (stf(Sea.Phonk.NuclearInterval) < 0.0) Sea.Phonk.NuclearInterval = 0.0; break;
		case "ACTION4": Sea.Phonk.NuclearInterval = stf(Sea.Phonk.NuclearInterval) + 0.1; if (stf(Sea.Phonk.NuclearInterval) > 10.0) Sea.Phonk.NuclearInterval = 10.0; break;
		case "ACTION10": Sea.Phonk.MissileRange = sti(Sea.Phonk.MissileRange) + 500; if (sti(Sea.Phonk.MissileRange) > 20000) Sea.Phonk.MissileRange = 500; break;
		}
	}
	else if (PhonkMissileSection == 1)
	{
		if (sti(Sea.Phonk.MissileWarhead) != 0 && (node == "ACTION7" || node == "ACTION8" || node == "ACTION9" || node == "ACTION10")) {
			string path = "SeaPhonk.Warhead" + sti(Sea.Phonk.MissileWarhead) + ".";
			if (node == "ACTION7" || node == "ACTION8") path += "Power"; else path += "Radius";
			float current = 1.0;
			if (CheckAttribute(pchar,path)) current = stf(pchar.(path));
			if (node == "ACTION7" || node == "ACTION9") current -= 0.05; else current += 0.05;
			if (current < 0.5) current = 0.5;
			if (current > 2.0) current = 2.0;
			pchar.(path) = current;
			PhonkMissile_Refresh();
			return;
		}
		switch (node)
	{
		case "ACTION0": Sea.Phonk.MissileWarhead = 0; break;
		case "ACTION1": Sea.Phonk.MissileWarhead = 1; break;
		case "ACTION2": Sea.Phonk.MissileWarhead = 2; break;
		case "ACTION3": Sea.Phonk.MissileWarhead = 4; break;
		case "ACTION4": Sea.Phonk.MissileWarhead = PHONK_WARHEAD_THERMOBARIC; break;
		case "ACTION11": Sea.Phonk.MissileWarhead = 5; break;
		case "ACTION5": if (sti(Sea.Phonk.MissileWarhead) == 0 && sti(Sea.Phonk.NuclearWarheads) > 1) Sea.Phonk.NuclearWarheads = sti(Sea.Phonk.NuclearWarheads) - 1; else if (sti(Sea.Phonk.MissileWarhead) == 2 && sti(Sea.Phonk.ClusterCount) > 20) Sea.Phonk.ClusterCount = sti(Sea.Phonk.ClusterCount) - 1; break;
		case "ACTION6": if (sti(Sea.Phonk.MissileWarhead) == 0 && sti(Sea.Phonk.NuclearWarheads) < 20) Sea.Phonk.NuclearWarheads = sti(Sea.Phonk.NuclearWarheads) + 1; else if (sti(Sea.Phonk.MissileWarhead) == 2 && sti(Sea.Phonk.ClusterCount) < 30) Sea.Phonk.ClusterCount = sti(Sea.Phonk.ClusterCount) + 1; break;
		case "ACTION7": if (sti(Sea.Phonk.NuclearPowerLimit) > 1) Sea.Phonk.NuclearPowerLimit = sti(Sea.Phonk.NuclearPowerLimit) - 1; break;
		case "ACTION8": if (sti(Sea.Phonk.NuclearPowerLimit) < 10) Sea.Phonk.NuclearPowerLimit = sti(Sea.Phonk.NuclearPowerLimit) + 1; break;
		}
	}
	else if (PhonkMissileSection == 2)
	{
		switch (node)
	{
		case "ACTION0": if (CheckAttribute(&Sea,"Phonk.AirDefenseAvailable") && sti(Sea.Phonk.AirDefenseAvailable) != 0) Sea.Phonk.AirDefenseEnabled = !sti(Sea.Phonk.AirDefenseEnabled); break;
		case "ACTION1": SeaPhonkTest_Start(); break;
		case "ACTION8": SeaPhonkTest_StartAlly(); break;
		case "ACTION9": Sea.Phonk.AirDefenseEconomy = !sti(Sea.Phonk.AirDefenseEconomy); break;
		case "ACTION10": Sea.Phonk.AirDefenseType = (sti(Sea.Phonk.AirDefenseType) + 1) % 3; pchar.SeaPhonk.AirDefenseType = sti(Sea.Phonk.AirDefenseType); break;
		case "ACTION11": Sea.Phonk.AirDefensePerTarget = sti(Sea.Phonk.AirDefensePerTarget) % 8 + 1; pchar.SeaPhonk.AirDefensePerTarget = sti(Sea.Phonk.AirDefensePerTarget); break;
		case "ACTION2": Sea.Phonk.AirDefenseInterval = stf(Sea.Phonk.AirDefenseInterval) - 0.1; if (stf(Sea.Phonk.AirDefenseInterval) < 0.1) Sea.Phonk.AirDefenseInterval = 0.1; break;
		case "ACTION3": Sea.Phonk.AirDefenseInterval = stf(Sea.Phonk.AirDefenseInterval) + 0.1; if (stf(Sea.Phonk.AirDefenseInterval) > 2.0) Sea.Phonk.AirDefenseInterval = 2.0; break;
		case "ACTION4": if (sti(Sea.Phonk.AirDefenseMax) > 1) Sea.Phonk.AirDefenseMax = sti(Sea.Phonk.AirDefenseMax) - 1; break;
		case "ACTION5": if (sti(Sea.Phonk.AirDefenseMax) < 12) Sea.Phonk.AirDefenseMax = sti(Sea.Phonk.AirDefenseMax) + 1; break;
		case "ACTION6": break;
		}
	}
	else if (PhonkMissileSection == 3)
	{
		if (node == "ACTION0") pchar.SeaPhonk.Spawn.Custom = !sti(pchar.SeaPhonk.Spawn.Custom);
		if (node == "ACTION1") pchar.SeaPhonk.Spawn.ShipClass = (sti(pchar.SeaPhonk.Spawn.ShipClass) + 1) % 6;
		if (node == "ACTION2") { pchar.SeaPhonk.Spawn.MissileType = SeaPhonk_NextMissileType(sti(pchar.SeaPhonk.Spawn.MissileType)); pchar.SeaPhonk.Spawn.Custom = 1; }
		if (node == "ACTION3") { int warhead = sti(pchar.SeaPhonk.Spawn.Warhead); if (warhead == 5) warhead = 1; else if (warhead == 1) warhead = 2; else if (warhead == 2) warhead = 4; else if (warhead == 4) warhead = PHONK_WARHEAD_THERMOBARIC; else warhead = 5; pchar.SeaPhonk.Spawn.Warhead = warhead; pchar.SeaPhonk.Spawn.Custom = 1; }
		if (node == "ACTION8") SeaPhonkTest_Start();
		if (node == "ACTION9") SeaPhonkTest_StartAlly();
	}
	else
	{
		if (node == "ACTION0") { pchar.SeaPhonk.Spawn.DefenseEnabled = !sti(pchar.SeaPhonk.Spawn.DefenseEnabled); pchar.SeaPhonk.Spawn.DefenseCustom = 1; }
		if (node == "ACTION1") pchar.SeaPhonk.Spawn.DefenseCustom = !sti(pchar.SeaPhonk.Spawn.DefenseCustom);
		if (node == "ACTION10") pchar.SeaPhonk.Spawn.AirDefenseType = (sti(pchar.SeaPhonk.Spawn.AirDefenseType) + 1) % 3;
		if (node == "ACTION8") SeaPhonkTest_Start();
		if (node == "ACTION9") SeaPhonkTest_StartAlly();
	}
	PhonkMissile_Refresh();
}

void PhonkMissile_Slide()
{
	string node = GetEventData();
	int step = GetEventData();
	float value = GetEventData();
	if (!PhonkMissileSettings || PhonkMissileSection < 2) return;
	if (PhonkMissileSection == 2) {
		if (node == "PHONK_SLIDE0") Sea.Phonk.AirDefenseRange = 400 + step * 10;
		if (node == "PHONK_SLIDE1") Sea.Phonk.AirDefenseInterval = 0.05 + step * 0.01;
		if (node == "PHONK_SLIDE2") Sea.Phonk.AirDefenseMax = 1 + step;
	} else if (PhonkMissileSection == 3) {
		if (node == "PHONK_SLIDE1") pchar.SeaPhonk.Spawn.Volley = 1 + step;
		if (node == "PHONK_SLIDE2") pchar.SeaPhonk.Spawn.FireInterval = 0.5 + step * 0.1;
		pchar.SeaPhonk.Spawn.Custom = 1;
	} else if (PhonkMissileSection == 4) {
		if (node == "PHONK_SLIDE0") pchar.SeaPhonk.Spawn.DefenseRange = 400 + step * 10;
		if (node == "PHONK_SLIDE1") pchar.SeaPhonk.Spawn.DefenseInterval = 0.05 + step * 0.01;
		if (node == "PHONK_SLIDE2") pchar.SeaPhonk.Spawn.DefenseMax = 1 + step;
		pchar.SeaPhonk.Spawn.DefenseCustom = 1;
	}
	PhonkMissile_Refresh();
}

void DebugMenu_Add(int group, string action, string label)
{
	if (group == 3) group = 2;
	else if (group >= 4) group--;
	string key = "a" + DebugMenuCount;
	DebugMenu.actions.(key).group = group;
	DebugMenu.actions.(key).action = action;
	DebugMenu.actions.(key).label = label;
	DebugMenuCount++;
}

bool DebugMenu_CheatEnabled(string name)
{
	string pending = "DebugCheats.Pending." + name;
	if (CheckAttribute(pchar,pending)) return sti(pchar.(pending)) != 0;
	string attribute = "DebugCheats." + name;
	return CheckAttribute(pchar,attribute) && sti(pchar.(attribute)) != 0;
}

string DebugMenu_CheatLabel(string name, string label)
{
	if (DebugMenu_CheatEnabled(name)) label += ": вкл";
	else label += ": выкл";
	if (CheckAttribute(pchar,"DebugCheats.Pending." + name)) label += " (после залпа)";
	return label;
}

string DebugMenu_PairLabel(string label, bool first, bool second, bool pending)
{
	if (first && second) label += ": вкл";
	else if (!first && !second) label += ": выкл";
	else label += ": частично";
	if (pending) label += " (после залпа)";
	return label;
}

string DebugMenu_Label(string action, string label)
{
	bool state;
	bool toggle = false;
	switch (action)
	{
		case "BTN_SPECIAL1": state = bNoEatNoRats; toggle = true; break;
		case "BTN_SPECIAL2": state = bQuestLogShow; toggle = true; break;
		case "BTN_SPECIAL3": state = TestRansackCaptain; toggle = true; break;
		case "BTN_SPECIAL4": state = bLocatorShow; toggle = true; break;
		case "BTN_SPECIAL5": state = bExpLogShow; toggle = true; break;
		case "BTN_SPECIAL6": state = bPGGLogShow; toggle = true; break;
		case "BTN_SPECIAL7": state = locCameraEnableFree; toggle = true; break;
		case "BTN_SPECIAL8": state = bWorldAlivePause; toggle = true; break;
		case "BTN_SPECIAL9": state = bBettaTestMode; toggle = true; break;
		case "B_F10": state = LAi_IsImmortal(pchar); toggle = true; break;
		case "B_F13": state = !CheckAttribute(pchar,"worldmapencountersoff") || pchar.worldmapencountersoff != "1"; toggle = true; break;
		case "B_F14": state = globalSGMode; toggle = true; break;
		case "PHONK": state = CheckAttribute(pchar,"SeaPhonk.Active") && sti(pchar.SeaPhonk.Active) != 0; toggle = true; break;
		case "PHONK_MUSIC": state = !SeaPhonk_IsMusicMuted(); toggle = true; break;
		case "INFINITE_ENERGY": state = LAi_HasInfiniteEnergy(pchar); toggle = true; break;
		case "ENEMY_ALL": state = CheckAttribute(pchar,"DebugEnemyAll") && sti(pchar.DebugEnemyAll); toggle = true; break;
		case "INVISIBLE_AI": state = CheckAttribute(pchar,"DebugInvisibleAI") && sti(pchar.DebugInvisibleAI); toggle = true; break;
		case "INSTANT_CANNON_RELOAD": return DebugMenu_CheatLabel("InstantCannonReload",label); break;
		case "UNLIMITED_AMMO": return DebugMenu_CheatLabel("UnlimitedAmmo",label); break;
		case "SURVIVAL_CHEATS": return DebugMenu_PairLabel(label,LAi_IsImmortal(pchar),LAi_HasInfiniteEnergy(pchar),false); break;
		case "CANNON_CHEATS": return DebugMenu_PairLabel(label,DebugMenu_CheatEnabled("InstantCannonReload"),DebugMenu_CheatEnabled("UnlimitedAmmo"),CheckAttribute(pchar,"DebugCheats.Pending.InstantCannonReload") || CheckAttribute(pchar,"DebugCheats.Pending.UnlimitedAmmo")); break;
		case "AUTOPILOT_TRADE": state = SeaAutopilot_Mode() == 1; toggle = true; break;
		case "AUTOPILOT_HUNT": state = SeaAutopilot_Mode() == 2; toggle = true; break;
		case "TSUNAMI_START":
			if (IsEntity(&worldMap)) return "Цунами: дуга";
			return label;
		break;
		case "WHIRLPOOL":
			if (IsEntity(&Sea) && CheckAttribute(&Sea,"Whirlpool.Active") && sti(Sea.Whirlpool.Active)) return "Убрать водоворот";
			return label;
		break;
	}
	if (toggle)
	{
		if (state) return label + ": вкл";
		return label + ": выкл";
	}
	return label;
}

bool DebugMenu_Enabled(string action)
{
	bool seaEffects = !bAbordageStarted && bSeaActive && IsEntity(&Sea) && !IsEntity(&worldMap);
	switch (action)
	{
		case "TSUNAMI_START": return !bAbordageStarted && ((bSeaActive && IsEntity(&Sea)) || IsEntity(&worldMap)); break;
		case "TSUNAMI_RING": return !bAbordageStarted && IsEntity(&worldMap); break;
		case "WEATHER_STORM": return !bAbordageStarted && bSeaActive && IsEntity(&Weather); break;
		case "WEATHER_NEXT": return !bAbordageStarted && bSeaActive && IsEntity(&Weather); break;
		case "PHONK": return !bAbordageStarted && !IsEntity(&worldMap); break;
		case "PHONK_MUSIC": return seaEffects; break;
		case "WHIRLPOOL": return seaEffects && !GreatRecipe_Locked(); break;
		case "AUTOPILOT_TRADE": return seaEffects || IsEntity(&worldMap); break;
		case "AUTOPILOT_HUNT": return seaEffects || IsEntity(&worldMap); break;
	}
	return true;
}

void DebugMenu_Refresh()
{
	DeleteAttribute(&DebugMenu,"slots");
	int selected = 1;
	if (CheckAttribute(&GameInterface,"DEBUG_ACTIONS.select")) selected = sti(GameInterface.DEBUG_ACTIONS.select);
	DeleteAttribute(&GameInterface,"DEBUG_ACTIONS");
	int slot = 0;
	for (int i = 0; i < DebugMenuCount; i++)
	{
		string key = "a" + i;
		if (sti(DebugMenu.actions.(key).group) != DebugMenuGroup) continue;
		slot++;
		string node = "tr" + slot;
		string action = DebugMenu.actions.(key).action;
		DebugMenu.slots.(node) = action;
		GameInterface.DEBUG_ACTIONS.(node).td1.str = DebugMenu_Label(action,DebugMenu.actions.(key).label);
		if (!DebugMenu_Enabled(action)) GameInterface.DEBUG_ACTIONS.(node).td1.color = argb(255,128,128,128);
	}
	if (selected < 1 || selected > slot) selected = 1;
	GameInterface.DEBUG_ACTIONS.select = selected;
	Table_UpdateWindow("DEBUG_ACTIONS");
}

string DebugMenu_GroupName(int group)
{
	switch (group)
	{
		case 0: return "Эффекты"; break;
		case 1: return "Персонаж"; break;
		case 2: return "Корабль"; break;
		case 3: return "Мир и перемещение"; break;
		case 4: return "Квесты"; break;
		case 5: return "Камера"; break;
		case 6: return "Диагностика"; break;
	}
	return "";
}

void DebugMenu_SelectGroup(int group)
{
	DebugMenuGroup = group;
	GameInterface.DEBUG_ACTIONS.select = 1;
	for (int i = 0; i < 8; i++) SetNodeUsing("SELECTED" + i,i == group);
	DebugMenu_Refresh();
	string title = DebugMenu_GroupName(group);
	string hint;
	switch (group)
	{
		case 0: hint = "Фонк доступен на суше и в море. Бессмертие включается отдельно. Цунами работает в море и на карте; кольцо — только на карте."; break;
		case 1: hint = "Деньги, навыки, репутация и режим бессмертия."; break;
		case 2: hint = "Наборы заменяют корабль, навыки, снаряжение и запасы героя. Кнопки классов заменяют корабль с экипажем и грузом."; break;
		case 3: hint = "Погода, время, встречи и отношения наций. Для точного перехода введите: локация, группа, локатор."; break;
		case 4: hint = "Состояние квестов и тестовый переход в истории проклятого идола."; break;
		case 5: hint = "Камера, локаторы и интерфейс. Свободная камера требует включённых debug-клавиш в engine.ini."; break;
		case 6: hint = "Автопилот эскадры, логи, атрибуты и обновление данных."; break;
	}
	if (group == 6 && !bSeaActive && !IsEntity(&worldMap))
		hint = "Автопилот включается в море или на карте. Ручное управление можно вернуть всегда.";
	SetFormatedText("SECTION_TITLE",title);
	SetNodeUsing("DEBUGER_INFO",true);
	SetNodeUsing("DEBUGER_SCROLL",true);
	SetNodeUsing("DEBUGER_INFO_2",false);
	SetFormatedText("DEBUGER_INFO",hint);
	SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"DEBUGER_INFO",5);
}

void DebugMenu_FirstAction()
{
	SetCurrentNode("DEBUG_ACTIONS");
}

bool DebugEffects_Whirlpool()
{
	if (!bSeaActive || bAbordageStarted || !IsEntity(&Sea) || IsEntity(&worldMap) || GreatRecipe_Locked()) return false;
	if (CheckAttribute(&Sea,"Whirlpool.Active") && sti(Sea.Whirlpool.Active))
	{
		Sea.Whirlpool.Start = 0;
		return true;
	}
	if (!CheckAttribute(pchar,"Ship.Pos.x") || !CheckAttribute(pchar,"Ship.Pos.z") || !CheckAttribute(pchar,"Ship.Ang.y")) return false;
	// Start at the navigable rim, using the existing native water/current field.
	float radius = 700.0;
	Sea.Whirlpool.CenterX = stf(pchar.Ship.Pos.x) + sin(stf(pchar.Ship.Ang.y)) * radius * 0.78;
	Sea.Whirlpool.CenterZ = stf(pchar.Ship.Pos.z) + cos(stf(pchar.Ship.Ang.y)) * radius * 0.78;
	Sea.Whirlpool.Radius = radius;
	Sea.Whirlpool.Intensity = 1.0;
	Sea.Whirlpool.Duration = 90.0;
	Sea.Whirlpool.ResumeAge = 0.0;
	Sea.Whirlpool.Start = 1;
	return true;
}

void DebugMenu_GivePerks()
{
	aref perks;
	makearef(perks,ChrPerksList.list);
	for (int i = 0; i < GetAttributesNum(perks); i++)
	{
		string name = GetAttributeName(GetAttributeN(perks,i));
		// Free perks select special game modes (WildCaribbean), not ordinary abilities.
		if (CheckAttribute(perks,name + ".NPCOnly") || CheckAttribute(perks,name + ".Hiden") || CheckAttribute(perks,name + ".Free")) continue;
		if (!CheckCharacterPerk(pchar,name)) SetCharacterPerk(pchar,name);
	}
	Event("eSwitchPerks","l",GetMainCharacterIndex());
	SetFormatedText("DEBUGER_INFO","Выданы все обычные личные и корабельные перки. Особые режимы игры не меняются.");
	Statistic_AddValue(pchar,"Cheats.AllPerks",1);
}

void DebugEffects_Run(string node)
{
	if (!DebugMenu_Enabled(node) || (bAbordageStarted && node != "AUTOPILOT_OFF")) return;
	bool resume = false;
	switch (node)
	{
		case "TSUNAMI_START": resume = SeaTsunami_Start(); break;
		case "TSUNAMI_RING": resume = SeaTsunami_StartMapWithShape(1); break;
		case "WEATHER_STORM": Whr_RequestWeather(1.0, 1.0, 30.0, 600.0, false, false); resume = true; break;
		case "WEATHER_NEXT": Whr_LoadNextWeather(1); resume = true; break;
		case "PHONK": SeaPhonk_Toggle(); DebugMenu_Refresh(); break;
		case "PHONK_MUSIC": SeaPhonk_ToggleMusic(); DebugMenu_Refresh(); break;
		case "WHIRLPOOL": resume = DebugEffects_Whirlpool(); break;
		case "AUTOPILOT_TRADE": SeaAutopilot_SetMode(1); resume = true; break;
		case "AUTOPILOT_HUNT": SeaAutopilot_SetMode(2); resume = true; break;
		case "AUTOPILOT_OFF": SeaAutopilot_Stop(); resume = true; break;
	}
	if (resume) IDoExit(RC_INTERFACE_QUICK_SAVE);
}

void DebugMenu_SetCheat(string name, bool reloadCannons, bool desired)
{
	string attribute = "DebugCheats." + name;
	if (reloadCannons)
	{
		string pending = "DebugCheats.Pending." + name;
		bool current = CheckAttribute(pchar,attribute) && sti(pchar.(attribute));
		if (desired == current)
		{
			DeleteAttribute(pchar,pending);
			aref flags;
			makearef(flags,pchar.DebugCheats.Pending);
			if (GetAttributesNum(flags) == 0) DeleteAttribute(pchar,"DebugCheats.Pending");
		}
		else pchar.(pending) = desired;
		// The sea controller commits after the current salvo; ashore there are no loaded rounds.
		if (!bSeaLoaded || !IsEntity(&AISea) || !IsEntity(pchar)) Cannon_ApplyPendingCheats(pchar);
	}
	else
	{
		if (desired) pchar.(attribute) = true;
		else DeleteAttribute(pchar,attribute);
		if (name == "InfiniteEnergy" && desired) pchar.chr_ai.energy = LAi_GetCharacterMaxEnergy(pchar);
	}
	Statistic_AddValue(pchar,"Cheats." + name,1);
}

void DebugMenu_ToggleCheat(string name, bool reloadCannons)
{
	DebugMenu_SetCheat(name,reloadCannons,!DebugMenu_CheatEnabled(name));
}

void DebugMenu_ToggleSurvival()
{
	bool enabled = !(LAi_IsImmortal(pchar) && LAi_HasInfiniteEnergy(pchar));
	LAi_SetImmortal(pchar,enabled);
	DebugMenu_SetCheat("InfiniteEnergy",false,enabled);
}

void DebugMenu_ToggleCannons()
{
	bool enabled = !(DebugMenu_CheatEnabled("InstantCannonReload") && DebugMenu_CheatEnabled("UnlimitedAmmo"));
	DebugMenu_SetCheat("InstantCannonReload",true,enabled);
	DebugMenu_SetCheat("UnlimitedAmmo",true,enabled);
}

void InitInterface(string iniName)
{
 	StartAboveForm(true);
	
	SendMessage(&GameInterface,"ls",MSG_INTERFACE_INIT,iniName);
	SetNodeUsing("ACTION12",false); SetSelectable("ACTION12",false);
	for (int slide = 0; slide < 3; slide++) { SetNodeUsing("PHONK_SLIDE" + slide,false); SetSelectable("PHONK_SLIDE" + slide,false); }
	PhonkMissileSettings = CheckAttribute(&Sea,"Phonk.MissileSettings") && sti(Sea.Phonk.MissileSettings) != 0;
	SetNodeUsing("DEBUG_ACTIONS",!PhonkMissileSettings); SetSelectable("DEBUG_ACTIONS",!PhonkMissileSettings);
	SetNodeUsing("DEBUG_ACTIONS_SCROLL",!PhonkMissileSettings); SetSelectable("DEBUG_ACTIONS_SCROLL",!PhonkMissileSettings);
	SetNodeUsing("PHONK_MISSILES",false); SetSelectable("PHONK_MISSILES",false);
	SetNodeUsing("PHONK_MISSILES_SCROLL",false); SetSelectable("PHONK_MISSILES_SCROLL",false);
	for (int control = 0; control < 8; control++) { SetNodeUsing("PHONK_CONTROL" + control,false); SetSelectable("PHONK_CONTROL" + control,false); }
	if (PhonkMissileSettings)
	{
		if (!CheckAttribute(&Sea,"Phonk.MissileRange")) Sea.Phonk.MissileRange = 9000;
		if (!CheckAttribute(&Sea,"Phonk.MissileType")) Sea.Phonk.MissileType = 3;
		if (!CheckAttribute(&Sea,"Phonk.MissileHoming")) Sea.Phonk.MissileHoming = 1;
		if (!CheckAttribute(&Sea,"Phonk.MissileWarhead")) Sea.Phonk.MissileWarhead = 5;
		if (sti(Sea.Phonk.MissileWarhead) == 3) Sea.Phonk.MissileWarhead = 1;
		if (!CheckAttribute(&Sea,"Phonk.ClusterCount")) Sea.Phonk.ClusterCount = 24;
		if (sti(Sea.Phonk.ClusterCount) < 20) Sea.Phonk.ClusterCount = 20;
		if (sti(Sea.Phonk.ClusterCount) > 30) Sea.Phonk.ClusterCount = 30;
		if (!CheckAttribute(&Sea,"Phonk.NuclearWarheads")) Sea.Phonk.NuclearWarheads = 1;
		if (!CheckAttribute(&Sea,"Phonk.NuclearPowerLimit")) Sea.Phonk.NuclearPowerLimit = 10;
		if (!CheckAttribute(&Sea,"Phonk.NuclearInterval")) Sea.Phonk.NuclearInterval = 1.0;
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseEnabled")) Sea.Phonk.AirDefenseEnabled = 0;
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseType")) Sea.Phonk.AirDefenseType = 0;
		Sea.Phonk.AirDefenseType = SeaPhonk_PlayerDefenseType(sti(Sea.Phonk.AirDefenseType));
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseEconomy")) Sea.Phonk.AirDefenseEconomy = 1;
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseRange")) Sea.Phonk.AirDefenseRange = 800;
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseInterval")) Sea.Phonk.AirDefenseInterval = 0.25;
		if (stf(Sea.Phonk.AirDefenseInterval) < 0.05) Sea.Phonk.AirDefenseInterval = 0.05;
		if (stf(Sea.Phonk.AirDefenseInterval) > 5.0) Sea.Phonk.AirDefenseInterval = 5.0;
		if (!CheckAttribute(&Sea,"Phonk.AirDefenseMax")) Sea.Phonk.AirDefenseMax = 8;
		if (sti(Sea.Phonk.AirDefenseMax) < 1) Sea.Phonk.AirDefenseMax = 1;
		if (sti(Sea.Phonk.AirDefenseMax) > 32) Sea.Phonk.AirDefenseMax = 32;
		if (!CheckAttribute(&Sea,"Phonk.AirDefensePerTarget")) Sea.Phonk.AirDefensePerTarget = 2;
		Sea.Phonk.AirDefensePerTarget = SeaPhonk_DefensePerTarget(sti(Sea.Phonk.AirDefensePerTarget));
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.Custom")) pchar.SeaPhonk.Spawn.Custom = 0;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseCustom")) pchar.SeaPhonk.Spawn.DefenseCustom = sti(pchar.SeaPhonk.Spawn.Custom);
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.ShipClass")) pchar.SeaPhonk.Spawn.ShipClass = 0;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.MissileType")) pchar.SeaPhonk.Spawn.MissileType = 3;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.Warhead")) pchar.SeaPhonk.Spawn.Warhead = 5;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.Volley")) pchar.SeaPhonk.Spawn.Volley = 3;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.FireInterval")) pchar.SeaPhonk.Spawn.FireInterval = 5.0;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseEnabled")) pchar.SeaPhonk.Spawn.DefenseEnabled = 1;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.AirDefenseType")) pchar.SeaPhonk.Spawn.AirDefenseType = 2;
		if (sti(pchar.SeaPhonk.Spawn.AirDefenseType) < 0 || sti(pchar.SeaPhonk.Spawn.AirDefenseType) > 2) pchar.SeaPhonk.Spawn.AirDefenseType = 2;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseRange")) pchar.SeaPhonk.Spawn.DefenseRange = 1600;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseInterval")) pchar.SeaPhonk.Spawn.DefenseInterval = 0.3;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseMax")) pchar.SeaPhonk.Spawn.DefenseMax = 4;
		if (!CheckAttribute(pchar,"SeaPhonk.Spawn.DefenseSpeed")) pchar.SeaPhonk.Spawn.DefenseSpeed = 600;
		PhonkMissileSection = 0;
		SetFormatedText("TITLE","Ракеты");
		for (int i = 0; i < 8; i++) { SetNodeUsing("CATEGORY" + i,i < 5); SetSelectable("CATEGORY" + i,i < 5); SetNodeUsing("SELECTED" + i,false); }
		for (i = 0; i < 12; i++) { SetNodeUsing("ACTION" + i,true); SetSelectable("ACTION" + i,true); }
		SetNodeUsing("TELEPORT_EDIT",false); SetNodeUsing("V_TELEPORT_EDIT",false); SetNodeUsing("TELEPORT_BTN",false);
		SetNodeUsing("DEBUGER_INFO",true); SetNodeUsing("DEBUGER_INFO_2",false); SetNodeUsing("DEBUGER_SCROLL",false);
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CLOSE",0,"#Вернуться в игру");
		SetEventHandler("InterfaceBreak","ProcessCancelExit",0);
		SetEventHandler("exitCancel","ProcessCancelExit",0);
		SetEventHandler("evntDoPostExit","DoPostExit",0);
		SetEventHandler("ievnt_command","ProcCommand",0);
		SetEventHandler("eSlideChange","PhonkMissile_Slide",0);
		SetEventHandler("OnTableClick","PhonkMissile_TableClick",0);
		SetEventHandler("TableActivate","PhonkMissile_TableActivate",0);
		PhonkMissile_Refresh();
		SetCurrentNode("CATEGORY0");
		return;
	}
	SetFormatedText("TITLE","Дебаг-меню");
	for (int actionSlot = 0; actionSlot < 13; actionSlot++) { SetNodeUsing("ACTION" + actionSlot,false); SetSelectable("ACTION" + actionSlot,false); }
	// TEXTBUTTON's INI string is a localization key; #literal is supported by message 0.
	for (int category = 0; category < 7; category++)
	{
		SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CATEGORY" + category,0,"#" + DebugMenu_GroupName(category));
	}
	SetNodeUsing("CATEGORY7",false);
	SetSelectable("CATEGORY7",false);
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"CLOSE",0,"#Вернуться в игру");
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"TELEPORT_BTN",0,"#Телепорт");

	CalculateInfoData();

	CalculateCheatsInfo(); // Warship. Статистика - сколько читов юзали

	DebugMenu_SelectGroup(0);

	SetEventHandler("InterfaceBreak","ProcessCancelExit",0); 
	SetEventHandler("exitCancel","ProcessCancelExit",0); 
	SetEventHandler("evntDoPostExit","DoPostExit",0); 
	SetEventHandler("ievnt_command","ProcCommand",0);
	//SetEventHandler("SetScrollerPos","SetScrollerPos",0);

    GameInterface.teleport_edit.str = "Pirates_Shipyard, reload, reload1";
	DebugMenu_FirstAction();
}

void ProcessCancelExit()
{
	IDoExit( RC_INTERFACE_QUICK_SAVE );
}

void IDoExit(int exitCode)
{
    if (PhonkMissileSettings && IsEntity(&Sea)) DeleteAttribute(&Sea,"Phonk.MissileSettings");
    EndAboveForm(true);
	
	DelEventHandler("InterfaceBreak","ProcessCancelExit");
	DelEventHandler("exitCancel","ProcessCancelExit");
	DelEventHandler("evntDoPostExit","DoPostExit");
	DelEventHandler("ievnt_command","ProcCommand");
	if (PhonkMissileSettings) {
		DelEventHandler("eSlideChange","PhonkMissile_Slide");
		DelEventHandler("OnTableClick","PhonkMissile_TableClick");
		DelEventHandler("TableActivate","PhonkMissile_TableActivate");
	}
	PhonkMissileSettings = false;
 	//DelEventHandler("SetScrollerPos","SetScrollerPos");

	if(bSeaActive)
    {
        RefreshBattleInterface();
    }
	
	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}


void DoPostExit()
{
	int exitCode = GetEventData();
	IDoExit(exitCode);
}

void CalculateInfoData()
{
	DeleteAttribute(&DebugMenu,"");
	DebugMenuCount = 0;
	DebugMenu_Add(0,"TSUNAMI_START","Цунами");
	DebugMenu_Add(0,"WHIRLPOOL","Водоворот");
	DebugMenu_Add(0,"TSUNAMI_RING","Цунами: кольцо");
	DebugMenu_Add(0,"WEATHER_STORM","Шторм (10 мин)");
	DebugMenu_Add(0,"WEATHER_NEXT","Сменить погоду");
	DebugMenu_Add(0,"PHONK","Фонк");
	DebugMenu_Add(0,"B_F10","Бессмертие героя и корабля");
	DebugMenu_Add(0,"PHONK_MUSIC","Фонк-музыка");
	DebugMenu_Add(0,"CANNON_CHEATS","Боезапас и пушки без перезарядки");
	DebugMenu_Add(0,"B_F14","Дробовик");
	DebugMenu_Add(0,"ENEMY_ALL","Враг всем");
	DebugMenu_Add(0,"INVISIBLE_AI","Невидимка");
	DebugMenu_Add(1,"B_F1","Золото +200 000");
	DebugMenu_Add(1,"B_F5","Очки навыков +35");
	DebugMenu_Add(1,"B_F7","Все навыки — 100");
	DebugMenu_Add(1,"ALL_PERKS","Все обычные перки");
	DebugMenu_Add(1,"HEAL","Здоровье и энергия");
	DebugMenu_Add(1,"SUPPLIES","Лекарства и патроны");
	DebugMenu_Add(1,"SURVIVAL_CHEATS","Бессмертие и энергия");
	DebugMenu_Add(1,"B_F11","Репутация +10");
	DebugMenu_Add(1,"B_F27","Репутация -10");
	DebugMenu_Add(1,"B_F19","Шкиперу: навыки +35");
	DebugMenu_Add(1,"B_F24","Лицензия на 30 дней");
	DebugMenu_Add(2,"B_F6","Ранг 20 и «Арабелла»");
	DebugMenu_Add(2,"B_F31","Ранг 35 и «Сантисима»");
	DebugMenu_Add(2,"B_F32","Ранг 10 и бриг");
	DebugMenu_Add(3,"BTN_SPECIAL1","Без еды и крыс");
	DebugMenu_Add(3,"BTN_SPECIAL3","Капитаны сдаются");
	DebugMenu_Add(3,"B_F4","Опыт команды +10");
	DebugMenu_Add(3,"B_F40","Следующая расцветка");
	DebugMenu_Add(3,"B_F33","Заменить корабль: класс 7");
	DebugMenu_Add(3,"B_F34","Заменить корабль: класс 6");
	DebugMenu_Add(3,"B_F35","Заменить корабль: класс 5");
	DebugMenu_Add(3,"B_F36","Заменить корабль: класс 4");
	DebugMenu_Add(3,"B_F37","Заменить корабль: класс 3");
	DebugMenu_Add(3,"B_F38","Заменить корабль: класс 2");
	DebugMenu_Add(3,"B_F39","Заменить корабль: класс 1");
	DebugMenu_Add(3,"B_F41","Обновить корабли");
	DebugMenu_Add(4,"BTN_SPECIAL8","Пауза живого мира");
	DebugMenu_Add(4,"B_F8","Следующая таверна");
	DebugMenu_Add(4,"B_F9","Ветер 16,5");
	DebugMenu_Add(4,"B_F12","Награда за голову +50");
	DebugMenu_Add(4,"B_F13","Встречи на карте");
	DebugMenu_Add(4,"B_F15","Отношения наций");
	DebugMenu_Add(4,"B_F16","Пропустить 5 дней");
	DebugMenu_Add(4,"B_F30","Пропустить 20 дней");
	DebugMenu_Add(4,"B_F20","Обновить свет маяков");
	DebugMenu_Add(4,"B_F25","Перезагрузить локацию");
	DebugMenu_Add(4,"B_F28","Летучий Голландец");
	DebugMenu_Add(4,"B_F29","Награда за голову -50");
	DebugMenu_Add(5,"B_F2","Запись об идоле");
	DebugMenu_Add(5,"B_F17","Список квестов героя");
	DebugMenu_Add(5,"B_F22","Аскольд и «Синяя птица»");
	DebugMenu_Add(6,"BTN_SPECIAL4","Локаторы");
	DebugMenu_Add(6,"BTN_SPECIAL7","Свободная камера");
	DebugMenu_Add(6,"B_F3","Локация и координаты");
	DebugMenu_Add(6,"B_F18","Скрыть / показать UI");
	DebugMenu_Add(6,"B_F21","Камера рядом с героем");
	DebugMenu_Add(6,"B_F23","Данные камеры");
	DebugMenu_Add(7,"BTN_SPECIAL2","Логи квестов");
	DebugMenu_Add(7,"BTN_SPECIAL5","Логи опыта");
	DebugMenu_Add(7,"BTN_SPECIAL6","Логи капитанов");
	DebugMenu_Add(7,"BTN_SPECIAL9","Бета-тест");
	DebugMenu_Add(7,"B_F26","Данные героя и корабля");
	DebugMenu_Add(7,"B_F42","Обновить локации");
	DebugMenu_Add(7,"DEBUG_STATS","Статистика читов");
	DebugMenu_Add(7,"AUTOPILOT_TRADE","Автопилот: торговля");
	DebugMenu_Add(7,"AUTOPILOT_HUNT","Автопилот: охота");
	DebugMenu_Add(7,"AUTOPILOT_OFF","Ручное управление");
}

void ProcCommand()
{
	string comName = GetEventData();
	string nodName = GetEventData();
	if (PhonkMissileSettings)
	{
		if (nodName == "CLOSE" && (comName == "activate" || comName == "click")) { ProcessCancelExit(); return; }
		if (nodName == "CLOSE" && comName == "upstep") { SetCurrentNode("CATEGORY4"); return; }
		if (nodName == "CATEGORY4" && comName == "downstep") { SetCurrentNode("CLOSE"); return; }
		for (int missileCategory = 0; missileCategory < 5; missileCategory++)
		{
			if (nodName != "CATEGORY" + missileCategory) continue;
			if (comName == "activate" || comName == "click" || comName == "rightstep")
			{
				PhonkMissile_Command(nodName);
				if (comName == "rightstep") { if (PhonkMissileSection == 0) SetCurrentNode("PHONK_MISSILES"); else SetCurrentNode("ACTION0"); }
			}
			return;
		}
		if (PhonkMissileSection == 0 && nodName == "PHONK_MISSILES")
		{
			if (comName == "leftstep") SetCurrentNode("CATEGORY0");
			if (comName == "rightstep") SetCurrentNode("PHONK_CONTROL0");
			return;
		}
		if (PhonkMissileSection == 0)
		{
			for (int control = 0; control < 8; control++)
			{
				if (nodName != "PHONK_CONTROL" + control || !GetSelectable(nodName)) continue;
				if (comName == "activate" || comName == "click")
				{
					string action = "ACTION2";
					if (control == 1) action = "ACTION3"; if (control == 2) action = "ACTION4";
					if (control >= 3 && control <= 6) action = "ACTION" + (control + 3);
					if (control == 7) action = "ACTION10";
					PhonkMissile_Command(action);
					return;
				}
				if (comName == "leftstep") { if (control == 2 || control == 4 || control == 6) SetCurrentNode("PHONK_CONTROL" + (control - 1)); else SetCurrentNode("PHONK_MISSILES"); return; }
				if (comName == "rightstep" && (control == 1 || control == 3 || control == 5)) { SetCurrentNode("PHONK_CONTROL" + (control + 1)); return; }
				int next = control;
				if (comName == "downstep") { if (control == 0) next = 1; else if (control < 5) next = control + 2; else next = 7; }
				if (comName == "upstep") { if (control == 7) next = 5; else if (control <= 2) next = 0; else next = control - 2; }
				if (next >= 3 && next <= 6 && !GetSelectable("PHONK_CONTROL" + next)) { if (comName == "downstep") next = 7; else next = 1; }
				if (next != control) SetCurrentNode("PHONK_CONTROL" + next);
				return;
			}
		}
		for (int missileSlot = 0; missileSlot < 13; missileSlot++)
		{
			if (nodName != "ACTION" + missileSlot) continue;
			if (!GetSelectable(nodName)) return;
			if (comName == "activate" || comName == "click") { PhonkMissile_Command(nodName); return; }
			if (PhonkMissileSection == 2 && missileSlot == 1 && comName == "downstep") { SetCurrentNode("ACTION8"); return; }
			if (PhonkMissileSection == 2 && missileSlot == 8 && comName == "upstep") { SetCurrentNode("ACTION1"); return; }
			if (PhonkMissileSection == 4 && (missileSlot == 0 || missileSlot == 1) && comName == "downstep") { SetCurrentNode("ACTION8"); return; }
			if (PhonkMissileSection == 4 && missileSlot == 8 && comName == "upstep") { SetCurrentNode("ACTION0"); return; }
			if ((PhonkMissileSection == 2 || PhonkMissileSection == 4) && missileSlot == 9 && comName == "downstep") { SetCurrentNode("ACTION10"); return; }
			if (comName == "leftstep") { if (missileSlot % 2 == 0) SetCurrentNode("CATEGORY" + PhonkMissileSection); else SetCurrentNode("ACTION" + (missileSlot - 1)); }
			if (comName == "rightstep" && missileSlot < 12 && missileSlot % 2 == 0 && GetSelectable("ACTION" + (missileSlot + 1))) SetCurrentNode("ACTION" + (missileSlot + 1));
			if (comName == "upstep" && missileSlot >= 2) SetCurrentNode("ACTION" + (missileSlot - 2));
			if (comName == "downstep" && missileSlot < 11 && GetSelectable("ACTION" + (missileSlot + 2))) SetCurrentNode("ACTION" + (missileSlot + 2));
			return;
		}
		return;
	}
	if (nodName == "CLOSE" && comName == "upstep") { SetCurrentNode("CATEGORY6"); return; }
	if (nodName == "CATEGORY6" && comName == "downstep") { SetCurrentNode("CLOSE"); return; }
	if (nodName == "CATEGORY2" && comName == "downstep") { SetCurrentNode("CATEGORY3"); return; }
	for (int category = 0; category < 7; category++)
	{
		if (nodName != "CATEGORY" + category) continue;
		if (comName == "activate" || comName == "click") DebugMenu_SelectGroup(category);
		if (comName == "rightstep") { DebugMenu_SelectGroup(category); DebugMenu_FirstAction(); }
		return;
	}
	if (nodName == "CLOSE" && (comName == "activate" || comName == "click")) { ProcessCancelExit(); return; }
	if ((nodName == "DEBUGER_INFO" || nodName == "DEBUGER_INFO_2") && comName == "leftstep") { SetCurrentNode("CATEGORY" + DebugMenuGroup); return; }
	if (nodName == "DEBUG_ACTIONS")
	{
		if (comName == "leftstep") { SetCurrentNode("CATEGORY" + DebugMenuGroup); return; }
		if (comName != "activate" && comName != "dblclick") return;
		string row = "tr" + sti(GameInterface.DEBUG_ACTIONS.select);
		if (!CheckAttribute(&DebugMenu,"slots." + row)) return;
		nodName = DebugMenu.slots.(row);
		if (!DebugMenu_Enabled(nodName)) return;
		SetNodeUsing("DEBUGER_INFO",true);
		SetNodeUsing("DEBUGER_SCROLL",true);
		SetNodeUsing("DEBUGER_INFO_2",false);
	}
	if (nodName == "DEBUG_STATS")
	{
		CalculateCheatsInfo();
		SetNodeUsing("DEBUGER_INFO",false);
		SetNodeUsing("DEBUGER_SCROLL",false);
		SetNodeUsing("DEBUGER_INFO_2",true);
		SetCurrentNode("DEBUGER_INFO_2");
		return;
	}
	if (nodName == "TSUNAMI_START" || nodName == "TSUNAMI_RING" || nodName == "PHONK" || nodName == "PHONK_MUSIC" || nodName == "WHIRLPOOL" ||
		nodName == "WEATHER_STORM" || nodName == "WEATHER_NEXT" ||
		nodName == "AUTOPILOT_TRADE" || nodName == "AUTOPILOT_HUNT" || nodName == "AUTOPILOT_OFF")
	{
		DebugEffects_Run(nodName);
		return;
	}

	switch(nodName)
	{
		case "SURVIVAL_CHEATS": DebugMenu_ToggleSurvival(); break;
		case "ENEMY_ALL": pchar.DebugEnemyAll = !CheckAttribute(pchar,"DebugEnemyAll") || !sti(pchar.DebugEnemyAll); break;
		case "INVISIBLE_AI": pchar.DebugInvisibleAI = !CheckAttribute(pchar,"DebugInvisibleAI") || !sti(pchar.DebugInvisibleAI); break;
		case "CANNON_CHEATS": DebugMenu_ToggleCannons(); break;
		case "INFINITE_ENERGY": DebugMenu_ToggleCheat("InfiniteEnergy",false); break;
		case "INSTANT_CANNON_RELOAD": DebugMenu_ToggleCheat("InstantCannonReload",true); break;
		case "UNLIMITED_AMMO": DebugMenu_ToggleCheat("UnlimitedAmmo",true); break;
		case "ALL_PERKS": DebugMenu_GivePerks(); break;
		case "HEAL":
			LAi_UseAtidoteBottle(pchar);
			LAi_SetCurHPMax(pchar);
			SetFormatedText("DEBUGER_INFO","Здоровье и энергия восстановлены, отравление снято.");
			Statistic_AddValue(pchar,"Cheats.Heal",1);
		break;
		case "SUPPLIES":
			TakeNItems(pchar,"potion2",20);
			TakeNItems(pchar,"bullet",100);
			SetFormatedText("DEBUGER_INFO","Получено 20 лечебных эликсиров и 100 пуль.");
			Statistic_AddValue(pchar,"Cheats.Supplies",1);
		break;
		case "TELEPORT_BTN":
		  if(comName=="activate" || comName=="click")
		  {
              ReloadByStr();
		  }
	    break;
		
		case "BTN_SPECIAL1":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataS1();
		  }
	    break;
		
		case "BTN_SPECIAL2":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataS2();
		  }
	    break;
		
		case "BTN_SPECIAL3":
		  if(comName=="activate" || comName=="click")
		  {
		     CalculateInfoDataS3();
		  }
	    break;
		
		case "BTN_SPECIAL4":
		  if(comName=="activate" || comName=="click")
		  {
		     CalculateInfoDataS4();
		  }
	    break;
		
		case "BTN_SPECIAL5":
		  if(comName=="activate" || comName=="click")
		  {
		    CalculateInfoDataS5();
		  }
	    break;
		
		case "BTN_SPECIAL6":
		  if(comName=="activate" || comName=="click")
		  {
		     CalculateInfoDataS6();
		  }
	    break;
		
		case "BTN_SPECIAL7":
		  if(comName=="activate" || comName=="click")
		  {
		     CalculateInfoDataS7();
		  }
	    break;
		
		case "BTN_SPECIAL8":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataS8();
		  }
	    break;
		
		case "BTN_SPECIAL9":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataS9();
		  }
	    break;
		
	    case "B_F1":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF1();
		  }
	    break;
		
	    case "B_F2":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF2();
		  }
	    break;
		
	    case "B_F3":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF3();
		  }
	    break;
		
	    case "B_F4":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF4();
		  }
	    break;
		
	    case "B_F5":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF5();
		  }
	    break;

	    case "B_F6":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF6();
		  }
	    break;

	    case "B_F7":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF7();
		  }
	    break;

	    case "B_F8":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF8();
		  }
	    break;

	    case "B_F9":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF9();
		  }
	    break;

	    case "B_F10":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF10();
		  }
	    break;

	    case "B_F11":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF11();
		  }
	    break;

	    case "B_F12":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF12();
		  }
	    break;

	    case "B_F13":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF13();
		  }
	    break;

	    case "B_F14":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF14();
		  }
	    break;

	    case "B_F15":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF15();
		  }
	    break;

	    case "B_F16":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF16();
		  }
	    break;

	    case "B_F17":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF17();
		  }
	    break;

	    case "B_F18":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF18();
		  }
	    break;
	    
	    case "B_F19":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF19();
		  }
	    break;

        case "B_F20":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF20();
		  }
	    break;
	    
	    case "B_F21":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF21();
		  }
	    break;
		
	    case "B_F22":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF22();
		  }
	    break;
		
	    case "B_F23":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF23();
		  }
	    break;
		
	    case "B_F24":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF24();
		  }
	    break;
		
	    case "B_F25":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF25();
		  }
	    break;
		
	    case "B_F26":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF26();
		  }
	    break;
		
	    case "B_F27":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF27();
		  }
	    break;
		
	    case "B_F28":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF28();
		  }
	    break;
		
	    case "B_F29":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF29();
		  }
	    break;
		
	    case "B_F30":
		  if(comName=="activate" || comName=="click")
		  {
		      CalculateInfoDataF30();
		  }
	    break;
		
	    case "B_F31":
		if(comName=="activate" || comName=="click")
		{
			CalculateInfoDataF31();
		}
	    break;
		
	    case "B_F32":
		if(comName=="activate" || comName=="click")
		{
			CalculateInfoDataF32();
		}
	    break;

		case "B_F33":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF33();
			}
			break;

		case "B_F34":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF34();
			}
			break;

		case "B_F35":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF35();
			}
			break;

		case "B_F36":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF36();
			}
			break;

		case "B_F37":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF37();
			}
			break;

		case "B_F38":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF38();
			}
			break;
		case "B_F39":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF39();
			}
			break;
		case "B_F40":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF40();
			}
			break;
		case "B_F41":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF41();
			}
			break;
		case "B_F42":
			if (comName == "activate" || comName == "click")
			{
				CalculateInfoDataF42();
			}
			break;
	}
	
	// These original handlers close and unload this segment themselves.
	if (nodName == "B_F2" || nodName == "B_F15" || nodName == "B_F25") return;
	CalculateCheatsInfo(); // Статистика по читам
	DebugMenu_Refresh();
}

string descS1 = "Не потреблять провизию и не получать ущерб от крыс";
void CalculateInfoDataS1()
{
	totalInfo = descS1;
	bNoEatNoRats = !bNoEatNoRats;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S1", 1);
}

string descS2 = "Логирование квестов";
void CalculateInfoDataS2()
{
	totalInfo = descS2;
	bQuestLogShow = !bQuestLogShow;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S2", 1);
}

string descS3 = "Капитаны всегда сдаются";
void CalculateInfoDataS3()
{
	totalInfo = descS3;
	TestRansackCaptain = !TestRansackCaptain;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S3", 1);
}

string descS4 = "Показывать локаторы";
void CalculateInfoDataS4()
{
	totalInfo = descS4;
	bLocatorShow = !bLocatorShow;
	if(bLocatorShow)
	{
		ShowAllLocators(); 
		worldMap.debug = "true"; 
		worldMap.evwin = "true";
	}
	else
	{
		HideAllLocators();
		worldMap.debug = "false";
		worldMap.evwin = "false";
	}
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);

	Statistic_AddValue(PChar, "Cheats.S4", 1);
}

string descS5 = "Логирование опыта";
void CalculateInfoDataS5()
{
	totalInfo = descS5;
	bExpLogShow = !bExpLogShow;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S5", 1);
}

string descS6 = "Логирование капитанов (ПГГ)";
void CalculateInfoDataS6()
{
	totalInfo = descS6;
	bPGGLogShow = !bPGGLogShow;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S6", 1);
}

string descS7 = "Свободная камера (должны быть включены дебаг клавиши в engine.ini!)";
void CalculateInfoDataS7()
{
	totalInfo = descS7;
	locCameraEnableFree = !locCameraEnableFree;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S7", 1);
}

string descS8 = "Живой мир - снять ограничения для наций";
void CalculateInfoDataS8()
{
	totalInfo = descS8;
	bWorldAlivePause = !bWorldAlivePause;
	
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S8", 1);
}

string descS9 = "Режим Бетатеста";
void CalculateInfoDataS9()
{
	totalInfo = descS9;
	bBettaTestMode = !bBettaTestMode;
		
	totalInfo = totalInfo + NewStr();
	totalInfo = totalInfo + "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
	
	Statistic_AddValue(PChar, "Cheats.S9", 1);
}

string descF1 = "Золото + 200 000";
void CalculateInfoDataF1()
{
	// тут высчитываем нужную информацию и выводим в totalInfo - Инициализация -->
	totalInfo = descF1;
	Pchar.money = sti(Pchar.money) + 200000;
	/*Pchar.quest.SetTreasureHunter.win_condition.l1          = "location";
                Pchar.quest.SetTreasureHunter.win_condition.l1.location = Pchar.location.from_sea;
                Pchar.quest.SetTreasureHunter.win_condition             = "SetTreasureHunter";
      */
	//Characters[GetCharacterIndex("Luisa Drake")].perks.list.Ciras = "1";
	totalInfo = totalInfo + GetAssembledString(" Денег у #sName#а теперь #dmoney#",	Pchar);
    // тут высчитываем нужную информацию и выводим в totalInfo <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);

	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F1", 1);
}

string descF2 = "Добавить запись №10 о проклятом идоле";
void CalculateInfoDataF2()
{
	totalInfo = descF2;
			pchar.questTemp.PDM_PI_Rostov = "PDM_PI_Rostov";

			AddQuestRecord("PDM_Cursed_Idol", "10");
			AddQuestUserData("PDM_Cursed_Idol", "sSex", GetSexPhrase("лся","лась"));
	//GiveItem2Character(pchar, "icollection");
	/*pchar.questTemp.Tichingitu = "true";
	AddDialogExitQuestFunction("SetTichingituJail");
	SetFunctionTimerCondition("FreeTichingituOver", 0, 0, 10, false);*/
    
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);

    ProcessCancelExit();
    
    // Статистика по читам
    Statistic_AddValue(PChar, "Cheats.F2", 1);
}
////////////////////////////////////////////////////////////////////////

string descF3 = "ID текущей локации +коорд     ";
void CalculateInfoDataF3()
{
	// -->
	totalInfo = descF3;
	ref mc;
	mc = GetMainCharacter();

	float locx, locy, locz;
	if (bSeaActive && !bAbordageStarted)
	{
	    if (CheckAttribute(mc, "Ship.pos.x"))
		{
		    totalInfo = totalInfo + NewStr() + mc.location + "     ship(x,z)= "+mc.Ship.pos.x +", "+mc.Ship.pos.z);
			totalInfo = totalInfo + NewStr() + " Sea_Coord.X " + Sea_GetRealCoordX(makefloat(mc.Ship.pos.x)) + " Sea_Coord.Z " + Sea_GetRealCoordZ(makefloat(mc.Ship.pos.z));
			
		    if (false)//CheckAttribute(mc, "WMShip.Pos.x") && worldMap.island != "")
	        {
	            string sTemp = mc.curIslandId;
	            float r1 = stf(worldMap.islands.(sTemp).GoMapRadius);

				r1 *= WDM_MAP_TO_SEA_SCALE;
	            float d2 = GetDistance2D(stf(mc.Ship.Pos.x),
	                              stf(mc.Ship.Pos.z),
	                              stf(worldMap.islands.(sTemp).position.rx),
	                              stf(worldMap.islands.(sTemp).position.rz));

	            totalInfo = totalInfo + "         Координаты для карты radius= " + r1 + "   GetDistance2D= " + d2 + "      ";
	        }
		}
	}
	else
	{
  		if (IsEntity(loadedLocation))
  		{
			GetCharacterPos(GetMainCharacter(), &locx, &locy, &locz);
			totalInfo = totalInfo + "id = " + mc.location + " (x,y,z)= "+locx + ", " +locy + ", "+locz;
			totalInfo = totalInfo + NewStr() + "filespath.models = " + loadedLocation.filespath.models;
			totalInfo = totalInfo + NewStr() + "image = " + loadedLocation.image;
			totalInfo = totalInfo + NewStr() + "models.locators = " + loadedLocation.models.always.locators;
			
			if(CheckAttribute(LoadedLocation, "islandId"))
			{
				totalInfo = totalInfo + NewStr() + "islandId = " + LoadedLocation.islandId;
			}
			
			if(CheckAttribute(LoadedLocation, "townsack"))
			{
				totalInfo = totalInfo + NewStr() + "townsack = " + LoadedLocation.townsack;
			}
		}
	}
/*	
	aref rootItems;
    makearef(rootItems, worldMap);  //Islands[0]
	DumpAttributes(rootItems);
*/	
	totalInfo = totalInfo + NewStr() + " MapShipX " +	worldMap.playerShipX + " MapShipZ " + worldMap.playerShipZ + " X " + worldMap.island.x + " Z " + worldMap.island.z;
	
	totalInfo = totalInfo + NewStr() + " Map_Coord.X " + Map_GetRealCoordX(makefloat(worldMap.playerShipX)) + " Map_Coord.Z " + Map_GetRealCoordZ(makefloat(worldMap.playerShipZ));
	
	totalInfo = totalInfo + NewStr() + " Zero_Point_X " + makefloat(worldMap.zeroX) + " Zero_Point_Z " + makefloat(worldMap.zeroZ);
    // <
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	GetRealCoordsObjects();
	
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F3", 1);
}
////////////////////////////////////////////////////////////////////////

string descF4 = "Опыт команды +10";
void CalculateInfoDataF4()
{
	// -->
	totalInfo = descF4;
	//locCameraLock(3);    Камеру зафиксировать на угол (мордой вперёд)
	///mc.model.animation = "man";
	//mc.model = "napitan";
	//SetNewModelToChar(mc);
    ChangeCrewExp(pchar, "Sailors", 10);
	ChangeCrewExp(pchar, "Cannoners", 10);
	ChangeCrewExp(pchar, "Soldiers", 10);

	//float mhp = LAi_GetCharacterMaxHP(mc) + 5;
	//LAi_SetHP(mc,mhp,mhp);
	//totalInfo = totalInfo+ " iNumShips="+iNumShips+" iNumFantoms="+iNumFantoms;
	//ddCharacterExp(mc, 3000);
    // <
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F4", 1);
}

////////////////////////////////////////////////////////////////////////
string descF5 = "Добавить 35 свободных очков навыков";
int BOAL_debug_num = 1;
void CalculateInfoDataF5()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF5;
    /*  pchar.PatentNation = "eng";
      GiveItem2Character(pchar, "patent_" + pchar.PatentNation);
      EquipCharacterbyItem(pchar, "patent_" + pchar.PatentNation);
      */
    pchar.Skill.FreeSkill = sti(pchar.Skill.FreeSkill) + 35;
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    	
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F5", 1);
}
////////////////////////////////////////////////////////////////////////
string descF6 = "Миллион, СЛ с 32-ым калибром, 20-ый ранг";

void CalculateInfoDataF6()
{
	ref mc;
	string sEquipItem;
	totalInfo = descF6;
	mc = GetMainCharacter();
	SetRandSelfSkill(mc, 50, 85);
	SetRandShipSkill(mc, 50, 85);
	
//	SetRandSelfSkill(mc, 100, 100);
//	SetRandShipSkill(mc, 100, 100);
	mc.rank = 20;
	LAi_SetHP(mc, 250.0, 250.0);
//	mc.Ship.Type = GenerateShipExt(SHIP_SANTISIMA, true, mc);
	mc.Ship.Type = GenerateShipExt(SHIP_ARABELLA, true, mc);
	SetBaseShipData(mc);
	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS32;
	SetCrewQuantityFull(mc);
	AddCrewMorale(mc, 80);
	ChangeCrewExp(mc, "Sailors", 50);
	ChangeCrewExp(mc, "Cannoners", 50);
	ChangeCrewExp(mc, "Soldiers", 50);

	SetCharacterGoods(mc, GOOD_BALLS, 1700);
	SetCharacterGoods(mc, GOOD_GRAPES, 1700);
	SetCharacterGoods(mc, GOOD_KNIPPELS, 1700);
	SetCharacterGoods(mc, GOOD_BOMBS, 1700);
	SetCharacterGoods(mc, GOOD_FOOD, 1000);
	SetCharacterGoods(mc, GOOD_POWDER, 3000);
	SetCharacterGoods(mc, GOOD_WEAPON, 600); 
	SetCharacterGoods(mc, GOOD_MEDICAMENT, 600);
	
	sEquipItem = GetCharacterEquipByGroup(mc, BLADE_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, GUN_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, SPYGLASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, CIRASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);

	AddMoneyToCharacter(mc, 1000000);
	AddItems(mc, "spyglass3", 1);
	AddItems(mc, "pistol5", 1);
	
	sEquipItem = GetGeneratedItem("blade27");
	AddItems(mc, sEquipItem, 1);
	
	AddItems(mc, "bullet", 50);
	AddItems(mc, "GunPowder", 50);
	AddItems(mc, "cirass3", 1);

	EquipCharacterbyItem(mc, "spyglass3");
	EquipCharacterbyItem(mc, "pistol5");
 	EquipCharacterbyItem(mc, sEquipItem);
 	EquipCharacterbyItem(mc, "cirass3");
 	
 	SetHalfPerksToChar(mc, false);
 	
	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	
	SetFormatedText("DEBUGER_INFO", totalInfo);
	
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F6", 1);
/*
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF6;
	ref mc = GetMainCharacter();
    mc.Ship.Type = GenerateShipExt(SHIP_FLYINGDUTCHMAN, true, mc);
    SetBaseShipData(mc);
    mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS24;
    SetCrewQuantityFull(mc);

    SetCharacterGoods(mc,GOOD_FOOD,2000);
	SetCharacterGoods(mc,GOOD_BALLS,2000);//2000);
    SetCharacterGoods(mc,GOOD_GRAPES,300);//700);
    SetCharacterGoods(mc,GOOD_KNIPPELS,400);//700);
    SetCharacterGoods(mc,GOOD_BOMBS,2000);//1500);
    SetCharacterGoods(mc,GOOD_POWDER,2000);
    SetCharacterGoods(mc,GOOD_PLANKS,10);
    SetCharacterGoods(mc,GOOD_RUM,40);//600);
    SetCharacterGoods(mc,GOOD_WEAPON,2000);//2000);

    if (mc.location.from_sea == "")
    {
        //setCharacterShipLocation(mc, mc.HeroParam.Location);
    }

	GiveItem2Character(mc,"spyglass5");
	GiveItem2Character(mc,"pistol7");
	aref arItm;
	Items_FindItem("pistol7",&arItm);
	arItm.chargespeed = 3;
	arItm.dmg_min = 1600.0;
	arItm.dmg_max = 2650.0;

	GiveItem2Character(mc,"blade15");
	//GiveItem2Character(mc,"indian11");
	//GiveItem2Character(mc,"cirass5");
	TakeNItems(mc,"bullet", 180);
	mc.perks.list.Gunman = "1";
	mc.perks.list.BasicDefense = "1";
	mc.perks.list.AdvancedDefense = "1";
	mc.perks.list.Ciras = "1";
	mc.perks.list.FlagPir = "1";
	mc.perks.list.FlagEng = "1";
	mc.perks.list.FlagHol = "1";
	mc.perks.list.FlagSpa = "1";
	mc.perks.list.FlagFra = "1";
	//mc.reputation = "89";

	EquipCharacterbyItem(mc, "spyglass5");
	EquipCharacterbyItem(mc, "pistol7");
 	EquipCharacterbyItem(mc, "blade32");
 	//EquipCharacterbyItem(mc, "map_good");
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);

	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F6", 1);
*/	
}
////////////////////////////////////////////////////////////////////////
string descF7 = "Поднять все личные и корабельные навыки до 100";

void CalculateInfoDataF7()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF7;

    /*pchar.questTemp.Waiting_time = "0";
    pchar.questTemp.State = "empty";
    SaveCurrentQuestDateParam("questTemp");
    if (sti(pchar.questTemp.CurQuestNumber)<12)
    {
        pchar.questTemp.CurQuestNumber = sti(pchar.questTemp.CurQuestNumber)+1;
    }
    else
    {
        pchar.questTemp.CurQuestNumber = "2";
    }
    Log_SetStringToLog("Вы можете получить у Мэдифорда квест " + pchar.questTemp.CurQuestNumber + ".");
    */
    /*int i;

	for (i=0; i<MAX_COLONIES; i++)
	{
		SetPriceListByStoreMan(&Colonies[i]);
	}
    // <*/
    
	SetSelfSkill(PChar, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX);
	SetShipSkill(PChar, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX, SKILL_MAX);
    
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F7", 1);
}
////////////////////////////////////////////////////////////////////////
string descF8 = "телепорт по тавернам";

void CalculateInfoDataF8()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF8;
	ref mc, ch;
	int n, idx;
	mc = GetMainCharacter();
    idx = GetCharIDXByParam("TavernMan", "location", mc.location);
    bool ok = true;
    while (ok)
    {
        for(n=0; n<MAX_CHARACTERS; n++)
    	{
    		makeref(ch,Characters[n]);
    		if (CheckAttribute(ch, "TavernMan"))
    		{
                if (ch.location == "none") continue; // фикс для новых, невидимых до поры островов
                if (n > idx)
                {
                    ok = false;
                    if (GetCityFrom_Sea(ch.City) != "")
                    {
                        setCharacterShipLocation(mc, GetCityFrom_Sea(ch.City));
                        setWDMPointXZ(GetCityFrom_Sea(ch.City));
                    }
                    DoQuestReloadToLocation(ch.location,"reload","reload1", "");
                    Log_Info(ch.location);
                    break;
                }
    		}
        }
        idx = -1;
    }
	// <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F8", 1);
}
////////////////////////////////////////////////////////////////////////
string descF9 = "Ветер == 16.5"//"DumpAttributes(&Weather)";//"Репутация в минус";
void CalculateInfoDataF9()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF9;
/*	
    aref	aCurWeather = GetCurrentWeather();
    trace("----------------------GetCurrentWeather()------------------");
    DumpAttributes(aCurWeather);
    trace("--------------------DumpAttributes(&Weather)------------------");
    DumpAttributes(&Weather)
    trace("--------------------     end dump       ------------------");      
*/
    Weather.Wind.Speed = 16.5;
	pchar.wind.speed = Weather.Wind.Speed;
	fWeatherSpeed = stf(Weather.Wind.Speed);
	
    /*
    ref mc;
	mc = GetMainCharacter();
	if ( sti(mc.reputation) > REPUTATION_MIN)
	{
	   mc.reputation = sti(mc.reputation) -10;
	   if (sti(mc.reputation) < REPUTATION_MIN)
	   {
	       mc.reputation = REPUTATION_MIN;
	   }
	}
	*/
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F9", 1);
}
////////////////////////////////////////////////////////////////////////
string descF10 = "Бессмертие вкл/выкл";

void CalculateInfoDataF10()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF10;
	ref mc;
	mc = GetMainCharacter();
			if(LAi_IsImmortal(mc))
			{
				LAi_SetImmortal(mc, false);
				Log_SetStringToLog("God mode OFF");
			}else{
				LAi_SetImmortal(mc, true);
				Log_SetStringToLog("God mode ON");
			}
    // <--
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F10", 1);
}
////////////////////////////////////////////////////////////////////////
string descF11 = "Репутация в плюс";

void CalculateInfoDataF11()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");

	totalInfo = descF11;
	// -->

    int i, cn;
    ref mc;
	mc = GetMainCharacter();

    if ( sti(mc.reputation) < REPUTATION_MAX)
	{
	   mc.reputation = sti(mc.reputation) + 10;
	   if (sti(mc.reputation) > REPUTATION_MAX)
	   {
	       mc.reputation = REPUTATION_MAX;
	   }
	}
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F11", 1);
}
////////////////////////////////////////////////////////////////////////

string descF12 = "НЗГ у всех наций +50";//Заменить текущий корабль на ЧА";  //"LaunchMunityCapture";
void CalculateInfoDataF12()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF12;
    for (int j=0; j< MAX_NATIONS; j++)
	{
        ChangeCharacterNationReputation(pchar, j, -50);
    }
/*	PChar.Ship.Type = SHIP_CYCLOPUS;
	SetBaseShipData(PChar);
	PChar.Ship.Crew.Quantity = 1400;
	PChar.Ship.Cannons.Type          = CANNON_TYPE_CULVERINE_LBS24;

	SetCharacterGoods(PChar,GOOD_BALLS,2000);
    SetCharacterGoods(PChar,GOOD_GRAPES,700);
    SetCharacterGoods(PChar,GOOD_KNIPPELS,700);
    SetCharacterGoods(PChar,GOOD_BOMBS,1500);
    SetCharacterGoods(PChar,GOOD_POWDER,1500);
    SetCharacterGoods(PChar,GOOD_SAILCLOTH,5);
    SetCharacterGoods(PChar,GOOD_PLANKS,10);
    SetCharacterGoods(PChar,GOOD_FOOD,2000);
    SetCharacterGoods(PChar,GOOD_WEAPON,2000);
	PChar.SystemInfo.CabinType = "My_Cabin";          */

	//ref rNames;
	//rNames = &sFrWomenNames;
	//makeref(rNames, sRndShpEnName);
    //totalInfo = GetArraySize(rNames) + "---- " +rNames[20];

    //LAi_SetActorType(PChar);
    //LAi_ActorAnimation(PChar, "attack_left_1", "", -1);
    //DoQuestCheckDelay("pchar_back_to_player", 15.0);

    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно! ";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F12", 1);
}

string descF13 = "Worldmap encounters ON/OFF";
void CalculateInfoDataF13()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF13;

    ref mc;
	mc = GetMainCharacter();
    if(CheckAttribute(mc,"worldmapencountersoff") == 0)
	{
		mc.worldmapencountersoff = "1";
		Log_SetStringToLog("Worldmap encounters OFF");
	}
	else
	{
		if(mc.worldmapencountersoff == "1")
		{
			mc.worldmapencountersoff = "0";
			Log_SetStringToLog("Worldmap encounters ON");
		}
		else
		{
			mc.worldmapencountersoff = "1";
			Log_SetStringToLog("Worldmap encounters OFF");
		}
	}
    // <--
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F13", 1);
}

string descF14 = "Shotgun mode ON\OFF";
void CalculateInfoDataF14()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF14;
    if(globalSGMode != false)
	{
		globalSGMode = false;
		Log_SetStringToLog("Shotgun mode OFF");
	}else{
		globalSGMode = true;
		Log_SetStringToLog("Shotgun mode ON");
	}
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно! Qfreze=";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F14", 1);
}

string descF15 = "Вызвать смену отношений наций";
void CalculateInfoDataF15()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF15;

    DoQuestCheckDelay("Nation_Legend_Map", 2.0);
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    ProcessCancelExit();
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F15", 1);
}

string descF16 = "+5 дней";
void CalculateInfoDataF16()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF16;

    LAi_Fade("", "");
    for (int i = 1; i<=5; i++)
    {
		AddDataToCurrent(0, 0, 1);
	}
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F16", 1);
}

string descF17 = "Список квестов в pchar";
void CalculateInfoDataF17()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF17;

    aref  curItem;
	aref  arItem;
	aref rootItems;
	int i, j, n;
	string attributeName;
	ref ch;

	//makeref(ch,Characters[GetCharacterIndex("Oxbay Commander")]);
	//makearef(rootItems,ch.Ship);
    //makearef(rootItems,Characters[GetMainCharacterIndex()].Items);
    //makearef(rootItems,ShipsTypes[Characters[GetMainCharacterIndex()].Ship.Type].Cannons.Borts.cannonl);

    makearef(rootItems, pchar.quest);

    //aref qst = makearef(qst, pchar.quest);

    trace("=================================================");
    DumpAttributes(PChar);//&GameInterface);
    trace("=================================================");
    Log_Info("DumpAttributes");
    /*for(i=0; i<GetAttributesNum(rootItems); i++)
    {
		curItem = GetAttributeN(rootItems, i);
		j = sti(GetAttributeValue(curItem));
		totalInfo = totalInfo+"   "+i+"= "+//LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                    GetAttributeName(curItem)+" value="+j;

        for(n=0; n<GetAttributesNum(curItem); n++)
        {
                 arItem = GetAttributeN(curItem,n);
		         j = sti(GetAttributeValue(arItem));
		         totalInfo = totalInfo+//LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                    "   --> "+n+"= "+GetAttributeName(arItem)+" value="+j;
        }

    }*/
    remInt++;
    totalInfo = totalInfo + " page " + remInt;
    for(i=0; i<GetAttributesNum(rootItems); i++)
    {
        if (i > remInt*60 || i < (remInt-1)*60) continue;

        curItem = GetAttributeN(rootItems, i);
		j = sti(GetAttributeValue(curItem));
		totalInfo = totalInfo+"   "+i+"= "+//LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                    GetAttributeName(curItem);// + " : " + GetAttributeValue(curItem);

        /*for(n=0; n<GetAttributesNum(curItem); n++)
        {
                 arItem = GetAttributeN(curItem,n);
		         j = sti(GetAttributeValue(arItem));
		         totalInfo = totalInfo+//LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                    "   --> "+n+"= "+GetAttributeName(arItem)+" value="+j;
        } */

    }
    if (i <= remInt*60) remInt = 0;
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F17", 1);
}

string descF18 = "ChangeShowIntarface - для скринов без надписей";
void CalculateInfoDataF18()
{
	idLngFile = LanguageOpenFile("ItemsDescribe.txt");
	// -->
	totalInfo = descF18;

    //LandEncTemplate[4].enc1.chance = 1.0;
    //LandEncTemplate[4].enc2.chance = 1.0;
    //
    /*if (bSeaActive)
    {
        Weathers[iCurWeatherNum].Wind.Speed.Min = 9.9;
    	Weathers[iCurWeatherNum].Wind.Speed.Max = 10.1;
	} */
	ChangeShowIntarface();
    // <
    totalInfo = totalInfo + LanguageConvertString(idLngFile,"new_string") + LanguageConvertString(idLngFile,"new_string") +
                "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F18", 1);
}

string descF19 = "Добавить шкиперу 35 свободных очков навыков";
void CalculateInfoDataF19()
{
    totalInfo = descF19;
	// -->
    if (sti(pchar.Fellows.Passengers.navigator) != -1)
	{
	    ref chr = GetCharacter(sti(pchar.Fellows.Passengers.navigator));

    	chr.Skill.FreeSkill  = sti(chr.Skill.FreeSkill) + 35;
    }
    else
    {
        totalInfo = "Нет офицера";
    }
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F19", 1);
}

string descF20 = "Обновление света на маяках";
void CalculateInfoDataF20()
{
    totalInfo = descF20;
	// -->
	Sea_CreateLighthouse("Jamaica");
	Sea_CreateLighthouse("Cuba1");
	Sea_CreateLighthouse("Cuba2");
	SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION,-116.0, 55.0, -1568.5, 1, 0, 0, 0, 1, 0, 0, 0, 1);
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F20", 1);
}

string descF21 = "Повернуть камеру в точку (-5, 2, -5) относительно ГГ";
void CalculateInfoDataF21()
{
    totalInfo = descF21;
	// -->
	//locCameraRotateAroundHero(0.0, 3.0, 0.0, 0.03, 0.0, 3.0, 0.0, 200);
	//locCameraRotateAroundHero(0.0, 1.0, 0.0, 0.0, 0.01, 5.0, 0.0, 150);
	//locCameraRotateAroundHero(0.0, 1.0, 0.0, -0.03, 0.03, 7.0, 0.0, 225);
	//locCameraFlyToPosition(stf(Camera.pos.x), stf(Camera.pos.y), stf(Camera.pos.z), stf(Camera.pos.x), stf(Camera.pos.y) + 30, stf(Camera.pos.z), 0.9, -1);
	locCameraLockNearHero(-5.0, 2.0, -5.0, 200, true);
	
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F21", 1);
}

string descF22 = "Аскольд инфо";
void CalculateInfoDataF22()
{
	ref sld = ItemsFromID("Rock_letter");
    totalInfo = descF22;
	totalInfo = "Рендомный хозяин магаза: " + pchar.questTemp.Ascold.TraderId + NewStr() +
	"Рендомный мерчант(колония): " + pchar.questTemp.Ascold.MerchantColony + NewStr() +
	"Рендомный верфист: " + pchar.questTemp.Ascold.ShipyarderId + NewStr() +
	"Скрижали: " + sld.startLocation;
	if (pchar.questTemp.BlueBird.City != "")
	{
		totalInfo = totalInfo + NewStr()+ "Где будут слухи по флейту торговца: " + pchar.questTemp.BlueBird.City;
	}
	else
	{
		totalInfo = totalInfo + NewStr()+ "Слухи по флейтам торговцев ещё не активированы.";
	}
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F22", 1);
}

string descF23 = "Дамп аттрибутов камеры";
void CalculateInfoDataF23()
{
    totalInfo = descF23;
	// -->
    DumpAttributes(&Camera);
	totalInfo += "\n\nCamera.pos.x == " + Camera.pos.x +
		"\nCamera.pos.y = " + Camera.pos.y +
		"\nCamera.pos.z == " + Camera.pos.z +
		"\n\nCamera.ang.z == " + Camera.ang.y +
		"\nCamera.ang.y == " + Camera.ang.z +
		"\nCamera.ang.x == " + Camera.ang.y + "\n\n";
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F23", 1);
}

string descF24 = "Лицензия";
void CalculateInfoDataF24()
{
    totalInfo = descF24;
	// -->	
    GiveNationLicence(sti(pchar.nation), 30);
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F24", 1);
}

string descF25 = "Перегруз локации - для КОЛОВ";
void CalculateInfoDataF25()
{
    totalInfo = descF25;
	// -->
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
    ProcessCancelExit();
    if( bSeaActive && !bAbordageStarted )
    {
    	Sea_ReloadStart();
	}
	else
	{
	    DoQuestReloadToLocation(pchar.location, pchar.location.group, pchar.location.locator, "");
	}
	
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F25", 1);
}

string descF26 = "Дамп атрибутов ГГ";
void CalculateInfoDataF26()
{
	totalInfo = descF26;
	// -->
    ref characterRef = GetMainCharacter();
		
	trace("==================== " + characterRef.ID + " ====================");
	DumpAttributes(characterRef);
	trace("========================================");

	ref realShip;
	int shipType = sti(characterRef.Ship.Type);

	if(shipType != SHIP_NOTUSED) // Есть ли корабль вообще
	{
		realShip = GetRealShip(shipType);	
		DumpAttributes(realShip);
	}
		
	Log_Info("Дамп аттрибутов ГГ");			
	// <--
	totalInfo = totalInfo + NewStr() + NewStr() +
		"Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.26", 1);
}

string descF27 = "Репутация ГГ в минус";
void CalculateInfoDataF27()
{
    totalInfo = descF27;
	// -->
	if ( sti(pchar.reputation) > 1)
	{
	   pchar.reputation = sti(pchar.reputation) - 10;
	   if (sti(pchar.reputation) < 1)
	   {
	       pchar.reputation = 1;
	   }
	}
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F27", 1);
}

string descF28 = "Летучий Голландец";
void CalculateInfoDataF28()
{
    totalInfo = descF28;
	// -->
	if (bSeaActive && !bAbordageStarted && IsEntity(&Sea) && !IsEntity(&worldMap))
	{
		if (!SeaDebugGhost_Start())
		{
			SetFormatedText("DEBUGER_INFO", totalInfo + NewStr() + NewStr() + "Не удалось создать корабль в море.");
			return;
		}
	}
	else GhostShipOnMap();
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F28", 1);
}

string descF29 = "НЗГ у всех наций -50";
void CalculateInfoDataF29()
{
    totalInfo = descF29;
	// -->
	for (int j=0; j< MAX_NATIONS; j++)
	{
        ChangeCharacterNationReputation(pchar, j, 50);
    }
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F29", 1);
}

string descF30 = "+20 дней";
void CalculateInfoDataF30()
{
    totalInfo = descF30;
	// -->
	LAi_Fade("", "");
	AddDataToCurrent(0, 0, 20);
    // <--
    totalInfo = totalInfo + NewStr() + NewStr() +
                "Команда отработала успешно!";
    SetFormatedText("DEBUGER_INFO",totalInfo);
    
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F30", 1);
}

string descF31 = "Супер корабль, полная прокачка ГГ, все перки, предметы";
void CalculateInfoDataF31()
{
	ref mc;
	string sEquipItem;
	totalInfo = descF31;
	mc = GetMainCharacter();
	SetRandSelfSkill(mc, 100, 100);
	SetRandShipSkill(mc, 100, 100);
	mc.rank = 35;
	LAi_SetHP(mc, 370.0, 370.0)
//	mc.Ship.Type = GenerateShipExt(SHIP_CORVETTE_QUEST, true, mc);
	//mc.Ship.Type = GenerateShipExt(SHIP_SOLEYRU, true, mc);
	mc.Ship.Type = GenerateShipExt(SHIP_SANTISIMA, true, mc);

	SetBaseShipData(mc);
	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS36;
//	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS24;
	SetCrewQuantityFull(mc);
	AddCrewMorale(mc, 100);
	ChangeCrewExp(mc, "Sailors", 100);
	ChangeCrewExp(mc, "Cannoners", 100);
	ChangeCrewExp(mc, "Soldiers", 100);

	SetCharacterGoods(mc, GOOD_BALLS, 3000);
	SetCharacterGoods(mc, GOOD_GRAPES, 3000);
	SetCharacterGoods(mc, GOOD_KNIPPELS, 3000);
	SetCharacterGoods(mc, GOOD_BOMBS, 3000);
	SetCharacterGoods(mc, GOOD_FOOD, 1500);
	SetCharacterGoods(mc, GOOD_POWDER, 6000);
	SetCharacterGoods(mc, GOOD_WEAPON, 1300);
	SetCharacterGoods(mc, GOOD_MEDICAMENT, 1300);

	sEquipItem = GetCharacterEquipByGroup(mc, BLADE_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, GUN_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, SPYGLASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, CIRASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);

	AddMoneyToCharacter(mc, 10000000);
	AddItems(mc, "spyglass5", 1);
	AddItems(mc, "pistol5", 1);
	
	sEquipItem = GetGeneratedItem("blade32");
	AddItems(mc, sEquipItem, 1);
	
	AddItems(mc, "bullet", 50);
	AddItems(mc, "GunPowder", 50);
	AddItems(mc, "Map_Best", 1);
	AddItems(mc, "cirass5", 1);

	EquipCharacterbyItem(mc, "spyglass5");
	EquipCharacterbyItem(mc, "pistol5");
 	EquipCharacterbyItem(mc, sEquipItem);
 	EquipCharacterbyItem(mc, "cirass5");
 	
 	SelAllPerksToChar(mc, false);
	 	
	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	
	SetFormatedText("DEBUGER_INFO", totalInfo);
		
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F31", 1);
}

string descF32 = "Простой корабль, неполная прокачка ГГ";
void CalculateInfoDataF32()
{
	ref mc;
	string sEquipItem;
	totalInfo = descF32;
	mc = GetMainCharacter();
	SetRandSelfSkill(mc, 40, 75);
	SetRandShipSkill(mc, 40, 75);
	mc.rank = 10;
//	mc.Ship.Type = GenerateShipExt(SHIP_XebekVML, true, mc);
	mc.Ship.Type = GenerateShipExt(SHIP_BRIGSW, true, mc);
	SetBaseShipData(mc);
	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS24;
	SetCrewQuantityFull(mc);
	AddCrewMorale(mc, 80);
	ChangeCrewExp(mc, "Sailors", 50);
	ChangeCrewExp(mc, "Cannoners", 50);
	ChangeCrewExp(mc, "Soldiers", 50);

	SetCharacterGoods(mc, GOOD_BALLS, 700);
	SetCharacterGoods(mc, GOOD_GRAPES, 700);
	SetCharacterGoods(mc, GOOD_KNIPPELS, 700);
	SetCharacterGoods(mc, GOOD_BOMBS, 700);
	SetCharacterGoods(mc, GOOD_FOOD, 500);
	SetCharacterGoods(mc, GOOD_POWDER, 2000);
	SetCharacterGoods(mc, GOOD_WEAPON, 500);
	SetCharacterGoods(mc, GOOD_MEDICAMENT, 600);

	sEquipItem = GetCharacterEquipByGroup(mc, BLADE_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, GUN_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, SPYGLASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);
	sEquipItem = GetCharacterEquipByGroup(mc, CIRASS_ITEM_TYPE);
	RemoveItems(mc, sEquipItem, 1);

	AddMoneyToCharacter(mc, 1000000);
	AddItems(mc, "spyglass3", 1);
	AddItems(mc, "pistol5", 1);
	
	sEquipItem = GetGeneratedItem("blade27");
	AddItems(mc, sEquipItem, 1);
	
	AddItems(mc, "bullet", 50);
	AddItems(mc, "GunPowder", 50);
	AddItems(mc, "cirass3", 1);

	EquipCharacterbyItem(mc, "spyglass3");
	EquipCharacterbyItem(mc, "pistol5");
 	EquipCharacterbyItem(mc, sEquipItem);
 	EquipCharacterbyItem(mc, "cirass3");
 	
 	SetHalfPerksToChar(mc, false);
 	
	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	
	SetFormatedText("DEBUGER_INFO", totalInfo);
	
	// Статистика по читам
	Statistic_AddValue(PChar, "Cheats.F32", 1);
}

// --->>> методы перебора кораблей через дебаг
int FindShipNum(int iTargetClass)	//ZhilyaevDm подбираем корабли определнного класса и возвращаем следующий
{
	int iShips[25];
	int iShipNum = 0;
	int iCurrentShipNum = 0;
	int iShipForGen = 0;
	int i;
	object rShip;

	ref mc;
	mc = GetMainCharacter();

	if (sti(mc.ship.type) != SHIP_NOTUSED) { iCurrentShipNum = sti(mc.Ship.Type); }

	for (i = SHIP_BARKAS; i <= SHIP_MANOWAR; i++)
	{
		rShip = GetShipByType(i);
		int iCurShipClass = MakeInt(rShip.Class);

		if (iTargetClass == iCurShipClass)
		{
			iShips[iShipNum] = i;
			iShipNum++;
		}
	}

	iShipForGen = iShips[0];
	for (i = 0; i < iShipNum, i++)
	{
		rShip = GetShipByType(iShips[i]);
		int ShipType = MakeInt(rShip.Type);

		if (i == iShipNum - 1) { iShipForGen = iShips[0]; break; }
		if (sti(RealShips[sti(mc.Ship.Type)].basetype) == iShips[i])
		{
			iShipForGen = iShips[i + 1];
			break;
		}
	}

	mc = GetMainCharacter();
	mc.Ship.Type = GenerateShipExt(iShipForGen, true, mc);
	SetBaseShipData(mc);

	if (sti(RealShips[sti(mc.Ship.Type)].CannonsQuantity) != 0) 
	{
		switch (sti(RealShips[sti(mc.Ship.Type)].MaxCaliber))
		{
			case 3:		mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS3; break;
			case 6:		mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS6; break;
			case 8:		mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS8; break;
			case 12:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS12; break;
			case 16:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS16; break;
			case 20:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS20; break;
			case 24:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS24; break;
			case 32:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS32; break;
			case 36:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS36; break;
			case 42:	mc.Ship.Cannons.Type = CANNON_TYPE_CANNON_LBS42; break;
		}
	}

	AddCrewMorale(mc, 100);
	SetCrewQuantityFull(mc);
	ChangeCrewExp(mc, "Sailors", 100);
	ChangeCrewExp(mc, "Cannoners", 100);
	ChangeCrewExp(mc, "Soldiers", 100);

	SetCharacterGoods(mc, GOOD_BALLS, 450 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_GRAPES, 450 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_KNIPPELS, 450 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_BOMBS, 450 * (7 - iTargetClass));

	SetCharacterGoods(mc, GOOD_FOOD, 150 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_POWDER, 800 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_WEAPON, 130 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_MEDICAMENT, 130 * (7 - iTargetClass));

	SetCharacterGoods(mc, GOOD_PLANKS, 40 * (7 - iTargetClass));
	SetCharacterGoods(mc, GOOD_SAILCLOTH, 40 * (7 - iTargetClass));
	SetCharacterGoods(pchar, GOOD_EBONY , 200 * (7 - iTargetClass));	
	//return RealShips[sti(mc.Ship.Type)].BaseName;
	return sti(mc.Ship.Type);
}

string NationalHullName(int iHullNum, bool isNationalHull)
{
	if (isNationalHull == true)
	{
		switch (iHullNum)
		{
		case 1: return " - Английская расцветка";	break;
		case 2: return " - Французская расцветка";	break;
		case 3: return " - Испанская расцветка";	break;
		case 4: return " - Голландская расцветка";	break;
		}
	}
	return " - нет национальных цветов";
}

string descF33 = "Перебрать корабли 7-го класса";	//ZhilyaevDm
void CalculateInfoDataF33()
{
	totalInfo = descF33;

	int iShipType = FindShipNum(7);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F33", 1);
}

string descF34 = "Перебрать корабли 6-го класса";	//ZhilyaevDm
void CalculateInfoDataF34()
{
	totalInfo = descF34;

	int iShipType = FindShipNum(6);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F34", 1);
}

string descF35 = "Перебрать корабли 5-го класса";	//ZhilyaevDm
void CalculateInfoDataF35()
{
	totalInfo = descF35;

	int iShipType = FindShipNum(5);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F35", 1);
}

string descF36 = "Перебрать корабли 4-го класса";	//ZhilyaevDm
void CalculateInfoDataF36()
{
	totalInfo = descF36;

	int iShipType = FindShipNum(4);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F36", 1);
}

string descF37 = "Перебрать корабли 3-го класса";	//ZhilyaevDm
void CalculateInfoDataF37()
{
	totalInfo = descF37;

	int iShipType = FindShipNum(3);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F37", 1);
}

string descF38 = "Перебрать корабли 2-го класса";	//ZhilyaevDm
void CalculateInfoDataF38()
{
	totalInfo = descF38;

	int iShipType = FindShipNum(2);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F38", 1);
}

string descF39 = "Перебрать корабли 1-го класса";	//ZhilyaevDm
void CalculateInfoDataF39()
{
	totalInfo = descF39;

	int iShipType = FindShipNum(1);

	int hullamount = sti(RealShips[iShipType].HullsAmount);
	string sName = RealShips[iShipType].BaseName;
	string str = "False";
	if (RealShips[iShipType].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\nВыбран корабль : " + sName + " - " + XI_ConvertString(sName);
	totalInfo += "\n\nНацирнальные цвета корпуса = " + str + "\n"
		+ "Выбранный хулл / Количество хуллов : " + sti(RealShips[iShipType].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[iShipType].ship.upgrades.hull), RealShips[iShipType].isNationalHull) + "\n\n";

	totalInfo = totalInfo + NewStr() + NewStr() + "Команда отработала успешно!";
	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F39", 1);
}

string descF40 = "Перекраска корабля ГГ";	//ZhilyaevDm
void CalculateInfoDataF40()
{
	ref mc;
	totalInfo = descF40;
	mc = GetMainCharacter();

	int iHullNum = sti(RealShips[sti(mc.Ship.Type)].ship.upgrades.hull) + 1;
	string str = "False";

	if (sti(mc.ship.type) != SHIP_NOTUSED)
	{
		if (RealShips[sti(mc.Ship.Type)].isNationalHull == true)
		{
			if (iHullNum > 4)
				iHullNum = 1;
		}
		else
		{
			if (iHullNum > sti(RealShips[sti(mc.Ship.Type)].HullsAmount))
				iHullNum = 1;
		}

		RealShips[sti(mc.Ship.Type)].ship.upgrades.hull = iHullNum;
	}
	int hullamount = sti(RealShips[sti(mc.Ship.Type)].HullsAmount);
	if (RealShips[sti(mc.Ship.Type)].isNationalHull == true) { hullamount = 4; str = "True"; }

	totalInfo += "\n\nНацирнальные цвета корпуса = " + str +
		"\nВыбранный хулл / Количество хуллов : " + sti(RealShips[sti(mc.Ship.Type)].ship.upgrades.hull) + " / "
		+ hullamount + NationalHullName(sti(RealShips[sti(mc.Ship.Type)].ship.upgrades.hull), RealShips[sti(mc.Ship.Type)].isNationalHull) + "\n";

	SetFormatedText("DEBUGER_INFO", totalInfo);
	Statistic_AddValue(PChar, "Cheats.F40", 1);
}

// --->>> обновления массивов кораблей и локаций
string descF41 = "Обновление массива кораблей";
void CalculateInfoDataF41()
{
    totalInfo = descF41;

    ShipsInit();
	//InitItems();
	
	
    totalInfo = totalInfo + NewStr() + "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);

    // Статистика по читам
    Statistic_AddValue(PChar, "Cheats.F41", 1);
}

string descF42 = "Обновление массива локаций";
void CalculateInfoDataF42()
{
    totalInfo = descF42;

    LocationInit();
	IslandsInit();

    totalInfo = totalInfo + NewStr() + "Команда отработала успешно!";
    LanguageCloseFile(idLngFile);
    SetFormatedText("DEBUGER_INFO",totalInfo);

    // Статистика по читам
    Statistic_AddValue(PChar, "Cheats.F41", 1);
}

void ReloadByStr()
{
	// разбор строки
	string  loc = stripblank(GetSubStringByNum(GameInterface.teleport_edit.str, 0));
	string  grp = stripblank(GetSubStringByNum(GameInterface.teleport_edit.str, 1));
	string  ltr = stripblank(GetSubStringByNum(GameInterface.teleport_edit.str, 2));

    int i = FindLocation(loc);
    if (i != -1)
    {
        if(CheckAttribute(&locations[i],"fastreload"))
        {
			if (GetCityFrom_Sea(locations[i].fastreload) != "")
		    {
		        setCharacterShipLocation(pchar, GetCityFrom_Sea(locations[i].fastreload));
		        setWDMPointXZ(GetCityFrom_Sea(locations[i].fastreload));
		    }
	    }
	    else
	    {
	        if (locations[i].type == "seashore" || locations[i].type == "mayak")
	        {
	            setCharacterShipLocation(pchar, loc));
		        setWDMPointXZ(loc);
	        }
	    }
	    DoQuestReloadToLocation(loc, grp, ltr, "");
	    Log_Info(loc);
	    
		// Статистика по читам
		Statistic_AddValue(PChar, "Cheats.ReloadByStr", 1);
    }
    else
    {
        Log_Info("Нет локации");
    }
}

void GetRealCoordsObjects()
{
	trace("---------");
	trace("Barbados (island)					: " + Map_GetRealCoordX(946.0062) + " " + Map_GetRealCoordZ(-486.9014));
	trace("Бриджтаун 							: " + Map_GetRealCoordX(903.6217) + " " + Map_GetRealCoordZ(-522.2799));
	trace("Mayak2 (маяк Барбадоса)				: " + Map_GetRealCoordX(918.619) + " " + Map_GetRealCoordZ(-506.827));
	trace("Shore4 (мыс Гаррисон)				: " + Map_GetRealCoordX(919.808) + " " + Map_GetRealCoordZ(-448.061));
	trace("Shore5 (мыс Раггед Пойнт)			: " + Map_GetRealCoordX(983.787) + " " + Map_GetRealCoordZ(-518.14));	
	trace("---------");
	trace("Antigua (island)						: " + Map_GetRealCoordX(903.9222) + " " + Map_GetRealCoordZ(109.0605));
	trace("Сент-Джонс 							: " + Map_GetRealCoordX(852.4083) + " " + Map_GetRealCoordZ(128.9092));
	trace("Shore1 (мыс Бун)						: " + Map_GetRealCoordX(880.892) + " " + Map_GetRealCoordZ(148.897));
	trace("Shore2 (залив Уиллогби)				: " + Map_GetRealCoordX(931.028) + " " + Map_GetRealCoordZ(74.5353));
	trace("---------");
	trace("Curacao (island) 					: " + Map_GetRealCoordX(383.4142) + " " + Map_GetRealCoordZ(-691.11));
	trace("Виллемштадт 							: " + Map_GetRealCoordX(396.1) + " " + Map_GetRealCoordZ(-722.8));	
	trace("Shore22 (Пальмовый берег)			: " + Map_GetRealCoordX(354.604) + " " + Map_GetRealCoordZ(-678.54));
	trace("Shore23 (Мыс Кальвинистов)			: " + Map_GetRealCoordX(347.313) + " " + Map_GetRealCoordZ(-705.112));
	trace("Shore24 (берег Приют Контрабандиста)	: " + Map_GetRealCoordX(455.427) + " " + Map_GetRealCoordZ(-740.655));
	trace("---------");
	trace("SentMartin (island)					: " + Map_GetRealCoordX(921.559) + " " + Map_GetRealCoordZ(523.596));
	trace("Мариго 								: " + Map_GetRealCoordX(900.0994) + " " + Map_GetRealCoordZ(501.6531));	
	trace("Shore40 (берег Спасения)				: " + Map_GetRealCoordX(924.442) + " " + Map_GetRealCoordZ(554.385));	
	trace("Shore41 (бухта Неистового ветра)		: " + Map_GetRealCoordX(898.877) + " " + Map_GetRealCoordZ(522.703));	
	trace("---------");
	trace("Guadeloupe (island)					: " + Map_GetRealCoordX(537.114) + " " + Map_GetRealCoordZ(-25.3853));	
	trace("Бас-Тер 								: " + Map_GetRealCoordX(462.2107) + " " + Map_GetRealCoordZ(-73.0837));		
	trace("Mayak4 (маяк Гваделупы)				: " + Map_GetRealCoordX(465.076) + " " + Map_GetRealCoordZ(-38.9423));		
	trace("Shore28 (бухта Морн л'О)				: " + Map_GetRealCoordX(536.636) + " " + Map_GetRealCoordZ(26.1279));		
	trace("Shore29 (пляж Капстер)				: " + Map_GetRealCoordX(507.669) + " " + Map_GetRealCoordZ(-75.4176));		
	trace("Shore30 (бухта Гран Бур)				: " + Map_GetRealCoordX(549.807) + " " + Map_GetRealCoordZ(-69.7107));		
	trace("Shore31 (мыс Недоступный)			: " + Map_GetRealCoordX(589.889) + " " + Map_GetRealCoordZ(-65.0601));		
	trace("---------");
	trace("Martinique (island) 					: " + Map_GetRealCoordX(538.524) + " " + Map_GetRealCoordZ(-385.567));		
	trace("Форт-де-Франс 						: " + Map_GetRealCoordX(513.51) + " " + Map_GetRealCoordZ(-397.645));		
	trace("Ле-Франсуа 							: " + Map_GetRealCoordX(559.008) + " " + Map_GetRealCoordZ(-368.16));		
	trace("Shore38 (пляж Ламантен)				: " + Map_GetRealCoordX(481.181) + " " + Map_GetRealCoordZ(-340.832));		
	trace("Shore39 (бухта Ле Марен)				: " + Map_GetRealCoordX(557.181) + " " + Map_GetRealCoordZ(-422.987));		
	trace("---------");
	trace("Bermudes (island)					: " + Map_GetRealCoordX(920.738) + " " + Map_GetRealCoordZ(920.767));		
	trace("Пиратское поселение 					: " + Map_GetRealCoordX(931.059) + " " + Map_GetRealCoordZ(928.281));		
	trace("Shore3 (залив Руны)					: " + Map_GetRealCoordX(915.745) + " " + Map_GetRealCoordZ(966.469));		
	trace("Shore_ship1 (бухта Разбитого Корыта) : " + Map_GetRealCoordX(968.947) + " " + Map_GetRealCoordZ(923.636));		
	trace("---------");
	trace("Maracaibo (areal) 					: " + Map_GetRealCoordX(178.51) + " " + Map_GetRealCoordZ(-870.37));		
	trace("Маракайбо 							: " + Map_GetRealCoordX(134.82) + " " + Map_GetRealCoordZ(-939.65));		
	trace("Shore37 (бухта Гуахира)				: " + Map_GetRealCoordX(140.753) + " " + Map_GetRealCoordZ(-828.99));		
	trace("Shore_ship3 (мыс Несбывшихся Надежд)	: " + Map_GetRealCoordX(198.162) + " " + Map_GetRealCoordZ(-994.035));		
	trace("---------");
	trace("Beliz (areal) 						: " + Map_GetRealCoordX(-896.084) + " " + Map_GetRealCoordZ(230.227));		
	trace("Shore_ship2 (залив Гибели)			: " + Map_GetRealCoordX(-1022.94) + " " + Map_GetRealCoordZ(629.355));		
	trace("Shore6 (мыс Каточе)					: " + Map_GetRealCoordX(-909.655) + " " + Map_GetRealCoordZ(629.079));		
	trace("Shore7 (залив Косумель)				: " + Map_GetRealCoordX(-915.581) + " " + Map_GetRealCoordZ(409.002));		
	trace("Shore8 (бухта Четумаль)				: " + Map_GetRealCoordX(-963.489) + " " + Map_GetRealCoordZ(281.628));		
	trace("Белиз 								: " + Map_GetRealCoordX(-978.396) + " " + Map_GetRealCoordZ(158.751));		
	trace("Shore9 (бухта Аматике)				: " + Map_GetRealCoordX(-1017.2) + " " + Map_GetRealCoordZ(-33.7293));		
	trace("Shore10 (лагуна Каратаска)			: " + Map_GetRealCoordX(-818.198) + " " + Map_GetRealCoordZ(-177.208));		
	trace("---------");
	trace("Jamaica (island)						: " + Map_GetRealCoordX(-298.0) + " " + Map_GetRealCoordZ(-128.0));		
	trace("Порт-Ройал 							: " + Map_GetRealCoordX(-250.157) + " " + Map_GetRealCoordZ(-150.839));		
	trace("Shore35 (мыс Негрил)					: " + Map_GetRealCoordX(-446.951) + " " + Map_GetRealCoordZ(-107.118));			
	trace("Shore36 (бухта Портленд)				: " + Map_GetRealCoordX(-281.543) + " " + Map_GetRealCoordZ(-164.245));			
	trace("Mayak3 (маяк Порт Рояля)				: " + Map_GetRealCoordX(-224.127) + " " + Map_GetRealCoordZ(-163.435));			
	trace("---------");
	trace("PuertoRico (island) 					: " + Map_GetRealCoordX(506.489) + " " + Map_GetRealCoordZ(460.815));			
	trace("Сан-Хуан 							: " + Map_GetRealCoordX(528.645) + " " + Map_GetRealCoordZ(484.18));				
	trace("Shore44 (бухта Агуадилья)			: " + Map_GetRealCoordX(392.82) + " " + Map_GetRealCoordZ(501.808));				
	trace("Shore45 (пляж Фахардо)				: " + Map_GetRealCoordX(602.049) + " " + Map_GetRealCoordZ(458.322));				
	trace("---------");
	trace("Cuba1 (areal) 						: " + Map_GetRealCoordX(-358.027) + " " + Map_GetRealCoordZ(819.223));				
	trace("Сантьяго 							: " + Map_GetRealCoordX(-339.796) + " " + Map_GetRealCoordZ(771.839));					
	trace("Пуэрто-Принсипе 						: " + Map_GetRealCoordX(-282.032) + " " + Map_GetRealCoordZ(839.056));					
	trace("Shore11 (мыс Камагуэй)				: " + Map_GetRealCoordX(-386.907) + " " + Map_GetRealCoordZ(883.044));				
	trace("Shore12 (залив Анна Мария)			: " + Map_GetRealCoordX(-468.486) + " " + Map_GetRealCoordZ(838.497));				
	trace("Mayak9 (маяк Сантьяго)				: " + Map_GetRealCoordX(-333.295) + " " + Map_GetRealCoordZ(723.486));				
	trace("---------");
	trace("Cuba2 (areal) 						: " + Map_GetRealCoordX(-640.61) + " " + Map_GetRealCoordZ(870.058));					
	trace("Гавана 								: " + Map_GetRealCoordX(-663.685) + " " + Map_GetRealCoordZ(913.862));					
	trace("Shore13 (залив Батабано)				: " + Map_GetRealCoordX(-613.56) + " " + Map_GetRealCoordZ(886.009));				
	trace("Shore14 (бухта Хувентуд)				: " + Map_GetRealCoordX(-653.269) + " " + Map_GetRealCoordZ(871.013));				
	trace("Shore15 (мыс Сан Антонио)			: " + Map_GetRealCoordX(-715.129) + " " + Map_GetRealCoordZ(797.212));				
	trace("Mayak10 (маяк Гаваны)				: " + Map_GetRealCoordX(-647.18) + " " + Map_GetRealCoordZ(925.046));				
	trace("---------");
	trace("Tortuga (island) 					: " + Map_GetRealCoordX(102.328) + " " + Map_GetRealCoordZ(852.912));				
	trace("Тортуга 								: " + Map_GetRealCoordX(116.875) + " " + Map_GetRealCoordZ(817.942));				
	trace("Shore58 (пляж Глаз Рыбы)				: " + Map_GetRealCoordX(131.064) + " " + Map_GetRealCoordZ(849.44));				
	trace("Mayak6 (маяк Тортуги)				: " + Map_GetRealCoordX(103.23) + " " + Map_GetRealCoordZ(812.356));				
	trace("---------");
	trace("Trinidad (island)					: " + Map_GetRealCoordX(889.76) + " " + Map_GetRealCoordZ(-774.746));				
	trace("Порт-оф-Спейн 						: " + Map_GetRealCoordX(848.282) + " " + Map_GetRealCoordZ(-822.588));				
	trace("Shore59 (бухта Скарборо)				: " + Map_GetRealCoordX(937.322) + " " + Map_GetRealCoordZ(-737.478));				
	trace("Shore60 (мыс Галера)					: " + Map_GetRealCoordX(906.25) + " " + Map_GetRealCoordZ(-757.928));				
	trace("Mayak1 (маяк Тринидада и Тобаго)		: " + Map_GetRealCoordX(821.24) + " " + Map_GetRealCoordZ(-847.69));				
	trace("---------");
	trace("Nevis (island)						: " + Map_GetRealCoordX(681.595) + " " + Map_GetRealCoordZ(256.013));				
	trace("Чарльзтаун 							: " + Map_GetRealCoordX(658.918) + " " + Map_GetRealCoordZ(241.972));				
	trace("Shore42 (бухта Ньюкасл)				: " + Map_GetRealCoordX(690.047) + " " + Map_GetRealCoordZ(291.621));					
	trace("Shore43 (пляж Бат)					: " + Map_GetRealCoordX(684.026) + " " + Map_GetRealCoordZ(212.793));					
	trace("Mayak5 (маяк Невиса)					: " + Map_GetRealCoordX(663.418) + " " + Map_GetRealCoordZ(228.281));					
	trace("---------");
	trace("Hispaniola1 (areal) 					: " + Map_GetRealCoordX(198.977) + " " + Map_GetRealCoordZ(588.696));					
	trace("Санто-Доминго 						: " + Map_GetRealCoordX(198.683) + " " + Map_GetRealCoordZ(547.97));					
	trace("Ла-Вега 								: " + Map_GetRealCoordX(166.708) + " " + Map_GetRealCoordZ(662.952));					
	trace("Shore32 (мыс Исабела)				: " + Map_GetRealCoordX(130.591) + " " + Map_GetRealCoordZ(682.362));						
	trace("Shore33 (бухта Самана)				: " + Map_GetRealCoordX(229.081) + " " + Map_GetRealCoordZ(593.727));						
	trace("Mayak8 (маяк Санто-Доминго)			: " + Map_GetRealCoordX(168.314) + " " + Map_GetRealCoordZ(520.218));						
	trace("---------");
	trace("Hispaniola2 (areal) 					: " + Map_GetRealCoordX(-47.1762) + " " + Map_GetRealCoordZ(625.596));					
	trace("Порт-о-Пренс 						: " + Map_GetRealCoordX(22.521) + " " + Map_GetRealCoordZ(592.233));					
	trace("Shore34 (бухта Гонаив)				: " + Map_GetRealCoordX(0.999614) + " " + Map_GetRealCoordZ(643.122));							
	trace("Mayak7 (маяк Порт-о-Пренса)			: " + Map_GetRealCoordX(-14.9756) + " " + Map_GetRealCoordZ(606.258));						
	trace("---------");
	trace("Dominica 							: " + Map_GetRealCoordX(783.057) + " " + Map_GetRealCoordZ(-256.833));					
	trace("Shore27 (мыс Скотс Хед)				: " + Map_GetRealCoordX(779.89) + " " + Map_GetRealCoordZ(-295.002));								
	trace("Shore26 (пляж Касл Брус)				: " + Map_GetRealCoordX(803.577) + " " + Map_GetRealCoordZ(-252.617));								
	trace("---------");
	trace("Теркс 								: " + Map_GetRealCoordX(443.257) + " " + Map_GetRealCoordZ(939.102));						
	trace("Shore57 (залив Южный)				: " + Map_GetRealCoordX(434.944) + " " + Map_GetRealCoordZ(923.524));								
	trace("Shore56 (залив Северный)				: " + Map_GetRealCoordX(444.298) + " " + Map_GetRealCoordZ(954.975));								
	trace("---------");
	trace("Кайман 								: " + Map_GetRealCoordX(-489.6) + " " + Map_GetRealCoordZ(362.455));						
	trace("Shore17 (залив Эсмеральды)			: " + Map_GetRealCoordX(-505.038) + " " + Map_GetRealCoordZ(348.115));									
	trace("Shore16 (Пустынный пляж)				: " + Map_GetRealCoordX(-449.837) + " " + Map_GetRealCoordZ(366.308));
	trace("---------");
	trace("Panama (areal) 						: " + Map_GetRealCoordX(-395.198) + " " + Map_GetRealCoordZ(-972.801));							
	trace("Панама 								: " + Map_GetRealCoordX(-389.965) + " " + Map_GetRealCoordZ(-949.118));							
	trace("Shore63 (залив Сан Мигель) 			: " + Map_GetRealCoordX(-328.166) + " " + Map_GetRealCoordZ(-1023.72));
	trace("---------");
	trace("PortoBello (areal) 					: " + Map_GetRealCoordX(-435.73) + " " + Map_GetRealCoordZ(-895.763));							
	trace("Порто-Белло 							: " + Map_GetRealCoordX(-427.183) + " " + Map_GetRealCoordZ(-907.277));
	trace("Shore46 (лагуна Чирики)				: " + Map_GetRealCoordX(-627.474) + " " + Map_GetRealCoordZ(-940.853));
	trace("Shore47 (залив Москитос)				: " + Map_GetRealCoordX(-486.49) + " " + Map_GetRealCoordZ(-976.966));
	trace("Shore48 (Дарьенский залив)			: " + Map_GetRealCoordX(-279.316) + " " + Map_GetRealCoordZ(-1005.49));
	trace("---------");
	trace("Cartahena (areal) 					: " + Map_GetRealCoordX(-51.2104) + " " + Map_GetRealCoordZ(-851.77));								
	trace("Картахена 							: " + Map_GetRealCoordX(-26.7669) + " " + Map_GetRealCoordZ(-872.535));								
	trace("Shore25 (бухта Ковеньяс)				: " + Map_GetRealCoordX(-115.589) + " " + Map_GetRealCoordZ(-937.387));
	trace("Mayak11 (маяк Картахены)				: " + Map_GetRealCoordX(-65.6347) + " " + Map_GetRealCoordZ(-842.072));
	trace("---------");
	trace("Caracas (areal) 						: " + Map_GetRealCoordX(435.126) + " " + Map_GetRealCoordZ(-936.197));								
	trace("Каракас 								: " + Map_GetRealCoordX(440.684) + " " + Map_GetRealCoordZ(-960.339));
	trace("Shore21 (мыс Аруба)					: " + Map_GetRealCoordX(410.688) + " " + Map_GetRealCoordZ(-913.531));
	trace("---------");
	trace("Cumana (areal) 						: " + Map_GetRealCoordX(767.999) + " " + Map_GetRealCoordZ(-936.266));								
	trace("Кумана 								: " + Map_GetRealCoordX(691.343) + " " + Map_GetRealCoordZ(-896.619));								
	trace("Shore20 (пляж Лос Текес)				: " + Map_GetRealCoordX(574.327) + " " + Map_GetRealCoordZ(-929.042));
	trace("Shore19 (бухта Карупано)				: " + Map_GetRealCoordX(768.622) + " " + Map_GetRealCoordZ(-914.469));
	trace("Shore18 (берег Бока де ла Сьерпе)	: " + Map_GetRealCoordX(924.423) + " " + Map_GetRealCoordZ(-969.921));
	trace("---------");
	trace("SantaCatalina (areal) 				: " + Map_GetRealCoordX(-742.269) + " " + Map_GetRealCoordZ(-563.205));									
	trace("Санта-Каталина 						: " + Map_GetRealCoordX(-769.375) + " " + Map_GetRealCoordZ(-537.051));
	trace("Shore53 (Берег Москитов)				: " + Map_GetRealCoordX(-792.718) + " " + Map_GetRealCoordZ(-331.728));
	trace("Shore54 (мыс Перлас)					: " + Map_GetRealCoordX(-760.343) + " " + Map_GetRealCoordZ(-484.106));
	trace("Shore55 (залив Сан Хуан дель Норте)	: " + Map_GetRealCoordX(-736.763) + " " + Map_GetRealCoordZ(-756.295));
	trace("---------");
	trace("ГПК									: " + Map_GetRealCoordX(-938.667) + " " + Map_GetRealCoordZ(938.654));										
	trace("---------");
	trace("Teno (areal)							: " + Map_GetRealCoordX(-966.062) + " " + Map_GetRealCoordZ(-294.232));										
	trace("Теночтитлан 							: " + Map_GetRealCoordX(-969.194) + " " + Map_GetRealCoordZ(-305.237));										
	trace("Shore61 (Тепейакак) 					: " + Map_GetRealCoordX(-955.553) + " " + Map_GetRealCoordZ(-281.197));
	trace("Shore62 (Истапалапу)					: " + Map_GetRealCoordX(-935.811) + " " + Map_GetRealCoordZ(-316.162));
	trace("---------");	
}

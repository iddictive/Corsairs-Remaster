#include "dialog_func.c"

#define EVENT_DIALOG_START		"evntDialogStart"
#define EVENT_DIALOG_EXIT		"evntDialogExit"

#event_handler("dlgReady", "StartDialogWithMainCharacter");
#event_handler("EmergencyDialogExit","DialogExit");

extern void ProcessDialogEvent();
extern void ProcessCommonDialogEvent(ref NPChar, aref Link, aref NextDiag); // метод, содержащий ветку квест и др ветки конкрентого НПС

bool dialogDisable = false;
object	Dialog;
ref		CharacterRef;
bool	dialogRun = false;

bool	dialogSelf = false;

string  FullDialogPath;
string	PathDlgLngExtn = "";//fix

string dialogEditStrings[10];

void  ProcessCommonDialog(ref NPChar, aref Link, aref NextDiag)
{
    ProcessCommonDialogEvent(NPChar, Link, NextDiag);
}

bool DialogAmbush_GroupHasQuest(ref target)
{
	if(!CheckAttribute(target, "chr_ai.group")) return false;
	if(!CheckAttribute(&LAi_grp_relations, "quests")) return false;
	aref quests;
	makearef(quests, LAi_grp_relations.quests);
	int num = GetAttributesNum(quests);
	for(int i = 0; i < num; i++)
	{
		aref quest = GetAttributeN(quests, i);
		if(CheckAttribute(quest, "group") && quest.group == target.chr_ai.group) return true;
	}
	return false;
}

bool DialogAmbush_CanAttack(ref target)
{
	return DialogAmbush_BlockReason(target) == "";
}

string DialogAmbush_BlockReason(ref target)
{
	if(dialogSelf) return "self";
	if(!IsEntity(target)) return "notentity";
	if(LAi_IsDead(target)) return "dead";
	if(!CheckAttribute(target, "location")) return "nolocation";
	if(target.location != pchar.location) return "otherlocation";
	if(CheckAttribute(target, "DialogAmbushProtected")) return "protected";
	//chr.quest - контейнер данных, который KVL вешает на каждого сгенерированного НПС (InitCharacter, LAi_CreateFantomCharacterEx), квестовым его считать нельзя
	if(CheckAttribute(target, "isquest")) return "quest";
	if(CheckAttribute(target, "chr_ai.hpchecker")) return "hpchecker";
	if(LAi_IsImmortal(target)) return "immortal";
	if(CheckAttribute(target, "DontClearDead") && FindFellowtravellers(pchar, target) == FELLOWTRAVEL_NO) return "dontcleardead";
	if(DialogAmbush_GroupHasQuest(target)) return "groupquest";
	if(!LAi_LocationCanFight()) return "nofightlocation";
	if(LAi_IsBoardingProcess()) return "boarding";
	if(LAi_IsCapturedLocation) return "captured";
	if(chrDisableReloadToLocation) return "reloadlock";
	if(bDisableFastReload) return "fastreloadlock";
	if(bDisableCharacterMenu) return "menulock";
	if(LAi_group_IsActivePlayerAlarm()) return "alarm";
	if(LAi_CheckFightMode(pchar)) return "fightmode";
	return "";
}

bool DialogAmbush_IsPlayerOwned(ref target)
{
	if(CheckAttribute(target, "chr_ai.group"))
	{
		if(target.chr_ai.group == LAI_GROUP_PLAYER || target.chr_ai.group == LAI_GROUP_PLAYER_OWN) return true;
	}
	return FindFellowtravellers(pchar, target) != FELLOWTRAVEL_NO;
}

void DialogAmbush_RestoreTargetLocation(ref target, string originalLocation)
{
	if(originalLocation == "" || originalLocation == "none" || originalLocation == "None") return;
	if(IsEntity(target)) ChangeCharacterAddressGroup(target, originalLocation, "goto", "random_free");
}

void DialogAmbush_DetachPlayerTarget(ref target)
{
	int fellowType = FindFellowtravellers(pchar, target);
	if(fellowType == FELLOWTRAVEL_NO) return;
	string originalLocation = "";
	if(CheckAttribute(target, "location")) originalLocation = target.location;
	if(fellowType == FELLOWTRAVEL_COMPANION)
	{
		RemoveCharacterCompanion(pchar, target);
	}
	else
	{
		RemovePassenger(pchar, target);
	}
	LAi_SetWarriorTypeNoGroup(target);
	DialogAmbush_RestoreTargetLocation(target, originalLocation);
}

bool DialogAmbush_IsOneOnOne(ref target)
{
	int num = FindNearCharacters(target, 15.0, -1.0, -1.0, 0.01, true, true);
	for(int i = 0; i < num; i++)
	{
		int idx = sti(chrFindNearCharacters[i].index);
		if(idx < 0 || idx == sti(pchar.index) || idx == sti(target.index)) continue;
		ref witness = &Characters[idx];
		if(!IsEntity(witness) || LAi_IsDead(witness)) continue;
		if(DialogAmbush_IsPlayerOwned(witness)) continue;
		return false;
	}
	return true;
}

bool DialogAmbush_AddLink(aref Link, string text, string node)
{
	for(int i = 1; i <= 99; i++)
	{
		string attr = "l" + i;
		if(CheckAttribute(Link, attr)) continue;
		Link.(attr) = text;
		Link.(attr).go = node;
		return true;
	}
	return false;
}

string DialogAmbush_TargetLabel()
{
	string id = "?";
	string group = "?";
	if(IsEntity(CharacterRef))
	{
		if(CheckAttribute(CharacterRef, "id")) id = CharacterRef.id;
		if(CheckAttribute(CharacterRef, "chr_ai.group")) group = CharacterRef.chr_ai.group;
	}
	return id + " group=" + group;
}

void DialogAmbush_AppendLinks()
{
	if(!DialogAmbush_CanAttack(CharacterRef))
	{
		string blocked = DialogAmbush_BlockReason(CharacterRef);
		Trace("DialogAmbush blocked: " + blocked + " " + DialogAmbush_TargetLabel());
		return;
	}
	aref Link;
	makearef(Link, Dialog.Links);
	if(DialogAmbush_IsOneOnOne(CharacterRef))
	{
		if(!DialogAmbush_AddLink(Link, "Ударить исподтишка", "DialogAmbush_Sneak")) return;
	}
	else if(!DialogAmbush_AddLink(Link, "Напасть", "DialogAmbush_Attack")) return;
	Trace("DialogAmbush links: " + DialogAmbush_TargetLabel() + " total=" + GetAttributesNum(Link));
}

void DialogAmbush_LocationUnload()
{
	if(CheckAttribute(pchar, "DialogAmbushSneak.Unarmed"))
	{
		SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	}
	DeleteAttribute(pchar, "DialogAmbushSneak");
}

void DialogAmbush_Start(bool trySneak)
{
	ref target = CharacterRef;
	if(!DialogAmbush_CanAttack(target))
	{
		DialogExit();
		return;
	}
	if(trySneak && !DialogAmbush_IsOneOnOne(target))
	{
		DialogExit();
		return;
	}

	if(trySneak) Crime_MarkPlayerIntent(target, "ambush");
	else Crime_MarkPlayerIntent(target, "attack");

	bool playerOwned = DialogAmbush_IsPlayerOwned(target);
	if(playerOwned)
	{
		DialogAmbush_DetachPlayerTarget(target);
		LAi_SetWarriorTypeNoGroup(target);
		LAi_group_MoveCharacter(target, LAI_DEFAULT_GROUP);
		SetCharacterRelationBoth(sti(target.index), GetMainCharacterIndex(), RELATION_ENEMY);
	}

	DialogExit();
	if(!LAi_IsCharacterControl(pchar)) LAi_SetPlayerType(pchar);
	LAi_LockFightMode(pchar, false);
	SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	if(trySneak)
	{
		pchar.DialogAmbushSneak.Target = target.index;
		LAi_SetActorTypeNoGroup(pchar);
		LAi_ActorTurnToCharacter(pchar, target);
		LAi_ActorAnimation(pchar, "ambush_punch", "", 0.9);
		PostEvent("DialogAmbush_SneakStrike", 500);
		PostEvent("DialogAmbush_SneakDraw", 900);
		return;
	}
	LAi_group_Attack(target, pchar);
	AddDialogExitQuest("MainHeroFightModeOn");
}

//Нанести удар кулаком исподтишка: урон на попадании
void DialogAmbush_ApplySneakStrike()
{
	if(!CheckAttribute(pchar, "DialogAmbushSneak.Target")) return;
	int idx = sti(pchar.DialogAmbushSneak.Target);
	if(idx < 0) return;
	ref target = &Characters[idx];
	if(!IsEntity(target) || LAi_IsDead(target)) return;
	LAi_group_Attack(target, pchar);
	Lai_CharacterChangeEnergy(pchar, -LAi_CalcUseEnergyForBlade(pchar, "break"));
	LAi_ApplyCharacterAttackDamage(pchar, target, "break", false);
}

//Вернуть управление и выхватить оружие сразу после удара
void DialogAmbush_DrawWeapon()
{
	if(!CheckAttribute(pchar, "DialogAmbushSneak.Target")) return;
	int idx = sti(pchar.DialogAmbushSneak.Target);
	DeleteAttribute(pchar, "DialogAmbushSneak.Target");
	LAi_SetPlayerType(pchar);
	LAi_LockFightMode(pchar, false);
	SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	LAi_SetFightMode(pchar, true);
	if(!LAi_CheckFightMode(pchar))
	{
		//Сабли в слоте нет - продолжаем бой на кулаках
		SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", true);
		LAi_SetFightMode(pchar, true);
		pchar.DialogAmbushSneak.Unarmed = true;
	}
	if(!CheckAttribute(pchar, "DialogAmbushSneak.Unarmed")) DeleteAttribute(pchar, "DialogAmbushSneak");
}

void ProcessDialogEventWithAmbush()
{
	if(Dialog.CurrentNode == "DialogAmbush_Attack")
	{
		DialogAmbush_Start(false);
		return;
	}
	if(Dialog.CurrentNode == "DialogAmbush_Sneak")
	{
		DialogAmbush_Start(true);
		return;
	}
	ProcessDialogEvent();
	DialogAmbush_AppendLinks();
}

//Инициализация
void DialogsInit()
{
	//Quest_Init();				//Инициализация начального состояния слухов и информации об NPC ------- Ренат
	Set_inDialog_Attributes(); // boal
	SetEventHandler(EVENT_LOCATION_UNLOAD, "DialogAmbush_LocationUnload", 0);
	SetEventHandler("DialogAmbush_SneakStrike", "DialogAmbush_ApplySneakStrike", 0);
	SetEventHandler("DialogAmbush_SneakDraw", "DialogAmbush_DrawWeapon", 0);
}

//Esc выход без последствий
string DialogCancel_FreeExitNode()
{
	if(!CheckAttribute(&Dialog, "Links")) return "";
	aref Link;
	makearef(Link, Dialog.Links);
	int num = GetAttributesNum(Link);
	for(int i = 0; i < num; i++)
	{
		aref lnk = GetAttributeN(Link, i);
		if(!CheckAttribute(lnk, "go")) continue;
		string go = lnk.go;
		if(go == "exit" || go == "Exit") return go;
	}
	return "";
}

void DialogCancel_Exit()
{
	if(!dialogRun) return;
	string node = DialogCancel_FreeExitNode();
	if(node == "") return;
	Dialog.CurrentNode = node;
	Event("DialogEvent");
}

//Начать диалог
bool DialogMain(ref Character)
{
	//Если диалог запущен, выходим
	if(dialogRun != false) return false;
	//Ссылка на главного персонажа
	ref mainChr = GetMainCharacter();
	//Если когото не заведено, выходим
	if(!IsEntity(mainChr)) return false;
	if(!IsEntity(Character)) return false;
	if(LAi_IsDead(mainChr)) return false;
	if(LAi_IsDead(Character)) return false;
	//Проверим на существование текущего нода
	if(!CheckAttribute(Character, "Dialog.CurrentNode"))
	{
		Trace("Dialog: Character <" + Character.id + "> can't have field Dialog.CurrentNode, exit from dialog!")
		return false;
	}
	//Если персонаж не готов говорить выходим
	if(!LAi_Character_CanDialog(mainChr, Character)) return false;
	//Если персонаж не готов говорить выходим
	if(!LAi_Character_CanDialog(Character, mainChr)) return false;
	//Сохраняем ссыклу на того с кем говорим
	CharacterRef = Character;
	// Попытка загрузить текст дилога
	if( !LoadDialogFiles(Character.Dialog.Filename) ) {
		// имеем ошибочный диалог
		if( !LoadDialogFiles("error_dialog.c") ) {
			return false;
		}
	}
	//Можем начинать диалог
	DelPerkFromActiveList("TimeSpeed");
	dialogRun = true;
	dialogSelf = false;
	LAi_Character_StartDialog(mainChr, Character);
	LAi_Character_StartDialog(Character, mainChr);
	SendMessage(mainChr, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 1);
	SendMessage(Character, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 1);	
	//Запускаем диалог
	//Trace("Dialog: dialog path for character <" + Character.id + "> = " + FullDialogPath);
	Dialog.CurrentNode = CharacterRef.Dialog.CurrentNode;
	startDialogMainCounter = 0;
	SetEventHandler("frame", "StartDialogMain", 1);
	SetTimeScale(0.0);
	if (locCameraCurMode == LOCCAMERA_FOLLOW && !CheckAttribute(loadedLocation, "lockCamAngle") && mainChr.location.group != "sit") // для квестов
	{
		SetCameraDialogMode(Character);  // boal
	}
	return true;	
}

int startDialogMainCounter = 0;
int dialogWaitGreetingSound = 0;
string dialogGreetingSound = "";

void StartDialogMain()
{
	startDialogMainCounter++;
	if(startDialogMainCounter < 3) return;
	
	DelEventHandler("frame", "StartDialogMain");

	CreateEntity(&Dialog, "dialog");
	Dialog.headModel = CharacterRef.headModel;
	Dialog.gender = CharacterRef.sex;

	DeleteAttribute(&Dialog,"Links");
	DeleteAttribute(&Dialog,"Text");

	if(CheckAttribute(CharacterRef, "greeting"))
	{
		if(CharacterRef.greeting != "")
		{
			if (!CheckAttribute(CharacterRef, "greeting.minute"))
			{
				dialogGreetingSound = CharacterRef.greeting;
				dialogWaitGreetingSound = 0;
				SetEventHandler("frame", "DialogPlayGreeting", 1);
			}
			else
			{
				if (sti(CharacterRef.greeting.minute) != sti(Environment.date.min))
				{
					dialogGreetingSound = CharacterRef.greeting;
					dialogWaitGreetingSound = 0;
					SetEventHandler("frame", "DialogPlayGreeting", 1);
				}
			}
		}
	}

	object persRef = GetCharacterModel(Characters[GetMainCharacterIndex()]);
	SendMessage(&Dialog, "lii", 0, &Characters[GetMainCharacterIndex()], &persRef);

	object charRef = GetCharacterModel(Characters[makeint(CharacterRef.index)]);
	SendMessage(&Dialog, "lii", 1, &Characters[makeint(CharacterRef.index)], &charRef);
	
	LayerSetRealize(REALIZE);
	LayerAddObject(REALIZE,Dialog,-256);
	Set_inDialog_Attributes();
	ProcessDialogEventWithAmbush();

	SetEventHandler("DialogEvent","ProcessDialogEventWithAmbush",0);
	SetEventHandler("DialogCancel","DialogCancel_Exit",0);

	Event(EVENT_DIALOG_START,"");
}

void DialogPlayGreeting()
{
	dialogWaitGreetingSound++;
	if(dialogWaitGreetingSound < 10) return;
	dialogWaitGreetingSound = 0;
	DelEventHandler("frame", "DialogPlayGreeting");
	//Dialog.greeting = LanguageGetLanguage() + " " + CharacterRef.greeting;
	Dialog.greeting = CharacterRef.greeting;
	CharacterRef.greeting.minute = GetMinute();
	//Dialog.greeting = "Gr_Barmen";
}

//Начать диалог с самим собой
void SelfDialog(ref Character)
{
	//Если диалог запущен, выходим
	if(dialogRun != false) return false;
	//Если когото не заведено, выходим
	if(!IsEntity(Character)) return false;
	//Проверим на существование текущего нода
	if(!CheckAttribute(Character, "Dialog.CurrentNode"))
	{
		Trace("SelfDialog: Character <" + Character.id + "> can't have field Dialog.CurrentNode, exit from dialog!")
		return false;
	}
	//Сохраняем ссыклу на того с кем говорим
	CharacterRef = Character;
	// Попытка загрузить текст дилога
	if( !LoadDialogFiles(Character.Dialog.Filename) ) {
		// имеем ошибочный диалог
		if( !LoadDialogFiles("error_dialog.c") ) {
			return false;
		}
	}
	//Если персонаж не готов говорить выходим
	LAi_Character_CanDialog(Character, Character);
	//Можем начинать диалог
	DelPerkFromActiveList("TimeSpeed");
	dialogRun = true;
	dialogSelf = true;
	SendMessage(Character, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 1);
	//Запускаем диалог
	Dialog.CurrentNode = CharacterRef.Dialog.CurrentNode;
	//Trace("SelfDialog: dialog path for self character <" + Character.id + "> = " + FullDialogPath);
	CreateEntity(&Dialog, "dialog");
	Dialog.headModel = Character.headModel;
	Dialog.gender = Character.sex;

	object persRef = GetCharacterModel(Characters[GetMainCharacterIndex()]);
	SendMessage(&Dialog, "lii", 0, Character, &persRef);
	SendMessage(&Dialog, "lii", 1, Character, &persRef);
	
	LayerSetRealize(REALIZE);
	LayerAddObject(REALIZE,Dialog,-256);
	Set_inDialog_Attributes();
	ProcessDialogEventWithAmbush();

	SetEventHandler("DialogEvent","ProcessDialogEventWithAmbush",0);
	SetEventHandler("DialogCancel","DialogCancel_Exit",0);

	Event(EVENT_DIALOG_START,"");
}

//Закончить диалог
void DialogExit()
{
	//Если диалога уже не ведётся, выйдем
	if(dialogRun == false) return;
	DelEventHandler("frame", "DialogPlayGreeting");
	//Освобождаем ресурсы
	DeleteClass(&Dialog);
	if(FullDialogPath!="")
	{
		if (CheckAttribute(CharacterRef, "FileDialog2"))
		{
		    UnloadSegment(CharacterRef.FileDialog2);
		    // мешало выходу в сегменте DeleteAttribute(CharacterRef, "FileDialog2");
		    //DeleteAttribute(CharacterRef, "FileDialog2Ready");
		}
		UnloadSegment(FullDialogPath);
	}
	if(PathDlgLngExtn!="") UnloadSegment(PathDlgLngExtn);
	if(dialogSelf == false)
	{
		//Ссылка на главного персонажа
		ref mainChr = GetMainCharacter();
		//Отметим, что персонажи освободились от диалога
		LAi_Character_EndDialog(mainChr, CharacterRef);
		LAi_Character_EndDialog(CharacterRef, mainChr);
		SendMessage(mainChr, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 0);
		SendMessage(CharacterRef, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 0);
		// boal
		if (locCameraCurMode != LOCCAMERA_FOLLOW)
		{
			locCameraTarget(mainChr);
			locCameraFollow();
		}
	}else{
		LAi_Character_EndDialog(CharacterRef, CharacterRef);
		SendMessage(CharacterRef, "lsl", MSG_CHARACTER_EX_MSG, "InDialog", 0);
	}
	dialogRun = false;
	//Сообщим об окончании диалога
	PostEvent(EVENT_DIALOG_EXIT, 1, "l", sti(CharacterRef.index));
}

//Это событие приходит от Player
void StartDialogWithMainCharacter()
{
	if(LAi_IsBoardingProcess()) return;
	if(dialogDisable) return;
	//С кем хотим говорить
	int person = GetEventData();
	//Сими с собой не беседуем
	if(person == GetMainCharacterIndex()) return;
	//С непрогруженными персонажами не беседуем
	if(!IsEntity(&Characters[person])) return;
	//Начинаем диалог
	DialogMain(&Characters[person]);	
	//Trace("Dialog: start dialog " + person + " whith main character");
}

bool LoadDialogFiles(string dialogPath)
{
	//FullDialogPath = "dialogs/" + dialogPath;
	FullDialogPath = "dialogs\" + LanguageGetLanguage() + "\" + dialogPath;

	// Выбор директории с языковыми файлами
	//string sLanguageDir = "dialogs\" + LanguageGetLanguage() + "\";
	//Путь до текста диалога
	/*int iTmp = strlen(dialogPath);
	if(iTmp<3)
	{
		Trace("Dialog: Missing dialog file: " + dialogPath);
		return false;
	}*/
	//PathDlgLngExtn = strcut(dialogPath,0,iTmp-2) + "h";

	//bool retVal = LoadSegment(PathDlgLngExtn);
	bool retVal;
	
	if( !LoadSegment(FullDialogPath) )
	{
		Trace("Dialog: Missing dialog file: " + FullDialogPath);
		retVal = false;
		// лишнее это UnloadSegment(FullDialogPath);
	} else {
		if(!retVal) {
			retVal = true;
			PathDlgLngExtn = "";
		}
	}

	return retVal;
}


string DText(string sString)
{
	return sString;
}

// boal -->
bool SetCameraDialogMode(ref chrRef)
{
	float x1,y1,z1, x2, y2, z2;
	if( false==GetCharacterPos(pchar,&x1,&y1,&z1) ) return false;
    if( false==GetCharacterPos(chrRef,&x2,&y2,&z2) ) return false;

    float a = 0.1;
    float len = GetDistance2D(x1,z1, x2,z2);
	float dx = x1*(1-a)+x2*a;
	float dz = z1*(1-a)+z2*a;
	len = 1;
	float s1 = (dx-x1)*len;
	float s2 = (dz-z1)*len;

    float xcam;
	float zcam;
	if (rand(1) == 0)
	{
		xcam = dx-s2;
		zcam = dz+s1;
	}
	else
	{
		xcam = dx+s2;
		zcam = dz-s1;
	}
	locCameraTarget(chrRef);
	float fH = 1.7;
	if (chrRef.location.group == "sit")
	{
	    fH = 1.15;
	}
	return locCameraToPos(xcam,y1+fH,zcam,false);
}
// boal <--
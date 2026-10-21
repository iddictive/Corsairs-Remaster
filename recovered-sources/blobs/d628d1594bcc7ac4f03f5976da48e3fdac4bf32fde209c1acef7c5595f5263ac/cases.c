#define USE_AXIS_AS_BUTTON 1
object objControlsState,Profile;
int Loads=0;
void RunControlsContainers(){}
void LoadGameOptions(){aref saved;makearef(saved,Profile.controls);RestoreKeysFromOptions(saved);Loads++;}
void OnLoad(){}
void DeckWalk_DefaultShipKeys()
{
    CI_CreateAndSetControls("Sailing1Pers", "Ship_TurnLeft1", CI_GetKeyCode("VK_LEFT"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_TurnRight1", CI_GetKeyCode("VK_RIGHT"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_SailUp1", CI_GetKeyCode("VK_UP"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_SailDown1", CI_GetKeyCode("VK_DOWN"), 0, true);
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_TurnLeft");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_TurnRight");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_SailUp");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_SailDown");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.DeckCamera_Forward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.DeckCamera_Backward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.ShipCamera_Forward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.ShipCamera_Backward");
}
void RebuildControlsRegistry()
{
    aref arControls;
    int i;
    makearef(arControls, objControlsState.map.controls);
    for (i = 0; i < GetAttributesNum(arControls); i++)
    {
        aref arControl = GetAttributeN(arControls, i);
        string controlName = GetAttributeName(arControl);
        objControlsState.map.controls.(controlName) = CreateControl(controlName);
    }
}
void RestoreKeysFromOptions(aref arControlsRoot)
{
	aref arRootKey,arKey;
	int nGroupQ,nKeyQ, i,j, state,ctrlCode,keyCode;
	string ctrlName,grName;

	// Savegames persist numeric script IDs, but the engine registry is rebuilt by name.
	RebuildControlsRegistry();
	nGroupQ = GetAttributesNum(arControlsRoot);

	for(i=0; i<nGroupQ; i++)
	{
		arRootKey = GetAttributeN(arControlsRoot,i);
		grName = GetAttributeName(arRootKey);
		nKeyQ = GetAttributesNum(arRootKey);
		for(j=0; j<nKeyQ; j++)
		{
			arKey = GetAttributeN(arRootKey,j);
			state = 0;
			if(CheckAttribute(arKey,"state"))
			{
				state = sti(arKey.state);
			}

			ctrlName = GetAttributeName(arKey);
			keyCode = CI_GetKeyCode(GetAttributeValue(arKey));
			// Legacy deck ship names share global bindings with the outside view.
			if (grName == "sailing1pers" && (ctrlName == "Ship_TurnLeft" || ctrlName == "Ship_TurnRight" || ctrlName == "Ship_SailUp" || ctrlName == "Ship_SailDown" || ctrlName == "DeckCamera_Forward" || ctrlName == "DeckCamera_Backward" || ctrlName == "ShipCamera_Forward" || ctrlName == "ShipCamera_Backward" || ctrlName == "DeckWalk_Forward" || ctrlName == "DeckWalk_Backward" || ctrlName == "DeckWalk_Left" || ctrlName == "DeckWalk_Right" || ctrlName == "DeckWalk_Run"))
			{
				continue;
			}
			CI_CreateAndSetControls( grName, ctrlName, keyCode, state, true );
		}
	}

	// Old profiles used WASD for the ship on deck. Migrate only once.
	if (!CheckAttribute(arControlsRoot, "sailing1pers.Ship_TurnLeft1"))
	{
		DeckWalk_DefaultShipKeys();
	}
	// The registry was rebuilt above; refresh the deck group's numeric IDs from canonical land controls.
	MapControlToGroup("ChrForward", "Sailing1Pers");
	MapControlToGroup("ChrBackward", "Sailing1Pers");
	MapControlToGroup("ChrStrafeLeft", "Sailing1Pers");
	MapControlToGroup("ChrStrafeRight", "Sailing1Pers");
	MapControlToGroup("ChrRun", "Sailing1Pers");
	// Refresh the fixed aim binding even for profiles saved before it existed.
	CI_CreateAndSetControls("Sailing1Pers", "DeckAimZoom", CI_GetKeyCode("VK_RBUTTON"), 0, false);
	RunControlsContainers();
}
string CI_CreateAndSetControls( string groupName, string controlName, int keyCode, int controlState, bool bRemappingEnable )
{
	if(controlName=="") return "";
	string retString = "";

	// CreateControl is name-idempotent; never trust a numeric ID restored from a save.
	int cntrlCode = CreateControl(controlName);
	objControlsState.map.controls.(controlName) = cntrlCode;

	if(keyCode>=0)
	{	string keyName = "key_" + keyCode;
		objControlsState.map.keys.(keyName) = controlName;
	}
	MapControl(cntrlCode,keyCode);

	if(keyCode<0 && controlState==0)
	{	controlState = USE_AXIS_AS_BUTTON;
	}

	if(controlState!=-1)
	{	SetControlFlags(cntrlCode,controlState);
	}
	else
	{	controlState=0;
	}

	if(groupName!="")
	{
		if( CheckAttribute(&objControlsState,"keygroups."+groupName+"."+controlName) )
		{	retString = objControlsState.keygroups.(groupName).(controlName);
		}
		else
		{	objControlsState.keygroups.(groupName).(controlName) = "";
		}
		SetControlsByKey(controlName,CI_GetKeyName(keyCode),controlState);
		objControlsState.keygroups.(groupName).(controlName).remapping = bRemappingEnable;
	}

	return retString;
}
void SetControlsByKey(string controlName, string keyName, int state)
{
	int i,nGQ;
	aref arKGRoot, arKG;

	makearef(arKGRoot,objControlsState.keygroups);
	nGQ = GetAttributesNum(arKGRoot);

	for(i=0; i<nGQ; i++)
	{
		arKG = GetAttributeN(arKGRoot,i);
		if( CheckAttribute(arKG,controlName) )
		{	arKG.(controlName) = keyName;
			arKG.(controlName).state = state;
		}
	}
}
void MapControlToGroup(string controlName,string groupName)
{
	// Найдем использование контрола
	int i,nRootSize;
	aref arRoot,arGroup;

	makearef(arRoot,objControlsState.keygroups);
	nRootSize = GetAttributesNum(arRoot);

	for(i=0; i<nRootSize; i++)
	{
		arGroup = GetAttributeN(arRoot,i);
		if( CheckAttribute(arGroup,controlName) )
		{
			objControlsState.keygroups.(groupName).(controlName) = arGroup.(controlName);
			objControlsState.keygroups.(groupName).(controlName).state = arGroup.(controlName).state;
			if( CheckAttribute(arGroup,controlName+".invisible") ) {
				objControlsState.keygroups.(groupName).(controlName).invisible = arGroup.(controlName).invisible;
			}
			if( CheckAttribute(arGroup,controlName+".remapping") ) {
				objControlsState.keygroups.(groupName).(controlName).remapping = arGroup.(controlName).remapping;
			}
			return;
		}
	}
}
int CI_GetKeyCode(string keyName)
{
	if( CheckAttribute(&objControlsState,"key_codes."+keyName) )
	{	return sti(objControlsState.key_codes.(keyName));
	}

	trace("Can`t key named as: " + keyName);
	return -1;
}
string CI_GetKeyName(int code)
{
	aref arKeys,arCur;
	makearef(arKeys,objControlsState.key_codes);
	int nq = GetAttributesNum(arKeys);

	for(int i=0; i<nq; i++)
	{
		arCur = GetAttributeN(arKeys,i);
		if( sti(GetAttributeValue(arCur)) == code )
		{	return GetAttributeName(arCur);
		}
	}

	return "";
}
int ControlNameToCode(string cname)
{
	if( CheckAttribute(&objControlsState,"map.controls."+cname) )
	{	return sti(objControlsState.map.controls.(cname));
	}
	return -1;
}
void SeaPhonk_InitControls()
{
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Laser"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Laser", CI_GetKeyCode("KEY_6"), 0, true);
        MapControlToGroup("Phonk_Laser", "Sailing1Pers");
        MapControlToGroup("Phonk_Laser", "BattleInterfaceControls");
    }
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.Phonk_Nuclear"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "Phonk_Nuclear", CI_GetKeyCode("KEY_7"), 0, true);
        MapControlToGroup("Phonk_Nuclear", "Sailing1Pers");
        MapControlToGroup("Phonk_Nuclear", "BattleInterfaceControls");
    }
}
void InitBattleInterface()
{
    SeaPhonk_InitControls();
    if (!CheckAttribute(&objControlsState, "keygroups.Sailing3Pers.hk_charge5"))
    {
        CI_CreateAndSetControls("Sailing3Pers", "hk_charge5", CI_GetKeyCode("KEY_5"), 0, true);
        MapControlToGroup("hk_charge5", "Sailing1Pers");
        MapControlToGroup("hk_charge5", "BattleInterfaceControls");
    }
}

void SequenceAfterOptions(){
LoadGameOptions();
	SeaPhonk_InitControls();

	
}
void FixtureKeys(){
 objControlsState.key_codes.KEY_5=53;objControlsState.key_codes.KEY_6=54;objControlsState.key_codes.KEY_7=55;
 objControlsState.key_codes.KEY_8=56;objControlsState.key_codes.KEY_9=57;objControlsState.key_codes.KEY_SPACE=32;
 objControlsState.key_codes.VK_LEFT=37;objControlsState.key_codes.VK_RIGHT=39;
 objControlsState.key_codes.VK_UP=38;objControlsState.key_codes.VK_DOWN=40;objControlsState.key_codes.VK_RBUTTON=2;
}
int ControlsCases(){
 FixtureKeys();
 objControlsState.map.controls.Ship_Fire=99999;
 objControlsState.keygroups.Sailing3Pers.Ship_Fire="KEY_SPACE";
 Profile.controls.Sailing3Pers.Ship_Fire="KEY_SPACE";
 InitBattleInterface();
 if(objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_6")return 1;
 SequenceAfterOptions();
 if(Loads!=1 || objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_6" || objControlsState.keygroups.Sailing3Pers.Phonk_Nuclear!="KEY_7")return 2;
 if(objControlsState.keygroups.Sailing1Pers.Phonk_Laser!="KEY_6" || objControlsState.keygroups.BattleInterfaceControls.Phonk_Nuclear!="KEY_7")return 3;
 if(ControlNameToCode("Phonk_Laser")!=CreateControl("Phonk_Laser") || ControlNameToCode("Phonk_Nuclear")!=CreateControl("Phonk_Nuclear"))return 4;
 if(objControlsState.keygroups.Sailing3Pers.Ship_Fire!="KEY_SPACE" || ControlNameToCode("Ship_Fire")!=CreateControl("Ship_Fire"))return 5;
 SeaPhonk_InitControls();InitBattleInterface();
 if(objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_6" || objControlsState.keygroups.Sailing3Pers.Ship_Fire!="KEY_SPACE")return 6;
 // Normal remapped profile restored after the sea interface's earlier defaults.
 Profile.controls.Sailing3Pers.Phonk_Laser="KEY_9";Profile.controls.Sailing3Pers.Phonk_Nuclear="KEY_8";
 objControlsState.map.controls.Phonk_Laser=99998;objControlsState.map.controls.Phonk_Nuclear=99997;
 SequenceAfterOptions();
 if(objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_9" || objControlsState.keygroups.Sailing3Pers.Phonk_Nuclear!="KEY_8")return 7;
 if(objControlsState.keygroups.Sailing1Pers.Phonk_Laser!="KEY_9" || objControlsState.keygroups.BattleInterfaceControls.Phonk_Nuclear!="KEY_8")return 8;
 SeaPhonk_InitControls();InitBattleInterface();
 if(objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_9" || objControlsState.keygroups.Sailing3Pers.Phonk_Nuclear!="KEY_8")return 9;
 if(ControlNameToCode("Phonk_Laser")!=CreateControl("Phonk_Laser") || ControlNameToCode("Phonk_Nuclear")!=CreateControl("Phonk_Nuclear"))return 10;
 // Map/land old-save branch has no battle interface before options restoration.
 DeleteAttribute(&objControlsState,"keygroups.Sailing3Pers.Phonk_Laser");DeleteAttribute(&objControlsState,"keygroups.Sailing3Pers.Phonk_Nuclear");
 DeleteAttribute(&objControlsState,"keygroups.Sailing1Pers.Phonk_Laser");DeleteAttribute(&objControlsState,"keygroups.Sailing1Pers.Phonk_Nuclear");
 DeleteAttribute(&objControlsState,"keygroups.BattleInterfaceControls.Phonk_Laser");DeleteAttribute(&objControlsState,"keygroups.BattleInterfaceControls.Phonk_Nuclear");
 DeleteAttribute(&Profile,"controls.Sailing3Pers.Phonk_Laser");DeleteAttribute(&Profile,"controls.Sailing3Pers.Phonk_Nuclear");
 SequenceAfterOptions();
 if(objControlsState.keygroups.Sailing3Pers.Phonk_Laser!="KEY_6" || objControlsState.keygroups.Sailing3Pers.Phonk_Nuclear!="KEY_7")return 11;
 if(objControlsState.keygroups.Sailing3Pers.Ship_Fire!="KEY_SPACE")return 12;
 return 0;
}

int g_nCurControlsMode = -1;
int g_ControlsLngFile = -1;

bool g_bToolTipStarted = false;

float 	fHUDRatio 	= 1.0;
int 	iHUDBase 	= screenscaling;
int 	newBase 	= screenscaling;

int metalGraphicsSnapshotDynamicLighting;
int metalGraphicsSnapshotShadowQuality;
int metalGraphicsSnapshotModernWater;
int metalGraphicsSnapshotModernLighting;
int metalGraphicsSnapshotCinematic;
int metalGraphicsSnapshotLightShafts;
int metalGraphicsSnapshotDynamicSky;
int metalGraphicsSnapshotAntialiasing;
int metalGraphicsSnapshotFullScreen;
int metalGraphicsSnapshotResolution;
int metalGraphicsSnapshotResolutionWidth;
int metalGraphicsSnapshotResolutionHeight;
bool metalGraphicsEditing = false;
int metalGraphicsDesktopWidth = 1920;
int metalGraphicsDesktopHeight = 1080;
int metalGraphicsPendingFlags = 0;
int metalGraphicsPendingShadowQuality = 1;
int metalGraphicsPendingFullscreen = 1;
int metalGraphicsPendingWidth = 1920;
int metalGraphicsPendingHeight = 1080;
int metalGraphicsPendingDesktop = 0;

#define BI_LOW_RATIO 	0.25
#define BI_HI_RATIO 	4.0
#define BI_DIF_RATIO 	3.75

int CalcHUDBase(float fSlider, float MyScreen)
{
    float fRes = BI_DIF_RATIO * fSlider;
    float curBase = MyScreen / (BI_LOW_RATIO + fRes);

    return makeint(curBase + 0.5);
}

float CalcHUDSlider(float fRatio)
{
    float fRes = fRatio - BI_LOW_RATIO;
    fRes /= BI_DIF_RATIO;

    return fRes;
}

void InitInterface(string iniName)
{
	fHUDRatio = stf(Render.screen_y) / screenscaling;
    iHUDBase = makeint(screenscaling);
    newBase = iHUDBase;
	
	float glowEffect;

	g_nCurControlsMode = -1;
	GameInterface.title = "titleOptions";
	g_ControlsLngFile = LanguageOpenFile("ControlsNames.txt");

	if( CheckAttribute(&InterfaceStates,"showGameMenuOnExit") && sti(InterfaceStates.showGameMenuOnExit) == true) {
		LoadGameOptions();
	} else {
		DeleteAttribute( &PlayerProfile, "name" );
		LoadGameOptions();
	}

	IReadVariableBeforeInit();
	SendMessage(&GameInterface,"ls",MSG_INTERFACE_INIT,iniName);
	IReadVariableAfterInit();

	SetControlsTabMode(1);

	SetEventHandler("exitCancel","ProcessCancelExit",0);
	SetEventHandler("InterfaceBreak","ProcessCancelExit",0);  // boal
	SetEventHandler("eTabControlPress","procTabChange",0);
	SetEventHandler("eventBtnAction","procBtnAction",0);
	SetEventHandler("eventKeyChange","procKeyChange",0);

	SetEventHandler("CheckButtonChange","procCheckBoxChange",0);
	SetEventHandler("eSlideChange","procSlideChange",0);

	SetEventHandler("evntKeyChoose","procKeyChoose",0);
	SetEventHandler("ShowInfo", "ShowInfo", 0);
	SetEventHandler("MouseRClickUP","HideInfo",0);

	SetEventHandler("evFaderFrame","FaderFrame",0);
	SetEventHandler("ievnt_command","ProcessMetalGraphicsCommand",0);
	SetEventHandler("OnTableClick","ClickMetalGraphicsRow",0);
	SetEventHandler("TableActivate","ActivateMetalGraphicsRow",0);
	SetEventHandler("evApplyMetalGraphicsDeferred","ApplyMetalGraphicsDeferred",0);
	SetEventHandler("evPollMetalGraphicsApply","PollMetalGraphicsApply",0);

	aref ar; makearef(ar,objControlsState.key_codes);
	SendMessage(&GameInterface,"lsla",MSG_INTERFACE_MSG_TO_NODE,"KEY_CHOOSER", 0,ar);
	
	// Hokkins: в движке добавили  эти настройки для оконного режима, нет смысла больше их блокировать -->
	
	/* if( sti(Render.full_screen)==0 )
	{
		SetSelectable("GAMMA_SLIDE",false);
		SetSelectable("BRIGHT_SLIDE",false);
		SetSelectable("CONTRAST_SLIDE",false);
	} */
	
	// Hokkins: <--

	float ftmp1 = -1.0;
	float ftmp2 = -1.0;
	float ftmp3 = -1.0;
	SendMessage(&sound,"leee",MSG_SOUND_GET_MASTER_VOLUME,&ftmp1,&ftmp2,&ftmp3);
	if( ftmp1==-1.0 && ftmp2==-1.0 && ftmp3==-1.0 )
	{
		SetSelectable("MUSIC_SLIDE",false);
		SetSelectable("SOUND_SLIDE",false);
		SetSelectable("DIALOG_SLIDE",false);
	}
	
	// Warship 07.07.09 Эффект свечения
	if(!CheckAttribute(&InterfaceStates, "GlowEffect"))
	{
		InterfaceStates.GlowEffect = 50;
	}
	
	glowEffect = sti(InterfaceStates.GlowEffect) / 250.0; // ≈сли делить на 250, то хер, т.к. целое, а если на 250.0 - все гуд
	
	GameInterface.nodes.GLOW_SLIDE.value = glowEffect;
	SendMessage(&GameInterface, "lslf", MSG_INTERFACE_MSG_TO_NODE, "GLOW_SLIDE", 0, glowEffect);
	
	fHUDRatio = stf(Render.screen_y) / screenscaling;
	float sl = CalcHUDSlider(fHUDRatio);
	SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE, "HUD_SLIDE", 2, makeint(sl * 100.0));
	GameInterface.nodes.hud_slide.value = sl;
	iHUDBase = CalcHUDBase(sl, stf(Render.screen_y));
	SetFormatedText("HUD_DESCRIP_TEXT","Размер интерфейса: " +  Render.screen_y + "  / " + newBase + " : " + fHUDRatio);
}

void ProcessCancelExit()
{
	if(metalGraphicsEditing) {
		RestoreMetalGraphicsOptions();
		CloseMetalGraphicsWindow();
		return;
	}
	LoadGameOptions();
	ProcessExit();
}

void ProcessOkExit()
{
	screenscaling = newBase;
	
	// Warship 07.07.09 Эффект свечения
	SetGlowParams(1.0, sti(InterfaceStates.GlowEffect), 2));

	SaveGameOptions();
	ProcessExit();
	Event("eventChangeOption");

	// change sea settings
	SetSeaGridStep(stf(InterfaceStates.SeaDetails));
	//Hokkins: перспектива на море
	SetPerspectiveSettings();
}

void ProcessExit()
{
	if( CheckAttribute(&InterfaceStates,"showGameMenuOnExit") && sti(InterfaceStates.showGameMenuOnExit) == true) {
		IDoExit(RC_INTERFACE_LAUNCH_GAMEMENU);
		return;
	}

	IDoExit(RC_INTERFACE_OPTIONSCREEN_EXIT);
	if( !CheckAttribute(&InterfaceStates,"InstantExit") || sti(InterfaceStates.InstantExit)==false ) {
		ReturnToMainMenu();
	}
}

void IDoExit(int exitCode)
{
	DelEventHandler("evntKeyChoose","procKeyChoose");
	DelEventHandler("eSlideChange","procSlideChange");
	DelEventHandler("CheckButtonChange","procCheckBoxChange");

	DelEventHandler("eventKeyChange","procKeyChange");
	DelEventHandler("eventBtnAction","procBtnAction");
	DelEventHandler("eTabControlPress","procTabChange");
	DelEventHandler("exitCancel","ProcessCancelExit");
	DelEventHandler("ShowInfo", "ShowInfo");
	DelEventHandler("MouseRClickUP","HideInfo");
	DelEventHandler("evFaderFrame","FaderFrame");
	DelEventHandler("ievnt_command","ProcessMetalGraphicsCommand");
	DelEventHandler("OnTableClick","ClickMetalGraphicsRow");
	DelEventHandler("TableActivate","ActivateMetalGraphicsRow");
	DelEventHandler("evApplyMetalGraphicsDeferred","ApplyMetalGraphicsDeferred");
	DelEventHandler("evPollMetalGraphicsApply","PollMetalGraphicsApply");
	DelEventHandler("InterfaceBreak","ProcessCancelExit");  // boal

	LanguageCloseFile( g_ControlsLngFile );

	interfaceResultCommand = exitCode;
	if( CheckAttribute(&InterfaceStates,"InstantExit") && sti(InterfaceStates.InstantExit)==true ) {
		EndCancelInterface(true);
	} else {
		EndCancelInterface(false);
	}
	ControlsMakeInvert();
}

void IReadVariableBeforeInit()
{
	GetSoundOptionsData();
	GetMouseOptionsData();
	GetVideoOptionsData();
}

void IReadVariableAfterInit()
{
	GetMetalGraphicsData();
	GetHerbOptionsData();
	GetControlsStatesData();
	
	int nEnabledSimpleSea = 0;
	if( CheckAttribute(&InterfaceStates,"SimpleSea") ) 
	{
		nEnabledSimpleSea = sti(InterfaceStates.SimpleSea);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"SIMPLESEA_CHECKBOX", 2, 1, nEnabledSimpleSea );

	int nShowBattleMode = 0;
	if( CheckAttribute(&InterfaceStates,"ShowBattleMode") ) {
		nShowBattleMode = sti(InterfaceStates.ShowBattleMode);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"BATTLE_MODE_CHECKBOX", 2, 1, nShowBattleMode );
	
	int nSkipStartVideo = 0;
	if( CheckAttribute(&InterfaceStates,"SkipStartVideo") ) {
		nSkipStartVideo = sti(InterfaceStates.SkipStartVideo);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"SKIPVIDEO_CHECKBOX", 2, 1, nSkipStartVideo );
	
	int nEnabledShipMarks = 1;
	if( CheckAttribute(&InterfaceStates,"EnabledShipMarks") ) {
		nEnabledShipMarks = sti(InterfaceStates.EnabledShipMarks);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"SHIPMARK_CHECKBOX", 2, 1, nEnabledShipMarks );

	int nEnabledAutoSaveMode = 1;
	if( CheckAttribute(&InterfaceStates,"EnabledAutoSaveMode") ) {
		nEnabledAutoSaveMode = sti(InterfaceStates.EnabledAutoSaveMode);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"AUTOSAVE_CHECKBOX", 2, 1, nEnabledAutoSaveMode );
}

void SetControlsTabMode(int nMode)
{
	int nColor1 = argb(255,196,196,196);
	int nColor2 = nColor1;
	int nColor3 = nColor1;
	int nColor4 = nColor1;

	string sPic1 = "TabSelected";
	string sPic2 = sPic1;
	string sPic3 = sPic1;
	string sPic4 = sPic1;

	switch( nMode )
	{
	case 1: // море от первого лица
		sPic1 = "TabDeSelected";
		nColor1 = argb(255,255,255,255);
	break;
	case 2: // режим путешествий на земле
		sPic2 = "TabDeSelected";
		nColor2 = argb(255,255,255,255);
	break;
	case 3: // море от 3-го лица
		sPic3 = "TabDeSelected";
		nColor3 = argb(255,255,255,255);
	break;
	case 4: // режим боя на земле
		sPic4 = "TabDeSelected";
		nColor4 = argb(255,255,255,255);
	break;
	}

	SetNewGroupPicture("TABBTN_SAILING_1ST", "TABS", sPic1);
	SetNewGroupPicture("TABBTN_PRIMARY_LAND", "TABS", sPic2);
	SetNewGroupPicture("TABBTN_SAILING_3RD", "TABS", sPic3);
	SetNewGroupPicture("TABBTN_FIGHT_MODE", "TABS", sPic4);
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"TABSTR_SAILING_1ST", 8,0,nColor1);
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"TABSTR_PRIMARY_LAND", 8,0,nColor2);
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"TABSTR_SAILING_3RD", 8,0,nColor3);
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"TABSTR_FIGHT_MODE", 8,0,nColor4);

	FillControlsList(nMode);
}

void procTabChange()
{
	int iComIndex = GetEventData();
	string sNodName = GetEventData();

	if( sNodName == "TABBTN_SAILING_1ST" ) {
		SetControlsTabMode( 1 );
		return;
	}
	if( sNodName == "TABBTN_PRIMARY_LAND" ) {
		SetControlsTabMode( 2 );
		return;
	}
	if( sNodName == "TABBTN_SAILING_3RD" ) {
		SetControlsTabMode( 3 );
		return;
	}
	if( sNodName == "TABBTN_FIGHT_MODE" ) {
		SetControlsTabMode( 4 );
		return;
	}
}

void procBtnAction()
{
	int iComIndex = GetEventData();
	string sNodName = GetEventData();

	if( sNodName == "BTN_OK" ) {
		if( iComIndex==ACTION_ACTIVATE || iComIndex==ACTION_MOUSECLICK ) {
			ProcessOkExit();
		}
		return;
	}
	if( sNodName == "BTN_CONTROLS_DEFAULT" ) {
		RestoreDefaultKeys();
		return;
	}
	if( sNodName == "METAL_GRAPHICS_BTN" ) {
		SnapshotMetalGraphicsOptions();
		metalGraphicsEditing = true;
		SetFormatedText("METAL_GRAPHICS_ERROR", "");
		XI_WindowDisable("MAIN_WINDOW", true);
		XI_WindowShow("METAL_GRAPHICS_WINDOW", true);
		XI_WindowDisable("METAL_GRAPHICS_WINDOW", false);
		SetCurrentNode("METAL_GRAPHICS_LIST");
		return;
	}
	if( sNodName == "METAL_GRAPHICS_APPLY" ) {
		ApplyMetalGraphicsOptions();
		return;
	}
	if( sNodName == "METAL_GRAPHICS_CANCEL" ) {
		RestoreMetalGraphicsOptions();
		CloseMetalGraphicsWindow();
		return;
	}
}

void procCheckBoxChange()
{
	string sNodName = GetEventData();
	int nBtnIndex = GetEventData();
	int bBtnState = GetEventData();

	if( sNodName == "HERB_CHECKBOX" ) {
		if( bBtnState==true ) {
			switch( nBtnIndex ) {
			case 1: iGrassQuality=3; break;
			case 2: iGrassQuality=2; break;
			case 3: iGrassQuality=1; break;
			case 4: iGrassQuality=0; break;
			}
		}
		return;
	}
	
	if( sNodName == "ALWAYS_RUN_CHECKBOX" ) 
	{
		{ // always run
			SetAlwaysRun( bBtnState );
		}
	}
	
	if( sNodName == "INVERT_MOUSE_CHECKBOX" ) 
	{
		{ // invert mouse
			InterfaceStates.InvertCameras = bBtnState;
		}
	}
	
	if( sNodName == "SIMPLESEA_CHECKBOX" ) 
	{
		{ // Show battle mode border
			InterfaceStates.SimpleSea = bBtnState;
		}
	}
	
	if( sNodName == "BATTLE_MODE_CHECKBOX" ) 
	{
		{ // Show battle mode border
			InterfaceStates.ShowBattleMode = bBtnState;
		}
	}
	
	if( sNodName == "SKIPVIDEO_CHECKBOX" ) 
	{
		{ // Skip Start Video
			InterfaceStates.SkipStartVideo = bBtnState;
		}
	}
	
	if( sNodName == "SHIPMARK_CHECKBOX" ) 
	{
		{ // Show battle mode border
			InterfaceStates.EnabledShipMarks = bBtnState;
		}
	}
	
	if( sNodName == "AUTOSAVE_CHECKBOX" ) 
	{
		{ // Show battle mode border
			InterfaceStates.EnabledAutoSaveMode = bBtnState;
		}
	}
	if( sNodName == "METAL_DYNAMIC_LIGHTING_CHECKBOX" ) InterfaceStates.MetalDynamicLighting = bBtnState;
	if( sNodName == "METAL_SHADOW_QUALITY_RADIO" && bBtnState ) InterfaceStates.MetalShadowQuality = nBtnIndex - 1;
	if( sNodName == "METAL_MODERN_WATER_CHECKBOX" ) InterfaceStates.MetalModernWater = bBtnState;
	if( sNodName == "METAL_MODERN_LIGHTING_CHECKBOX" ) InterfaceStates.MetalModernLighting = bBtnState;
	if( sNodName == "METAL_CINEMATIC_CHECKBOX" ) InterfaceStates.MetalCinematic = bBtnState;
	if( sNodName == "METAL_LIGHT_SHAFTS_CHECKBOX" ) InterfaceStates.MetalLightShafts = bBtnState;
	if( sNodName == "METAL_DYNAMIC_SKY_CHECKBOX" ) InterfaceStates.MetalDynamicSky = bBtnState;
	if( sNodName == "METAL_ANTIALIASING_CHECKBOX" ) InterfaceStates.MetalAntialiasing = bBtnState;
	if( sNodName == "METAL_FULLSCREEN_CHECKBOX" ) InterfaceStates.MetalFullScreen = bBtnState;
	// Resolution is owned by the bounded previous/next picker.
}

void procSlideChange()
{
	string sNodeName = GetEventData();
	int nVal = GetEventData();
	float fVal = GetEventData();

	if( sNodeName=="GAMMA_SLIDE" || sNodeName=="BRIGHT_SLIDE" || sNodeName=="CONTRAST_SLIDE" ) 
	{
		ChangeVideoColor();
		return;
	}
	
	// Warship 07.07.09 Эффект свечения
	if(sNodeName == "GLOW_SLIDE")
	{
		InterfaceStates.GlowEffect = fVal*250;
		return;
	}
	
	if( sNodeName == "SEA_DETAILS_SLIDE" ) 
	{
		ChangeSeaDetail();
		return;
	}
	
	// Hokkins: настройки камеры -->
	if( sNodeName == "SEA_CAM_PERSP_SLIDE" ) 
	{
		ChangePerspDetail();
		return;
	}
	
	if( sNodeName == "LAND_CAM_RAD_SLIDE" ) 
	{
		ChangeRadDetail();
		return;
	}
	//Hokkins: настройки камеры <--
	
	if( sNodeName == "HUD_SLIDE" ) {
		ChangeHUDDetail();
		return;
	}
	
	if( sNodeName=="MUSIC_SLIDE" || sNodeName=="SOUND_SLIDE" || sNodeName=="DIALOG_SLIDE" ) 
	{
		ChangeSoundSetting();
		return;
	}
	
	if( sNodeName=="VMOUSE_SENSITIVITY_SLIDE" || sNodeName=="HMOUSE_SENSITIVITY_SLIDE" )
	{
		ChangeMouseSensitivity();
	}
}

void ChangeMouseSensitivity()
{
	InterfaceStates.mouse.x_sens = stf(GameInterface.nodes.hmouse_sensitivity_slide.value);
	InterfaceStates.mouse.y_sens = stf(GameInterface.nodes.vmouse_sensitivity_slide.value);
	SetRealMouseSensitivity();
}

void ChangeVideoColor()
{
	float fCurContrast = stf(GameInterface.nodes.contrast_slide.value);
	float fCurGamma = stf(GameInterface.nodes.GAMMA_SLIDE.value);
	float fCurBright = stf(GameInterface.nodes.BRIGHT_SLIDE.value);

	float fContrast = ConvertContrast(fCurContrast,false);
	float fGamma = ConvertGamma(fCurGamma,false);
	float fBright = ConvertBright(fCurBright,false);

	if( !CheckAttribute(&InterfaceStates,"video.contrast") ||
		(stf(InterfaceStates.video.contrast)!=fContrast) ||
		(stf(InterfaceStates.video.gamma)!=fGamma) ||
		(stf(InterfaceStates.video.brightness)!=fBright) ) {
			InterfaceStates.video.contrast = fContrast;
			InterfaceStates.video.gamma = fGamma;
			InterfaceStates.video.brightness = fBright;
			XI_SetColorCorrection(fContrast,fGamma,fBright);
	}
}

void ChangeSeaDetail()
{
	float fCurSeaDetail = stf(GameInterface.nodes.sea_details_slide.value);
	float fSeaDetail = ConvertSeaDetails(fCurSeaDetail,false);
	if( !CheckAttribute(&InterfaceStates,"SeaDetails") ||
		(stf(InterfaceStates.SeaDetails)!=fSeaDetail) ) {
			InterfaceStates.SeaDetails = fSeaDetail;
	}
}

//Hokkins: настройки камеры -->
void ChangePerspDetail()
{
    float fCurPerspDetail = stf(GameInterface.nodes.sea_cam_persp_slide.value);
	float fPerspDetail = ConvertPerspDetails(fCurPerspDetail,false);
	if( !CheckAttribute(&InterfaceStates,"PerspDetails") ||
		(stf(InterfaceStates.PerspDetails)!=fPerspDetail) ) {
			InterfaceStates.PerspDetails = fPerspDetail;
			float fCamPersp = CalcSeaPerspective();
            string sMsg = "#"+ FloatToString(fCamPersp, 3);
	}
}

void ChangeRadDetail()
{
    float fCurPerspDetail = stf(GameInterface.nodes.land_cam_rad_slide.value);
	float fPerspDetail = ConvertRadDetails(fCurPerspDetail,false);
	if( !CheckAttribute(&InterfaceStates,"RadDetails") ||
		(stf(InterfaceStates.RadDetails)!=fPerspDetail) ) {
			InterfaceStates.RadDetails = fPerspDetail;
			float fCamPersp = CalcLandRadius();
            string sMsg = "#"+ FloatToString(fCamPersp, 1);
            SendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE, "TITLES_STR", 1, 24, sMsg);
            if(IsEntity(&locCamera))
            {
                SendMessage(&locCamera, "lf", MSG_CAMERA_SET_RADIUS, fCamPersp);
            }
	}
}
//Hokkins: настройки камеры <--

void ChangeSoundSetting()
{
	float fMusic = stf(GameInterface.nodes.music_slide.value);
	float fSound = stf(GameInterface.nodes.sound_slide.value);
	float fDialog = stf(GameInterface.nodes.dialog_slide.value);
	SendMessage(&sound,"lfff", MSG_SOUND_SET_MASTER_VOLUME, fSound,	fMusic,	fDialog);
}

void FillControlsList(int nMode)
{
	int n,qC,idx;
	string groupName;
	aref arGrp, arC;

	if( nMode == g_nCurControlsMode ) {return;}
	g_nCurControlsMode = nMode;
	DeleteAttribute(&GameInterface,"controls_list");
	GameInterface.controls_list.select = 0;

	groupName = GetGroupNameByMode(nMode);
	if( CheckAttribute(&objControlsState,"keygroups."+groupName) ) {
		makearef(arGrp,objControlsState.keygroups.(groupName));
		qC = GetAttributesNum(arGrp);
		idx = 0;
		for( n=0; n<qC; n++ ) {
			arC = GetAttributeN(arGrp,n);
			if( false==CheckAttribute(arC,"invisible") || arC.invisible!="1" ) {
			//if( CheckAttribute(arC,"remapping") && arC.remapping=="1" ) {
				if( AddToControlsList( idx, GetAttributeName(arC), GetAttributeValue(arC), CheckAttribute(arC,"remapping") && arC.remapping=="1" ) ) {
					idx++;
				}
			}
		}
	}
	SendMessage( &GameInterface, "lsl", MSG_INTERFACE_MSG_TO_NODE, "CONTROLS_LIST", 0 );
}

bool AddToControlsList(int row, string sControl, string sKey, bool bRemapable)
{
	string rowname = "tr" + (row+1);
	GameInterface.controls_list.(rowname).userdata.remapable = bRemapable;
	GameInterface.controls_list.(rowname).userdata.control = sControl;
	GameInterface.controls_list.(rowname).userdata.key = sKey;
	GameInterface.controls_list.(rowname).td2.str = LanguageConvertString(g_ControlsLngFile,sControl);
	if( GameInterface.controls_list.(rowname).td2.str == "" ) {
		trace("Warning!!! " + sControl + " hav`t translate value");
	}
	if( !bRemapable ) { // выделение контролок которые нельзя поменять
		GameInterface.controls_list.(rowname).td2.color = argb(255,128,128,128);
	}
	if( CheckAttribute(&objControlsState,"key_codes."+sKey+".img") ) {
		GameInterface.controls_list.(rowname).td1.fontidx = 0;
		GameInterface.controls_list.(rowname).td1.textoffset = "2,-1";
		GameInterface.controls_list.(rowname).td1.scale = 0.5;
		GameInterface.controls_list.(rowname).td1.str = objControlsState.key_codes.(sKey).img;
	}
	return true;
}

string GetGroupNameByMode(int nMode)
{
	switch( nMode ) {
	case 1: return "Sailing1Pers"; break;
	case 2: return "PrimaryLand"; break;
	case 3: return "Sailing3Pers"; break;
	case 4: return "FightModeControls"; break;
	}
	return "unknown";
}

void GetSoundOptionsData()
{
	float fCurMusic = 0.5;
	float fCurSound = 0.5;
	float fCurDialog = 0.5;
	SendMessage(&sound,"leee",MSG_SOUND_GET_MASTER_VOLUME,&fCurSound,&fCurMusic,&fCurDialog);
	GameInterface.nodes.music_slide.value = fCurMusic;
	GameInterface.nodes.sound_slide.value = fCurSound;
	GameInterface.nodes.dialog_slide.value = fCurDialog;
}

void GetMouseOptionsData()
{
	float fCurXSens = 0.5;
	float fCurYSens = 0.5;
	if( CheckAttribute(&InterfaceStates,"mouse.x_sens") ) {fCurXSens=stf(InterfaceStates.mouse.x_sens);}
	if( CheckAttribute(&InterfaceStates,"mouse.y_sens") ) {fCurYSens=stf(InterfaceStates.mouse.y_sens);}
	if(fCurXSens<0.0) fCurXSens = 0.0;
	if(fCurXSens>1.0) fCurXSens = 1.0;
	if(fCurYSens<0.0) fCurYSens = 0.0;
	if(fCurYSens>1.0) fCurYSens = 1.0;
	GameInterface.nodes.hmouse_sensitivity_slide.value = fCurXSens;
	GameInterface.nodes.vmouse_sensitivity_slide.value = fCurYSens;
}

void GetVideoOptionsData()
{
	float fC = 1.0;
	float fG = 1.0;
	float fB = 0.0;
	float fD = 1.0;
	float fE = 0.0;
	float fR = 0.0;

	if( CheckAttribute(&InterfaceStates,"video.contrast") ) {
		fC = stf(InterfaceStates.video.contrast);
	}
	
	if( CheckAttribute(&InterfaceStates,"video.gamma") ) {
		fG = stf(InterfaceStates.video.gamma);
	}
	
	if( CheckAttribute(&InterfaceStates,"video.brightness") ) {
		fB = stf(InterfaceStates.video.brightness);
	}

	if( CheckAttribute(&InterfaceStates,"SeaDetails") ) {
		fD = stf(InterfaceStates.SeaDetails);
	}
	
	// Hokkins: настройки камеры -->
	if( CheckAttribute(&InterfaceStates,"PerspDetails") ) {
		fE = stf(InterfaceStates.PerspDetails);
	}
	
	if( CheckAttribute(&InterfaceStates,"RadDetails") ) {
		fR = stf(InterfaceStates.RadDetails);
	}
	// Hokkins: настройки камеры <--

	ISetColorCorrection(fC, fG, fB, fD, fE, fR);
}

void ISetColorCorrection(float fContrast, float fGamma, float fBright, float fSeaDetails, float fPerspDetails, float fRad)
{
	float fCurContrast = ConvertContrast(fContrast,true);
	float fCurGamma = ConvertGamma(fGamma,true);
	float fCurBright = ConvertBright(fBright,true);
	float fCurSeaDetails = ConvertSeaDetails(fSeaDetails, true);

	if(fCurContrast>1.0) fCurContrast = 1.0;
	if(fCurContrast<0.0) fCurContrast = 0.0;
	if(fCurGamma>1.0) fCurGamma = 1.0;
	if(fCurGamma<0.0) fCurGamma = 0.0;
	if(fCurBright>1.0) fCurBright = 1.0;
	if(fCurBright<0.0) fCurBright = 0.0;
	if(fCurSeaDetails<0.0) fCurSeaDetails = 0.0;
	if(fCurSeaDetails>1.0) fCurSeaDetails = 1.0;
	
	//Hokkins: настройки камеры -->
	if(fPerspDetails<0.0) fPerspDetails = 0.0;
	if(fPerspDetails>1.0) fPerspDetails = 1.0;
	
	if(fRad<0.0) fRad = 0.0;
	if(fRad>1.0) fRad = 1.0;
	//Hokkins: настройки камеры <--

	GameInterface.nodes.CONTRAST_SLIDE.value = fCurContrast;
	GameInterface.nodes.GAMMA_SLIDE.value = fCurGamma;
	GameInterface.nodes.BRIGHT_SLIDE.value = fCurBright;
	GameInterface.nodes.SEA_DETAILS_SLIDE.value = fCurSeaDetails;
	
	//Hokkins: настройки камеры -->
	GameInterface.nodes.SEA_CAM_PERSP_SLIDE.value = fPerspDetails;
	GameInterface.nodes.LAND_CAM_RAD_SLIDE.value = fRad;
	//Hokkins: настройки камеры <--

	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"CONTRAST_SLIDE", 0,fCurContrast);
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"GAMMA_SLIDE", 0,fCurGamma);
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"BRIGHT_SLIDE", 0,fCurBright);
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"SEA_DETAILS_SLIDE", 0, fCurSeaDetails);
	
	//Hokkins: настройки камеры -->
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"SEA_CAM_PERSP_SLIDE", 0, fPerspDetails);
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"LAND_CAM_RAD_SLIDE", 0, fRad);
	//Hokkins: настройки камеры <--

	XI_SetColorCorrection(fContrast,fGamma,fBright);
	//Set sea detail
}

float ConvertContrast(float fContrast, bool Real2Slider)
{ // контрастность от 0.75 до 1.25
	if(Real2Slider) {
		return fContrast*2.0-1.5;
	}
	return fContrast*0.5+0.75;
}

float ConvertGamma(float fGamma, bool Real2Slider)
{ // гамма от 0.5 до 2.0
	if(Real2Slider)
	{
		if(fGamma<=1.0) {return fGamma-0.5;}
		return fGamma*0.5;
	}
	if(fGamma<=0.5) {return fGamma+0.5;}
	return fGamma*2.0;
}

float ConvertBright(float fBright, bool Real2Slider)
{
	if(Real2Slider) {
		return (fBright+50.0)/100.0;
	}
	return fBright*100-50;
}

float ConvertSeaDetails(float fDetails, bool Real2Slider)
{
	return fDetails;
}

void GetHerbOptionsData()
{
	int nSelBtn = 0;
	switch( iGrassQuality ) {
	case 0: nSelBtn=4; break;
	case 1: nSelBtn=3; break;
	case 2: nSelBtn=2; break;
	case 3: nSelBtn=1; break;
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"HERB_CHECKBOX", 2, nSelBtn, true );
}

void GetControlsStatesData()
{
	int nAlwaysRun = 0;
	if( CheckAttribute(&InterfaceStates,"alwaysrun") ) {
		nAlwaysRun = sti(InterfaceStates.alwaysrun);
	}
	int nInvertCam = 0;
	if( CheckAttribute(&InterfaceStates,"InvertCameras") ) {
		nInvertCam = sti(InterfaceStates.InvertCameras);
	}
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"ALWAYS_RUN_CHECKBOX", 2, 1, nAlwaysRun );
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"INVERT_MOUSE_CHECKBOX", 2, 1, nInvertCam );
}

void SnapshotMetalGraphicsOptions()
{
	metalGraphicsSnapshotDynamicLighting = sti(InterfaceStates.MetalDynamicLighting);
	metalGraphicsSnapshotShadowQuality = sti(InterfaceStates.MetalShadowQuality);
	metalGraphicsSnapshotModernWater = sti(InterfaceStates.MetalModernWater);
	metalGraphicsSnapshotModernLighting = sti(InterfaceStates.MetalModernLighting);
	metalGraphicsSnapshotCinematic = sti(InterfaceStates.MetalCinematic);
	metalGraphicsSnapshotLightShafts = sti(InterfaceStates.MetalLightShafts);
	metalGraphicsSnapshotDynamicSky = sti(InterfaceStates.MetalDynamicSky);
	metalGraphicsSnapshotAntialiasing = sti(InterfaceStates.MetalAntialiasing);
	metalGraphicsSnapshotFullScreen = sti(InterfaceStates.MetalFullScreen);
	metalGraphicsSnapshotResolution = sti(InterfaceStates.MetalResolution);
	metalGraphicsSnapshotResolutionWidth = sti(InterfaceStates.MetalResolutionWidth);
	metalGraphicsSnapshotResolutionHeight = sti(InterfaceStates.MetalResolutionHeight);
}

void RestoreMetalGraphicsOptions()
{
	InterfaceStates.MetalDynamicLighting = metalGraphicsSnapshotDynamicLighting;
	InterfaceStates.MetalShadowQuality = metalGraphicsSnapshotShadowQuality;
	InterfaceStates.MetalModernWater = metalGraphicsSnapshotModernWater;
	InterfaceStates.MetalModernLighting = metalGraphicsSnapshotModernLighting;
	InterfaceStates.MetalCinematic = metalGraphicsSnapshotCinematic;
	InterfaceStates.MetalLightShafts = metalGraphicsSnapshotLightShafts;
	InterfaceStates.MetalDynamicSky = metalGraphicsSnapshotDynamicSky;
	InterfaceStates.MetalAntialiasing = metalGraphicsSnapshotAntialiasing;
	InterfaceStates.MetalFullScreen = metalGraphicsSnapshotFullScreen;
	InterfaceStates.MetalResolution = metalGraphicsSnapshotResolution;
	InterfaceStates.MetalResolutionWidth = metalGraphicsSnapshotResolutionWidth;
	InterfaceStates.MetalResolutionHeight = metalGraphicsSnapshotResolutionHeight;
	GetMetalGraphicsData();
}

void CloseMetalGraphicsWindow()
{
	metalGraphicsEditing = false;
	XI_WindowShow("METAL_GRAPHICS_WINDOW", false);
	XI_WindowDisable("MAIN_WINDOW", false);
	SetCurrentNode("METAL_GRAPHICS_BTN");
}

void ResolveMetalResolution(ref result, int nResolution)
{
	int nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);
	int nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);
	int nDisplayWidth = metalGraphicsDesktopWidth;
	int nDisplayHeight = metalGraphicsDesktopHeight;
	if(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }
	if(nDisplayWidth < 320 || nDisplayHeight < 200) { nDisplayWidth = nBaseWidth; nDisplayHeight = nBaseHeight; }
	result.width = nBaseWidth;
	result.height = nBaseHeight;
	result.desktop = 0;
	if(nResolution==0) result.width = 1280;
	if(nResolution==1) result.width = 1600;
	if(nResolution==2) result.width = 1920;
	if(nResolution==3) result.width = 2560;
	if(nResolution>=0 && nResolution<=3) result.height = makeint((sti(result.width) * nDisplayHeight + nDisplayWidth / 2) / nDisplayWidth);
	if(nResolution==5) {
		result.width = nDisplayWidth;
		result.height = nDisplayHeight;
		if(sti(InterfaceStates.MetalFullScreen)) result.desktop = 1;
	}
}

string FormatMetalAspect(int nWidth, int nHeight)
{
	int nA = nWidth;
	int nB = nHeight;
	int nRemainder;
	while(nB > 0) {
		nRemainder = nA % nB;
		nA = nB;
		nB = nRemainder;
	}
	if(nA > 0 && nWidth / nA <= 32 && nHeight / nA <= 32) return "" + nWidth / nA + ":" + nHeight / nA;
	int nAspect = makeint((nWidth * 100 + nHeight / 2) / nHeight);
	int nWhole = makeint(nAspect / 100);
	int nFraction = nAspect % 100;
	string sAspect = "" + nWhole + ".";
	if(nFraction < 10) sAspect = sAspect + "0";
	return sAspect + nFraction + ":1";
}

string FormatMetalResolution()
{
	object resolved;
	int nResolution = sti(InterfaceStates.MetalResolution);
	string sPrefix = "";
	if(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }
	ResolveMetalResolution(&resolved, nResolution);
	if(nResolution==4) sPrefix = XI_ConvertString("Resolution Current") + ": ";
	if(nResolution==5) sPrefix = XI_ConvertString("Resolution Desktop") + ": ";
	return sPrefix + resolved.width + " x " + resolved.height + "  (" + FormatMetalAspect(sti(resolved.width),sti(resolved.height)) + ")";
}

string MetalGraphicsToggleLabel(int nEnabled)
{
	if(nEnabled) return XI_ConvertString("Yes");
	return XI_ConvertString("No");
}

string MetalGraphicsShadowLabel()
{
	int nQuality = sti(InterfaceStates.MetalShadowQuality);
	if(nQuality==0) return XI_ConvertString("Shadow Quality Low");
	if(nQuality==2) return XI_ConvertString("Shadow Quality High");
	return XI_ConvertString("Shadow Quality Medium");
}

void SetMetalGraphicsRow(int nRow, string sGroup, string sName, string sValue)
{
	string sRow = "tr" + nRow;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td1.str = sGroup;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td2.str = sName;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td3.str = "<  " + sValue + "  >";
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td1.scale = 0.78;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td2.scale = 0.82;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td3.scale = 0.82;
	GameInterface.METAL_GRAPHICS_LIST.(sRow).td3.align = "center";
}

void GetMetalGraphicsData()
{
	int nSelected = 1;
	int nTop = 0;
	RefreshMetalDesktopSize();
	if(CheckAttribute(&GameInterface,"METAL_GRAPHICS_LIST.select")) nSelected = sti(GameInterface.METAL_GRAPHICS_LIST.select);
	if(CheckAttribute(&GameInterface,"METAL_GRAPHICS_LIST.top")) nTop = sti(GameInterface.METAL_GRAPHICS_LIST.top);
	DeleteAttribute(&GameInterface,"METAL_GRAPHICS_LIST");
	GameInterface.METAL_GRAPHICS_LIST.select = nSelected;
	GameInterface.METAL_GRAPHICS_LIST.top = nTop;
	GameInterface.METAL_GRAPHICS_LIST.hr.td1.str = XI_ConvertString("Metal Graphics Group");
	GameInterface.METAL_GRAPHICS_LIST.hr.td2.str = XI_ConvertString("Metal Graphics Setting");
	GameInterface.METAL_GRAPHICS_LIST.hr.td3.str = XI_ConvertString("Metal Graphics Value");
	SetMetalGraphicsRow(1, XI_ConvertString("Metal Display Group"), XI_ConvertString("Metal Fullscreen"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalFullScreen)));
	SetMetalGraphicsRow(2, "", XI_ConvertString("Resolution"), FormatMetalResolution());
	SetMetalGraphicsRow(3, XI_ConvertString("Metal Lighting Group"), XI_ConvertString("Metal Dynamic Shadows"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalDynamicLighting)));
	SetMetalGraphicsRow(4, "", XI_ConvertString("Metal Shadow Quality"), MetalGraphicsShadowLabel());
	SetMetalGraphicsRow(5, "", XI_ConvertString("Metal Modern Lighting"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalModernLighting)));
	SetMetalGraphicsRow(6, "", XI_ConvertString("Metal Light Shafts"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalLightShafts)));
	SetMetalGraphicsRow(7, "", XI_ConvertString("Metal Dynamic Sky"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalDynamicSky)));
	SetMetalGraphicsRow(8, "", XI_ConvertString("Metal Cinematic Interior"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalCinematic)));
	SetMetalGraphicsRow(9, XI_ConvertString("Metal Effects Group"), XI_ConvertString("Metal Modern Water"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalModernWater)));
	SetMetalGraphicsRow(10, "", XI_ConvertString("Metal FXAA"), MetalGraphicsToggleLabel(sti(InterfaceStates.MetalAntialiasing)));
	SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"METAL_GRAPHICS_LIST",0);
}

void ChangeMetalGraphicsRow(int nRow, int nDirection)
{
	if(nDirection==0) nDirection = 1;
	if(nRow==1) {
		InterfaceStates.MetalFullScreen = !sti(InterfaceStates.MetalFullScreen);
		if(!sti(InterfaceStates.MetalFullScreen) && sti(InterfaceStates.MetalResolution)==5) InterfaceStates.MetalResolution = 4;
	}
	if(nRow==2) {
		InterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) + nDirection;
		if(sti(InterfaceStates.MetalFullScreen)) {
			if(sti(InterfaceStates.MetalResolution)<0) InterfaceStates.MetalResolution = 5;
			if(sti(InterfaceStates.MetalResolution)>5) InterfaceStates.MetalResolution = 0;
		} else {
			if(sti(InterfaceStates.MetalResolution)<0) InterfaceStates.MetalResolution = 4;
			if(sti(InterfaceStates.MetalResolution)>4) InterfaceStates.MetalResolution = 0;
		}
	}
	if(nRow==3) InterfaceStates.MetalDynamicLighting = !sti(InterfaceStates.MetalDynamicLighting);
	if(nRow==4) {
		InterfaceStates.MetalShadowQuality = sti(InterfaceStates.MetalShadowQuality) + nDirection;
		if(sti(InterfaceStates.MetalShadowQuality)<0) InterfaceStates.MetalShadowQuality = 2;
		if(sti(InterfaceStates.MetalShadowQuality)>2) InterfaceStates.MetalShadowQuality = 0;
	}
	if(nRow==5) InterfaceStates.MetalModernLighting = !sti(InterfaceStates.MetalModernLighting);
	if(nRow==6) InterfaceStates.MetalLightShafts = !sti(InterfaceStates.MetalLightShafts);
	if(nRow==7) InterfaceStates.MetalDynamicSky = !sti(InterfaceStates.MetalDynamicSky);
	if(nRow==8) InterfaceStates.MetalCinematic = !sti(InterfaceStates.MetalCinematic);
	if(nRow==9) InterfaceStates.MetalModernWater = !sti(InterfaceStates.MetalModernWater);
	if(nRow==10) InterfaceStates.MetalAntialiasing = !sti(InterfaceStates.MetalAntialiasing);
	GetMetalGraphicsData();
}

void ProcessMetalGraphicsCommand()
{
	string sCommand = GetEventData();
	string sNode = GetEventData();
	if(sNode != "METAL_GRAPHICS_LIST") return;
	if(sCommand=="leftstep" || sCommand=="speedleft") ChangeMetalGraphicsRow(sti(GameInterface.METAL_GRAPHICS_LIST.select),-1);
	if(sCommand=="rightstep" || sCommand=="speedright") ChangeMetalGraphicsRow(sti(GameInterface.METAL_GRAPHICS_LIST.select),1);
}

void ClickMetalGraphicsRow()
{
	string sNode = GetEventData();
	int nRow = GetEventData();
	int nColumn = GetEventData();
	if(sNode=="METAL_GRAPHICS_LIST") ChangeMetalGraphicsRow(nRow+1,1);
}

void ActivateMetalGraphicsRow()
{
	string sNode = GetEventData();
	int nRow = GetEventData();
	if(sNode=="METAL_GRAPHICS_LIST") ChangeMetalGraphicsRow(nRow+1,1);
}

extern bool SaveMetalGraphicsOptions();

bool SaveMetalGraphicsOptionsNow()
{
	bool saved = false;
	if(LoadSegment("interface\option_sl.c")) {
		saved = SaveMetalGraphicsOptions();
		UnloadSegment("interface\option_sl.c");
	}
	return saved;
}

void ApplyMetalGraphicsOptions()
{
	object resolved;
	int flagsMask = 0;
	int nResolution = sti(InterfaceStates.MetalResolution);
	if(nResolution < 0 || nResolution > 5) {
		SetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));
		return;
	}
	if(!sti(InterfaceStates.MetalFullScreen) && nResolution==5) { nResolution = 4; InterfaceStates.MetalResolution = 4; }
	if(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;
	if(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;
	if(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;
	if(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;
	if(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;
	if(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;
	if(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;
	ResolveMetalResolution(&resolved,nResolution);
	InterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;
	InterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;
	if(!SaveMetalGraphicsOptionsNow()) {
		SetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Save Error"));
		return;
	}
	metalGraphicsPendingFlags = flagsMask;
	metalGraphicsPendingShadowQuality = sti(InterfaceStates.MetalShadowQuality);
	metalGraphicsPendingFullscreen = sti(Render.full_screen);
	metalGraphicsPendingWidth = sti(Render.screen_x);
	metalGraphicsPendingHeight = sti(Render.screen_y);
	metalGraphicsPendingDesktop = 0;
	SnapshotMetalGraphicsOptions();
	CloseMetalGraphicsWindow();
	PostEvent("evApplyMetalGraphicsDeferred",1);
}

void ApplyMetalGraphicsDeferred()
{
	int queued = SendMessage(&GameInterface,"lllllll",45062,metalGraphicsPendingFlags,metalGraphicsPendingShadowQuality,metalGraphicsPendingFullscreen,metalGraphicsPendingWidth,metalGraphicsPendingHeight,metalGraphicsPendingDesktop);
	if(queued != 3) return;
	PostEvent("evPollMetalGraphicsApply",50);
}

void PollMetalGraphicsApply()
{
	int result = SendMessage(&GameInterface,"l",45065);
	if(result == 0) { PostEvent("evPollMetalGraphicsApply",50); return; }
}

void RefreshMetalDesktopSize()
{
	int fallbackWidth = sti(InterfaceStates.MetalDisplayWidth);
	int fallbackHeight = sti(InterfaceStates.MetalDisplayHeight);
	if(fallbackWidth < 320 || fallbackHeight < 200) {
		fallbackWidth = sti(InterfaceStates.MetalResolutionWidth);
		fallbackHeight = sti(InterfaceStates.MetalResolutionHeight);
	}
	if(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; fallbackHeight = 1080; }
	metalGraphicsDesktopWidth = fallbackWidth;
	metalGraphicsDesktopHeight = fallbackHeight;
	int displayWidth = SendMessage(&GameInterface,"l",45063);
	int displayHeight = SendMessage(&GameInterface,"l",45064);
	if(displayWidth >= 320 && displayHeight >= 200) {
		metalGraphicsDesktopWidth = displayWidth;
		metalGraphicsDesktopHeight = displayHeight;
	}
}

void SetAlwaysRun(bool bRun)
{
	InterfaceStates.alwaysrun = bRun;
}

void procKeyChange()
{
	//FillControlsList();
	string srow = "tr" + GameInterface.controls_list.select;
	if( !CheckAttribute(&GameInterface,"controls_list."+srow) ) {return;}
	if( sti(GameInterface.controls_list.(srow).userdata.remapable)!=1 ) {return;}
	ChooseOtherControl();
}

void ChooseOtherControl()
{
	XI_WindowDisable("MAIN_WINDOW",true);
	XI_WindowShow("CHANGEKEY_WINDOW",true);
	SetCurrentNode("KEY_CHOOSER");
	string srow = "tr" + GameInterface.controls_list.select;
	SetFormatedText("CHANGEKEY_TEXT", XI_ConvertString("Press any key"));
	AddLineToFormatedText("CHANGEKEY_TEXT", " ");
	AddLineToFormatedText("CHANGEKEY_TEXT", GameInterface.controls_list.(srow).td2.str);
	AddLineToFormatedText("CHANGEKEY_TEXT", " ");
	AddLineToFormatedText("CHANGEKEY_TEXT", XI_ConvertString("KeyAlreadyUsed"));
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 8, 0, argb(255,255,128,128) );
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 8, 4, argb(0,255,64,64) );
	SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 5);
}

int glob_retVal;
ref procKeyChoose()
{
	int keyIdx = GetEventData();
	int stickUp = GetEventData();

	glob_retVal = false;

	if (keyIdx == 7) {
		ReturnFromReassign();
		glob_retVal = true;
		return &glob_retVal;
	}

	if( DoMapToOtherKey(keyIdx,stickUp) )
	{
		ReturnFromReassign();
		glob_retVal = true;
	}

	return &glob_retVal;
}

void ReturnFromReassign()
{
	XI_WindowShow("CHANGEKEY_WINDOW",false);
	XI_WindowDisable("MAIN_WINDOW",false);
	SetCurrentNode("CONTROLS_LIST");
}

bool DoMapToOtherKey(int keyIdx,int stickUp)
{
	string srow = "tr" + GameInterface.controls_list.select;
	string groupName = GetGroupNameByMode( g_nCurControlsMode );
	string sControl = GameInterface.controls_list.(srow).userdata.control;
	string sKey = GameInterface.controls_list.(srow).userdata.key;


	aref arControlGroup;
	aref arKeyRoot,arKey;
	string tmpstr;
	int keyCode;

	if( stickUp )
	{
		//SetStickNotAvailable();
		return false;
	}

	makearef(arControlGroup,objControlsState.keygroups.(groupName));
	makearef(arKeyRoot,objControlsState.key_codes);
	arKey = GetAttributeN(arKeyRoot,keyIdx);
	keyCode = sti(GetAttributeValue(arKey));

	// check for not allowed keys
	if( //keyCode==sti(objControlsState.key_codes.VK_F1) ||
		keyCode==sti(objControlsState.key_codes.VK_F2) ||
		//keyCode==sti(objControlsState.key_codes.VK_F3) ||
		//keyCode==sti(objControlsState.key_codes.VK_F4) ||
		//keyCode==sti(objControlsState.key_codes.VK_F5) ||
		keyCode==sti(objControlsState.key_codes.VK_F6) ||
		//keyCode==sti(objControlsState.key_codes.VK_F7) ||
		keyCode==sti(objControlsState.key_codes.VK_F8) ||
		keyCode==sti(objControlsState.key_codes.VK_F9) )
	{
		return false;
	}

	if( CheckAttribute(arKey,"stick") && sti(arKey.stick)==true ) return false;

	if( KeyAlreadyUsed(groupName, sControl, GetAttributeName(arKey)) )
	{
		SetKeyChooseWarning( XI_ConvertString("KeyAlreadyUsed") );
		return false;
	}

	tmpstr = arControlGroup.(sControl);
	if( CheckAttribute(arKeyRoot,tmpstr+".stick") && sti(arKeyRoot.(tmpstr).stick)==true ) return false;

	int state = 0;
	if(CheckAttribute(arControlGroup,sControl+".state"))
	{	state = sti(arControlGroup.(sControl).state);	}

	CI_CreateAndSetControls( groupName,sControl,keyCode, state, true );
	GameInterface.controls_list.(srow).userdata.key = arKey;
	GameInterface.controls_list.(srow).td1.str = arKey.img;
	SendMessage( &GameInterface, "lsl", MSG_INTERFACE_MSG_TO_NODE, "CONTROLS_LIST", 0 );
	return true;
}

void RestoreDefaultKeys()
{
	SetMouseToDefault();
	ControlsInit(GetTargetPlatform(),false);

	int nMode = g_nCurControlsMode;
	g_nCurControlsMode = -1;
	FillControlsList(nMode);
}

void SetMouseToDefault()
{
	InterfaceStates.InvertCameras = false;
	InterfaceStates.mouse.x_sens = 0.5;
	InterfaceStates.mouse.y_sens = 0.5;

	SetRealMouseSensitivity();
	SetAlwaysRun(true);

	GetControlsStatesData();
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"VMOUSE_SENSITIVITY_SLIDE", 0,stf(InterfaceStates.mouse.y_sens));
	SendMessage(&GameInterface,"lslf",MSG_INTERFACE_MSG_TO_NODE,"HMOUSE_SENSITIVITY_SLIDE", 0,stf(InterfaceStates.mouse.x_sens));
}

void ShowInfo()
{
	g_bToolTipStarted = true;
	string sHeader = "TEST";
	string sNode = GetCurrentNode();

	string sText1, sText2, sText3, sPicture, sGroup, sGroupPicture;
	sPicture = "none";
	sGroup = "none";
	sGroupPicture = "none";

	switch (sNode)
	{
		case "GAMMA_SLIDE":
			sHeader = XI_ConvertString("gamma");
			sText1 = XI_ConvertString("gamma_descr");
			sText3 = XI_ConvertString("FullScreenOnly");
		break;
		case "BRIGHT_SLIDE":
			sHeader = XI_ConvertString("Brightness");
			sText1 = XI_ConvertString("brightness_descr");
			sText3 = XI_ConvertString("FullScreenOnly");
		break;
		case "CONTRAST_SLIDE":
			sHeader = XI_ConvertString("Contrast");
			sText1 = XI_ConvertString("Contrast_descr");
			sText3 = XI_ConvertString("FullScreenOnly");
		break;
		
		case "GLOW_SLIDE":
			sHeader = XI_ConvertString("Glow");
			sText1 = XI_ConvertString("Glow_descr");
			sText2 = XI_ConvertString("PostProcessOnly");
		break;
		
		case "SEA_DETAILS_SLIDE":
			sHeader = XI_ConvertString("Sea Detail");
			sText1 = XI_ConvertString("Sea Detail_descr");
			sText2 = XI_ConvertString("ItCanRedusePerfomance");
			sText3 = XI_ConvertString("NeedToExitFromSea");
		break;
		
		case "SEA_CAM_PERSP_SLIDE":
			sHeader = XI_ConvertString("SeaCameraPerspective");
			sText1 = XI_ConvertString("SeaCameraPerspective_descr1");
		break;
		
		case "LAND_CAM_RAD_SLIDE":
			sHeader = XI_ConvertString("LandCameraRadius");
			sText1 = XI_ConvertString("LandCameraRadius_descr1");
		break;

		case "HERB_CHECKBOX":
			sHeader = XI_ConvertString("Herb Quantity");
			sText1 = XI_ConvertString("Herb Quantity_descr");
			sText2 = XI_ConvertString("ItCanRedusePerfomance");
			//sText3 = XI_ConvertString("NeedToExitFromSea");
		break;

		case "MUSIC_SLIDE":
			sHeader = XI_ConvertString("Music Volume");
			sText1 = XI_ConvertString("Music Volume_descr");
		break;

		case "SOUND_SLIDE":
			sHeader = XI_ConvertString("Sound Volume");
			sText1 = XI_ConvertString("Sound Volume_descr");
		break;

		case "DIALOG_SLIDE":
			sHeader = XI_ConvertString("Dialog Volume");
			sText1 = XI_ConvertString("Dialog Volume_descr");
		break;
		
		case "HUD_SLIDE":
			sHeader = XI_ConvertString("HUD_SLIDE");
			sText1 = XI_ConvertString("HUD_SLIDE_descr");
		break;

		case "ALWAYS_RUN_CHECKBOX":
			sHeader = XI_ConvertString("Always Run");
			sText1 = XI_ConvertString("Always Run_descr");
		break;

		case "INVERT_MOUSE_CHECKBOX":
			sHeader = XI_ConvertString("Invert Vertical Mouse Control");
			sText1 = XI_ConvertString("Invert Vertical Mouse Control_descr");
		break;

		case "VMOUSE_SENSITIVITY_SLIDE":
			sHeader = XI_ConvertString("Vertical Mouse Sensitivity");
			sText1 = XI_ConvertString("Vertical Mouse Sensitivity_descr");
		break;

		case "HMOUSE_SENSITIVITY_SLIDE":
			sHeader = XI_ConvertString("Horizontal Mouse Sensitivity");
			sText1 = XI_ConvertString("Horizontal Mouse Sensitivity_descr");
		break;
		
		case "SIMPLESEA_CHECKBOX":
			sHeader = XI_ConvertString("SimpleSea Mode");
			sText1 = XI_ConvertString("SimpleSea Mode_descr");
		break;

		case "BATTLE_MODE_CHECKBOX":
			sHeader = XI_ConvertString("Show battle mode");
			sText1 = XI_ConvertString("Show battle mode_descr");
		break;
		
		case "SKIPVIDEO_CHECKBOX":
			sHeader = XI_ConvertString("SkipStartVideo");
			sText1 = XI_ConvertString("SkipStartVideo_descr");
		break;
		
		case "SHIPMARK_CHECKBOX":
			sHeader = XI_ConvertString("ShipMark Mode");
			sText1 = XI_ConvertString("ShipMark Mode_descr");
		break;

		case "AUTOSAVE_CHECKBOX":
			sHeader = XI_ConvertString("AutoSave Mode");
			sText1 = XI_ConvertString("AutoSave Mode_descr");
		break;
	}

	CreateTooltip("#" + sHeader, sText1, argb(255,255,255,255), sText2, argb(255,255,192,192), sText3, argb(255,255,255,255), "", argb(255,255,255,255), sPicture, sGroup, sGroupPicture, 64, 64);
}

void HideInfo()
{
	if( g_bToolTipStarted ) {
		g_bToolTipStarted = false;
		CloseTooltip();
		SetCurrentNode("OK_BUTTON");
	}
}

bool KeyAlreadyUsed(string sGrpName, string sControl, string sKey)
{
	if( !CheckAttribute(&objControlsState,"keygroups."+sGrpName+"."+sControl) ) {return false;}
	if( objControlsState.keygroups.(sGrpName).(sControl) == sKey ) {return false;}

	bool bAlreadyUsed = false;
	int n,q, i,grp;
	aref arGrp,arCntrl, arGrpList;

	// проверка на совпадение в той же группе
	makearef(arGrp,objControlsState.keygroups.(sGrpName));
	q = GetAttributesNum(arGrp);
	for(n=0; n<q; n++)
	{
		arCntrl = GetAttributeN(arGrp,n);
		if( GetAttributeValue(arCntrl) == sKey ) {
			bAlreadyUsed = true;
			break;
		}
	}

	if( bAlreadyUsed ) {return bAlreadyUsed;}

	// найдем группу в которой эта контролка также отображается
	makearef(arGrpList, objControlsState.keygroups);
	grp = GetAttributesNum(arGrpList);
	for( i=0; i<grp; i++ )
	{
		arGrp = GetAttributeN(arGrpList,i);
		if( !CheckAttribute(arGrp,sControl) ) {continue;}

		q = GetAttributesNum(arGrp);
		for(n=0; n<q; n++)
		{
			arCntrl = GetAttributeN(arGrp,n);
			if( GetAttributeValue(arCntrl) == sKey ) {
				bAlreadyUsed = true;
				break;
			}
		}
		if( bAlreadyUsed ) {break;}
	}

	return bAlreadyUsed;
}

void SetKeyChooseWarning( string sWarningText )
{
	SendMessage(&GameInterface,"lslle",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 10, 4, &sWarningText );
	SendMessage( &GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 5 );
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 8, 4, argb(255,255,64,64) );
	PostEvent("evFaderFrame",700,"lll",500,0,50);
}

void FaderFrame()
{
	int nTotalTime = GetEventData();
	int nCurTime = GetEventData();
	int nDeltaTime = GetEventData();

	nCurTime = nCurTime + nDeltaTime;
	if( nCurTime>nTotalTime ) {nCurTime=nTotalTime;}

	int nAlpha = 255*(nTotalTime-nCurTime) / nTotalTime;
	SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"CHANGEKEY_TEXT", 8, 4, argb(nAlpha,255,64,64) );

	if( nCurTime<nTotalTime ) {
		PostEvent("evFaderFrame",nDeltaTime,"lll",nTotalTime,nCurTime,nDeltaTime);
	}
}

void ChangeHUDDetail()
{
    float sl = stf(GameInterface.nodes.hud_slide.value);
	newBase = CalcHUDBase(sl, stf(Render.screen_y));
	if( newBase != iHUDBase)
	{
        fHUDRatio = stf(Render.screen_y) / newBase;
		//SetFormatedText("HUD_DESCRIP_TEXT", "Размер интерфейса: " + Render.screen_y + " / " + newBase + " : " + fHUDRatio;
		SetFormatedText("HUD_DESCRIP_TEXT", "Размер интерфейса: " + Render.screen_y + " / " + newBase + " : " + stf(sti(fHUDRatio * 100)) / 100);
	}
}

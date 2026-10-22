bool bSeePeoplesOnDeck = true; // Warship 08.06.09 видеть матросов на палубе при виде от первого лица или нет

#define DEFAULT_CAM_PERSP      1.0
#define DEFAULT_CAM_PERSP_DEN  1.85

object	SeaCameras;
aref	Crosshair;
object	SeaShipCamera,SeaFreeCamera,SeaDeckCamera;
ref		SeaShipCharacterForCamera;
bool	bCanSwitchCameras = true;

void DeleteSeaCamerasEnvironment()
{
	SeaCameras_CancelDartView();
	if (CheckAttribute(&worldMap, "directSail.retainCameras") && sti(worldMap.directSail.retainCameras) != 0 &&
		IsEntity(&SeaMapTransition) && CheckAttribute(&SeaMapTransition, "seaOnly") && sti(SeaMapTransition.seaOnly) != 0 &&
		IsEntity(&SeaCameras) && IsEntity(&SeaFreeCamera) && IsEntity(&SeaShipCamera) && IsEntity(&SeaDeckCamera)) return;

	DeleteClass(&SeaCameras);
	DeleteClass(&SeaFreeCamera);
	DeleteClass(&SeaShipCamera);
	DeleteClass(&SeaDeckCamera);
	DelEventHandler("SeaCameras_Switch", "SeaCameras_Switch");
	DelEventHandler(TELESCOPE_ACTIVE, "SeaCameras_TelescopeActive");
	DelEventHandler("Control Activation", "SeaCameras_DartViewControl");
}

//Hokkins: перспектива на море -->
void SetPerspectiveSettings()
{
    float fCamPersp = CalcSeaPerspective();

    SeaFreeCamera.Perspective = fCamPersp;
	SeaShipCamera.Perspective = fCamPersp;
	/* SeaDeckCamera.Perspective = fCamPersp; */
	
	SendMessage(&locCamera, "lf", MSG_CAMERA_SET_RADIUS, CalcLandRadius());
}

float CalcSeaPerspective()
{
    float fCamPerspAdj = 0.0;
    if(CheckAttribute(&InterfaceStates, "PerspDetails"))
       fCamPerspAdj = stf(InterfaceStates.PerspDetails) / DEFAULT_CAM_PERSP_DEN;

    return (DEFAULT_CAM_PERSP + fCamPerspAdj);
}
//Hokkins: перспектива на море <--

void CreateSeaCamerasEnvironment()
{
	DartView_InitControls();
	if (CheckAttribute(&worldMap, "directSail.retainCameras") && sti(worldMap.directSail.retainCameras) != 0 &&
		IsEntity(&SeaMapTransition) && CheckAttribute(&SeaMapTransition, "seaOnly") && sti(SeaMapTransition.seaOnly) != 0 &&
		IsEntity(&SeaCameras) && IsEntity(&SeaFreeCamera) && IsEntity(&SeaShipCamera) && IsEntity(&SeaDeckCamera))
	{
		SendMessage(&AISea, "la", AI_MESSAGE_SET_CAMERAS_ATTRIBUTE, &SeaCameras);
		return;
	}

	bCanSwitchCameras = true;

	CreateEntity(&SeaCameras, "SEA_CAMERAS");
	SeaCameras.DartViewEnabled = 0;
	SeaCameras.DartViewCancel = 0;
	CreateEntity(&SeaFreeCamera, "FREE_CAMERA");
	CreateEntity(&SeaShipCamera, "SHIP_CAMERA");
	CreateEntity(&SeaDeckCamera, "DECK_CAMERA");
	
	LayerAddObject(SEA_EXECUTE, &SeaShipCamera, iShipPriorityExecute + 5);
	LayerAddObject(SEA_EXECUTE, &SeaFreeCamera, 1);
	LayerAddObject(SEA_EXECUTE, &SeaDeckCamera, iShipPriorityExecute + 5);
	LayerAddObject(SEA_REALIZE, &SeaDeckCamera, 65531);
	
	SetPerspectiveSettings();

	// Ship camera paramerets
	SeaShipCamera.SensivityDistance = 30.0;
	SeaShipCamera.SensivityHeightAngle = 0.02;
	SeaShipCamera.SensivityHeightAngleOnShip = 0.005;
	SeaShipCamera.SensivityAzimuthAngle = 0.04;
	SeaShipCamera.MaxAngleX = 0.07;
	SeaShipCamera.MinAngleX = -1.4;
	SeaShipCamera.Distance = 70.0; //69.0
	SeaShipCamera.MinDistance = 25.0;
	SeaShipCamera.MaxDistance = 1500.0; //90.0
	SeaShipCamera.MinHeightOnSea = 1.0;
	SeaShipCamera.MaxHeightOnShip = 5.0; //16.0
	SeaShipCamera.InvertMouseX = 1.0;
	SeaShipCamera.InvertMouseY = -1.0;
	
	// Deck camera paramerets
	SeaDeckCamera.Perspective = 1.285;
	SeaDeckCamera.WalkMode = 1;
	if (!bSeaLoad || !CheckAttribute(&SeaDeckCamera, "ThirdPerson"))
		SeaDeckCamera.ThirdPerson = 1;
	SeaDeckCamera.TelescopeActive = 0;
	SeaDeckCamera.SensivityDistance = 0.01;
	SeaDeckCamera.SensivityHeightAngle = 0.0015;
	SeaDeckCamera.SensivityAzimuthAngle = 0.0015;
	SeaDeckCamera.StepUp = 0.5;
	SeaDeckCamera.StepMin = 0.01;
	SeaDeckCamera.h_max = 2.0;
	SeaDeckCamera.h_min = 0.5;
	SeaDeckCamera.h_step = 0.1;
	SeaDeckCamera.h_def = 1.6;
	SeaDeckCamera.MaxAngleX = 0.7;
	SeaDeckCamera.MinAngleX = -1.3;
	SeaDeckCamera.RockingX = 0.5;
	SeaDeckCamera.RockingZ = 0.5;

	makearef(Crosshair,SeaCameras.Crosshair);

	if (!bSeaLoad)
	{
		SeaCameras.Camera = "SeaShipCamera";
	
		Crosshair.OutsideCamera = true;
	}

	Crosshair.OutsideCamera = SeaCameras_isCameraOutside() || (SeaCameras.Camera == "SeaDeckCamera" && sti(SeaDeckCamera.ThirdPerson) != 0);
	Crosshair.Texture = "crosshair\crosshair.tga";
	Crosshair.Size = 0.05;
	Crosshair.Technique = "Crosshair";
	Crosshair.SubTexX = 1;
	Crosshair.SubTexY = 1;
	Crosshair.Colors.Default = argb(0, 255, 255, 0);
	Crosshair.Colors.Enemy = argb(0, 255, 0, 0);
	Crosshair.Colors.Friend = argb(0, 0, 255, 0);
	Crosshair.Colors.Neutral = argb(0, 128, 128, 128);

	SendMessage(&AISea, "la", AI_MESSAGE_SET_CAMERAS_ATTRIBUTE, &SeaCameras);

	SetEventHandler("SeaCameras_Switch", "SeaCameras_Switch", 1);
	SetEventHandler(TELESCOPE_ACTIVE, "SeaCameras_TelescopeActive", 0);
	SetEventHandler("Control Activation", "SeaCameras_DartViewControl", 0);

	// add cameras to list
	SendMessage(&SeaCameras, "li", AI_CAMERAS_ADD_CAMERA, &SeaShipCamera);
	SendMessage(&SeaCameras, "li", AI_CAMERAS_ADD_CAMERA, &SeaFreeCamera);
	SendMessage(&SeaCameras, "li", AI_CAMERAS_ADD_CAMERA, &SeaDeckCamera);
}

void SeaCameras_CancelDartView()
{
	if (!IsEntity(&SeaCameras)) return;
	if (!CheckAttribute(&SeaCameras, "DartViewCancel")) SeaCameras.DartViewCancel = 0;
	SeaCameras.DartViewCancel = sti(SeaCameras.DartViewCancel) + 1;
}

void SeaCameras_DartViewControl()
{
	string controlName = GetEventData();
	if (controlName != "DartImpactCamera" || !IsEntity(&SeaCameras)) return;
	if (sti(InterfaceStates.Launched) != 0) return;
	SeaCameras_CancelDartView();
	if (!CheckAttribute(&SeaCameras, "DartViewEnabled")) SeaCameras.DartViewEnabled = 0;
	SeaCameras.DartViewEnabled = !sti(SeaCameras.DartViewEnabled);
	if (sti(SeaCameras.DartViewEnabled) != 0) Log_Info(XI_ConvertString("DartViewOn"));
	else Log_Info(XI_ConvertString("DartViewOff"));
}

void SeaCameras_TelescopeActive()
{
	int iTelescopeActive = GetEventData();
	SeaDeckCamera.TelescopeActive = iTelescopeActive;
	if (iTelescopeActive)
	{
		SeaCameras_CancelDartView();
		bCanSwitchCameras = false;
		Crosshair.OutsideCamera = true;
	}
	else
	{
		bCanSwitchCameras = true;
		Crosshair.OutsideCamera = SeaCameras_isCameraOutside() || (SeaCameras.Camera == "SeaDeckCamera" && sti(SeaDeckCamera.ThirdPerson) != 0);
	}
}

void SeaCameras_Switch()
{
	SeaCameras_CancelDartView();
	if (!bCanSwitchCameras) return;

	int bSwitch = false;
	
	// Коммент - выбираем состояние, из которого переключаемся
	// Sailors.IsOnDeck = 1; - Флаг, находимся ли мы на палубе
	// если Sailors.IsOnDeck == 1, значит, мы на палубе, и матросов бегающих мы не увидем
	switch (SeaCameras.Camera)
	{
		case "SeaFreeCamera":
			SeaCameras.Camera = "SeaShipCamera";
			Crosshair.OutsideCamera = true;
			Sailors.IsOnDeck = 0;
			bSwitch = true;
		break;
		case "SeaShipCamera":
			if (!LAi_IsDead(&Characters[nMainCharacterIndex]))
			{
				SeaDeckCamera.ThirdPerson = 0;
				SeaCameras.Camera = "SeaDeckCamera";
				Crosshair.OutsideCamera = false;
				Sailors.IsOnDeck = !bSeePeoplesOnDeck;
				bSwitch = true;
			}
		break;
		case "SeaDeckCamera":
			if (sti(SeaDeckCamera.ThirdPerson) == 0)
			{
				SeaDeckCamera.ThirdPerson = 1;
				Crosshair.OutsideCamera = true;
				break;
			}
			//SeaCameras.Camera = "SeaFreeCamera"; break;		// Debug : SeaFreeCamera, release : SeaShipCamera
			SeaCameras.Camera = "SeaShipCamera"; 
			// boal -->
			if (locCameraEnableFree)
            {
			    SeaCameras.Camera = "SeaFreeCamera";
			}
			// boal <--
			Crosshair.OutsideCamera = true;
			Sailors.IsOnDeck = 0;
			bSwitch = true;
		break;
	}
	if (bSwitch) SeaCameras_UpdateCamera();
}

void SeaCameras_UpdateCamera()
{
	switch (SeaCameras.Camera)
	{
		case "SeaShipCamera":
			SendMessage(&SeaCameras, "lia", AI_CAMERAS_SET_CAMERA, &SeaShipCamera, &SeaShipCharacterForCamera);
		break;
		case "SeaFreeCamera":
			SendMessage(&SeaCameras, "lia", AI_CAMERAS_SET_CAMERA, &SeaFreeCamera, &SeaShipCharacterForCamera);
		break;
		case "SeaDeckCamera":
			SendMessage(&SeaCameras, "lia", AI_CAMERAS_SET_CAMERA, &SeaDeckCamera, &SeaShipCharacterForCamera);
		break;
	}
}

void SeaCameras_SetDieCamera()
{
	SeaCameras_CancelDartView();
	bool bOldCanSwitchCameras;
	if (SeaCameras.Camera == "SeaDeckCamera")
	{
		SeaCameras.Camera = "SeaShipCamera";
		Crosshair.OutsideCamera = true;
		Sailors.IsOnDeck = 0;
		DeleteClass(&Telescope);
		SeaCameras_UpdateCamera();
		SendMessage(&objISpyGlass, "ll", MSG_TELESCOPE_REQUEST,0); // выключить телескоп
	}
}

bool SeaCameras_isCameraOutside()
{
	if (SeaCameras.Camera == "SeaShipCamera") return true;
	if (SeaCameras.Camera == "SeaFreeCamera") return true;
	if (SeaCameras.Camera == "SeaDeckCamera") return false;
	return false;
}

void SeaCameras_SetShipForSeaCamera(object Character)
{
	makeref(SeaShipCharacterForCamera, Character);
	SeaCameras_UpdateCamera();
}

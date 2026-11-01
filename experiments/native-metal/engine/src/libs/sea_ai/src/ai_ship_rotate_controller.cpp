#include "ai_ship.h"

AIShipRotateController::AIShipRotateController(AIShip *pShip)
{
    fRotate = 0.0f;
    dwRotateNum = 0;
    fRotateSmooth = 0.0f;
    fRotateTime = 0.0f;
    fRotateMode = 0.0f;
    fGlobalMultiply = 1.0f;

    SetAIShip(pShip);
}

AIShipRotateController::~AIShipRotateController()
{
}

bool AIShipRotateController::Init()
{
    return true;
}

void AIShipRotateController::Execute(float fDeltaTime)
{
    if (!GetAIShip()->isAIControlled())
        return;

    // Direct proportional steering with exponential decay smoothing:
    // Blends target rudder angle smoothly each frame without lag or discrete step transitions
    fRotate = std::clamp(fRotate, -1.0f, 1.0f);
    const float blend = std::clamp(fDeltaTime * 6.0f, 0.05f, 1.0f);
    fRotateMode += (fRotate - fRotateMode) * blend;

    GetAIShip()->GetShipBasePointer()->SetRotate(fGlobalMultiply * fRotateMode);

    fRotate = 0.0f;
    fGlobalMultiply = 1.0f;
}

void AIShipRotateController::Realize(float fDeltaTime)
{
    // AIHelper::pRS->Print(0,0,"%f",__fRotate);
}

void AIShipRotateController::AddRotate(float _fRotate)
{
    fRotate += _fRotate;
}

void AIShipRotateController::SetGlobalMultiply(float _fGlobalMultiply)
{
    if (_fGlobalMultiply < fGlobalMultiply)
        fGlobalMultiply = _fGlobalMultiply;
}

void AIShipRotateController::Save(CSaveLoad *pSL) const
{
    pSL->SaveDword(dwRotateNum);
    pSL->SaveFloat(fRotateMode);
    pSL->SaveFloat(fRotateTime);
    pSL->SaveFloat(fRotateSmooth);
    pSL->SaveFloat(fRotate);
    pSL->SaveFloat(fGlobalMultiply);
}

void AIShipRotateController::Load(CSaveLoad *pSL)
{
    dwRotateNum = pSL->LoadDword();
    fRotateMode = pSL->LoadFloat();
    fRotateTime = pSL->LoadFloat();
    fRotateSmooth = pSL->LoadFloat();
    fRotate = pSL->LoadFloat();
    fGlobalMultiply = pSL->LoadFloat();
}

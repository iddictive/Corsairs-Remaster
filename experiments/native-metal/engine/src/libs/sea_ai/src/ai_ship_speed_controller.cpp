#include "ai_ship.h"

AIShipSpeedController::AIShipSpeedController(AIShip *pShip)
{
    fSpeed = 0.0f;
    dwSpeedNum = 0;
    fSpeedSmooth = 0.0f;
    fSpeedTime = 0.0f;
    fGlobalMultiply = 1.0f;
    fTopSpeed = 1.0f;
    SetAIShip(pShip);
}

AIShipSpeedController::~AIShipSpeedController()
{
}

bool AIShipSpeedController::Init()
{
    return true;
}

void AIShipSpeedController::Execute(float fDeltaTime)
{
    if (!GetAIShip()->isAIControlled())
        return;

    // Direct progressive sail throttle with smooth exponential interpolation:
    // Prevents discrete step fluctuations while preserving dynamic maneuvers
    fSpeed = std::clamp(fSpeed, 0.0f, fTopSpeed);
    const float blend = std::clamp(fDeltaTime * 4.0f, 0.05f, 1.0f);
    fSpeedSmooth += (fSpeed - fSpeedSmooth) * blend;

    GetAIShip()->GetShipBasePointer()->SetSailState(fGlobalMultiply * fSpeedSmooth);

    fGlobalMultiply = 1.0f;
    fSpeed = 0.0f;
    fTopSpeed = 1.0f;
}

void AIShipSpeedController::Realize(float fDeltaTime)
{
}

void AIShipSpeedController::TopSpeed(float _fSpeed)
{
    fTopSpeed = _fSpeed;
}

void AIShipSpeedController::MulSpeed(float _fSpeed)
{
    fSpeed *= _fSpeed;
}

void AIShipSpeedController::AddSpeed(float _fSpeed)
{
    fSpeed += _fSpeed;
}

void AIShipSpeedController::SetGlobalMultiply(float _fGlobalMultiply)
{
    if (_fGlobalMultiply < fGlobalMultiply)
        fGlobalMultiply = _fGlobalMultiply;
}

void AIShipSpeedController::Save(CSaveLoad *pSL) const
{
    pSL->SaveDword(dwSpeedNum);
    pSL->SaveFloat(fSpeedSmooth);
    pSL->SaveFloat(fSpeed);
    pSL->SaveFloat(fSpeedTime);
    pSL->SaveFloat(fTopSpeed);
    pSL->SaveFloat(fGlobalMultiply);
}

void AIShipSpeedController::Load(CSaveLoad *pSL)
{
    dwSpeedNum = pSL->LoadDword();
    fSpeedSmooth = pSL->LoadFloat();
    fSpeed = pSL->LoadFloat();
    fSpeedTime = pSL->LoadFloat();
    fTopSpeed = pSL->LoadFloat();
    fGlobalMultiply = pSL->LoadFloat();
}

#include "seafoam.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "island_base.h"

#include "entity.h"
#include "core.h"
#include "math_inlines.h"
#include "string_compare.hpp"

CREATE_CLASS(SEAFOAM)

// entid_t arrowModel;

#define U_SPEED_K 24e-4f
#define V_SPEED_K 10e-5f
#define START_FADE_SPEED 5.f

//--------------------------------------------------------------------
SEAFOAM::SEAFOAM()
    : seaID(0), sea(nullptr), shipsCount(0), carcassTexture(0), isStorm(false), soundService(nullptr)
{
    psIni = nullptr;
    renderer = nullptr;
}

//--------------------------------------------------------------------
SEAFOAM::~SEAFOAM()
{
    // GUARD(SEAFOAM::~SEAFOAM)

    ReleaseShipFoam();
    ResetTsunamiShore();
    if (renderer && (carcassTexture >= 0))
        renderer->TextureRelease(carcassTexture);
    // UNGUARD
}

//--------------------------------------------------------------------
bool SEAFOAM::Init()
{
    // GUARD(SEAFOAM::Init)

    /*if (core.IsNetActive())
    {
      NetFindClass(false, &seaID, "NetSea");
      sea = (SEA_BASE*) core.GetEntityPointer(seaID);
    }
    else*/
    {
        seaID = core.GetEntityId("sea");
        sea = static_cast<SEA_BASE *>(core.GetEntityPointer(seaID));
    }

    renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    soundService = static_cast<VSoundService *>(core.GetService("SoundService"));

    psIni = fio->OpenIniFile("resource\\ini\\particles.ini");

    InitializeShipFoam();

    // core.CreateEntity(&arrowModel,"MODELR");
    // core.Send_Message(arrowModel,"ls",MSG_MODEL_LOAD_GEO, "fish01");

    carcassTexture = renderer->TextureCreate("seafoam_2.tga");
    return true;
    // UNGUARD
}

//--------------------------------------------------------------------
void SEAFOAM::InitializeShipFoam()
{
    auto &&entities = core.GetEntityIds("ship");
    for (auto ent : entities)
    {
        AddShip(ent);
    }
}

void SEAFOAM::AddShip(entid_t pShipEID)
{
    if (shipsCount >= MAX_SHIPS)
        return;

    auto *foamInfo = &shipFoamInfo[shipsCount++];

    foamInfo->ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShipEID));
    foamInfo->shipModel = foamInfo->ship->GetModel();
    foamInfo->shipModel->GetNode(0)->geo->GetInfo(foamInfo->hullInfo);
    foamInfo->enabled = true;
    foamInfo->frontEmitter[0] = new SEAFOAM_PS();
    foamInfo->frontEmitter[0]->Init(psIni.get(), "seafoam");
    foamInfo->frontEmitter[1] = new SEAFOAM_PS();
    foamInfo->frontEmitter[1]->Init(psIni.get(), "seafoam2");
    foamInfo->frontEmitter[2] = new SEAFOAM_PS();
    foamInfo->frontEmitter[2]->Init(psIni.get(), "seafoam_front");

    CreateTracePoints(foamInfo);
    const auto wideK = sqrtf(foamInfo->hullInfo.boxsize.y / 17.f);
    foamInfo->carcass[0] = new TCarcass(TRACE_STEPS_Z, MEASURE_POINTS, renderer, true);
    foamInfo->carcass[0]->Initialize();
    foamInfo->carcass[0]->InitCircleMeasure(wideK * 1.4f, wideK * -1.f, .55f);

    foamInfo->carcass[1] = new TCarcass(TRACE_STEPS_Z, MEASURE_POINTS, renderer, false);
    foamInfo->carcass[1]->Initialize();
    foamInfo->carcass[1]->InitCircleMeasure(wideK * 1.4f, wideK * 1.f, .55f);

    foamInfo->firstSoundPlay = true;
    foamInfo->doSplash = false;
    foamInfo->sound = 0;
    foamInfo->previousBowY = 0.f;
    foamInfo->previousBowValid = false;
    foamInfo->tsunamiSerial = 0;
    foamInfo->whirlpoolSerial = 0;
    foamInfo->emergenceGeneration = 0;
    foamInfo->emergenceSplashOwned = false;
    foamInfo->waterImpactGeneration = 0;
    foamInfo->waterImpactOwned = false;
}

//--------------------------------------------------------------------
void SEAFOAM::ReleaseShipFoam()
{
    tShipFoamInfo *foamInfo = nullptr;

    for (auto ship = 0; ship < shipsCount; ship++)
    {
        foamInfo = &shipFoamInfo[ship];

        foamInfo->carcass[0]->Uninitialize();
        delete foamInfo->carcass[0];
        foamInfo->carcass[1]->Uninitialize();
        delete foamInfo->carcass[1];

        delete foamInfo->frontEmitter[0];
        delete foamInfo->frontEmitter[1];
        delete foamInfo->frontEmitter[2];
    }
    shipsCount = 0;
}

//--------------------------------------------------------------------
void SEAFOAM::CreateTracePoints(tShipFoamInfo *_shipFoamInfo)
{
    const auto yStep = 0.9f * _shipFoamInfo->hullInfo.boxsize.y / (TRACE_STEPS_Y - 1);
    const auto zStep = .15f * _shipFoamInfo->hullInfo.boxsize.z / TRACE_STEPS_Z;
    float curY, curZ;
    GEOS::VERTEX startSrcV, startDestV{};
    float startZ[TRACE_STEPS_Y];

    // <find_startZ>
    curY = _shipFoamInfo->hullInfo.boxcenter.y + (_shipFoamInfo->hullInfo.boxsize.y / 2.0f);

    startSrcV.x = _shipFoamInfo->hullInfo.boxcenter.x;
    startSrcV.z = _shipFoamInfo->hullInfo.boxcenter.z + _shipFoamInfo->hullInfo.boxsize.z / 2.0f;

    startDestV.x = _shipFoamInfo->hullInfo.boxcenter.x;
    startDestV.z = _shipFoamInfo->hullInfo.boxcenter.z - _shipFoamInfo->hullInfo.boxsize.z / 2.0f;
    int y;
    for (y = 0; y < TRACE_STEPS_Y; y++, curY -= yStep)
    {
        startSrcV.y = curY;
        startDestV.y = curY;
        const auto d = _shipFoamInfo->shipModel->GetNode(0)->geo->Trace(startSrcV, startDestV);
        if (d <= 1.0f)
        {
            startZ[y] = d * _shipFoamInfo->hullInfo.boxsize.z
                /*(startSrcV.z - 2.0f * _shipFoamInfo->hullInfo.boxcenter.z) + _shipFoamInfo->hullInfo.boxcenter.z*/;
            if (startZ[y] > (_shipFoamInfo->hullInfo.boxsize.z / 4.0f))
                startZ[y] = 0.0f;
        }
        else
            startZ[y] = 0.0f;
    }

    // <trace_from_sides>
    const auto halfZ = (_shipFoamInfo->hullInfo.boxsize.z / 2.0f);
    curZ = _shipFoamInfo->hullInfo.boxcenter.z + halfZ;
    for (auto z = 0; z < TRACE_STEPS_Z;
         z++, curZ -= zStep * (1 + 8 * sinf(PId2 * (_shipFoamInfo->hullInfo.boxcenter.z + halfZ - curZ) / halfZ)))
    {
        curY = _shipFoamInfo->hullInfo.boxcenter.y + (_shipFoamInfo->hullInfo.boxsize.y / 2.0f);
        CVECTOR meanPointSum(0.0f, 0.0f, 0.0f);
        auto meanCount = 0;
        auto firstEnabledY = 0;
        float deltaZ;
        auto kSum = 0.0f;

        for (y = 0; y < TRACE_STEPS_Y; y++, curY -= yStep)
        {
            deltaZ = startZ[y];
            const CVECTOR srcLeft(_shipFoamInfo->hullInfo.boxcenter.x - _shipFoamInfo->hullInfo.boxsize.x / 2.0f, curY,
                                  curZ - deltaZ);
            const CVECTOR srcRight(_shipFoamInfo->hullInfo.boxcenter.x + _shipFoamInfo->hullInfo.boxsize.x / 2.0f, curY,
                                   curZ - deltaZ);
            const CVECTOR dst(_shipFoamInfo->hullInfo.boxcenter.x, curY, curZ - deltaZ);
            GEOS::VERTEX srcLeftV, srcRightV, dstV;
            srcLeftV.x = srcLeft.x;
            srcLeftV.y = srcLeft.y;
            srcLeftV.z = srcLeft.z;
            srcRightV.x = srcRight.x;
            srcRightV.y = srcRight.y;
            srcRightV.z = srcRight.z;
            dstV.x = dst.x;
            dstV.y = dst.y;
            dstV.z = dst.z;

            _shipFoamInfo->hull[0][z].center[y].y = curY;
            _shipFoamInfo->hull[0][z].center[y].z = curZ - deltaZ;
            _shipFoamInfo->hull[1][z].center[y].y = curY;
            _shipFoamInfo->hull[1][z].center[y].z = curZ - deltaZ;
            // <from_left>
            auto d = _shipFoamInfo->shipModel->GetNode(0)->geo->Trace(srcLeftV, dstV);
            if (d > 1.0f)
                _shipFoamInfo->hull[0][z].center[y].x = _shipFoamInfo->hullInfo.boxcenter.x;
            else
                _shipFoamInfo->hull[0][z].center[y].x = -0.0f +
                                                        (1.0f - d) * (srcLeft.x - _shipFoamInfo->hullInfo.boxcenter.x) +
                                                        _shipFoamInfo->hullInfo.boxcenter.x;

            // <from_right>
            d = _shipFoamInfo->shipModel->GetNode(0)->geo->Trace(srcRightV, dstV);
            if (d > 1.0f)
                _shipFoamInfo->hull[1][z].center[y].x = _shipFoamInfo->hullInfo.boxcenter.x;
            else
                _shipFoamInfo->hull[1][z].center[y].x =
                    0.0f + (1.0f - d) * (srcRight.x - _shipFoamInfo->hullInfo.boxcenter.x) +
                    _shipFoamInfo->hullInfo.boxcenter.x;
        }
    }
}

//--------------------------------------------------------------------
void SEAFOAM::InterpolateLeftParticle(tShipFoamInfo &_shipFoamInfo, int z, uint32_t dTime)
{
    CVECTOR testPoint{};
    float seaY, interpK;
    int curY;
    CVECTOR lowPoint, highPoint, lastTestPoint;
    int testY, lastTestY;
    float lowSeaY, highSeaY, seaK;

    lowPoint = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.hull[0][z].center[TRACE_STEPS_Y - 1]);
    highPoint = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.hull[0][z].center[0]);
    lowSeaY = sea->WaveXZ(lowPoint.x, lowPoint.z);
    highSeaY = sea->WaveXZ(highPoint.x, highPoint.z);

    seaK = (highSeaY - lowSeaY) /
           (_shipFoamInfo.hull[0][z].center[0].y - _shipFoamInfo.hull[0][z].center[TRACE_STEPS_Y - 1].y); // k < 0
    if ((lowSeaY > lowPoint.y) && (highSeaY > highPoint.y))
    {
        // above sea
        _shipFoamInfo.levelStarts[0][z] = _shipFoamInfo.hull[0][z].center[0];
        CVECTOR tempVector{};
        _shipFoamInfo.shipModel->mtx.MulToInv(CVECTOR(highPoint.x, (lowSeaY + highSeaY) * 0.5f, highPoint.z),
                                              tempVector);
        _shipFoamInfo.levelStarts[0][z].y = tempVector.y;
        return;
    }

    if ((lowSeaY < lowPoint.y) && (highSeaY < highPoint.y))
    {
        // under sea
        _shipFoamInfo.levelStarts[0][z] = _shipFoamInfo.hull[0][z].center[TRACE_STEPS_Y - 1];
        CVECTOR tempVector{};
        _shipFoamInfo.shipModel->mtx.MulToInv(CVECTOR(lowPoint.x, (lowSeaY + highSeaY) * 0.5f, lowPoint.z), tempVector);
        _shipFoamInfo.levelStarts[0][z].y = tempVector.y;
        return;
    }

    curY = 0;
    testPoint = _shipFoamInfo.hull[0][z].center[curY];
    lastTestPoint = testPoint;
    testY = curY;
    lastTestY = testY;

    seaY = /*__max(lowSeaY, highSeaY)*/ (lowSeaY + highSeaY) / 2.0f;
    for (curY = 0; curY < TRACE_STEPS_Y; ++curY)
    {
        testPoint = _shipFoamInfo.shipModel->mtx * _shipFoamInfo.hull[0][z].center[curY];

        if (testPoint.y < seaY)
            break;
        lastTestPoint = testPoint;
        lastTestY = testY;
        testY = curY;
    }

    if (curY < TRACE_STEPS_Y)
    {
        // successful search
        interpK = (seaY - lastTestPoint.y) / (testPoint.y - lastTestPoint.y);
        testPoint = _shipFoamInfo.hull[0][z].center[curY];
        lastTestPoint = _shipFoamInfo.hull[0][z].center[curY - 1];
        _shipFoamInfo.levelStarts[0][z] = lastTestPoint + interpK * (testPoint - lastTestPoint);
        // interpK*(testPoint - lastTestPoint);
    }
    else
    {
        _shipFoamInfo.levelStarts[0][z] = _shipFoamInfo.hull[0][z].center[TRACE_STEPS_Y - 1];
    }
}

//--------------------------------------------------------------------
void SEAFOAM::InterpolateRightParticle(tShipFoamInfo &_shipFoamInfo, int z, uint32_t dTime)
{
    float interpK;
    CVECTOR lowPoint, highPoint;
    float lowSeaY, highSeaY, seaK;

    lowPoint = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.hull[1][z].center[TRACE_STEPS_Y - 1]);
    highPoint = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.hull[1][z].center[1]);
    lowSeaY = sea->WaveXZ(lowPoint.x, lowPoint.z);
    highSeaY = sea->WaveXZ(highPoint.x, highPoint.z);

    seaK = (highSeaY - lowSeaY) /
           (_shipFoamInfo.hull[1][z].center[1].y - _shipFoamInfo.hull[1][z].center[TRACE_STEPS_Y - 1].y); // k < 0
    if ((lowSeaY > lowPoint.y) && (highSeaY > highPoint.y))
    {
        // above sea
        _shipFoamInfo.levelStarts[1][z] = _shipFoamInfo.hull[1][z].center[1];
        CVECTOR tempVector;
        _shipFoamInfo.shipModel->mtx.MulToInv(CVECTOR(highPoint.x, (lowSeaY + highSeaY) * 0.5f, highPoint.z),
                                              tempVector);
        _shipFoamInfo.levelStarts[1][z].y = tempVector.y;
        return;
    }

    if ((lowSeaY < lowPoint.y) && (highSeaY < highPoint.y))
    {
        // under sea
        _shipFoamInfo.levelStarts[1][z] = _shipFoamInfo.hull[1][z].center[TRACE_STEPS_Y - 1];
        CVECTOR tempVector;
        _shipFoamInfo.shipModel->mtx.MulToInv(CVECTOR(lowPoint.x, (lowSeaY + highSeaY) * 0.5f, lowPoint.z), tempVector);
        _shipFoamInfo.levelStarts[1][z].y = tempVector.y;
        return;
    }

    auto curY = 0;
    auto testPoint = _shipFoamInfo.hull[1][z].center[curY];
    auto lastTestPoint = testPoint;
    auto testY = curY;
    auto lastTestY = testY;

    const auto seaY = /*__max(lowSeaY, highSeaY)*/ (lowSeaY + highSeaY) / 2.0f;
    for (curY = 0; curY < TRACE_STEPS_Y; ++curY)
    {
        testPoint = _shipFoamInfo.shipModel->mtx * _shipFoamInfo.hull[1][z].center[curY];

        if (testPoint.y < seaY)
            break;
        lastTestPoint = testPoint;
        lastTestY = testY;
        testY = curY;
    }

    if (curY > 0 && curY < TRACE_STEPS_Y) //~!~
    {
        // successful search
        interpK = (seaY - lastTestPoint.y) / (testPoint.y - lastTestPoint.y);
        testPoint = _shipFoamInfo.hull[1][z].center[curY];
        lastTestPoint = _shipFoamInfo.hull[1][z].center[curY - 1];
        _shipFoamInfo.levelStarts[1][z] = lastTestPoint + interpK * (testPoint - lastTestPoint);
        // interpK*(testPoint - lastTestPoint);
    }
    else
    {
        _shipFoamInfo.levelStarts[1][z] = _shipFoamInfo.hull[1][z].center[TRACE_STEPS_Y - 1];
    }
}

//--------------------------------------------------------------------
void SEAFOAM::RealizeShipFoam_Particles(tShipFoamInfo &_shipFoamInfo, uint32_t _dTime)
{
    if (!_shipFoamInfo.ship->HasWaterContact())
    {
        for (auto *emitter : _shipFoamInfo.frontEmitter)
        {
            emitter->EnableEmit(false);
            emitter->ProcessParticles(_dTime); // age old particles without a water draw
        }
        _shipFoamInfo.previousBowValid = false;
        _shipFoamInfo.doSplash = false;
        return;
    }
    _shipFoamInfo.frontEmitter[0]->EnableEmit(true);
    _shipFoamInfo.frontEmitter[1]->EnableEmit(true);
    // MODEL *arrow = (MODEL*)core.GetEntityPointer(arrowModel);

    for (auto z = 0; z < TRACE_STEPS_Z; ++z)
    {
        /*
        for (int y=0; y<TRACE_STEPS_Y; ++y)
        {
          CVECTOR fPos = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.hull[0][z].center[y]);
          arrow->mtx.BuildMatrix(CVECTOR(1.0f, 0.0f, 0.0f), fPos);
          arrow->Realize(_dTime);

          fPos = _shipFoamInfo.shipModel->mtx * (_shipFoamInfo.levelStarts[0][z]);
          arrow->mtx.BuildMatrix(CVECTOR(0.0f, 0.0f, 0.0f), fPos);
          arrow->Realize(_dTime);
          arrow->mtx.BuildMatrix(CVECTOR(0.0f, PId2, 0.0f), fPos);
          arrow->Realize(_dTime);
          arrow->mtx.BuildMatrix(CVECTOR(PId2, 0.0f, 0.0f), fPos);
          arrow->Realize(_dTime);

        }
        */
        InterpolateLeftParticle(_shipFoamInfo, z, _dTime);
        InterpolateRightParticle(_shipFoamInfo, z, _dTime);
    }

    uint64_t ticks = 0;
    RDTSC_B(ticks)

    auto frontEmitterPos = 0.5f * (_shipFoamInfo.levelStarts[0][0] + _shipFoamInfo.levelStarts[1][0]);
    const auto shipSpeed = _shipFoamInfo.ship->GetCurrentSpeed();
    auto speedDeltaY = 0.f;
    if (shipSpeed < START_FADE_SPEED)
        speedDeltaY = 1.f * (1.f - shipSpeed / START_FADE_SPEED);
    const auto *shipCharacter = _shipFoamInfo.ship->GetACharacter();
    auto *seaAttributes = sea ? sea->AttributesPointer : nullptr;
    const auto *phonkMode = seaAttributes ? seaAttributes->FindAClass(seaAttributes, "Phonk.Active") : nullptr;
    const bool phonkShip = !_shipFoamInfo.ship->isDead() && shipCharacter &&
        shipCharacter->GetAttributeAsDword("MainCharacter", 0) != 0 &&
        phonkMode && phonkMode->GetAttributeAsDword() != 0;
    frontEmitterPos.z += FOAM_SHIFT_Z + (phonkShip ?
        std::min(.25f * shipSpeed, .08f * _shipFoamInfo.hullInfo.boxsize.z) : .25f * shipSpeed);
    frontEmitterPos.y -= speedDeltaY;
    frontEmitterPos.x -= FOAM_SHIFT_X;
    _shipFoamInfo.frontEmitter[0]->SetEmitter(_shipFoamInfo.shipModel->mtx * frontEmitterPos, CVECTOR(0.f, 1.f, 0.f));
    _shipFoamInfo.frontEmitter[0]->Realize(_dTime);

    frontEmitterPos.x += 2 * FOAM_SHIFT_X;
    _shipFoamInfo.frontEmitter[1]->SetEmitter(_shipFoamInfo.shipModel->mtx * frontEmitterPos, CVECTOR(0.f, 1.f, 0.f));
    _shipFoamInfo.frontEmitter[1]->Realize(_dTime);

    frontEmitterPos.x -= FOAM_SHIFT_X;

    const auto bow = _shipFoamInfo.shipModel->mtx * frontEmitterPos;
    const auto wave = sea->GetTsunamiSample(bow.x, bow.z);
    const auto vortex = sea->GetWhirlpoolSample(bow.x, bow.z);
    auto *character = _shipFoamInfo.ship->GetACharacter();
    auto *emergence = character ? character->FindAClass(character, "Ship.SeaEmergence") : nullptr;
    const bool emerging = emergence && emergence->GetAttributeAsDword("Active", 0) != 0;
    const float entrySplash = emerging ? std::clamp(emergence->GetAttributeAsFloat("Splash"), 0.f, 1.f) : 0.f;
    const unsigned generation = emergence ? emergence->GetAttributeAsDword("Generation", 0) : 0;
    if (generation != _shipFoamInfo.emergenceGeneration ||
        (vortex.serial == 0 && _shipFoamInfo.whirlpoolSerial != 0)) {
        if (_shipFoamInfo.emergenceSplashOwned && wave.serial == 0)
            _shipFoamInfo.frontEmitter[2]->ClearParticles();
        _shipFoamInfo.emergenceSplashOwned = false;
    }
    _shipFoamInfo.emergenceGeneration = generation;
    _shipFoamInfo.whirlpoolSerial = vortex.serial;
    if (wave.serial != _shipFoamInfo.tsunamiSerial)
    {
        _shipFoamInfo.tsunamiSerial = wave.serial;
        _shipFoamInfo.previousBowValid = false;
        if (_shipFoamInfo.tsunamiSplashOwned)
            _shipFoamInfo.frontEmitter[2]->ClearParticles();
        _shipFoamInfo.tsunamiSplashOwned = false;
    }
    const bool stormSplash = isStorm && _dTime > 0 && _shipFoamInfo.previousBowValid &&
        (frontEmitterPos.y - _shipFoamInfo.previousBowY) / static_cast<float>(_dTime) >= 5e-4f;
    const bool waveSplash = _dTime > 0 && wave.foam > .18f;
    if (waveSplash) _shipFoamInfo.tsunamiSplashOwned = true;
    _shipFoamInfo.previousBowY = frontEmitterPos.y;
    _shipFoamInfo.previousBowValid = true;
    const bool emergenceSplash = _dTime > 0 && entrySplash > 0.f;
    if (emergenceSplash) _shipFoamInfo.emergenceSplashOwned = true;
    const auto impact = _shipFoamInfo.ship->GetWaterImpact();
    const bool impactSplash = _dTime > 0 && impact.strength > 0.f;
    const bool newImpact = impactSplash && impact.generation != _shipFoamInfo.waterImpactGeneration;
    if (newImpact)
    {
        _shipFoamInfo.frontEmitter[2]->ClearParticles();
        _shipFoamInfo.waterImpactOwned = true;
    }
    _shipFoamInfo.waterImpactGeneration = impact.generation;
    if (!impactSplash && _shipFoamInfo.waterImpactOwned &&
        !_shipFoamInfo.frontEmitter[2]->HasLivingParticles())
        _shipFoamInfo.waterImpactOwned = false;
    _shipFoamInfo.doSplash = stormSplash || waveSplash || emergenceSplash || impactSplash;
    _shipFoamInfo.frontEmitter[2]->SetSplashScale(impactSplash ? 1.f + 2.f * impact.strength :
        emergenceSplash ? 1.f + 2.f * entrySplash : 1.f);
    _shipFoamInfo.frontEmitter[2]->EnableEmit(_shipFoamInfo.doSplash);
    if (isStorm || wave.serial != 0 || _shipFoamInfo.emergenceSplashOwned ||
        impactSplash || _shipFoamInfo.waterImpactOwned)
    {
        auto mtx2 = _shipFoamInfo.shipModel->mtx;
        mtx2.Pos() = 0.f;
        const auto impactWave = impactSplash ? sea->GetTsunamiSample(impact.position.x, impact.position.z) : wave;
        const auto a = impactSplash ? !CVECTOR(-impactWave.slopeX, 1.f, -impactWave.slopeZ) :
                      waveSplash ? !CVECTOR(-wave.slopeX, 1.f, -wave.slopeZ) :
                                   mtx2 * CVECTOR(0.f, 1.0f, -.15f);
        auto emitter = bow;
        if (emergenceSplash) emitter.y = sea->WaveXZ(emitter.x, emitter.z) + .5f;
        if (impactSplash)
        {
            emitter = impact.position;
            emitter.y = sea->WaveXZ(emitter.x, emitter.z) + .5f;
        }
        _shipFoamInfo.frontEmitter[2]->SetEmitter(emitter, a);
        _shipFoamInfo.frontEmitter[2]->Realize(_dTime);
    }

    // Vortex/entry motion is flowing water, not a repeated bow impact.
    // The ordinary storm producer and real collision/cannon aliases stay owned.
    if (soundService && _shipFoamInfo.doSplash && (impactSplash || (vortex.serial == 0 && !emerging)))
    {
        auto pos = _shipFoamInfo.shipModel->mtx * CVECTOR(0.f, 0.f, _shipFoamInfo.hullInfo.boxsize.z / 2.f);
        pos.y = sea->WaveXZ(pos.x, pos.z);
        if (impactSplash)
        {
            pos = impact.position;
            pos.y = sea->WaveXZ(pos.x, pos.z) + .5f;
        }

        if ((impactSplash && newImpact) || (!impactSplash &&
            (!_shipFoamInfo.sound || !soundService->SoundIsPlaying(_shipFoamInfo.sound))))
        {
            _shipFoamInfo.sound = soundService->SoundPlay("ship_bow", PCM_3D, VOLUME_FX, false, false, true, 0, &pos);
        }
        else if (_shipFoamInfo.sound)
        {
            soundService->SoundSet3DParam(_shipFoamInfo.sound, SM_POSITION, &pos);
        }
    }

    RDTSC_E(ticks)
    // core.Trace("Seafoam realize(carcass->Execute) = %d", ticks);
}

void SEAFOAM::RealizeShipFoam_Mesh(tShipFoamInfo &_shipFoamInfo, uint32_t _dTime)
{
    if (!_shipFoamInfo.ship->HasWaterContact()) return;
    _shipFoamInfo.carcass[0]->Execute(_dTime, _shipFoamInfo.shipModel->mtx, _shipFoamInfo.levelStarts[0]);
    _shipFoamInfo.carcass[1]->Execute(_dTime, _shipFoamInfo.shipModel->mtx, _shipFoamInfo.levelStarts[1]);

    _shipFoamInfo.carcass[0]->Realize(nullptr);
    _shipFoamInfo.carcass[1]->Realize(nullptr);
}

//--------------------------------------------------------------------
uint64_t SEAFOAM::ProcessMessage(MESSAGE &message)
{
    // GUARD(SEAFOAM::ProcessMessage)

    const auto code = message.Long();
    const uint32_t outValue = 0;

    switch (code)
    {
    case MSG_SEA_TSUNAMI_SHORE_SPLASH: {
        const auto serial = static_cast<unsigned>(message.Long());
        if (serial == 0) { ResetTsunamiShore(); return 0; }
        CVECTOR hit, seaward;
        hit.x = message.Float(); hit.y = message.Float(); hit.z = message.Float();
        seaward.x = message.Float(); seaward.y = message.Float(); seaward.z = message.Float();
        const float strength = message.Float();
        if (!sea || !psIni || !std::isfinite(hit.x) || !std::isfinite(hit.y) || !std::isfinite(hit.z) ||
            !std::isfinite(seaward.x) || !std::isfinite(seaward.y) || !std::isfinite(seaward.z) ||
            !std::isfinite(strength) || strength <= 0.f) return 0;
        const auto sample = sea->GetTsunamiSample(hit.x, hit.z);
        if (sample.serial != serial || sample.height <= 0.f) return 0;
        for (auto &splash : tsunamiShoreSplashes)
            if (splash.serial == 0)
            {
                if (!splash.particles)
                {
                    splash.particles = std::make_unique<SEAFOAM_PS>();
                    splash.particles->Init(psIni.get(), "seafoam_front");
                }
                splash.particles->ClearParticles();
                splash.particles->SetSplashScale((.8f + 1.6f * sample.severity) * (.5f + .5f * std::clamp(strength, 0.f, 1.f)));
                splash.hit = hit;
                splash.direction = !CVECTOR(seaward.x * .4f, 1.f, seaward.z * .4f);
                hit.y = std::max(hit.y, sea->WaveXZ(hit.x, hit.z)) + .75f;
                splash.particles->SetEmitter(hit, splash.direction);
                splash.serial = serial;
                splash.age = 0;
                splash.emissionTime = static_cast<uint32_t>(300.f + 700.f * sample.severity);
                splash.lifeTime = splash.emissionTime + 1500;
                return 1;
            }
        return 0;
    }
    case MSG_SHIP_DELETE: {
        auto *const attrs = message.AttributePointer();
        tShipFoamInfo *foamInfo = nullptr;
        if (attrs)
            for (auto ship = 0; ship < shipsCount; ship++)
            {
                if (attrs == shipFoamInfo[ship].ship->GetACharacter())
                {
                    shipFoamInfo[ship].enabled = false;

                    // shipFoamInfo[ship].carcass[0]->Uninitialize();
                    // delete shipFoamInfo[ship].carcass[0];
                    // shipFoamInfo[ship].carcass[1]->Uninitialize();
                    // delete shipFoamInfo[ship].carcass[1];

                    return outValue;
                }
            }
    }
    break;
    }

    return outValue;
    // UNGUARD
}

//--------------------------------------------------------------------
void SEAFOAM::ResetTsunamiShore()
{
    for (auto &splash : tsunamiShoreSplashes)
    {
        if (splash.particles && splash.serial != 0) splash.particles->ClearParticles();
        splash.serial = 0;
        splash.age = splash.emissionTime = splash.lifeTime = 0;
    }
}

void SEAFOAM::RealizeTsunamiShore(uint32_t delta)
{
    const auto event = sea ? sea->GetTsunamiSample(0.f, 0.f) : SeaTsunamiSample{};
    const auto islandID = core.GetEntityId("island");
    auto *island = static_cast<ISLAND_BASE *>(core.GetEntityPointer(islandID));
    if (event.serial == 0 || !island || island->GetCurrentImmersion() != 0.f)
    {
        ResetTsunamiShore();
        return;
    }
    for (auto &splash : tsunamiShoreSplashes)
    {
        if (splash.serial == 0) continue;
        if (splash.serial != event.serial || delta >= splash.lifeTime - splash.age)
        {
            splash.particles->ClearParticles();
            splash.serial = 0;
            splash.age = splash.emissionTime = splash.lifeTime = 0;
            continue;
        }
        splash.age += delta;
        if (splash.age < splash.emissionTime)
        {
            auto hit = splash.hit;
            hit.y = std::max(hit.y, sea->WaveXZ(hit.x, hit.z)) + .75f;
            splash.particles->SetEmitter(hit, splash.direction);
        }
        splash.particles->EnableEmit(delta > 0 && splash.age < splash.emissionTime);
        splash.particles->Realize(delta);
    }
}

void SEAFOAM::Realize(uint32_t _dTime)
{
    // GUARD(SEAFOAM::Realize)

    uint64_t ticks = 0;
    RDTSC_B(ticks)
    RealizeTsunamiShore(_dTime);

    tShipFoamInfo *foamInfo = nullptr;
    int ship;

    for (ship = 0; ship < shipsCount; ship++)
    {
        foamInfo = &shipFoamInfo[ship];
        if (!foamInfo->enabled)
            continue;
        RealizeShipFoam_Particles(*foamInfo, _dTime);
    }

    static CMatrix wMatrix;
    renderer->SetTransform(D3DTS_WORLD, static_cast<D3DMATRIX *>(wMatrix));

    renderer->TextureSet(0, carcassTexture);
    const auto techniqueStarted = renderer->TechniqueExecuteStart("new_seafoam");
    for (ship = 0; ship < shipsCount; ship++)
    {
        foamInfo = &shipFoamInfo[ship];
        if (!foamInfo->enabled)
            continue;
        RealizeShipFoam_Mesh(*foamInfo, _dTime);
    }
    if (techniqueStarted)
        while (renderer->TechniqueExecuteNext())
            ;

    RDTSC_E(ticks)
    // core.Trace("Seafoam realize = %d", ticks);
    // UNGUARD
}

//--------------------------------------------------------------------
void SEAFOAM::Execute(uint32_t dTime)
{
    // GUARD(SEAFOAM::Execute);
    tShipFoamInfo *foamInfo = nullptr;
    float shipSpeed, speedA;

    for (auto ship = 0; ship < shipsCount; ship++)
    {
        foamInfo = &shipFoamInfo[ship];
        if (!foamInfo->enabled)
            continue;

        shipSpeed = foamInfo->ship->GetCurrentSpeed();
        if (shipSpeed < 0.f)
            shipSpeed = 0.f;
        if (shipSpeed > START_FADE_SPEED)
            speedA = 1.f;
        else
            speedA = shipSpeed / START_FADE_SPEED;

        foamInfo->carcass[0]->SetSpeed(U_SPEED_K * shipSpeed / shipFoamInfo->hullInfo.boxsize.z, V_SPEED_K * shipSpeed,
                                       speedA);
        foamInfo->carcass[1]->SetSpeed(U_SPEED_K * shipSpeed / shipFoamInfo->hullInfo.boxsize.z, V_SPEED_K * shipSpeed,
                                       speedA);
    }

    // UNGUARD
}

//--------------------------------------------------------------------
uint32_t SEAFOAM::AttributeChanged(ATTRIBUTES *pA)
{
    const auto *const nm = pA->GetThisName();
    if (storm::iEquals(nm, "Clear")) ResetTsunamiShore();

    if (storm::iEquals(nm, "storm"))
    {
        if (!strcmp(pA->GetThisAttr(), "true"))
            isStorm = true;
        else
            isStorm = false;
    }

    /*if (storm::iEquals(nm, "AddNetShip"))
    {
      entid_t shipID;
      uint32_t dwShipNetID = pA->GetAttributeAsDword();
      if (core.IsNetActive())
      {
        if (NetFindClass(false, &shipID, "NetShip")) do
        {
          Entity * pE = core.GetEntityPointer(shipID);
          if (pE->GetNetID() == dwShipNetID)
          {
            AddShip(&shipID);
            break;
          }
        } while (NetFindClassNext(false, &shipID));
      }
    }*/

    return 0;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
//--------------------------------------------------------------------

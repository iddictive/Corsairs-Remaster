#include "model_realizer.h"

#include "core.h"
#include "c_vector4.h"
#include "lights.h"
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" unsigned StormMetalBeginLandModel(void*, unsigned);
extern "C" bool StormMetalLandLightingReady(void*);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" bool StormMetalSetGeometryEmitter(void*, bool);
#endif


// ============================================================================================
// Construction, destruction
// ============================================================================================

extern float fCausticScale, fCausticDelta, fFogDensity, fCausticDistance;
extern CVECTOR4 v4CausticColor;
extern bool bCausticEnable;
extern float fCausticFrame;
extern int32_t iCausticTex[32];

LocModelRealizer::LocModelRealizer()
{
    lights = nullptr;
    bShow = true;
}

LocModelRealizer::~LocModelRealizer()
{
}

// Initialization
bool LocModelRealizer::Init()
{
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    gs = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    return true;
}

// Execution
void LocModelRealizer::Execute(uint32_t delta_time)
{
}

void LocModelRealizer::Realize(uint32_t delta_time) const
{
    Lights* activeLights = lights;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    if (dynamicLights && StormMetalLandLightingReady(rs->GetD3DDevice())) activeLights = dynamicLights;
#endif
    if (!bShow)
        return;
    auto *pE = core.GetEntityPointer(eid_model);
    if (pE)
    {
        BOOL bLight0Enable;
        uint32_t dwLighting;
        if (activeLights)
        {
            static CVECTOR camPos, camAng;
            rs->GetCamera(camPos, camAng, camAng.x);

            rs->GetRenderState(D3DRS_LIGHTING, &dwLighting);
            rs->GetLightEnable(0, &bLight0Enable);

            // calc lightning at camera pos
            activeLights->SetLightsAt(camPos);

            rs->SetRenderState(D3DRS_LIGHTING, TRUE);
            rs->LightEnable(0, TRUE);
        }

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        struct EmitterScope {
            void *device;
            bool previous;
            ~EmitterScope() { StormMetalSetGeometryEmitter(device, previous); }
        } emitter{rs->GetD3DDevice(), StormMetalSetGeometryEmitter(rs->GetD3DDevice(), geometryEmitter)};
        const unsigned previousLandScope = StormMetalBeginLandModel(rs->GetD3DDevice(), 1);
#endif
        pE->ProcessStage(Stage::realize, delta_time);
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        StormMetalEndLandModel(rs->GetD3DDevice(), previousLandScope);
#endif
        if (activeLights)
        {
            activeLights->UnsetLights();
            rs->SetRenderState(D3DRS_LIGHTING, dwLighting);
            rs->LightEnable(0, bLight0Enable);
        }

        if (bCausticEnable)
        {
            // constants
            // 10 - (caustic scale, caustic frame, 0, 0)
            // 11 - diffuse
            // 12 - (fog density, fog_start, 0, 0)
            // 13 - (0, 0, 0, 0)

            fCausticDelta = fCausticFrame - static_cast<int32_t>(fCausticFrame);
            const auto vec1 = CVECTOR4(fCausticScale, fCausticDelta, 0.0f, 0.0f);
            const auto vec2 = CVECTOR4(fFogDensity, 0.0f, 0.0f, 0.0f);
            const auto vec3 = CVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
            const auto vec4 = CVECTOR4(0.0f, 1.0f, 0.0f, 0.0f);
            const auto vec5 = CVECTOR4(1.0f / fCausticDistance, 1.0f, 0.0f, 0.0f);
            rs->SetVertexShaderConstantF(10, reinterpret_cast<const float *>(&vec1), 1);
            rs->SetVertexShaderConstantF(11, reinterpret_cast<const float *>(&v4CausticColor), 1);
            rs->SetVertexShaderConstantF(12, reinterpret_cast<const float *>(&vec2), 1);
            rs->SetVertexShaderConstantF(13, reinterpret_cast<const float *>(&vec3), 1);
            rs->SetVertexShaderConstantF(14, reinterpret_cast<const float *>(&vec4), 1);
            rs->SetVertexShaderConstantF(15, reinterpret_cast<const float *>(&vec5), 1);

            rs->TextureSet(1, iCausticTex[static_cast<int32_t>(fCausticFrame) % 32]);
            rs->TextureSet(2, iCausticTex[(static_cast<int32_t>(fCausticFrame) + 1) % 32]);

            // draw caustics
            gs->SetCausticMode(true);
            pE->ProcessStage(Stage::realize, 0);
            gs->SetCausticMode(false);
        }
    }
}

// Messages
uint64_t LocModelRealizer::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case 1:
        eid_model = message.EntityID();
        lights = (Lights *)message.Pointer();
        break;
    case 2:
        bShow = message.Long() != 0;
        break;
    }
    return 0;
}

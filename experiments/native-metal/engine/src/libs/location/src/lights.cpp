//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    Lights
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "lights.h"
#include <algorithm>
#include <utility>
#include <vector>
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" void StormMetalSetLightIdentity(void*, unsigned, uint64_t);
extern "C" void StormMetalBeginSceneLights(void*);
extern "C" bool StormMetalSceneLight(void*, uint64_t, const D3DLIGHT9*);
#endif

#include "core.h"
#include "string_compare.hpp"

// ============================================================================================
// Construction, destruction
// ============================================================================================

Lights::Lights()
    : lighter_code(0), lampModels{}, buf{}
{
    rs = nullptr;
    collide = nullptr;
    numTypes = 0;
    maxTypes = 0;
    numLights = 0;
    maxLights = 0;
    for (int32_t i = 0; i < 8; i++)
    {
        lt[i].light = -1;
        lt[i].set = false;
    }
    numLampModels = 0;
}

Lights::~Lights()
{
    for (int32_t i = 0; i < numTypes; i++)
    {
        if (types[i].corona >= 0 && rs)
            rs->TextureRelease(types[i].corona);
        delete types[i].name;
    }
    if (rs)
        for (int32_t i = 1; i < 8; i++)
            rs->LightEnable(i, false);
}

// Initialization
bool Lights::Init()
{
    // DX9 render
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!rs)
        throw std::runtime_error("No service: dx9render");
    collide = static_cast<COLLIDE *>(core.GetService("COLL"));
    // read the parameters
    auto ini = fio->OpenIniFile("RESOURCE\\Ini\\lights.ini");
    if (!ini)
    {
        core.Trace("Location lights not inited -> RESOURCES\\Ini\\lights.ini not found");
        return false;
    }
    char lName[256];
    auto res = ini->GetSectionName(lName, sizeof(lName) - 1);
    while (res)
    {
        lName[sizeof(lName) - 1] = 0;
        int32_t i;
        for (i = 0; i < numTypes; i++)
        {
            if (storm::iEquals(lName, types[i].name))
            {
                core.Trace("Location lights redefinition light: %s", lName);
                break;
            }
        }
        if (i == numTypes)
        {
            types.push_back({});
            // Save the name
            const auto len = strlen(lName) + 1;
            types[numTypes].name = new char[len];
            memcpy(types[numTypes].name, lName, len);
            // Reading parameters
            types[numTypes].color.b = ini->GetFloat(lName, "b", 1.0f);
            types[numTypes].color.g = ini->GetFloat(lName, "g", 1.0f);
            types[numTypes].color.r = ini->GetFloat(lName, "r", 1.0f);
            types[numTypes].color.a = 1.0f;
            types[numTypes].dxLight.Type = D3DLIGHT_POINT;
            types[numTypes].dxLight.Diffuse = types[numTypes].color;
            types[numTypes].dxLight.Range = ini->GetFloat(lName, "range", 10.0f);
            types[numTypes].dxLight.Attenuation0 = ini->GetFloat(lName, "att0", 0.0f);
            types[numTypes].dxLight.Attenuation1 = ini->GetFloat(lName, "att1", 0.0f);
            types[numTypes].dxLight.Attenuation2 = ini->GetFloat(lName, "att2", 1.0f);
            types[numTypes].flicker = ini->GetFloat(lName, "flicker", 0.0f);
            types[numTypes].freq = ini->GetFloat(lName, "freq", 0.0f);
            types[numTypes].flickerSlow = ini->GetFloat(lName, "flickerSlow", 0.0f);
            types[numTypes].freqSlow = ini->GetFloat(lName, "freqSlow", 0.0f);
            types[numTypes].corona = -1;
            types[numTypes].coronaRange = ini->GetFloat(lName, "coronaRange", 20.0f);
            types[numTypes].coronaSize = ini->GetFloat(lName, "coronaSize", 1.0f);
            if (types[numTypes].coronaRange > 0.0f && types[numTypes].coronaSize > 0.0f)
            {
                types[numTypes].invCoronaRange = 1.0f / types[numTypes].coronaRange;
                char texture[256];
                if (ini->ReadString(lName, "corona", texture, sizeof(texture), ""))
                {
                    if (texture[0])
                    {
                        types[numTypes].corona = rs->TextureCreate(texture);
                    }
                }
            }
            types[numTypes].coronaRange2 = types[numTypes].coronaRange * types[numTypes].coronaRange;
            if (types[numTypes].flicker <= 0.0f)
                types[numTypes].freq = 0.0f;
            if (types[numTypes].flicker > 1.0f)
                types[numTypes].flicker = 1.0f;
            if (types[numTypes].freq <= 0.0f)
                types[numTypes].flicker = 0.0f;
            if (types[numTypes].freq > 0.0f)
                types[numTypes].p = 1.0f / types[numTypes].freq;
            if (types[numTypes].flickerSlow <= 0.0f)
                types[numTypes].freqSlow = 0.0f;
            if (types[numTypes].flickerSlow > 1.0f)
                types[numTypes].flickerSlow = 1.0f;
            if (types[numTypes].freqSlow <= 0.0f)
                types[numTypes].flickerSlow = 0.0f;
            if (types[numTypes].freqSlow > 0.0f)
                types[numTypes].pSlow = 1.0f / types[numTypes].freqSlow;
            numTypes++;
        }
        res = ini->GetSectionNameNext(lName, sizeof(lName) - 1);
    }
    if (numTypes == 0)
    {
        core.Trace("Location lights not inited -> 0 light types");
        return false;
    }
    // start executing
    core.SetLayerType(EXECUTE, layer_type_t::execute);
    core.AddToLayer(EXECUTE, GetId(), 10);
    core.SetLayerType(REALIZE, layer_type_t::realize);
    core.AddToLayer(REALIZE, GetId(), -1000);
    return true;
}

// Execution
void Lights::Execute(uint32_t delta_time)
{
    if (core.Controls->GetDebugAsyncKeyState(VK_SHIFT) < 0 && core.Controls->GetDebugAsyncKeyState(VK_F11) < 0)
    {
        for (int32_t i = 0; i < numTypes; i++)
            UpdateLightTypes(i);
    }
    for (int32_t i = 0; i < numLights; i++)
    {
        // See what is there
        auto &l = types[lights[i].type];
        if (l.flicker == 0.0f && l.flickerSlow == 0.0f)
            continue;
        // Updating the state
        auto &ls = lights[i];
        const float elapsed = delta_time * 0.001f;
        // Keep the authored fast frequency and amplitude, but interpolate
        // between random targets so light and shadow intensity cannot jump.
        if (l.freq > 0.0f)
        {
            ls.time += elapsed;
            while (ls.time >= l.p)
            {
                ls.time -= l.p;
                const float previous = ls.itensTarget;
                ls.itensTarget = (1.0f - rand() * 2.0f / RAND_MAX) * l.flicker;
                ls.itensDltFast = ls.itensTarget - previous;
            }
            const float kFast = ls.time * l.freq;
            ls.itens = ls.itensTarget - ls.itensDltFast + kFast * ls.itensDltFast;
        }
        // Smooth flickering
        if (l.freqSlow > 0.0f)
        {
            ls.timeSlow += elapsed;
            while (ls.timeSlow >= l.pSlow)
            {
                ls.timeSlow -= l.pSlow;
                const float previous = ls.itensSlow;
                ls.itensSlow = (1.0f - rand() * 2.0f / RAND_MAX) * l.flickerSlow;
                ls.itensDlt = ls.itensSlow - previous;
            }
            const float kSlow = ls.timeSlow * l.freqSlow;
            ls.i = ls.itensSlow - ls.itensDlt + kSlow * ls.itensDlt + ls.itens;
        }
        else
            ls.i = ls.itens;
        auto k = 1.0f + ls.i;
        if (k < 0.0f)
            k = 0.0f;
        lights[i].color.b = k * l.color.b;
        lights[i].color.g = k * l.color.g;
        lights[i].color.r = k * l.color.r;
    }
}

void Lights::Realize(uint32_t delta_time)
{
    // Camera position
    CVECTOR pos, ang;
    rs->GetCamera(pos, ang, ang.x);

    ///////////////////
    // Dynamic lighting augmentation
    auto lightsAtPos = GetLightsAt(pos);

    // 8 lights in total in D3D. 1 reserved for sun. Smooth both sides of
    // nearest-light selection so crossing its boundary never pops a lamp.
    const float elapsed = delta_time * 0.001f;
    constexpr float attackSeconds = 0.22f;
    constexpr float releaseSeconds = 0.38f;
    uint32_t d3d_light_index = max_d3d_lights - max_d3d_custom_lights;
    for (auto i = std::cbegin(lightsAtPos); i != std::cend(lightsAtPos); ++i)
    {
        const bool selected = d3d_light_index < max_d3d_lights;
        const float target = selected ? 1.0f : 0.0f;
        const float seconds = selected ? attackSeconds : releaseSeconds;
        const float blend = 1.0f - expf(-elapsed / seconds);
        lights[*i].visibility += (target - lights[*i].visibility) * blend;
        if (lights[*i].visibility < 0.0005f)
            lights[*i].visibility = 0.0f;
        else if (lights[*i].visibility > 0.9995f)
            lights[*i].visibility = 1.0f;
        lights[*i].intensity = static_cast<uint8_t>(lights[*i].visibility * 255.0f + 0.5f);
        if (selected)
            d3d_light_index++;
    }

    ////////////////
    // Draw coronas
    CMatrix camMtx;
    rs->GetTransform(D3DTS_VIEW, camMtx);
    rs->SetTransform(D3DTS_VIEW, CMatrix());
    rs->SetTransform(D3DTS_WORLD, CMatrix());
    const auto camPDist = -(pos.x * camMtx.Vx().z + pos.y * camMtx.Vy().z + pos.z * camMtx.Vz().z);
    for (int32_t i = 0, n = 0; i < numLights; i++)
    {
        // Source
        auto &ls = lights[i];
        auto &l = types[ls.type];
        if (l.corona < 0)
            continue;
        // in the foreground
        auto dist = ls.pos.x * camMtx.Vx().z + ls.pos.y * camMtx.Vy().z + ls.pos.z * camMtx.Vz().z + camPDist;
        if (dist < -2.0f * l.coronaSize)
            continue;
        // Distance
        const auto dx = ls.pos.x - pos.x;
        const auto dy = ls.pos.y - pos.y;
        const auto dz = ls.pos.z - pos.z;
        auto d = dx * dx + dy * dy + dz * dz;
        auto isVisible = d < l.coronaRange2;
        if (!isVisible)
            continue;

        // Visibility
        if (collide)
        {
            const auto dist = collide->Trace(core.GetEntityIds(SUN_TRACE), pos,
                                             CVECTOR(ls.pos.x, ls.pos.y, ls.pos.z), lampModels, numLampModels);
            isVisible = dist > 1.0f;
        }
        ls.corona += isVisible ? 0.008f * delta_time : -0.008f * delta_time;
        if (ls.corona <= 0.0f)
        {
            ls.corona = 0.0f;
            continue;
        }
        if (ls.corona > 1.0f)
            ls.corona = 1.0f;
        // Distance
        dist = sqrtf(d);
        d = dist * l.invCoronaRange;
        if (d > 1.0f)
            d = 1.0f;
        auto alpha = 1.0f;
        if (d < 0.3f)
            alpha *= 0.2f + 0.8f * d / 0.3f;
        if (d > 0.4f)
        {
            alpha *= 1.0f - (d - 0.4f) / 0.6f;
            alpha *= alpha;
        }
        alpha *= ls.corona * 255.0f;
        // Deviation coefficient
        d = ls.i * 0.4f;
        if (d < -0.1f)
            d = -0.1f;
        if (d > 0.1f)
            d = 0.1f;
        d += 1.0f;
        // Current size
        const auto size = d * l.coronaSize;
        // Transparency
        alpha *= d;
        if (alpha < 0.0f)
            alpha = 0.0f;
        if (alpha > 255.0f)
            alpha = 255.0f;
        // Position
        auto pos = camMtx * CVECTOR(ls.pos.x, ls.pos.y, ls.pos.z);
        // Colour
        auto c = static_cast<uint32_t>(alpha);
        c |= (c << 24) | (c << 16) | (c << 8);
        // Angle of rotation
        float cs, sn;
        if (dist > 0.0f)
        {
            auto _cs = (dx * camMtx.Vx().z + dz * camMtx.Vz().z);
            auto _sn = (dx * camMtx.Vz().z - dz * camMtx.Vx().z);
            auto kn = _cs * _cs + _sn * _sn;
            if (kn > 0.0f)
            {
                kn = 1.0f / sqrtf(kn);
                _cs *= kn;
                _sn *= kn;
                cs = (_cs * _cs - _sn * _sn);
                sn = 2.0f * _cs * _sn;
            }
            else
            {
                cs = 1.0f;
                sn = 0.0f;
            }
        }
        else
        {
            cs = 1.0f;
            sn = 0.0f;
        }
        // fill in
        buf[n * 6 + 0].pos = pos + CVECTOR(size * (-cs + sn), size * (sn + cs), 0.0f);
        buf[n * 6 + 0].color = c;
        buf[n * 6 + 0].u = 0.0f;
        buf[n * 6 + 0].v = 0.0f;
        buf[n * 6 + 1].pos = pos + CVECTOR(size * (-cs - sn), size * (sn - cs), 0.0f);
        buf[n * 6 + 1].color = c;
        buf[n * 6 + 1].u = 0.0f;
        buf[n * 6 + 1].v = 1.0f;
        buf[n * 6 + 2].pos = pos + CVECTOR(size * (cs + sn), size * (-sn + cs), 0.0f);
        buf[n * 6 + 2].color = c;
        buf[n * 6 + 2].u = 1.0f;
        buf[n * 6 + 2].v = 0.0f;
        buf[n * 6 + 3].pos = buf[n * 6 + 2].pos;
        buf[n * 6 + 3].color = c;
        buf[n * 6 + 3].u = 1.0f;
        buf[n * 6 + 3].v = 0.0f;
        buf[n * 6 + 4].pos = buf[n * 6 + 1].pos;
        buf[n * 6 + 4].color = c;
        buf[n * 6 + 4].u = 0.0f;
        buf[n * 6 + 4].v = 1.0f;
        buf[n * 6 + 5].pos = pos + CVECTOR(size * (cs - sn), size * (-sn - cs), 0.0f);
        buf[n * 6 + 5].color = c;
        buf[n * 6 + 5].u = 1.0f;
        buf[n * 6 + 5].v = 1.0f;
        n++;
        rs->TextureSet(0, l.corona);
        rs->DrawPrimitiveUP(D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, n * 2, buf, sizeof(Vertex),
                            "Coronas");
        n = 0;
    }
    rs->SetTransform(D3DTS_VIEW, camMtx);

    // Debug
    if (core.Controls->GetDebugAsyncKeyState(VK_SHIFT) < 0 && core.Controls->GetDebugAsyncKeyState(VK_SPACE) < 0)
    {
        PrintDebugInfo();
    }
}

// Find source index
int32_t Lights::FindLight(const char *name)
{
    if (!name || !name[0])
        return -1;
    for (int32_t i = 0; i < numTypes; i++)
        if (storm::iEquals(name, types[i].name))
            return i;
    return -1;
}

// Add source to location
void Lights::AddLight(int32_t index, const CVECTOR &pos)
{
    if (index == -1)
        return;
    Assert(index >= 0 && index < numTypes);
    if (numLights >= maxLights)
    {
        maxLights += 32;
        lights.resize(maxLights);
    }
    lights[numLights].color = types[index].color;
    lights[numLights].pos.x = pos.x;
    lights[numLights].pos.y = pos.y;
    lights[numLights].pos.z = pos.z;
    lights[numLights].time = 0.0f;
    lights[numLights].itens = 0.0f;
    lights[numLights].itensTarget = 0.0f;
    lights[numLights].itensDltFast = 0.0f;
    lights[numLights].itensSlow = 0.0f;
    lights[numLights].itensDlt = 0.0f;
    lights[numLights].timeSlow = 0.0f;
    lights[numLights].visibility = 0.0f;
    lights[numLights].type = index;
    lights[numLights].intensity = 0;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    lights[numLights].metalLightId=nextMetalLightId++;
#endif

    // Send a message to the lighter
    if (const auto eid = core.GetEntityId("Lighter"))
    {
        core.Send_Message(eid, "sffffffffffs", "AddLight", pos.x, pos.y, pos.z, types[index].color.r,
                          types[index].color.g, types[index].color.b, types[index].dxLight.Attenuation0,
                          types[index].dxLight.Attenuation1, types[index].dxLight.Attenuation2,
                          types[index].dxLight.Range, types[index].name);
    }

    numLights++;
}

// Add lantern model
bool Lights::AddLampModel(const entid_t lampModel)
{
    if (numLampModels >= sizeof(lampModels) / sizeof(entid_t))
        return false;
    lampModels[numLampModels++] = lampModel;
    return true;
}

void Lights::DelAllLights()
{
    aMovingLight.clear();
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    ResetMetalShadowSelection();
#endif
    numLights = 0;
}

// Add portable source
int32_t Lights::AddMovingLight(const char *type, const CVECTOR &pos)
{
    int32_t idx;
    for (idx = 0; idx < 1000; idx++)
    {
        int32_t n;
        for (n = 0; n < aMovingLight.size(); n++)
            if (aMovingLight[n].id == idx)
                break;
        if (n == aMovingLight.size())
            break;
    }
    if (idx == 1000)
        return -1;

    const auto nType = FindLight(type);
    if (nType < 0)
        return -1;

    aMovingLight.emplace_back(MovingLight{idx, static_cast<int32_t>(numLights)});
    AddLight(nType, pos);
    return idx;
}

// Put portable source in new position
void Lights::UpdateMovingLight(int32_t id, const CVECTOR &pos)
{
    for (int32_t n = 0; n < aMovingLight.size(); n++)
    {
        if (aMovingLight[n].id == id)
        {
            const auto i = aMovingLight[n].light;
            if (i >= 0 && i < numLights)
                lights[i].pos = *(D3DVECTOR *)&pos;
            return;
        }
    }
}

// Remove portable source
void Lights::DelMovingLight(int32_t id)
{
    for (int32_t n = 0; n < aMovingLight.size(); n++)
        if (aMovingLight[n].id == id)
        {
            const auto removedIndex=aMovingLight[n].light;
            numLights--;
            for (auto i = aMovingLight[n].light; i < numLights; i++)
                lights[i] = lights[i + 1];
            aMovingLight.erase(aMovingLight.begin() + n);
            for (auto& source:aMovingLight)
                if (source.light>removedIndex) --source.light;
            return;
        }
}

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
void Lights::ResetMetalShadowSelection()
{
    metalShadowSelector.reset();
}

unsigned Lights::GetShadowLights(const CVECTOR& pos, MetalShadowLight* out, unsigned capacity)
{
    struct Candidate { float distanceSquared; uint64_t id; D3DLIGHT9 light; };
    std::vector<Candidate> candidates;
    metal_shadow::Plane planes[4];
    const auto* viewPlanes=rs->GetPlanes();
    for(unsigned i=0;i<4;i++)planes[i]={viewPlanes[i].Nx,viewPlanes[i].Ny,viewPlanes[i].Nz,viewPlanes[i].D};
    for (int32_t index=0; index<numLights; ++index) {
        const auto& source=lights[index]; const auto& type=types[source.type];
        if(type.dxLight.Type!=D3DLIGHT_POINT || type.dxLight.Range<=.1f)continue;
        if(source.color.r<=0 && source.color.g<=0 && source.color.b<=0)continue;
        const float dx=pos.x-source.pos.x,dy=pos.y-source.pos.y,dz=pos.z-source.pos.z;
        const float distanceSquared=dx*dx+dy*dy+dz*dz;
        // A distant lamp still owns shadows on visible receivers inside its
        // influence sphere. Player distance is only an initial budget ranking.
        const float position[]={source.pos.x,source.pos.y,source.pos.z};
        if(!metal_shadow::overlapsView(planes,position,type.dxLight.Range))continue;
        auto light=type.dxLight; light.Position=source.pos; light.Diffuse=source.color;
        candidates.push_back({distanceSquared,source.metalLightId,light});
    }
    const auto candidateFor=[&](uint64_t id)->const Candidate*{
        const auto it=std::find_if(candidates.begin(),candidates.end(),[&](const Candidate& candidate){return candidate.id==id;});
        return it==candidates.end()?nullptr:&*it;
    };
    // Keep admitted sources while their influence remains visible; walking must
    // not cross-fade shadows between otherwise unchanged local lamps.
    std::vector<metal_shadow::Candidate> identities;
    for(const auto& candidate:candidates)identities.push_back({candidate.id,candidate.distanceSquared});
    const auto selections=metalShadowSelector.select(std::move(identities));
    unsigned count=0;
    for(const auto& selection:selections){
        if(count>=std::min(capacity,metal_shadow::Capacity))break;
        if(const auto* candidate=candidateFor(selection.id))out[count++]={selection.id,candidate->light,selection.weight};
    }
    return count;
}

void Lights::PublishMetalSceneLights()
{
    // One source-owned catalog for the whole frame. Camera/player selection of
    // legacy D3D slots must never switch lighting on a fixed world receiver.
    StormMetalBeginSceneLights(rs->GetD3DDevice());
    for (int32_t index=0; index<numLights; ++index) {
        const auto& source=lights[index];
        auto light=types[source.type].dxLight;
        if(light.Type!=D3DLIGHT_POINT || light.Range<=.1f)continue;
        light.Position=source.pos;
        light.Diffuse.r=source.color.r;
        light.Diffuse.g=source.color.g;
        light.Diffuse.b=source.color.b;
        if(light.Diffuse.r<=0 && light.Diffuse.g<=0 && light.Diffuse.b<=0)continue;
        if(!StormMetalSceneLight(rs->GetD3DDevice(),source.metalLightId,&light))break;
    }
}

bool Lights::GetShadowLight(const CVECTOR& pos, uint64_t& id, D3DLIGHT9& light, bool stableIndoor)
{
    // One cube is available. Pin its owner to authored scene order indoors,
    // never nearest camera/player distance or instantaneous flicker intensity.
    // Every other lamp still contributes through the complete scene catalog.
    if (stableIndoor) {
        for (int32_t index=0; index<numLights; ++index) {
            const auto& source=lights[index];
            light=types[source.type].dxLight;
            if(light.Type!=D3DLIGHT_POINT || light.Range<=.1f)continue;
            light.Position=source.pos;
            light.Diffuse.r=source.color.r;
            light.Diffuse.g=source.color.g;
            light.Diffuse.b=source.color.b;
            id=uint64_t(index)+1;
            return true;
        }
        id=0;return false;
    }
    for (auto index : GetLightsAt(pos)) {
        const auto& source = lights[index];
        light = types[source.type].dxLight;
        if (light.Type != D3DLIGHT_POINT || light.Range <= .1f) continue;
        const float dx = pos.x-source.pos.x, dy = pos.y-source.pos.y, dz = pos.z-source.pos.z;
        if (dx*dx+dy*dy+dz*dz >= light.Range*light.Range) continue;
        light.Diffuse.r = source.color.r;
        light.Diffuse.g = source.color.g;
        light.Diffuse.b = source.color.b;
        if (light.Diffuse.r <= 0 && light.Diffuse.g <= 0 && light.Diffuse.b <= 0) continue;
        light.Position = source.pos;
        id = uint64_t(index) + 1; // Stable only inside this source-owned frame.
        return true;
    }
    id = 0;
    return false;
}
#endif

// Set light sources for the character
void Lights::SetLightsAt(const CVECTOR &pos)
{
    // get lights at specified pos
    auto const lightsAtPos = GetLightsAt(pos);

    // unset all lights
    UnsetLights();

    // enable only involved lights
    uint32_t d3d_light_index = max_d3d_lights - max_d3d_custom_lights;
    for (auto i = std::cbegin(lightsAtPos); i != std::cend(lightsAtPos) && d3d_light_index < max_d3d_lights; ++i)
    {
        // Setting the source
        LightType &l = types[lights[*i].type];
        l.dxLight.Diffuse.r = lights[*i].color.r * static_cast<float>(lights[*i].intensity) / 255.0f;
        l.dxLight.Diffuse.g = lights[*i].color.g * static_cast<float>(lights[*i].intensity) / 255.0f;
        l.dxLight.Diffuse.b = lights[*i].color.b * static_cast<float>(lights[*i].intensity) / 255.0f;
        l.dxLight.Position = lights[*i].pos;

        rs->SetLight(d3d_light_index, &l.dxLight);
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        StormMetalSetLightIdentity(rs->GetD3DDevice(), d3d_light_index, lights[*i].metalLightId);
#endif
        rs->LightEnable(d3d_light_index, true);
        lt[d3d_light_index].light = *i;
        lt[d3d_light_index].set = true;

        d3d_light_index++;
    }
}

void Lights::UnsetLights()
{
    for (auto i = max_d3d_lights - max_d3d_custom_lights; i < max_d3d_lights; i++)
    {
        if (lt[i].set)
        {
            lt[i].set = false;
            rs->LightEnable(i, false);
        }
    }
}

// Update source types
void Lights::UpdateLightTypes(int32_t i)
{
    auto ini = fio->OpenIniFile("RESOURCE\\Ini\\lights.ini");
    if (!ini)
        return;
    // Source name
    char *lName = types[i].name;
    // Reading parameters
    types[i].color.b = ini->GetFloat(lName, "b", 1.0f);
    types[i].color.g = ini->GetFloat(lName, "g", 1.0f);
    types[i].color.r = ini->GetFloat(lName, "r", 1.0f);
    types[i].color.a = 1.0f;
    types[i].dxLight.Type = D3DLIGHT_POINT;
    types[i].dxLight.Diffuse = types[i].color;
    types[i].dxLight.Range = ini->GetFloat(lName, "range", 10.0f);
    types[i].dxLight.Attenuation0 = ini->GetFloat(lName, "att0", 0.0f);
    types[i].dxLight.Attenuation1 = ini->GetFloat(lName, "att1", 0.0f);
    types[i].dxLight.Attenuation2 = ini->GetFloat(lName, "att2", 1.0f);
    types[i].flicker = ini->GetFloat(lName, "flicker", 0.0f);
    types[i].freq = ini->GetFloat(lName, "freq", 0.0f);
    types[i].flickerSlow = ini->GetFloat(lName, "flickerSlow", 0.0f);
    types[i].freqSlow = ini->GetFloat(lName, "freqSlow", 0.0f);
    types[i].corona = -1;
    types[i].coronaRange = ini->GetFloat(lName, "coronaRange", 20.0f);
    types[i].coronaSize = ini->GetFloat(lName, "coronaSize", 1.0f);
    if (types[i].coronaRange > 0.0f && types[i].coronaSize > 0.0f)
    {
        types[i].invCoronaRange = 1.0f / types[i].coronaRange;
        char texture[256];
        if (ini->ReadString(lName, "corona", texture, sizeof(texture), ""))
        {
            if (texture[0])
            {
                types[i].corona = rs->TextureCreate(texture);
            }
        }
    }
    types[i].coronaRange2 = types[i].coronaRange * types[i].coronaRange;
    if (types[i].flicker <= 0.0f)
        types[i].freq = 0.0f;
    if (types[i].flicker > 1.0f)
        types[i].flicker = 1.0f;
    if (types[i].freq <= 0.0f)
        types[i].flicker = 0.0f;
    if (types[i].freq > 0.0f)
        types[i].p = 1.0f / types[i].freq;
    if (types[i].flickerSlow <= 0.0f)
        types[i].freqSlow = 0.0f;
    if (types[i].flickerSlow > 1.0f)
        types[i].flickerSlow = 1.0f;
    if (types[i].freqSlow <= 0.0f)
        types[i].flickerSlow = 0.0f;
    if (types[i].freqSlow > 0.0f)
        types[i].pSlow = 1.0f / types[i].freqSlow;
    for (int32_t c = 0; c < numLights; c++)
    {
        if (lights[c].type != i)
            continue;
        lights[c].color = types[i].color;
    }
}

void Lights::PrintDebugInfo()
{
    for (uint32_t i = 0; i < numLights; ++i)
    {
        auto lightPos = *(CVECTOR*)&lights[i].pos;
        auto scale = 1.0f;

        static CMatrix mtx, view, prj;
        static D3DVIEWPORT9 vp;
        MTX_PRJ_VECTOR vrt;
        rs->GetTransform(D3DTS_VIEW, view);
        rs->GetTransform(D3DTS_PROJECTION, prj);
        mtx.EqMultiply(view, prj);
        view.Transposition();
        rs->GetViewport(&vp);
        mtx.Projection(&lightPos, &vrt, 1, vp.Width * 0.5f, vp.Height * 0.5f, sizeof(CVECTOR),
                       sizeof(MTX_PRJ_VECTOR));
        vrt.y -= rs->CharHeight(FONT_DEFAULT) / 2;

        CVECTOR pos, ang;
        rs->GetCamera(pos, ang, ang.x);
        // visibility
        const auto camPDist = -(pos.x * view.Vx().z + pos.y * view.Vy().z + pos.z * view.Vz().z);

        auto &ls = lights[i];
        auto &l = types[ls.type];
        if (l.corona < 0)
            continue;
        // in the foreground
        auto dist = ls.pos.x * view.Vx().z + ls.pos.y * view.Vy().z + ls.pos.z * view.Vz().z + camPDist;
        if (dist < -2.0f * l.coronaSize)
            continue;
        // Distance
        const auto dx = ls.pos.x - pos.x;
        const auto dy = ls.pos.y - pos.y;
        const auto dz = ls.pos.z - pos.z;
        auto d = dx * dx + dy * dy + dz * dz;
        auto isVisible = d < l.coronaRange2;
        if (!isVisible)
            continue;

        rs->SetTransform(D3DTS_VIEW, CMatrix());
        rs->SetTransform(D3DTS_WORLD, CMatrix());

        rs->ExtPrint(FONT_DEFAULT, D3DCOLOR_ARGB(255, 255, 255, 255), 0x00000000, PR_ALIGN_CENTER, 
            false, scale, 0, 0,
                     static_cast<int32_t>(vrt.x), static_cast<int32_t>(vrt.y), fmt::format("{}", d).c_str());

        // print idx
        auto color = D3DCOLOR_ARGB(255, 233, 30, 30);
        vrt.y -= 1.5 * rs->CharHeight(FONT_DEFAULT) / 2;
        for (const auto [_, light] : lt)
        {
            if (i == light)
            {
                color = D3DCOLOR_ARGB(255, 30, 233, 30);
            }
        }
        rs->ExtPrint(FONT_DEFAULT, color, 0x00000000, PR_ALIGN_CENTER, false, scale, 0, 0, static_cast<int32_t>(vrt.x),
                     static_cast<int32_t>(vrt.y),
                     fmt::format("{}", i).c_str());

        
        rs->SetTransform(D3DTS_VIEW, view);
    }
}

#include "particles.h"

#include "../particles.h"
#include "core.h"
#include "entity.h"
#include "math_inlines.h"
#include "p_system.h"
#include "string_compare.hpp"
#include "shared/messages.h"
#include <algorithm>
#include <iterator>
#include <utility>

#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
#include "render_device_api.hpp"
#endif

PARTICLES::PARTICLES()
{
    bSystemDelete = false;
    pService = nullptr;
    pManager = nullptr;
    CreationCapture = false;
}

PARTICLES::~PARTICLES()
{
    bSystemDelete = true;
    DeleteAll();
}

bool PARTICLES::Init()
{
    core.AddToLayer(REALIZE, GetId(), 0xfffff);
    core.AddToLayer(EXECUTE, GetId(), 0);

    pService = static_cast<IParticleService *>(core.GetService("ParticleService"));
    Assert(pService);
    pManager = pService->DefManager();
    Assert(pManager);
    return true;
}

uint64_t PARTICLES::ProcessMessage(MESSAGE &message)
{
    const auto code = message.Long();

    CVECTOR pos, angles;
    int32_t lifetime;

    switch (code)
    {
        // Pause all particles
        // new not paused ones are emitted ...
    case PS_PAUSEALL: {
        PauseAllActive(message.Long() != 0);
        break;
    }

        // Delete a specific system
    case PS_DELETE: {
        DeleteSystem(message.Long());
        break;
    }

    case PS_CREATIONCAPTURE_BEG: {
        CreationCapture = true;
        break;
    }
    case PS_CREATIONCAPTURE_END: {
        CreationCapture = false;
        break;
    }
    case PS_CLEAR_CAPTURED: {
        DeleteCaptured();
        break;
    }

        // delete everything
    case PS_CLEARALL: {
        DeleteAll();
        break;
    }
    // Only explicitly marked naval effects participate in spatial admission.
    case PS_CREATE_NAVAL_EFFECT: {
        const std::string &name = message.String();
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();
        angles.x = message.Float();
        angles.y = message.Float();
        angles.z = message.Float();
        lifetime = message.Long();
        if (lifetime > 0 && lifetime < 50) lifetime *= 1000;
        const auto category = static_cast<uint32_t>(message.Long());
        if (category < 1 || category > 10)
            return 0;

        const int queueIndex = NavalQueueIndex(category);
        if (queueIndex >= 0)
        {
            auto &queue = NavalQueues[queueIndex];
            if (category == 2 || category == 10) queue.Hard = 1080;
            PendingEffect effect{name, pos, angles, static_cast<uint32_t>(lifetime), CreationCapture};
            if (category == 1 || category == 2 || category == 10)
            {
                const auto nearby = [&](const SystemInfo *info) {
                    const float dx = info->NavalX - pos.x, dz = info->NavalZ - pos.z;
                    if (dx * dx + dz * dz >= 0.25f) return false;
                    Matrix transform; info->pSystem->GetSystem()->GetTransform(transform);
                    const float dy = transform.v.pos.y - pos.y;
                    return dx * dx + dy * dy + dz * dz < 0.25f;
                };
                size_t activeNear = 0, fadingNear = 0;
                auto firstNear = queue.Active.end();
                auto firstFadingNear = queue.Fading.end();
                for (auto it = queue.Active.begin(); it != queue.Active.end(); ++it)
                    if (nearby(*it)) { if (firstNear == queue.Active.end()) firstNear = it; ++activeNear; }
                for (auto it = queue.Fading.begin(); it != queue.Fading.end(); ++it)
                    if (nearby(*it)) { if (firstFadingNear == queue.Fading.end()) firstFadingNear = it; ++fadingNear; }
                if (activeNear + fadingNear >= 12)
                {
                    SystemInfo *old = nullptr;
                    for (auto *info : queue.Fading) if (nearby(info) && !info->NavalHasUpdated) { old = info; break; }
                    if (!old) for (auto *info : queue.Active) if (nearby(info) && !info->NavalHasUpdated) { old = info; break; }
                    if (old) { bSystemDelete = true; RemoveSystem(old->Self, true); bSystemDelete = false; }
                    firstNear = std::find_if(queue.Active.begin(), queue.Active.end(), nearby);
                }
                if (activeNear >= 3 && firstNear != queue.Active.end() && (*firstNear)->PassedTime >= 32)
                {
                    auto *old = *firstNear;
                    if (queue.Next == firstNear) { queue.Next = std::next(firstNear); if (queue.Next == queue.Active.end() && queue.Active.size() > 1) queue.Next = queue.Active.begin(); }
                    if (queue.Active.size() == 1) queue.Next = queue.Active.end();
                    old->pSystem->GetSystem()->BeginNavalFade(0.25f);
                    queue.Fading.splice(queue.Fading.end(), queue.Active, old->NavalIt);
                    old->Fading = true;
                }
                if (queue.Active.size() >= queue.Soft && !queue.Active.empty())
                {
                    auto it = queue.Active.begin();
                    auto *old = *it;
                    if (old->PassedTime >= 32)
                    {
                        if (queue.Next == it) { queue.Next = std::next(it); if (queue.Next == queue.Active.end() && queue.Active.size() > 1) queue.Next = queue.Active.begin(); }
                        if (queue.Active.size() == 1) queue.Next = queue.Active.end();
                        old->pSystem->GetSystem()->BeginNavalFade(0.25f);
                        queue.Fading.splice(queue.Fading.end(), queue.Active, old->NavalIt);
                        old->Fading = true;
                    }
                }
                if (queue.Active.size() + queue.Fading.size() >= queue.Hard)
                {
                    auto *old = !queue.Fading.empty() ? queue.Fading.front() : queue.Active.front();
                    bSystemDelete = true;
                    RemoveSystem(old->Self, true);
                    bSystemDelete = false;
                }
                return reinterpret_cast<uintptr_t>(CreateNavalSystem(queue, category, effect));
            }
            if (queue.Pending.empty() && queue.Active.size() < queue.Soft &&
                queue.Active.size() + queue.Fading.size() < queue.Hard)
                return reinterpret_cast<uintptr_t>(CreateNavalSystem(queue, category, effect));
            if (queue.Pending.size() == queue.PendingLimit)
                queue.Pending.pop_front();
            queue.Pending.push_back(std::move(effect));
            return 0;
        }

        // Projectile-owned systems cannot be evicted: flying balls retain a
        // raw pointer to their emitter. Preserve the original spatial limit
        // for shrapnel fragments and shrapnel-shell trails instead.
        if (category == 7 || category == 8)
        {
            uint32_t nearby = 0;
            for (const auto &info : CreatedSystems)
            {
                if (info.NavalEffect != category || !info.pSystem->GetSystem()->IsAlive()) continue;
                const float dx = info.NavalX - pos.x, dz = info.NavalZ - pos.z;
                if (dx * dx + dz * dz <= 25.0f && ++nearby == 3) return 0;
            }
        }

        // Hull impact splinters persist for several seconds. Bound overlapping
        // bursts across the fleet without affecting smoke or other effects.
        if (category == 9 && HullEffects >= 6)
            return 0;

        auto *system = CreateSystem(name.c_str(), lifetime, CreationCapture);
        if (!system)
            return 0;
        auto &info = CreatedSystems.back();
        info.NavalEffect = category;
        if (category == 9)
            ++HullEffects;
        info.NavalX = pos.x;
        info.NavalZ = pos.z;
        system->SetEmitter(pos, angles);
        system->SetDelay(0);
        system->SetLifeTime(lifetime);
        return reinterpret_cast<uintptr_t>(system);
    }
        // create system (string name, float x, y, z position, float rx, ry, rz rotation, float life_time lifetime)
    case PS_CREATE_RIC: {
        const std::string &ps_name = message.String();
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();

        angles.x = message.Float();
        angles.y = message.Float();
        angles.z = message.Float();
        lifetime = message.Long();

        auto *pSystem = CreateSystem(ps_name.c_str(), lifetime, CreationCapture);
        if (!pSystem)
            return 0;

        pSystem->SetEmitter(pos, angles);
        pSystem->SetDelay(0);
        if (lifetime > 0 && lifetime < 50) lifetime *= 1000;
        pSystem->SetLifeTime(lifetime);
        return (uintptr_t)pSystem;

        break;
    }
    case PS_CREATEX_RIC: {
        throw std::runtime_error("Unsupported particle manager command !!!");
    }

        // create a system
    case PS_CREATE: {
        const std::string &ps_name = message.String();
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();

        angles.x = message.Float();
        angles.y = message.Float();
        angles.z = message.Float();
        lifetime = message.Long();

        auto *pSystem = CreateSystem(ps_name.c_str(), lifetime, CreationCapture);
        if (!pSystem)
            return 0;

        pSystem->SetEmitter(pos, angles);
        pSystem->SetDelay(0);
        if (lifetime > 0 && lifetime < 50) lifetime *= 1000;
        pSystem->SetLifeTime(lifetime);
        return (uintptr_t)pSystem;

        break;
    }
        // create a system
    case PS_CREATEX: {
        const std::string &ps_name = message.String();
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();

        Vector normal;
        normal.x = message.Float();
        normal.y = message.Float();
        normal.z = message.Float();
        const auto fLen = static_cast<double>(normal.Normalize());

        if (fLen)
        {
            angles.y = normal.GetAY();
            auto fDiv = -(normal.y / fLen);
            fDiv = Min(Max(fDiv, -1.0), 1.0);
            angles.x = static_cast<float>(asin(fDiv));
        }
        else
        {
            angles.x = 0.0f;
            angles.y = 0.0f;
        }

        angles.z = 0.0f;

        lifetime = message.Long();

        auto *pSystem = CreateSystem(ps_name.c_str(), lifetime, CreationCapture);
        if (!pSystem)
            return 0;

        pSystem->SetEmitter(pos, angles);
        pSystem->SetDelay(0);
        if (lifetime > 0 && lifetime < 50) lifetime *= 1000;
        pSystem->SetLifeTime(lifetime);
        return (uintptr_t)pSystem;

        break;
    }
    case PS_ADDTRACKPOINT: {
        throw std::runtime_error("Unsupported particle manager command !!!");
    }
    case PS_USESURFACE: {
        throw std::runtime_error("Unsupported particle manager command !!!");
    }
    case PS_VALIDATE_PARTICLE: {
        auto *const SystemID = reinterpret_cast<PARTICLE_SYSTEM *>(message.Pointer());
        for (const auto &info : CreatedSystems)
            if (info.pSystem == SystemID)
                return 1;
        return 0;
        break;
    }
    }
    return 0;
}

PARTICLE_SYSTEM *PARTICLES::CreateSystem(const char *pFileName, uint32_t LifeTime, bool captured)
{
    // std::string pFullFileName;
    // pFullFileName = "resource\\particles\\";
    // pFullFileName += pFileName;
    // pFullFileName.AddExtention(".xps");
    //psnip_trap(); //~!~
    auto path = std::filesystem::path() / "resource" / "particles" / pFileName;
    std::string pathStr = path.extension().string();
    if (!storm::iEquals(pathStr, ".xps"))
        path += ".xps";
    pathStr = path.string();
    // MessageBoxA(NULL, (LPCSTR)path.c_str(), "", MB_OK); //~!~

    // core.Trace("K2 Particles Wrapper: Create system '%s'", pFileName);
    IParticleSystem *pSys = pManager->CreateParticleSystemEx(pathStr.c_str(), __FILE__, __LINE__);
    if (!pSys && (storm::iEquals(pFileName, "splash_big") || storm::iEquals(pFileName, "splash_large")))
    {
        pathStr = (std::filesystem::path() / "resource" / "particles" / "Splash.xps").string();
        pSys = pManager->CreateParticleSystemEx(pathStr.c_str(), __FILE__, __LINE__);
    }
    if (!pSys)
    {
        // core.Trace("Can't create particles system '%s'", pFileName);
        return nullptr;
    }

    pSys->AutoDelete(false);

    auto *pNewPS = new PARTICLE_SYSTEM(pSys);
    pNewPS->SetManager(this);

    // core.Trace("PSYS Created ok");

    CreatedSystems.emplace_back();
    auto &Info = CreatedSystems.back();
    Info.Self = std::prev(CreatedSystems.end());
    Info.pSystem = pNewPS;
    Info.LifeTime = LifeTime;
    Info.FileName = pathStr;
    if (captured)
    {
        Info.Captured = true;
        CaptureBuffer.push_back(&Info);
        Info.CaptureIt = std::prev(CaptureBuffer.end());
    }

    return pNewPS;
}

int PARTICLES::NavalQueueIndex(uint32_t category)
{
    return category == 1 ? 0 : category == 2 ? 1 : category == 10 ? 2 : -1;
}

PARTICLE_SYSTEM *PARTICLES::CreateNavalSystem(NavalQueue &queue, uint32_t category, const PendingEffect &effect)
{
    auto *system = CreateSystem(effect.Name.c_str(), effect.LifeTime, effect.Captured);
    if (!system)
        return nullptr;
    system->GetSystem()->SetNavalBudgeted(true);
    auto &info = CreatedSystems.back();
    info.NavalEffect = category;
    info.NavalX = effect.Pos.x;
    info.NavalZ = effect.Pos.z;
    info.Budgeted = true;
    const bool wasEmpty = queue.Active.empty();
    queue.Active.push_back(&info);
    info.NavalIt = std::prev(queue.Active.end());
    if (wasEmpty)
        queue.Next = info.NavalIt;
    system->SetEmitter(effect.Pos, effect.Angles);
    system->SetDelay(0);
    system->SetLifeTime(effect.LifeTime);
    return system;
}

void PARTICLES::RemoveSystem(std::list<SystemInfo>::iterator it, bool destroy)
{
    auto &info = *it;
    if (info.Captured)
        CaptureBuffer.erase(info.CaptureIt);
    if (info.NavalEffect == 9)
        --HullEffects;
    if (info.Budgeted)
    {
        auto &queue = NavalQueues[NavalQueueIndex(info.NavalEffect)];
        if (info.Fading)
            queue.Fading.erase(info.NavalIt);
        else
        {
            if (queue.Next == info.NavalIt)
            {
                queue.Next = std::next(info.NavalIt);
                if (queue.Next == queue.Active.end() && queue.Active.size() > 1)
                    queue.Next = queue.Active.begin();
            }
            queue.Active.erase(info.NavalIt);
        }
    }
    auto *system = info.pSystem;
    CreatedSystems.erase(it);
    if (destroy)
        delete system;
}

void PARTICLES::DeleteSystem(uintptr_t SystemID)
{
    bSystemDelete = true;
    for (auto it = CreatedSystems.begin(); it != CreatedSystems.end(); ++it)
    {
        if (it->pSystem == (PARTICLE_SYSTEM *)SystemID)
        {
            RemoveSystem(it, true);
            bSystemDelete = false;
            return;
        }
    }

    // core.Trace("Can't delete particle system with GUID %d", SystemID);

    bSystemDelete = false;
}

void PARTICLES::DeleteAll()
{
    bSystemDelete = true;
    for (auto &queue : NavalQueues)
        queue.Pending.clear();
    while (!CreatedSystems.empty())
        RemoveSystem(CreatedSystems.begin(), true);
    bSystemDelete = false;
}

void PARTICLES::DeleteResource(PARTICLE_SYSTEM *pResource)
{
    if (bSystemDelete)
        return;

    for (auto it = CreatedSystems.begin(); it != CreatedSystems.end(); ++it)
    {
        if (it->pSystem == pResource)
        {
            RemoveSystem(it, false);
            return;
        }
    }
}

void PARTICLES::Realize(uint32_t Delta_Time)
{
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
    // Sea actors have finished. Resolve their shadows before alpha smoke/fire:
    // applying per-channel light removal later also recolors the blended effects.
    // The backend keeps reflection targets and already-resolved frames untouched.
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Effects);
#endif
    bSystemDelete = true;
    const float fDeltaTime = static_cast<float>(Delta_Time) * 0.001f;
    pManager->Execute(fDeltaTime);

    // If it's time, pause emission
    // when all particles die, the system will retire by itself ...
    for (auto &info : CreatedSystems)
    {
        info.PassedTime += Delta_Time;
        if (Delta_Time > 0 && info.Budgeted && !info.Fading && info.NavalActiveMs < 500 &&
            !info.pSystem->GetSystem()->IsEmissionPaused())
        {
            if (!info.NavalHasUpdated)
                info.NavalHasUpdated = true;
            else
            {
                const uint32_t remaining = 500 - info.NavalActiveMs;
                info.NavalActiveMs += Delta_Time < remaining ? Delta_Time : remaining;
            }
        }

        if (Delta_Time > 0 && info.Budgeted) info.NavalHasUpdated = true;

        if (info.LifeTime == 0)
            continue;

        if (info.PassedTime > info.LifeTime)
        {
            info.pSystem->Stop();
        }
    }

    // Removing dead systems ...
    for (auto it = CreatedSystems.begin(); it != CreatedSystems.end();)
    {
        if (!it->pSystem->GetSystem()->IsAlive())
        {
            auto dead = it++;
            RemoveSystem(dead, true);
        }
        else
            ++it;
    }
    if (Delta_Time > 0)
        DrainNavalQueues();
    bSystemDelete = false;
}

void PARTICLES::Execute(uint32_t Delta_Time)
{
}

void PARTICLES::PauseAllActive(bool bPaused)
{
    for (auto &info : CreatedSystems)
        info.pSystem->Pause(bPaused);
}

void PARTICLES::DeleteCaptured()
{
    for (auto &queue : NavalQueues)
        queue.Pending.erase(std::remove_if(queue.Pending.begin(), queue.Pending.end(),
                                           [](const PendingEffect &effect) { return effect.Captured; }),
                            queue.Pending.end());
    bSystemDelete = true;
    while (!CaptureBuffer.empty())
        RemoveSystem(CaptureBuffer.front()->Self, true);
    bSystemDelete = false;
}

void PARTICLES::DrainNavalQueues()
{
    constexpr size_t batch = 60;
    constexpr uint32_t categories[] = {1, 2, 10};
    for (size_t i = 0; i < NavalQueues.size(); ++i)
    {
        auto &queue = NavalQueues[i];
        size_t examined = 0;
        const size_t candidates = queue.Active.size() < batch ? queue.Active.size() : batch;
        while (examined < candidates && !queue.Active.empty() && !queue.Pending.empty() &&
               queue.Active.size() + queue.Pending.size() > queue.Soft)
        {
            auto current = queue.Next;
            auto *info = *current;
            queue.Next = std::next(current);
            if (queue.Next == queue.Active.end())
                queue.Next = queue.Active.begin();
            ++examined;
            if (info->NavalActiveMs < 500 || info->pSystem->GetSystem()->IsEmissionPaused())
                continue;
            if (queue.Active.size() == 1)
                queue.Next = queue.Active.end();
            info->pSystem->GetSystem()->BeginNavalFade(0.25f);
            queue.Fading.splice(queue.Fading.end(), queue.Active, info->NavalIt);
            info->Fading = true;
        }
        size_t spawned = 0;
        while (spawned < batch && !queue.Pending.empty() &&
               queue.Active.size() + queue.Fading.size() < queue.Hard)
        {
            CreateNavalSystem(queue, categories[i], queue.Pending.front());
            queue.Pending.pop_front();
            ++spawned;
        }
    }
}

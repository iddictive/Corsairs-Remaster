#pragma once

#include "entity.h"
#include "c_vector.h"
#include <array>
#include <deque>
#include <list>
#include <string>
#include <vector>

class IParticleService;
class IParticleManager;
class IParticleSystem;
class PARTICLE_SYSTEM;

class PARTICLES : public Entity
{
    bool CreationCapture;
    bool bSystemDelete;

    struct SystemInfo
    {
        std::string FileName;
        PARTICLE_SYSTEM *pSystem;
        uint32_t LifeTime;
        uint32_t PassedTime;
        uint32_t NavalEffect = 0;
        float NavalX = 0.0f;
        float NavalZ = 0.0f;
        bool Budgeted = false;
        bool Fading = false;
        bool Captured = false;
        bool NavalHasUpdated = false;
        uint32_t NavalActiveMs = 0;
        std::list<SystemInfo>::iterator Self;
        std::list<SystemInfo *>::iterator NavalIt;
        std::list<SystemInfo *>::iterator CaptureIt;

        SystemInfo()
        {
            PassedTime = 0;
            pSystem = nullptr;
            LifeTime = 0xFFFFFF00;
        }
    };

    struct PendingEffect
    {
        std::string Name;
        CVECTOR Pos, Angles;
        uint32_t LifeTime;
        bool Captured;
    };
    struct NavalQueue
    {
        std::list<SystemInfo *> Active, Fading;
        std::deque<PendingEffect> Pending;
        size_t Soft, Hard, PendingLimit;
        std::list<SystemInfo *>::iterator Next;
    };
    std::list<SystemInfo> CreatedSystems;
    std::array<NavalQueue, 3> NavalQueues{{{{}, {}, {}, 60, 120, 720},
                                           {{}, {}, {}, 360, 720, 360},
                                           {{}, {}, {}, 360, 720, 720}}};
    size_t HullEffects = 0;

    IParticleService *pService;
    IParticleManager *pManager;

    PARTICLE_SYSTEM *CreateSystem(const char *pFileName, uint32_t LifeTime, bool captured);
    PARTICLE_SYSTEM *CreateNavalSystem(NavalQueue &queue, uint32_t category, const PendingEffect &effect);
    static int NavalQueueIndex(uint32_t category);
    void DrainNavalQueues();
    void RemoveSystem(std::list<SystemInfo>::iterator it, bool destroy);
    void DeleteSystem(uintptr_t SystemID);
    void DeleteAll();

    void PauseAllActive(bool bPaused);

    std::list<SystemInfo *> CaptureBuffer;

    void DeleteCaptured();

  public:
    void DeleteResource(PARTICLE_SYSTEM *pResource);

  public:
    PARTICLES();
    ~PARTICLES() override;

    bool Init() override;

    uint64_t ProcessMessage(MESSAGE &message) override;

    void Realize(uint32_t Delta_Time);
    void Execute(uint32_t Delta_Time);

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            Execute(delta);
            break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
              LostRender(delta); break;
            case Stage::restore_render:
              RestoreRender(delta); break;*/
        }
    }
};

#pragma once

#include "matrix.h"
#include "cannon_trace.h"
#include "blast.h"
#include "dx9render.h"
#include "geometry.h"
#include "vma.hpp"
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include "ship_hull_chips.hpp"
#include "ship_hull_damage_bridge.hpp"
#endif

using GEOPARTICLE = struct
{
    CVECTOR pos;
    CVECTOR ang;
    CVECTOR ang_speed;
    CVECTOR dir;
    GEOS *geo;
    float speed;
    bool bDouble;
    bool bEffect;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    storm_ship_hull::Chip chip;
    float chipScale=1;
#endif
};

class BLAST : public Entity
{
    VDX9RENDER *rs;
    VGEOMETRY *gs;
    std::vector<GEOPARTICLE> Item;
    CANNON_TRACE_BASE *pSea;
    entid_t sea_eid;
    float AngleDeviation;
    uint32_t ItemsNum;
    CMatrix Center;
    entid_t Splash;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    bool hullChipMode=false;
    storm_ship_hull::ChipLease hullChipLease;
    void LaunchHullChips(MESSAGE&message);
    void StepHullChips(uint32_t delta);
#endif

  public:
    ~BLAST() override;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    explicit BLAST(bool hullChips=false);
#else
    BLAST();
#endif
    bool Init() override;
    uint64_t ProcessMessage(MESSAGE &message) override;
    uint32_t AttributeChanged(ATTRIBUTES *) override;
    void Realize(uint32_t Delta_Time);
    void ProcessTime(uint32_t Delta_Time);
    void SetBlastCenter(CVECTOR pos, CVECTOR ang);
    void AddGeometry(char *name, int32_t num);

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        case Stage::execute: if(hullChipMode)StepHullChips(delta);break;
#endif
            // case Stage::execute:
            //    Execute(delta); break;
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

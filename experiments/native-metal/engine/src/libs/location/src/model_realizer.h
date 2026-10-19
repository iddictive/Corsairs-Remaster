#pragma once

#include "dx9render.h"
#include "geometry.h"
#include "vma.hpp"

class Lights;

class LocModelRealizer : public Entity
{
    entid_t eid_model;
    Lights *lights;
    VDX9RENDER *rs;
    VGEOMETRY *gs;
    bool bShow;

    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    LocModelRealizer();
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    bool IsVisible() const { return bShow; }
    void SetGeometryEmitter(bool value) { geometryEmitter = value; }
  private:
    bool geometryEmitter = false;
  public:
#endif
    ~LocModelRealizer() override;

    // Initialization
    bool Init() override;
    // Execution
    void Execute(uint32_t delta_time);
    void Realize(uint32_t delta_time) const;

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

    // Messages
    uint64_t ProcessMessage(MESSAGE &message) override;
};

#pragma once

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalSetShipAuxiliaryDetail(void *, bool);
#endif

namespace storm::ship
{
// The backend gate affects caster admission only; ordinary geometry remains a
// receiver. Restore the surrounding scope even when a model returns early.
class AuxiliaryDetailScope
{
  public:
    AuxiliaryDetailScope(void *device, bool visible)
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        : device_(device), previous_(StormMetalSetShipAuxiliaryDetail(device, visible))
#endif
    {
    }
    ~AuxiliaryDetailScope()
    {
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        StormMetalSetShipAuxiliaryDetail(device_, previous_);
#endif
    }
    AuxiliaryDetailScope(const AuxiliaryDetailScope &) = delete;
    AuxiliaryDetailScope &operator=(const AuxiliaryDetailScope &) = delete;

  private:
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    void *device_;
    bool previous_;
#endif
};
}

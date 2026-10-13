#pragma once
#include <cstdint>
#include <cstddef>

// Ship-local metres; simulation owns intensity, persistent char and residual heat.
// Plain records deliberately match MSL packed_float3, rather than aligned float3.
struct StormMetalShipFirePoint {
    float position[3];
    float radius;
    float intensity;
    float charred;
    float heat;
    float seed;
};
static_assert(sizeof(StormMetalShipFirePoint)==32);
static_assert(offsetof(StormMetalShipFirePoint,intensity)==16);

#ifdef STORM_METAL_SHIP_FIRE
extern "C" void StormMetalBeginShipFireSurface(void* device,const float* world,
    const StormMetalShipFirePoint* points,uint32_t count);
extern "C" void StormMetalEndShipFireSurface(void* device);
extern "C" void StormMetalDrawShipFire(void* device,const float* world,
    const StormMetalShipFirePoint* points,uint32_t count,const float* wind,float seconds);
#endif

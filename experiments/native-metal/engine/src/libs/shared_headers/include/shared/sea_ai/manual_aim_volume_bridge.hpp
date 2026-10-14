#pragma once
#include <cstdint>

namespace storm::sea_ai::manual_aim
{
inline constexpr std::uint32_t maxVolumeSections=1024;
inline constexpr std::uint32_t maxVolumePlanesPerSlab=256;
inline constexpr std::uint32_t maxVolumePlanes=65536;
inline constexpr std::uint32_t maxVolumeRelationVertices=49152;
inline constexpr std::uint32_t maxWaterContactVertices=49152;
inline constexpr std::uint32_t contactSectionsPerField=1;
inline constexpr std::uint32_t maxContactFields=112; // All current GM batteries <=52; no truncation.
inline constexpr std::uint32_t maxContactSections=2; // Sea impacts and selected-range projection.
inline constexpr std::uint32_t maxContactTriangles=8192;
inline constexpr std::uint32_t maxRenderedReceivers=128;
inline constexpr std::uint32_t excludedReceiverColor=0x01000000u;

// Absolute world centers; the backend applies its current camera-relative
// world origin once. x/y planes are in the supplied lateral/up basis.
// Section i owns the exact side half-planes of slab i -> i+1. Last count is0.
// An internal zero planeCount explicitly marks an empty, zero-density slab.
struct alignas(16) AimVolumeSection
{
    float center[3];
    float progress;
    std::uint32_t firstPlane,planeCount;
    float halfWidth,halfHeight;
};
struct alignas(16) AimVolumePlane
{
    float x,y,offset0,offset1;
};
// Exact collision-mesh polygons identify a receiver only when they agree with
// the current scene depth. They never define contact coverage or draw visibly.
struct alignas(16) AimVolumeRelationVertex
{
    float position[3];
    std::uint32_t color;
};
// Absolute world XZ of actual first-water contacts. Triangles are unioned in
// a private chart; they are never drawn as alpha geometry in the scene.
struct AimWaterContactVertex {float x,z;};
// Approximate main dispersion density, not an extremal shot boundary.
// True impact-plane mean and world covariance across all eligible guns.
// padding selects the chart: 0=sea XZ, 1=selected-range lateral/up.
// reserved stores center-relative normal-depth bounds, independent of chart variance.
struct alignas(16) AimContactSection
{
    float center[3],padding;
    float covariance[6]; // xx,xy,xz,yy,yz,zz
    float reserved[2];
};
// A same-first-receiver support prism, used only as a coverage/occlusion gate.
struct alignas(16) AimContactTriangle
{
    float a[3],thickness;
    float b[3];std::uint32_t receiverToken;
    float c[3];std::uint32_t padding;
    float normal[3],reserved;
};
struct alignas(16) AimRenderedReceiver
{
    std::uint64_t modelId;
    std::uint32_t color,receiverToken;
};
static_assert(sizeof(AimContactSection)==48);
static_assert(sizeof(AimContactTriangle)==64);
static_assert(sizeof(AimRenderedReceiver)==16);
static_assert(sizeof(AimWaterContactVertex)==8);
static_assert(sizeof(AimVolumeRelationVertex)==16 && alignof(AimVolumeRelationVertex)==16);
static_assert(sizeof(AimVolumeSection)==32 && alignof(AimVolumeSection)==16);
static_assert(sizeof(AimVolumePlane)==16 && alignof(AimVolumePlane)==16);
}

extern "C" bool StormMetalDrawAimVolume(void *device,
    const storm::sea_ai::manual_aim::AimVolumeSection *sections,std::uint32_t sectionCount,
    const storm::sea_ai::manual_aim::AimVolumePlane *planes,std::uint32_t planeCount,
    const storm::sea_ai::manual_aim::AimVolumeRelationVertex *relationVertices,std::uint32_t relationVertexCount,
    const storm::sea_ai::manual_aim::AimWaterContactVertex *waterVertices,std::uint32_t waterVertexCount,
    const storm::sea_ai::manual_aim::AimContactSection *contactSections,std::uint32_t contactSectionCount,
    const storm::sea_ai::manual_aim::AimContactTriangle *contactTriangles,std::uint32_t contactTriangleCount,
    const float *axis,const float *lateral,const float *up,float readiness);
extern "C" void StormMetalSetAimEnabled(void *device,bool enabled);
extern "C" bool StormMetalBeginAimOwnShip(void *device);
extern "C" void StormMetalEndAimOwnShip(void *device);
extern "C" bool StormMetalBeginAimWater(void *device);
extern "C" void StormMetalEndAimWater(void *device);

extern "C" void StormMetalSetAimRenderedReceivers(void *device,
    const storm::sea_ai::manual_aim::AimRenderedReceiver *receivers,std::uint32_t count);
extern "C" bool StormMetalBeginAimModel(void *device,std::uint64_t modelId);
extern "C" void StormMetalEndAimModel(void *device);
extern "C" bool StormMetalBeginAimRigging(void *device,std::uint64_t modelId);

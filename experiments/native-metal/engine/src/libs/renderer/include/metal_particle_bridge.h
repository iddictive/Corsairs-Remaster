#pragma once

#include <cstddef>
#include <cstdint>

#ifdef STORM_METAL_PARTICLE_BRIDGE
struct StormMetalParticleSprite
{
    float center[3];
    float halfSize;
    float angle;
    uint32_t color;
    float uv[4];
};

struct StormMetalAdvancedParticle
{
    float center[3];
    float halfWidth;
    float halfHeight;
    float angle;
    uint32_t color;
    float uv0[4];
    float uv1[4];
    float frameBlend;
    float addPower;
};

enum StormMetalParticlePlane : uint32_t
{
    StormMetalParticleCameraXY = 0,
    StormMetalParticleWorldXZ = 1,
};

enum StormMetalParticleBlend : uint32_t
{
    // D3DBLEND_SRCALPHA / D3DBLEND_INVSRCALPHA.
    StormMetalParticleStraightAlpha = 0,
    // D3DBLEND_ONE / D3DBLEND_INVSRCALPHA.
    StormMetalParticlePremultipliedAlpha = 1,
};

extern "C" void StormMetalDrawParticleSprites(void *device, const StormMetalParticleSprite *sprites,
                                                size_t count, StormMetalParticlePlane plane,
                                                StormMetalParticleBlend blend);
extern "C" void StormMetalDrawAdvancedParticles(void *device, const StormMetalAdvancedParticle *particles,
                                                 size_t count, StormMetalParticleBlend blend);
#endif

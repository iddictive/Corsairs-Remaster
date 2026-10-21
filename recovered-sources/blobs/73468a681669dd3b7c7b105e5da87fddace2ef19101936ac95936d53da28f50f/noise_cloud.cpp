//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    NoiseCloud
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "noise_cloud.h"
#include "metal_particle_bridge.h"
#include <vector>

//============================================================================================

#define TRND_CLDRADIUS 2000.0f

// ============================================================================================
// Construction, destruction
// ============================================================================================

NoiseCloud::NoiseCloud(Pillar &_pillar) : pillar(_pillar)
{
    for (int32_t i = 0; i < sizeof(rect) / sizeof(Rect); i++)
    {
        rect[i].a = rand() * 2.0f * TRND_PI / RAND_MAX;
        rect[i].r = i * (TRND_CLDRADIUS / static_cast<float>(sizeof(rect) / sizeof(Rect) - 1));
        rect[i].angle = 0.0f;
        rect[i].size = TRND_CLDRADIUS * 0.4f;
        rect[i].alpha = 0.5f;
        rect[i].light = 0.5f;
        rect[i].kLight = -1.0f;
        rect[i].tLight = rand() * 10.0f / RAND_MAX;
    }
    texture = -1;
    lightCnt = 0;
    SetGlobalAlpha(1.0f);
}

NoiseCloud::~NoiseCloud()
{
    if (renderer)
    {
        if (vertexBuffer >= 0)
            renderer->ReleaseVertexBuffer(vertexBuffer);
        if (indexBuffer >= 0)
            renderer->ReleaseIndexBuffer(indexBuffer);
    }
}

void NoiseCloud::EnsureBuffers(VDX9RENDER *rs)
{
    if (vertexBuffer >= 0 && indexBuffer >= 0)
        return;
    renderer = rs;
    vertexBuffer = rs->CreateVertexBuffer(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                                           64 * 4 * sizeof(Vertex),
                                           D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC);
    indexBuffer = rs->CreateIndexBuffer(64 * 6 * sizeof(uint16_t));
    auto *indices = static_cast<uint16_t *>(rs->LockIndexBuffer(indexBuffer));
    for (int32_t i = 0; i < 64; ++i)
    {
        const auto base = static_cast<uint16_t>(i * 4);
        indices[i * 6 + 0] = base + 0;
        indices[i * 6 + 1] = base + 1;
        indices[i * 6 + 2] = base + 2;
        indices[i * 6 + 3] = base + 2;
        indices[i * 6 + 4] = base + 1;
        indices[i * 6 + 5] = base + 3;
    }
    rs->UnLockIndexBuffer(indexBuffer);
}

void NoiseCloud::Update(float dltTime)
{
    for (int32_t i = 0; i < sizeof(rect) / sizeof(Rect); i++)
    {
        if (rect[i].r >= TRND_CLDRADIUS)
            rect[i].r = 0.0f;
        const auto k = rect[i].r / TRND_CLDRADIUS;
        auto rot = dltTime * (2.0f - powf(k, 0.3f) * 2.2f);
        if (rot < 0.0f)
            rot = 0.0f;
        rect[i].a += rot;
        if (rect[i].a > 2.0f * TRND_PI)
            rect[i].a -= 2.0f * TRND_PI;
        rect[i].angle += rot;
        if (rect[i].angle > 2.0f * TRND_PI)
            rect[i].angle -= 2.0f * TRND_PI;
        rect[i].r += dltTime * TRND_CLDRADIUS * 0.1f;
        if (rect[i].r < TRND_CLDRADIUS * 0.2f)
            rect[i].alpha = rect[i].r / (TRND_CLDRADIUS * 0.2f);
        if (rect[i].r > TRND_CLDRADIUS * 0.6f)
            rect[i].alpha = 1.0f - (rect[i].r - TRND_CLDRADIUS * 0.6f) / (TRND_CLDRADIUS * 0.4f);
        if (rect[i].alpha < 0.0f)
            rect[i].alpha = 0.0f;
        if (rect[i].alpha > 1.0f)
            rect[i].alpha = 1.0f;
        if (rect[i].kLight < 0.0f)
        {
            rect[i].tLight += dltTime;
            if (lightCnt < 3 && rect[i].r < TRND_CLDRADIUS * 0.4f)
            {
                if (rect[i].tLight > 5.0f && (rand() & 127) == 7)
                {
                    rect[i].kLight = 0.0f;
                    rect[i].tLight = 0.0f;
                    lightCnt++;
                }
            }
        }
        else
        {
            rect[i].kLight += dltTime * 1.1f;
            if (rect[i].kLight < 1.0f)
            {
                rect[i].light = 0.2f + 0.8f * (1.0f - (rect[i].kLight - 0.5f) * (rect[i].kLight - 0.5f) * 4.0f);
            }
            else
            {
                rect[i].tLight = 0.0f;
                rect[i].kLight = -1.0f;
                rect[i].light = 0.5f;
                lightCnt--;
            }
        }
    }
}

void NoiseCloud::Draw(VDX9RENDER *rs)
{
#ifdef STORM_METAL_PARTICLE_BRIDGE
    std::vector<StormMetalParticleSprite> sprites;
    sprites.reserve(sizeof(rect) / sizeof(Rect));
    rs->TextureSet(0, texture);
    for (const auto &item : rect)
    {
        const auto colorValue = item.light * 200.0f;
        StormMetalParticleSprite sprite{};
        sprite.center[0] = pillar.X() + item.r * sinf(item.a);
        sprite.center[1] = pillar.GetHeight();
        sprite.center[2] = pillar.Z() + item.r * cosf(item.a);
        sprite.halfSize = item.size;
        sprite.angle = item.angle;
        sprite.color = (static_cast<int32_t>(colorValue) << 16) |
                       (static_cast<int32_t>(colorValue * 0.95f) << 8) |
                       static_cast<int32_t>(colorValue * 0.9f) |
                       (static_cast<int32_t>(item.alpha * galpha * 0.4f) << 24);
        sprite.uv[2] = 1.0f;
        sprite.uv[3] = 1.0f;
        sprites.push_back(sprite);
    }
    StormMetalDrawParticleSprites(rs->GetD3DDevice(), sprites.data(), sprites.size(), StormMetalParticleWorldXZ,
                                  StormMetalParticleStraightAlpha);
    return;
#endif
    EnsureBuffers(rs);
    auto *vertices = static_cast<Vertex *>(rs->LockVertexBuffer(vertexBuffer, D3DLOCK_DISCARD));
    rs->TextureSet(0, texture);
    for (int32_t i = 0; i < sizeof(rect) / sizeof(Rect); i++)
    {
        // The size
        const auto size = rect[i].size;
        // Position
        const auto sn = sinf(rect[i].angle);
        const auto cs = cosf(rect[i].angle);
        CVECTOR pos(pillar.X() + rect[i].r * sinf(rect[i].a), pillar.GetHeight(),
                    pillar.Z() + rect[i].r * cosf(rect[i].a));
        auto *quad = vertices + i * 4;
        // Colour
        const auto clr = rect[i].light * 200.0f;
        const auto color = (static_cast<int32_t>(clr * 1.0f) << 16) | (static_cast<int32_t>(clr * 0.95f) << 8) |
                           static_cast<int32_t>(clr * 0.9f) | (static_cast<int32_t>(rect[i].alpha * galpha * 0.4f) << 24);
        quad[0] = {pos + CVECTOR(size * (-cs + sn), 0.0f, size * (sn + cs)), static_cast<uint32_t>(color), 0.0f, 0.0f};
        quad[1] = {pos + CVECTOR(size * (-cs - sn), 0.0f, size * (sn - cs)), static_cast<uint32_t>(color), 0.0f, 1.0f};
        quad[2] = {pos + CVECTOR(size * (cs + sn), 0.0f, size * (-sn + cs)), static_cast<uint32_t>(color), 1.0f, 0.0f};
        quad[3] = {pos + CVECTOR(size * (cs - sn), 0.0f, size * (-sn - cs)), static_cast<uint32_t>(color), 1.0f, 1.0f};
    }
    rs->UnLockVertexBuffer(vertexBuffer);
    rs->DrawBuffer(vertexBuffer, sizeof(Vertex), indexBuffer, 0, 64 * 4, 0, 64 * 2, "TornadoClouds");
}

void NoiseCloud::SetGlobalAlpha(float a)
{
    if (a < 0.0f)
        a = 0.0f;
    if (a > 1.0f)
        a = 1.0f;
    galpha = a * 255.0f;
}

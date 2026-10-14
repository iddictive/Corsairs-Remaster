//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmSea
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "world_map_resources.hpp"

#include "wdm_render_object.h"

class WdmSea : public WdmRenderObject
{
#pragma pack(push, 1)

    struct Triangle
    {
        uint16_t index[3];
    };

    struct Vertex
    {
        float x, y, z;
        uint32_t color;
        float tu, tv;
    };

    struct WhiteHorses
    {
        float x;
        float z;
        float angle;
        float size;
        float speed;
        float curTime;
        float kTime;
        int32_t textureIndex;
    };

    struct Flare
    {
        int32_t index;  // index of a used rectangle
        float time;  // Lifetime 0 to 1
        float k;     // The speed of time
        float phase; // Phase defining the direction of rotation
    };

#pragma pack(pop)

    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    explicit WdmSea(bool incremental = false);
    bool LoadNext();
    ~WdmSea() override;

    void Update(float dltTime) override;

    void PRender(VDX9RENDER *rs) override;
    void LRender(VDX9RENDER *rs) override;
    // Same XYZ/DIFFUSE/TEX1 surface as the ordinary sea, with vertex coverage.
    void DrawTsunamiSurface(VDX9RENDER *rs, const void *vertices, uint32_t triangles, uint32_t stride) const;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    // Draw
    void Render(VDX9RENDER *rs, const char *tech = nullptr) const;
    // Adjust texture coordinate transformations
    void PresetMain(VDX9RENDER *rs) const;
    void PresetAnimated(VDX9RENDER *rs) const;

  private:
    // Geometry buffers
    int32_t vb, ib;
    // Waves
    WhiteHorses wh[16];
    // Textures
    int32_t baseTexture;
    int32_t loadTexture = 0;
    int32_t aniTextures[storm_map_resources::SeaAnimationTextureCount];
    float aniFrame;
    int32_t whiteHorses[1];
    float seaSizeX;
    float seaSizeZ;
    float aniTiling;
    // Flares
    Flare flare[1024];
    RS_RECT flareRect[1024];
    int32_t flareCount;
    int32_t flareTexture;
    float flareCerateCounter;
};

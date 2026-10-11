#pragma once
#include "core.h"
#include "weather_engine.hpp"
#include "weather_base.h"
#include "wdm_render_object.h"
#include "wdm_objects.h"
#include "metal_world_map_bridge.h"
#include <array>
#include <algorithm>
#include <cmath>

// WEATHER owns fronts and their calendar. This projects the same cells using
// authored cloud/rain art; the map retains camera, input and label ownership.
class WdmWeatherRegions final : public WdmRenderObject
{
    static constexpr unsigned puffs=18, drops=12;
    std::array<RS_RECT,storm_weather::Engine::frontCount*puffs> cloudRects{};
    std::array<RS_RECT,storm_weather::Engine::frontCount*drops> rainRects{};
    IDirect3DVertexDeclaration9 *declaration=nullptr;
    int32_t cloudTexture=-1, cloudLight=-1, stormLight=-1, rainTexture=-1, lightningTexture=-1;
    double animationSeconds=0;
    static float noise(uint32_t id,unsigned index)
    {
        uint32_t v=id^(index*0x9e3779b9u); v^=v>>16; v*=0x7feb352du;
        v^=v>>15; v*=0x846ca68bu; v^=v>>16;
        return float(v>>8)/16777216.f;
    }
    void drawClouds(VDX9RENDER *rs,unsigned count,int32_t light)
    {
        if (!count || light<0) return;
        rs->SetVertexDeclaration(declaration);
        rs->TextureSet(0,cloudTexture); rs->TextureSet(1,light);
        CMatrix view, constants;
        rs->GetTransform(D3DTS_VIEW,view); view.Transposition();
        rs->GetTransform(D3DTS_PROJECTION,constants);
        rs->SetVertexShaderConstantF(0,constants,4);
        constants.matrix[0]=view.matrix[1]; constants.matrix[1]=view.matrix[5];
        constants.matrix[2]=view.matrix[9]; constants.matrix[3]=view.matrix[13]-24.f;
        constants.matrix[4]=40.f*1.4f; constants.matrix[5]=0.f;
        constants.matrix[6]=200.f*1.4f; constants.matrix[7]=.5f;
        constants.matrix[8]=.8f; constants.matrix[9]=.6f;
        constants.matrix[10]=1.f; constants.matrix[11]=1.f;
        rs->SetVertexShaderConstantF(4,constants,3);
        rs->DrawRects(cloudRects.data(),count,"WdmClouds",2,2);
    }
public:
    void Update(float deltaSeconds) override
    {
        if (std::isfinite(deltaSeconds) && deltaSeconds>0)
            animationSeconds+=deltaSeconds;
    }
    WdmWeatherRegions()
    {
        auto *rs=wdmObjects->rs;
        cloudTexture=rs->TextureCreate("\\WorldMap\\clouds.tga");
        cloudLight=rs->TextureCreate("\\WorldMap\\cloudslight.tga");
        stormLight=rs->TextureCreate("\\WorldMap\\stormlight.tga");
        rainTexture=rs->TextureCreate("\\WorldMap\\rain.tga");
        lightningTexture=rs->TextureCreate("\\WorldMap\\lightnings.tga");
        const D3DVERTEXELEMENT9 elements[]={
            {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,12,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},
            {0,16,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        rs->CreateVertexDeclaration(elements,&declaration);
    }
    ~WdmWeatherRegions() override
    {
        auto *rs=wdmObjects->rs;
        for (int32_t texture:{cloudTexture,cloudLight,stormLight,rainTexture,lightningTexture})
            if (texture>=0) rs->TextureRelease(texture);
        if (declaration) declaration->Release();
    }
    void LRender(VDX9RENDER *rs) override
    {
        const auto id=core.GetEntityId("weather");
        if (!id || !rs || !wdmObjects || !declaration || cloudTexture<0) return;
        auto *weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
        if (!weather) return;
        CMatrix view; rs->GetTransform(D3DTS_VIEW,view);
        CVECTOR camera; view.MulToInv(CVECTOR(0.f),camera);
        // The native Tab overview rises to500; ship view starts at50.
        float zoom=std::clamp((camera.y-60.f)/360.f,0.f,1.f);
        zoom=zoom*zoom*(3.f-2.f*zoom);
        const float opacity=.12f+.83f*zoom;
        unsigned rainCount=0;
        for (unsigned wetBatch=0;wetBatch<2;++wetBatch) {
            unsigned cloudCount=0;
            for (unsigned slot=0;slot<storm_weather::Engine::frontCount;++slot) {
                storm_weather::Front front;
                if (!weather->GetWeatherFront(slot,front) || front.coverage<=.01f) continue;
                if (unsigned(front.rain>.03f || front.storm>.05f)!=wetBatch) continue;
                for (unsigned puff=0;puff<puffs;++puff) {
                    const float angle=noise(front.id,puff*4)*6.2831853f;
                    const float radial=std::sqrt(noise(front.id,puff*4+1))*.70f;
                    auto &r=cloudRects[cloudCount++];
                    r.vPos=CVECTOR(front.x+std::sin(angle)*radial*front.radius,24.f,
                                  front.z+std::cos(angle)*radial*front.radius);
                    r.fSize=front.radius*(.26f+.10f*noise(front.id,puff*4+2));
                    r.fAngle=noise(front.id,puff*4+3)*6.2831853f;
                    r.dwSubTexture=puff%4;
                    const uint32_t alpha=uint32_t(255.f*opacity*front.coverage*(1.f-radial*radial));
                    const uint32_t shade=uint32_t(245.f-65.f*front.rain-25.f*front.storm);
                    const uint32_t size=uint32_t(255.f*std::clamp((r.fSize-40.f)/200.f,0.f,1.f));
                    r.dwColor=(alpha<<24)|(shade<<16)|size;
                }
                if (wetBatch && rainTexture>=0) for (unsigned drop=0;drop<drops;++drop) {
                    const float angle=noise(front.id,100+drop*3)*6.2831853f;
                    const float radial=std::sqrt(noise(front.id,101+drop*3))*.48f;
                    auto &r=rainRects[rainCount++];
                    const float phase=float(std::fmod(animationSeconds,.8)/.8)+noise(front.id,102+drop*3);
                    r.vPos=CVECTOR(front.x+std::sin(angle)*radial*front.radius,
                                  3.f+18.f*(1.f-(phase-std::floor(phase))),
                                  front.z+std::cos(angle)*radial*front.radius);
                    r.fSize=front.radius*.16f; r.fAngle=0.f; r.dwSubTexture=0;
                    r.dwColor=(uint32_t(130.f*opacity*front.rain)<<24)|0x00c8d8e8u;
                }
            }
            drawClouds(rs,cloudCount,wetBatch?stormLight:cloudLight);
        }
        if (rainCount) {
            rs->TextureSet(0,rainTexture); rs->TextureSet(1,-1);
            rs->DrawRects(rainRects.data(),rainCount,"WdmRain",1,1);
        }
        if (lightningTexture>=0) for (unsigned slot=0;slot<storm_weather::Engine::frontCount;++slot) {
            storm_weather::Front front;
            if (!weather->GetWeatherFront(slot,front) || front.storm<.5f) continue;
            const float phase=float(std::fmod(animationSeconds+noise(front.id,190)*11.,11.));
            if (phase>.18f) continue;
            rs->TextureSet(0,lightningTexture); rs->TextureSet(1,-1);
            const uint32_t color=(uint32_t(200.f*opacity*front.storm)<<24)|0x00e0e8ffu;
            CMatrix cameraView; rs->GetTransform(D3DTS_VIEW,cameraView); cameraView.Transposition();
            const WdmMetalQuad3D bolt{{front.x,12.f,front.z},cameraView.Vx(),
                {0.f,-1.f,0.f},6.f,12.f,color,{float(front.id%4)*.25f,.01f,.25f,.98f}};
            WdmMetalDrawQuad3D(rs,bolt,"WdmLightning");
        }
    }
};

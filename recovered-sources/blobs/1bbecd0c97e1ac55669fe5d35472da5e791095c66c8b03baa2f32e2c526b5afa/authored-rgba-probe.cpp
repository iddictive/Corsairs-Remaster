#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <initializer_list>
extern "C" void StormMetalSetAuthoredSky(void*,float,bool,float);

extern "C" bool StormMetalBeginDynamicSky(void *, float, float, float, float, float, float, float, float, float, float, uint32_t);
extern "C" bool StormMetalPrepareDynamicSky(void *, float, float, float, float, float, float, float, float, float, float, uint32_t);
extern "C" void StormMetalEndDynamicSky(void *);
extern "C" bool StormMetalDynamicSkyActive(void *);
extern "C" bool StormMetalBeginSkyFog(void *, uint32_t);
extern "C" void StormMetalEndSkyFog(void *);
extern "C" bool StormMetalSkyFogActive(void *);
extern "C" bool StormMetalBeginSolarVisual(void *);
extern "C" void StormMetalEndSolarVisual(void *);
extern "C" bool StormMetalSolarVisualActive(void *);
extern "C" bool StormMetalCloudFieldCurrent(void *);

namespace
{
void need(bool value, const char *message)
{
    if (!value)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct SkyVertex
{
    float position[3];
    uint32_t color;
    float uv0[2];
    float uv1[2];
};
}

int main()
{
    setenv("STORM_METAL_DYNAMIC_SKY", "0", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Dynamic sky bridge", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());

    storm_render::Presentation parameters{};

    parameters.width = parameters.height = 32;
    parameters.colorFormat = storm_render::AssetFormat::A8R8G8B8;


    parameters.depthEnabled = true;
    parameters.depthFormat = storm_render::AssetFormat::D32;

    storm_render::Device *device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &parameters, &device)), "device");


    storm_render::Surface *target=nullptr,*readback=nullptr;
    device->GetRenderTarget(&target);device->CreateOffscreenPlainSurface(32,32,storm_render::AssetFormat::A8R8G8B8,&readback);
    storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;
    for(auto which:{storm_render::TransformWorld,storm_render::TransformView,storm_render::TransformProjection})device->SetTransform(which,&identity);
    device->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex2|storm_render::vertexFormat::texCoord2);
    device->SetRenderState(storm_render::RenderLighting,false);device->SetRenderState(storm_render::RenderDepthTest,false);device->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);
    device->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpBlendFactorAlpha);
    storm_render::Texture*tex[4]{};
    uint32_t colors[]={0x00204060,0x806080a0,0x40081018,0xc0183048};
    for(unsigned i=0;i<4;++i){need(storm_render::succeeded(device->CreateTexture(1,1,1,0,storm_render::AssetFormat::A8R8G8B8,&tex[i])),"texture");storm_render::LockedRect l{};tex[i]->LockRect(0,&l,nullptr,0);std::memcpy(l.pBits,&colors[i],4);tex[i]->UnlockRect(0);device->SetTexture(i,tex[i]);}
    const SkyVertex triangle[]={{{-1,-1,.5f},0xffffffff,{.5,.5},{.5,.5}},{{3,-1,.5f},0xffffffff,{.5,.5},{.5,.5}},{{-1,3,.5f},0xffffffff,{.5,.5},{.5,.5}}};
    for(bool alpha:{false,true})for(float hourly:{0.f,.5f,1.f})for(float storm:{0.f,.5f,1.f}){
        device->Present();device->Clear(storm_render::clear::color|storm_render::clear::depth,0xff000000,1);
        device->SetRenderState(storm_render::RenderBlend,alpha);device->SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendInverseSourceAlpha);device->SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendSourceAlpha);
        unsigned h=unsigned(hourly*255.f);device->SetRenderState(storm_render::RenderTextureFactor,h<<24);
        StormMetalSetAuthoredSky(device,.5f,true,storm);
        need(StormMetalBeginDynamicSky(device,12,0,.95,.9,0,1,0,0,1,0,0),"authored scope when atmosphere off");
        device->BeginScene();device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,1,triangle,sizeof(SkyVertex));StormMetalEndDynamicSky(device);device->EndScene();
        device->GetRenderTargetData(target,readback);storm_render::LockedRect lock{};readback->LockRect(&lock,nullptr,storm_render::lock::readOnly);uint32_t actual=0;std::memcpy(&actual,(char*)lock.pBits+16*lock.Pitch+16*4,4);readback->UnlockRect();
        float channels[4]{};
        for(unsigned c=0;c<4;++c){unsigned shift=c*8;float clear=((colors[0]>>shift)&255)*(1-h/255.f)+((colors[1]>>shift)&255)*(h/255.f);float cloudy=(((colors[2]>>shift)&255)+((colors[3]>>shift)&255))*.5f;channels[c]=clear*(1-storm)+cloudy*storm;}
        for(unsigned c=0;c<3;++c){float expected=channels[c]*(alpha?1-channels[3]/255.f:1.f);int measured=(actual>>(c*8))&255;need(std::abs(measured-expected)<=2.f,"authored current/next, storm blend and astronomy inverse-alpha oracle");}
        std::printf("RGBA alpha=%u hour=%.1f storm=%.1f pixel=%08x\n",alpha,hourly,storm,actual);
    }
    for(auto*t:tex)t->Release();readback->Release();target->Release();device->Release();SDL_DestroyWindow(window);SDL_Quit();std::puts("PASS original/storm current-next RGBA and astronomy alpha, atmosphere OFF");
}

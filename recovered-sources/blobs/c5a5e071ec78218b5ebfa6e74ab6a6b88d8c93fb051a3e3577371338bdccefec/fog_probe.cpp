#include <SDL.h>
#include "render_device_api.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void need(bool v,const char* s) { if(!v){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);} }
struct V { float x,y,z; uint32_t color; };
int main() {
    setenv("STORM_METAL_DYNAMIC_SKY","0",1);
    need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("Distance fog transport",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);
    need(window,"hidden window");
    storm_render::Presentation pp{};pp.width=pp.height=32;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device* d=nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window,&pp,&d)),"device");
    storm_render::Surface *target=nullptr,*readback=nullptr;
    d->GetRenderTarget(&target);
    d->CreateOffscreenPlainSurface(32,32,storm_render::AssetFormat::A8R8G8B8,&readback);
    d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse);
    d->SetRenderState(storm_render::RenderDepthTest,false);
    d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);
    d->SetRenderState(storm_render::RenderLighting,false);
    d->SetRenderState(storm_render::RenderFogColor,0xffa0b0c0);
    d->SetRenderState(storm_render::RenderFogTableMode,storm_render::FogExponential);
    d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);
    d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);
    const auto draw=[&](float depth,float density,bool enabled) {
        storm_render::Matrix p{};p._11=p._22=p._33=p._44=depth;
        d->SetTransform(storm_render::TransformProjection,&p);
        uint32_t bits;std::memcpy(&bits,&density,4);
        d->SetRenderState(storm_render::RenderFogDensity,bits);
        d->SetRenderState(storm_render::RenderFog,enabled);
        const V q[]={{-.8f,-.8f,.5f,0xff203040},{.8f,-.8f,.5f,0xff203040},{0,.8f,.5f,0xff203040}};
        d->Clear(storm_render::clear::color,0,1.f);d->BeginScene();
        need(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,1,q,sizeof(V))),"draw");
        d->EndScene();need(storm_render::succeeded(d->GetRenderTargetData(target,readback)),"readback");
        storm_render::LockedRect lock{};readback->LockRect(&lock,nullptr,storm_render::lock::readOnly);
        uint32_t pixel;std::memcpy(&pixel,(const char*)lock.pBits+16*lock.Pitch+16*4,4);readback->UnlockRect();
        return pixel;
    };
    for(float depth:{30.f,1000.f,3000.f,6000.f}) {
        for(float density:{.000005f,.00007f,.00011f}) {
            uint32_t pixel=draw(depth,density,true);float t=std::exp(-density*depth);
            for(unsigned shift:{0u,8u,16u}) {
                int a=int(0xff203040u>>shift&255),b=int(0xffa0b0c0u>>shift&255);
                need(std::abs(int(pixel>>shift&255)-int(std::lround(b+(a-b)*t)))<=1,"neutral scalar transport matches independent Beer-Lambert oracle");
            }
            std::printf("depth=%.0f density=%.8f transmission=%.6f pixel=%08x\n",depth,density,t,pixel);
        }
        need(draw(depth,.00007f,false)==0xff203040,"fog disabled preserves material exactly");
    }
    readback->Release();target->Release();d->Release();SDL_DestroyWindow(window);SDL_Quit();
    std::puts("PASS actual Metal FFP near/far/rain transport and fog-disabled negative; no game-scene verdict");
}

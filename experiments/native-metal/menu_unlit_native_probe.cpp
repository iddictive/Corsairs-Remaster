#include <SDL.h>
#include "render_device_api.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>

extern "C" bool StormMetalDrawCompactPrimitive(void *, uint32_t, uint32_t, uint32_t, const void *, uint32_t);
extern "C" bool StormMetalDrawCompactIndexedPrimitive(void *, uint32_t, uint32_t, uint32_t, uint32_t, const void *, uint32_t, const void *, uint32_t);
extern "C" uint64_t StormMetalNativeUnlitDraws(void *);
extern "C" uint64_t StormMetalNativeUnlitVertices(void *);
extern "C" uint64_t StormMetalLegacyUnlitCpuDraws(void *);
extern "C" uint64_t StormMetalLegacyUnlitCpuVertices(void *);

static void need(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Menu unlit native fixture", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());

    storm_render::Presentation pp{};
    pp.width = pp.height = 64;

    pp.colorFormat = storm_render::AssetFormat::A8R8G8B8;



    storm_render::Device *device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "device");

    struct Vertex { float x, y, z, rhw; uint32_t color; float u, v; };
    const uint32_t fvf = storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1;
    Vertex up[] = {{2, 2, .5f, 1, 0xffff2020, 0, 0}, {30, 2, .5f, 1, 0xffff2020, 1, 0},
                   {2, 62, .5f, 1, 0xffff2020, 0, 1}};
    Vertex dynamic[] = {{34, 2, .5f, 1, 0xff20ff20, 0, 0}, {62, 2, .5f, 1, 0xff20ff20, 1, 0},
                        {62, 62, .5f, 1, 0xff20ff20, 1, 1}};
    storm_render::VertexBuffer *vb = nullptr;
    need(storm_render::succeeded(device->CreateVertexBuffer(sizeof(dynamic), storm_render::usage::dynamic | storm_render::usage::writeOnly, fvf, &vb)), "dynamic menu VB");
    void *mapped = nullptr;
    need(storm_render::succeeded(vb->Lock(0, 0, &mapped, storm_render::lock::discard)), "dynamic menu VB lock");
    std::memcpy(mapped, dynamic, sizeof(dynamic));
    need(storm_render::succeeded(vb->Unlock()), "dynamic menu VB unlock");

    device->SetFVF(fvf);
    device->SetRenderState(storm_render::RenderLighting, false);
    device->SetRenderState(storm_render::RenderDepthTest, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    device->SetTextureStageState(0, storm_render::StageAlphaOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageAlphaArg1, storm_render::textureArg::diffuse);
    device->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    device->Clear(storm_render::clear::color, 0xff101010, 1);
    device->BeginScene();
    need(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, 1, up, sizeof(Vertex))), "menu UP draw");
    device->SetStreamSource(0, vb, 0, sizeof(Vertex));
    need(storm_render::succeeded(device->DrawPrimitive(storm_render::PrimitiveTriangleList, 0, 1)), "menu dynamic VB draw");
    device->EndScene();

    need(StormMetalNativeUnlitDraws(device) == 2, "both menu draws use native compact submission");
    need(StormMetalNativeUnlitVertices(device) == 6, "native compact vertex accounting is exact");
    need(StormMetalLegacyUnlitCpuDraws(device) == 0 && StormMetalLegacyUnlitCpuVertices(device) == 0,
         "menu has zero CPU FFP compatibility conversion");

    storm_render::Surface *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(&target);
    device->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback);
    need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "readback");
    storm_render::LockedRect lock{};
    need(storm_render::succeeded(readback->LockRect(&lock, nullptr, storm_render::lock::readOnly)), "readback lock");
    uint32_t left = 0, right = 0;
    std::memcpy(&left, static_cast<char *>(lock.pBits) + 16 * lock.Pitch + 8 * 4, 4);
    std::memcpy(&right, static_cast<char *>(lock.pBits) + 16 * lock.Pitch + 54 * 4, 4);
    readback->UnlockRect();
    need(left == 0xffff2020, "UP authored color preserved");
    need(right == 0xff20ff20, "dynamic VB authored color preserved");

    // Compare complete images against a triangle-list oracle, including sparse
    // indexed fans and a negative base vertex. No CPU vertex expansion is allowed.
    Vertex fan[] = {{2,2,.5f,1,0xffff2020,0,0}, {62,2,.5f,1,0xff20ff20,1,0},
                    {62,62,.5f,1,0xff2020ff,1,1}, {2,62,.5f,1,0xffffffff,0,1}};
    Vertex triangles[] = {fan[0],fan[1],fan[2],fan[0],fan[2],fan[3]};
    auto pixels = [&] {
        need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "fan readback");
        storm_render::LockedRect r{};
        need(storm_render::succeeded(readback->LockRect(&r,nullptr,storm_render::lock::readOnly)), "fan lock");
        std::vector<uint32_t> result(64*64);
        for(int y=0;y<64;y++) std::memcpy(result.data()+y*64,static_cast<char*>(r.pBits)+y*r.Pitch,64*4);
        readback->UnlockRect(); return result;
    };
    auto begin = [&] { device->Clear(storm_render::clear::color, 0xff101010, 1); device->BeginScene(); };
    begin();
    need(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,triangles,sizeof(Vertex))), "fan oracle");
    device->EndScene(); const auto expected=pixels();
    const auto cpuBefore=StormMetalLegacyUnlitCpuDraws(device);
    const uint16_t compactIndices[]={0,1,2,3};
    for(int route=0;route<6;route++) {
        storm_render::VertexBuffer *fanVB=nullptr; storm_render::IndexBuffer *fanIB=nullptr;
        if(route>=3) {
            const uint32_t usage=route==4?storm_render::usage::dynamic:0;
            need(storm_render::succeeded(device->CreateVertexBuffer(sizeof(fan), usage, fvf, &fanVB)), "fan VB");
            need(storm_render::succeeded(fanVB->Lock(0,0,&mapped,usage?storm_render::lock::discard:0)), "fan VB lock");
            std::memcpy(mapped,fan,sizeof(fan)); fanVB->Unlock();
            device->SetStreamSource(0,fanVB,0,sizeof(Vertex));
            if(route==5) {
                const uint32_t ix[]={99,2,3,4,5};
                need(storm_render::succeeded(device->CreateIndexBuffer(sizeof(ix), 0, storm_render::IndexFormat::UInt32, &fanIB)), "fan IB");
                need(storm_render::succeeded(fanIB->Lock(0,0,&mapped,0)), "fan IB lock");
                std::memcpy(mapped,ix,sizeof(ix)); fanIB->Unlock(); device->SetIndices(fanIB);
            }
        }
        begin();
        bool ok=false;
        if(route==0) ok=storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleFan,2,fan,sizeof(Vertex)));
        if(route==1) ok=StormMetalDrawCompactPrimitive(device,storm_render::PrimitiveTriangleFan,fvf,2,fan,sizeof(Vertex));
        if(route==2) ok=StormMetalDrawCompactIndexedPrimitive(device,storm_render::PrimitiveTriangleFan,0,4,2,compactIndices,uint32_t(storm_render::IndexFormat::UInt16),fan,sizeof(Vertex));
        if(route==3||route==4) ok=storm_render::succeeded(device->DrawPrimitive(storm_render::PrimitiveTriangleFan,0,2));
        if(route==5) ok=storm_render::succeeded(device->DrawIndexedPrimitive(storm_render::PrimitiveTriangleFan,-2,2,4,1,2));
        need(ok,"fan submission"); device->EndScene();
        need(pixels()==expected,"fan pixels match triangle-list oracle");
        need(StormMetalLegacyUnlitCpuDraws(device)==cpuBefore,"fan avoids CPU vertex expansion");
        device->SetStreamSource(0,nullptr,0,0); device->SetIndices(nullptr);
        if(fanVB) fanVB->Release(); if(fanIB) fanIB->Release();
    }
    need(!StormMetalDrawCompactPrimitive(device,storm_render::PrimitiveTriangleFan,fvf,2,fan,4),"short fan stride rejected");

    target->Release();
    readback->Release();
    vb->Release();
    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: menu and six fan submission routes preserve pixels with zero CPU vertex expansion");
}

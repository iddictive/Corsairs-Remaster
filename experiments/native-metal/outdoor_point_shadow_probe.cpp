#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>

extern "C" void StormMetalLandLocation(void*, bool);
extern "C" void StormMetalLandInterior(void*, bool);
extern "C" bool StormMetalBeginShadowFrame(void*, uint64_t, uint64_t, const float*);
extern "C" bool StormMetalBeginShadowPass(void*, unsigned, unsigned, uint64_t, const float*, float, float*, float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" bool StormMetalOutdoorSunShadowsReady(void*);
extern "C" bool StormMetalOutdoorPointShadowReady(void*);
extern "C" bool StormMetalRenderPointShadowCube(void*, uint64_t, const float*, float);
extern "C" bool StormMetalSetPointShadowWeight(void*, uint64_t, float);
extern "C" unsigned StormMetalBeginLandModel(void*, unsigned);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" uint64_t StormMetalPointShadowCpuFaceTraversals(void*);
extern "C" uint64_t StormMetalPointShadowCpuDraws(void*);
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*);
extern "C" uint64_t StormMetalPointShadowIndirectFaceSubmissions(void*);

static void need(bool value, const char* label) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}

int main() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto* window = SDL_CreateWindow("Outdoor point shadow", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, "window");

    storm_render::Presentation pp{};
     pp.width = pp.height = 64;
    pp.colorFormat = storm_render::AssetFormat::A8R8G8B8;

    storm_render::Device* device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "device");

    StormMetalLandLocation(device, true);
    StormMetalLandInterior(device, false);
    const float focus[] = {0, 0, 8};
    struct Vertex {float x,y,z,nx,ny,nz;unsigned color;float u,v;};
    Vertex vertices[]={{-1,-1,6,0,0,-1,0xffffffff,0,0},{1,-1,6,0,0,-1,0xffffffff,1,0},{1,1,6,0,0,-1,0xffffffff,1,1},{-1,1,6,0,0,-1,0xffffffff,0,1}};uint16_t indices[]={0,1,2,0,2,3};
    storm_render::VertexBuffer* vb=nullptr;storm_render::IndexBuffer* ib=nullptr;need(storm_render::succeeded(device->CreateVertexBuffer(sizeof(vertices), 0, storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1, &vb)),"VB");need(storm_render::succeeded(device->CreateIndexBuffer(sizeof(indices), 0, storm_render::IndexFormat::UInt16, &ib)),"IB");void*memory=nullptr;vb->Lock(0,0,&memory,0);std::memcpy(memory,vertices,sizeof(vertices));vb->Unlock();ib->Lock(0,0,&memory,0);std::memcpy(memory,indices,sizeof(indices));ib->Unlock();
    need(StormMetalBeginShadowFrame(device, 7, 1, focus), "frame");
    auto pass = [&](unsigned kind, unsigned face, uint64_t id, const float* source, float range) {
        storm_render::Matrix view{}, projection{};
        need(StormMetalBeginShadowPass(device, kind, face, id, source, range,
                                       &view.m[0][0], &projection.m[0][0]), "pass");
        StormMetalEndShadowPass(device);
    };
    const float lamp[] = {2, 3, 4};
    const float sun[] = {.6f, .1f, 1};
    pass(0, 0, 1, sun, 32); pass(0, 1, 1, sun, 96);
    need(StormMetalEndShadowFrame(device), "registry seed frame");
    storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;device->SetTransform(storm_render::TransformWorld,&identity);device->SetTransform(storm_render::TransformView,&identity);device->SetTransform(storm_render::TransformProjection,&identity);device->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);device->SetStreamSource(0,vb,0,sizeof(Vertex));device->SetIndices(ib);storm_render::Light directional{};directional.Type=storm_render::LightKind::Directional;directional.Direction={.6f,.1f,1};directional.Diffuse={1,1,1,1};device->SetLight(0,&directional);device->LightEnable(0,true);unsigned scope=StormMetalBeginLandModel(device,1);need(storm_render::succeeded(device->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"registry caster draw");StormMetalEndLandModel(device,scope);device->Present();
    need(StormMetalBeginShadowFrame(device,7,2,focus),"GPU point frame");need(StormMetalRenderPointShadowCube(device,17,lamp,24),"single GPU cube submission");need(StormMetalSetPointShadowWeight(device,17,.5f),"current point weight");need(!StormMetalSetPointShadowWeight(device,18,.5f),"unknown point weight rejected");need(!StormMetalSetPointShadowWeight(device,17,std::numeric_limits<float>::quiet_NaN()),"nonfinite point weight rejected");pass(0,0,1,sun,32);pass(0,1,1,sun,96);need(StormMetalEndShadowFrame(device), "complete frame");
    need(StormMetalOutdoorPointShadowReady(device), "one complete outdoor lamp cube");
    need(StormMetalOutdoorSunShadowsReady(device), "outdoor sun cascades preserved");
    need(StormMetalPointShadowCpuFaceTraversals(device)==0,"zero CPU face traversal");need(StormMetalPointShadowCpuDraws(device)==0,"zero CPU point draws");need(StormMetalPointShadowGpuCullDispatches(device)==1,"one GPU cull dispatch");need(StormMetalPointShadowIndirectFaceSubmissions(device)==6,"six indirect face submissions");

    vb->Release();ib->Release();device->Release();  SDL_DestroyWindow(window); SDL_Quit();
    std::puts("PASS GPU-driven outdoor point cube and independent sun cascades");
}

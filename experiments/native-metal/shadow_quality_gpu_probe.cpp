#include <SDL.h>
#include "render_device_api.hpp"

#include <cstdio>
#include <cstdlib>

extern "C" void StormMetalLandLocation(void*, bool);
extern "C" bool StormMetalBeginShadowFrame(void*, uint64_t, uint64_t, const float*);
extern "C" bool StormMetalBeginShadowPass(void*, unsigned, unsigned, uint64_t, const float*, float, float*, float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" unsigned StormMetalSunShadowResolution(void*);
extern "C" unsigned StormMetalPointShadowResolution(void*);
extern "C" bool StormMetalCanApplyGraphicsSettings(void*, uint32_t, uint32_t);
extern "C" bool StormMetalApplyGraphicsSettings(void*, uint32_t, uint32_t);
extern "C" bool StormMetalBeginDynamicSky(void*, float, float, float, float, float, float, float, float, float, float, uint32_t);
extern "C" void StormMetalEndDynamicSky(void*);
extern "C" void StormMetalSetBakedStaticEnvironment(void*, bool);
extern "C" bool StormMetalBakedStaticEnvironment(void*);

static void need(bool value, const char* label) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", label); std::exit(1); }
}

static void seedMaps(storm_render::Device* device, uint64_t scene, unsigned sunExpected, unsigned pointExpected) {
    const float focus[]={0,0,8}, sun[]={.6f,0,1}, point[]={0,2,0};
    float view[16]{}, projection[16]{};
    need(StormMetalBeginShadowFrame(device,scene,1,focus), "shadow frame retains active location");
    need(StormMetalBeginShadowPass(device,0,0,1,sun,64,view,projection), "sun pass");
    need(StormMetalSunShadowResolution(device)==sunExpected, "sun map allocation");
    StormMetalEndShadowPass(device);
    need(StormMetalBeginShadowPass(device,1,0,2,point,16,view,projection), "point pass");
    need(StormMetalPointShadowResolution(device)==pointExpected, "point map allocation");
    StormMetalEndShadowPass(device);
}

static void verify(const char* tier, unsigned sunExpected, unsigned pointExpected) {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    setenv("STORM_METAL_SHADOW_QUALITY", tier, 1);
    auto* window = SDL_CreateWindow("shadow quality", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window, "window");

    storm_render::Presentation pp{};  pp.width=pp.height=32;
    pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device* device=nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "device");
    StormMetalLandLocation(device,true);
    seedMaps(device,100+unsigned(tier[0]),sunExpected,pointExpected);
    device->Present();
    device->Release();  SDL_DestroyWindow(window);
}

static void verifyLiveApply() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    setenv("STORM_METAL_SHADOW_QUALITY", "0", 1);
    auto* window = SDL_CreateWindow("live shadow quality", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window, "live window");

    storm_render::Presentation pp{};  pp.width=pp.height=32;
    pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device* device=nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "live device");
    StormMetalLandLocation(device,true);
    need(!StormMetalCanApplyGraphicsSettings(device, 1u << 7u, 0) &&
         !StormMetalApplyGraphicsSettings(device, 1u << 7u, 0), "unknown settings flag rejects without mutation");
    need(!StormMetalCanApplyGraphicsSettings(device, 1u, 3) &&
         !StormMetalApplyGraphicsSettings(device, 1u, 3), "invalid shadow tier rejects without mutation");

    need(storm_render::succeeded(device->Clear(storm_render::clear::color | storm_render::clear::depth, 0, 1.f)),
         "active encoder fixture");
    need(StormMetalCanApplyGraphicsSettings(device, 1u, 0),
         "valid transaction admitted with an active frame encoder");
    need(StormMetalApplyGraphicsSettings(device, 1u, 0),
         "valid transaction synchronizes the active frame encoder");

    seedMaps(device,200,1024,256);
    StormMetalSetBakedStaticEnvironment(device,true);
    need(StormMetalBakedStaticEnvironment(device), "baked location flag survives receiver phase");
    constexpr uint32_t dynamicLighting = 1u << 0u;
    constexpr uint32_t dynamicSky = 1u << 5u;
    need(StormMetalApplyGraphicsSettings(device,dynamicLighting | dynamicSky,0), "dynamic-sky flag transaction");
    need(StormMetalBeginDynamicSky(device,18.f,1.f/60.f,.7f,.8f,.4f,9.f,.16f,0.f,1.f,0.f,0xff315273u),
         "dynamic-sky flag enables its existing backend owner");
    StormMetalEndDynamicSky(device);
    need(StormMetalApplyGraphicsSettings(device,dynamicLighting,0), "dynamic-sky flag disable transaction");
    need(!StormMetalBeginDynamicSky(device,18.f,1.f/60.f,.7f,.8f,.4f,9.f,.16f,0.f,1.f,0.f,0xff315273u),
         "dynamic-sky flag disables its existing backend owner");
    need(StormMetalApplyGraphicsSettings(device,dynamicLighting,1), "quality transaction");
    need(StormMetalSunShadowResolution(device)==0 && StormMetalPointShadowResolution(device)==0,
         "quality transaction discards seeded maps");
    need(!StormMetalBakedStaticEnvironment(device), "quality reconstruction discards prior baked location flag");
    seedMaps(device,201,2048,512);
    StormMetalSetBakedStaticEnvironment(device,true);
    seedMaps(device,202,2048,512);
    need(!StormMetalBakedStaticEnvironment(device), "new frame clears the previous location's baked flag");
    need(StormMetalApplyGraphicsSettings(device,dynamicLighting,1), "same settings noop");
    need(StormMetalSunShadowResolution(device)==2048 && StormMetalPointShadowResolution(device)==512,
         "same settings retains rebuilt maps");

    need(StormMetalApplyGraphicsSettings(device,0,1), "lighting off");
    const float focus[]={0,0,8};
    need(!StormMetalBeginShadowFrame(device,202,1,focus), "off disables shadow frame admission");
    need(StormMetalApplyGraphicsSettings(device,dynamicLighting,2), "lighting on at high quality");
    seedMaps(device,203,4096,512);
    need(storm_render::succeeded(device->Reset(&pp)), "backbuffer reset");
    seedMaps(device,204,4096,512);

    device->Present();
    device->Release();  SDL_DestroyWindow(window);
}

int main() {
    need(SDL_Init(SDL_INIT_VIDEO)==0, "SDL");
    verify("0",1024,256);
    verify("1",2048,512);
    verify("2",4096,512);
    verifyLiveApply();
    SDL_Quit();
    std::puts("PASS shadow quality profiles and live settings transaction preserve safe map lifecycle");
}

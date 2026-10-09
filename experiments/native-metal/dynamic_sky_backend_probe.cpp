#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>

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
    setenv("STORM_METAL_DYNAMIC_SKY", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Dynamic sky bridge", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());
    auto *api = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS parameters{};
    parameters.Windowed = TRUE;
    parameters.BackBufferWidth = parameters.BackBufferHeight = 32;
    parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.EnableAutoDepthStencil = TRUE;
    parameters.AutoDepthStencilFormat = D3DFMT_D32;
    parameters.hDeviceWindow = window;
    IDirect3DDevice9 *device = nullptr;
    need(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &parameters, &device)), "device");

    need(!StormMetalDynamicSkyActive(device), "scope starts inactive");
    need(!StormMetalCloudFieldCurrent(device), "authored astronomy is retained before a cloud snapshot exists");
    need(StormMetalPrepareDynamicSky(device, 18.f, 1.f / 60.f, .7f, .8f, .4f, 9.f, .16f, 0.f, 1.f, 0.f, 0xff315273u),
         "early SUNGLOW owner prepares the field before SKY realizes");
    need(!StormMetalDynamicSkyActive(device), "preparing astronomy must not activate the sky draw scope");
    need(StormMetalCloudFieldCurrent(device), "early astronomy sees the prepared cloud field");
    need(StormMetalBeginDynamicSky(device, 18.f, 1.f / 60.f, .7f, .8f, .4f, 9.f, .16f, 0.f, 1.f, 0.f, 0xff315273u),
         "explicit SKY owner enables dynamic pipeline");
    need(StormMetalDynamicSkyActive(device), "scope is visible only after explicit begin");
    need(StormMetalCloudFieldCurrent(device), "astronomy and water can consume the sky's completed snapshot");
    need(StormMetalPrepareDynamicSky(device, 18.f, 0.f, .7f, .8f, .4f, 9.f, .16f, 0.f, 1.f, 0.f, 0xff315273u),
         "nested astronomy can reuse the current sky snapshot");
    need(StormMetalDynamicSkyActive(device), "nested astronomy preparation must retain the enclosing sky draw scope");
    need(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.f, 0)),
         "frame clear");
    need(!StormMetalDynamicSkyActive(device), "frame boundary rejects a leaked sky scope");
    need(StormMetalBeginDynamicSky(device, 18.f, 0.f, .7f, .8f, .4f, 9.f, .16f, 0.f, 1.f, 0.f, 0xff315273u),
         "scope can restart after frame cleanup");
    StormMetalEndDynamicSky(device);
    need(!StormMetalDynamicSkyActive(device), "explicit end prevents unrelated draws from inheriting sky");

    need(!StormMetalSkyFogActive(device), "fog scope starts inactive");
    need(!StormMetalSolarVisualActive(device), "solar scope starts inactive");
    need(StormMetalBeginSolarVisual(device), "astronomy opens its explicit visual scope");
    need(!StormMetalBeginSolarVisual(device), "nested solar begin does not steal the outer scope");
    need(StormMetalSolarVisualActive(device), "outer solar scope survives a rejected nested begin");
    StormMetalEndSolarVisual(device);
    need(!StormMetalSolarVisualActive(device), "solar end prevents unrelated draws from inheriting cloud attenuation");
    need(StormMetalBeginSolarVisual(device), "solar scope can restart before frame cleanup");
    need(StormMetalBeginSkyFog(device, 0xff6d8ca7u), "explicit SKY owner enables draw-time fog color");
    need(StormMetalSkyFogActive(device), "fog scope is visible only after explicit begin");
    StormMetalEndSkyFog(device);
    need(!StormMetalSkyFogActive(device), "explicit end prevents unrelated draws from inheriting fog color");
    need(StormMetalBeginSkyFog(device, 0xff6d8ca7u), "fog scope can restart before frame cleanup");
    need(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.f, 0)),
         "fog frame clear");
    need(!StormMetalSolarVisualActive(device), "frame cleanup clears an abandoned solar scope");
    need(!StormMetalSkyFogActive(device), "frame boundary rejects a leaked fog scope");

    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS explicit dynamic-sky and draw-time fog backend scopes with cleanup");
}

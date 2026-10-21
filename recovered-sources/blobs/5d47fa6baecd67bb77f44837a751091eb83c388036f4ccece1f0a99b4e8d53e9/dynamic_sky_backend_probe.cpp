#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>

extern "C" bool StormMetalBeginDynamicSky(void *, float, float, float, float, float, float, float);
extern "C" void StormMetalEndDynamicSky(void *);
extern "C" bool StormMetalDynamicSkyActive(void *);

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
    need(StormMetalBeginDynamicSky(device, 18.f, 1.f / 60.f, .7f, .8f, .4f, 9.f, .16f),
         "explicit SKY owner enables dynamic pipeline");
    need(StormMetalDynamicSkyActive(device), "scope is visible only after explicit begin");
    StormMetalEndDynamicSky(device);
    need(!StormMetalDynamicSkyActive(device), "explicit end prevents unrelated draws from inheriting sky");

    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS explicit dynamic-sky backend scope and cleanup");
}

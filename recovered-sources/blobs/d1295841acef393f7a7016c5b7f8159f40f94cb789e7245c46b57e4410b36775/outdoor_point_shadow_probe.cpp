#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>

extern "C" void StormMetalLandLocation(void*, bool);
extern "C" void StormMetalLandInterior(void*, bool);
extern "C" bool StormMetalBeginShadowFrame(void*, uint64_t, uint64_t, const float*);
extern "C" bool StormMetalBeginShadowPass(void*, unsigned, unsigned, uint64_t, const float*, float, float*, float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" bool StormMetalOutdoorSunShadowsReady(void*);
extern "C" bool StormMetalOutdoorPointShadowReady(void*);

static void need(bool value, const char* label) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}

int main() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto* window = SDL_CreateWindow("Outdoor point shadow", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, "window");
    auto* api = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp{};
    pp.Windowed = TRUE; pp.BackBufferWidth = pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = window;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* device = nullptr;
    need(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &pp, &device)), "device");

    StormMetalLandLocation(device, true);
    StormMetalLandInterior(device, false);
    const float focus[] = {0, 0, 8};
    need(StormMetalBeginShadowFrame(device, 7, 1, focus), "frame");
    auto pass = [&](unsigned kind, unsigned face, uint64_t id, const float* source, float range) {
        D3DMATRIX view{}, projection{};
        need(StormMetalBeginShadowPass(device, kind, face, id, source, range,
                                       &view.m[0][0], &projection.m[0][0]), "pass");
        StormMetalEndShadowPass(device);
    };
    const float lamp[] = {2, 3, 4};
    for (unsigned face = 0; face != 6; ++face) pass(1, face, 17, lamp, 24);
    const float sun[] = {.6f, .1f, 1};
    pass(0, 0, 1, sun, 32); pass(0, 1, 1, sun, 96);
    need(StormMetalEndShadowFrame(device), "complete frame");
    need(StormMetalOutdoorPointShadowReady(device), "one complete outdoor lamp cube");
    need(StormMetalOutdoorSunShadowsReady(device), "outdoor sun cascades preserved");

    device->Release(); api->Release(); SDL_DestroyWindow(window); SDL_Quit();
    std::puts("PASS bounded outdoor point cube and independent sun cascades");
}

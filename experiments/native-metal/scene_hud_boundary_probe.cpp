#include <d3d9.h>
#include <SDL.h>
#include <cstdio>
#include <cstdlib>

extern "C" bool StormMetalSceneHudSnapshotReady(void *device);
extern "C" uint64_t StormMetalCinematicPostFXFrames(void *device);
extern "C" uint64_t StormMetalFXAAFrames(void *device);
extern "C" void StormMetalLandLocation(void *device, bool active);
extern "C" void StormMetalLandInterior(void *device, bool indoor);
extern "C" void StormMetalInterfaceBackScene(void *device, bool active);
extern "C" bool StormMetalInterfaceBackSceneActive(void *device);

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
}

int main()
{
    setenv("STORM_METAL_CINEMATIC", "1", 1);
    setenv("STORM_METAL_ANTIALIASING", "1", 1);
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Scene HUD boundary", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    require(window != nullptr, SDL_GetError());
    auto *api = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS parameters{};
    parameters.Windowed = TRUE;
    parameters.BackBufferWidth = parameters.BackBufferHeight = 32;
    parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow = window;
    IDirect3DDevice9 *device = nullptr;
    require(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &parameters, &device)), "device");

    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "scene clear");
    require(!StormMetalSceneHudSnapshotReady(device), "fresh frame has no scene snapshot");
    require(StormMetalCinematicPostFXFrames(device) == 0, "fresh device has no postfx frame");
    require(StormMetalFXAAFrames(device) == 0, "fresh device has no FXAA frame");
    D3DPERF_SetMarker(0, L"unrelated marker");
    require(!StormMetalSceneHudSnapshotReady(device), "unrelated marker is ignored");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "sea captures FXAA input without cinematic processing");
    require(StormMetalCinematicPostFXFrames(device) == 0, "sea marker skips every cinematic pass");
    require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "sea frame present");
    require(StormMetalCinematicPostFXFrames(device) == 0, "sea Present fallback also skips cinematic passes");
    require(StormMetalFXAAFrames(device) == 1, "sea marker plus Present performs FXAA only once");

    StormMetalLandLocation(device, true);
    StormMetalLandInterior(device, false);
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "outdoor scene clear");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "outdoor scene captures FXAA input without cinematic processing");
    require(StormMetalCinematicPostFXFrames(device) == 0, "outdoor marker skips every cinematic pass");
    require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "outdoor frame present");
    require(StormMetalCinematicPostFXFrames(device) == 0, "outdoor Present fallback also skips cinematic passes");
    require(StormMetalFXAAFrames(device) == 2, "outdoor marker plus Present performs FXAA only once");

    StormMetalLandInterior(device, true);
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "interior scene clear");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "scene/HUD marker captures scene");
    require(StormMetalCinematicPostFXFrames(device) == 1, "scene/HUD marker invokes postfx exactly once");
    require(StormMetalFXAAFrames(device) == 3, "scene/HUD marker invokes FXAA exactly once");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "repeated marker preserves first scene snapshot");
    require(StormMetalCinematicPostFXFrames(device) == 1, "repeated marker cannot double-process scene");
    require(StormMetalFXAAFrames(device) == 3, "repeated marker cannot double-process FXAA");
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER, 0, 1.f, 0)), "HUD depth clear");
    require(StormMetalSceneHudSnapshotReady(device), "depth clear cannot re-arm scene marker during HUD");
    require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "frame present");
    require(!StormMetalSceneHudSnapshotReady(device), "present re-arms scene marker for next frame");
    require(StormMetalCinematicPostFXFrames(device) == 1, "marked frame is processed once");
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "fallback scene clear");
    require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "fallback frame present");
    require(StormMetalCinematicPostFXFrames(device) == 2, "Present fallback invokes postfx when marker is absent");
    require(StormMetalFXAAFrames(device) == 4, "Present fallback invokes FXAA exactly once");

    StormMetalInterfaceBackScene(device, true);
    require(StormMetalInterfaceBackSceneActive(device), "interface background enters the interior scene domain atomically");
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "menu scene clear");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "menu scene is captured before interface rendering");
    require(StormMetalCinematicPostFXFrames(device) == 3, "menu scene receives the interior cinematic pass");
    require(StormMetalFXAAFrames(device) == 5, "menu scene receives FXAA exactly once");
    require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "menu frame present");

    StormMetalInterfaceBackScene(device, false);
    require(!StormMetalInterfaceBackSceneActive(device), "interface background exit clears its scene domain");
    StormMetalLandLocation(device, true);
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff123456, 1.f, 0)), "post-menu outdoor clear");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "outdoor resumes with FXAA snapshot after menu");
    require(StormMetalFXAAFrames(device) == 6, "outdoor FXAA resumes once after menu");
    require(StormMetalCinematicPostFXFrames(device) == 3, "post-menu outdoor scene remains cinematic-free");

    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: menu interior scene/HUD bridge is atomic and does not leak into outdoor scenes");
}

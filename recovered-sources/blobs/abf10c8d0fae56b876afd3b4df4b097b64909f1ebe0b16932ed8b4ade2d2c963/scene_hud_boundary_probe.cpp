#include <d3d9.h>
#include <SDL.h>
#include <cstdio>
#include <cstdlib>

extern "C" bool StormMetalSceneHudSnapshotReady(void *device);

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
    D3DPERF_SetMarker(0, L"unrelated marker");
    require(!StormMetalSceneHudSnapshotReady(device), "unrelated marker is ignored");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "scene/HUD marker captures scene");
    D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary");
    require(StormMetalSceneHudSnapshotReady(device), "repeated marker preserves first scene snapshot");
    require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff654321, 1.f, 0)), "next frame clear");
    require(!StormMetalSceneHudSnapshotReady(device), "next frame invalidates stale scene snapshot");

    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: explicit scene/HUD boundary snapshot contract");
}

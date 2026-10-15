#include "render_device_api.hpp"
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

    storm_render::Presentation parameters{};

    parameters.width = parameters.height = 32;
    parameters.colorFormat = storm_render::AssetFormat::A8R8G8B8;



    storm_render::Device *device = nullptr;
    require(storm_render::succeeded(storm_render::createMetalDevice(window, &parameters, &device)), "device");

    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "scene clear");
    require(!StormMetalSceneHudSnapshotReady(device), "fresh frame has no scene snapshot");
    require(StormMetalCinematicPostFXFrames(device) == 0, "fresh device has no postfx frame");
    require(StormMetalFXAAFrames(device) == 0, "fresh device has no FXAA frame");
    storm_render::markSceneBoundary(nullptr, storm_render::SceneBoundary::Hud);
    require(!StormMetalSceneHudSnapshotReady(device), "missing device boundary is ignored");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "sea captures FXAA input without cinematic processing");
    require(StormMetalCinematicPostFXFrames(device) == 0, "sea marker skips every cinematic pass");
    require(storm_render::succeeded(device->Present()), "sea frame present");
    require(StormMetalCinematicPostFXFrames(device) == 0, "sea Present fallback also skips cinematic passes");
    require(StormMetalFXAAFrames(device) == 1, "sea marker plus Present performs FXAA only once");

    StormMetalLandLocation(device, true);
    StormMetalLandInterior(device, false);
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "outdoor scene clear");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "outdoor scene captures FXAA input without cinematic processing");
    require(StormMetalCinematicPostFXFrames(device) == 0, "outdoor marker skips every cinematic pass");
    require(storm_render::succeeded(device->Present()), "outdoor frame present");
    require(StormMetalCinematicPostFXFrames(device) == 0, "outdoor Present fallback also skips cinematic passes");
    require(StormMetalFXAAFrames(device) == 2, "outdoor marker plus Present performs FXAA only once");

    StormMetalLandInterior(device, true);
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "interior scene clear");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "scene/HUD marker captures scene");
    require(StormMetalCinematicPostFXFrames(device) == 1, "scene/HUD marker invokes postfx exactly once");
    require(StormMetalFXAAFrames(device) == 3, "scene/HUD marker invokes FXAA exactly once");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "repeated marker preserves first scene snapshot");
    require(StormMetalCinematicPostFXFrames(device) == 1, "repeated marker cannot double-process scene");
    require(StormMetalFXAAFrames(device) == 3, "repeated marker cannot double-process FXAA");
    require(storm_render::succeeded(device->Clear(storm_render::clear::depth, 0, 1.f)), "HUD depth clear");
    require(StormMetalSceneHudSnapshotReady(device), "depth clear cannot re-arm scene marker during HUD");
    require(storm_render::succeeded(device->Present()), "frame present");
    require(!StormMetalSceneHudSnapshotReady(device), "present re-arms scene marker for next frame");
    require(StormMetalCinematicPostFXFrames(device) == 1, "marked frame is processed once");
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "fallback scene clear");
    require(storm_render::succeeded(device->Present()), "fallback frame present");
    require(StormMetalCinematicPostFXFrames(device) == 2, "Present fallback invokes postfx when marker is absent");
    require(StormMetalFXAAFrames(device) == 4, "Present fallback invokes FXAA exactly once");

    StormMetalInterfaceBackScene(device, true);
    require(StormMetalInterfaceBackSceneActive(device), "interface background enters the interior scene domain atomically");
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "menu scene clear");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "menu scene is captured before interface rendering");
    require(StormMetalCinematicPostFXFrames(device) == 3, "menu scene receives the interior cinematic pass");
    require(StormMetalFXAAFrames(device) == 5, "menu scene receives FXAA exactly once");
    require(storm_render::succeeded(device->Present()), "menu frame present");

    StormMetalInterfaceBackScene(device, false);
    require(!StormMetalInterfaceBackSceneActive(device), "interface background exit clears its scene domain");
    StormMetalLandLocation(device, true);
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1.f)), "post-menu outdoor clear");
    storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);
    require(StormMetalSceneHudSnapshotReady(device), "outdoor resumes with FXAA snapshot after menu");
    require(StormMetalFXAAFrames(device) == 6, "outdoor FXAA resumes once after menu");
    require(StormMetalCinematicPostFXFrames(device) == 3, "post-menu outdoor scene remains cinematic-free");

    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: menu interior scene/HUD bridge is atomic and does not leak into outdoor scenes");
}

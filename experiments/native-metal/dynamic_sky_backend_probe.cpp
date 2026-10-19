#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

    storm_render::Presentation parameters{};

    parameters.width = parameters.height = 32;
    parameters.colorFormat = storm_render::AssetFormat::A8R8G8B8;


    parameters.depthEnabled = true;
    parameters.depthFormat = storm_render::AssetFormat::D32;

    storm_render::Device *device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &parameters, &device)), "device");

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
    need(storm_render::succeeded(device->Clear(storm_render::clear::color | storm_render::clear::depth, 0, 1.f)),
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
    need(storm_render::succeeded(device->Clear(storm_render::clear::color | storm_render::clear::depth, 0, 1.f)),
         "fog frame clear");
    need(!StormMetalSolarVisualActive(device), "frame cleanup clears an abandoned solar scope");
    need(!StormMetalSkyFogActive(device), "frame boundary rejects a leaked fog scope");

    // Exercise the actual SKY stream, rather than accepting an active scope
    // while an earlier generic unlit path still draws the authored panorama.
    storm_render::Surface *target = nullptr, *readback = nullptr;
    need(storm_render::succeeded(device->GetRenderTarget(&target)), "sky target");
    need(storm_render::succeeded(device->CreateOffscreenPlainSurface(
             32, 32, storm_render::AssetFormat::A8R8G8B8, &readback)), "sky readback surface");
    storm_render::Matrix identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.f;
    device->SetTransform(storm_render::TransformWorld, &identity);
    device->SetTransform(storm_render::TransformView, &identity);
    device->SetTransform(storm_render::TransformProjection, &identity);
    device->SetFVF(storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse |
                   storm_render::vertexFormat::tex2 | storm_render::vertexFormat::texCoord2);
    device->SetRenderState(storm_render::RenderLighting, false);
    device->SetRenderState(storm_render::RenderDepthTest, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    device->SetTextureStageState(0, storm_render::StageAlphaOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageAlphaArg1, storm_render::textureArg::diffuse);
    device->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    const SkyVertex triangle[] = {
        {{-1.f, -1.f, .5f}, 0xffff00ff, {}, {}},
        {{ 3.f, -1.f, .5f}, 0xffff00ff, {}, {}},
        {{-1.f,  3.f, .5f}, 0xffff00ff, {}, {}},
    };
    const auto skyPixel = [&](bool dynamic, float hour) {
        device->Present();
        device->Clear(storm_render::clear::color | storm_render::clear::depth, 0, 1.f);
        device->BeginScene();
        if (dynamic)
            need(StormMetalBeginDynamicSky(device, hour, 0.f, .7f, .8f, .4f, 9.f, .16f,
                                          0.f, 1.f, 0.f, 0xff315273u), "draw-time sky scope");
        need(storm_render::succeeded(device->DrawPrimitiveUP(
                 storm_render::PrimitiveTriangleList, 1, triangle, sizeof(SkyVertex))), "SKY-format draw");
        if (dynamic) StormMetalEndDynamicSky(device);
        device->EndScene();
        need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "sky pixel readback");
        storm_render::LockedRect lock{};
        need(storm_render::succeeded(readback->LockRect(&lock, nullptr, storm_render::lock::readOnly)), "sky read lock");
        uint32_t pixel = 0;
        std::memcpy(&pixel, static_cast<const char *>(lock.pBits) + 16 * lock.Pitch + 16 * 4, 4);
        readback->UnlockRect();
        return pixel & 0x00ffffffu;
    };
    const auto authored = skyPixel(false, 12.f), day = skyPixel(true, 12.f), night = skyPixel(true, 23.f);
    std::printf("SKY pixels authored=%06x day=%06x night=%06x\n", authored, day, night);
    need(authored == 0x00ff00ffu, "authored SKY control retains vertex colour");
    need(day != authored && night != authored, "dynamic SKY scope replaces authored pixels");
    need(day != night, "dynamic SKY pixels follow WEATHER hour");
    need(skyPixel(false, 23.f) == authored, "ending SKY scope preserves ordinary unlit draws");
    readback->Release();
    target->Release();

    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS explicit dynamic-sky and draw-time fog backend scopes with cleanup");
}

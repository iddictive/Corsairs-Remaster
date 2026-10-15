#include <SDL.h>
#include "render_device_api.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void need(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct Vertex { float x, y, z, nx, ny, nz; };

static uint32_t draw(storm_render::Device* device, storm_render::Surface* target,
                  storm_render::Surface* readback, storm_render::Light light,
                  bool specular, bool localViewer, float power = 24.f,
                  float materialSpecular = 1.f) {
    storm_render::Material material{};
    material.Diffuse = {.25f, .25f, .25f, 1.f};
    material.Specular = {materialSpecular, materialSpecular * .25f,
                         materialSpecular * .125f, 1.f};
    material.Power = power;
    device->SetMaterial(&material);
    device->SetLight(0, &light);
    device->LightEnable(0, true);
    device->SetRenderState(storm_render::RenderSpecular, specular);
    device->SetRenderState(storm_render::RenderLocalViewer, localViewer);
    device->Clear(storm_render::clear::color, 0xff000000, 1.f);
    const Vertex quad[] = {
        {-.05f,  .05f, .5f, 0.f, 0.f, -1.f},
        { .05f,  .05f, .5f, 0.f, 0.f, -1.f},
        {-.05f, -.05f, .5f, 0.f, 0.f, -1.f},
        { .05f, -.05f, .5f, 0.f, 0.f, -1.f},
    };
    device->BeginScene();
    const storm_render::Result result = device->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, 2, quad, sizeof(Vertex));
    device->EndScene();
    need(storm_render::succeeded(result), "GPU fixture draw");
    need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "GPU fixture readback");
    storm_render::LockedRect lock{};
    readback->LockRect(&lock, nullptr, storm_render::lock::readOnly);
    uint32_t pixel = 0;
    std::memcpy(&pixel, static_cast<const char*>(lock.pBits) + 16 * lock.Pitch + 16 * 4, 4);
    readback->UnlockRect();
    return pixel;
}

static int red(uint32_t value) { return int((value >> 16) & 255); }

int main() {
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto* window = SDL_CreateWindow("FFP lighting parity", 0, 0, 32, 32,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());

    storm_render::Presentation pp{};
    pp.width = pp.height = 32;

    pp.colorFormat = storm_render::AssetFormat::A8R8G8B8;



    storm_render::Device* device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "device");
    need(storm_render::succeeded(device->SetFVF(storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal)), "normal-bearing vertex format");
    // FFP evaluates lighting at vertices, then interpolates. Keep all four
    // vertices near the light axis and magnify only their projected footprint.
    storm_render::Matrix projection{};
    projection._11 = projection._22 = 20.f;
    projection._33 = projection._44 = 1.f;
    device->SetTransform(storm_render::TransformProjection, &projection);
    device->SetRenderState(storm_render::RenderDepthTest, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetRenderState(storm_render::RenderLighting, true);
    device->SetRenderState(storm_render::RenderVertexColor, false);
    device->SetRenderState(storm_render::RenderDiffuseMaterialSource, storm_render::MaterialSourceMaterial);
    device->SetRenderState(storm_render::RenderSpecularMaterialSource, storm_render::MaterialSourceMaterial);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    storm_render::Surface *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(&target);
    device->CreateOffscreenPlainSurface(32, 32, storm_render::AssetFormat::A8R8G8B8, &readback);

    storm_render::Light directional{};
    directional.Type = storm_render::LightKind::Directional;
    directional.Direction = {0.f, 0.f, 1.f};
    directional.Diffuse = {.1f, .1f, .1f, 1.f};
    directional.Specular = {1.f, 1.f, 1.f, 1.f};
    const uint32_t noSpecular = draw(device, target, readback, directional, false, true);
    const uint32_t localSpecular = draw(device, target, readback, directional, true, true);
    const uint32_t infiniteViewer = draw(device, target, readback, directional, true, false);
    need(red(localSpecular) > red(noSpecular) + 80,
         "material/light specular and Power reach the GPU when enabled");
    need(red(infiniteViewer) + 40 < red(localSpecular),
         "LOCALVIEWER changes the halfway vector");
    storm_render::Light noLightSpecular = directional;
    noLightSpecular.Specular = {0.f, 0.f, 0.f, 1.f};
    const uint32_t zeroLightSpecular = draw(device, target, readback, noLightSpecular, true, true);
    const uint32_t zeroMaterialSpecular = draw(device, target, readback, directional, true, true, 24.f, 0.f);
    need(red(localSpecular) > red(zeroLightSpecular) + 80,
         "D3DLIGHT9 Specular is the highlight energy owner");
    need(red(localSpecular) > red(zeroMaterialSpecular) + 80,
         "D3DMATERIAL9 Specular is the highlight reflectance owner");
    directional.Direction = {.45f, 0.f, 1.f};
    const uint32_t broadHighlight = draw(device, target, readback, directional, true, true, 2.f);
    const uint32_t narrowHighlight = draw(device, target, readback, directional, true, true, 64.f);
    need(red(broadHighlight) > red(narrowHighlight) + 20,
         "material Power narrows an off-axis GPU highlight");

    storm_render::Light point{};
    point.Type = storm_render::LightKind::Point;
    point.Position = {0.f, 0.f, -1.f};
    point.Diffuse = {1.f, 1.f, 1.f, 1.f};
    point.Range = 2.f;
    point.Attenuation0 = 1.f;
    const uint32_t insidePoint = draw(device, target, readback, point, false, true);
    point.Range = 1.f;
    const uint32_t outsidePoint = draw(device, target, readback, point, false, true);
    need(red(insidePoint) > red(outsidePoint) + 40,
         "point light keeps its hard D3D Range cutoff");

    storm_render::Light spot{};
    spot.Type = storm_render::LightKind::Spot;
    spot.Position = {0.f, 0.f, -1.f};
    spot.Direction = {0.f, 0.f, 1.f};
    spot.Diffuse = {1.f, 1.f, 1.f, 1.f};
    spot.Range = 10.f;
    spot.Attenuation0 = 1.f;
    spot.Theta = .25f;
    spot.Phi = .5f;
    spot.Falloff = 1.f;
    const uint32_t insideSpot = draw(device, target, readback, spot, false, true);
    spot.Direction = {1.f, 0.f, 0.f};
    const uint32_t outsideSpot = draw(device, target, readback, spot, false, true);
    need(red(insideSpot) > red(outsideSpot) + 40,
         "spot theta/phi/falloff reject a receiver outside the cone");

    target->Release();
    readback->Release();
    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: GPU D3D9 material, light, specular/local-viewer and spot semantics");
}

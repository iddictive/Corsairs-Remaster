#include <SDL.h>
#include "render_device_api.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" void storm_metal_label_texture(storm_render::BaseTexture*, const char*);

namespace {
constexpr unsigned side = 128;
constexpr unsigned textureSide = 64;

struct Vertex { float x, y, z, rhw, u, v; };

void need(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

std::vector<uint32_t> smoothPattern(unsigned level) {
    const unsigned width = std::max(1u, textureSide >> level);
    std::vector<uint32_t> pixels(width * width);
    for (unsigned y = 0; y < width; ++y) for (unsigned x = 0; x < width; ++x) {
        const float phaseX = 6.28318530718f * float(x) / float(width);
        const float phaseY = 6.28318530718f * float(y) / float(width);
        const auto red = uint32_t(std::lround(127.5f + 110.f * std::sin(phaseX)));
        const auto green = uint32_t(std::lround(127.5f + 95.f * std::sin(phaseY)));
        const auto blue = uint32_t(std::lround(127.5f + 80.f * std::sin(phaseX + phaseY)));
        const auto alpha = ((x + y) & 7u) == 0 ? 96u : 255u;
        pixels[y * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
    }
    return pixels;
}

std::vector<uint32_t> mipColors(unsigned level) {
    const unsigned width = std::max(1u, textureSide >> level);
    constexpr std::array<uint32_t, 7> colors{
        0xffff0000u, 0xff00ff00u, 0xff0000ffu, 0xffffff00u,
        0xffff00ffu, 0xff00ffffu, 0xffffffffu};
    return std::vector<uint32_t>(width * width, colors[level]);
}

storm_render::Texture* makeTexture(storm_render::Device* device, bool colouredMips) {
    storm_render::Texture* texture = nullptr;
    need(storm_render::succeeded(device->CreateTexture(textureSide, textureSide, 7, 0, storm_render::AssetFormat::A8R8G8B8, &texture)), "fixture texture");
    for (unsigned level = 0; level < 7; ++level) {
        const unsigned width = std::max(1u, textureSide >> level);
        const auto pixels = colouredMips ? mipColors(level) : smoothPattern(level);
        storm_render::LockedRect locked{};
        need(storm_render::succeeded(texture->LockRect(level, &locked, nullptr, 0)), "lock fixture mip");
        for (unsigned y = 0; y < width; ++y)
            std::memcpy(static_cast<unsigned char*>(locked.pBits) + y * locked.Pitch,
                        pixels.data() + y * width, width * sizeof(uint32_t));
        texture->UnlockRect(level);
    }
    return texture;
}

std::vector<uint32_t> sourceMip(storm_render::Texture* texture) {
    storm_render::SurfaceDesc desc{};
    texture->GetLevelDesc(0, &desc);
    std::vector<uint32_t> copy(desc.Width * desc.Height);
    storm_render::LockedRect locked{};
    need(storm_render::succeeded(texture->LockRect(0, &locked, nullptr, storm_render::lock::readOnly)), "read fixture source");
    for (uint32_t y = 0; y < desc.Height; ++y)
        std::memcpy(copy.data() + y * desc.Width,
                    static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch,
                    desc.Width * sizeof(uint32_t));
    texture->UnlockRect(0);
    return copy;
}

std::vector<uint32_t> draw(storm_render::Device* device, storm_render::Texture* texture,
                        const char* label, float repeats, float uvOffset) {
    storm_metal_label_texture(texture, label);
    need(storm_render::succeeded(device->SetTexture(0, texture)), "bind fixture texture");
    device->Clear(storm_render::clear::color, 0, 1.f);
    const Vertex quad[] = {
        {0.f, 0.f, 0.f, 1.f, uvOffset, uvOffset},
        {float(side), 0.f, 0.f, 1.f, uvOffset + repeats, uvOffset},
        {0.f, float(side), 0.f, 1.f, uvOffset, uvOffset + repeats},
        {float(side), float(side), 0.f, 1.f, uvOffset + repeats, uvOffset + repeats},
    };
    need(storm_render::succeeded(device->BeginScene()), "begin fixture scene");
    need(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, 2, quad, sizeof(Vertex))),
         "draw fixture quad");
    need(storm_render::succeeded(device->EndScene()), "end fixture scene");
    storm_render::Surface *target = nullptr, *readback = nullptr;
    need(storm_render::succeeded(device->GetRenderTarget(&target)), "fixture target");
    need(storm_render::succeeded(device->CreateOffscreenPlainSurface(side, side, storm_render::AssetFormat::A8R8G8B8, &readback)),
         "fixture readback");
    need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "fixture readback copy");
    storm_render::LockedRect locked{};
    need(storm_render::succeeded(readback->LockRect(&locked, nullptr, storm_render::lock::readOnly)), "lock fixture readback");
    std::vector<uint32_t> pixels(side * side);
    for (unsigned y = 0; y < side; ++y)
        std::memcpy(pixels.data() + y * side,
                    static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch,
                    side * sizeof(uint32_t));
    readback->UnlockRect();
    readback->Release();
    target->Release();
    return pixels;
}

size_t differences(const std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
    size_t total = 0;
    for (size_t index = 0; index < left.size(); ++index) total += left[index] != right[index];
    return total;
}

unsigned rgbDelta(uint32_t left, uint32_t right) {
    unsigned worst = 0;
    for (unsigned shift : {0u, 8u, 16u}) {
        const int a = int((left >> shift) & 255u), b = int((right >> shift) & 255u);
        worst = std::max(worst, unsigned(std::abs(a - b)));
    }
    return worst;
}

size_t periodicRgbMatches(const std::vector<uint32_t>& pixels, unsigned period) {
    size_t matches = 0;
    for (unsigned y = 0; y < side; ++y) for (unsigned x = 0; x + period < side; ++x)
        matches += (pixels[y * side + x] & 0x00ffffffu) ==
                   (pixels[y * side + x + period] & 0x00ffffffu);
    return matches;
}

} // namespace

int main() {
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto* window = SDL_CreateWindow("anti tiling GPU", 0, 0, side, side,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());


    storm_render::Presentation pp{};
    pp.width = pp.height = side;

    pp.colorFormat = storm_render::AssetFormat::A8R8G8B8;



    storm_render::Device* device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "D3D9 device");
    need(storm_render::succeeded(device->SetFVF(storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::tex1)), "fixture FVF");
    device->SetRenderState(storm_render::RenderDepthTest, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetRenderState(storm_render::RenderLighting, false);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::texture);
    device->SetTextureStageState(0, storm_render::StageAlphaOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageAlphaArg1, storm_render::textureArg::texture);
    device->SetSamplerState(0, storm_render::SamplerAddressU, storm_render::TextureAddressWrap);
    device->SetSamplerState(0, storm_render::SamplerAddressV, storm_render::TextureAddressWrap);
    device->SetSamplerState(0, storm_render::SamplerMinFilter, storm_render::TextureFilterPoint);
    device->SetSamplerState(0, storm_render::SamplerMagFilter, storm_render::TextureFilterPoint);
    device->SetSamplerState(0, storm_render::SamplerMipFilter, storm_render::TextureFilterPoint);

    auto* baselineTexture = makeTexture(device, false);
    auto* floorTexture = makeTexture(device, false);
    auto* floorTxTexture = makeTexture(device, false);
    auto* sandTexture = makeTexture(device, false);
    auto* pavingTexture = makeTexture(device, false);
    auto* alphaTexture = makeTexture(device, false);
    const auto originalFloorBytes = sourceMip(floorTexture);
    // 3/64 shifts every random-cell boundary inside a 2x2 derivative quad;
    // the old implicit-gradient path can therefore no longer evade the probe
    // by landing each boundary on a derivative-quad edge.
    constexpr float uvOffset = 3.f / 64.f;
    const auto baseline = draw(device, baselineTexture, "unlabelled.tga", 4.f, uvOffset);
    const auto floor = draw(device, floorTexture, "floorU3.tga", 4.f, uvOffset);
    const auto floorTx = draw(device, floorTxTexture, "RESOURCE/Textures/floorU3.tga.tx", 4.f, uvOffset);
    const auto sand = draw(device, sandTexture, "Sandtile.tga", 4.f, uvOffset);
    const auto paving = draw(device, pavingTexture, "cobbleM1.tga", 4.f, uvOffset);
    const auto alpha = draw(device, alphaTexture, "trees.tga", 4.f, uvOffset);
    need(differences(floor, baseline) > side * side / 4, "floorU3 labels activate non-periodic sampling");
    need(differences(floorTx, baseline) > side * side / 4, ".tga.tx floorU3 labels activate anti-tiling");
    need(differences(sand, baseline) > side * side / 4, "Sandtile labels activate non-periodic sampling");
    need(floor == floorTx, "equivalent .tga and .tga.tx labels select the same sampling");
    need(paving == baseline, "directional paving retains byte-identical authored sampling");
    need(alpha == baseline, "alpha/foliage label retains byte-identical authored sampling");
    need(sourceMip(floorTexture) == originalFloorBytes, "sampling preserves original texture bytes");
    device->SetSamplerState(0, storm_render::SamplerAddressU, storm_render::TextureAddressClamp);
    need(draw(device, floorTexture, "floorU3.tga", 4.f, uvOffset) == draw(device, baselineTexture, "unlabelled.tga", 4.f, uvOffset),
         "explicit U clamp keeps authored sampling");
    device->SetSamplerState(0, storm_render::SamplerAddressU, storm_render::TextureAddressWrap);
    device->SetSamplerState(0, storm_render::SamplerAddressV, storm_render::TextureAddressClamp);
    need(draw(device, floorTexture, "floorU3.tga", 4.f, uvOffset) == draw(device, baselineTexture, "unlabelled.tga", 4.f, uvOffset),
         "explicit V clamp keeps authored sampling");
    device->SetSamplerState(0, storm_render::SamplerAddressV, storm_render::TextureAddressWrap);
    const size_t originalPeriod = periodicRgbMatches(baseline, 32);
    need(originalPeriod > side * side * 2 / 3, "baseline fixture retains its four exact repeats");
    need(periodicRgbMatches(floor, 32) < originalPeriod / 3,
         "floorU3 sampling suppresses the source texture's repeated period");
    need(periodicRgbMatches(sand, 32) < originalPeriod / 3,
         "Sandtile sampling suppresses the source texture's repeated period");

    // At u=0.5+n the stochastic-cell owner changes.  The low-frequency periodic
    // fixture makes an actual discontinuity visible as a jump larger than one
    // source texel; the blend must remain continuous through that boundary.
    unsigned seamJump = 0;
    for (unsigned y = 24; y < 104; ++y) for (unsigned left : {14u, 46u, 78u, 110u})
        seamJump = std::max(seamJump, rgbDelta(floor[y * side + left], floor[y * side + left + 1]));
    need(seamJump <= 32, "four-tap cell boundary remains visually continuous");

    auto* mipTexture = makeTexture(device, true);
    const auto mipBaseline = draw(device, mipTexture, "unlabelled.tga", 8.f, uvOffset);
    const auto mipFloor = draw(device, mipTexture, "floorU3.tga", 8.f, uvOffset);
    // Every mip is one solid, distinct colour.  Equal values on both sides of
    // two cell boundaries prove the helper supplies the same transformed
    // gradient to every tap instead of allowing a boundary-crossing derivative
    // quad to choose a different LOD.
    for (unsigned left : {6u, 22u, 38u, 54u, 70u, 86u, 102u, 118u})
        for (unsigned x : {left, left + 1u})
            need(mipFloor[64 * side + x] == mipBaseline[64 * side + x],
                 "gradient-aware anti-tiling keeps the mip LOD stable at a cell boundary");

    for (auto* texture : {baselineTexture, floorTexture, floorTxTexture, sandTexture,
                          pavingTexture, alphaTexture, mipTexture}) texture->Release();
    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: anti-tiling labels, exact exclusions, source-byte preservation, cell continuity and mip LOD stability");
}

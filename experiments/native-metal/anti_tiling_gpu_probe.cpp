#include <SDL.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" void storm_metal_label_texture(IDirect3DBaseTexture9*, const char*);

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

std::vector<DWORD> smoothPattern(unsigned level) {
    const unsigned width = std::max(1u, textureSide >> level);
    std::vector<DWORD> pixels(width * width);
    for (unsigned y = 0; y < width; ++y) for (unsigned x = 0; x < width; ++x) {
        const float phaseX = 6.28318530718f * float(x) / float(width);
        const float phaseY = 6.28318530718f * float(y) / float(width);
        const auto red = DWORD(std::lround(127.5f + 110.f * std::sin(phaseX)));
        const auto green = DWORD(std::lround(127.5f + 95.f * std::sin(phaseY)));
        const auto blue = DWORD(std::lround(127.5f + 80.f * std::sin(phaseX + phaseY)));
        const auto alpha = ((x + y) & 7u) == 0 ? 96u : 255u;
        pixels[y * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
    }
    return pixels;
}

std::vector<DWORD> mipColors(unsigned level) {
    const unsigned width = std::max(1u, textureSide >> level);
    constexpr std::array<DWORD, 7> colors{
        0xffff0000u, 0xff00ff00u, 0xff0000ffu, 0xffffff00u,
        0xffff00ffu, 0xff00ffffu, 0xffffffffu};
    return std::vector<DWORD>(width * width, colors[level]);
}

IDirect3DTexture9* makeTexture(IDirect3DDevice9* device, bool colouredMips) {
    IDirect3DTexture9* texture = nullptr;
    need(SUCCEEDED(device->CreateTexture(textureSide, textureSide, 7, 0,
                                         D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                         &texture, nullptr)), "fixture texture");
    for (unsigned level = 0; level < 7; ++level) {
        const unsigned width = std::max(1u, textureSide >> level);
        const auto pixels = colouredMips ? mipColors(level) : smoothPattern(level);
        D3DLOCKED_RECT locked{};
        need(SUCCEEDED(texture->LockRect(level, &locked, nullptr, 0)), "lock fixture mip");
        for (unsigned y = 0; y < width; ++y)
            std::memcpy(static_cast<unsigned char*>(locked.pBits) + y * locked.Pitch,
                        pixels.data() + y * width, width * sizeof(DWORD));
        texture->UnlockRect(level);
    }
    return texture;
}

std::vector<DWORD> sourceMip(IDirect3DTexture9* texture) {
    D3DSURFACE_DESC desc{};
    texture->GetLevelDesc(0, &desc);
    std::vector<DWORD> copy(desc.Width * desc.Height);
    D3DLOCKED_RECT locked{};
    need(SUCCEEDED(texture->LockRect(0, &locked, nullptr, D3DLOCK_READONLY)), "read fixture source");
    for (UINT y = 0; y < desc.Height; ++y)
        std::memcpy(copy.data() + y * desc.Width,
                    static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch,
                    desc.Width * sizeof(DWORD));
    texture->UnlockRect(0);
    return copy;
}

std::vector<DWORD> draw(IDirect3DDevice9* device, IDirect3DTexture9* texture,
                        const char* label, float repeats, float uvOffset) {
    storm_metal_label_texture(texture, label);
    need(SUCCEEDED(device->SetTexture(0, texture)), "bind fixture texture");
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.f, 0);
    const Vertex quad[] = {
        {0.f, 0.f, 0.f, 1.f, uvOffset, uvOffset},
        {float(side), 0.f, 0.f, 1.f, uvOffset + repeats, uvOffset},
        {0.f, float(side), 0.f, 1.f, uvOffset, uvOffset + repeats},
        {float(side), float(side), 0.f, 1.f, uvOffset + repeats, uvOffset + repeats},
    };
    need(SUCCEEDED(device->BeginScene()), "begin fixture scene");
    need(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex))),
         "draw fixture quad");
    need(SUCCEEDED(device->EndScene()), "end fixture scene");
    IDirect3DSurface9 *target = nullptr, *readback = nullptr;
    need(SUCCEEDED(device->GetRenderTarget(0, &target)), "fixture target");
    need(SUCCEEDED(device->CreateOffscreenPlainSurface(side, side, D3DFMT_A8R8G8B8,
                                                        D3DPOOL_SYSTEMMEM, &readback, nullptr)),
         "fixture readback");
    need(SUCCEEDED(device->GetRenderTargetData(target, readback)), "fixture readback copy");
    D3DLOCKED_RECT locked{};
    need(SUCCEEDED(readback->LockRect(&locked, nullptr, D3DLOCK_READONLY)), "lock fixture readback");
    std::vector<DWORD> pixels(side * side);
    for (unsigned y = 0; y < side; ++y)
        std::memcpy(pixels.data() + y * side,
                    static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch,
                    side * sizeof(DWORD));
    readback->UnlockRect();
    readback->Release();
    target->Release();
    return pixels;
}

size_t differences(const std::vector<DWORD>& left, const std::vector<DWORD>& right) {
    size_t total = 0;
    for (size_t index = 0; index < left.size(); ++index) total += left[index] != right[index];
    return total;
}

unsigned rgbDelta(DWORD left, DWORD right) {
    unsigned worst = 0;
    for (unsigned shift : {0u, 8u, 16u}) {
        const int a = int((left >> shift) & 255u), b = int((right >> shift) & 255u);
        worst = std::max(worst, unsigned(std::abs(a - b)));
    }
    return worst;
}

size_t periodicRgbMatches(const std::vector<DWORD>& pixels, unsigned period) {
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
    auto* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    need(d3d != nullptr, "D3D9 factory");
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth = pp.BackBufferHeight = side;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.Windowed = TRUE;
    pp.hDeviceWindow = window;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* device = nullptr;
    need(SUCCEEDED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &pp, &device)), "D3D9 device");
    need(SUCCEEDED(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1)), "fixture FVF");
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

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
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    need(draw(device, floorTexture, "floorU3.tga", 4.f, uvOffset) == draw(device, baselineTexture, "unlabelled.tga", 4.f, uvOffset),
         "explicit U clamp keeps authored sampling");
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    need(draw(device, floorTexture, "floorU3.tga", 4.f, uvOffset) == draw(device, baselineTexture, "unlabelled.tga", 4.f, uvOffset),
         "explicit V clamp keeps authored sampling");
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
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
    d3d->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: anti-tiling labels, exact exclusions, source-byte preservation, cell continuity and mip LOD stability");
}

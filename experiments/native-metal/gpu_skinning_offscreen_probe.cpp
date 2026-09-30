#include "gpu_skinning_bridge.hpp"

#include <SDL.h>
#include <d3d9.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using storm::metal::skinning::AnimatedVertex;
using storm::metal::skinning::Matrix4x4;
extern "C" std::uint64_t StormMetalRawSkinnedDraws(void *);

namespace {
constexpr UINT kWidth = 64;
constexpr UINT kHeight = 64;
constexpr DWORD kClear = 0x7f102030u;
constexpr DWORD kLayout = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

struct CpuVertex {
    float position[3];
    float normal[3];
    std::int32_t color;
    float uv[2];
};
static_assert(sizeof(CpuVertex) == 36);

[[noreturn]] void fail(const char *message)
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

void require(bool condition, const char *message)
{
    if (!condition)
        fail(message);
}

CpuVertex cpuSkin(const AnimatedVertex &vertex, const Matrix4x4 *palette)
{
    const auto first = vertex.packedBones & 0xffu;
    const auto second = (vertex.packedBones >> 8) & 0xffu;
    const float firstWeight = vertex.weight;
    const float secondWeight = 1.0f - firstWeight;
    float transform[16];
    for (int i = 0; i < 16; ++i)
        transform[i] = palette[first].elements[i] * firstWeight + palette[second].elements[i] * secondWeight;
    transform[0] = -transform[0];
    transform[4] = -transform[4];
    transform[8] = -transform[8];
    transform[12] = -transform[12];

    CpuVertex result{};
    for (int component = 0; component < 3; ++component) {
        result.position[component] = vertex.position[0] * transform[component] +
                                     vertex.position[1] * transform[4 + component] +
                                     vertex.position[2] * transform[8 + component] + transform[12 + component];
        result.normal[component] = vertex.normal[0] * transform[component] +
                                   vertex.normal[1] * transform[4 + component] +
                                   vertex.normal[2] * transform[8 + component];
    }
    result.color = vertex.color;
    result.uv[0] = vertex.uv[0];
    result.uv[1] = vertex.uv[1];
    return result;
}

std::vector<DWORD> readPixels(IDirect3DDevice9 *device, IDirect3DSurface9 *target,
                              IDirect3DSurface9 *readback)
{
    require(SUCCEEDED(device->GetRenderTargetData(target, readback)), "GPU readback");
    D3DLOCKED_RECT locked{};
    require(SUCCEEDED(readback->LockRect(&locked, nullptr, D3DLOCK_READONLY)), "readback lock");
    std::vector<DWORD> pixels(kWidth * kHeight);
    for (UINT y = 0; y < kHeight; ++y)
        std::memcpy(pixels.data() + y * kWidth,
                    static_cast<const char *>(locked.pBits) + y * locked.Pitch,
                    kWidth * sizeof(DWORD));
    readback->UnlockRect();
    return pixels;
}

void compare(const std::vector<DWORD> &cpu, const std::vector<DWORD> &gpu, int pose)
{
    require(cpu.size() == gpu.size(), "matching readback sizes");
    std::size_t covered = 0;
    for (std::size_t i = 0; i < cpu.size(); ++i) {
        const bool cpuCoverage = cpu[i] != kClear;
        const bool gpuCoverage = gpu[i] != kClear;
        if (cpuCoverage != gpuCoverage) {
            std::fprintf(stderr, "FAIL: pose %d coverage/depth mismatch at pixel %zu\n", pose, i);
            std::exit(1);
        }
        covered += cpuCoverage;
        for (unsigned shift : {0u, 8u, 16u}) {
            const int a = int((cpu[i] >> shift) & 0xffu);
            const int b = int((gpu[i] >> shift) & 0xffu);
            if (std::abs(a - b) > 1) {
                std::fprintf(stderr, "FAIL: pose %d RGB mismatch at pixel %zu: cpu=%08x gpu=%08x\n",
                             pose, i, cpu[i], gpu[i]);
                std::exit(1);
            }
        }
        if ((cpu[i] >> 24) != (gpu[i] >> 24)) {
            std::fprintf(stderr, "FAIL: pose %d alpha mismatch at pixel %zu\n", pose, i);
            std::exit(1);
        }
    }
    require(covered > 200, "fixture has useful raster coverage");
}
} // namespace

int main()
{
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    SDL_Window *window = SDL_CreateWindow("GPU skinning offscreen fixture", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, kWidth, kHeight,
                                          SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN);
    require(window != nullptr, SDL_GetError());
    IDirect3D9 *api = Direct3DCreate9(D3D_SDK_VERSION);
    require(api != nullptr, "Direct3DCreate9");
    D3DPRESENT_PARAMETERS parameters{};
    parameters.Windowed = TRUE;
    parameters.BackBufferWidth = kWidth;
    parameters.BackBufferHeight = kHeight;
    parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.hDeviceWindow = window;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.EnableAutoDepthStencil = TRUE;
    parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    IDirect3DDevice9 *device = nullptr;
    require(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                        &parameters, &device)) && device,
            "CreateDevice");

    IDirect3DSurface9 *target = nullptr;
    IDirect3DSurface9 *readback = nullptr;
    require(SUCCEEDED(device->GetRenderTarget(0, &target)) && target, "render target");
    require(SUCCEEDED(device->CreateOffscreenPlainSurface(kWidth, kHeight, D3DFMT_A8R8G8B8,
                                                          D3DPOOL_SYSTEMMEM, &readback, nullptr)) && readback,
            "readback surface");

    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    device->SetTransform(D3DTS_WORLD, &identity);
    device->SetTransform(D3DTS_VIEW, &identity);
    device->SetTransform(D3DTS_PROJECTION, &identity);
    device->SetFVF(kLayout);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    const std::array<std::uint16_t, 6> indices{{0, 1, 2, 3, 4, 5}};
    IDirect3DIndexBuffer9 *indexBuffer = nullptr;
    require(SUCCEEDED(device->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED,
                                                 &indexBuffer, nullptr)) && indexBuffer,
            "fixture index buffer");
    void *mappedIndices = nullptr;
    require(SUCCEEDED(indexBuffer->Lock(0, 0, &mappedIndices, 0)) && mappedIndices,
            "fixture index buffer lock");
    std::memcpy(mappedIndices, indices.data(), sizeof(indices));
    indexBuffer->Unlock();
    require(SUCCEEDED(device->SetIndices(indexBuffer)), "bind fixture indices");

    const std::array<AnimatedVertex, 6> vertices{{
        {{0.72f, -0.72f, 0.72f}, 1, 0x00000100u, {0, 0, -1}, std::int32_t(0xd04080c0u), {0, 1}},
        {{-0.72f, -0.72f, 0.72f}, 0, 0x00000100u, {0, 0, -1}, std::int32_t(0xd04080c0u), {1, 1}},
        {{0, 0.72f, 0.72f}, .5f, 0x00000100u, {0, 0, -1}, std::int32_t(0xd04080c0u), {.5f, 0}},
        {{0.50f, -0.50f, 0.28f}, 1, 0x00000100u, {0, 0, -1}, std::int32_t(0x9030c060u), {0, 1}},
        {{-0.50f, -0.50f, 0.28f}, 0, 0x00000100u, {0, 0, -1}, std::int32_t(0x9030c060u), {1, 1}},
        {{0, 0.50f, 0.28f}, .5f, 0x00000100u, {0, 0, -1}, std::int32_t(0x9030c060u), {.5f, 0}},
    }};
    require(storm::metal::skinning::acceptVertices(vertices.data(), vertices.size(), 2),
            "backend accepts valid AVERTEX0 stream");
    auto invalid = vertices;
    invalid[0].packedBones = 0x00000200u;
    require(!storm::metal::skinning::acceptVertices(invalid.data(), invalid.size(), 2),
            "backend rejects out-of-range bone index");

    std::array<std::array<Matrix4x4, 2>, 2> poses{};
    for (auto &pose : poses)
        for (auto &matrix : pose)
            matrix.elements[0] = matrix.elements[5] = matrix.elements[10] = matrix.elements[15] = 1.0f;
    poses[0][0].elements[12] = -.12f;
    poses[0][1].elements[12] = .12f;
    poses[1][0].elements[12] = .18f;
    poses[1][0].elements[13] = -.08f;
    poses[1][1].elements[12] = -.14f;
    poses[1][1].elements[13] = .10f;

    for (std::size_t poseIndex = 0; poseIndex < poses.size(); ++poseIndex) {
        std::array<CpuVertex, 6> cpuVertices{};
        for (std::size_t i = 0; i < vertices.size(); ++i)
            cpuVertices[i] = cpuSkin(vertices[i], poses[poseIndex].data());

        auto draw = [&](IDirect3DVertexBuffer9 *buffer, UINT stride) {
            require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, kClear, 1.0f, 0)),
                    "clear fixture");
            require(SUCCEEDED(device->BeginScene()), "BeginScene");
            require(SUCCEEDED(device->SetStreamSource(0, buffer, 0, stride)), "bind fixture vertices");
            require(SUCCEEDED(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 6, 0, 2)),
                    "draw fixture triangles");
            require(SUCCEEDED(device->EndScene()), "EndScene");
            return readPixels(device, target, readback);
        };

        IDirect3DVertexBuffer9 *cpuBuffer = nullptr;
        require(SUCCEEDED(device->CreateVertexBuffer(sizeof(cpuVertices), 0, kLayout, D3DPOOL_MANAGED,
                                                     &cpuBuffer, nullptr)) && cpuBuffer,
                "CPU oracle vertex buffer");
        void *mapped = nullptr;
        require(SUCCEEDED(cpuBuffer->Lock(0, 0, &mapped, 0)) && mapped, "CPU oracle buffer lock");
        std::memcpy(mapped, cpuVertices.data(), sizeof(cpuVertices));
        cpuBuffer->Unlock();
        const auto cpuPixels = draw(cpuBuffer, sizeof(CpuVertex));
        cpuBuffer->Release();

        require(storm::metal::skinning::beginPose(device, poses[poseIndex].data(), poses[poseIndex].size()),
                "begin GPU pose");
        auto *gpuBuffer = static_cast<IDirect3DVertexBuffer9 *>(storm::metal::skinning::bindVertices(
            vertices.data(), 0, vertices.size(), vertices.size()));
        require(gpuBuffer != nullptr, "bind AVERTEX0 GPU buffer");
        const auto gpuPixels = draw(gpuBuffer, sizeof(CpuVertex));
        require(StormMetalRawSkinnedDraws(device) == poseIndex + 1, "GPU skinning draw counter");
        storm::metal::skinning::endPose();
        compare(cpuPixels, gpuPixels, static_cast<int>(poseIndex));
    }

    device->SetStreamSource(0, nullptr, 0, 0);
    device->SetIndices(nullptr);
    indexBuffer->Release();
    readback->Release();
    target->Release();
    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: two AVERTEX0 GPU poses match the CPU 36-byte oracle in coverage, depth and pixels");
}

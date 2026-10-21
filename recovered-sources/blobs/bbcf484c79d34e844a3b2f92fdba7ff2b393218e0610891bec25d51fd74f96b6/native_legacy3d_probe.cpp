#include <SDL.h>
#include <d3d9.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

static void need(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

extern "C" std::uint64_t StormMetalRawNativeDraws(void *);

template <class Vertex, class Index>
static void exercise(IDirect3DDevice9 *device, IDirect3DSurface9 *target,
                     IDirect3DSurface9 *readback, DWORD fvf,
                     std::vector<Vertex> &vertices, bool dynamic, bool uniqueIndices,
                     D3DPRIMITIVETYPE primitive = D3DPT_TRIANGLELIST)
{
    const std::vector<Index> indices = primitive == D3DPT_TRIANGLESTRIP
        ? std::vector<Index>{2, 3, 4, 5}
        : uniqueIndices
        ? std::vector<Index>{2, 3, 4}
        : std::vector<Index>{2, 3, 4, 4, 3, 5};
    const UINT primitiveCount = primitive == D3DPT_TRIANGLESTRIP ? 2 : uniqueIndices ? 1 : 2;
    auto pixels = [&] {
        need(SUCCEEDED(device->GetRenderTargetData(target, readback)), "readback");
        D3DLOCKED_RECT lock{};
        need(SUCCEEDED(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY)), "readback lock");
        std::vector<DWORD> result(64 * 64);
        for (int y = 0; y < 64; ++y)
            std::memcpy(result.data() + y * 64, static_cast<char *>(lock.pBits) + y * lock.Pitch, 64 * 4);
        readback->UnlockRect();
        return result;
    };
    auto reference = [&] {
        const std::vector<unsigned> expandedIndices = primitive == D3DPT_TRIANGLESTRIP
            ? std::vector<unsigned>{0, 1, 2, 3}
            : uniqueIndices
            ? std::vector<unsigned>{0, 1, 2}
            : std::vector<unsigned>{0, 1, 2, 2, 1, 3};
        std::vector<Vertex> expanded;
        for (unsigned index : expandedIndices)
            expanded.push_back(vertices[index]);
        unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
        device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff102030, 1, 0);
        device->BeginScene();
        need(SUCCEEDED(device->DrawPrimitiveUP(primitive, primitiveCount, expanded.data(), sizeof(Vertex))),
             "compatibility reference");
        device->EndScene();
        return pixels();
    };

    IDirect3DVertexBuffer9 *vb = nullptr;
    IDirect3DIndexBuffer9 *ib = nullptr;
    const DWORD usage = dynamic ? D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY : 0;
    const D3DPOOL pool = dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
    need(SUCCEEDED(device->CreateVertexBuffer(UINT(vertices.size() * sizeof(Vertex)), usage, fvf, pool, &vb, nullptr)), "VB");
    need(SUCCEEDED(device->CreateIndexBuffer(UINT(indices.size() * sizeof(Index)), usage, sizeof(Index) == 4 ? D3DFMT_INDEX32 : D3DFMT_INDEX16, pool, &ib, nullptr)), "IB");
    auto upload = [&] {
        void *bytes = nullptr;
        need(SUCCEEDED(vb->Lock(0, 0, &bytes, dynamic ? D3DLOCK_DISCARD : 0)), "VB lock");
        std::memcpy(bytes, vertices.data(), vertices.size() * sizeof(Vertex));
        need(SUCCEEDED(vb->Unlock()), "VB unlock");
        need(SUCCEEDED(ib->Lock(0, 0, &bytes, dynamic ? D3DLOCK_DISCARD : 0)), "IB lock");
        std::memcpy(bytes, indices.data(), indices.size() * sizeof(Index));
        need(SUCCEEDED(ib->Unlock()), "IB unlock");
    };
    upload();
    device->SetStreamSource(0, vb, 0, sizeof(Vertex));
    device->SetIndices(ib);

    auto drawNative = [&](const std::vector<DWORD> &expected) {
        const auto before = StormMetalRawNativeDraws(device);
        setenv("STORM_METAL_REQUIRE_NATIVE_3D", "1", 1);
        device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff102030, 1, 0);
        device->BeginScene();
        need(SUCCEEDED(device->DrawIndexedPrimitive(primitive, -2, 2, primitive == D3DPT_TRIANGLESTRIP ? 4 : uniqueIndices ? 3 : 4, 0, primitiveCount)),
             "validated indexed FVF stays native");
        device->EndScene();
        need(StormMetalRawNativeDraws(device) == before + 1,
             "draw is accounted by the raw native submission owner");
        need(pixels() == expected, "native raw FVF pixels match compatibility lighting");
    };
    drawNative(reference());
    if (dynamic) {
        vertices[0].x += .08f;
        upload();
        drawNative(reference());
    }

    DWORD lighting = FALSE;
    device->GetRenderState(D3DRS_LIGHTING, &lighting);
    setenv("STORM_METAL_REQUIRE_NATIVE_3D", "1", 1);
    if (lighting)
        need(FAILED(device->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN, -2, 2, 4, 0, 2)),
             "lit triangle fan remains an explicit conversion boundary");
    unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
    device->SetIndices(nullptr);
    device->SetStreamSource(0, nullptr, 0, 0);
    ib->Release();
    vb->Release();
}

int main()
{
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Native legacy-3D coverage", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());
    auto *api = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS present{};
    present.BackBufferWidth = present.BackBufferHeight = 64;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    present.BackBufferCount = 1;
    present.Windowed = TRUE;
    present.hDeviceWindow = window;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9 *device = nullptr;
    need(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &present, &device)), "device");

    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1;
    device->SetTransform(D3DTS_WORLD, &identity);
    device->SetTransform(D3DTS_VIEW, &identity);
    device->SetTransform(D3DTS_PROJECTION, &identity);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetRenderState(D3DRS_AMBIENT, 0xff303030);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    D3DMATERIAL9 material{};
    material.Diffuse = {.8f, .65f, .5f, 1};
    material.Ambient = {.3f, .25f, .2f, 1};
    device->SetMaterial(&material);
    D3DLIGHT9 light{};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Direction = {0, 0, 1};
    light.Diffuse = {.7f, .8f, .9f, 1};
    device->SetLight(0, &light);
    device->LightEnable(0, TRUE);

    IDirect3DSurface9 *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(0, &target);
    device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, nullptr);

    struct RichVertex { float x, y, z, nx, ny, nz; DWORD diffuse, specular; float u0, v0, u1, v1; };
    std::vector<RichVertex> rich = {
        {-.8f, -.8f, .5f, 0, 0, -1, 0xffff3030, 0xff102030, 0, 0, 0, 0},
        { .8f, -.8f, .5f, 0, 0, -1, 0xff30ff30, 0xff203040, 1, 0, 1, 0},
        {-.8f,  .8f, .5f, 0, 0, -1, 0xff3030ff, 0xff304050, 0, 1, 0, 1},
        { .8f,  .8f, .5f, 0, 0, -1, 0xffeeeeee, 0xff405060, 1, 1, 1, 1},
    };
    device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2);
    exercise<RichVertex, std::uint16_t>(device, target, readback,
        D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, rich, false, false);

    struct CompactVertex { float x, y, z, nx, ny, nz; };
    std::vector<CompactVertex> compact = {
        {-.8f, -.8f, .5f, 0, 0, -1}, { .8f, -.8f, .5f, 0, 0, -1},
        {-.8f,  .8f, .5f, 0, 0, -1}, { .8f,  .8f, .5f, 0, 0, -1},
    };
    device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL);
    exercise<CompactVertex, std::uint32_t>(device, target, readback,
        D3DFVF_XYZ | D3DFVF_NORMAL, compact, true, true);

    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2);
    exercise<RichVertex, std::uint32_t>(device, target, readback,
        D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2,
        rich, true, true, D3DPT_TRIANGLESTRIP);

    std::puts("PASS: lit/unlit static/dynamic 16/32-bit indexed list/strip XYZ/NORMAL FVF draws stay native; rich attributes match; fan rejects native-only mode");
    readback->Release();
    target->Release();
    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

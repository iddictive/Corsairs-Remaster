#include <SDL.h>
#include <d3d9.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

extern "C" unsigned StormMetalBeginWdmModel(void *, unsigned);
extern "C" void StormMetalEndWdmModel(void *, unsigned);
extern "C" uint64_t StormMetalRawWorldDraws(void *);

static void need(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

template <class Vertex, class Index>
static void exercise(IDirect3DDevice9 *device, IDirect3DSurface9 *target,
                     IDirect3DSurface9 *readback, DWORD fvf,
                     std::vector<Vertex> &vertices, bool dynamic, bool uniqueIndices,
                     D3DPRIMITIVETYPE primitive = D3DPT_TRIANGLELIST)
{
    const std::vector<Index> indices = primitive == D3DPT_TRIANGLEFAN
        ? std::vector<Index>{2, 3, 5, 4}
        : primitive == D3DPT_TRIANGLESTRIP
        ? std::vector<Index>{2, 3, 4, 5}
        : uniqueIndices
        ? std::vector<Index>{2, 3, 4}
        : std::vector<Index>{2, 3, 4, 4, 3, 5};
    const UINT primitiveCount = primitive == D3DPT_TRIANGLESTRIP ||
                                primitive == D3DPT_TRIANGLEFAN ? 2 :
                                uniqueIndices ? 1 : 2;
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
        const std::vector<unsigned> expandedIndices = primitive == D3DPT_TRIANGLEFAN
            ? std::vector<unsigned>{0, 1, 3, 2}
            : primitive == D3DPT_TRIANGLESTRIP
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
        setenv("STORM_METAL_REQUIRE_NATIVE_3D", "1", 1);
        device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff102030, 1, 0);
        device->BeginScene();
        const HRESULT drawResult = device->DrawIndexedPrimitive(
            primitive, -2, 2,
            primitive == D3DPT_TRIANGLESTRIP || primitive == D3DPT_TRIANGLEFAN
                ? 4 : uniqueIndices ? 3 : 4,
            0, primitiveCount);
        if (FAILED(drawResult))
            std::fprintf(stderr, "native draw failed: 0x%08x primitive=%u\n",
                         unsigned(drawResult), unsigned(primitive));
        need(SUCCEEDED(drawResult), "validated indexed FVF stays native");
        device->EndScene();
        const auto actual = pixels();
        std::size_t coverageMismatch = 0, colorMismatch = 0;
        unsigned maxChannelDelta = 0;
        for (std::size_t i = 0; i < actual.size(); ++i) {
            const bool expectedCovered = expected[i] != 0xff102030;
            const bool actualCovered = actual[i] != 0xff102030;
            coverageMismatch += expectedCovered != actualCovered;
            if (expected[i] != actual[i]) ++colorMismatch;
            for (unsigned shift = 0; shift < 32; shift += 8) {
                const int delta = int((expected[i] >> shift) & 0xff) -
                                  int((actual[i] >> shift) & 0xff);
                maxChannelDelta = std::max(maxChannelDelta, unsigned(std::abs(delta)));
            }
        }
        if (coverageMismatch || maxChannelDelta > 1)
            std::fprintf(stderr,
                         "primitive=%u dynamic=%d unique=%d coverage=%zu colors=%zu maxDelta=%u\n",
                         unsigned(primitive), dynamic, uniqueIndices, coverageMismatch,
                         colorMismatch, maxChannelDelta);
        need(coverageMismatch == 0, "native raw FVF geometry matches compatibility coverage");
        need(maxChannelDelta <= 1, "native raw FVF lighting matches compatibility within one channel step");
    };
    drawNative(reference());
    if (dynamic) {
        vertices[0].x += .08f;
        upload();
        drawNative(reference());
    }

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
    exercise<RichVertex, std::uint32_t>(device, target, readback,
        D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2,
        rich, false, false, D3DPT_TRIANGLEFAN);

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

    // The authored map is unlit. Opening a model role must not inject stale
    // scene lights, alter vertex alpha or leak the role into the next draw.
    const DWORD mapFVF=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2;
    device->SetFVF(mapFVF);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    for(auto &v:rich)v.diffuse=(v.diffuse&0xffffff)|0x80000000;
    const WORD mapIndices[]={0,1,2,2,1,3};
    IDirect3DVertexBuffer9 *mapVB=nullptr; IDirect3DIndexBuffer9 *mapIB=nullptr;
    need(SUCCEEDED(device->CreateVertexBuffer(UINT(rich.size()*sizeof(RichVertex)),0,mapFVF,D3DPOOL_MANAGED,&mapVB,nullptr)),"map VB");
    need(SUCCEEDED(device->CreateIndexBuffer(sizeof(mapIndices),0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&mapIB,nullptr)),"map IB");
    void *bytes=nullptr; mapVB->Lock(0,0,&bytes,0); std::memcpy(bytes,rich.data(),rich.size()*sizeof(RichVertex));mapVB->Unlock();
    mapIB->Lock(0,0,&bytes,0);std::memcpy(bytes,mapIndices,sizeof(mapIndices));mapIB->Unlock();
    device->SetStreamSource(0,mapVB,0,sizeof(RichVertex));device->SetIndices(mapIB);
    auto mapPixels=[&] {
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0);device->BeginScene();
        need(SUCCEEDED(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"map draw");device->EndScene();
        need(SUCCEEDED(device->GetRenderTargetData(target,readback)),"map readback");D3DLOCKED_RECT r{};
        need(SUCCEEDED(readback->LockRect(&r,nullptr,D3DLOCK_READONLY)),"map readback lock");
        std::vector<DWORD> result(64*64);for(int y=0;y<64;y++)std::memcpy(result.data()+y*64,static_cast<char*>(r.pBits)+y*r.Pitch,64*4);
        readback->UnlockRect();return result;
    };
    device->SetRenderState(D3DRS_SPECULARENABLE,TRUE);
    const auto mapReference=mapPixels();const auto worldBefore=StormMetalRawWorldDraws(device);
    const unsigned previous=StormMetalBeginWdmModel(device,1);need(previous==0,"map scope starts empty");
    const auto mapActual=mapPixels();
    need(StormMetalRawWorldDraws(device)==worldBefore+1,"model role reaches resident map path");
    need(mapActual==mapReference,"unlit map preserves full color and alpha image");
    const auto nested=StormMetalBeginWdmModel(device,2);need(nested==1,"nested map role");
    StormMetalEndWdmModel(device,nested);need(StormMetalBeginWdmModel(device,1)==1,"nested role restored");
    StormMetalEndWdmModel(device,previous);need(mapPixels()==mapReference,"outside map scope unchanged");
    need(StormMetalRawWorldDraws(device)==worldBefore+1,"map role does not leak");
    device->SetStreamSource(0,nullptr,0,0);device->SetIndices(nullptr);mapVB->Release();mapIB->Release();

    std::puts("PASS: lit/unlit static/dynamic 16/32-bit indexed list/strip/fan XYZ/NORMAL FVF draws stay native; signed base and rich attributes match");
    readback->Release();
    target->Release();
    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

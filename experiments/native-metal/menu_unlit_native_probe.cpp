#include <SDL.h>
#include <d3d9.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>

extern "C" bool StormMetalDrawCompactPrimitive(void *, uint32_t, uint32_t, uint32_t, const void *, uint32_t);
extern "C" bool StormMetalDrawCompactIndexedPrimitive(void *, uint32_t, uint32_t, uint32_t, uint32_t, const void *, uint32_t, const void *, uint32_t);
extern "C" uint64_t StormMetalNativeUnlitDraws(void *);
extern "C" uint64_t StormMetalNativeUnlitVertices(void *);
extern "C" uint64_t StormMetalLegacyUnlitCpuDraws(void *);
extern "C" uint64_t StormMetalLegacyUnlitCpuVertices(void *);

static void need(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Menu unlit native fixture", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());
    auto *api = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth = pp.BackBufferHeight = 64;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.Windowed = TRUE;
    pp.hDeviceWindow = window;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9 *device = nullptr;
    need(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &pp, &device)), "device");

    struct Vertex { float x, y, z, rhw; DWORD color; float u, v; };
    const DWORD fvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    Vertex up[] = {{2, 2, .5f, 1, 0xffff2020, 0, 0}, {30, 2, .5f, 1, 0xffff2020, 1, 0},
                   {2, 62, .5f, 1, 0xffff2020, 0, 1}};
    Vertex dynamic[] = {{34, 2, .5f, 1, 0xff20ff20, 0, 0}, {62, 2, .5f, 1, 0xff20ff20, 1, 0},
                        {62, 62, .5f, 1, 0xff20ff20, 1, 1}};
    IDirect3DVertexBuffer9 *vb = nullptr;
    need(SUCCEEDED(device->CreateVertexBuffer(sizeof(dynamic), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                               fvf, D3DPOOL_DEFAULT, &vb, nullptr)), "dynamic menu VB");
    void *mapped = nullptr;
    need(SUCCEEDED(vb->Lock(0, 0, &mapped, D3DLOCK_DISCARD)), "dynamic menu VB lock");
    std::memcpy(mapped, dynamic, sizeof(dynamic));
    need(SUCCEEDED(vb->Unlock()), "dynamic menu VB unlock");

    device->SetFVF(fvf);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff101010, 1, 0);
    device->BeginScene();
    need(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, up, sizeof(Vertex))), "menu UP draw");
    device->SetStreamSource(0, vb, 0, sizeof(Vertex));
    need(SUCCEEDED(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1)), "menu dynamic VB draw");
    device->EndScene();

    need(StormMetalNativeUnlitDraws(device) == 2, "both menu draws use native compact submission");
    need(StormMetalNativeUnlitVertices(device) == 6, "native compact vertex accounting is exact");
    need(StormMetalLegacyUnlitCpuDraws(device) == 0 && StormMetalLegacyUnlitCpuVertices(device) == 0,
         "menu has zero CPU FFP compatibility conversion");

    IDirect3DSurface9 *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(0, &target);
    device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, nullptr);
    need(SUCCEEDED(device->GetRenderTargetData(target, readback)), "readback");
    D3DLOCKED_RECT lock{};
    need(SUCCEEDED(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY)), "readback lock");
    DWORD left = 0, right = 0;
    std::memcpy(&left, static_cast<char *>(lock.pBits) + 16 * lock.Pitch + 8 * 4, 4);
    std::memcpy(&right, static_cast<char *>(lock.pBits) + 16 * lock.Pitch + 54 * 4, 4);
    readback->UnlockRect();
    need(left == 0xffff2020, "UP authored color preserved");
    need(right == 0xff20ff20, "dynamic VB authored color preserved");

    // Compare complete images against a triangle-list oracle, including sparse
    // indexed fans and a negative base vertex. No CPU vertex expansion is allowed.
    Vertex fan[] = {{2,2,.5f,1,0xffff2020,0,0}, {62,2,.5f,1,0xff20ff20,1,0},
                    {62,62,.5f,1,0xff2020ff,1,1}, {2,62,.5f,1,0xffffffff,0,1}};
    Vertex triangles[] = {fan[0],fan[1],fan[2],fan[0],fan[2],fan[3]};
    auto pixels = [&] {
        need(SUCCEEDED(device->GetRenderTargetData(target, readback)), "fan readback");
        D3DLOCKED_RECT r{};
        need(SUCCEEDED(readback->LockRect(&r,nullptr,D3DLOCK_READONLY)), "fan lock");
        std::vector<DWORD> result(64*64);
        for(int y=0;y<64;y++) std::memcpy(result.data()+y*64,static_cast<char*>(r.pBits)+y*r.Pitch,64*4);
        readback->UnlockRect(); return result;
    };
    auto begin = [&] { device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff101010,1,0); device->BeginScene(); };
    begin();
    need(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,triangles,sizeof(Vertex))), "fan oracle");
    device->EndScene(); const auto expected=pixels();
    const auto cpuBefore=StormMetalLegacyUnlitCpuDraws(device);
    const WORD compactIndices[]={0,1,2,3};
    for(int route=0;route<6;route++) {
        IDirect3DVertexBuffer9 *fanVB=nullptr; IDirect3DIndexBuffer9 *fanIB=nullptr;
        if(route>=3) {
            const DWORD usage=route==4?D3DUSAGE_DYNAMIC:0;
            need(SUCCEEDED(device->CreateVertexBuffer(sizeof(fan),usage,fvf,D3DPOOL_DEFAULT,&fanVB,nullptr)), "fan VB");
            need(SUCCEEDED(fanVB->Lock(0,0,&mapped,usage?D3DLOCK_DISCARD:0)), "fan VB lock");
            std::memcpy(mapped,fan,sizeof(fan)); fanVB->Unlock();
            device->SetStreamSource(0,fanVB,0,sizeof(Vertex));
            if(route==5) {
                const DWORD ix[]={99,2,3,4,5};
                need(SUCCEEDED(device->CreateIndexBuffer(sizeof(ix),0,D3DFMT_INDEX32,D3DPOOL_DEFAULT,&fanIB,nullptr)), "fan IB");
                need(SUCCEEDED(fanIB->Lock(0,0,&mapped,0)), "fan IB lock");
                std::memcpy(mapped,ix,sizeof(ix)); fanIB->Unlock(); device->SetIndices(fanIB);
            }
        }
        begin();
        bool ok=false;
        if(route==0) ok=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,fan,sizeof(Vertex)));
        if(route==1) ok=StormMetalDrawCompactPrimitive(device,D3DPT_TRIANGLEFAN,fvf,2,fan,sizeof(Vertex));
        if(route==2) ok=StormMetalDrawCompactIndexedPrimitive(device,D3DPT_TRIANGLEFAN,0,4,2,compactIndices,D3DFMT_INDEX16,fan,sizeof(Vertex));
        if(route==3||route==4) ok=SUCCEEDED(device->DrawPrimitive(D3DPT_TRIANGLEFAN,0,2));
        if(route==5) ok=SUCCEEDED(device->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN,-2,2,4,1,2));
        need(ok,"fan submission"); device->EndScene();
        need(pixels()==expected,"fan pixels match triangle-list oracle");
        need(StormMetalLegacyUnlitCpuDraws(device)==cpuBefore,"fan avoids CPU vertex expansion");
        device->SetStreamSource(0,nullptr,0,0); device->SetIndices(nullptr);
        if(fanVB) fanVB->Release(); if(fanIB) fanIB->Release();
    }
    need(!StormMetalDrawCompactPrimitive(device,D3DPT_TRIANGLEFAN,fvf,2,fan,4),"short fan stride rejected");

    target->Release();
    readback->Release();
    vb->Release();
    device->Release();
    api->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: menu and six fan submission routes preserve pixels with zero CPU vertex expansion");
}

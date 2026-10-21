#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>

static void require(bool value, const char *what) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}

int main() {
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Storm Metal API probe", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_METAL);
    require(window, SDL_GetError());
    auto *api = Direct3DCreate9(D3D_SDK_VERSION);
    require(api, "Direct3DCreate9");
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth = 640; pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.BackBufferCount = 1;
    pp.Windowed = TRUE; pp.hDeviceWindow = window;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.EnableAutoDepthStencil = TRUE; pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    IDirect3DDevice9 *device = nullptr;
    require(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device)) && device, "CreateDevice");
    IDirect3DVertexBuffer9 *buffer = nullptr;
    require(SUCCEEDED(device->CreateVertexBuffer(256, 0, D3DFVF_XYZ,
        D3DPOOL_MANAGED, &buffer, nullptr)) && buffer, "CreateVertexBuffer");
    void *bytes = nullptr;
    require(SUCCEEDED(buffer->Lock(0, 256, &bytes, 0)) && bytes, "buffer Lock");
    static_cast<unsigned char *>(bytes)[255] = 0x7b;
    require(SUCCEEDED(buffer->Unlock()), "buffer Unlock");
    require(SUCCEEDED(buffer->Lock(255, 1, &bytes, D3DLOCK_READONLY)) &&
        *static_cast<unsigned char *>(bytes) == 0x7b, "buffer round trip");
    buffer->Unlock();
    require(FAILED(buffer->Lock(255, 2, &bytes, 0)), "reject out of range Lock");
    buffer->Release();
    struct Vertex { float x, y, z, rhw; DWORD color; };
    const Vertex vertices[] = {{80,400,0.5f,1,0xffff0000},
        {320,60,0.5f,1,0xff00ff00}, {560,400,0.5f,1,0xff0000ff}};
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    for (int frame = 0; frame < 120; ++frame) {
        SDL_Event event; while (SDL_PollEvent(&event)) {}
        require(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            0xff183040, 1.0f, 0)), "Clear");
        require(SUCCEEDED(device->BeginScene()), "BeginScene");
        require(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1,
            vertices, sizeof(Vertex))), "DrawPrimitiveUP");
        require(SUCCEEDED(device->EndScene()), "EndScene");
        require(SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)), "Present");
    }
    IDirect3DSurface9 *target = nullptr, *readback = nullptr;
    require(SUCCEEDED(device->GetRenderTarget(0, &target)) && target, "GetRenderTarget");
    require(SUCCEEDED(device->CreateOffscreenPlainSurface(640, 480,
        D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, nullptr)) && readback,
        "CreateOffscreenPlainSurface");
    require(SUCCEEDED(device->GetRenderTargetData(target, readback)), "GPU readback");
    D3DLOCKED_RECT pixels{};
    require(SUCCEEDED(readback->LockRect(&pixels, nullptr, D3DLOCK_READONLY)), "readback LockRect");
    auto *outside = static_cast<unsigned char *>(pixels.pBits) + 10 * pixels.Pitch + 10 * 4;
    require(outside[0] == 0x40 && outside[1] == 0x30 && outside[2] == 0x18,
        "GPU clear preserves BGRA channels");
    auto *inside = static_cast<unsigned char *>(pixels.pBits) + 240 * pixels.Pitch + 320 * 4;
    require(inside[1] > 60 && inside[0] > 20 && inside[2] > 20,
        "GPU triangle interpolates vertex colors");
    readback->UnlockRect(); readback->Release(); target->Release();
    device->Release(); api->Release();
    SDL_DestroyWindow(window); SDL_Quit();
    std::puts("PASS: resource bounds, CPU round trip, GPU clear channels and triangle pixels, present");
}

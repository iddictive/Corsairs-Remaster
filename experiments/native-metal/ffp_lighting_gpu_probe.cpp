#include <SDL.h>
#include <d3d9.h>

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

static DWORD draw(IDirect3DDevice9* device, IDirect3DSurface9* target,
                  IDirect3DSurface9* readback, D3DLIGHT9 light,
                  bool specular, bool localViewer, float power = 24.f,
                  float materialSpecular = 1.f) {
    D3DMATERIAL9 material{};
    material.Diffuse = {.25f, .25f, .25f, 1.f};
    material.Specular = {materialSpecular, materialSpecular * .25f,
                         materialSpecular * .125f, 1.f};
    material.Power = power;
    device->SetMaterial(&material);
    device->SetLight(0, &light);
    device->LightEnable(0, TRUE);
    device->SetRenderState(D3DRS_SPECULARENABLE, specular);
    device->SetRenderState(D3DRS_LOCALVIEWER, localViewer);
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff000000, 1.f, 0);
    const Vertex quad[] = {
        {-.05f,  .05f, .5f, 0.f, 0.f, -1.f},
        { .05f,  .05f, .5f, 0.f, 0.f, -1.f},
        {-.05f, -.05f, .5f, 0.f, 0.f, -1.f},
        { .05f, -.05f, .5f, 0.f, 0.f, -1.f},
    };
    device->BeginScene();
    const HRESULT result = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
    device->EndScene();
    need(SUCCEEDED(result), "GPU fixture draw");
    need(SUCCEEDED(device->GetRenderTargetData(target, readback)), "GPU fixture readback");
    D3DLOCKED_RECT lock{};
    readback->LockRect(&lock, nullptr, D3DLOCK_READONLY);
    DWORD pixel = 0;
    std::memcpy(&pixel, static_cast<const char*>(lock.pBits) + 16 * lock.Pitch + 16 * 4, 4);
    readback->UnlockRect();
    return pixel;
}

static int red(DWORD value) { return int((value >> 16) & 255); }

int main() {
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto* window = SDL_CreateWindow("FFP lighting parity", 0, 0, 32, 32,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());
    auto* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth = pp.BackBufferHeight = 32;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.Windowed = TRUE;
    pp.hDeviceWindow = window;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* device = nullptr;
    need(SUCCEEDED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, window, 0, &pp, &device)), "device");
    need(SUCCEEDED(device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL)), "normal-bearing vertex format");
    // FFP evaluates lighting at vertices, then interpolates. Keep all four
    // vertices near the light axis and magnify only their projected footprint.
    D3DMATRIX projection{};
    projection._11 = projection._22 = 20.f;
    projection._33 = projection._44 = 1.f;
    device->SetTransform(D3DTS_PROJECTION, &projection);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetRenderState(D3DRS_COLORVERTEX, FALSE);
    device->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    device->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    IDirect3DSurface9 *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(0, &target);
    device->CreateOffscreenPlainSurface(32, 32, D3DFMT_A8R8G8B8,
                                        D3DPOOL_SYSTEMMEM, &readback, nullptr);

    D3DLIGHT9 directional{};
    directional.Type = D3DLIGHT_DIRECTIONAL;
    directional.Direction = {0.f, 0.f, 1.f};
    directional.Diffuse = {.1f, .1f, .1f, 1.f};
    directional.Specular = {1.f, 1.f, 1.f, 1.f};
    const DWORD noSpecular = draw(device, target, readback, directional, false, true);
    const DWORD localSpecular = draw(device, target, readback, directional, true, true);
    const DWORD infiniteViewer = draw(device, target, readback, directional, true, false);
    need(red(localSpecular) > red(noSpecular) + 80,
         "material/light specular and Power reach the GPU when enabled");
    need(red(infiniteViewer) + 40 < red(localSpecular),
         "LOCALVIEWER changes the halfway vector");
    D3DLIGHT9 noLightSpecular = directional;
    noLightSpecular.Specular = {0.f, 0.f, 0.f, 1.f};
    const DWORD zeroLightSpecular = draw(device, target, readback, noLightSpecular, true, true);
    const DWORD zeroMaterialSpecular = draw(device, target, readback, directional, true, true, 24.f, 0.f);
    need(red(localSpecular) > red(zeroLightSpecular) + 80,
         "D3DLIGHT9 Specular is the highlight energy owner");
    need(red(localSpecular) > red(zeroMaterialSpecular) + 80,
         "D3DMATERIAL9 Specular is the highlight reflectance owner");
    directional.Direction = {.45f, 0.f, 1.f};
    const DWORD broadHighlight = draw(device, target, readback, directional, true, true, 2.f);
    const DWORD narrowHighlight = draw(device, target, readback, directional, true, true, 64.f);
    need(red(broadHighlight) > red(narrowHighlight) + 20,
         "material Power narrows an off-axis GPU highlight");

    D3DLIGHT9 point{};
    point.Type = D3DLIGHT_POINT;
    point.Position = {0.f, 0.f, -1.f};
    point.Diffuse = {1.f, 1.f, 1.f, 1.f};
    point.Range = 2.f;
    point.Attenuation0 = 1.f;
    const DWORD insidePoint = draw(device, target, readback, point, false, true);
    point.Range = 1.f;
    const DWORD outsidePoint = draw(device, target, readback, point, false, true);
    need(red(insidePoint) > red(outsidePoint) + 40,
         "point light keeps its hard D3D Range cutoff");

    D3DLIGHT9 spot{};
    spot.Type = D3DLIGHT_SPOT;
    spot.Position = {0.f, 0.f, -1.f};
    spot.Direction = {0.f, 0.f, 1.f};
    spot.Diffuse = {1.f, 1.f, 1.f, 1.f};
    spot.Range = 10.f;
    spot.Attenuation0 = 1.f;
    spot.Theta = .25f;
    spot.Phi = .5f;
    spot.Falloff = 1.f;
    const DWORD insideSpot = draw(device, target, readback, spot, false, true);
    spot.Direction = {1.f, 0.f, 0.f};
    const DWORD outsideSpot = draw(device, target, readback, spot, false, true);
    need(red(insideSpot) > red(outsideSpot) + 40,
         "spot theta/phi/falloff reject a receiver outside the cone");

    target->Release();
    readback->Release();
    device->Release();
    d3d->Release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: GPU D3D9 material, light, specular/local-viewer and spot semantics");
}

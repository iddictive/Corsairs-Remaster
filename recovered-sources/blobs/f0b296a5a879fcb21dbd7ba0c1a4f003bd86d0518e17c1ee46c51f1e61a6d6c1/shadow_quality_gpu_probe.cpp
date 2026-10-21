#include <SDL.h>
#include <d3d9.h>

#include <cstdio>
#include <cstdlib>

extern "C" void StormMetalLandLocation(void*, bool);
extern "C" bool StormMetalBeginShadowFrame(void*, uint64_t, uint64_t, const float*);
extern "C" bool StormMetalBeginShadowPass(void*, unsigned, unsigned, uint64_t, const float*, float, float*, float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" unsigned StormMetalSunShadowResolution(void*);
extern "C" unsigned StormMetalPointShadowResolution(void*);

static void need(bool value, const char* label) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", label); std::exit(1); }
}

static void verify(const char* tier, unsigned sunExpected, unsigned pointExpected) {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    setenv("STORM_METAL_SHADOW_QUALITY", tier, 1);
    auto* window = SDL_CreateWindow("shadow quality", 0, 0, 32, 32, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window, "window");
    auto* api = Direct3DCreate9(D3D_SDK_VERSION); need(api, "D3D9");
    D3DPRESENT_PARAMETERS pp{}; pp.Windowed=TRUE; pp.BackBufferWidth=pp.BackBufferHeight=32;
    pp.BackBufferFormat=D3DFMT_A8R8G8B8; pp.hDeviceWindow=window; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* device=nullptr;
    need(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,0,&pp,&device)), "device");
    StormMetalLandLocation(device,true);
    const float focus[]={0,0,8}, sun[]={.6f,0,1}, point[]={0,2,0};
    float view[16]{}, projection[16]{};
    need(StormMetalBeginShadowFrame(device,100+unsigned(tier[0]),1,focus), "frame");
    need(StormMetalBeginShadowPass(device,0,0,1,sun,64,view,projection), "sun pass");
    need(StormMetalSunShadowResolution(device)==sunExpected, "sun allocation");
    StormMetalEndShadowPass(device);
    need(StormMetalBeginShadowPass(device,1,0,2,point,16,view,projection), "point pass");
    need(StormMetalPointShadowResolution(device)==pointExpected, "point allocation");
    StormMetalEndShadowPass(device);
    device->Present(nullptr,nullptr,nullptr,nullptr);
    device->Release(); api->Release(); SDL_DestroyWindow(window);
}

int main() {
    need(SDL_Init(SDL_INIT_VIDEO)==0, "SDL");
    verify("0",1024,256);
    verify("1",2048,512);
    verify("2",4096,512);
    SDL_Quit();
    std::puts("PASS shadow quality allocates all three reviewed Metal profiles");
}

#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" void StormMetalSetLightIdentity(void*, unsigned, uint64_t);
extern "C" uint64_t StormMetalRawWorldDraws(void*);
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*);
extern "C" uint64_t StormMetalPointShadowIndirectFaceSubmissions(void*);

static void need(bool ok, const char* label) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); } }
struct Vertex { float x,y,z,nx,ny,nz; DWORD color; float u,v; };
struct FramePixels { DWORD red{},blue{}; };

int main() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto* window=SDL_CreateWindow("Sea point shadows",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL); need(window,"window");
    auto* d3d=Direct3DCreate9(D3D_SDK_VERSION); D3DPRESENT_PARAMETERS pp{}; pp.Windowed=TRUE;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* d=nullptr; need(SUCCEEDED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,0,&pp,&d)),"device");
    IDirect3DSurface9 *target=nullptr,*read=nullptr; d->GetRenderTarget(0,&target); need(SUCCEEDED(d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)),"readback");
    const WORD ix[]={0,1,2,0,2,3}; IDirect3DVertexBuffer9* meshVb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
    need(SUCCEEDED(d->CreateVertexBuffer(sizeof(Vertex)*4,D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,&meshVb,nullptr)),"shared mesh VB");need(SUCCEEDED(d->CreateIndexBuffer(sizeof(ix),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)),"IB");void* p=nullptr;ib->Lock(0,0,&p,0);std::memcpy(p,ix,sizeof(ix));ib->Unlock();Vertex q[]={{-1,-1,0,0,0,-1,0xffffffff,0,0},{1,-1,0,0,0,-1,0xffffffff,1,0},{1,1,0,0,0,-1,0xffffffff,1,1},{-1,1,0,0,0,-1,0xffffffff,0,1}};meshVb->Lock(0,0,&p,0);std::memcpy(p,q,sizeof(q));meshVb->Unlock();
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&identity);d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetIndices(ib);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,FALSE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);D3DMATERIAL9 material{};material.Diffuse={1,1,1,1};d->SetMaterial(&material);
    auto frame=[&](bool blocker,bool red,bool blue){
        d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
        D3DLIGHT9 redLamp{};redLamp.Type=D3DLIGHT_POINT;redLamp.Position={-.6f,0,-.4f};redLamp.Diffuse={1,0,0,1};redLamp.Range=3;redLamp.Attenuation0=1;d->SetLight(1,&redLamp);d->LightEnable(1,red);StormMetalSetLightIdentity(d,1,71);
        D3DLIGHT9 blueLamp=redLamp;blueLamp.Position={.6f,0,-.4f};blueLamp.Diffuse={0,0,1,1};d->SetLight(2,&blueLamp);d->LightEnable(2,blue);StormMetalSetLightIdentity(d,2,72);d->LightEnable(0,FALSE);
        auto draw=[&](float x,float half,float z){D3DMATRIX world=identity;world._11=world._22=half;world._41=x;world._43=z;d->SetTransform(D3DTS_WORLD,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"raw sea geometry");};
        unsigned scope=StormMetalBeginSceneModel(d);if(blocker&&red)draw(-.1f,.14f,.1f);if(blocker&&blue)draw(.05f,.14f,.1f);draw(0,.9f,.6f);StormMetalEndLandModel(d,scope);D3DPERF_SetMarker(0,L"Storm.SceneHudBoundary");d->EndScene();
        need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT lock{};read->LockRect(&lock,nullptr,D3DLOCK_READONLY);FramePixels value{};std::memcpy(&value.red,static_cast<char*>(lock.pBits)+32*lock.Pitch+48*4,4);std::memcpy(&value.blue,static_cast<char*>(lock.pBits)+32*lock.Pitch+16*4,4);read->UnlockRect();need(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"frame reset");return value;
    };
    const auto before=StormMetalRawWorldDraws(d);const auto redLit=frame(false,true,false),redShadow=frame(true,true,false),blueLit=frame(false,false,true),blueShadow=frame(true,false,true),twoShadow=frame(true,true,true),off=frame(true,false,false);
    auto channel=[](DWORD c,unsigned shift){return int((c>>shift)&255);};
    std::fprintf(stderr,"[sea-point] red=%d/%d blue=%d/%d other-blue=%d/%d two=(red=%d blue=%d) off=(%d,%d) raw=%llu cull=%llu faces=%llu\n",channel(redLit.red,16),channel(redShadow.red,16),channel(blueLit.blue,0),channel(blueShadow.blue,0),channel(blueLit.red,0),channel(twoShadow.red,0),channel(twoShadow.red,16),channel(twoShadow.blue,0),channel(off.red,16),channel(off.blue,0),(unsigned long long)StormMetalRawWorldDraws(d),(unsigned long long)StormMetalPointShadowGpuCullDispatches(d),(unsigned long long)StormMetalPointShadowIndirectFaceSubmissions(d));
    need(StormMetalRawWorldDraws(d)>before,"scene-model scope captures raw sea geometry");need(channel(redLit.red,16)>channel(redShadow.red,16)+8,"same-frame point cube removes the red lamp direct term behind its caster");need(channel(blueLit.blue,0)>channel(blueShadow.blue,0)+8,"blue lamp has an independent cube shadow");need(channel(twoShadow.red,0)>=channel(blueLit.red,0)-3,"red correction preserves the nonselected blue direct term");need(channel(twoShadow.blue,0)>=channel(blueShadow.blue,0)-3,"combined lamps retain the blue shadow result");need(channel(off.red,16)<8&&channel(off.blue,0)<8,"disabled lamps create neither light nor correction");
    ib->Release();meshVb->Release();read->Release();target->Release();d->Release();d3d->Release();SDL_DestroyWindow(window);SDL_Quit();std::puts("PASS sea point-only same-frame shadows preserve other lamps");
}

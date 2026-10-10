#include <SDL.h>
#include <d3d9.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gpu_skinning_bridge.hpp"

extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" void StormMetalSetLightIdentity(void*, unsigned, uint64_t);
extern "C" uint64_t StormMetalRawWorldDraws(void*);
extern "C" uint64_t StormMetalRawSkinnedDraws(void*);
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*);
extern "C" uint64_t StormMetalPointShadowIndirectFaceSubmissions(void*);
extern "C" bool StormMetalLandShadowResolved(void*);

static void need(bool ok, const char* label) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); } }
struct Vertex { float x,y,z,nx,ny,nz; DWORD color; float u,v; };
struct FramePixels { DWORD red{},blue{}; };
struct SunFrame { std::array<DWORD,64*64> pixels{}; bool resolved=false; };

int main() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto* window=SDL_CreateWindow("Sea point shadows",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL); need(window,"window");
    auto* d3d=Direct3DCreate9(D3D_SDK_VERSION); D3DPRESENT_PARAMETERS pp{}; pp.Windowed=TRUE;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9* d=nullptr; need(SUCCEEDED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,0,&pp,&d)),"device");
    IDirect3DSurface9 *target=nullptr,*read=nullptr; d->GetRenderTarget(0,&target); need(SUCCEEDED(d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)),"readback");
    const WORD ix[]={0,1,2,0,2,3}; IDirect3DVertexBuffer9* meshVb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;IDirect3DTexture9* alphaTexture=nullptr;
    need(SUCCEEDED(d->CreateVertexBuffer(sizeof(Vertex)*4,D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,&meshVb,nullptr)),"shared mesh VB");need(SUCCEEDED(d->CreateIndexBuffer(sizeof(ix),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)),"IB");need(SUCCEEDED(d->CreateTexture(1,1,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&alphaTexture,nullptr)),"alpha texture");void* p=nullptr;ib->Lock(0,0,&p,0);std::memcpy(p,ix,sizeof(ix));ib->Unlock();Vertex q[]={{-1,-1,0,0,0,-1,0xffffffff,0,0},{1,-1,0,0,0,-1,0xffffffff,1,0},{1,1,0,0,0,-1,0xffffffff,1,1},{-1,1,0,0,0,-1,0xffffffff,0,1}};meshVb->Lock(0,0,&p,0);std::memcpy(p,q,sizeof(q));meshVb->Unlock();D3DLOCKED_RECT alphaLock{};alphaTexture->LockRect(0,&alphaLock,nullptr,0);*static_cast<DWORD*>(alphaLock.pBits)=0xffffffff;alphaTexture->UnlockRect(0);
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&identity);d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetIndices(ib);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,FALSE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);D3DMATERIAL9 material{};material.Diffuse={1,1,1,1};d->SetMaterial(&material);
    auto frame=[&](bool blocker,bool red,bool blue,float translation=0.f){
        d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
        D3DMATRIX view=identity;view._41=-translation;d->SetTransform(D3DTS_VIEW,&view);
        D3DLIGHT9 redLamp{};redLamp.Type=D3DLIGHT_POINT;redLamp.Position={translation-.6f,0,-.4f};redLamp.Diffuse={1,0,0,1};redLamp.Range=3;redLamp.Attenuation0=1;d->SetLight(1,&redLamp);d->LightEnable(1,red);StormMetalSetLightIdentity(d,1,71);
        D3DLIGHT9 blueLamp=redLamp;blueLamp.Position={translation+.6f,0,-.4f};blueLamp.Diffuse={0,0,1,1};d->SetLight(2,&blueLamp);d->LightEnable(2,blue);StormMetalSetLightIdentity(d,2,72);d->LightEnable(0,FALSE);
        auto draw=[&](float x,float half,float z){D3DMATRIX world=identity;world._11=world._22=half;world._41=x+translation;world._43=z;d->SetTransform(D3DTS_WORLD,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"raw sea geometry");};
        unsigned scope=StormMetalBeginSceneModel(d);if(blocker&&red)draw(-.1f,.14f,.1f);if(blocker&&blue)draw(.05f,.14f,.1f);draw(0,.9f,.6f);StormMetalEndLandModel(d,scope);D3DPERF_SetMarker(0,L"Storm.SceneHudBoundary");d->EndScene();
        need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT lock{};read->LockRect(&lock,nullptr,D3DLOCK_READONLY);FramePixels value{};std::memcpy(&value.red,static_cast<char*>(lock.pBits)+32*lock.Pitch+48*4,4);std::memcpy(&value.blue,static_cast<char*>(lock.pBits)+32*lock.Pitch+16*4,4);read->UnlockRect();need(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"frame reset");return value;
    };
    const auto before=StormMetalRawWorldDraws(d);const auto redLit=frame(false,true,false),redShadow=frame(true,true,false),blueLit=frame(false,false,true),blueShadow=frame(true,false,true),twoShadow=frame(true,true,true),off=frame(true,false,false);
    // Translating the camera and its illuminated geometry together must preserve
    // lamp occlusion, including beyond the former 30-unit world-origin cutoff.
    const auto translatedLit=frame(false,true,false,40),translatedShadow=frame(true,true,false,40);
    need(int((translatedLit.red>>16)&255)>int((translatedShadow.red>>16)&255)+8,"translated visible receiver retains its lamp shadow");
    need(std::abs(int((redShadow.red>>16)&255)-int((translatedShadow.red>>16)&255))<=3,"point shadows are covariant under world/view translation");
    d->SetTransform(D3DTS_VIEW,&identity);
    // ShipLights clears the live D3D slot after the deck receiver. This imitates
    // the later DECK_CAMERA/Sailors draw: a valid earlier packet must admit its
    // GPU-skinned caster, while an empty frame must not create a point shadow.
    auto lateSkinnedFrame=[&](bool precedingSnapshot,bool lateBlocker){
        d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
        D3DLIGHT9 lamp{};lamp.Type=D3DLIGHT_POINT;lamp.Position={-.6f,0,-.4f};lamp.Diffuse={1,0,0,1};lamp.Range=3;lamp.Attenuation0=1;
        d->SetLight(1,&lamp);d->LightEnable(1,precedingSnapshot);StormMetalSetLightIdentity(d,1,precedingSnapshot?81:0);d->LightEnable(0,FALSE);
        auto drawDeck=[&](){D3DMATRIX world=identity;world._11=world._22=.9f;world._43=.6f;d->SetTransform(D3DTS_WORLD,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"deck receiver");};
        unsigned scope=StormMetalBeginSceneModel(d);
        if(precedingSnapshot)drawDeck();
        d->LightEnable(1,FALSE);StormMetalSetLightIdentity(d,1,0);
        if(lateBlocker){
            using namespace storm::metal::skinning;
            // `landSkin` negates the palette X axis. Mirror input so this
            // skinned blocker occupies the same x=[-.24,.04] footprint as the
            // static positive-control blocker after skinning.
            const AnimatedVertex actor[]={
                {{ .24f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,0}},
                {{-.04f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,0}},
                {{-.04f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,1}},
                {{ .24f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,1}},
            };
            D3DMATRIX actorWorld=identity;d->SetTransform(D3DTS_WORLD,&actorWorld);
            Matrix4x4 palette[1]{};palette[0].elements[0]=palette[0].elements[5]=palette[0].elements[10]=palette[0].elements[15]=1;
            need(acceptVertices(actor,4,1),"late actor accepted");need(beginPose(d,palette,1),"late actor pose");
            auto* actorVb=static_cast<IDirect3DVertexBuffer9*>(bindVertices(actor,0,4,4));need(actorVb,"late actor vertices");
            need(SUCCEEDED(d->SetStreamSource(0,actorVb,0,36)),"late actor stream");need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"late actor draw");endPose();
        }
        StormMetalEndLandModel(d,scope);D3DPERF_SetMarker(0,L"Storm.SceneHudBoundary");d->EndScene();
        need(SUCCEEDED(d->GetRenderTargetData(target,read)),"late readback");D3DLOCKED_RECT lock{};read->LockRect(&lock,nullptr,D3DLOCK_READONLY);FramePixels value{};std::memcpy(&value.red,static_cast<char*>(lock.pBits)+32*lock.Pitch+48*4,4);read->UnlockRect();need(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"late frame reset");return value;
    };
    const auto rawSkinnedBefore=StormMetalRawSkinnedDraws(d);const auto lateLit=lateSkinnedFrame(true,false),lateShadow=lateSkinnedFrame(true,true);const auto noSourceCulls=StormMetalPointShadowGpuCullDispatches(d);const auto noSource=lateSkinnedFrame(false,false),noSourceLateActor=lateSkinnedFrame(false,true);
    // The live ship/deck receiver is submitted while Weather owns slot 3.
    // ShipLights then changes slot 0 before DECK_CAMERA draws the
    // animated actor. Both caster admission and resolve must use the immutable
    // directional snapshot carried by the earlier receiver packet.
    auto lateSunFrame=[&](bool precedingSnapshot,bool earlyCutout,bool lateBlocker){
        d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
        D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={1,0,1};sun.Diffuse={1,1,1,1};
        d->SetLight(3,&sun);d->LightEnable(3,FALSE);
        auto drawDeck=[&](){D3DMATRIX world=identity;world._11=world._22=.9f;world._43=.6f;d->SetTransform(D3DTS_WORLD,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"sun deck receiver");};
        unsigned shipScope=StormMetalBeginSceneModel(d);
        if(earlyCutout){
            d->SetTexture(0,alphaTexture);d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);d->SetRenderState(D3DRS_ALPHAREF,24);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
            D3DMATRIX mast=identity;mast._11=mast._22=.14f;mast._41=.48f;mast._43=.1f;d->SetTransform(D3DTS_WORLD,&mast);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"pre-source alpha-cutout mast");
            d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetTexture(0,nullptr);
        }
        d->LightEnable(3,precedingSnapshot);if(precedingSnapshot)drawDeck();d->LightEnable(3,FALSE);
        StormMetalEndLandModel(d,shipScope);
        D3DLIGHT9 replacement=sun;replacement.Direction={0,1,0};replacement.Diffuse={0,1,0,1};d->LightEnable(0,FALSE);d->SetLight(0,&replacement);
        if(lateBlocker){
            unsigned scope=StormMetalBeginSceneModel(d);
            using namespace storm::metal::skinning;
            const AnimatedVertex actor[]={
                {{ .24f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,0}},
                {{-.04f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,0}},
                {{-.04f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,1}},
                {{ .24f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,1}},
            };
            D3DMATRIX actorWorld=identity;d->SetTransform(D3DTS_WORLD,&actorWorld);
            Matrix4x4 palette[1]{};palette[0].elements[0]=palette[0].elements[5]=palette[0].elements[10]=palette[0].elements[15]=1;
            need(acceptVertices(actor,4,1),"late sun actor accepted");need(beginPose(d,palette,1),"late sun actor pose");
            auto* actorVb=static_cast<IDirect3DVertexBuffer9*>(bindVertices(actor,0,4,4));need(actorVb,"late sun actor vertices");
            need(SUCCEEDED(d->SetStreamSource(0,actorVb,0,36)),"late sun actor stream");need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"late sun actor draw");endPose();
            StormMetalEndLandModel(d,scope);
        }
        D3DPERF_SetMarker(0,L"Storm.SceneHudBoundary");d->EndScene();
        SunFrame value{};value.resolved=StormMetalLandShadowResolved(d);need(SUCCEEDED(d->GetRenderTargetData(target,read)),"sun readback");D3DLOCKED_RECT lock{};read->LockRect(&lock,nullptr,D3DLOCK_READONLY);for(int y=0;y<64;y++)std::memcpy(value.pixels.data()+y*64,static_cast<char*>(lock.pBits)+y*lock.Pitch,64*4);read->UnlockRect();need(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"sun frame reset");return value;
    };
    const auto sunLit=lateSunFrame(true,false,false),sunMastShadow=lateSunFrame(true,true,false),sunShadow=lateSunFrame(true,true,true),noSun=lateSunFrame(false,false,false),noSunActors=lateSunFrame(false,true,true);
    auto channel=[](DWORD c,unsigned shift){return int((c>>shift)&255);};
    unsigned mastDarkened=0,actorDarkened=0,noSunChanged=0;for(int y=16;y<48;y++)for(int x=0;x<64;x++){const auto index=size_t(y*64+x);mastDarkened+=channel(sunLit.pixels[index],16)>channel(sunMastShadow.pixels[index],16)+8;actorDarkened+=channel(sunMastShadow.pixels[index],16)>channel(sunShadow.pixels[index],16)+8;noSunChanged+=noSun.pixels[index]!=noSunActors.pixels[index];}
    std::fprintf(stderr,"[sea-point] red=%d/%d blue=%d/%d other-blue=%d/%d two=(red=%d blue=%d) off=(%d,%d) late-red=%d/%d no-source=%d/%d mast-darkened=%u actor-darkened=%u no-sun-changed=%u raw=%llu cull=%llu faces=%llu\n",channel(redLit.red,16),channel(redShadow.red,16),channel(blueLit.blue,0),channel(blueShadow.blue,0),channel(blueLit.red,0),channel(twoShadow.red,0),channel(twoShadow.red,16),channel(twoShadow.blue,0),channel(off.red,16),channel(off.blue,0),channel(lateLit.red,16),channel(lateShadow.red,16),channel(noSource.red,16),channel(noSourceLateActor.red,16),mastDarkened,actorDarkened,noSunChanged,(unsigned long long)StormMetalRawWorldDraws(d),(unsigned long long)StormMetalPointShadowGpuCullDispatches(d),(unsigned long long)StormMetalPointShadowIndirectFaceSubmissions(d));
    need(StormMetalRawWorldDraws(d)>before,"scene-model scope captures raw sea geometry");need(channel(redLit.red,16)>channel(redShadow.red,16)+8,"same-frame point cube removes the red lamp direct term behind its caster");need(channel(blueLit.blue,0)>channel(blueShadow.blue,0)+8,"blue lamp has an independent cube shadow");need(channel(twoShadow.red,0)>=channel(blueLit.red,0)-3,"red correction preserves the nonselected blue direct term");need(channel(twoShadow.blue,0)>=channel(blueShadow.blue,0)-3,"combined lamps retain the blue shadow result");need(channel(off.red,16)<8&&channel(off.blue,0)<8,"disabled lamps create neither light nor correction");need(StormMetalRawSkinnedDraws(d)>rawSkinnedBefore,"late GPU-skinned actor uses the raw Metal path");need(channel(lateLit.red,16)>channel(lateShadow.red,16)+8,"preceding lamp snapshot admits a late skinned actor as a deck caster");need(StormMetalPointShadowGpuCullDispatches(d)==noSourceCulls,"no preceding point snapshot does not dispatch a point-shadow cube");need(noSource.red==noSourceLateActor.red,"no preceding point snapshot keeps a late skinned actor out of the registry");need(sunLit.resolved&&sunMastShadow.resolved&&sunShadow.resolved,"frozen directional snapshot resolves after slot 0 is replaced");need(mastDarkened>8,"pre-source SELECTARG1 ship cutout casts a directional shadow");need(actorDarkened>8,"late GPU-skinned sailor casts an additional directional shadow");need(!noSun.resolved&&!noSunActors.resolved,"next frame without a directional source does not resolve stale sun");need(noSunChanged==0,"next frame without a directional source does not apply stale sun");
    alphaTexture->Release();ib->Release();meshVb->Release();read->Release();target->Release();d->Release();d3d->Release();SDL_DestroyWindow(window);SDL_Quit();std::puts("PASS sea point and mixed-order deck shadows preserve source ownership");
}

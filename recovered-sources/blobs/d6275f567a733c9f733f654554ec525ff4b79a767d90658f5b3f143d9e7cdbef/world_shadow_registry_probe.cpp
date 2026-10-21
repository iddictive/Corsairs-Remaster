#include <SDL.h>
#include <d3d9.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalLandShadowResolved(void*);
static void need(bool ok,const char*name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
int main(){
 setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);
 auto*w=SDL_CreateWindow("World shadow registry",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*api=Direct3DCreate9(D3D_SDK_VERSION);
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 IDirect3DTexture9*t=nullptr;need(SUCCEEDED(d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr)),"texture");D3DLOCKED_RECT lr{};t->LockRect(0,&lr,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)reinterpret_cast<DWORD*>(static_cast<char*>(lr.pBits)+y*lr.Pitch)[x]=0xff8090a0;t->UnlockRect(0);
 IDirect3DSurface9*target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection=identity;projection._11=projection._22=1.f/8;projection._33=1.f/32;
 struct V{float x,y,z,nx,ny,nz,u,v;};auto quad=[&](float r,float z){V v[]={{-r,-r,z,0,0,-1,0,0},{r,-r,z,0,0,-1,1,0},{r,r,z,0,0,-1,1,1},{-r,-r,z,0,0,-1,0,0},{r,r,z,0,0,-1,1,1},{-r,r,z,0,0,-1,0,1}};unsigned scope=StormMetalBeginSceneModel(d);need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"MODELR draw");StormMetalEndLandModel(d,scope);};
 auto frame=[&](bool caster,bool blended,bool multistage){d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,false);d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff101010,1,0);d->BeginScene();d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1);d->SetTexture(0,t);d->SetTexture(1,multistage?t:nullptr);d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_AMBIENT,0x202020);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,multistage?D3DTOP_MODULATE:D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);D3DMATERIAL9 m{};m.Diffuse=m.Ambient={1,1,1,1};d->SetMaterial(&m);D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};sun.Diffuse={.8f,.8f,.8f,1};d->SetLight(0,&sun);d->LightEnable(0,TRUE);quad(7,8);if(caster){d->SetRenderState(D3DRS_ALPHABLENDENABLE,blended);quad(1.5f,7.5f);}D3DPERF_SetMarker(0,L"Storm.SceneHudBoundary");need(StormMetalLandShadowResolved(d),"unconditional scene-boundary registry resolve");d->EndScene();need(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");readback->LockRect(&lr,nullptr,D3DLOCK_READONLY);std::vector<DWORD> px(4096);for(int y=0;y<64;y++)std::memcpy(px.data()+y*64,static_cast<char*>(lr.pBits)+y*lr.Pitch,256);readback->UnlockRect();return px;};
 auto empty=frame(false,false,false),opaque=frame(true,false,false),blended=frame(true,true,false),lightmapped=frame(true,false,true);auto b=[](DWORD c){return int(c&255)+int(c>>8&255)+int(c>>16&255);};auto darker=[&](const std::vector<DWORD>&p){int n=0;for(int y=20;y<44;y++)for(int x=20;x<44;x++)n+=b(p[y*64+x])+8<b(empty[y*64+x]);return n;};int a=darker(opaque),z=darker(blended),m=darker(lightmapped);std::printf("world registry shadows: opaque=%d blended-zwrite=%d multistage=%d\n",a,z,m);need(a>8,"opaque MODELR caster");need(z>8,"blended Z-writing MODELR caster");need(m>8,"multistage/lightmapped MODELR caster");
 t->Release();target->Release();readback->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS shared world shadow registry");
}

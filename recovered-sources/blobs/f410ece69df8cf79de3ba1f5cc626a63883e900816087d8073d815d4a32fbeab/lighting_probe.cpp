#include <SDL.h>
#include <d3d9.h>
#include "lighting.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
struct Result {DWORD up,side,down,night,dark,emissive,unlit,ui;};
static Result render(bool modern){
 setenv("STORM_METAL_MODERN_LIGHTING",modern?"1":"0",1);
 auto*w=SDL_CreateWindow("Lighting fixture",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);check(w,SDL_GetError());
 auto*api=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=32;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;
 check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_COLORVERTEX,FALSE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 IDirect3DSurface9 *target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(32,32,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 D3DMATERIAL9 material{};material.Diffuse={.5f,.4f,.3f,.4f};material.Ambient={.5f,.4f,.3f,1};
 struct V{float p[3],n[3];DWORD color;};V vertices[]={{{-1,1,.5f},{0,1,0},0xff336699},{{1,1,.5f},{0,1,0},0xff336699},{{-1,-1,.5f},{0,1,0},0xff336699},{{1,-1,.5f},{0,1,0},0xff336699}};
 auto draw=[&](float ny,DWORD ambient,float sunlight,bool lit,bool emissive,bool ui){
  for(auto&v:vertices){v.n[0]=std::sqrt(std::max(0.f,1-ny*ny));v.n[1]=ny;v.n[2]=0;}
  material.Emissive=emissive?D3DCOLORVALUE{.1f,.2f,.3f,1}:D3DCOLORVALUE{};d->SetMaterial(&material);d->SetRenderState(D3DRS_AMBIENT,ambient);d->SetRenderState(D3DRS_LIGHTING,lit);
  D3DLIGHT9 light{};light.Type=D3DLIGHT_DIRECTIONAL;light.Direction={0,-1,0};light.Diffuse={sunlight,sunlight,sunlight,1};d->SetLight(0,&light);d->LightEnable(0,TRUE);
  d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);d->BeginScene();HRESULT hr;
  if(ui){struct UI{float x,y,z,rhw;DWORD color;};UI v[]={{0,0,.5f,1,0x80336699},{32,0,.5f,1,0x80336699},{0,32,.5f,1,0x80336699},{32,32,.5f,1,0x80336699}};d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(UI));}
  else{d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE);hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V));}
  check(SUCCEEDED(hr),"draw");d->EndScene();check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");D3DLOCKED_RECT lock{};readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);DWORD pixel;std::memcpy(&pixel,(char*)lock.pBits+16*lock.Pitch+16*4,4);readback->UnlockRect();return pixel;
 };
 Result result{draw(1,0xff404040,.5f,true,false,false),draw(0,0xff404040,.5f,true,false,false),draw(-1,0xff404040,.5f,true,false,false),draw(0,0xff101010,.05f,true,false,false),draw(0,0,0,true,false,false),draw(0,0,0,true,true,false),draw(0,0xff404040,.5f,false,false,false),draw(0,0xff404040,.5f,true,false,true)};
 target->Release();readback->Release();d->Release();api->Release();SDL_DestroyWindow(w);return result;
}
int main(){check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto a=render(false),b=render(true);auto red=[](DWORD c){return int((c>>16)&255);};
 check(a.up==b.up,"sun-facing peak unchanged");check(b.side!=a.side,"side lighting responds");check(red(b.down)<red(a.down),"hemisphere underside");check(red(b.night)<red(b.side),"night illumination preserved");check((b.dark&0xffffff)==0,"no invented light");check(a.emissive==b.emissive,"emissive unchanged");check(a.unlit==b.unlit&&a.ui==b.ui,"unlit and UI exact bypass");check((a.side>>24)==(b.side>>24)&&std::abs(int(b.side>>24)-102)<=1,"material alpha preserved");
 constexpr int count=2000000;float checksums[2]{};double times[2]{};
 for(int mode=0;mode<2;mode++){volatile float sink=0;auto start=std::chrono::steady_clock::now();for(int i=0;i<count;i++){float n=float(i%2001)/1000.f-1;sink=sink+(mode?sm::modernAmbientFactor(n):1.f)+(mode?sm::modernSunResponse(n):std::max(0.f,n));}times[mode]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();checksums[mode]=sink;}
 std::printf("PASS: day/night, normals, alpha, emissive, unlit/UI; helper CPU proxy original %.3f ms, modern %.3f ms / %d pairs (not game FPS), checksums %.3f/%.3f\n",times[0],times[1],count,checksums[0],checksums[1]);SDL_Quit();
}

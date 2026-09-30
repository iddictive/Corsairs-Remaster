#include <SDL.h>
#include <d3d9.h>
#include "lighting.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" void StormMetalEndLandModel(void*,unsigned);
static void check(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
struct Result {DWORD up,side,down,night,dark,emissive,unlit,ui;};
static Result render(bool modern){
 setenv("STORM_METAL_MODERN_LIGHTING",modern?"1":"0",1);
 auto*w=SDL_CreateWindow("Lighting fixture",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);check(w,SDL_GetError());
 auto*api=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=32;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;
 check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 DWORD defaultDiffuse=0,defaultAmbient=0,defaultColorVertex=0;d->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,&defaultDiffuse);d->GetRenderState(D3DRS_AMBIENTMATERIALSOURCE,&defaultAmbient);d->GetRenderState(D3DRS_COLORVERTEX,&defaultColorVertex);
 check(defaultColorVertex==TRUE&&defaultDiffuse==D3DMCS_COLOR1&&defaultAmbient==D3DMCS_COLOR1,"D3D9 location defaults use vertex COLOR1 for diffuse and ambient");
 d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_COLORVERTEX,FALSE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 IDirect3DSurface9 *target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(32,32,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 D3DMATERIAL9 material{};material.Diffuse={.5f,.4f,.3f,.4f};material.Ambient={.5f,.4f,.3f,1};
 struct V{float p[3],n[3];DWORD color;};V vertices[]={{{-1,1,.5f},{0,1,0},0xff336699},{{1,1,.5f},{0,1,0},0xff336699},{{-1,-1,.5f},{0,1,0},0xff336699},{{1,-1,.5f},{0,1,0},0xff336699}};
 auto draw=[&](float ny,DWORD ambient,float sunlight,bool lit,bool emissive,bool ui,bool noDiffuse=false,bool sceneModel=false){
  for(auto&v:vertices){v.n[0]=std::sqrt(std::max(0.f,1-ny*ny));v.n[1]=ny;v.n[2]=0;}
  material.Emissive=emissive?D3DCOLORVALUE{.1f,.2f,.3f,1}:D3DCOLORVALUE{};d->SetMaterial(&material);d->SetRenderState(D3DRS_AMBIENT,ambient);d->SetRenderState(D3DRS_LIGHTING,lit);
  D3DLIGHT9 light{};light.Type=D3DLIGHT_DIRECTIONAL;light.Direction={0,-1,0};light.Diffuse={sunlight,sunlight,sunlight,1};d->SetLight(0,&light);d->LightEnable(0,TRUE);
  d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);d->BeginScene();unsigned previousScope=sceneModel?StormMetalBeginLandModel(d,2):0;HRESULT hr;
  if(ui){struct UI{float x,y,z,rhw;DWORD color;};UI v[]={{0,0,.5f,1,0x80336699},{32,0,.5f,1,0x80336699},{0,32,.5f,1,0x80336699},{32,32,.5f,1,0x80336699}};d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(UI));}
  else if(noDiffuse){struct N{float p[3],n[3];};N v[4];for(int i=0;i<4;i++){std::memcpy(v[i].p,vertices[i].p,12);std::memcpy(v[i].n,vertices[i].n,12);}d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL);hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(N));}
  else{d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE);hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V));}
  check(SUCCEEDED(hr),"draw");if(sceneModel)StormMetalEndLandModel(d,previousScope);d->EndScene();check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");D3DLOCKED_RECT lock{};readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);DWORD pixel;std::memcpy(&pixel,(char*)lock.pBits+16*lock.Pitch+16*4,4);readback->UnlockRect();return pixel;
 };
 Result result{draw(1,0xff404040,.5f,true,false,false),draw(0,0xff404040,.5f,true,false,false),draw(-1,0xff404040,.5f,true,false,false),draw(0,0xff101010,.05f,true,false,false),draw(0,0,0,true,false,false),draw(0,0,0,true,true,false),draw(0,0xff404040,.5f,false,false,false),draw(0,0xff404040,.5f,true,false,true)};
 // Actual shipped characters: authored COLOR1=127, GM diffuse=.8. These are
 // intentionally different; an all-white fixture cannot catch this default.
 material.Diffuse={.8f,.8f,.8f,.4f};material.Ambient={0,0,0,0};
 for(auto&v:vertices)v.color=0x7f7f7f7f;
 d->SetRenderState(D3DRS_COLORVERTEX,TRUE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,defaultDiffuse);
 DWORD authored=draw(1,0,1,true,false,false),night=draw(1,0,.1f,true,false,false),fallback=draw(1,0,1,true,false,false,true);
 d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);DWORD explicitMaterial=draw(1,0,1,true,false,false);
 auto red=[](DWORD c){return int((c>>16)&255);};
 check(std::abs(red(authored)-127)<=1&&std::abs(int(authored>>24)-127)<=1,"authored character vertex color and alpha govern default lighting");
 check(std::abs(red(night)-13)<=1,"authored diffuse follows night energy");
 check(std::abs(red(explicitMaterial)-204)<=1&&std::abs(int(explicitMaterial>>24)-102)<=1,"explicit material override preserved");
 check(fallback==explicitMaterial,"missing vertex diffuse falls back to material");

 // GEOS writes zero material ambient for animated models while their authored
 // albedo lives in COLOR1. Outside direct sun, weather ambient must still reach
 // those models; its real day/night energy remains the sole brightness owner.
 d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,defaultDiffuse);
 DWORD colorVertex=0,diffuseSource=0;
 d->GetRenderState(D3DRS_COLORVERTEX,&colorVertex);
 d->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,&diffuseSource);
 check(colorVertex==TRUE&&diffuseSource==D3DMCS_COLOR1,
       "character fixture uses authored COLOR1 diffuse state");
 material.Ambient={0,0,0,0};
 StormMetalLandLocation(d,true);
 DWORD outdoorDay=draw(0,0xff696969,220.f/255.f,true,false,false,false,true);
 DWORD outdoorNight=draw(0,0xff1c1c23,30.f/255.f,true,false,false,false,true);
 material.Ambient={.25f,.25f,.25f,1};
 DWORD explicitOutdoor=draw(0,0xff696969,220.f/255.f,true,false,false,false,true);
 material.Ambient={0,0,0,0};
 StormMetalLandLocation(d,false);
 DWORD unclassified=draw(0,0xff696969,220.f/255.f,true,false,false);
 const float explicitHemisphere=modern?sm::modernAmbientFactor(0.f,true):1.f;
 const int expectedExplicit=int(std::lround(255.f*.25f*(105.f/255.f)*explicitHemisphere));
std::printf("authored outdoor ambient day=%d night=%d explicit=%d unclassified=%d\n",
            red(outdoorDay),red(outdoorNight),red(explicitOutdoor),red(unclassified));
 // The outdoor-only sky floor survives the actual authored COLOR1 material
 // path.  Legacy lighting remains at its original weather energy.
 const int expectedNight=modern?45:14;
 check(std::abs(red(outdoorDay)-52)<=1&&std::abs(red(outdoorNight)-expectedNight)<=1&&red(outdoorDay)>red(outdoorNight),
      "outdoor floor remains below authored daytime ambient");
 check(std::abs(red(explicitOutdoor)-expectedExplicit)<=1,
       "explicit nonzero material ambient remains authoritative outdoors");
 check((unclassified&0xffffff)==0,
       "unclassified and interior-style material lighting keeps zero authored ambient");
 target->Release();readback->Release();d->Release();api->Release();SDL_DestroyWindow(w);return result;
}
int main(){check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto a=render(false),b=render(true);auto red=[](DWORD c){return int((c>>16)&255);};
 auto rotateY=[](simd_float3 p,float radians){float c=std::cos(radians),s=std::sin(radians);return simd_make_float3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);};
 const simd_float3 absoluteLight={17,3,-11},absolutePoint={13,1,-7};
 const simd_float3 cameraA={2,0,-1},cameraB={-9,0,6};
 auto relativeA=sm::relativeLightPosition(absoluteLight,-cameraA),relativeB=sm::relativeLightPosition(absoluteLight,-cameraB);
 auto pointA=sm::relativeLightPosition(absolutePoint,-cameraA),pointB=sm::relativeLightPosition(absolutePoint,-cameraB);
 check(simd_length((relativeA-pointA)-(relativeB-pointB))<1e-5f,"camera-relative origin preserves world light-to-surface vector");
 check(simd_length(rotateY(relativeA-pointA,.83f)-rotateY(relativeB-pointB,.83f))<1e-5f,"camera rotation cannot move an authored world light");
 check(a.up==b.up&&a.side==b.side&&a.down==b.down&&a.night==b.night,
       "unclassified material lighting remains unchanged");check((b.dark&0xffffff)==0,"no invented light");check(a.emissive==b.emissive,"emissive unchanged");check(a.unlit==b.unlit&&a.ui==b.ui,"unlit and UI exact bypass");check((a.side>>24)==(b.side>>24)&&std::abs(int(b.side>>24)-102)<=1,"material alpha preserved");
 constexpr int count=2000000;float checksums[2]{};double times[2]{};
 for(int mode=0;mode<2;mode++){volatile float sink=0;auto start=std::chrono::steady_clock::now();for(int i=0;i<count;i++){float n=float(i%2001)/1000.f-1;sink=sink+(mode?sm::modernAmbientFactor(n,true):1.f)+(mode?sm::modernSunResponse(n,true):std::max(0.f,n));}times[mode]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();checksums[mode]=sink;}
 std::printf("PASS: day/night, normals, alpha, emissive, unlit/UI; helper CPU proxy original %.3f ms, modern %.3f ms / %d pairs (not game FPS), checksums %.3f/%.3f\n",times[0],times[1],count,checksums[0],checksums[1]);SDL_Quit();
}

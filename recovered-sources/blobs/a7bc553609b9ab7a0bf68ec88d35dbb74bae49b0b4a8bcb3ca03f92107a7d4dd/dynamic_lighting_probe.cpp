#include <initializer_list>
#include <vector>
#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void need(bool x,const char*s){if(!x){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
extern "C" void StormMetalShadowLamp(void*,const D3DLIGHT9*);
extern "C" void StormMetalBeginSceneLights(void*);
extern "C" bool StormMetalSceneLight(void*,uint64_t,const D3DLIGHT9*);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalResolveLandLighting(void*);
extern "C" bool StormMetalLandShadowResolved(void*);
extern "C" bool StormMetalBeginShadowFrame(void*,uint64_t,uint64_t,const float*);
extern "C" bool StormMetalBeginShadowPass(void*,unsigned,unsigned,uint64_t,const float*,float,float*,float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" void StormMetalSetLightIdentity(void*,unsigned,uint64_t);
int main(){setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Weapon FFP contract",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 IDirect3DTexture9*t=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT l{};t->LockRect(0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=0xff204060;t->UnlockRect(0);

 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection=identity;projection._11=projection._22=1.f/16;projection._33=1.f/32;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);
 struct V{float x,y,z,nx,ny,nz;DWORD color;float u,v;};
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixels=[&](){need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT q{};read->LockRect(&q,nullptr,D3DLOCK_READONLY);std::vector<DWORD> out(4096);for(int y=0;y<64;y++)memcpy(out.data()+y*64,(char*)q.pBits+y*q.Pitch,256);read->UnlockRect();return out;};
 auto energy=[](DWORD c){return int(c&255)+int(c>>8&255)+int(c>>16&255);};
 auto quad=[&](float x,float radius,float z,DWORD color,unsigned role){V v[]={{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,-radius,z,0,0,-1,color,1,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,radius,z,0,0,-1,color,0,1}};unsigned previous=StormMetalBeginLandModel(d,role);need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"scoped geometry");StormMetalEndLandModel(d,previous);};
 auto frame=[&](bool active,int ambient,int sunlight,float casterX,bool resolve){d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,active);d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);DWORD a=ambient;d->SetRenderState(D3DRS_AMBIENT,a|(a<<8)|(a<<16));D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};float k=sunlight/255.f;sun.Diffuse={k,k,k,1};d->SetLight(0,&sun);d->LightEnable(0,TRUE);d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);quad(0,14,8,0xff202020,1);if(casterX<50)quad(casterX,1.5,3,0xff202020,1);bool applied=resolve&&StormMetalResolveLandLighting(d);if(active&&resolve)need(applied&&StormMetalLandShadowResolved(d),"shadow resolve succeeds");if(!active)need(!StormMetalLandShadowResolved(d),"disabled location keeps legacy fallback");quad(11,1,2,0xff808080,0);d->EndScene();return pixels();};
 auto day=frame(true,70,180,100,true),night=frame(true,15,20,100,true);printf("lighting day=%08x night=%08x\n",day[32*64+32],night[32*64+32]);need(energy(day[2080])>energy(night[2080])*2,"weather lights replace fixed baked COL");
 need(day[32*64+54]==night[32*64+54],"unscoped unlit overlay remains weather independent");
 auto base=frame(false,70,180,100,false);need(energy(day[2080])>energy(base[2080]),"explicit static scope replaces baked dark color");
 auto shadowA=frame(true,70,180,-3,true),shadowB=frame(true,70,180,3,true);int darkA=0,darkB=0,moved=0;for(int y=27;y<37;y++)for(int x=18;x<49;x++){// Exclude both visible caster footprints; inspect receiver only.
 if((x>=22&&x<=30)||(x>=34&&x<=42))continue;int i=y*64+x;if(energy(shadowA[i])+8<energy(day[i]))darkA++;if(energy(shadowB[i])+8<energy(day[i]))darkB++;if(abs(energy(shadowA[i])-energy(shadowB[i]))>8)moved++;}printf("shadow receiver pixels %d %d moved %d\n",darkA,darkB,moved);need(darkA>2&&darkB>2&&moved>4,"moving caster moves receiver shadow outside caster silhouette");
 // The next frames use source replay, not the screen-visible receiver packets.
 // The directional caster lies outside the main camera's X and near planes.
 uint64_t generation=0;
 auto prepassFrame=[&](bool point,bool caster,float casterX,bool matchingId,bool complete=true,unsigned receiverRole=1,bool faded=false,bool indoor=false,float lampX=0,float sunEnergy=.7f,int catalog=0,float focusX=0,float contactGap=0){
  d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,true);StormMetalLandInterior(d,indoor);
  d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
  d->SetRenderState(D3DRS_AMBIENT,0x181818);d->SetRenderState(D3DRS_LIGHTING,FALSE);
  d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
  D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};sun.Diffuse={sunEnergy,sunEnergy,sunEnergy,1};d->SetLight(0,&sun);d->LightEnable(0,!point);
  D3DLIGHT9 lamp{};lamp.Type=D3DLIGHT_POINT;lamp.Position={lampX,0,0};lamp.Diffuse={.8f,.8f,.8f,1};lamp.Range=indoor?12:24;lamp.Attenuation0=1;lamp.Attenuation2=indoor?.05f:0;d->SetLight(1,&lamp);d->LightEnable(1,point);
  const float focus[]={focusX,0,8};++generation;need(StormMetalBeginShadowFrame(d,100+generation,generation,focus),"source prepass begins");
  if(catalog){StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,17,&lamp),"source lamp catalog");auto second=lamp;second.Position.x=4;second.Diffuse={.2f,.6f,.35f,1};need(StormMetalSceneLight(d,18,&second),"second source lamp catalog");}
  if(indoor){StormMetalShadowLamp(d,&lamp);d->LightEnable(1,FALSE);d->LightEnable(0,TRUE);} // selected lamp must survive camera-light mismatch; outdoor sun must not leak
  const float ray[]={.6f,0,1},position[]={lampX,0,0};
  for(unsigned face=0;face<(point?(complete?6u:5u):1u);face++){
   D3DMATRIX lightView{},lightProjection{};
   need(StormMetalBeginShadowPass(d,point?1:0,face,point?17:1,point?position:ray,point?lamp.Range:32,&lightView.m[0][0],&lightProjection.m[0][0]),"shadow face begins");
   d->SetTransform(D3DTS_VIEW,&lightView);d->SetTransform(D3DTS_PROJECTION,&lightProjection);
   quad(0,14,8,0xff202020,1);
   if(caster)quad(point?casterX:contactGap>0?-.6f*contactGap:-20,point?1:1.5f,point?4:contactGap>0?8-contactGap:-24,0xff202020,1);
   StormMetalEndShadowPass(d);
  }
  const bool applied=StormMetalEndShadowFrame(d);need(applied==complete,"partial cube is not a valid shadow");
  d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);
  StormMetalSetLightIdentity(d,1,matchingId?17:99);
  if(catalog){auto cameraSelected=lamp;cameraSelected.Position.x=catalog==1?-5:5;cameraSelected.Diffuse=catalog==1?D3DCOLORVALUE{1,0,0,1}:D3DCOLORVALUE{0,0,1,1};d->SetLight(2,&cameraSelected);d->LightEnable(2,TRUE);StormMetalSetLightIdentity(d,2,catalog==1?31:32);}
  if(receiverRole==2){
   // CPU-skinned Animation output uses this same XYZ/NORMAL/DIFFUSE/TEX1
   // declaration. Its normal and animated position are consumed in world space.
   d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,TRUE);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_COLOR1);
   d->SetRenderState(D3DRS_ALPHATESTENABLE,!faded);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);d->SetRenderState(D3DRS_ALPHAREF,0);
   d->SetTextureStageState(0,D3DTSS_ALPHAOP,faded?D3DTOP_SELECTARG1:D3DTOP_MODULATE);
   d->SetTextureStageState(0,D3DTSS_ALPHAARG1,faded?D3DTA_TFACTOR:D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
   d->SetRenderState(D3DRS_TEXTUREFACTOR,0x66000000);d->SetRenderState(D3DRS_ZWRITEENABLE,!faded);d->SetRenderState(D3DRS_ALPHABLENDENABLE,faded);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
  }
  quad(0,14,8,0xff202020,receiverRole); // no caster submitted to the visible color pass
  d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
  quad(11,1,2,0xff808080,0);d->LightEnable(2,FALSE);d->EndScene();return pixels();
 };
 auto sunClear=prepassFrame(false,false,0,true),sunOffscreen=prepassFrame(false,true,0,true);
 int offscreenDark=0;for(int i=0;i<4096;i++)if(energy(sunOffscreen[i])+12<energy(sunClear[i]))offscreenDark++;
 need(offscreenDark>8,"off-camera source caster casts into visible scene");
 auto lampClear=prepassFrame(true,false,0,true),lampA=prepassFrame(true,true,-2,true),lampB=prepassFrame(true,true,2,true);
 int lampDark=0,lampMoved=0;for(int i=0;i<4096;i++){if(energy(lampA[i])+12<energy(lampClear[i]))lampDark++;if(abs(energy(lampA[i])-energy(lampB[i]))>12)lampMoved++;}
 need(lampDark>8&&lampMoved>16,"point lamp cube shadow follows source caster");
 auto otherLamp=prepassFrame(true,true,-2,false),lampFresh=prepassFrame(true,false,0,true);
 need(otherLamp==lampClear,"shadow identity does not darken another light");
 need(lampFresh==lampClear,"new scene/frame cannot reuse removed caster depth");
 need(lampA[32*64+54]==lampClear[32*64+54],"unscoped overlay bypasses real shadow sampling");
 auto characterClear=prepassFrame(false,false,0,true,true,2),characterShadow=prepassFrame(false,true,0,true,true,2);
 auto fadedClear=prepassFrame(false,false,0,true,true,2,true),fadedShadow=prepassFrame(false,true,0,true,true,2,true);
 int characterDark=0,fadedDark=0;for(int i=0;i<4096;i++){if(energy(characterShadow[i])+12<energy(characterClear[i]))characterDark++;if(energy(fadedShadow[i])+4<energy(fadedClear[i]))fadedDark++;}
 need(characterDark>8,"opaque Animation receiver gets source shadow");need(fadedDark>8,"camera-faded AnimationBlend receiver gets source shadow before alpha blend");
 printf("character receiver opaque %d faded %d\n",characterDark,fadedDark);
 if(std::getenv("STORM_METAL_TEST_CLOSE_CONTACT")){
  auto close=prepassFrame(false,true,0,true,true,2,false,false,0,.7f,0,0,.02f);
  int closePixels=0;for(int i=0;i<4096;i++)if(energy(close[i])+8<energy(characterClear[i]))closePixels++;
  printf("character 2cm contact occlusion pixels %d\n",closePixels);need(closePixels>8,"character shadow preserves 2cm self-occlusion");
 }
 auto interiorClear=prepassFrame(true,false,0,true,true,1,false,true),interiorShadow=prepassFrame(true,true,-2,true,true,1,false,true),interiorMoved=prepassFrame(true,false,0,true,true,1,false,true,4);
 auto interiorNoSun=prepassFrame(true,false,0,true,true,1,false,true,0,0);need(interiorNoSun==interiorClear,"outdoor sun cannot illuminate indoor receivers");
 need(energy(interiorClear[32*64+32])>energy(interiorClear[32*64+57])+15,"interior lamp illuminates wall center when large triangle vertices lie outside range");
 int interiorDark=0,interiorChanged=0;for(int i=0;i<4096;i++){if(energy(interiorShadow[i])+6<energy(interiorClear[i]))interiorDark++;if(abs(energy(interiorMoved[i])-energy(interiorClear[i]))>6)interiorChanged++;}
 need(interiorDark>8,"interior wall receives point-light occlusion");need(interiorChanged>40,"moving interior lamp changes spatial illumination");
 printf("interior center %08x far %08x occluded %d moved %d\n",interiorClear[32*64+32],interiorClear[32*64+57],interiorDark,interiorChanged);
 auto catalogLeft=prepassFrame(true,false,0,true,true,1,false,true,0,.7f,1,-6);
 auto catalogRight=prepassFrame(true,false,0,true,true,1,false,true,0,.7f,2,6);
 need(catalogLeft==catalogRight,"fixed-world indoor illumination ignores camera slots and player nearest-light boundaries");
 need(catalogLeft!=interiorClear,"second physical source lamp contributes independently of selected shadow lamp");
 prepassFrame(true,true,-2,true,false);need(!StormMetalLandShadowResolved(d),"incomplete prepass retains legacy shadow owner");
 printf("prepass offscreen pixels %d lamp pixels %d moved %d\n",offscreenDark,lampDark,lampMoved);
 d->SetTexture(0,nullptr);t->Release();read->Release();target->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS dynamic lighting and real caster GPU fixture");}

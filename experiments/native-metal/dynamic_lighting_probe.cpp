#include <initializer_list>
#include <vector>
#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gpu_skinning_bridge.hpp"
static void need(bool x,const char*s){if(!x){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
extern "C" void StormMetalShadowLamp(void*,const D3DLIGHT9*);
extern "C" void StormMetalBeginSceneLights(void*);
extern "C" bool StormMetalSceneLight(void*,uint64_t,const D3DLIGHT9*);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalBeginShadowFrame(void*,uint64_t,uint64_t,const float*);
extern "C" bool StormMetalBeginShadowPass(void*,unsigned,unsigned,uint64_t,const float*,float,float*,float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" void StormMetalSetLightIdentity(void*,unsigned,uint64_t);
extern "C" uint64_t StormMetalRawLocationDraws(void*);
extern "C" uint64_t StormMetalRawSkinnedDraws(void*);
extern "C" void StormMetalSetWorldOrigin(void*,const float*);
extern "C" bool StormMetalBeginOffscreenPointCaster(void*,const float*,float);
extern "C" void StormMetalEndOffscreenPointCaster(void*);
extern "C" bool StormMetalRenderPointShadowCube(void*,uint64_t,const float*,float);
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*);
int main(){setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);setenv("STORM_METAL_MODERN_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Weapon FFP contract",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 IDirect3DTexture9*t=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT l{};t->LockRect(0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=0xff204060;t->UnlockRect(0);

 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection=identity;projection._11=projection._22=1.f/16;projection._33=1.f/32;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
 struct V{float x,y,z,nx,ny,nz;DWORD color;float u,v;};
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixels=[&](){need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT q{};read->LockRect(&q,nullptr,D3DLOCK_READONLY);std::vector<DWORD> out(4096);for(int y=0;y<64;y++)memcpy(out.data()+y*64,(char*)q.pBits+y*q.Pitch,256);read->UnlockRect();return out;};
 auto quad=[&](float x,float radius,float z,DWORD color,unsigned role){V v[]={{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,-radius,z,0,0,-1,color,1,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,radius,z,0,0,-1,color,0,1}};unsigned previous=StormMetalBeginLandModel(d,role);need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"scoped geometry");StormMetalEndLandModel(d,previous);};
 auto boundStaticQuad=[&](float x,float radius,float z,DWORD color,unsigned role){
  const V vertices[]={
   {x-radius,-radius,z,0,0,-1,color,0,0},
   {x+radius,-radius,z,0,0,-1,color,1,0},
   {x+radius,radius,z,0,0,-1,color,1,1},
   {x-radius,radius,z,0,0,-1,color,0,1}
  };
  const WORD indices[]={0,1,2,0,2,3};
  IDirect3DVertexBuffer9*vb=nullptr;IDirect3DIndexBuffer9*ib=nullptr;
  need(SUCCEEDED(d->CreateVertexBuffer(sizeof(vertices),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,&vb,nullptr)),"static location VB");
  need(SUCCEEDED(d->CreateIndexBuffer(sizeof(indices),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)),"static location IB");
  void*storage=nullptr;need(SUCCEEDED(vb->Lock(0,0,&storage,0)),"static location VB lock");memcpy(storage,vertices,sizeof(vertices));need(SUCCEEDED(vb->Unlock()),"static location VB unlock");
  need(SUCCEEDED(ib->Lock(0,0,&storage,0)),"static location IB lock");memcpy(storage,indices,sizeof(indices));need(SUCCEEDED(ib->Unlock()),"static location IB unlock");
  need(SUCCEEDED(d->SetStreamSource(0,vb,0,sizeof(V))),"static location stream");need(SUCCEEDED(d->SetIndices(ib)),"static location indices");
  unsigned previous=StormMetalBeginLandModel(d,role);need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"static location geometry");StormMetalEndLandModel(d,previous);
  need(SUCCEEDED(d->SetStreamSource(0,nullptr,0,0)),"static location stream reset");need(SUCCEEDED(d->SetIndices(nullptr)),"static location indices reset");vb->Release();ib->Release();
 };
 auto boundSkinnedQuad=[&](float x,float radius,float z,DWORD color,unsigned role){
  using namespace storm::metal::skinning;
  AnimatedVertex v[]={
   {{-(x-radius),-radius,z},1,0,{0,0,-1},int32_t(color),{0,0}},
   {{-(x+radius),-radius,z},1,0,{0,0,-1},int32_t(color),{1,0}},
   {{-(x+radius),radius,z},1,0,{0,0,-1},int32_t(color),{1,1}},
   {{-(x-radius),radius,z},1,0,{0,0,-1},int32_t(color),{0,1}}
  };
  Matrix4x4 palette[1]{};
  palette[0].elements[0]=palette[0].elements[5]=palette[0].elements[10]=palette[0].elements[15]=1;
  need(acceptVertices(v,4,1),"GPU accepts animated receiver vertices");
  need(beginPose(d,palette,1),"GPU begins animated receiver pose");
  auto*vb=static_cast<IDirect3DVertexBuffer9*>(bindVertices(v,0,4,4));
  need(vb,"GPU binds animated receiver vertices");
  const WORD ix[]={0,1,2,0,2,3};
  IDirect3DIndexBuffer9*ib=nullptr;
  need(SUCCEEDED(d->CreateIndexBuffer(sizeof(ix),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)),"skinned IB");
  void*p=nullptr;
  need(SUCCEEDED(ib->Lock(0,0,&p,0)),"skinned IB lock");
  memcpy(p,ix,sizeof(ix));
  need(SUCCEEDED(ib->Unlock()),"skinned IB unlock");
  // MODELR advertises its legacy 36-byte VERTEX0 output layout. The provider
  // recognizes the facade and switches the backend to AnimatedVertex's 44 bytes.
  need(SUCCEEDED(d->SetStreamSource(0,vb,0,36)),"GPU binds skinned stream");
  need(SUCCEEDED(d->SetIndices(ib)),"GPU binds skinned indices");
  unsigned previous=StormMetalBeginLandModel(d,role);
  need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"Metal-native GPU-skinned geometry");
  StormMetalEndLandModel(d,previous);
  need(SUCCEEDED(d->SetStreamSource(0,nullptr,0,0)),"GPU unbinds skinned stream");
  need(SUCCEEDED(d->SetIndices(nullptr)),"GPU unbinds skinned indices");
  ib->Release();
  endPose();
 };
 // The next frames use source replay, not the screen-visible receiver packets.
 // The directional caster lies outside the main camera's X and near planes.
 uint64_t generation=0;
 auto prepassFrame=[&](bool requestPointPass,bool caster,float casterX,bool matchingId,unsigned receiverRole=1,bool faded=false,bool indoor=false,float lampX=0,float sunEnergy=.7f,int catalog=0,float focusX=0,float contactGap=0,bool shadowSecond=false,bool canonicalAlpha=false,bool boundSkinned=false,bool boundStatic=false,float staticMaterial=.8f,bool staticColorSource=true,DWORD weatherAmbient=0x181818){
  d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,true);StormMetalLandInterior(d,indoor);
  d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
  d->SetRenderState(D3DRS_AMBIENT,weatherAmbient);d->SetRenderState(D3DRS_LIGHTING,FALSE);
  d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
  if(canonicalAlpha){d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);d->SetRenderState(D3DRS_ALPHAREF,0xa0);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_TEXTURE);}
  D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};sun.Diffuse={sunEnergy,sunEnergy,sunEnergy,1};d->SetLight(0,&sun);d->LightEnable(0,!requestPointPass);
  D3DLIGHT9 lamp{};lamp.Type=D3DLIGHT_POINT;lamp.Position={lampX,0,0};lamp.Diffuse={.8f,.8f,.8f,1};lamp.Range=indoor?12:24;lamp.Attenuation0=1;lamp.Attenuation2=indoor?.05f:0;d->SetLight(1,&lamp);d->LightEnable(1,requestPointPass);
  const float focus[]={focusX,0,8};++generation;need(StormMetalBeginShadowFrame(d,100+generation,generation,focus),"source prepass begins");
  if(catalog){StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,17,&lamp),"source lamp catalog");auto second=lamp;second.Position.x=4;second.Diffuse={.2f,.6f,.35f,1};need(StormMetalSceneLight(d,18,&second),"second source lamp catalog");}
  auto shadowLamp=lamp;if(shadowSecond){shadowLamp.Position.x=4;shadowLamp.Diffuse={.2f,.6f,.35f,1};}
  if(requestPointPass){StormMetalShadowLamp(d,&shadowLamp);if(indoor){d->LightEnable(1,FALSE);d->LightEnable(0,TRUE);}} // catalogued lamps remain independent of the camera-selected slot
  const float ray[]={.6f,0,1},position[]={shadowLamp.Position.x,0,0};
  if(requestPointPass){
   for(unsigned face=0;face<6;face++){
    D3DMATRIX lightView{},lightProjection{};need(StormMetalBeginShadowPass(d,1,face,shadowSecond?18:17,position,lamp.Range,&lightView.m[0][0],&lightProjection.m[0][0]),"selected indoor lamp cube face begins");
    d->SetTransform(D3DTS_VIEW,&lightView);d->SetTransform(D3DTS_PROJECTION,&lightProjection);quad(0,14,8,0xff202020,1);if(caster)quad(casterX,1.5f,6,0xff202020,0);StormMetalEndShadowPass(d);
   }
  }
  for(unsigned face=0;face<2;face++){
   D3DMATRIX lightView{},lightProjection{};
   need(StormMetalBeginShadowPass(d,0,face,1,ray,face?96:32,&lightView.m[0][0],&lightProjection.m[0][0]),"sun shadow cascade begins");
   d->SetTransform(D3DTS_VIEW,&lightView);d->SetTransform(D3DTS_PROJECTION,&lightProjection);
   quad(0,14,8,0xff202020,1);
   if(caster){
    if(canonicalAlpha&&receiverRole==2){d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,TRUE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);}
    quad(contactGap>0?-.6f*contactGap:(requestPointPass?casterX:-20),1.5f,contactGap>0?8-contactGap:-24,0xff202020,canonicalAlpha&&receiverRole==2?2:1);
    if(canonicalAlpha){d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_TEXTURE);}
   }
   StormMetalEndShadowPass(d);
  }
  const bool applied=StormMetalEndShadowFrame(d);need(applied,"sun source prepass resolves");
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
  if(boundSkinned){
   const bool localLampFixture=sunEnergy==0;
   boundSkinnedQuad(0,localLampFixture?4:14,localLampFixture?4:8,localLampFixture?0xff808080:0xff202020,receiverRole);
  }else if(boundStatic){
   D3DMATERIAL9 fixture{};fixture.Diffuse={staticMaterial,staticMaterial,staticMaterial,1};d->SetMaterial(&fixture);
   d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,TRUE);
   d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,staticColorSource?D3DMCS_COLOR1:D3DMCS_MATERIAL);
   d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,staticColorSource?D3DMCS_COLOR1:D3DMCS_MATERIAL);
   boundStaticQuad(0,14,8,0xff7f7f7f,receiverRole);
  }else quad(0,14,8,0xff202020,receiverRole); // no caster submitted to the visible color pass
  d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
  quad(11,1,2,0xff808080,0);d->LightEnable(2,FALSE);d->EndScene();return pixels();
 };
 auto sunClear=prepassFrame(false,false,0,true);prepassFrame(false,true,0,true);
 auto darkGpuSkinned=prepassFrame(false,false,0,true,2,false,true,0,0,0,0,0,false,false,true);
 // One selected point lamp owns six cube faces in both outdoor and indoor locations.
 auto directLamp=prepassFrame(true,false,0,true);
 need(directLamp[32*64+54]==sunClear[32*64+54],"unscoped overlay bypasses source shadows");
 prepassFrame(false,false,0,true,2);prepassFrame(false,true,0,true,2);
 prepassFrame(false,false,0,true,2,true);prepassFrame(false,true,0,true,2,true);
 prepassFrame(true,false,0,true,1,false,true);
 auto indoorCatalog=prepassFrame(true,false,0,true,1,false,true,0,.7f,1,-6);
 auto indoorCatalogMovedCamera=prepassFrame(true,false,0,true,1,false,true,0,.7f,2,6);
 need(indoorCatalog==indoorCatalogMovedCamera,"full indoor lamp catalog ignores camera light slots");
 auto litGpuSkinned=prepassFrame(true,false,0,true,2,false,true,0,0,1,0,0,false,false,true);
 auto energy=[](const std::vector<DWORD>&frame){uint64_t value=0;for(int y=16;y<48;y++)for(int x=16;x<48;x++){DWORD p=frame[y*64+x];value+=(p&255)+((p>>8)&255)+((p>>16)&255);}return value;};
 auto outdoorFillFrame=[&](DWORD ambient,bool skinned){
  // Seed a genuinely lamp-free scene after the preceding two-lamp fixture.
  StormMetalBeginSceneLights(d);
  for(DWORD slot=2;slot<8;++slot)d->LightEnable(slot,FALSE);
  return prepassFrame(false,false,0,true,skinned?2:1,false,false,0,0,0,0,0,false,false,skinned,!skinned,.8f,true,ambient);
 };
 for(bool skinned:{false,true}){
  const auto noAmbient=outdoorFillFrame(0,skinned),nightAmbient=outdoorFillFrame(0x181818,skinned),dayAmbient=outdoorFillFrame(0x696969,skinned);
  need(energy(noAmbient)>0,"lamp-free outdoor geometry retains visible base fill");
  need(noAmbient==nightAmbient,"zero and weak night ambient meet the same bounded sky floor");
  if(energy(dayAmbient)<=energy(nightAmbient))fprintf(stderr,"Outdoor ambient energy: skinned=%d night=%llu day=%llu\n",skinned,(unsigned long long)energy(nightAmbient),(unsigned long long)energy(dayAmbient));
  need(energy(dayAmbient)>energy(nightAmbient),"night fill remains below authored daytime lighting");
 }
 const auto darkGpuEnergy=energy(darkGpuSkinned),litGpuEnergy=energy(litGpuSkinned);
 if(litGpuEnergy<=darkGpuEnergy*2)fprintf(stderr,"GPU-skinned catalog energy: dark=%llu lit=%llu\n",static_cast<unsigned long long>(darkGpuEnergy),static_cast<unsigned long long>(litGpuEnergy));
 need(litGpuEnergy>darkGpuEnergy*2,"GPU-skinned character receives authored local lamp catalog");
 prepassFrame(true,true,-2,true,1,false,true,0,.7f,1,-6);
 prepassFrame(true,true,-2,true,1,false,true,0,.7f,1,0,0,false,true);
 prepassFrame(true,true,-2,true,2,false,true,0,.7f,1,0,0,false,true);
 prepassFrame(true,false,-2,true,2,false,true,0,.7f,1,0,0,false,true);
 auto expandedSkinned=prepassFrame(false,false,0,true,2,false,false,0,.7f,0,0,0,false,false,false);
 auto nativeSkinned=prepassFrame(false,false,0,true,2,false,false,0,.7f,0,0,0,false,false,true);
 need(expandedSkinned==nativeSkinned,"bound dynamic 36-byte skinned FVF matches compatibility lighting and shadows");
 need(StormMetalRawSkinnedDraws(d)>0,"bound dynamic skinned draw used the raw Metal vertex path");
 const auto rawLocationBefore=StormMetalRawLocationDraws(d);
 auto zeroMaterialLocation=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,true,0,true);
 auto ordinaryMaterialLocation=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,true,.8f,true);
 auto explicitZeroMaterial=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,true,0,false);
 need(StormMetalRawLocationDraws(d)>=rawLocationBefore+3,"static town fixtures used the raw Metal location path");
 need(zeroMaterialLocation==ordinaryMaterialLocation,"town COLOR1 lighting is independent of zero or ordinary material diffuse");
 need(energy(zeroMaterialLocation)>energy(explicitZeroMaterial)*2,"explicit material source remains dark while town COLOR1 stays visible");
 // An occluder outside the colour pass must still be registered without
 // painting over the visible receiver. A camera turn alone keeps the cube.
 D3DLIGHT9 source{};source.Type=D3DLIGHT_POINT;source.Position={0,0,0};source.Diffuse={.8f,.8f,.8f,1};source.Range=24;source.Attenuation0=1;
 const float center[]={0,0,8},lampPosition[]={0,0,0},casterCenter[]={0,0,6},farCenter[]={200,0,6};
 IDirect3DIndexBuffer9* registryIb=nullptr;const WORD registryIndices[]={0,1,2,0,2,3};void* registryStorage=nullptr;
 need(SUCCEEDED(d->CreateIndexBuffer(sizeof(registryIndices),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&registryIb,nullptr)),"persistent registry IB");registryIb->Lock(0,0,&registryStorage,0);memcpy(registryStorage,registryIndices,sizeof(registryIndices));registryIb->Unlock();
 auto makeRegistryQuad=[&](float radius,float z){const V vertices[]={{-radius,-radius,z,0,0,-1,0xff808080,0,0},{radius,-radius,z,0,0,-1,0xff808080,1,0},{radius,radius,z,0,0,-1,0xff808080,1,1},{-radius,radius,z,0,0,-1,0xff808080,0,1}};IDirect3DVertexBuffer9* vb=nullptr;need(SUCCEEDED(d->CreateVertexBuffer(sizeof(vertices),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,&vb,nullptr)),"persistent registry VB");vb->Lock(0,0,&registryStorage,0);memcpy(registryStorage,vertices,sizeof(vertices));vb->Unlock();return vb;};
 auto* receiverVb=makeRegistryQuad(14,8);auto* casterVb=makeRegistryQuad(1.5f,6);
 auto drawRegistryQuad=[&](IDirect3DVertexBuffer9*vb){d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(registryIb);auto scope=StormMetalBeginLandModel(d,1);need(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)),"persistent source geometry");StormMetalEndLandModel(d,scope);};
 auto registryFrame=[&](bool capture,float cameraOrigin=0.f){
  d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();StormMetalLandLocation(d,true);StormMetalLandInterior(d,true);
  for(unsigned i=0;i<8;i++)d->LightEnable(i,FALSE);
  const float frameOrigin[]={cameraOrigin,0,0};StormMetalSetWorldOrigin(d,frameOrigin);D3DMATRIX frameWorld=identity,frameView=identity;frameWorld._41=cameraOrigin;frameView._41=-cameraOrigin;d->SetTransform(D3DTS_WORLD,&frameWorld);d->SetTransform(D3DTS_VIEW,&frameView);d->SetTransform(D3DTS_PROJECTION,&projection);
  d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,TRUE);d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_COLOR1);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_COLOR1);d->SetRenderState(D3DRS_AMBIENT,0);
  d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
  need(StormMetalBeginShadowFrame(d,50001,++generation,center),"persistent scene begins");StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,701,&source),"persistent scene source");StormMetalRenderPointShadowCube(d,701,lampPosition,24);StormMetalEndShadowFrame(d);
  drawRegistryQuad(receiverVb);const auto before=pixels();
  need(!StormMetalBeginOffscreenPointCaster(d,farCenter,1.5f),"off-camera geometry outside lamp range stays culled");
  if(capture){need(StormMetalBeginOffscreenPointCaster(d,casterCenter,1.5f),"off-camera nearby caster admitted");need(!StormMetalBeginOffscreenPointCaster(d,farCenter,1.5f),"nested out-of-range caster remains culled");drawRegistryQuad(casterVb);StormMetalEndOffscreenPointCaster(d);need(before==pixels(),"off-camera caster never paints the colour target");}
  d->EndScene();need(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"registry frame reset");return before;
 };
 registryFrame(true);const auto shadowWithCaster=registryFrame(true);const auto cubeSubmissions=StormMetalPointShadowGpuCullDispatches(d);registryFrame(true);
 need(StormMetalPointShadowGpuCullDispatches(d)==cubeSubmissions,"unchanged scene reuses its point depth cube");
 registryFrame(true,-100);registryFrame(true,-100);need(StormMetalPointShadowGpuCullDispatches(d)==cubeSubmissions,"camera origin changes do not rebuild physical caster cubes");
 registryFrame(false);const auto shadowWithoutCaster=registryFrame(false);
 need(energy(shadowWithoutCaster)>energy(shadowWithCaster),"removing the source caster invalidates its shadow");
 // Before a first cube exists, catalog positions are rebased while NODER
 // supplies world centers. Admission must agree with the warmed-up cube path.
 d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
 const float origin[]={-100,0,0},translatedCaster[]={100,0,6},translatedFar[]={300,0,6},translatedLamp[]={100,0,0};
 StormMetalSetWorldOrigin(d,origin);source.Position.x=100;
 need(StormMetalBeginShadowFrame(d,60002,++generation,translatedCaster),"translated empty scene begins");StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,702,&source),"translated physical source");StormMetalRenderPointShadowCube(d,702,translatedLamp,24);need(!StormMetalEndShadowFrame(d),"empty scene has not produced a cube yet");
 need(StormMetalBeginOffscreenPointCaster(d,translatedCaster,1.5f),"first-frame off-camera admission preserves world origin");StormMetalEndOffscreenPointCaster(d);
 need(!StormMetalBeginOffscreenPointCaster(d,translatedFar,1.5f),"translated out-of-range caster remains culled");
 const float zeroOrigin[]={0,0,0};StormMetalSetWorldOrigin(d,zeroOrigin);d->EndScene();d->Present(nullptr,nullptr,nullptr,nullptr);
 d->SetStreamSource(0,nullptr,0,0);d->SetIndices(nullptr);receiverVb->Release();casterVb->Release();registryIb->Release();
 puts("PASS sun source prepass, role1/role2 paths, town COLOR1 materials, direct lamp catalog, selected outdoor/indoor point cube");
 d->SetTexture(0,nullptr);t->Release();read->Release();target->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS dynamic lighting and real caster GPU fixture");}

#include <initializer_list>
#include <vector>
#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gpu_skinning_bridge.hpp"
static void need(bool x,const char*s){if(!x){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
extern "C" void StormMetalShadowLamp(void*,const storm_render::Light*);
extern "C" void StormMetalBeginSceneLights(void*);
extern "C" bool StormMetalSceneLight(void*,uint64_t,const storm_render::Light*);
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
extern "C" bool StormMetalApplyGraphicsSettings(void*,uint32_t,uint32_t);
int main(){setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);setenv("STORM_METAL_MODERN_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Weapon FFP contract",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");storm_render::Presentation p{};p.width=p.height=64;p.colorFormat=storm_render::AssetFormat::A8R8G8B8;storm_render::Device*d=nullptr;need(storm_render::succeeded(storm_render::createMetalDevice(w, &p, &d)),"device");
 storm_render::Texture*t=nullptr;d->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &t);storm_render::LockedRect l{};t->LockRect(0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((uint32_t*)((char*)l.pBits+y*l.Pitch))[x]=0xff204060;t->UnlockRect(0);

 storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;storm_render::Matrix projection=identity;projection._11=projection._22=1.f/16;projection._33=1.f/32;d->SetTransform(storm_render::TransformWorld,&identity);d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&projection);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);
 struct V{float x,y,z,nx,ny,nz;uint32_t color;float u,v;};
 storm_render::Surface*target=nullptr,*read=nullptr;d->GetRenderTarget(&target);d->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &read);
 auto pixels=[&](){need(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect q{};read->LockRect(&q,nullptr,storm_render::lock::readOnly);std::vector<uint32_t> out(4096);for(int y=0;y<64;y++)memcpy(out.data()+y*64,(char*)q.pBits+y*q.Pitch,256);read->UnlockRect();return out;};
 auto quad=[&](float x,float radius,float z,uint32_t color,unsigned role){V v[]={{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,-radius,z,0,0,-1,color,1,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,radius,z,0,0,-1,color,0,1}};unsigned previous=StormMetalBeginLandModel(d,role);need(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,v,sizeof(V))),"scoped geometry");StormMetalEndLandModel(d,previous);};
 auto boundStaticQuad=[&](float x,float radius,float z,uint32_t color,unsigned role){
  const V vertices[]={
   {x-radius,-radius,z,0,0,-1,color,0,0},
   {x+radius,-radius,z,0,0,-1,color,1,0},
   {x+radius,radius,z,0,0,-1,color,1,1},
   {x-radius,radius,z,0,0,-1,color,0,1}
  };
  const uint16_t indices[]={0,1,2,0,2,3};
  storm_render::VertexBuffer*vb=nullptr;storm_render::IndexBuffer*ib=nullptr;
  need(storm_render::succeeded(d->CreateVertexBuffer(sizeof(vertices), storm_render::usage::writeOnly, 0, &vb)),"static location VB");
  need(storm_render::succeeded(d->CreateIndexBuffer(sizeof(indices), storm_render::usage::writeOnly, storm_render::IndexFormat::UInt16, &ib)),"static location IB");
  void*storage=nullptr;need(storm_render::succeeded(vb->Lock(0,0,&storage,0)),"static location VB lock");memcpy(storage,vertices,sizeof(vertices));need(storm_render::succeeded(vb->Unlock()),"static location VB unlock");
  need(storm_render::succeeded(ib->Lock(0,0,&storage,0)),"static location IB lock");memcpy(storage,indices,sizeof(indices));need(storm_render::succeeded(ib->Unlock()),"static location IB unlock");
  need(storm_render::succeeded(d->SetStreamSource(0,vb,0,sizeof(V))),"static location stream");need(storm_render::succeeded(d->SetIndices(ib)),"static location indices");
  unsigned previous=StormMetalBeginLandModel(d,role);need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"static location geometry");StormMetalEndLandModel(d,previous);
  need(storm_render::succeeded(d->SetStreamSource(0,nullptr,0,0)),"static location stream reset");need(storm_render::succeeded(d->SetIndices(nullptr)),"static location indices reset");vb->Release();ib->Release();
 };
 auto boundSkinnedQuad=[&](float x,float radius,float z,uint32_t color,unsigned role){
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
  auto*vb=static_cast<storm_render::VertexBuffer*>(bindVertices(v,0,4,4));
  need(vb,"GPU binds animated receiver vertices");
  const uint16_t ix[]={0,1,2,0,2,3};
  storm_render::IndexBuffer*ib=nullptr;
  need(storm_render::succeeded(d->CreateIndexBuffer(sizeof(ix), storm_render::usage::writeOnly, storm_render::IndexFormat::UInt16, &ib)),"skinned IB");
  void*p=nullptr;
  need(storm_render::succeeded(ib->Lock(0,0,&p,0)),"skinned IB lock");
  memcpy(p,ix,sizeof(ix));
  need(storm_render::succeeded(ib->Unlock()),"skinned IB unlock");
  // MODELR advertises its legacy 36-byte VERTEX0 output layout. The provider
  // recognizes the facade and switches the backend to AnimatedVertex's 44 bytes.
  need(storm_render::succeeded(d->SetStreamSource(0,vb,0,36)),"GPU binds skinned stream");
  need(storm_render::succeeded(d->SetIndices(ib)),"GPU binds skinned indices");
  unsigned previous=StormMetalBeginLandModel(d,role);
  need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"Metal-native GPU-skinned geometry");
  StormMetalEndLandModel(d,previous);
  need(storm_render::succeeded(d->SetStreamSource(0,nullptr,0,0)),"GPU unbinds skinned stream");
  need(storm_render::succeeded(d->SetIndices(nullptr)),"GPU unbinds skinned indices");
  ib->Release();
  endPose();
  invalidateVertices(v);
 };
 // The next frames use source replay, not the screen-visible receiver packets.
 // The directional caster lies outside the main camera's X and near planes.
 uint64_t generation=0;
 auto prepassFrame=[&](bool requestPointPass,bool caster,float casterX,bool matchingId,unsigned receiverRole=1,bool faded=false,bool indoor=false,float lampX=0,float sunEnergy=.7f,int catalog=0,float focusX=0,float contactGap=0,bool shadowSecond=false,bool canonicalAlpha=false,bool boundSkinned=false,bool boundStatic=false,float staticMaterial=.8f,bool staticColorSource=true,uint32_t weatherAmbient=0x181818,bool expandedStatic=false){
  d->Present();StormMetalLandLocation(d,true);StormMetalLandInterior(d,indoor);
  d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpModulate2X);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageColorArg2,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);
  d->SetRenderState(storm_render::RenderAmbient,weatherAmbient);d->SetRenderState(storm_render::RenderLighting,false);
  d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderAlphaTest,false);
  if(canonicalAlpha){d->SetRenderState(storm_render::RenderAlphaTest,true);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,0xa0);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpModulate);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaArg2,storm_render::textureArg::texture);}
  storm_render::Light sun{};sun.Type=storm_render::LightKind::Directional;sun.Direction={.6f,0,1};sun.Diffuse={sunEnergy,sunEnergy,sunEnergy,1};d->SetLight(0,&sun);d->LightEnable(0,!requestPointPass);
  storm_render::Light lamp{};lamp.Type=storm_render::LightKind::Point;lamp.Position={lampX,0,0};lamp.Diffuse={.8f,.8f,.8f,1};lamp.Range=indoor?12:24;lamp.Attenuation0=1;lamp.Attenuation2=indoor?.05f:0;d->SetLight(1,&lamp);d->LightEnable(1,requestPointPass);
  const float focus[]={focusX,0,8};++generation;need(StormMetalBeginShadowFrame(d,100+generation,generation,focus),"source prepass begins");
  StormMetalBeginSceneLights(d);
  if(catalog)need(StormMetalSceneLight(d,(uint64_t(1)<<63)|17,&lamp),"ship source lamp catalog");
  auto shadowLamp=lamp;if(shadowSecond){shadowLamp.Position.x=4;shadowLamp.Diffuse={.2f,.6f,.35f,1};}
  if(requestPointPass){StormMetalShadowLamp(d,&shadowLamp);if(indoor){d->LightEnable(1,false);d->LightEnable(0,true);}} // catalogued lamps remain independent of the camera-selected slot
  const float ray[]={.6f,0,1},position[]={shadowLamp.Position.x,0,0};
  if(requestPointPass){
   for(unsigned face=0;face<6;face++){
    storm_render::Matrix lightView{},lightProjection{};need(StormMetalBeginShadowPass(d,1,face,shadowSecond?18:17,position,lamp.Range,&lightView.m[0][0],&lightProjection.m[0][0]),"selected indoor lamp cube face begins");
    d->SetTransform(storm_render::TransformView,&lightView);d->SetTransform(storm_render::TransformProjection,&lightProjection);quad(0,14,8,0xff202020,1);if(caster)quad(casterX,1.5f,6,0xff202020,0);StormMetalEndShadowPass(d);
   }
  }
  for(unsigned face=0;face<2;face++){
   storm_render::Matrix lightView{},lightProjection{};
   need(StormMetalBeginShadowPass(d,0,face,1,ray,face?96:32,&lightView.m[0][0],&lightProjection.m[0][0]),"sun shadow cascade begins");
   d->SetTransform(storm_render::TransformView,&lightView);d->SetTransform(storm_render::TransformProjection,&lightProjection);
   quad(0,14,8,0xff202020,1);
   if(caster){
    if(canonicalAlpha&&receiverRole==2){d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,true);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaArg2,storm_render::textureArg::current);}
    quad(contactGap>0?-.6f*contactGap:(requestPointPass?casterX:-20),1.5f,contactGap>0?8-contactGap:-24,0xff202020,canonicalAlpha&&receiverRole==2?2:1);
    if(canonicalAlpha){d->SetRenderState(storm_render::RenderLighting,false);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaArg2,storm_render::textureArg::texture);}
   }
   StormMetalEndShadowPass(d);
  }
  const bool applied=StormMetalEndShadowFrame(d);need(applied,"sun source prepass resolves");
  d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&projection);
  StormMetalSetLightIdentity(d,1,(uint64_t(1)<<63)|17);
  d->LightEnable(1,catalog!=0);
  for(unsigned slot=2;slot<8;++slot)d->LightEnable(slot,false);
  if(receiverRole==2){
   // CPU-skinned Animation output uses this same XYZ/NORMAL/DIFFUSE/TEX1
   // declaration. Its normal and animated position are consumed in world space.
   d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,true);d->SetRenderState(storm_render::RenderAmbientMaterialSource,storm_render::MaterialSourceDiffuseColor);
   d->SetRenderState(storm_render::RenderAlphaTest,!faded);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,0);
   d->SetTextureStageState(0,storm_render::StageAlphaOp,faded?storm_render::TextureOpSelectArg1:storm_render::TextureOpModulate);
   d->SetTextureStageState(0,storm_render::StageAlphaArg1,faded?storm_render::textureArg::factor:storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaArg2,storm_render::textureArg::current);
   d->SetRenderState(storm_render::RenderTextureFactor,0x66000000);d->SetRenderState(storm_render::RenderDepthWrite,!faded);d->SetRenderState(storm_render::RenderBlend,faded);d->SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendSourceAlpha);d->SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendInverseSourceAlpha);
  }
  if(boundSkinned){
   const bool localLampFixture=sunEnergy==0;
   boundSkinnedQuad(0,localLampFixture?4:14,localLampFixture?4:8,localLampFixture?0xff808080:0xff202020,receiverRole);
  }else if(boundStatic){
   storm_render::Material fixture{};fixture.Diffuse={staticMaterial,staticMaterial,staticMaterial,1};d->SetMaterial(&fixture);
   d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,true);
   d->SetRenderState(storm_render::RenderDiffuseMaterialSource,staticColorSource?storm_render::MaterialSourceDiffuseColor:storm_render::MaterialSourceMaterial);
   d->SetRenderState(storm_render::RenderAmbientMaterialSource,staticColorSource?storm_render::MaterialSourceDiffuseColor:storm_render::MaterialSourceMaterial);
   if(expandedStatic)quad(0,14,8,0xff7f7f7f,receiverRole);
   else boundStaticQuad(0,14,8,0xff7f7f7f,receiverRole);
  }else quad(0,14,8,0xff202020,receiverRole); // no caster submitted to the visible color pass
  d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
  quad(11,1,2,0xff808080,0);d->LightEnable(2,false);d->EndScene();return pixels();
 };
 auto energy=[](const std::vector<uint32_t>&frame){uint64_t value=0;for(int y=16;y<48;y++)for(int x=16;x<48;x++){uint32_t p=frame[y*64+x];value+=(p&255)+((p>>8)&255)+((p>>16)&255);}return value;};
 auto receiverFrame=[&](unsigned role,bool expanded,bool lamp){
  return prepassFrame(false,false,0,true,role,false,false,0,0,lamp?1:0,0,0,false,false,false,true,.8f,true,0,expanded);
 };
 for(unsigned role:{1u,2u}){
  // Both frames carry the same immutable rigid FVF/material and source catalog.
  // Role 2 is the harbor SHIP consumer; role 1 is its existing main-raw neighbor.
  const auto darkRaw=receiverFrame(role,false,false),darkExpanded=receiverFrame(role,true,false);
  const auto litRaw=receiverFrame(role,false,true),litExpanded=receiverFrame(role,true,true);
  unsigned darkChanged=0,litChanged=0;for(size_t i=0;i<litRaw.size();++i){darkChanged+=darkRaw[i]!=darkExpanded[i];litChanged+=litRaw[i]!=litExpanded[i];}
  std::printf("boat-catalog role=%u darkChanged=%u litChanged=%u energy(darkRaw,darkExpanded,litRaw,litExpanded)=%llu,%llu,%llu,%llu\n",role,darkChanged,litChanged,(unsigned long long)energy(darkRaw),(unsigned long long)energy(darkExpanded),(unsigned long long)energy(litRaw),(unsigned long long)energy(litExpanded));
  std::fflush(stdout);
  need(darkRaw==darkExpanded,"lamp-free rigid receiver parity");
  need(energy(litExpanded)>energy(darkExpanded)*2,"expanded receiver receives physical ship lamp");
  need(litRaw==litExpanded,"bound rigid receiver retains the same physical ship lamp");
 }
 d->SetTexture(0,nullptr);t->Release();read->Release();target->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS rigid harbor receiver catalog and lamp-free controls");}

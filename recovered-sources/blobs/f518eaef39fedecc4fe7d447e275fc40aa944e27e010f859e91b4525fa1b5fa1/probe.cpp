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
 bool authoredStaticFixture=false;uint64_t generation=0;
 auto prepassFrame=[&](bool requestPointPass,bool caster,float casterX,bool matchingId,unsigned receiverRole=1,bool faded=false,bool indoor=false,float lampX=0,float sunEnergy=.7f,int catalog=0,float focusX=0,float contactGap=0,bool shadowSecond=false,bool canonicalAlpha=false,bool boundSkinned=false,bool boundStatic=false,float staticMaterial=.8f,bool staticColorSource=true,uint32_t weatherAmbient=0x181818,bool expandedStatic=false,bool maps=true){
  d->Present();StormMetalLandLocation(d,true);StormMetalLandInterior(d,indoor);
  d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpModulate2X);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageColorArg2,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);
  d->SetRenderState(storm_render::RenderAmbient,weatherAmbient);d->SetRenderState(storm_render::RenderLighting,false);
  d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderAlphaTest,false);
  if(canonicalAlpha){d->SetRenderState(storm_render::RenderAlphaTest,true);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,0xa0);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpModulate);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaArg2,storm_render::textureArg::texture);}
  storm_render::Light sun{};sun.Type=storm_render::LightKind::Directional;sun.Direction={.6f,0,1};sun.Diffuse={sunEnergy,sunEnergy,sunEnergy,1};d->SetLight(0,&sun);d->LightEnable(0,!requestPointPass);
  storm_render::Light lamp{};lamp.Type=storm_render::LightKind::Point;lamp.Position={lampX,0,0};lamp.Diffuse={.8f,.8f,.8f,1};lamp.Range=indoor?12:24;lamp.Attenuation0=1;lamp.Attenuation2=indoor?.05f:0;d->SetLight(1,&lamp);d->LightEnable(1,requestPointPass);
  const float focus[]={focusX,0,8};++generation;need(StormMetalBeginShadowFrame(d,100+generation,generation,focus),"source prepass begins");
  if(catalog){StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,17,&lamp),"source lamp catalog");auto second=lamp;second.Position.x=4;second.Diffuse={.2f,.6f,.35f,1};need(StormMetalSceneLight(d,18,&second),"second source lamp catalog");}
  auto shadowLamp=lamp;if(shadowSecond){shadowLamp.Position.x=4;shadowLamp.Diffuse={.2f,.6f,.35f,1};}
  if(requestPointPass){StormMetalShadowLamp(d,&shadowLamp);if(indoor){d->LightEnable(1,false);d->LightEnable(0,true);}} // catalogued lamps remain independent of the camera-selected slot
  const float ray[]={.6f,0,1},position[]={shadowLamp.Position.x,0,0};
  if(maps&&requestPointPass){
   for(unsigned face=0;face<6;face++){
    storm_render::Matrix lightView{},lightProjection{};need(StormMetalBeginShadowPass(d,1,face,shadowSecond?18:17,position,lamp.Range,&lightView.m[0][0],&lightProjection.m[0][0]),"selected indoor lamp cube face begins");
    d->SetTransform(storm_render::TransformView,&lightView);d->SetTransform(storm_render::TransformProjection,&lightProjection);quad(0,14,8,0xff202020,1);if(caster)quad(casterX,1.5f,6,0xff202020,0);StormMetalEndShadowPass(d);
   }
  }
  for(unsigned face=0;maps&&face<2;face++){
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
  const bool applied=StormMetalEndShadowFrame(d);need(applied==maps,"map validity follows map work");
  d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&projection);
  StormMetalSetLightIdentity(d,1,matchingId?17:99);
  if(catalog){auto cameraSelected=lamp;cameraSelected.Position.x=catalog==1?-5:5;cameraSelected.Diffuse=catalog==1?storm_render::Color4{1,0,0,1}:storm_render::Color4{0,0,1,1};d->SetLight(2,&cameraSelected);d->LightEnable(2,true);StormMetalSetLightIdentity(d,2,catalog==1?31:32);}
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
   d->SetRenderState(storm_render::RenderLighting,!authoredStaticFixture);d->SetRenderState(storm_render::RenderVertexColor,true);
   d->SetRenderState(storm_render::RenderDiffuseMaterialSource,staticColorSource?storm_render::MaterialSourceDiffuseColor:storm_render::MaterialSourceMaterial);
   d->SetRenderState(storm_render::RenderAmbientMaterialSource,staticColorSource?storm_render::MaterialSourceDiffuseColor:storm_render::MaterialSourceMaterial);
   if(expandedStatic)quad(0,14,8,0xff7f7f7f,receiverRole);
   else boundStaticQuad(0,14,8,0xff7f7f7f,receiverRole);
  }else quad(0,14,8,0xff202020,receiverRole); // no caster submitted to the visible color pass
  d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
  quad(11,1,2,0xff808080,0);d->LightEnable(2,false);d->EndScene();return pixels();
 };
 auto sunClear=prepassFrame(false,false,0,true);



 authoredStaticFixture=true;
 auto prelitFrame=[&](unsigned role,bool expanded,bool maps,bool blocker){return prepassFrame(false,blocker,0,true,role,false,false,0,.7f,0,0,0,false,false,false,true,.8f,true,0x181818,expanded,maps);};
 for(bool modern:{false,true}){
  need(StormMetalApplyGraphicsSettings(d,1u|(modern?4u:0u),2),"prelit setting");
  auto authoredRaw=prelitFrame(0,false,false,false),raw=prelitFrame(1,false,false,false),expanded=prelitFrame(1,true,false,false);
  need(authoredRaw==raw&&raw==expanded,"prelit raw/expanded preserve authored original without maps");
  auto clear=prelitFrame(1,false,true,false),blocked=prelitFrame(1,false,true,true);
  need(clear!=blocked,"prelit base receives a real character shadow");
  need(clear[32*64+54]==blocked[32*64+54],"prelit unoccluded neighbor retains its base");
 }
 authoredStaticFixture=false;
 auto authored=prepassFrame(false,false,0,true,0,false,false,0,.7f,0,0,0,false,false,false,false,.8f,true,0x181818,false,false);
 auto admitted=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,false,.8f,true,0x181818,false,false);
 need(authored==admitted,"unoccluded prelit static retains authored COLOR1 base");
 auto noMapFrame=[&](bool shadows,bool skin,bool indoor){
  need(StormMetalApplyGraphicsSettings(d,(shadows?1u:0u)|4u,2),"toggle map production");
  StormMetalBeginSceneLights(d);
  return prepassFrame(false,false,0,true,skin?2:1,false,indoor,0,.7f,0,0,0,false,false,skin,!skin,.8f,true,0x181818,false,false);
 };
 for(bool indoor:{false,true})for(bool skin:{false,true}){
  auto on=noMapFrame(true,skin,indoor),off=noMapFrame(false,skin,indoor);
  need(on==off,"shadow toggle preserves unoccluded location illumination");
 }
 need(StormMetalApplyGraphicsSettings(d,5,2),"restore shadows");
 puts("PASS map-off illumination parity after a valid prior frame");
prepassFrame(false,true,0,true);
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
 auto energy=[](const std::vector<uint32_t>&frame){uint64_t value=0;for(int y=16;y<48;y++)for(int x=16;x<48;x++){uint32_t p=frame[y*64+x];value+=(p&255)+((p>>8)&255)+((p>>16)&255);}return value;};
 auto outdoorFillFrame=[&](uint32_t ambient,bool skinned){
  // Seed a genuinely lamp-free scene after the preceding two-lamp fixture.
  StormMetalBeginSceneLights(d);
  for(uint32_t slot=2;slot<8;++slot)d->LightEnable(slot,false);
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
 // This lamp reaches the animated quad's right edge, but not its model origin.
 // A zero facade AABB wrongly rejects it before the shader sees the catalog.
 auto edgeLampGpuSkinned=prepassFrame(true,false,0,true,2,false,true,14,0,1,0,0,false,false,true);
 need(energy(edgeLampGpuSkinned)>darkGpuEnergy,"posed bounds admit a lamp reaching vertices outside the model origin");
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
 // The setting controls the response, independent of resident/raw versus
 // expanded FFP submission. Keep both setting states in this existing oracle.
 for(bool modern:{false,true}){
  need(StormMetalApplyGraphicsSettings(d,1u|(modern?4u:0u),1),"apply modern lighting setting");
  auto raw=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,true,.8f,true,0x696969,false);
  auto expanded=prepassFrame(false,false,0,true,1,false,false,0,.7f,0,0,0,false,false,false,true,.8f,true,0x696969,true);
  unsigned changed=0;for(size_t i=0;i<raw.size();++i)changed+=raw[i]!=expanded[i];
  std::printf("Modern lighting parity modern=%d changed=%u\n",modern,changed);
  need(raw==expanded,"modern lighting option preserves raw/expanded FFP parity");
 }
 // An occluder outside the colour pass must still be registered without
 // painting over the visible receiver. A camera turn alone keeps the cube.
 storm_render::Light source{};source.Type=storm_render::LightKind::Point;source.Position={0,0,0};source.Diffuse={.8f,.8f,.8f,1};source.Range=24;source.Attenuation0=1;
 const float center[]={0,0,8},lampPosition[]={0,0,0},casterCenter[]={0,0,6},farCenter[]={200,0,6};
 storm_render::IndexBuffer* registryIb=nullptr;const uint16_t registryIndices[]={0,1,2,0,2,3};void* registryStorage=nullptr;
 need(storm_render::succeeded(d->CreateIndexBuffer(sizeof(registryIndices), storm_render::usage::writeOnly, storm_render::IndexFormat::UInt16, &registryIb)),"persistent registry IB");registryIb->Lock(0,0,&registryStorage,0);memcpy(registryStorage,registryIndices,sizeof(registryIndices));registryIb->Unlock();
 auto makeRegistryQuad=[&](float radius,float z){const V vertices[]={{-radius,-radius,z,0,0,-1,0xff808080,0,0},{radius,-radius,z,0,0,-1,0xff808080,1,0},{radius,radius,z,0,0,-1,0xff808080,1,1},{-radius,radius,z,0,0,-1,0xff808080,0,1}};storm_render::VertexBuffer* vb=nullptr;need(storm_render::succeeded(d->CreateVertexBuffer(sizeof(vertices), storm_render::usage::writeOnly, 0, &vb)),"persistent registry VB");vb->Lock(0,0,&registryStorage,0);memcpy(registryStorage,vertices,sizeof(vertices));vb->Unlock();return vb;};
 auto* receiverVb=makeRegistryQuad(14,8);auto* casterVb=makeRegistryQuad(1.5f,6);
 auto drawRegistryQuad=[&](storm_render::VertexBuffer*vb){d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(registryIb);auto scope=StormMetalBeginLandModel(d,1);need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"persistent source geometry");StormMetalEndLandModel(d,scope);};
 auto registryFrame=[&](bool capture,float cameraOrigin=0.f,bool warmCandidate=false){
  d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();StormMetalLandLocation(d,true);StormMetalLandInterior(d,true);
  for(unsigned i=0;i<8;i++)d->LightEnable(i,false);
  const float frameOrigin[]={cameraOrigin,0,0};StormMetalSetWorldOrigin(d,frameOrigin);storm_render::Matrix frameWorld=identity,frameView=identity;frameWorld._41=cameraOrigin;frameView._41=-cameraOrigin;d->SetTransform(storm_render::TransformWorld,&frameWorld);d->SetTransform(storm_render::TransformView,&frameView);d->SetTransform(storm_render::TransformProjection,&projection);
  d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,true);d->SetRenderState(storm_render::RenderDiffuseMaterialSource,storm_render::MaterialSourceDiffuseColor);d->SetRenderState(storm_render::RenderAmbientMaterialSource,storm_render::MaterialSourceDiffuseColor);d->SetRenderState(storm_render::RenderAmbient,0);
  d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderDepthTest,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
  need(StormMetalBeginShadowFrame(d,50001,++generation,center),"persistent scene begins");StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,701,&source),"persistent scene source");if(warmCandidate){auto candidate=source;candidate.Position.x=30;candidate.Range=12;need(StormMetalSceneLight(d,702,&candidate),"view-eligible unselected source");}StormMetalRenderPointShadowCube(d,701,lampPosition,24);StormMetalEndShadowFrame(d);
  drawRegistryQuad(receiverVb);const auto before=pixels();
  need(!StormMetalBeginOffscreenPointCaster(d,farCenter,1.5f),"off-camera geometry outside lamp range stays culled");
  if(warmCandidate){const float candidateCaster[]={30,0,6};need(StormMetalBeginOffscreenPointCaster(d,candidateCaster,1.5f),"unselected view-eligible lamp warms its offscreen caster before admission");StormMetalEndOffscreenPointCaster(d);need(before==pixels(),"warming an unselected lamp never paints the colour target");}
  if(capture){need(StormMetalBeginOffscreenPointCaster(d,casterCenter,1.5f),"off-camera nearby caster admitted");need(!StormMetalBeginOffscreenPointCaster(d,farCenter,1.5f),"nested out-of-range caster remains culled");drawRegistryQuad(casterVb);StormMetalEndOffscreenPointCaster(d);need(before==pixels(),"off-camera caster never paints the colour target");}
  d->EndScene();need(storm_render::succeeded(d->Present()),"registry frame reset");return before;
 };
 registryFrame(true);const auto shadowWithCaster=registryFrame(true);const auto cubeSubmissions=StormMetalPointShadowGpuCullDispatches(d);registryFrame(true);
 need(StormMetalPointShadowGpuCullDispatches(d)==cubeSubmissions,"unchanged scene reuses its point depth cube");
 registryFrame(true,0,true);need(StormMetalPointShadowGpuCullDispatches(d)==cubeSubmissions,"warming eligible sources does not allocate additional cube faces");
 registryFrame(true,-100);registryFrame(true,-100);need(StormMetalPointShadowGpuCullDispatches(d)==cubeSubmissions,"camera origin changes do not rebuild physical caster cubes");
 registryFrame(false);const auto shadowWithoutCaster=registryFrame(false);
 need(energy(shadowWithoutCaster)>energy(shadowWithCaster),"removing the source caster invalidates its shadow");
 // Before a first cube exists, catalog positions are rebased while NODER
 // supplies world centers. Admission must agree with the warmed-up cube path.
 d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();
 const float origin[]={-100,0,0},translatedCaster[]={100,0,6},translatedFar[]={300,0,6},translatedLamp[]={100,0,0};
 StormMetalSetWorldOrigin(d,origin);source.Position.x=100;
 need(StormMetalBeginShadowFrame(d,60002,++generation,translatedCaster),"translated empty scene begins");StormMetalBeginSceneLights(d);need(StormMetalSceneLight(d,702,&source),"translated physical source");StormMetalRenderPointShadowCube(d,702,translatedLamp,24);need(!StormMetalEndShadowFrame(d),"empty scene has not produced a cube yet");
 need(StormMetalBeginOffscreenPointCaster(d,translatedCaster,1.5f),"first-frame off-camera admission preserves world origin");StormMetalEndOffscreenPointCaster(d);
 need(!StormMetalBeginOffscreenPointCaster(d,translatedFar,1.5f),"translated out-of-range caster remains culled");
 const float zeroOrigin[]={0,0,0};StormMetalSetWorldOrigin(d,zeroOrigin);d->EndScene();d->Present();
 d->SetStreamSource(0,nullptr,0,0);d->SetIndices(nullptr);receiverVb->Release();casterVb->Release();registryIb->Release();
 puts("PASS sun source prepass, role1/role2 paths, town COLOR1 materials, direct lamp catalog, selected outdoor/indoor point cube");
 d->SetTexture(0,nullptr);t->Release();read->Release();target->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS dynamic lighting and real caster GPU fixture");}

#include <SDL.h>
#include "render_device_api.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalBeginShadowFrame(void*,uint64_t,uint64_t,const float*);
extern "C" bool StormMetalBeginShadowPass(void*,unsigned,unsigned,uint64_t,const float*,float,float*,float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
static void need(bool value,const char*name){if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
int main(){
 setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);
 auto*w=SDL_CreateWindow("Character contact shadow",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");
 storm_render::Presentation pp{};pp.width=pp.height=64;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
 storm_render::Device*d=nullptr;need(storm_render::succeeded(storm_render::createMetalDevice(w, &pp, &d)),"device");
 storm_render::Texture*texture=nullptr;need(storm_render::succeeded(d->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &texture)),"texture");storm_render::LockedRect locked{};texture->LockRect(0,&locked,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)reinterpret_cast<uint32_t*>(static_cast<char*>(locked.pBits)+y*locked.Pitch)[x]=0xff6080a0;texture->UnlockRect(0);
 storm_render::Surface*target=nullptr,*readback=nullptr;d->GetRenderTarget(&target);d->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback);
 storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;storm_render::Matrix projection=identity;projection._11=projection._22=1.f/8;projection._33=1.f/32;
 struct Vertex{float x,y,z,nx,ny,nz,u,v;};
 auto draw=[&](float radius,float z,unsigned role,bool sceneModel){Vertex v[]={{-radius,-radius,z,0,0,-1,0,0},{radius,-radius,z,0,0,-1,1,0},{radius,radius,z,0,0,-1,1,1},{-radius,-radius,z,0,0,-1,0,0},{radius,radius,z,0,0,-1,1,1},{-radius,radius,z,0,0,-1,0,1}};unsigned previous=sceneModel?StormMetalBeginSceneModel(d):StormMetalBeginLandModel(d,role);need(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,v,sizeof(Vertex))),"geometry");StormMetalEndLandModel(d,previous);};
 auto quad=[&](float radius,float z,unsigned role){draw(radius,z,role,false);};
 uint64_t frame=0;
 auto render=[&](float gap,bool blendedCaster=false,bool translucentCaster=false){
  d->Present();StormMetalLandLocation(d,true);StormMetalLandInterior(d,false);d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();
  d->SetTransform(storm_render::TransformWorld,&identity);d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::tex1);d->SetTexture(0,texture);d->SetTexture(1,nullptr);
  d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,false);d->SetRenderState(storm_render::RenderAmbient,0x181818);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderDepthTest,true);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderDepthCompare,storm_render::CompareLessEqual);d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderFog,false);
  d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpModulate);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageColorArg2,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);
  storm_render::Material material{};material.Diffuse=material.Ambient={1,1,1,1};d->SetMaterial(&material);storm_render::Light sun{};sun.Type=storm_render::LightKind::Directional;sun.Direction={.6f,0,1};sun.Diffuse={.7f,.7f,.7f,1};d->SetLight(0,&sun);d->LightEnable(0,true);
  float center[]={0,0,8},ray[]={.6f,0,1};need(StormMetalBeginShadowFrame(d,1,++frame,center),"frame");storm_render::Matrix view{},lightProjection{};need(StormMetalBeginShadowPass(d,0,0,1,ray,32,&view.m[0][0],&lightProjection.m[0][0]),"sun depth");d->SetTransform(storm_render::TransformView,&view);d->SetTransform(storm_render::TransformProjection,&lightProjection);
  quad(7,8,1);if(gap>=0){d->SetRenderState(storm_render::RenderBlend,blendedCaster||translucentCaster);d->SetRenderState(storm_render::RenderDepthWrite,!translucentCaster);draw(1.5f,8-gap,0,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderDepthWrite,true);}StormMetalEndShadowPass(d);need(StormMetalEndShadowFrame(d),"shadow ready");
  d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&projection);quad(7,8,2);d->EndScene();need(storm_render::succeeded(d->GetRenderTargetData(target,readback)),"readback");readback->LockRect(&locked,nullptr,storm_render::lock::readOnly);std::vector<uint32_t> pixels(4096);for(int y=0;y<64;y++)std::memcpy(pixels.data()+y*64,static_cast<char*>(locked.pBits)+y*locked.Pitch,256);readback->UnlockRect();return pixels;
 };
 auto empty=render(-1),coplanar=render(0),contact=render(.02f),blended=render(.02f,true),translucent=render(.02f,false,true);
 auto brightness=[](uint32_t c){return int(c&255)+int((c>>8)&255)+int((c>>16)&255);};int acne=0,dark=0,blendedDark=0;
 for(int y=18;y<46;y++)for(int x=18;x<46;x++){int i=y*64+x;if(brightness(coplanar[i])+4<brightness(empty[i]))acne++;if(brightness(contact[i])+8<brightness(empty[i]))dark++;if(brightness(blended[i])+8<brightness(empty[i]))blendedDark++;}
 std::printf("character 2cm contact: occluded=%d blended=%d coplanar_acne=%d\n",dark,blendedDark,acne);need(acne==0,"coplanar duplicate does not produce shadow acne");need(dark>8,"2cm character self-occlusion survives shadow bias");need(blendedDark>8,"Z-writing blended character remains a caster");need(translucent==empty,"non-Z-writing translucent model remains excluded from caster pass");
 unsigned location=StormMetalBeginLandModel(d,1);unsigned nested=StormMetalBeginSceneModel(d);need(nested==1,"scene model preserves location scope");StormMetalEndLandModel(d,nested);unsigned after=StormMetalBeginLandModel(d,1);need(after==1,"nested model restores location scope");StormMetalEndLandModel(d,after);StormMetalEndLandModel(d,location);
 d->SetTexture(0,nullptr);texture->Release();target->Release();readback->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS character contact shadows");
}

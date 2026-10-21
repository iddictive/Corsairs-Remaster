#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" bool StormMetalPrepareDynamicSky(void*,float,float,float,float,float,float,float,float,float,float,uint32_t);
extern "C" bool StormMetalDynamicSkyActive(void*);
extern "C" bool StormMetalCloudFieldCurrent(void*);
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
struct Vertex {float x,y,z,nx,ny,nz;};
int main(int argc,char**argv){
 if(argc!=4)return 2;
 setenv("STORM_METAL_DYNAMIC_SKY",argv[1],1);
 setenv("STORM_METAL_MODERN_LIGHTING",argv[2],1);
 setenv("STORM_METAL_MODERN_WATER",argv[2],1);
 if(SDL_Init(SDL_INIT_VIDEO))return 3;
 auto*w=SDL_CreateWindow("sky light readback",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);
 storm_render::Presentation pp{};pp.width=pp.height=32;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
 storm_render::Device*d=nullptr;if(storm_render::failed(storm_render::createMetalDevice(w,&pp,&d)))return 4;
 d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal);
 storm_render::Matrix p{};p._11=p._22=20;p._33=p._44=1;d->SetTransform(storm_render::TransformProjection,&p);
 d->SetRenderState(storm_render::RenderDepthTest,false);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);
 d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,false);
 d->SetRenderState(storm_render::RenderAmbient,0);
 d->SetRenderState(storm_render::RenderDiffuseMaterialSource,storm_render::MaterialSourceMaterial);
 d->SetRenderState(storm_render::RenderAmbientMaterialSource,storm_render::MaterialSourceMaterial);
 d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);
 d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);
 storm_render::Material m{};m.Diffuse=m.Ambient={1,1,1,1};d->SetMaterial(&m);
 storm_render::Surface*t=nullptr,*r=nullptr;d->GetRenderTarget(&t);d->CreateOffscreenPlainSurface(32,32,storm_render::AssetFormat::A8R8G8B8,&r);
 const Vertex quad[]={{-.05f,.05f,.5f,0,1,0},{.05f,.05f,.5f,0,1,0},{-.05f,-.05f,.5f,0,1,0},{.05f,-.05f,.5f,0,1,0}};
 uint32_t pixel=0;bool active=false,cloud=false;
 for(int i=0;i<10;++i){
  const bool indoor=std::atoi(argv[3]);StormMetalLandLocation(d,indoor);StormMetalLandInterior(d,indoor);
  d->Clear(storm_render::clear::color,0xff000000,1);
  StormMetalPrepareDynamicSky(d,12,1.f/60,.38f,.55f,0,9,.16f,0,1,0,0xff7397b6);
  active=StormMetalDynamicSkyActive(d);cloud=StormMetalCloudFieldCurrent(d);
  d->BeginScene();if(storm_render::failed(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip,2,quad,sizeof(Vertex))))return 5;d->EndScene();
  if(storm_render::failed(d->GetRenderTargetData(t,r)))return 6;
  storm_render::LockedRect lock{};r->LockRect(&lock,nullptr,storm_render::lock::readOnly);std::memcpy(&pixel,static_cast<char*>(lock.pBits)+16*lock.Pitch+16*4,4);r->UnlockRect();d->Present();
 }
 std::printf("sky=%s modern=%s indoor=%s pixel=%08x active=%d cloud=%d\n",argv[1],argv[2],argv[3],pixel,active,cloud);
 r->Release();t->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();
}

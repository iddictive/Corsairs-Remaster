#include <SDL.h>
#include "render_device_api.hpp"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void require(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static std::vector<uint32_t> load(const std::string&p){std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f),p.c_str());size_t n=f.tellg();std::vector<uint32_t>v(n/4);f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){
 require(argc==2,"depth-water-probe techniques");setenv("STORM_METAL_MODERN_WATER","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Depth water GPU fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);require(w,"window");storm_render::Presentation p{};p.width=p.height=128;p.colorFormat=storm_render::AssetFormat::A8R8G8B8;storm_render::Device*d=nullptr;require(storm_render::succeeded(storm_render::createMetalDevice(w, &p, &d)),"device");
 storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;storm_render::Matrix projection{};projection._11=projection._22=1;projection._33=100.f/99.f;projection._34=1;projection._43=-100.f/99.f;d->SetTransform(storm_render::TransformWorld,&identity);d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&projection);
 storm_render::Texture*bump=nullptr;d->CreateTexture(4, 4, 1, 0, storm_render::AssetFormat::A8R8G8B8, &bump);auto paintBump=[&](uint32_t value){storm_render::LockedRect l{};bump->LockRect(0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((uint32_t*)((char*)l.pBits+y*l.Pitch))[x]=value;bump->UnlockRect(0);};paintBump(0xff80ff80);
 storm_render::CubeTexture*cube=nullptr;d->CreateCubeTexture(4, 1, 0, storm_render::AssetFormat::A8R8G8B8, &cube);auto paintCube=[&](uint32_t value){for(int f=0;f<6;f++){storm_render::LockedRect l{};cube->LockRect(storm_render::CubeFace(f),0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((uint32_t*)((char*)l.pBits+y*l.Pitch))[x]=value;cube->UnlockRect(storm_render::CubeFace(f),0);}};paintCube(0xff000000);
 auto vc=load(std::string(argv[1])+"/weather/sea_sea2_vertex_shader.vso"),pc=load(std::string(argv[1])+"/weather/sea_sea2_pixel_shader.pso");storm_render::Program*vs=nullptr;storm_render::Program*ps=nullptr;d->ImportVertexProgram(vc.data(),&vs);d->ImportFragmentProgram(pc.data(),&ps);
 float c[256][4]{};auto set=[&](int i,float x,float y,float z,float w){c[i][0]=x;c[i][1]=y;c[i][2]=z;c[i][3]=w;};set(0,0,1,.5,-.04);set(1,2,-1,0,0);set(23,0,0,0,1);set(24,1,0,0,0);set(25,0,1,0,0);set(26,0,0,projection._33,projection._43);set(27,0,0,1,0);set(28,1,1,1,0);set(29,0,0,0,1);set(30,0,0,0,1);set(33,1,0,0,1);set(34,.75,1,.5,1);set(36,1,0,0,0);set(37,0,1,0,0);set(38,0,0,1,0);set(58,0,1,0,1);d->SetVertexShaderConstantF(0,&c[0][0],256);
 storm_render::Surface*target=nullptr,*read=nullptr,*originalDepth=nullptr;d->GetRenderTarget(&target);d->GetDepthStencilSurface(&originalDepth);d->CreateOffscreenPlainSurface(128, 128, storm_render::AssetFormat::A8R8G8B8, &read);
 auto pixel=[&](int x=64,int y=64){require(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect l{};read->LockRect(&l,nullptr,storm_render::lock::readOnly);uint32_t v;memcpy(&v,(char*)l.pBits+y*l.Pitch+x*4,4);read->UnlockRect();return v;};
 struct V{float x,y,z;uint32_t color;};struct S{float x,y,z,nx,ny,nz,u,v;};S sea[]={{-8,-1,2,0,1,0,0,0},{8,-1,2,0,1,0,1,0},{0,-1,20,0,1,0,.5,1}};
 auto geometry=[&](float depth,uint32_t color,float left=-2,float right=2){V v[]={{left*depth,-2*depth,depth,color},{right*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,2*depth,depth,color}};d->BindVertexProgram(nullptr);d->BindFragmentProgram(nullptr);d->SetVertexDeclaration(nullptr);d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse);d->SetTexture(0,nullptr);d->SetRenderState(storm_render::RenderBlend,false);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);require(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,v,sizeof(V))),"background geometry");};
 auto water=[&](){d->BindVertexProgram(vs);d->BindFragmentProgram(ps);d->SetTexture(0,bump);d->SetTexture(3,cube);d->SetRenderState(storm_render::RenderBlend,true);d->SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendOne);d->SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendSourceAlpha);require(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,1,sea,sizeof(S))),"water draw");};
 auto start=[&](){require(storm_render::succeeded(d->Present()),"frame boundary");d->SetRenderState(storm_render::RenderDepthTest,true);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderDepthCompare,storm_render::CompareLessEqual);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderFog,false);require(storm_render::succeeded(d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1)),"fresh color+depth");d->BeginScene();};
 auto finish=[&](){d->EndScene();return pixel();};

 namespace aim=storm::sea_ai::manual_aim;
 const float axis[]={0,0,1},lateral[]={1,0,0},up[]={0,1,0};
 aim::AimVolumeSection sections[]={{{0,-1,2},0,0,4,2,2},{{0,-1,6},1,4,0,2,2}};
 aim::AimVolumePlane planes[]={{1,0,2,2},{0,1,2,2},{-1,0,2,2},{0,-1,2,2}};
 auto render=[&](float centerX){
   start();StormMetalSetAimEnabled(d,true);
   StormMetalSetAimRenderedReceivers(d,nullptr,0);
   geometry(20,0xff203040);
   require(StormMetalBeginAimWater(d),"begin water identity");water();StormMetalEndAimWater(d);
   if(std::getenv("DRAPE_AFTER_HUD"))storm_render::markSceneBoundary(d,storm_render::SceneBoundary::Hud);
   aim::AimContactSection field{{centerX,-1,4},0,{1,0,0,1,0,1},{-2,2}};
   require(StormMetalDrawAimVolume(d,sections,2,planes,4,nullptr,0,nullptr,0,&field,1,nullptr,0,axis,lateral,up,1),"draw aim");
   d->EndScene();require(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");
   storm_render::LockedRect l{};read->LockRect(&l,nullptr,storm_render::lock::readOnly);
   std::vector<uint32_t> out(128*128);
   for(int y=0;y<128;y++)memcpy(out.data()+y*128,(char*)l.pBits+y*l.Pitch,128*4);
   read->UnlockRect();return out;
 };
 const auto absent=render(50),present=render(0);
 int changed=0,ground=0,sky=0,maxDelta=0;
 for(int y=0;y<128;y++)for(int x=0;x<128;x++){
   int i=y*128+x,delta=0;for(int shift:{0,8,16})delta+=std::abs(int((absent[i]>>shift)&255)-int((present[i]>>shift)&255));
   maxDelta=std::max(maxDelta,delta);if(delta>6){++changed;if(y>=64)++ground;else ++sky;}
 }
 printf("mode=%s AA=%s afterHUD=%d contactPixels=%d ground=%d sky=%d maxDelta=%d\n",std::getenv("STORM_METAL_UPSCALING")?std::getenv("STORM_METAL_UPSCALING"):"0",std::getenv("STORM_METAL_ANTIALIASING")?std::getenv("STORM_METAL_ANTIALIASING"):"0",bool(std::getenv("DRAPE_AFTER_HUD")),changed,ground,sky,maxDelta);
 d->BindVertexProgram(nullptr);d->BindFragmentProgram(nullptr);d->SetTexture(0,nullptr);d->SetTexture(3,nullptr);
 vs->Release();ps->Release();bump->Release();cube->Release();target->Release();read->Release();originalDepth->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();
}

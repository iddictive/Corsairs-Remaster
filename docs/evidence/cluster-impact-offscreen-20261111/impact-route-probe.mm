// Isolated fixture: include the current backend, link existing support archives,
// use a hidden non-game device and the installed read-only atlas resources.
#include "backend.mm"
#include <SDL.h>
#include <unistd.h>
#include <cstdio>
#include <vector>
static void need(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
int main(){@autoreleasepool{
 need(chdir("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources")==0,"resource cwd");
 need(SDL_Init(SDL_INIT_VIDEO)==0,"SDL");auto*w=SDL_CreateWindow("impact route fixture",0,0,160,120,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");
 storm_render::Presentation p{};p.width=160;p.height=120;p.colorFormat=storm_render::AssetFormat::A8R8G8B8;
 storm_render::Device*api=nullptr;need(storm_render::succeeded(storm_render::createMetalDevice(w,&p,&api)),"device");auto*d=static_cast<Device*>(api);
 storm_render::Matrix id{};id._11=id._22=id._33=id._44=1;storm_render::Matrix projection{};
 projection._11=1;projection._22=1.333333f;projection._33=1000.f/999.f;projection._34=1;projection._43=-1000.f/999.f;
 api->SetTransform(storm_render::TransformWorld,&id);api->SetTransform(storm_render::TransformView,&id);api->SetTransform(storm_render::TransformProjection,&projection);
 storm_render::Surface*target=nullptr,*read=nullptr;api->GetRenderTarget(&target);
 need(storm_render::succeeded(api->CreateOffscreenPlainSurface(160,120,storm_render::AssetFormat::A8R8G8B8,&read)),"readback alloc");
 auto pixels=[&](){need(storm_render::succeeded(api->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect r{};
  need(storm_render::succeeded(read->LockRect(&r,nullptr,storm_render::lock::readOnly)),"lock");std::vector<unsigned>a(160*120);
  for(int y=0;y<120;y++)memcpy(a.data()+160*y,(char*)r.pBits+y*r.Pitch,640);read->UnlockRect();return a;};
 for(unsigned surface=0;surface<3;surface++) for(unsigned warhead:{1u,2u,3u,4u}){
  api->Clear(storm_render::clear::color|storm_render::clear::depth,0xff18232a,1);api->BeginScene();auto before=pixels();
  StormMetalMissileVfxTick(api,1,.016f);float pos[]={0,0,40},dir[]={.4f,-1,0};
  need(StormMetalClusterImpact(api,pos,dir,surface,warhead==2?8.f:18.f,warhead),"producer admitted");d->clusterImpacts.back().age=.55f;
  StormMetalClusterImpactCapture(api);need(d->missileVfxCameraValid,"camera");storm_render::markSceneBoundary(api,storm_render::SceneBoundary::Hud);
  auto after=pixels();size_t changed=0;for(size_t i=0;i<before.size();i++)if(before[i]!=after[i])changed++;
  printf("surface=%u warhead=%u changed=%zu fallback=%zu\n",surface,warhead,changed,d->clusterImpactFallbacks.size());
  need(changed>20,"deferred frame visible");need(d->clusterImpactFallbacks.empty(),"no duplicate fallback");
  d->clusterImpacts.clear();d->clusterImpactFallbacks.clear();d->clusterImpactDrawn=false;d->missileVfxCameraValid=false;d->hudSceneSnapshotReady=false;
 }
 api->SetTransform(storm_render::TransformProjection,&id);float pos[]={0,0,40},dir[]={0,-1,0};
 need(StormMetalClusterImpact(api,pos,dir,2,8,4),"admit fallback case");d->missileVfxCameraValid=false;d->flushClusterImpacts();
 float result[3]{};unsigned surface=0,warhead=0;
 need(StormMetalClusterImpactTakeFallback(api,result,&surface,&warhead)&&surface==2&&warhead==4&&result[2]==40,"fallback identity");
 need(!StormMetalClusterImpactTakeFallback(api,result,&surface,&warhead),"fallback once");
 puts("PASS bridge -> deferred GPU: all surfaces and conventional warheads; camera failure -> single fallback");
 read->Release();target->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();return 0;
}}

#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char*text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main(){
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Stars multistream fixture",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);check(w,SDL_GetError());
 storm_render::Presentation pp{};pp.width=pp.height=64;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;storm_render::Device*d=nullptr;check(storm_render::succeeded(storm_render::createMetalDevice(w, &pp, &d)),"device");
 const float positions[]={-.5f,0,.5f,.5f,0,.5f};const uint32_t colors[]={0xffff0000,0xff00ff00};storm_render::VertexBuffer *position=nullptr,*color=nullptr;void*bytes=nullptr;
 check(storm_render::succeeded(d->CreateVertexBuffer(sizeof(positions), 0, 0, &position)),"position buffer");position->Lock(0,0,&bytes,0);memcpy(bytes,positions,sizeof(positions));position->Unlock();check(storm_render::succeeded(d->CreateVertexBuffer(sizeof(colors), 0, 0, &color)),"color buffer");color->Lock(0,0,&bytes,0);memcpy(bytes,colors,sizeof(colors));color->Unlock();
 const storm_render::VertexElement elements[]={{0,0,storm_render::VertexComponent::Float3,storm_render::VertexMethod::Default,storm_render::VertexSemantic::Position,0},{1,0,storm_render::VertexComponent::Color,storm_render::VertexMethod::Default,storm_render::VertexSemantic::Color,0},storm_render::vertexLayoutTerminator()};storm_render::VertexLayout*decl=nullptr;d->CreateVertexDeclaration(elements,&decl);d->SetVertexDeclaration(decl);d->SetStreamSource(0,position,0,12);d->SetStreamSource(1,color,0,4);
 storm_render::VertexBuffer*bound=nullptr;uint32_t offset=0,stride=0;check(storm_render::succeeded(d->GetStreamSource(0,&bound,&offset,&stride))&&bound==position&&stride==12,"stream1 preserves stream0");bound->Release();
 storm_render::Texture*texture=nullptr;d->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &texture);storm_render::LockedRect lock{};texture->LockRect(0,&lock,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((uint32_t*)((char*)lock.pBits+y*lock.Pitch))[x]=0xffffffff;texture->UnlockRect(0);d->SetTexture(0,texture);
 d->SetRenderState(storm_render::RenderDepthTest,false);d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderPointSprite,true);float pointSize=16;uint32_t bits;memcpy(&bits,&pointSize,4);d->SetRenderState(storm_render::RenderPointSize,bits);
 storm_render::Surface *target=nullptr,*readback=nullptr;d->GetRenderTarget(&target);d->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback);
 auto draw=[&](uint32_t start,uint32_t count){d->Clear(storm_render::clear::color, 0xff000000, 1);d->BeginScene();auto result=d->DrawPrimitive(storm_render::PrimitivePointList,start,count);d->EndScene();return result;};
 auto pixel=[&](int x,int y){check(storm_render::succeeded(d->GetRenderTargetData(target,readback)),"readback");storm_render::LockedRect r{};readback->LockRect(&r,nullptr,storm_render::lock::readOnly);uint32_t value;memcpy(&value,(char*)r.pBits+y*r.Pitch+x*4,4);readback->UnlockRect();return value;};
 check(storm_render::succeeded(draw(0,2)),"original stars declaration draw");check(pixel(16,32)==0xffff0000&&pixel(48,32)==0xff00ff00,"independent position and color streams");check(pixel(22,32)==0xffff0000&&pixel(26,32)==0xff000000,"point screen-space size");
 check(storm_render::succeeded(draw(1,1)),"nonzero start vertex");check(pixel(16,32)==0xff000000&&pixel(48,32)==0xff00ff00,"start applies to both streams");check(storm_render::failed(draw(1,2)),"out-of-range stream read rejected");
 d->SetStreamSource(1,nullptr,0,0);check(storm_render::succeeded(d->GetStreamSource(0,&bound,&offset,&stride))&&bound==position,"unbind stream1 preserves positions");bound->Release();check(storm_render::failed(draw(0,1)),"missing color stream rejected");
 target->Release();readback->Release();texture->Release();decl->Release();position->Release();color->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: stars FFP declaration, multistream, point sprites, bounds");
}

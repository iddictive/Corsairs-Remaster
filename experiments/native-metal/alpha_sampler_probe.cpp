#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void storm_metal_label_texture(storm_render::BaseTexture*,const char*);
static void need(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
 need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Alpha sampler fixture",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);need(w,SDL_GetError());storm_render::Presentation pp{};pp.width=pp.height=32;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;storm_render::Device*d=nullptr;need(storm_render::succeeded(storm_render::createMetalDevice(w, &pp, &d)),"device");
 storm_render::Texture*t=nullptr;need(storm_render::succeeded(d->CreateTexture(1, 1, 1, 0, storm_render::AssetFormat::A8R8G8B8, &t)),"texture");storm_render::LockedRect lock{};t->LockRect(0,&lock,nullptr,0);*(uint32_t*)lock.pBits=0xff00ff00;t->UnlockRect(0);d->SetTexture(0,t);
 struct V{float x,y,z,rhw,u,v;};V quad[]={{0,0,.5,1,2,2},{32,0,.5,1,2,2},{0,32,.5,1,2,2},{32,32,.5,1,2,2}};d->SetFVF(storm_render::vertexFormat::xyzwTransformed|storm_render::vertexFormat::tex1);d->SetRenderState(storm_render::RenderDepthTest,false);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderBlend,true);d->SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendSourceAlpha);d->SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendInverseSourceAlpha);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetSamplerState(0,storm_render::SamplerMinFilter,storm_render::TextureFilterPoint);d->SetSamplerState(0,storm_render::SamplerMagFilter,storm_render::TextureFilterPoint);
 storm_render::Surface *target=nullptr,*read=nullptr;d->GetRenderTarget(&target);d->CreateOffscreenPlainSurface(32, 32, storm_render::AssetFormat::A8R8G8B8, &read);auto draw=[&](uint32_t address){d->SetSamplerState(0,storm_render::SamplerAddressU,address);d->SetSamplerState(0,storm_render::SamplerAddressV,address);d->SetSamplerState(0,storm_render::SamplerBorderColor,0);d->Clear(storm_render::clear::color, 0xffff0000, 1);d->BeginScene();need(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip,2,quad,sizeof(V))),"draw");d->EndScene();need(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect r{};read->LockRect(&r,nullptr,storm_render::lock::readOnly);uint32_t value;std::memcpy(&value,(char*)r.pBits+16*r.Pitch+16*4,4);read->UnlockRect();return value;};
 need(draw(storm_render::TextureAddressWrap)==0xff00ff00,"wrap repeats texture");need(draw(storm_render::TextureAddressClamp)==0xff00ff00,"clamp keeps edge texel");need(draw(storm_render::TextureAddressBorder)==0xffff0000,"transparent border preserves destination");// Town backdrops repeat horizontally, but their opaque bottom must never
 // wrap into the authored transparent upper edge under bilinear filtering.
 storm_render::Texture* backdrop=nullptr;need(storm_render::succeeded(d->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &backdrop)),"backdrop texture");
 backdrop->LockRect(0,&lock,nullptr,0);uint32_t rows[4]={0x00000000,0x00000000,0xff00ff00,0xff0000ff};for(unsigned y=0;y<2;y++)std::memcpy((char*)lock.pBits+y*lock.Pitch,rows+y*2,8);backdrop->UnlockRect(0);d->SetTexture(0,backdrop);
 d->SetSamplerState(0,storm_render::SamplerMinFilter,storm_render::TextureFilterLinear);d->SetSamplerState(0,storm_render::SamplerMagFilter,storm_render::TextureFilterLinear);
 for(auto&v:quad){v.u=.25f;v.v=0;}
 need(draw(storm_render::TextureAddressWrap)!=0xffff0000,"ordinary wrapped texture reproduces edge bleed");
 storm_metal_label_texture(backdrop,"RESOURCE\\Textures\\backJungleU1.tga.tx");
 need(draw(storm_render::TextureAddressWrap)==0xffff0000,"backdrop top edge stays transparent");
 for(auto&v:quad){v.u=1.25f;v.v=.75f;}
 need(draw(storm_render::TextureAddressWrap)==0xff00ff00,"backdrop horizontal repetition remains");
 storm_metal_label_texture(backdrop,"trees.tga");for(auto&v:quad){v.u=.25f;v.v=0;}
 need(draw(storm_render::TextureAddressWrap)!=0xffff0000,"unrelated alpha atlas retains sampler contract");
 backdrop->Release();
 target->Release();read->Release();t->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: wrap, clamp and transparent border sampler semantics");
}

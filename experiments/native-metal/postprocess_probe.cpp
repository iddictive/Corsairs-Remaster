#include <SDL.h>
#include "render_device_api.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(int argc,char**argv){
 check(argc==2,"usage: postprocess-probe RESOURCE/techniques");
 std::ifstream f(std::string(argv[1])+"/postprocess/postprocess_shader.pso",std::ios::binary|std::ios::ate);check(bool(f),"original shader");auto size=f.tellg();check(size>0&&size%4==0,"shader size");std::vector<uint32_t> code(size_t(size)/4);f.seekg(0);f.read((char*)code.data(),size);
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Postprocess and FFP fixture",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,SDL_GetError());
 storm_render::Presentation pp{};pp.width=pp.height=64;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
 storm_render::Device*d=nullptr;check(storm_render::succeeded(storm_render::createMetalDevice(w, &pp, &d)),"device");
 storm_render::Program*ps=nullptr;check(storm_render::succeeded(d->ImportFragmentProgram(code.data(),&ps))&&ps,"original postprocess accepted");
 storm_render::Texture*textures[4]{};uint32_t colors[]={0xff204060,0xff406080,0xff6080a0,0xff80a0c0};
 for(int i=0;i<4;i++){check(storm_render::succeeded(d->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &textures[i])),"texture");storm_render::LockedRect r{};check(storm_render::succeeded(textures[i]->LockRect(0,&r,nullptr,0)),"texture lock");for(int y=0;y<2;y++)for(int x=0;x<2;x++)((uint32_t*)((char*)r.pBits+y*r.Pitch))[x]=colors[i];textures[i]->UnlockRect(0);d->SetTexture(i,textures[i]);}
 struct V{float x,y,z,rhw;uint32_t diffuse,specular;float uv[8];};
 V vertices[]={{0,0,.5f,1,0xff102030,0xff305070,{0,0,0,0,0,0,0,0}},{64,0,.5f,1,0xff102030,0xff305070,{1,0,1,0,1,0,1,0}},{0,64,.5f,1,0xff102030,0xff305070,{0,1,0,1,0,1,0,1}},{64,64,.5f,1,0xff102030,0xff305070,{1,1,1,1,1,1,1,1}}};
 d->SetFVF(storm_render::vertexFormat::xyzwTransformed|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::specular|storm_render::vertexFormat::tex4);d->SetRenderState(storm_render::RenderDepthTest,false);d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderBlend,false);
 storm_render::Surface *target=nullptr,*readback=nullptr;check(storm_render::succeeded(d->GetRenderTarget(&target)),"target");check(storm_render::succeeded(d->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback)),"readback");
 auto pixel=[&](){check(storm_render::succeeded(d->Clear(storm_render::clear::color, 0xff000000, 1)),"clear");check(storm_render::succeeded(d->BeginScene()),"begin");check(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip,2,vertices,sizeof(V))),"draw");check(storm_render::succeeded(d->EndScene()),"end");check(storm_render::succeeded(d->GetRenderTargetData(target,readback)),"readback copy");storm_render::LockedRect r{};check(storm_render::succeeded(readback->LockRect(&r,nullptr,storm_render::lock::readOnly)),"readback lock");uint32_t value;memcpy(&value,(char*)r.pBits+32*r.Pitch+32*4,4);readback->UnlockRect();return value;};
 auto near=[&](uint32_t value,uint32_t expected,const char*what){std::printf("%s pixel=%08x expected=%08x\n",what,value,expected);for(int shift:{0,8,16,24})check(std::abs(int((value>>shift)&255)-int((expected>>shift)&255))<=2,what);};
 d->BindFragmentProgram(ps);float ignored[]={1,1,1,1};d->SetPixelShaderConstantF(1,ignored,1);near(pixel(),0xff507090,"original four sample mean with shader-local c1");
 d->BindFragmentProgram(nullptr);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);near(pixel(),colors[0],"unaffected fixed function sample");
 d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::specular);near(pixel(),0xff305070,"worldmap specular argument");
 d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageResultArg,storm_render::textureArg::temporary);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpAdd);d->SetTextureStageState(1,storm_render::StageColorArg1,storm_render::textureArg::temporary);d->SetTextureStageState(1,storm_render::StageColorArg2,storm_render::textureArg::specular);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(1,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);near(pixel(),0xff4070a0,"temporary combiner register");
 check(storm_render::failed(d->Reset(&pp)),"Reset rejects retained backbuffer reference");target->Release();target=nullptr;readback->Release();readback=nullptr;pp.width=48;pp.height=32;check(storm_render::succeeded(d->Reset(&pp)),"Reset resizes target");check(storm_render::succeeded(d->GetRenderTarget(&target)),"new target");storm_render::SurfaceDesc desc{};target->GetDesc(&desc);check(desc.Width==48&&desc.Height==32,"actual reset target dimensions");storm_render::Viewport vp{};d->GetViewport(&vp);check(vp.Width==48&&vp.Height==32,"reset viewport");storm_render::Program*bound=nullptr;d->GetFragmentProgram(&bound);check(!bound,"reset shader binding cleared");
 target->Release();ps->Release();for(auto*t:textures)t->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: postprocess GPU, FFP specular/TEMP, Reset");
}

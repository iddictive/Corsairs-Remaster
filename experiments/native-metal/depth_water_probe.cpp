#include <SDL.h>
#include "render_device_api.hpp"
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
 struct V{float x,y,z;uint32_t color;};struct S{float x,y,z,nx,ny,nz,u,v;};S sea[]={{-8,-8,4,0,0,-1,0,0},{8,-8,4,0,0,-1,1,0},{0,8,4,0,0,-1,.5,1}};
 auto geometry=[&](float depth,uint32_t color,float left=-2,float right=2){V v[]={{left*depth,-2*depth,depth,color},{right*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,2*depth,depth,color}};d->BindVertexProgram(nullptr);d->BindFragmentProgram(nullptr);d->SetVertexDeclaration(nullptr);d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse);d->SetTexture(0,nullptr);d->SetRenderState(storm_render::RenderBlend,false);d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);require(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,v,sizeof(V))),"background geometry");};
 auto water=[&](){d->BindVertexProgram(vs);d->BindFragmentProgram(ps);d->SetTexture(0,bump);d->SetTexture(3,cube);d->SetRenderState(storm_render::RenderBlend,true);d->SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendOne);d->SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendSourceAlpha);require(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,1,sea,sizeof(S))),"water draw");};
 auto start=[&](){require(storm_render::succeeded(d->Present()),"frame boundary");d->SetRenderState(storm_render::RenderDepthTest,true);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderDepthCompare,storm_render::CompareLessEqual);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderLighting,false);d->SetRenderState(storm_render::RenderFog,false);require(storm_render::succeeded(d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1)),"fresh color+depth");d->BeginScene();};
 auto finish=[&](){d->EndScene();return pixel();};
 start();geometry(5,0xffffffff);water();uint32_t shallow=finish();start();geometry(20,0xffffffff);water();uint32_t deep=finish();printf("depth shallow=%08x deep=%08x\n",shallow,deep);int sr=shallow>>16&255,sg=shallow>>8&255,sb=shallow&255,dr=deep>>16&255,dg=deep>>8&255,db=deep&255;require(sr>180&&sg>180&&sb>180,"shallow background remains visible");require(dr<sr-50&&dg<sg-25&&db<sb-10,"actual increased depth attenuates all channels");require(dr<dg&&dg<db,"Beer Lambert red attenuates fastest");
 // A new frame must capture its own geometry rather than reuse the previous white scene.
 start();geometry(5,0xff000000);water();uint32_t black=finish();require((black&255)<50&&((black>>16)&255)<50,"snapshot invalidated at frame boundary");
 // Depth testing preserves an above-water object exactly, including beside refracted water.
 start();geometry(8,0xffffffff);geometry(2,0xffff0000,-.2,.2);water();uint32_t foreground=finish();require((foreground&0xffffff)==0xff0000,"foreground object is not painted over by water");require(((pixel(82,64)>>16)&255)<250,"foreground red not spread into neighbouring water");
 // Normal changes must move actual screen-space texture detail, not merely tint water.
 // A foreground alpha atlas writes depth only for actual leaves. Sea is drawn
 // with ALPHATEST still enabled to expose cross-pipeline state leakage.
 start();geometry(8,0xffffffff);water();finish();uint32_t cleanHole=pixel(48,64);
 storm_render::Texture*leaves=nullptr;require(storm_render::succeeded(d->CreateTexture(2, 1, 1, 0, storm_render::AssetFormat::A8R8G8B8, &leaves)),"leaf atlas");
 storm_render::LockedRect leafLock{};leaves->LockRect(0,&leafLock,nullptr,0);((uint32_t*)leafLock.pBits)[0]=0x0000ff00;((uint32_t*)leafLock.pBits)[1]=0xff00ff00;leaves->UnlockRect(0);
 start();geometry(8,0xffffffff);
 struct Leaf{float x,y,z;uint32_t color;float u,v;};Leaf leaf[]={{-2,-2,2,0xffffffff,0,0},{2,-2,2,0xffffffff,1,0},{2,2,2,0xffffffff,1,1},{-2,-2,2,0xffffffff,0,0},{2,2,2,0xffffffff,1,1},{-2,2,2,0xffffffff,0,1}};
 d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetTexture(0,leaves);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
 d->SetRenderState(storm_render::RenderAlphaTest,true);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,128);
 require(storm_render::succeeded(d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,leaf,sizeof(Leaf))),"alpha foreground draw");water();finish();
 require(pixel(48,64)==cleanHole,"transparent foreground atlas holes preserve sea exactly");
 require((pixel(80,64)&0xffffff)==0x00ff00,"opaque foreground leaf survives sea draw");
 d->SetRenderState(storm_render::RenderAlphaTest,false);leaves->Release();
 // Normal changes may alter reflection, but transmission stays on the current
 // pixel so camera motion cannot drag background detail across depth edges.
 auto checker=[&](){for(int i=0;i<16;i++)geometry(8,i%2?0xff202020:0xffffffff,-1+i*.125f,-1+(i+1)*.125f);};
 start();checker();water();finish();std::vector<uint32_t> first;for(int x=36;x<92;x++)first.push_back(pixel(x,72));paintBump(0xff80ffff);start();checker();water();finish();int shifted=0;for(int x=36;x<92;x++)if(((pixel(x,72)&255)>100)!=((first[x-36]&255)>100))shifted++;require(shifted==0,"normal changes do not shift transmitted background boundaries");
 // Fractional foreground edges sweep through the bilinear footprint. Water
 // samples on the right must never import the bright red above-water strip.
 paintBump(0xff80ffff);
 for(int phase=0;phase<4;phase++){
  float edgePixel=76.125f+phase*.25f,edgeNdc=(edgePixel/128.f)*2.f-1.f;
  start();geometry(8,0xff0000ff);geometry(2,0xffff0000,0,edgeNdc);water();finish();
  for(int x=78;x<89;x++){uint32_t v=pixel(x,64);require(int(v>>16&255)<=int(v&255)+3,"bilinear refraction does not bleed foreground red across fractional edge");}
 }
 // Fixed authored pigment/sky, varying only real weather lighting.
 paintBump(0xff80ff80);paintCube(0xff000000);set(29,.04f,.35f,.6f,1);set(30,1,1,1,1);d->SetVertexShaderConstantF(29,c[29],2);
 storm_render::Light sun{};sun.Type=storm_render::LightKind::Directional;sun.Direction={0,-1,0};
 auto weather=[&](int ambient,int direct){uint32_t a=uint32_t(ambient);d->SetRenderState(storm_render::RenderAmbient,(a<<16)|(a<<8)|a);float v=direct/255.f;sun.Diffuse={v,v,v,1};d->SetLight(0,&sun);d->LightEnable(0,true);};
 weather(95,220);start();geometry(90,0xff000000);water();uint32_t day=finish();
 weather(28,30);start();geometry(90,0xff000000);water();uint32_t night=finish();
 auto energy=[](uint32_t v){return int(v&255)+int(v>>8&255)+int(v>>16&255);};
 printf("depth lighting day=%08x night=%08x\n",day,night);require(energy(day)>energy(night)*2&&energy(day)>45,"actual night weather darkens deep water with unchanged artistic colors");
 // Reflection and existing emissive geometry are not multiplied by weather light.
 paintCube(0xffffffff);weather(95,220);start();geometry(4.05f,0xffffffff);water();uint32_t dayEmission=finish();weather(28,30);start();geometry(4.05f,0xffffffff);water();uint32_t nightEmission=finish();require(std::abs(energy(dayEmission)-energy(nightEmission))<12,"weather does not dim transmitted emissive scene or reflection");
 paintCube(0xff000000);
 // Actual runtime near=.115/far=32000: a constant NDC epsilon of 1e-6 spans
 // ~12.5 world units at z1200, and must not erase ten real submerged units.
 storm_render::Matrix farProjection=projection;farProjection._33=32000.f/(32000.f-.115f);farProjection._43=-.115f*32000.f/(32000.f-.115f);
 d->SetTransform(storm_render::TransformProjection,&farProjection);set(26,0,0,farProjection._33,farProjection._43);d->SetVertexShaderConstantF(26,c[26],1);
 for(auto&v:sea){v.x*=300;v.y*=300;v.z*=300;}
 start();geometry(1210,0xffffffff);water();uint32_t distantSubmerged=finish();
 printf("distant 10-unit submerged depth=%08x\n",distantSubmerged);
 require(int(distantSubmerged>>16&255)<200,"distant real submerged depth is not erased by fixed NDC foreground epsilon");
 for(auto&v:sea){v.x/=300;v.y/=300;v.z/=300;}
 d->SetTransform(storm_render::TransformProjection,&projection);set(26,0,0,projection._33,projection._43);d->SetVertexShaderConstantF(26,c[26],1);
 // Without a depth attachment the bounded angular fallback still renders.
 d->SetDepthStencilSurface(nullptr);start();geometry(5,0xff808080);water();uint32_t fallback=finish();require((fallback&0xffffff)!=0,"missing-depth fallback remains visible");d->SetDepthStencilSurface(originalDepth);
 d->BindVertexProgram(nullptr);d->BindFragmentProgram(nullptr);d->SetTexture(0,nullptr);d->SetTexture(3,nullptr);vs->Release();ps->Release();bump->Release();cube->Release();target->Release();read->Release();originalDepth->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS depth water GPU fixture");
}

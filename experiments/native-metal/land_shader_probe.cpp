#import <Metal/Metal.h>
#include <SDL.h>
#include "render_device_api.hpp"
#include "land_shaders.hpp"
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
extern "C" bool StormMetalDrawGrass(void*,const void*,size_t,size_t,const float*,size_t,bool,float,float,float,bool,unsigned,bool,unsigned,unsigned);
extern "C" void StormMetalDrawBillboards(void*,const void*,uint32_t,const char*,uint32_t,uint32_t,float,float);
static void check(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static std::vector<uint32_t> load(std::string p){std::ifstream f(p,std::ios::binary|std::ios::ate);check(bool(f),p.c_str());size_t n=f.tellg();std::vector<uint32_t> v(n/4);f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){
 check(argc==2,"usage: land-shader-probe techniques");SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Land shader fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,"window");
 storm_render::Presentation p{};p.width=p.height=128;p.colorFormat=storm_render::AssetFormat::A8R8G8B8;storm_render::Device*d=nullptr;check(storm_render::succeeded(storm_render::createMetalDevice(w, &p, &d)),"device");
	storm_render::Texture*t=nullptr;d->CreateTexture(4, 4, 1, 0, storm_render::AssetFormat::A8R8G8B8, &t);storm_render::LockedRect lr{};t->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((uint32_t*)((char*)lr.pBits+y*lr.Pitch))[x]=0xff808080;t->UnlockRect(0);
	storm_render::Texture*t1=nullptr;d->CreateTexture(4, 4, 1, 0, storm_render::AssetFormat::A8R8G8B8, &t1);t1->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((uint32_t*)((char*)lr.pBits+y*lr.Pitch))[x]=0x00808080;t1->UnlockRect(0);
 for(int i=0;i<2;i++){d->SetTexture(i,t);d->SetSamplerState(i,storm_render::SamplerMinFilter,storm_render::TextureFilterPoint);d->SetSamplerState(i,storm_render::SamplerMagFilter,storm_render::TextureFilterPoint);d->SetTextureStageState(i,storm_render::StageTexCoordIndex,i);}
 d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderDepthTest,false);d->SetRenderState(storm_render::RenderBlend,false);d->SetRenderState(storm_render::RenderFog,false);
 storm_render::Surface*target=nullptr,*read=nullptr;d->GetRenderTarget(&target);d->CreateOffscreenPlainSurface(128, 128, storm_render::AssetFormat::A8R8G8B8, &read);
 auto pixel=[&](int x,int y){check(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect r{};read->LockRect(&r,nullptr,storm_render::lock::readOnly);uint32_t v;memcpy(&v,(char*)r.pBits+y*r.Pitch+x*4,4);read->UnlockRect();return v;};
 auto near=[&](uint32_t v,int r,int g,int b,const char*s){printf("%s %08x\n",s,v);check(abs(int(v>>16&255)-r)<4&&abs(int(v>>8&255)-g)<4&&abs(int(v&255)-b)<4,s);};
 auto draw=[&](void*v,int stride){d->Clear(storm_render::clear::color, 0xff070b13, 1);d->BeginScene();auto hr=d->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,1,v,stride);d->EndScene();check(storm_render::succeeded(hr),"actual shader draw");};
 float c[256][4]{};auto set=[&](int n,float x,float y,float z,float w){c[n][0]=x;c[n][1]=y;c[n][2]=z;c[n][3]=w;};
 auto tokens=load(std::string(argv[1])+"/worldmap/worldmap_shader.vso");check(identifyLandVertexShader(tokens.data())==LandShaderKind::Worldmap,"strict worldmap fingerprint");auto changed=tokens;changed[4]^=1;check(identifyLandVertexShader(changed.data())==LandShaderKind::None,"changed shader rejected by fingerprint");storm_render::Program*vs=nullptr;d->ImportVertexProgram(tokens.data(),&vs);d->BindVertexProgram(vs);d->BindFragmentProgram(nullptr);
 struct W{float x,y,z;uint32_t color;float u,v;};W cloud[]={{-.8f,-.8f,.5f,0xff804000,0,0},{.8f,-.8f,.5f,0xff804000,1,0},{0,.8f,.5f,0xff804000,.5f,1}};
 set(0,1,0,0,0);set(1,0,1,0,0);set(2,0,0,1,0);set(3,0,0,0,1);set(5,1,0,0,.5);set(6,.4,.6,.8,1);d->SetVertexShaderConstantF(0,&c[0][0],256);
 d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpModulate);d->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(0,storm_render::StageColorArg2,storm_render::textureArg::diffuse);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);
 d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpModulate);d->SetTextureStageState(1,storm_render::StageColorArg1,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageColorArg2,storm_render::textureArg::current);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpDisable);
 d->SetTextureStageState(2,storm_render::StageColorOp,storm_render::TextureOpAdd);d->SetTextureStageState(2,storm_render::StageColorArg1,storm_render::textureArg::specular);d->SetTextureStageState(2,storm_render::StageColorArg2,storm_render::textureArg::current);d->SetTextureStageState(2,storm_render::StageAlphaOp,storm_render::TextureOpDisable);d->SetTextureStageState(3,storm_render::StageColorOp,storm_render::TextureOpDisable);
 draw(cloud,sizeof(W));near(pixel(64,64),58,71,84,"cloud original three-stage diffuse/specular");near(pixel(2,2),7,11,19,"outside triangle");d->BindVertexProgram(nullptr);vs->Release();
 tokens=load(std::string(argv[1])+"/effects/grass_main.vso");check(identifyLandVertexShader(tokens.data())==LandShaderKind::Grass,"strict grass fingerprint");d->ImportVertexProgram(tokens.data(),&vs);d->BindVertexProgram(vs);
	struct G{float x,y,z;uint32_t params,offset;float wx,wy,alpha;};G grass[]={{0,-.8f,.5f,0,0,0,0,1},{0,-.8f,.5f,0,0x00ff0000,0,0,1},{0,-.8f,.5f,0,0x0080ff00,0,0,1}};struct BG{float x,y,z;uint32_t data;float wx,wz,alpha;};BG bridgeGrass[]={{0,-.8f,.5f,0,0,0,1}};
	memset(c,0,sizeof(c));set(0,1,0,0,1);set(32,1,0,0,0);set(33,0,1,0,0);set(34,0,0,1,0);set(35,0,0,0,1);set(37,.25,.5,.75,1);set(38,0,0,0,1);set(39,.9,1,.245,-.245);set(40,15,-.5,1,.8);set(41,0,0,1.6,2.56);d->SetVertexShaderConstantF(0,&c[0][0],256);
	d->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpModulate2X);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);draw(grass,sizeof(G));near(pixel(64,64),64,128,192,"grass original transform/color and modulate2x");
	near(pixel(88,40),7,11,19,"grass pre-wind outside");for(auto&v:grass)v.wx=.4f;draw(grass,sizeof(G));near(pixel(88,40),64,128,192,"grass wind moves original blade geometry");
	for(auto&v:grass){v.wx=0;v.alpha=1;}d->SetVertexShaderConstantF(0,&c[0][0],256);d->SetTexture(1,t1);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);
	auto bridge=[&](){d->Clear(storm_render::clear::color, 0xff070b13, 1);d->BeginScene();bool ok=StormMetalDrawGrass(d,bridgeGrass,1,sizeof(BG),&c[0][0],42*4,true,1,1,.2,true,80,true,storm_render::BlendSourceAlpha,storm_render::BlendInverseSourceAlpha);d->EndScene();check(ok,"grass bridge accepted compact fixture");return pixel(64,64);};
	auto restored=[&](uint32_t cull,uint32_t colorOp,uint32_t alphaOp,const char*label){uint32_t value=0;d->GetRenderState(storm_render::RenderCullMode,&value);check(value==cull,label);d->GetTextureStageState(1,storm_render::StageColorOp,&value);check(value==colorOp,label);d->GetTextureStageState(1,storm_render::StageAlphaOp,&value);check(value==alphaOp,label);};
	d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpDisable);uint32_t clean=bridge();restored(storm_render::CullNone,storm_render::TextureOpDisable,storm_render::TextureOpDisable,"grass bridge restores clean caller state");
	d->SetRenderState(storm_render::RenderCullMode,storm_render::CullClockwise);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpDisable);uint32_t cw=bridge();restored(storm_render::CullClockwise,storm_render::TextureOpDisable,storm_render::TextureOpDisable,"grass bridge restores CW caller state");
	d->SetRenderState(storm_render::RenderCullMode,storm_render::CullCounterClockwise);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpDisable);uint32_t ccw=bridge();restored(storm_render::CullCounterClockwise,storm_render::TextureOpDisable,storm_render::TextureOpDisable,"grass bridge restores CCW caller state");
	d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpModulate);d->SetTextureStageState(1,storm_render::StageColorArg1,storm_render::textureArg::current);d->SetTextureStageState(1,storm_render::StageColorArg2,storm_render::textureArg::texture);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(1,storm_render::StageAlphaArg1,storm_render::textureArg::texture);uint32_t stage1=bridge();restored(storm_render::CullNone,storm_render::TextureOpModulate,storm_render::TextureOpSelectArg1,"grass bridge restores stage-1 caller state");
	printf("grass bridge inherited clean=%08x cw=%08x ccw=%08x stage1=%08x\n",clean,cw,ccw,stage1);near(clean,64,128,192,"grass bridge clean technique state");near(cw,64,128,192,"grass bridge ignores inherited CW cull");near(ccw,64,128,192,"grass bridge ignores inherited CCW cull");near(stage1,64,128,192,"grass bridge disables inherited stage-1 color/alpha");
	bridgeGrass[0].alpha=.1f;d->SetRenderState(storm_render::RenderCullMode,storm_render::CullClockwise);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpModulate);d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);uint32_t cutout=bridge();restored(storm_render::CullClockwise,storm_render::TextureOpModulate,storm_render::TextureOpSelectArg1,"grass bridge restores cutout caller state");near(cutout,7,11,19,"grass bridge retains authored alpha cutout");
	for(auto&v:grass){v.wx=0;v.alpha=.1;}d->SetRenderState(storm_render::RenderAlphaTest,true);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,80);draw(grass,sizeof(G));near(pixel(64,64),7,11,19,"grass fade respects original alpha test");
	// Every world-space consumer must use the same fog colour and camera depth.
	// The existing grass fixtures previously exercised only fog-disabled draws.
	struct Flat {float x,y,z;uint32_t color;float u,v;};
	Flat flat[]={{-.8f,-.8f,.5f,0xff4080bf,0,0},{.8f,-.8f,.5f,0xff4080bf,1,0},{0,.8f,.5f,0xff4080bf,.5f,1}};
	struct Billboard {float x,y,z,size,angle;uint32_t color,subtexture;};
	Billboard billboard{0,0,.5f,.8f,0,0xff4080bf,0};
	bridgeGrass[0].alpha=1;for(auto&v:grass)v.alpha=1;
	d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetRenderState(storm_render::RenderLighting,false);
	d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetRenderState(storm_render::RenderBlend,false);
	d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);
	d->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpDisable);d->SetRenderState(storm_render::RenderFogColor,0xff60a0e0);
	auto stateFloat=[&](storm_render::RenderStateField state,float value){uint32_t bits;memcpy(&bits,&value,4);d->SetRenderState(state,bits);};
	stateFloat(storm_render::RenderFogDensity,.35f);stateFloat(storm_render::RenderFogStart,0);stateFloat(storm_render::RenderFogEnd,4);
	const char*paths[]={"expanded grass","compact grass","fixed geometry","billboard"};
	for(float depth:{1.f,3.f}) {
		storm_render::Matrix projection{};for(int axis=0;axis<4;axis++)projection.m[axis][axis]=depth;
		d->SetTransform(storm_render::TransformProjection,&projection);
		set(32,depth,0,0,0);set(33,0,depth,0,0);set(34,0,0,depth,0);set(35,0,0,0,depth);
		d->SetVertexShaderConstantF(0,&c[0][0],256);
		for(unsigned path=0;path<4;path++) {
			auto render=[&](){
				d->BindVertexProgram(path<2?vs:nullptr);
				if(path==1)return bridge();
				if(path==0){draw(grass,sizeof(G));return pixel(64,64);}
				if(path==2){d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);draw(flat,sizeof(Flat));return pixel(64,64);}
				d->Clear(storm_render::clear::color, 0xff070b13, 1);d->BeginScene();
				StormMetalDrawBillboards(d,&billboard,1,"fog parity",1,1,1,1);d->EndScene();return pixel(64,64);
			};
			d->SetRenderState(storm_render::RenderFog,false);uint32_t base=render();
			for(unsigned mode:{unsigned(storm_render::FogNone),unsigned(storm_render::FogExponential),unsigned(storm_render::FogExponentialSquared),unsigned(storm_render::FogLinear)}) {
				for(bool table:{true,false}) {
					d->SetRenderState(storm_render::RenderFog,true);
					d->SetRenderState(storm_render::RenderFogTableMode,table?mode:storm_render::FogNone);
					d->SetRenderState(storm_render::RenderFogVertexMode,table?storm_render::FogNone:mode);
					float factor=mode==storm_render::FogExponential?std::exp(-.35f*depth):mode==storm_render::FogExponentialSquared?std::exp(-std::pow(.35f*depth,2.f)):mode==storm_render::FogLinear?1.f-depth/4.f:1.f;
					uint32_t actual=render();const int fog[]={96,160,224};bool matches=true;
					for(unsigned channel=0;channel<3;channel++){unsigned shift=16-channel*8;int expected=std::lround(fog[channel]+(int(base>>shift&255)-fog[channel])*factor);matches&=std::abs(int(actual>>shift&255)-expected)<=2;}
					if(!matches)std::fprintf(stderr,"fog path=%s depth=%.1f mode=%u table=%u base=%08x actual=%08x factor=%.5f\n",paths[path],depth,mode,table,base,actual,factor);
					check(matches,"world-space fog colour/depth/mode parity");
				}
			}
		}
	}
	d->BindVertexProgram(nullptr);vs->Release();read->Release();target->Release();d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);t1->Release();t->Release();d->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS land original bytecodes -> Metal -> FFP pixels; grass/geometry/billboard fog parity");
}

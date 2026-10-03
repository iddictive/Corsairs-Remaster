#include <SDL.h>
#include <d3d9.h>
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
static std::vector<DWORD> load(std::string p){std::ifstream f(p,std::ios::binary|std::ios::ate);check(bool(f),p.c_str());size_t n=f.tellg();std::vector<DWORD> v(n/4);f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){
 check(argc==2,"usage: land-shader-probe techniques");SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Land shader fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);
 D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=p.BackBufferHeight=128;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.Windowed=TRUE;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;check(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
	IDirect3DTexture9*t=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT lr{};t->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0xff808080;t->UnlockRect(0);
	IDirect3DTexture9*t1=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t1,nullptr);t1->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0x00808080;t1->UnlockRect(0);
 for(int i=0;i<2;i++){d->SetTexture(i,t);d->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetTextureStageState(i,D3DTSS_TEXCOORDINDEX,i);}
 d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixel=[&](int x,int y){check(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT r{};read->LockRect(&r,nullptr,D3DLOCK_READONLY);DWORD v;memcpy(&v,(char*)r.pBits+y*r.Pitch+x*4,4);read->UnlockRect();return v;};
 auto near=[&](DWORD v,int r,int g,int b,const char*s){printf("%s %08x\n",s,v);check(abs(int(v>>16&255)-r)<4&&abs(int(v>>8&255)-g)<4&&abs(int(v&255)-b)<4,s);};
 auto draw=[&](void*v,int stride){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff070b13,1,0);d->BeginScene();auto hr=d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,v,stride);d->EndScene();check(SUCCEEDED(hr),"actual shader draw");};
 float c[256][4]{};auto set=[&](int n,float x,float y,float z,float w){c[n][0]=x;c[n][1]=y;c[n][2]=z;c[n][3]=w;};
 auto tokens=load(std::string(argv[1])+"/worldmap/worldmap_shader.vso");check(identifyLandVertexShader(tokens.data())==LandShaderKind::Worldmap,"strict worldmap fingerprint");auto changed=tokens;changed[4]^=1;check(identifyLandVertexShader(changed.data())==LandShaderKind::None,"changed shader rejected by fingerprint");IDirect3DVertexShader9*vs=nullptr;d->CreateVertexShader(tokens.data(),&vs);d->SetVertexShader(vs);d->SetPixelShader(nullptr);
 struct W{float x,y,z;DWORD color;float u,v;};W cloud[]={{-.8f,-.8f,.5f,0xff804000,0,0},{.8f,-.8f,.5f,0xff804000,1,0},{0,.8f,.5f,0xff804000,.5f,1}};
 set(0,1,0,0,0);set(1,0,1,0,0);set(2,0,0,1,0);set(3,0,0,0,1);set(5,1,0,0,.5);set(6,.4,.6,.8,1);d->SetVertexShaderConstantF(0,&c[0][0],256);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
 d->SetTextureStageState(2,D3DTSS_COLOROP,D3DTOP_ADD);d->SetTextureStageState(2,D3DTSS_COLORARG1,D3DTA_SPECULAR);d->SetTextureStageState(2,D3DTSS_COLORARG2,D3DTA_CURRENT);d->SetTextureStageState(2,D3DTSS_ALPHAOP,D3DTOP_DISABLE);d->SetTextureStageState(3,D3DTSS_COLOROP,D3DTOP_DISABLE);
 draw(cloud,sizeof(W));near(pixel(64,64),58,71,84,"cloud original three-stage diffuse/specular");near(pixel(2,2),7,11,19,"outside triangle");d->SetVertexShader(nullptr);vs->Release();
 tokens=load(std::string(argv[1])+"/effects/grass_main.vso");check(identifyLandVertexShader(tokens.data())==LandShaderKind::Grass,"strict grass fingerprint");d->CreateVertexShader(tokens.data(),&vs);d->SetVertexShader(vs);
	struct G{float x,y,z;DWORD params,offset;float wx,wy,alpha;};G grass[]={{0,-.8f,.5f,0,0,0,0,1},{0,-.8f,.5f,0,0x00ff0000,0,0,1},{0,-.8f,.5f,0,0x0080ff00,0,0,1}};struct BG{float x,y,z;DWORD data;float wx,wz,alpha;};BG bridgeGrass[]={{0,-.8f,.5f,0,0,0,1}};
	memset(c,0,sizeof(c));set(0,1,0,0,1);set(32,1,0,0,0);set(33,0,1,0,0);set(34,0,0,1,0);set(35,0,0,0,1);set(37,.25,.5,.75,1);set(38,0,0,0,1);set(39,.9,1,.245,-.245);set(40,15,-.5,1,.8);set(41,0,0,1.6,2.56);d->SetVertexShaderConstantF(0,&c[0][0],256);
	d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);draw(grass,sizeof(G));near(pixel(64,64),64,128,192,"grass original transform/color and modulate2x");
	near(pixel(88,40),7,11,19,"grass pre-wind outside");for(auto&v:grass)v.wx=.4f;draw(grass,sizeof(G));near(pixel(88,40),64,128,192,"grass wind moves original blade geometry");
	for(auto&v:grass){v.wx=0;v.alpha=1;}d->SetVertexShaderConstantF(0,&c[0][0],256);d->SetTexture(1,t1);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
	auto bridge=[&](){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff070b13,1,0);d->BeginScene();bool ok=StormMetalDrawGrass(d,bridgeGrass,1,sizeof(BG),&c[0][0],42*4,true,1,1,.2,true,80,true,D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA);d->EndScene();check(ok,"grass bridge accepted compact fixture");return pixel(64,64);};
	auto restored=[&](DWORD cull,DWORD colorOp,DWORD alphaOp,const char*label){DWORD value=0;d->GetRenderState(D3DRS_CULLMODE,&value);check(value==cull,label);d->GetTextureStageState(1,D3DTSS_COLOROP,&value);check(value==colorOp,label);d->GetTextureStageState(1,D3DTSS_ALPHAOP,&value);check(value==alphaOp,label);};
	d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);DWORD clean=bridge();restored(D3DCULL_NONE,D3DTOP_DISABLE,D3DTOP_DISABLE,"grass bridge restores clean caller state");
	d->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);DWORD cw=bridge();restored(D3DCULL_CW,D3DTOP_DISABLE,D3DTOP_DISABLE,"grass bridge restores CW caller state");
	d->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);DWORD ccw=bridge();restored(D3DCULL_CCW,D3DTOP_DISABLE,D3DTOP_DISABLE,"grass bridge restores CCW caller state");
	d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);DWORD stage1=bridge();restored(D3DCULL_NONE,D3DTOP_MODULATE,D3DTOP_SELECTARG1,"grass bridge restores stage-1 caller state");
	printf("grass bridge inherited clean=%08x cw=%08x ccw=%08x stage1=%08x\n",clean,cw,ccw,stage1);near(clean,64,128,192,"grass bridge clean technique state");near(cw,64,128,192,"grass bridge ignores inherited CW cull");near(ccw,64,128,192,"grass bridge ignores inherited CCW cull");near(stage1,64,128,192,"grass bridge disables inherited stage-1 color/alpha");
	bridgeGrass[0].alpha=.1f;d->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);DWORD cutout=bridge();restored(D3DCULL_CW,D3DTOP_MODULATE,D3DTOP_SELECTARG1,"grass bridge restores cutout caller state");near(cutout,7,11,19,"grass bridge retains authored alpha cutout");
	for(auto&v:grass){v.wx=0;v.alpha=.1;}d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);d->SetRenderState(D3DRS_ALPHAREF,80);draw(grass,sizeof(G));near(pixel(64,64),7,11,19,"grass fade respects original alpha test");
	// Every world-space consumer must use the same fog colour and camera depth.
	// The existing grass fixtures previously exercised only fog-disabled draws.
	struct Flat {float x,y,z;DWORD color;float u,v;};
	Flat flat[]={{-.8f,-.8f,.5f,0xff4080bf,0,0},{.8f,-.8f,.5f,0xff4080bf,1,0},{0,.8f,.5f,0xff4080bf,.5f,1}};
	struct Billboard {float x,y,z,size,angle;DWORD color,subtexture;};
	Billboard billboard{0,0,.5f,.8f,0,0xff4080bf,0};
	bridgeGrass[0].alpha=1;for(auto&v:grass)v.alpha=1;
	d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetRenderState(D3DRS_LIGHTING,FALSE);
	d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
	d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);d->SetRenderState(D3DRS_FOGCOLOR,0xff60a0e0);
	auto stateFloat=[&](D3DRENDERSTATETYPE state,float value){DWORD bits;memcpy(&bits,&value,4);d->SetRenderState(state,bits);};
	stateFloat(D3DRS_FOGDENSITY,.35f);stateFloat(D3DRS_FOGSTART,0);stateFloat(D3DRS_FOGEND,4);
	const char*paths[]={"expanded grass","compact grass","fixed geometry","billboard"};
	for(float depth:{1.f,3.f}) {
		D3DMATRIX projection{};for(int axis=0;axis<4;axis++)projection.m[axis][axis]=depth;
		d->SetTransform(D3DTS_PROJECTION,&projection);
		set(32,depth,0,0,0);set(33,0,depth,0,0);set(34,0,0,depth,0);set(35,0,0,0,depth);
		d->SetVertexShaderConstantF(0,&c[0][0],256);
		for(unsigned path=0;path<4;path++) {
			auto render=[&](){
				d->SetVertexShader(path<2?vs:nullptr);
				if(path==1)return bridge();
				if(path==0){draw(grass,sizeof(G));return pixel(64,64);}
				if(path==2){d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);draw(flat,sizeof(Flat));return pixel(64,64);}
				d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff070b13,1,0);d->BeginScene();
				StormMetalDrawBillboards(d,&billboard,1,"fog parity",1,1,1,1);d->EndScene();return pixel(64,64);
			};
			d->SetRenderState(D3DRS_FOGENABLE,FALSE);DWORD base=render();
			for(unsigned mode:{unsigned(D3DFOG_NONE),unsigned(D3DFOG_EXP),unsigned(D3DFOG_EXP2),unsigned(D3DFOG_LINEAR)}) {
				for(bool table:{true,false}) {
					d->SetRenderState(D3DRS_FOGENABLE,TRUE);
					d->SetRenderState(D3DRS_FOGTABLEMODE,table?mode:D3DFOG_NONE);
					d->SetRenderState(D3DRS_FOGVERTEXMODE,table?D3DFOG_NONE:mode);
					float factor=mode==D3DFOG_EXP?std::exp(-.35f*depth):mode==D3DFOG_EXP2?std::exp(-std::pow(.35f*depth,2.f)):mode==D3DFOG_LINEAR?1.f-depth/4.f:1.f;
					DWORD actual=render();const int fog[]={96,160,224};bool matches=true;
					for(unsigned channel=0;channel<3;channel++){unsigned shift=16-channel*8;int expected=std::lround(fog[channel]+(int(base>>shift&255)-fog[channel])*factor);matches&=std::abs(int(actual>>shift&255)-expected)<=2;}
					if(!matches)std::fprintf(stderr,"fog path=%s depth=%.1f mode=%u table=%u base=%08x actual=%08x factor=%.5f\n",paths[path],depth,mode,table,base,actual,factor);
					check(matches,"world-space fog colour/depth/mode parity");
				}
			}
		}
	}
	d->SetVertexShader(nullptr);vs->Release();read->Release();target->Release();d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);t1->Release();t->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS land original bytecodes -> Metal -> FFP pixels; grass/geometry/billboard fog parity");
}

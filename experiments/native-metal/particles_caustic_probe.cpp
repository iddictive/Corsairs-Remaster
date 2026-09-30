#include <SDL.h>
#include <d3d9.h>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void need(bool v,const char*s){if(!v){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static std::vector<DWORD> load(std::string p){std::ifstream f(p,std::ios::binary|std::ios::ate);need(bool(f),p.c_str());size_t n=f.tellg();need(n&&n%4==0,"shader bytes");std::vector<DWORD>b(n/4);f.seekg(0);f.read((char*)b.data(),n);need(bool(f),"shader read");return b;}
int main(int argc,char**argv){need(argc==2,"arg: RESOURCE/techniques");bool modern=std::getenv("STORM_METAL_MODERN_EFFECTS")&&std::strcmp(std::getenv("STORM_METAL_MODERN_EFFECTS"),"1")==0;need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Particle/caustic GPU fixture",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,SDL_GetError());auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.BackBufferCount=1;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 IDirect3DTexture9*t[4]{};auto paint=[&](int i,DWORD color){D3DLOCKED_RECT r{};need(SUCCEEDED(t[i]->LockRect(0,&r,nullptr,0)),"texture lock");for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)r.pBits+y*r.Pitch))[x]=color;t[i]->UnlockRect(0);};
 for(int i=0;i<4;i++){need(SUCCEEDED(d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t[i],nullptr)),"texture");d->SetTexture(i,t[i]);d->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);}
 d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixel=[&](int x=32,int y=32){need(SUCCEEDED(d->GetRenderTargetData(target,read)),"GPU readback");D3DLOCKED_RECT r{};need(SUCCEEDED(read->LockRect(&r,nullptr,D3DLOCK_READONLY)),"readback lock");DWORD p;memcpy(&p,(char*)r.pBits+y*r.Pitch+x*4,4);read->UnlockRect();return p;};
 auto check=[&](DWORD p,int rgb,int alpha,const char*label){printf("%s: %08x expected gray=%d alpha=%d\n",label,p,rgb,alpha);need(std::abs(int(p&255)-rgb)<=3&&std::abs(int((p>>8)&255)-rgb)<=3&&std::abs(int((p>>16)&255)-rgb)<=3&&std::abs(int(p>>24)-alpha)<=3,label);};
 WORD indices[]={0,1,2};auto draw=[&](const void*v,UINT stride){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff070b13,1,0);d->BeginScene();HRESULT r=d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,3,1,indices,D3DFMT_INDEX16,v,stride);d->EndScene();need(SUCCEEDED(r),"indexed programmable draw");need((pixel(1,1)&0xffffff)==0x070b13,"outside untouched");return pixel();};
 float c[256][4]{};auto set=[&](int n,float x,float y,float z,float w){c[n][0]=x;c[n][1]=y;c[n][2]=z;c[n][3]=w;};auto matrix=[&](int n){for(int i=0;i<4;i++)c[n+i][i]=1;};
 std::vector<IDirect3DVertexShader9*>vs;std::vector<IDirect3DPixelShader9*>ps;auto bind=[&](const char*stem){auto v=load(std::string(argv[1])+stem+"_vertex_shader.vso"),p=load(std::string(argv[1])+stem+"_pixel_shader.pso");IDirect3DVertexShader9*x=nullptr;IDirect3DPixelShader9*y=nullptr;need(SUCCEEDED(d->CreateVertexShader(v.data(),&x)),"VS create");need(SUCCEEDED(d->CreatePixelShader(p.data(),&y)),"PS create");vs.push_back(x);ps.push_back(y);d->SetVertexShader(x);d->SetPixelShader(y);};
 bind("/particles/particles");set(0,.0416666f,1,0,-.5f);set(1,.159155f,.5f,.25f,6.28319f);set(2,-3.14159f,.0000247609f,-.00138884f,-.000000252399f);matrix(3);matrix(7);set(13,0,1,.5f,0);d->SetVertexShaderConstantF(0,&c[0][0],256);
 struct P{float x,y,z;DWORD color;float u,v,u1,v1,angle,blend,cx,cy,cz,alpha;};P particles[]={{-.8f,-.8f,0,0xffffffff,0,0,0,0,0,1,0,0,.5f,1},{.8f,-.8f,0,0xffffffff,1,0,1,0,0,1,0,0,.5f,1},{0,.8f,0,0xffffffff,.5f,1,.5f,1,0,1,0,0,.5f,1}};
 paint(0,0xffcccccc);paint(1,0xff666666);paint(2,0xffffffff);paint(3,0xff808080);check(draw(particles,sizeof(P)),122,255,"particles frame 0");for(auto&v:particles){v.blend=0;v.alpha=.25f;}check(draw(particles,sizeof(P)),61,64,"particles frame blend and alpha multiplier");
 if(modern){
  // One invisible red frame and one opaque blue frame: hidden red must not tint smoke.
  paint(0,0x00ff0000);paint(1,0xff0000ff);for(auto&v:particles){v.blend=.5f;v.alpha=1;}
  DWORD mixed=draw(particles,sizeof(P));printf("modern transparent-frame crossfade: %08x\n",mixed);
  need(((mixed>>16)&255)<=2&&((mixed>>8)&255)<=2&&std::abs(int(mixed&255)-77)<=3&&std::abs(int(mixed>>24)-128)<=3,"premultiplied frames reject hidden RGB");
  for(auto&v:particles)v.alpha=0;DWORD fire=draw(particles,sizeof(P));need((fire&0xffffff)==(mixed&0xffffff)&&(fire>>24)==0,"AddPowerK zero preserves additive fire RGB");
  for(auto&v:particles)v.color=0x00ffffff;check(draw(particles,sizeof(P)),0,0,"vertex lifecycle fade zero contributes nothing");
 }
 bind("/weather/caustic");memset(c,0,sizeof(c));matrix(0);matrix(4);set(10,1,.5f,0,0);set(11,1,1,1,1);set(14,0,1,0,0);set(15,1,1,0,0);d->SetVertexShaderConstantF(0,&c[0][0],256);
 struct C{float x,y,z,nx,ny,nz;DWORD color;float u,v;};C caustics[]={{-.8f,-.8f,.5f,0,1,0,0xffffffff,0,0},{.8f,-.8f,.5f,0,1,0,0xffffffff,1,0},{0,.8f,.5f,0,1,0,0xffffffff,.5f,1}};
 paint(0,0xffffffff);paint(1,0xffcccccc);paint(2,0xffffffff);check(draw(caustics,sizeof(C)),102,128,"caustic interpolated frames and distance alpha");for(auto&v:caustics)v.ny=-1;check(draw(caustics,sizeof(C)),102,0,"caustic downward normal has no additive contribution");
 d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);for(auto*p:vs)p->Release();for(auto*p:ps)p->Release();for(int i=0;i<4;i++){d->SetTexture(i,nullptr);t[i]->Release();}read->Release();target->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS particles and caustics original bytecode GPU fixture");}

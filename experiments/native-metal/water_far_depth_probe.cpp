#include <SDL.h>
#include <initializer_list>
#include <d3d9.h>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void require(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static std::vector<DWORD> load(const std::string&p){std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f),p.c_str());size_t n=f.tellg();std::vector<DWORD>v(n/4);f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){
 require(argc==2,"water-far-depth-probe techniques");setenv("STORM_METAL_MODERN_WATER","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Depth water GPU fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);require(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=128;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;require(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection{};projection._11=projection._22=1;projection._33=100.f/99.f;projection._34=1;projection._43=-100.f/99.f;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);
 IDirect3DTexture9*bump=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&bump,nullptr);auto paintBump=[&](DWORD value){D3DLOCKED_RECT l{};bump->LockRect(0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=value;bump->UnlockRect(0);};paintBump(0xff80ff80);
 IDirect3DCubeTexture9*cube=nullptr;d->CreateCubeTexture(4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr);auto paintCube=[&](DWORD value){for(int f=0;f<6;f++){D3DLOCKED_RECT l{};cube->LockRect(D3DCUBEMAP_FACES(f),0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=value;cube->UnlockRect(D3DCUBEMAP_FACES(f),0);}};paintCube(0xff000000);
 auto vc=load(std::string(argv[1])+"/weather/sea_sea2_vertex_shader.vso"),pc=load(std::string(argv[1])+"/weather/sea_sea2_pixel_shader.pso");IDirect3DVertexShader9*vs=nullptr;IDirect3DPixelShader9*ps=nullptr;d->CreateVertexShader(vc.data(),&vs);d->CreatePixelShader(pc.data(),&ps);
 float c[256][4]{};auto set=[&](int i,float x,float y,float z,float w){c[i][0]=x;c[i][1]=y;c[i][2]=z;c[i][3]=w;};set(0,0,1,.5,-.04);set(1,2,-1,0,0);set(23,0,0,0,1);set(24,1,0,0,0);set(25,0,1,0,0);set(26,0,0,projection._33,projection._43);set(27,0,0,1,0);set(28,1,1,1,0);set(29,0,0,0,1);set(30,0,0,0,1);set(33,1,0,0,1);set(34,.75,1,.5,1);set(36,1,0,0,0);set(37,0,1,0,0);set(38,0,0,1,0);set(58,0,1,0,1);d->SetVertexShaderConstantF(0,&c[0][0],256);
 IDirect3DSurface9*target=nullptr,*read=nullptr,*originalDepth=nullptr;d->GetRenderTarget(0,&target);d->GetDepthStencilSurface(&originalDepth);d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixel=[&](int x=64,int y=64){require(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT l{};read->LockRect(&l,nullptr,D3DLOCK_READONLY);DWORD v;memcpy(&v,(char*)l.pBits+y*l.Pitch+x*4,4);read->UnlockRect();return v;};
 struct V{float x,y,z;DWORD color;};struct S{float x,y,z,nx,ny,nz,u,v;};S sea[]={{-8,-8,4,0,0,-1,0,0},{8,-8,4,0,0,-1,1,0},{0,8,4,0,0,-1,.5,1}};
 auto geometry=[&](float depth,DWORD color,float left=-2,float right=2){V v[]={{left*depth,-2*depth,depth,color},{right*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,2*depth,depth,color}};d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetVertexDeclaration(nullptr);d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);d->SetTexture(0,nullptr);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);require(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"background geometry");};
 auto water=[&](){d->SetVertexShader(vs);d->SetPixelShader(ps);d->SetTexture(0,bump);d->SetTexture(3,cube);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCALPHA);require(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,sea,sizeof(S))),"water draw");};
 auto start=[&](){require(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"frame boundary");d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);require(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)),"fresh color+depth");d->BeginScene();};
 auto finish=[&](){d->EndScene();return pixel();};

 auto sampleDepth=[&](float surface,float bottom){
  projection={};projection._11=projection._22=1;projection._33=10000.f/(10000.f-.1f);projection._34=1;projection._43=-.1f*10000.f/(10000.f-.1f);d->SetTransform(D3DTS_PROJECTION,&projection);
  set(26,0,0,projection._33,projection._43);d->SetVertexShaderConstantF(0,&c[0][0],256);
  sea[0].x=-2*surface;sea[0].y=-2*surface;sea[1].x=2*surface;sea[1].y=-2*surface;sea[2].y=2*surface;for(auto&v:sea)v.z=surface;
  start();geometry(bottom,0xffffffff);water();return finish();
 };
 DWORD nearby=sampleDepth(4,14),distant=sampleDepth(1000,1010);
 printf("equal 10-world-unit depth near=%08x distant=%08x\n",nearby,distant);
 for(int shift:{0,8,16})require(abs(int(nearby>>shift&255)-int(distant>>shift&255))<=8,"same physical water thickness cannot become transparent with camera distance");
 originalDepth->Release();target->Release();read->Release();bump->Release();cube->Release();vs->Release();ps->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS far-depth water fixture");}

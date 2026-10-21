#include <SDL.h>
#include <d3d9.h>

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
struct Q {float center[3],axisX[3],axisY[3],halfWidth,halfHeight;uint32_t color;float uv[4];};
extern "C" bool StormMetalDrawWdmQuad(void*,const Q&,const char*);
int main(int argc,char**argv){
 (void)argc;(void)argv;SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Land shader fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);
 D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=p.BackBufferHeight=128;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.Windowed=TRUE;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;check(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
	IDirect3DTexture9*t=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT lr{};t->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0xff808080;t->UnlockRect(0);
	IDirect3DTexture9*t1=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t1,nullptr);t1->LockRect(0,&lr,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0x00808080;t1->UnlockRect(0);
 for(int i=0;i<2;i++){d->SetTexture(i,t);d->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetTextureStageState(i,D3DTSS_TEXCOORDINDEX,i);}
 d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);

 auto pixel=[&](int x,int y){check(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT r{};read->LockRect(&r,nullptr,D3DLOCK_READONLY);DWORD v;memcpy(&v,(char*)r.pBits+y*r.Pitch+x*4,4);read->UnlockRect();return v;};
 d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);
 D3DMATRIX m{};for(int i=0;i<4;i++)m.m[i][i]=1;d->SetTransform(D3DTS_WORLD,&m);d->SetTransform(D3DTS_VIEW,&m);d->SetTransform(D3DTS_PROJECTION,&m);
 d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 Q q{{0,0,.5f},{1,0,0},{0,1,0},.8f,.8f,0xc0687078,{0,0,1,1}};
 auto draw=[&](const char* material){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff202830,1,0);d->BeginScene();bool ok=StormMetalDrawWdmQuad(d,q,material);d->EndScene();check(ok,"actual compact quad bridge");};
 draw("WdmRain");DWORD center=pixel(64,64),edge=pixel(104,104),mid=pixel(64,96);printf("rain center=%08x mid=%08x edge=%08x\n",center,mid,edge);
 check((center&255)>(mid&255)&&(mid&255)>48,"continuous rain profile");check((edge&0xffffff)==0x202830,"zero outside analytic parcel");
 for(int i=0;i<5;i++){draw("WdmRain");check(pixel(64,64)==center&&pixel(64,96)==mid,"fixed field has no respawn flicker");}
 q.color=0x00687078;draw("WdmRain");check((pixel(64,64)&0xffffff)==0x202830,"dry identity");
 q.color=0xc0687078;draw("ordinary_quad");check((pixel(104,104)&0xffffff)!=0x202830,"other quad path unchanged");
 read->Release();target->Release();d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);t1->Release();t->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS native rain veil: soft field, stable pixels, dry identity, ordinary quad negative");}

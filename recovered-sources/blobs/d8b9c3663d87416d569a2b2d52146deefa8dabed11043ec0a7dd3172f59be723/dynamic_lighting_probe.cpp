#include <initializer_list>
#include <vector>
#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void need(bool x,const char*s){if(!x){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalResolveLandLighting(void*);
extern "C" bool StormMetalLandShadowResolved(void*);
int main(){setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Weapon FFP contract",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 IDirect3DTexture9*t=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT l{};t->LockRect(0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=0xff204060;t->UnlockRect(0);

 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection=identity;projection._11=projection._22=1.f/16;projection._33=1.f/32;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);
 struct V{float x,y,z,nx,ny,nz;DWORD color;float u,v;};
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixels=[&](){need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT q{};read->LockRect(&q,nullptr,D3DLOCK_READONLY);std::vector<DWORD> out(4096);for(int y=0;y<64;y++)memcpy(out.data()+y*64,(char*)q.pBits+y*q.Pitch,256);read->UnlockRect();return out;};
 auto energy=[](DWORD c){return int(c&255)+int(c>>8&255)+int(c>>16&255);};
 auto quad=[&](float x,float radius,float z,DWORD color,unsigned role){V v[]={{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,-radius,z,0,0,-1,color,1,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,-radius,z,0,0,-1,color,0,0},{x+radius,radius,z,0,0,-1,color,1,1},{x-radius,radius,z,0,0,-1,color,0,1}};unsigned previous=StormMetalBeginLandModel(d,role);need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"scoped geometry");StormMetalEndLandModel(d,previous);};
 auto frame=[&](bool active,int ambient,int sunlight,float casterX,bool resolve){d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,active);d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);DWORD a=ambient;d->SetRenderState(D3DRS_AMBIENT,a|(a<<8)|(a<<16));D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};float k=sunlight/255.f;sun.Diffuse={k,k,k,1};d->SetLight(0,&sun);d->LightEnable(0,TRUE);d->SetTexture(0,t);d->SetTexture(1,nullptr);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);quad(0,14,8,0xff202020,1);if(casterX<50)quad(casterX,1.5,3,0xff202020,1);bool applied=resolve&&StormMetalResolveLandLighting(d);if(active&&resolve)need(applied&&StormMetalLandShadowResolved(d),"shadow resolve succeeds");if(!active)need(!StormMetalLandShadowResolved(d),"disabled location keeps legacy fallback");quad(11,1,2,0xff808080,0);d->EndScene();return pixels();};
 auto day=frame(true,70,180,100,true),night=frame(true,15,20,100,true);printf("lighting day=%08x night=%08x\n",day[32*64+32],night[32*64+32]);need(energy(day[2080])>energy(night[2080])*2,"weather lights replace fixed baked COL");
 need(day[32*64+54]==night[32*64+54],"unscoped unlit overlay remains weather independent");
 auto base=frame(false,70,180,100,false);need(energy(day[2080])>energy(base[2080]),"explicit static scope replaces baked dark color");
 auto shadowA=frame(true,70,180,-3,true),shadowB=frame(true,70,180,3,true);int darkA=0,darkB=0,moved=0;for(int y=27;y<37;y++)for(int x=18;x<49;x++){// Exclude both visible caster footprints; inspect receiver only.
 if((x>=22&&x<=30)||(x>=34&&x<=42))continue;int i=y*64+x;if(energy(shadowA[i])+8<energy(day[i]))darkA++;if(energy(shadowB[i])+8<energy(day[i]))darkB++;if(abs(energy(shadowA[i])-energy(shadowB[i]))>8)moved++;}printf("shadow receiver pixels %d %d moved %d\n",darkA,darkB,moved);need(darkA>2&&darkB>2&&moved>4,"moving caster moves receiver shadow outside caster silhouette");
 d->SetTexture(0,nullptr);t->Release();read->Release();target->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS dynamic lighting and real caster GPU fixture");}

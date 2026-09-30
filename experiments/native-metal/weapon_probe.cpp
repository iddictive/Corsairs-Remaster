#include <initializer_list>
#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void need(bool x,const char*s){if(!x){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
int main(){SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Weapon FFP contract",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 IDirect3DTexture9*t=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr);D3DLOCKED_RECT l{};t->LockRect(0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=0xff204060;t->UnlockRect(0);
 IDirect3DCubeTexture9*cube=nullptr;d->CreateCubeTexture(2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr);for(int f=0;f<6;f++){cube->LockRect(D3DCUBEMAP_FACES(f),0,&l,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=0xffc04020;cube->UnlockRect(D3DCUBEMAP_FACES(f),0);}
 D3DMATRIX m{};m._11=m._22=m._33=m._44=1;for(auto state:{D3DTS_WORLD,D3DTS_VIEW,D3DTS_PROJECTION,D3DTS_TEXTURE1})d->SetTransform(state,&m);
 struct V{float x,y,z,nx,ny,nz;DWORD c;float u,v;};V v[]={{-.9f,-.9f,.5f,0,0,-1,0xff808080,0,0},{.9f,-.9f,.5f,0,0,-1,0xff808080,1,0},{0,.9f,.5f,0,0,-1,0xff808080,.5f,1}};
 d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetTexture(0,t);d->SetTexture(1,nullptr);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE2X);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
 d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_BLENDCURRENTALPHA);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_CURRENT);d->SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);d->SetTextureStageState(1,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3);d->SetTextureStageState(2,D3DTSS_COLOROP,D3DTOP_DISABLE);
 IDirect3DSurface9*target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto draw=[&](int r,int g,int b,const char*name){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0);d->BeginScene();need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,v,sizeof(V))),"draw");d->EndScene();need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT q{};read->LockRect(&q,nullptr,D3DLOCK_READONLY);DWORD c;memcpy(&c,(char*)q.pBits+32*q.Pitch+32*4,4);read->UnlockRect();printf("%s %08x\n",name,c);need(abs(int(c>>16&255)-r)<3&&abs(int(c>>8&255)-g)<3&&abs(int(c&255)-b)<3,name);};
 draw(32,64,96,"EnvAmmo missing reflection retains gun diffuse");d->SetTexture(1,cube);draw(192,64,32,"EnvAmmo bound cubemap retains authored reflection");
 d->SetTexture(1,nullptr);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_DIFFUSE);draw(16,32,48,"textureless CURRENT DIFFUSE stage remains active");
 d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);t->Release();cube->Release();read->Release();target->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS weapon null-stage GPU contract");}

#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char*text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main(){
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Stars multistream fixture",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);check(w,SDL_GetError());auto*api=Direct3DCreate9(D3D_SDK_VERSION);
 D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 const float positions[]={-.5f,0,.5f,.5f,0,.5f};const DWORD colors[]={0xffff0000,0xff00ff00};IDirect3DVertexBuffer9 *position=nullptr,*color=nullptr;void*bytes=nullptr;
 check(SUCCEEDED(d->CreateVertexBuffer(sizeof(positions),0,0,D3DPOOL_MANAGED,&position,nullptr)),"position buffer");position->Lock(0,0,&bytes,0);memcpy(bytes,positions,sizeof(positions));position->Unlock();check(SUCCEEDED(d->CreateVertexBuffer(sizeof(colors),0,0,D3DPOOL_MANAGED,&color,nullptr)),"color buffer");color->Lock(0,0,&bytes,0);memcpy(bytes,colors,sizeof(colors));color->Unlock();
 const D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{1,0,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},D3DDECL_END()};IDirect3DVertexDeclaration9*decl=nullptr;d->CreateVertexDeclaration(elements,&decl);d->SetVertexDeclaration(decl);d->SetStreamSource(0,position,0,12);d->SetStreamSource(1,color,0,4);
 IDirect3DVertexBuffer9*bound=nullptr;UINT offset=0,stride=0;check(SUCCEEDED(d->GetStreamSource(0,&bound,&offset,&stride))&&bound==position&&stride==12,"stream1 preserves stream0");bound->Release();
 IDirect3DTexture9*texture=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr);D3DLOCKED_RECT lock{};texture->LockRect(0,&lock,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)lock.pBits+y*lock.Pitch))[x]=0xffffffff;texture->UnlockRect(0);d->SetTexture(0,texture);
 d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_POINTSPRITEENABLE,TRUE);float pointSize=16;DWORD bits;memcpy(&bits,&pointSize,4);d->SetRenderState(D3DRS_POINTSIZE,bits);
 IDirect3DSurface9 *target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 auto draw=[&](UINT start,UINT count){d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0);d->BeginScene();auto result=d->DrawPrimitive(D3DPT_POINTLIST,start,count);d->EndScene();return result;};
 auto pixel=[&](int x,int y){check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");D3DLOCKED_RECT r{};readback->LockRect(&r,nullptr,D3DLOCK_READONLY);DWORD value;memcpy(&value,(char*)r.pBits+y*r.Pitch+x*4,4);readback->UnlockRect();return value;};
 check(SUCCEEDED(draw(0,2)),"original stars declaration draw");check(pixel(16,32)==0xffff0000&&pixel(48,32)==0xff00ff00,"independent position and color streams");check(pixel(22,32)==0xffff0000&&pixel(26,32)==0xff000000,"point screen-space size");
 check(SUCCEEDED(draw(1,1)),"nonzero start vertex");check(pixel(16,32)==0xff000000&&pixel(48,32)==0xff00ff00,"start applies to both streams");check(FAILED(draw(1,2)),"out-of-range stream read rejected");
 d->SetStreamSource(1,nullptr,0,0);check(SUCCEEDED(d->GetStreamSource(0,&bound,&offset,&stride))&&bound==position,"unbind stream1 preserves positions");bound->Release();check(FAILED(draw(0,1)),"missing color stream rejected");
 target->Release();readback->Release();texture->Release();decl->Release();position->Release();color->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: stars FFP declaration, multistream, point sprites, bounds");
}

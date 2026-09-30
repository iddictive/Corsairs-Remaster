#include <SDL.h>
#include <d3d9.h>
#include "indexed_geometry.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Indexed FFP fixture",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);check(w,SDL_GetError());auto*api=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 struct V{float x,y,z,rhw;DWORD color;float u,v;};const V vertices[]={{8,8,.5f,1,0xffff2020,0,0},{56,8,.5f,1,0xff20ff20,1,0},{8,56,.5f,1,0xff2020ff,0,1},{56,56,.5f,1,0xffeeeeee,1,1}};
 d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 IDirect3DTexture9*texture=nullptr;d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr);D3DLOCKED_RECT lock{};texture->LockRect(0,&lock,nullptr,0);const DWORD texels[]={0xffffffff,0xff808080,0xffc0a080,0xff80a0c0};for(int y=0;y<2;y++)std::memcpy((char*)lock.pBits+y*lock.Pitch,texels+y*2,8);texture->UnlockRect(0);d->SetTexture(0,texture);
 IDirect3DVertexBuffer9*vb=nullptr;d->CreateVertexBuffer(sizeof(vertices)+16,0,0,D3DPOOL_MANAGED,&vb,nullptr);void*data=nullptr;vb->Lock(0,0,&data,0);std::memset(data,0xee,16);std::memcpy((char*)data+16,vertices,sizeof(vertices));vb->Unlock();d->SetStreamSource(0,vb,16,sizeof(V));
 IDirect3DSurface9 *target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 auto pixels=[&](){check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");D3DLOCKED_RECT r{};readback->LockRect(&r,nullptr,D3DLOCK_READONLY);std::vector<DWORD> result(64*64);for(int y=0;y<64;y++)std::memcpy(result.data()+y*64,(char*)r.pBits+y*r.Pitch,64*4);readback->UnlockRect();return result;};
 struct Case{D3DPRIMITIVETYPE type;UINT primitives;std::vector<uint32_t> indices;};
 const Case cases[]={{D3DPT_TRIANGLELIST,2,{0,1,2,2,1,3}},{D3DPT_TRIANGLESTRIP,4,{0,1,2,3,2,1}},{D3DPT_TRIANGLEFAN,2,{0,1,3,2}},{D3DPT_LINELIST,4,{0,1,1,3,3,2,2,0}},{D3DPT_LINESTRIP,5,{0,1,3,2,0,1}},{D3DPT_TRIANGLELIST,1,{0,1,3}}};
 for(auto&c:cases){
  std::vector<V> expanded;for(auto index:c.indices)expanded.push_back(vertices[index]);d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0);d->BeginScene();check(SUCCEEDED(d->DrawPrimitiveUP(c.type,c.primitives,expanded.data(),sizeof(V))),"expanded reference");d->EndScene();auto reference=pixels();
  for(bool wide:{false,true}){size_t width=wide?4:2;std::vector<uint8_t> indexBytes((c.indices.size()+1)*width);for(size_t i=0;i<c.indices.size();i++){uint32_t value=c.indices[i]+2;if(wide)std::memcpy(indexBytes.data()+(i+1)*width,&value,4);else{uint16_t small=uint16_t(value);std::memcpy(indexBytes.data()+(i+1)*width,&small,2);}}
   IDirect3DIndexBuffer9*ib=nullptr;d->CreateIndexBuffer(UINT(indexBytes.size()),0,wide?D3DFMT_INDEX32:D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr);ib->Lock(0,0,&data,0);std::memcpy(data,indexBytes.data(),indexBytes.size());ib->Unlock();d->SetIndices(ib);d->SetStreamSource(0,vb,16,sizeof(V));d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0);d->BeginScene();check(SUCCEEDED(d->DrawIndexedPrimitive(c.type,-2,2,4,1,c.primitives)),"signed base and nonzero min/start/stream offset");d->EndScene();check(pixels()==reference,"indexed topology/colors/UV match expanded");
   check(FAILED(d->DrawIndexedPrimitive(c.type,-3,2,4,1,c.primitives)),"negative physical vertex rejected");check(FAILED(d->DrawIndexedPrimitive(c.type,-2,3,3,1,c.primitives)),"index below declared min rejected");check(FAILED(d->DrawIndexedPrimitive(c.type,-2,2,3,1,c.primitives)),"index above declared range rejected");check(FAILED(d->DrawIndexedPrimitive(c.type,-2,2,5,1,c.primitives)),"declared VB overrun rejected");check(FAILED(d->DrawIndexedPrimitive(c.type,-2,2,4,2,c.primitives)),"IB overrun rejected");check(FAILED(d->DrawIndexedPrimitive(c.type,-2,2,4,UINT32_MAX,c.primitives)),"index offset overflow rejected");d->SetIndices(nullptr);ib->Release();
  }
  d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0);d->BeginScene();check(SUCCEEDED(d->DrawIndexedPrimitiveUP(c.type,0,4,c.primitives,c.indices.data(),D3DFMT_INDEX32,vertices,sizeof(V))),"indexed UP");d->EndScene();check(pixels()==reference,"indexed UP matches expanded");
 }
 sm::IndexedSpan span;const uint16_t repeated[]={2,3,4,4,3,5};check(sm::indexedSpan(repeated,false,6,-2,2,4,4,span)&&span.vertices()==4,"six references convert four vertices");check(!sm::indexedSpan(repeated,false,6,INT32_MIN,2,4,4,span),"signed base extreme rejected");check(!sm::indexedSpan(repeated,false,6,0,UINT32_MAX,4,4,span),"declared range overflow rejected");
 std::puts("PASS: 16/32-bit indexed list/strip/fan/lines match expanded colors+UV; signed base/min/start/offset; invalid VB/IB/ranges; 6 references -> 4 conversions");target->Release();readback->Release();texture->Release();vb->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();
}

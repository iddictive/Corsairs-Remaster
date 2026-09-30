#include <SDL.h>
#include <d3d9.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" void StormMetalLandInterior(void*,bool);
extern "C" unsigned StormMetalBeginLandModel(void*,unsigned);
extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*,unsigned);
extern "C" bool StormMetalBeginShadowFrame(void*,uint64_t,uint64_t,const float*);
extern "C" bool StormMetalBeginShadowPass(void*,unsigned,unsigned,uint64_t,const float*,float,float*,float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
static void need(bool value,const char*name){if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
int main(){
 setenv("STORM_METAL_DYNAMIC_LIGHTING","1",1);SDL_Init(SDL_INIT_VIDEO);
 auto*w=SDL_CreateWindow("Character contact shadow",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);need(w,"window");auto*api=Direct3DCreate9(D3D_SDK_VERSION);
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
 IDirect3DDevice9*d=nullptr;need(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 IDirect3DTexture9*texture=nullptr;need(SUCCEEDED(d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr)),"texture");D3DLOCKED_RECT locked{};texture->LockRect(0,&locked,nullptr,0);for(int y=0;y<2;y++)for(int x=0;x<2;x++)reinterpret_cast<DWORD*>(static_cast<char*>(locked.pBits)+y*locked.Pitch)[x]=0xff6080a0;texture->UnlockRect(0);
 IDirect3DSurface9*target=nullptr,*readback=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr);
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection=identity;projection._11=projection._22=1.f/8;projection._33=1.f/32;
 struct Vertex{float x,y,z,nx,ny,nz,u,v;};
 auto draw=[&](float radius,float z,unsigned role,bool sceneModel){Vertex v[]={{-radius,-radius,z,0,0,-1,0,0},{radius,-radius,z,0,0,-1,1,0},{radius,radius,z,0,0,-1,1,1},{-radius,-radius,z,0,0,-1,0,0},{radius,radius,z,0,0,-1,1,1},{-radius,radius,z,0,0,-1,0,1}};unsigned previous=sceneModel?StormMetalBeginSceneModel(d):StormMetalBeginLandModel(d,role);need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(Vertex))),"geometry");StormMetalEndLandModel(d,previous);};
 auto quad=[&](float radius,float z,unsigned role){draw(radius,z,role,false);};
 uint64_t frame=0;
 auto render=[&](float gap,bool blendedCaster=false,bool translucentCaster=false){
  d->Present(nullptr,nullptr,nullptr,nullptr);StormMetalLandLocation(d,true);StormMetalLandInterior(d,false);d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);d->BeginScene();
  d->SetTransform(D3DTS_WORLD,&identity);d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1);d->SetTexture(0,texture);d->SetTexture(1,nullptr);
  d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_COLORVERTEX,FALSE);d->SetRenderState(D3DRS_AMBIENT,0x181818);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
  d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
  D3DMATERIAL9 material{};material.Diffuse=material.Ambient={1,1,1,1};d->SetMaterial(&material);D3DLIGHT9 sun{};sun.Type=D3DLIGHT_DIRECTIONAL;sun.Direction={.6f,0,1};sun.Diffuse={.7f,.7f,.7f,1};d->SetLight(0,&sun);d->LightEnable(0,TRUE);
  float center[]={0,0,8},ray[]={.6f,0,1};need(StormMetalBeginShadowFrame(d,1,++frame,center),"frame");D3DMATRIX view{},lightProjection{};need(StormMetalBeginShadowPass(d,0,0,1,ray,32,&view.m[0][0],&lightProjection.m[0][0]),"sun depth");d->SetTransform(D3DTS_VIEW,&view);d->SetTransform(D3DTS_PROJECTION,&lightProjection);
  quad(7,8,1);if(gap>=0){d->SetRenderState(D3DRS_ALPHABLENDENABLE,blendedCaster||translucentCaster);d->SetRenderState(D3DRS_ZWRITEENABLE,!translucentCaster);draw(1.5f,8-gap,0,true);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);}StormMetalEndShadowPass(d);need(StormMetalEndShadowFrame(d),"shadow ready");
  d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);quad(7,8,2);d->EndScene();need(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback");readback->LockRect(&locked,nullptr,D3DLOCK_READONLY);std::vector<DWORD> pixels(4096);for(int y=0;y<64;y++)std::memcpy(pixels.data()+y*64,static_cast<char*>(locked.pBits)+y*locked.Pitch,256);readback->UnlockRect();return pixels;
 };
 auto empty=render(-1),coplanar=render(0),contact=render(.02f),blended=render(.02f,true),translucent=render(.02f,false,true);
 auto brightness=[](DWORD c){return int(c&255)+int((c>>8)&255)+int((c>>16)&255);};int acne=0,dark=0,blendedDark=0;
 for(int y=18;y<46;y++)for(int x=18;x<46;x++){int i=y*64+x;if(brightness(coplanar[i])+4<brightness(empty[i]))acne++;if(brightness(contact[i])+8<brightness(empty[i]))dark++;if(brightness(blended[i])+8<brightness(empty[i]))blendedDark++;}
 std::printf("character 2cm contact: occluded=%d blended=%d coplanar_acne=%d\n",dark,blendedDark,acne);need(acne==0,"coplanar duplicate does not produce shadow acne");need(dark>8,"2cm character self-occlusion survives shadow bias");need(blendedDark>8,"Z-writing blended character remains a caster");need(translucent==empty,"non-Z-writing translucent model remains excluded from caster pass");
 unsigned location=StormMetalBeginLandModel(d,1);unsigned nested=StormMetalBeginSceneModel(d);need(nested==1,"scene model preserves location scope");StormMetalEndLandModel(d,nested);unsigned after=StormMetalBeginLandModel(d,1);need(after==1,"nested model restores location scope");StormMetalEndLandModel(d,after);StormMetalEndLandModel(d,location);
 d->SetTexture(0,nullptr);texture->Release();target->Release();readback->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS character contact shadows");
}

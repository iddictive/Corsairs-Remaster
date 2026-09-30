#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void storm_metal_label_texture(IDirect3DBaseTexture9*,const char*);
static void need(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
 need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Alpha sampler fixture",0,0,32,32,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL);need(w,SDL_GetError());auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.BackBufferCount=1;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;need(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 IDirect3DTexture9*t=nullptr;need(SUCCEEDED(d->CreateTexture(1,1,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr)),"texture");D3DLOCKED_RECT lock{};t->LockRect(0,&lock,nullptr,0);*(DWORD*)lock.pBits=0xff00ff00;t->UnlockRect(0);d->SetTexture(0,t);
 struct V{float x,y,z,rhw,u,v;};V quad[]={{0,0,.5,1,2,2},{32,0,.5,1,2,2},{0,32,.5,1,2,2},{32,32,.5,1,2,2}};d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
 IDirect3DSurface9 *target=nullptr,*read=nullptr;d->GetRenderTarget(0,&target);d->CreateOffscreenPlainSurface(32,32,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);auto draw=[&](DWORD address){d->SetSamplerState(0,D3DSAMP_ADDRESSU,address);d->SetSamplerState(0,D3DSAMP_ADDRESSV,address);d->SetSamplerState(0,D3DSAMP_BORDERCOLOR,0);d->Clear(0,nullptr,D3DCLEAR_TARGET,0xffff0000,1,0);d->BeginScene();need(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(V))),"draw");d->EndScene();need(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT r{};read->LockRect(&r,nullptr,D3DLOCK_READONLY);DWORD value;std::memcpy(&value,(char*)r.pBits+16*r.Pitch+16*4,4);read->UnlockRect();return value;};
 need(draw(D3DTADDRESS_WRAP)==0xff00ff00,"wrap repeats texture");need(draw(D3DTADDRESS_CLAMP)==0xff00ff00,"clamp keeps edge texel");need(draw(D3DTADDRESS_BORDER)==0xffff0000,"transparent border preserves destination");// Town backdrops repeat horizontally, but their opaque bottom must never
 // wrap into the authored transparent upper edge under bilinear filtering.
 IDirect3DTexture9* backdrop=nullptr;need(SUCCEEDED(d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&backdrop,nullptr)),"backdrop texture");
 backdrop->LockRect(0,&lock,nullptr,0);DWORD rows[4]={0x00000000,0x00000000,0xff00ff00,0xff0000ff};for(unsigned y=0;y<2;y++)std::memcpy((char*)lock.pBits+y*lock.Pitch,rows+y*2,8);backdrop->UnlockRect(0);d->SetTexture(0,backdrop);
 d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
 for(auto&v:quad){v.u=.25f;v.v=0;}
 need(draw(D3DTADDRESS_WRAP)!=0xffff0000,"ordinary wrapped texture reproduces edge bleed");
 storm_metal_label_texture(backdrop,"RESOURCE\\Textures\\backJungleU1.tga.tx");
 need(draw(D3DTADDRESS_WRAP)==0xffff0000,"backdrop top edge stays transparent");
 for(auto&v:quad){v.u=1.25f;v.v=.75f;}
 need(draw(D3DTADDRESS_WRAP)==0xff00ff00,"backdrop horizontal repetition remains");
 storm_metal_label_texture(backdrop,"trees.tga");for(auto&v:quad){v.u=.25f;v.v=0;}
 need(draw(D3DTADDRESS_WRAP)!=0xffff0000,"unrelated alpha atlas retains sampler contract");
 backdrop->Release();
 target->Release();read->Release();t->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: wrap, clamp and transparent border sampler semantics");
}

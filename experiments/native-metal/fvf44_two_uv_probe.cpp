#include <SDL.h>
#include <d3d9.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" void StormMetalLandLocation(void*, bool);
extern "C" void StormMetalLandInterior(void*, bool);
extern "C" unsigned StormMetalBeginLandModel(void*, unsigned);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" bool StormMetalBeginShadowFrame(void*, uint64_t, uint64_t, const float*);
extern "C" bool StormMetalBeginShadowPass(void*, unsigned, unsigned, uint64_t, const float*, float, float*, float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" uint64_t StormMetalRawLocationDraws(void*);

static void need(bool value, const char* label) { if (!value) { std::fprintf(stderr, "FAIL %s\n", label); std::exit(1); } }

struct Vertex { float x,y,z,nx,ny,nz; DWORD color; float u0,v0,u1,v1; };
static_assert(sizeof(Vertex) == 44, "FVF XYZ|NORMAL|DIFFUSE|TEX2 must remain 44 bytes");

int main() {
  setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
  setenv("STORM_METAL_MODERN_LIGHTING", "1", 1);
  need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
  auto* window = SDL_CreateWindow("FVF44 two-UV raw location", 0, 0, 64, 64, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
  need(window, "window");
  auto* api = Direct3DCreate9(D3D_SDK_VERSION); need(api, "D3D9");
  D3DPRESENT_PARAMETERS pp{}; pp.Windowed=TRUE; pp.BackBufferWidth=pp.BackBufferHeight=64; pp.BackBufferFormat=D3DFMT_A8R8G8B8; pp.hDeviceWindow=window; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
  IDirect3DDevice9* device=nullptr; need(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,0,&pp,&device)), "device");
  auto texture=[&](DWORD a,DWORD b) {
    IDirect3DTexture9* result=nullptr; need(SUCCEEDED(device->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&result,nullptr)), "texture");
    D3DLOCKED_RECT lock{}; need(SUCCEEDED(result->LockRect(0,&lock,nullptr,0)), "texture lock");
    for(int y=0;y<2;++y) { auto* row=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch); row[0]=y ? b : a; row[1]=y ? a : b; }
    need(SUCCEEDED(result->UnlockRect(0)), "texture unlock"); return result;
  };
  IDirect3DTexture9* base=texture(0xffffffff,0xffffffff);
  // Strong repeat-only signal: UV0 stays constant while UV1 covers sixteen tiles.
  IDirect3DTexture9* detail=texture(0xff202020,0xff808080);
  IDirect3DSurface9 *target=nullptr,*readback=nullptr;
  need(SUCCEEDED(device->GetRenderTarget(0,&target)), "target");
  need(SUCCEEDED(device->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr)), "readback");
  D3DMATRIX identity{}; identity._11=identity._22=identity._33=identity._44=1.f;
  D3DMATRIX projection=identity; projection._11=projection._22=1.f/8.f; projection._33=1.f/32.f;
  const Vertex vertices[]={{-7,-7,8,0,0,-1,0xffffffff,.25f,.25f,0,0},{7,-7,8,0,0,-1,0xffffffff,.25f,.25f,16,0},{7,7,8,0,0,-1,0xffffffff,.25f,.25f,16,16},{-7,7,8,0,0,-1,0xffffffff,.25f,.25f,0,16}};
  const WORD indices[]={0,1,2,0,2,3};
  auto sourceDraw=[&](float radius,float z) {
    Vertex caster[]={{-radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,0,0},{radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,8,0},{radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,8,8},{-radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,0,0},{radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,8,8},{-radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,0,8}};
    unsigned previous=StormMetalBeginLandModel(device,1); need(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,caster,sizeof(Vertex))), "shadow source geometry"); StormMetalEndLandModel(device,previous);
  };
  auto boundDraw=[&] {
    IDirect3DVertexBuffer9* vb=nullptr; IDirect3DIndexBuffer9* ib=nullptr;
    const DWORD fvf=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX2;
    need(SUCCEEDED(device->CreateVertexBuffer(sizeof(vertices),D3DUSAGE_WRITEONLY,fvf,D3DPOOL_MANAGED,&vb,nullptr)), "FVF44 VB");
    need(SUCCEEDED(device->CreateIndexBuffer(sizeof(indices),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)), "FVF44 IB");
    void* storage=nullptr; need(SUCCEEDED(vb->Lock(0,0,&storage,0)), "FVF44 VB lock"); std::memcpy(storage,vertices,sizeof(vertices)); need(SUCCEEDED(vb->Unlock()), "FVF44 VB unlock");
    need(SUCCEEDED(ib->Lock(0,0,&storage,0)), "FVF44 IB lock"); std::memcpy(storage,indices,sizeof(indices)); need(SUCCEEDED(ib->Unlock()), "FVF44 IB unlock");
    need(SUCCEEDED(device->SetStreamSource(0,vb,0,sizeof(Vertex))), "FVF44 stream"); need(SUCCEEDED(device->SetIndices(ib)), "FVF44 indices");
    unsigned previous=StormMetalBeginLandModel(device,1); need(SUCCEEDED(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)), "FVF44 bound location geometry"); StormMetalEndLandModel(device,previous);
    need(SUCCEEDED(device->SetStreamSource(0,nullptr,0,0)), "stream reset"); need(SUCCEEDED(device->SetIndices(nullptr)), "indices reset"); vb->Release(); ib->Release();
  };
  uint64_t generation=0;
  auto frame=[&](bool bound,bool stage1UsesUV0) {
    device->Present(nullptr,nullptr,nullptr,nullptr); StormMetalLandLocation(device,true); StormMetalLandInterior(device,false);
    need(SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)), "clear"); need(SUCCEEDED(device->BeginScene()), "begin scene");
    device->SetTransform(D3DTS_WORLD,&identity); device->SetTransform(D3DTS_VIEW,&identity); device->SetTransform(D3DTS_PROJECTION,&projection);
    device->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX2); device->SetTexture(0,base); device->SetTexture(1,detail);
    for(unsigned slot=0;slot<2;++slot) { device->SetSamplerState(slot,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP); device->SetSamplerState(slot,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP); }
    device->SetSamplerState(1,D3DSAMP_MINFILTER,D3DTEXF_POINT); device->SetSamplerState(1,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1); device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE); device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0); device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1); device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE2X); device->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT); device->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TEXTURE); device->SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,stage1UsesUV0?0:1); device->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1); device->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_CURRENT); device->SetTextureStageState(2,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE); device->SetRenderState(D3DRS_ZENABLE,TRUE); device->SetRenderState(D3DRS_ZWRITEENABLE,TRUE); device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE); device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE); device->SetRenderState(D3DRS_LIGHTING,TRUE); device->SetRenderState(D3DRS_COLORVERTEX,FALSE); device->SetRenderState(D3DRS_AMBIENT,0);
    D3DMATERIAL9 material{}; material.Diffuse={1,1,1,1}; material.Ambient={0,0,0,1}; material.Emissive={1,1,1,1}; device->SetMaterial(&material);
    D3DLIGHT9 sun{}; sun.Type=D3DLIGHT_DIRECTIONAL; sun.Direction={.6f,0,1}; sun.Diffuse={0,0,0,1}; device->SetLight(0,&sun); device->LightEnable(0,TRUE);
    const float focus[]={0,0,8},ray[]={.6f,0,1}; ++generation;
    need(StormMetalBeginShadowFrame(device,700 + generation,generation,focus), "shadow frame");
    for(unsigned face=0;face<2;++face) { D3DMATRIX lightView{},lightProjection{}; need(StormMetalBeginShadowPass(device,0,face,1,ray,face?96.f:32.f,&lightView.m[0][0],&lightProjection.m[0][0]), "sun cascade"); device->SetTransform(D3DTS_VIEW,&lightView); device->SetTransform(D3DTS_PROJECTION,&lightProjection); sourceDraw(7,8); StormMetalEndShadowPass(device); }
    need(StormMetalEndShadowFrame(device), "shadow frame resolves"); device->SetTransform(D3DTS_VIEW,&identity); device->SetTransform(D3DTS_PROJECTION,&projection);
    if(bound) boundDraw(); else { const Vertex expanded[]={vertices[0],vertices[1],vertices[2],vertices[0],vertices[2],vertices[3]}; unsigned previous=StormMetalBeginLandModel(device,1); need(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,expanded,sizeof(Vertex))), "FVF44 expanded geometry"); StormMetalEndLandModel(device,previous); }
    need(SUCCEEDED(device->EndScene()), "end scene"); need(SUCCEEDED(device->GetRenderTargetData(target,readback)), "readback"); D3DLOCKED_RECT lock{}; need(SUCCEEDED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY)), "readback lock"); std::vector<DWORD> pixels(64*64); for(unsigned y=0;y<64;++y)std::memcpy(pixels.data()+y*64,static_cast<char*>(lock.pBits)+y*lock.Pitch,64*sizeof(DWORD)); need(SUCCEEDED(readback->UnlockRect()), "readback unlock"); return pixels;
  };
  const auto expanded=frame(false,false); const auto uv0Mutation=frame(false,true);
  need(expanded!=uv0Mutation, "stage 1 must not silently sample constant UV0 instead of high-repeat UV1");
  const uint64_t rawBefore=StormMetalRawLocationDraws(device); setenv("STORM_METAL_REQUIRE_NATIVE_3D","1",1); const auto raw=frame(true,false); unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
  need(StormMetalRawLocationDraws(device)==rawBefore+1, "bound FVF44 role-1 receiver must use raw location path");
  need(raw==expanded, "raw role-1 FVF44 TEX2 result matches expanded compatibility path");
  device->SetTexture(0,nullptr); device->SetTexture(1,nullptr); detail->Release(); base->Release(); readback->Release(); target->Release(); device->Release(); api->Release(); SDL_DestroyWindow(window); SDL_Quit();
  std::puts("PASS FVF44 TEX2 raw role-1 UV1 discrimination");
}

#include <SDL.h>
#include "render_device_api.hpp"
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

struct Vertex { float x,y,z,nx,ny,nz; uint32_t color; float u0,v0,u1,v1; };
static_assert(sizeof(Vertex) == 44, "FVF XYZ|NORMAL|DIFFUSE|TEX2 must remain 44 bytes");

int main() {
  setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
  setenv("STORM_METAL_MODERN_LIGHTING", "1", 1);
  need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
  auto* window = SDL_CreateWindow("FVF44 two-UV raw location", 0, 0, 64, 64, SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
  need(window, "window");

  storm_render::Presentation pp{};  pp.width=pp.height=64; pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
  storm_render::Device* device=nullptr; need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)), "device");
  auto texture=[&](uint32_t a,uint32_t b) {
    storm_render::Texture* result=nullptr; need(storm_render::succeeded(device->CreateTexture(2, 2, 1, 0, storm_render::AssetFormat::A8R8G8B8, &result)), "texture");
    storm_render::LockedRect lock{}; need(storm_render::succeeded(result->LockRect(0,&lock,nullptr,0)), "texture lock");
    for(int y=0;y<2;++y) { auto* row=reinterpret_cast<uint32_t*>(static_cast<char*>(lock.pBits)+y*lock.Pitch); row[0]=y ? b : a; row[1]=y ? a : b; }
    need(storm_render::succeeded(result->UnlockRect(0)), "texture unlock"); return result;
  };
  storm_render::Texture* base=texture(0xffffffff,0xffffffff);
  // Strong repeat-only signal: UV0 stays constant while UV1 covers sixteen tiles.
  storm_render::Texture* detail=texture(0xff202020,0xff808080);
  storm_render::Surface *target=nullptr,*readback=nullptr;
  need(storm_render::succeeded(device->GetRenderTarget(&target)), "target");
  need(storm_render::succeeded(device->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback)), "readback");
  storm_render::Matrix identity{}; identity._11=identity._22=identity._33=identity._44=1.f;
  storm_render::Matrix projection=identity; projection._11=projection._22=1.f/8.f; projection._33=1.f/32.f;
  const Vertex vertices[]={{-7,-7,8,0,0,-1,0xffffffff,.25f,.25f,0,0},{7,-7,8,0,0,-1,0xffffffff,.25f,.25f,16,0},{7,7,8,0,0,-1,0xffffffff,.25f,.25f,16,16},{-7,7,8,0,0,-1,0xffffffff,.25f,.25f,0,16}};
  const uint16_t indices[]={0,1,2,0,2,3};
  auto sourceDraw=[&](float radius,float z) {
    Vertex caster[]={{-radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,0,0},{radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,8,0},{radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,8,8},{-radius,-radius,z,0,0,-1,0xffffffff,.25f,.25f,0,0},{radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,8,8},{-radius,radius,z,0,0,-1,0xffffffff,.25f,.25f,0,8}};
    unsigned previous=StormMetalBeginLandModel(device,1); need(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,caster,sizeof(Vertex))), "shadow source geometry"); StormMetalEndLandModel(device,previous);
  };
  auto boundDraw=[&] {
    storm_render::VertexBuffer* vb=nullptr; storm_render::IndexBuffer* ib=nullptr;
    const uint32_t fvf=storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex2;
    need(storm_render::succeeded(device->CreateVertexBuffer(sizeof(vertices), storm_render::usage::writeOnly, fvf, &vb)), "FVF44 VB");
    need(storm_render::succeeded(device->CreateIndexBuffer(sizeof(indices), storm_render::usage::writeOnly, storm_render::IndexFormat::UInt16, &ib)), "FVF44 IB");
    void* storage=nullptr; need(storm_render::succeeded(vb->Lock(0,0,&storage,0)), "FVF44 VB lock"); std::memcpy(storage,vertices,sizeof(vertices)); need(storm_render::succeeded(vb->Unlock()), "FVF44 VB unlock");
    need(storm_render::succeeded(ib->Lock(0,0,&storage,0)), "FVF44 IB lock"); std::memcpy(storage,indices,sizeof(indices)); need(storm_render::succeeded(ib->Unlock()), "FVF44 IB unlock");
    need(storm_render::succeeded(device->SetStreamSource(0,vb,0,sizeof(Vertex))), "FVF44 stream"); need(storm_render::succeeded(device->SetIndices(ib)), "FVF44 indices");
    unsigned previous=StormMetalBeginLandModel(device,1); need(storm_render::succeeded(device->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)), "FVF44 bound location geometry"); StormMetalEndLandModel(device,previous);
    need(storm_render::succeeded(device->SetStreamSource(0,nullptr,0,0)), "stream reset"); need(storm_render::succeeded(device->SetIndices(nullptr)), "indices reset"); vb->Release(); ib->Release();
  };
  uint64_t generation=0;
  auto frame=[&](bool bound,bool stage1UsesUV0) {
    device->Present(); StormMetalLandLocation(device,true); StormMetalLandInterior(device,false);
    need(storm_render::succeeded(device->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1)), "clear"); need(storm_render::succeeded(device->BeginScene()), "begin scene");
    device->SetTransform(storm_render::TransformWorld,&identity); device->SetTransform(storm_render::TransformView,&identity); device->SetTransform(storm_render::TransformProjection,&projection);
    device->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex2); device->SetTexture(0,base); device->SetTexture(1,detail);
    for(unsigned slot=0;slot<2;++slot) { device->SetSamplerState(slot,storm_render::SamplerAddressU,storm_render::TextureAddressWrap); device->SetSamplerState(slot,storm_render::SamplerAddressV,storm_render::TextureAddressWrap); }
    device->SetSamplerState(1,storm_render::SamplerMinFilter,storm_render::TextureFilterPoint); device->SetSamplerState(1,storm_render::SamplerMagFilter,storm_render::TextureFilterPoint);
    device->SetTextureStageState(0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1); device->SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::texture); device->SetTextureStageState(0,storm_render::StageTexCoordIndex,0); device->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1); device->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
    device->SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpModulate2X); device->SetTextureStageState(1,storm_render::StageColorArg1,storm_render::textureArg::current); device->SetTextureStageState(1,storm_render::StageColorArg2,storm_render::textureArg::texture); device->SetTextureStageState(1,storm_render::StageTexCoordIndex,stage1UsesUV0?0:1); device->SetTextureStageState(1,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1); device->SetTextureStageState(1,storm_render::StageAlphaArg1,storm_render::textureArg::current); device->SetTextureStageState(2,storm_render::StageColorOp,storm_render::TextureOpDisable);
    device->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone); device->SetRenderState(storm_render::RenderDepthTest,true); device->SetRenderState(storm_render::RenderDepthWrite,true); device->SetRenderState(storm_render::RenderAlphaTest,false); device->SetRenderState(storm_render::RenderBlend,false); device->SetRenderState(storm_render::RenderLighting,true); device->SetRenderState(storm_render::RenderVertexColor,false); device->SetRenderState(storm_render::RenderAmbient,0);
    storm_render::Material material{}; material.Diffuse={1,1,1,1}; material.Ambient={0,0,0,1}; material.Emissive={1,1,1,1}; device->SetMaterial(&material);
    storm_render::Light sun{}; sun.Type=storm_render::LightKind::Directional; sun.Direction={.6f,0,1}; sun.Diffuse={0,0,0,1}; device->SetLight(0,&sun); device->LightEnable(0,true);
    const float focus[]={0,0,8},ray[]={.6f,0,1}; ++generation;
    need(StormMetalBeginShadowFrame(device,700 + generation,generation,focus), "shadow frame");
    for(unsigned face=0;face<2;++face) { storm_render::Matrix lightView{},lightProjection{}; need(StormMetalBeginShadowPass(device,0,face,1,ray,face?96.f:32.f,&lightView.m[0][0],&lightProjection.m[0][0]), "sun cascade"); device->SetTransform(storm_render::TransformView,&lightView); device->SetTransform(storm_render::TransformProjection,&lightProjection); sourceDraw(7,8); StormMetalEndShadowPass(device); }
    need(StormMetalEndShadowFrame(device), "shadow frame resolves"); device->SetTransform(storm_render::TransformView,&identity); device->SetTransform(storm_render::TransformProjection,&projection);
    if(bound) boundDraw(); else { const Vertex expanded[]={vertices[0],vertices[1],vertices[2],vertices[0],vertices[2],vertices[3]}; unsigned previous=StormMetalBeginLandModel(device,1); need(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,2,expanded,sizeof(Vertex))), "FVF44 expanded geometry"); StormMetalEndLandModel(device,previous); }
    need(storm_render::succeeded(device->EndScene()), "end scene"); need(storm_render::succeeded(device->GetRenderTargetData(target,readback)), "readback"); storm_render::LockedRect lock{}; need(storm_render::succeeded(readback->LockRect(&lock,nullptr,storm_render::lock::readOnly)), "readback lock"); std::vector<uint32_t> pixels(64*64); for(unsigned y=0;y<64;++y)std::memcpy(pixels.data()+y*64,static_cast<char*>(lock.pBits)+y*lock.Pitch,64*sizeof(uint32_t)); need(storm_render::succeeded(readback->UnlockRect()), "readback unlock"); return pixels;
  };
  const auto expanded=frame(false,false); const auto uv0Mutation=frame(false,true);
  need(expanded!=uv0Mutation, "stage 1 must not silently sample constant UV0 instead of high-repeat UV1");
  const uint64_t rawBefore=StormMetalRawLocationDraws(device); setenv("STORM_METAL_REQUIRE_NATIVE_3D","1",1); const auto raw=frame(true,false); unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
  need(StormMetalRawLocationDraws(device)==rawBefore+1, "bound FVF44 role-1 receiver must use raw location path");
  need(raw==expanded, "raw role-1 FVF44 TEX2 result matches expanded compatibility path");
  device->SetTexture(0,nullptr); device->SetTexture(1,nullptr); detail->Release(); base->Release(); readback->Release(); target->Release(); device->Release();  SDL_DestroyWindow(window); SDL_Quit();
  std::puts("PASS FVF44 TEX2 raw role-1 UV1 discrimination");
}

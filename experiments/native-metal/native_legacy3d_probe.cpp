#include <SDL.h>
#include "render_device_api.hpp"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

extern "C" unsigned StormMetalBeginWdmModel(void *, unsigned);
extern "C" void StormMetalEndWdmModel(void *, unsigned);
extern "C" uint64_t StormMetalRawWorldDraws(void *);

static void need(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

template <class Vertex, class Index>
static void exercise(storm_render::Device *device, storm_render::Surface *target,
                     storm_render::Surface *readback, uint32_t fvf,
                     std::vector<Vertex> &vertices, bool dynamic, bool uniqueIndices,
                     storm_render::Primitive primitive = storm_render::PrimitiveTriangleList)
{
    const std::vector<Index> indices = primitive == storm_render::PrimitiveTriangleFan
        ? std::vector<Index>{2, 3, 5, 4}
        : primitive == storm_render::PrimitiveTriangleStrip
        ? std::vector<Index>{2, 3, 4, 5}
        : uniqueIndices
        ? std::vector<Index>{2, 3, 4}
        : std::vector<Index>{2, 3, 4, 4, 3, 5};
    const uint32_t primitiveCount = primitive == storm_render::PrimitiveTriangleStrip ||
                                primitive == storm_render::PrimitiveTriangleFan ? 2 :
                                uniqueIndices ? 1 : 2;
    auto pixels = [&] {
        need(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "readback");
        storm_render::LockedRect lock{};
        need(storm_render::succeeded(readback->LockRect(&lock, nullptr, storm_render::lock::readOnly)), "readback lock");
        std::vector<uint32_t> result(64 * 64);
        for (int y = 0; y < 64; ++y)
            std::memcpy(result.data() + y * 64, static_cast<char *>(lock.pBits) + y * lock.Pitch, 64 * 4);
        readback->UnlockRect();
        return result;
    };
    auto reference = [&] {
        const std::vector<unsigned> expandedIndices = primitive == storm_render::PrimitiveTriangleFan
            ? std::vector<unsigned>{0, 1, 3, 2}
            : primitive == storm_render::PrimitiveTriangleStrip
            ? std::vector<unsigned>{0, 1, 2, 3}
            : uniqueIndices
            ? std::vector<unsigned>{0, 1, 2}
            : std::vector<unsigned>{0, 1, 2, 2, 1, 3};
        std::vector<Vertex> expanded;
        for (unsigned index : expandedIndices)
            expanded.push_back(vertices[index]);
        unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
        device->Clear(storm_render::clear::color, 0xff102030, 1);
        device->BeginScene();
        need(storm_render::succeeded(device->DrawPrimitiveUP(primitive, primitiveCount, expanded.data(), sizeof(Vertex))),
             "compatibility reference");
        device->EndScene();
        return pixels();
    };

    storm_render::VertexBuffer *vb = nullptr;
    storm_render::IndexBuffer *ib = nullptr;
    const uint32_t usage = dynamic ? storm_render::usage::dynamic | storm_render::usage::writeOnly : 0;
    const storm_render::MemoryClass pool = dynamic ? storm_render::MemoryClass::Default : storm_render::MemoryClass::Managed;
    need(storm_render::succeeded(device->CreateVertexBuffer(uint32_t(vertices.size() * sizeof(Vertex)), usage, fvf, &vb)), "VB");
    need(storm_render::succeeded(device->CreateIndexBuffer(uint32_t(indices.size() * sizeof(Index)), usage, sizeof(Index) == 4 ? storm_render::IndexFormat::UInt32 : storm_render::IndexFormat::UInt16, &ib)), "IB");
    auto upload = [&] {
        void *bytes = nullptr;
        need(storm_render::succeeded(vb->Lock(0, 0, &bytes, dynamic ? storm_render::lock::discard : 0)), "VB lock");
        std::memcpy(bytes, vertices.data(), vertices.size() * sizeof(Vertex));
        need(storm_render::succeeded(vb->Unlock()), "VB unlock");
        need(storm_render::succeeded(ib->Lock(0, 0, &bytes, dynamic ? storm_render::lock::discard : 0)), "IB lock");
        std::memcpy(bytes, indices.data(), indices.size() * sizeof(Index));
        need(storm_render::succeeded(ib->Unlock()), "IB unlock");
    };
    upload();
    device->SetStreamSource(0, vb, 0, sizeof(Vertex));
    device->SetIndices(ib);

    auto drawNative = [&](const std::vector<uint32_t> &expected) {
        setenv("STORM_METAL_REQUIRE_NATIVE_3D", "1", 1);
        device->Clear(storm_render::clear::color, 0xff102030, 1);
        device->BeginScene();
        const storm_render::Result drawResult = device->DrawIndexedPrimitive(
            primitive, -2, 2,
            primitive == storm_render::PrimitiveTriangleStrip || primitive == storm_render::PrimitiveTriangleFan
                ? 4 : uniqueIndices ? 3 : 4,
            0, primitiveCount);
        if (storm_render::failed(drawResult))
            std::fprintf(stderr, "native draw failed: 0x%08x primitive=%u\n",
                         unsigned(drawResult), unsigned(primitive));
        need(storm_render::succeeded(drawResult), "validated indexed FVF stays native");
        device->EndScene();
        const auto actual = pixels();
        std::size_t coverageMismatch = 0, colorMismatch = 0;
        unsigned maxChannelDelta = 0;
        for (std::size_t i = 0; i < actual.size(); ++i) {
            const bool expectedCovered = expected[i] != 0xff102030;
            const bool actualCovered = actual[i] != 0xff102030;
            coverageMismatch += expectedCovered != actualCovered;
            if (expected[i] != actual[i]) ++colorMismatch;
            for (unsigned shift = 0; shift < 32; shift += 8) {
                const int delta = int((expected[i] >> shift) & 0xff) -
                                  int((actual[i] >> shift) & 0xff);
                maxChannelDelta = std::max(maxChannelDelta, unsigned(std::abs(delta)));
            }
        }
        if (coverageMismatch || maxChannelDelta > 1)
            std::fprintf(stderr,
                         "primitive=%u dynamic=%d unique=%d coverage=%zu colors=%zu maxDelta=%u\n",
                         unsigned(primitive), dynamic, uniqueIndices, coverageMismatch,
                         colorMismatch, maxChannelDelta);
        need(coverageMismatch == 0, "native raw FVF geometry matches compatibility coverage");
        need(maxChannelDelta <= 1, "native raw FVF lighting matches compatibility within one channel step");
    };
    drawNative(reference());
    if (dynamic) {
        vertices[0].x += .08f;
        upload();
        drawNative(reference());
    }

    unsetenv("STORM_METAL_REQUIRE_NATIVE_3D");
    device->SetIndices(nullptr);
    device->SetStreamSource(0, nullptr, 0, 0);
    ib->Release();
    vb->Release();
}

int main()
{
    need(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Native legacy-3D coverage", 0, 0, 64, 64,
                                    SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL);
    need(window != nullptr, SDL_GetError());

    storm_render::Presentation present{};
    present.width = present.height = 64;
    present.colorFormat = storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device *device = nullptr;
    need(storm_render::succeeded(storm_render::createMetalDevice(window, &present, &device)), "device");

    storm_render::Matrix identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1;
    device->SetTransform(storm_render::TransformWorld, &identity);
    device->SetTransform(storm_render::TransformView, &identity);
    device->SetTransform(storm_render::TransformProjection, &identity);
    device->SetRenderState(storm_render::RenderDepthTest, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetRenderState(storm_render::RenderLighting, true);
    device->SetRenderState(storm_render::RenderAmbient, 0xff303030);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    device->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    storm_render::Material material{};
    material.Diffuse = {.8f, .65f, .5f, 1};
    material.Ambient = {.3f, .25f, .2f, 1};
    device->SetMaterial(&material);
    storm_render::Light light{};
    light.Type = storm_render::LightKind::Directional;
    light.Direction = {0, 0, 1};
    light.Diffuse = {.7f, .8f, .9f, 1};
    device->SetLight(0, &light);
    device->LightEnable(0, true);

    storm_render::Surface *target = nullptr, *readback = nullptr;
    device->GetRenderTarget(&target);
    device->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback);

    struct RichVertex { float x, y, z, nx, ny, nz; uint32_t diffuse, specular; float u0, v0, u1, v1; };
    std::vector<RichVertex> rich = {
        {-.8f, -.8f, .5f, 0, 0, -1, 0xffff3030, 0xff102030, 0, 0, 0, 0},
        { .8f, -.8f, .5f, 0, 0, -1, 0xff30ff30, 0xff203040, 1, 0, 1, 0},
        {-.8f,  .8f, .5f, 0, 0, -1, 0xff3030ff, 0xff304050, 0, 1, 0, 1},
        { .8f,  .8f, .5f, 0, 0, -1, 0xffeeeeee, 0xff405060, 1, 1, 1, 1},
    };
    device->SetFVF(storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::specular | storm_render::vertexFormat::tex2);
    exercise<RichVertex, std::uint16_t>(device, target, readback,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::specular | storm_render::vertexFormat::tex2, rich, false, false);
    exercise<RichVertex, std::uint32_t>(device, target, readback,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::specular | storm_render::vertexFormat::tex2,
        rich, false, false, storm_render::PrimitiveTriangleFan);

    struct CompactVertex { float x, y, z, nx, ny, nz; };
    std::vector<CompactVertex> compact = {
        {-.8f, -.8f, .5f, 0, 0, -1}, { .8f, -.8f, .5f, 0, 0, -1},
        {-.8f,  .8f, .5f, 0, 0, -1}, { .8f,  .8f, .5f, 0, 0, -1},
    };
    device->SetFVF(storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal);
    exercise<CompactVertex, std::uint32_t>(device, target, readback,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal, compact, true, true);

    device->SetRenderState(storm_render::RenderLighting, false);
    device->SetFVF(storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::specular | storm_render::vertexFormat::tex2);
    exercise<RichVertex, std::uint32_t>(device, target, readback,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::specular | storm_render::vertexFormat::tex2,
        rich, true, true, storm_render::PrimitiveTriangleStrip);

    // The authored map is unlit. Opening a model role must not inject stale
    // scene lights, alter vertex alpha or leak the role into the next draw.
    const uint32_t mapFVF=storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::specular|storm_render::vertexFormat::tex2;
    device->SetFVF(mapFVF);
    device->SetRenderState(storm_render::RenderLighting,false);
    device->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);
    for(auto &v:rich)v.diffuse=(v.diffuse&0xffffff)|0x80000000;
    const uint16_t mapIndices[]={0,1,2,2,1,3};
    storm_render::VertexBuffer *mapVB=nullptr; storm_render::IndexBuffer *mapIB=nullptr;
    need(storm_render::succeeded(device->CreateVertexBuffer(uint32_t(rich.size()*sizeof(RichVertex)), 0, mapFVF, &mapVB)),"map VB");
    need(storm_render::succeeded(device->CreateIndexBuffer(sizeof(mapIndices), 0, storm_render::IndexFormat::UInt16, &mapIB)),"map IB");
    void *bytes=nullptr; mapVB->Lock(0,0,&bytes,0); std::memcpy(bytes,rich.data(),rich.size()*sizeof(RichVertex));mapVB->Unlock();
    mapIB->Lock(0,0,&bytes,0);std::memcpy(bytes,mapIndices,sizeof(mapIndices));mapIB->Unlock();
    device->SetStreamSource(0,mapVB,0,sizeof(RichVertex));device->SetIndices(mapIB);
    auto mapPixels=[&] {
        device->Clear(storm_render::clear::color, 0xff102030, 1);device->BeginScene();
        need(storm_render::succeeded(device->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"map draw");device->EndScene();
        need(storm_render::succeeded(device->GetRenderTargetData(target,readback)),"map readback");storm_render::LockedRect r{};
        need(storm_render::succeeded(readback->LockRect(&r,nullptr,storm_render::lock::readOnly)),"map readback lock");
        std::vector<uint32_t> result(64*64);for(int y=0;y<64;y++)std::memcpy(result.data()+y*64,static_cast<char*>(r.pBits)+y*r.Pitch,64*4);
        readback->UnlockRect();return result;
    };
    device->SetRenderState(storm_render::RenderSpecular,true);
    const auto mapReference=mapPixels();const auto worldBefore=StormMetalRawWorldDraws(device);
    const unsigned previous=StormMetalBeginWdmModel(device,1);need(previous==0,"map scope starts empty");
    const auto mapActual=mapPixels();
    need(StormMetalRawWorldDraws(device)==worldBefore+1,"model role reaches resident map path");
    need(mapActual==mapReference,"unlit map preserves full color and alpha image");
    const auto nested=StormMetalBeginWdmModel(device,2);need(nested==1,"nested map role");
    StormMetalEndWdmModel(device,nested);need(StormMetalBeginWdmModel(device,1)==1,"nested role restored");
    StormMetalEndWdmModel(device,previous);need(mapPixels()==mapReference,"outside map scope unchanged");
    need(StormMetalRawWorldDraws(device)==worldBefore+1,"map role does not leak");
    device->SetStreamSource(0,nullptr,0,0);device->SetIndices(nullptr);mapVB->Release();mapIB->Release();

    std::puts("PASS: lit/unlit static/dynamic 16/32-bit indexed list/strip/fan XYZ/NORMAL FVF draws stay native; signed base and rich attributes match");
    readback->Release();
    target->Release();
    device->Release();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

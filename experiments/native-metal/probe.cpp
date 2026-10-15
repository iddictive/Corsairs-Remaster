#include <SDL.h>
#include "render_device_api.hpp"
#include "image_import.hpp"
#include <cstdio>
#include <cstdlib>

static void require(bool value, const char *what) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}

int main() {
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    auto *window = SDL_CreateWindow("Storm Metal API probe", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN);
    require(window, SDL_GetError());


    storm_render::Presentation pp{};
    pp.width = 640; pp.height = 480;
    pp.colorFormat = storm_render::AssetFormat::A8R8G8B8;


    pp.depthEnabled = true; pp.depthFormat = storm_render::AssetFormat::D24S8;
    storm_render::Device *device = nullptr;
    require(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &device)) && device, "CreateDevice");
    storm_render::VertexBuffer *buffer = nullptr;
    require(storm_render::succeeded(device->CreateVertexBuffer(256, 0, storm_render::vertexFormat::xyz, &buffer)) && buffer, "CreateVertexBuffer");
    void *bytes = nullptr;
    require(storm_render::succeeded(buffer->Lock(0, 256, &bytes, 0)) && bytes, "buffer Lock");
    static_cast<unsigned char *>(bytes)[255] = 0x7b;
    require(storm_render::succeeded(buffer->Unlock()), "buffer Unlock");
    require(storm_render::succeeded(buffer->Lock(255, 1, &bytes, storm_render::lock::readOnly)) &&
        *static_cast<unsigned char *>(bytes) == 0x7b, "buffer round trip");
    buffer->Unlock();
    require(storm_render::failed(buffer->Lock(255, 2, &bytes, 0)), "reject out of range Lock");
    buffer->Release();
    struct Vertex { float x, y, z, rhw; uint32_t color; };
    const Vertex vertices[] = {{80,400,0.5f,1,0xffff0000},
        {320,60,0.5f,1,0xff00ff00}, {560,400,0.5f,1,0xff0000ff}};
    device->SetRenderState(storm_render::RenderLighting, false);
    device->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    device->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    device->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    device->SetFVF(storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse);
    for (int frame = 0; frame < 120; ++frame) {
        SDL_Event event; while (SDL_PollEvent(&event)) {}
        require(storm_render::succeeded(device->Clear(storm_render::clear::color | storm_render::clear::depth, 0xff183040, 1.0f)), "Clear");
        require(storm_render::succeeded(device->BeginScene()), "BeginScene");
        require(storm_render::succeeded(device->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, 1,
            vertices, sizeof(Vertex))), "DrawPrimitiveUP");
        require(storm_render::succeeded(device->EndScene()), "EndScene");
        require(storm_render::succeeded(device->Present()), "Present");
    }
    storm_render::Surface *target = nullptr, *readback = nullptr;
    require(storm_render::succeeded(device->GetRenderTarget(&target)) && target, "GetRenderTarget");
    storm_render::Device *owner = nullptr;
    require(storm_render::succeeded(target->GetDevice(&owner)) && owner == device, "surface device owner");
    owner->Release();
    require(storm_render::succeeded(device->CreateOffscreenPlainSurface(640, 480, storm_render::AssetFormat::A8R8G8B8, &readback)) && readback,
        "CreateOffscreenPlainSurface");
    require(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "GPU readback");
    storm_render::LockedRect pixels{};
    require(storm_render::succeeded(readback->LockRect(&pixels, nullptr, storm_render::lock::readOnly)), "readback LockRect");
    auto *outside = static_cast<unsigned char *>(pixels.pBits) + 10 * pixels.Pitch + 10 * 4;
    require(outside[0] == 0x40 && outside[1] == 0x30 && outside[2] == 0x18,
        "GPU clear preserves BGRA channels");
    auto *inside = static_cast<unsigned char *>(pixels.pBits) + 240 * pixels.Pitch + 320 * 4;
    require(inside[1] > 60 && inside[0] > 20 && inside[2] > 20,
        "GPU triangle interpolates vertex colors");
    readback->UnlockRect();
    require(storm_render::succeeded(device->Clear(storm_render::clear::color, 0xff123456, 1)),
        "second GPU write");
    require(storm_render::succeeded(device->GetRenderTargetData(target, readback)), "refresh changed GPU data");
    require(storm_render::succeeded(readback->LockRect(&pixels, nullptr, storm_render::lock::readOnly)), "changed readback LockRect");
    outside = static_cast<unsigned char *>(pixels.pBits);
    require(outside[0] == 0x56 && outside[1] == 0x34 && outside[2] == 0x12,
        "GPU write invalidates prior CPU copy");
    readback->UnlockRect(); readback->Release(); target->Release();
    storm_render::Texture *copySource = nullptr, *copyDestination = nullptr;
    require(storm_render::succeeded(device->CreateTexture(32, 32, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &copySource)), "copy source texture");
    require(storm_render::succeeded(device->CreateTexture(32, 32, 1, 0, storm_render::AssetFormat::A8R8G8B8, &copyDestination)), "copy destination texture");
    require(storm_render::succeeded(copySource->LockRect(0, &pixels, nullptr, 0)), "copy source LockRect");
    for (int y = 0; y < 32; ++y) {
        auto *row = reinterpret_cast<uint32_t *>(static_cast<unsigned char *>(pixels.pBits) + y * pixels.Pitch);
        for (int x = 0; x < 32; ++x) row[x] = 0xff2878c8;
    }
    copySource->UnlockRect(0);
    storm_render::Surface *sourceSurface = nullptr, *destinationSurface = nullptr;
    copySource->GetSurfaceLevel(0, &sourceSurface);
    copyDestination->GetSurfaceLevel(0, &destinationSurface);
    device->GetRenderTarget(&target);
    require(storm_render::succeeded(device->SetRenderTarget(sourceSurface)), "set texture render target");
    storm_render::Viewport viewport{};
    device->GetViewport(&viewport);
    require(viewport.Width == 32 && viewport.Height == 32, "target changes viewport");
    require(storm_render::succeeded(device->SetRenderTarget(target)), "restore backbuffer target");
    device->GetViewport(&viewport);
    require(viewport.Width == 640 && viewport.Height == 480, "restore target restores viewport");
    target->Release();
    require(storm_render::succeeded(storm_render::ImageLoadSurfaceFromSurface(destinationSurface, nullptr, nullptr,
        sourceSurface, nullptr, nullptr, storm_render::ImageDefault, 0)), "actual screenshot D3DX copy path");
    sourceSurface->Release(); destinationSurface->Release();
    copySource->Release(); copyDestination->Release();
    storm_render::VertexBuffer *survivor = nullptr;
    require(storm_render::succeeded(device->CreateVertexBuffer(32, 0, storm_render::vertexFormat::xyz, &survivor)), "retained resource creation");
    device->Release();
    owner = nullptr;
    require(storm_render::succeeded(survivor->GetDevice(&owner)) && owner,
        "resource retains device after caller release");
    owner->Release(); survivor->Release();
    SDL_DestroyWindow(window); SDL_Quit();
    std::puts("PASS: resource bounds, CPU round trip, GPU clear channels and triangle pixels, present");
}

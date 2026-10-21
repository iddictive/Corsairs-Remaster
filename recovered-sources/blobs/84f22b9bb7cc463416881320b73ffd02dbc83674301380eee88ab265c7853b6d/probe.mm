#ifdef BEFORE
#include "before-source/backend.mm"
#else
#include "backend.mm"
#endif
#include <fstream>
#define main ExistingLightingMain
#include "dynamic_lighting_probe.cpp"
#undef main

static void require(bool yes, const char *what) {
    if (!yes) { fprintf(stderr, "FAIL %s\n", what); exit(1); }
}

int main(int argc, char **argv) { @autoreleasepool {
    require(argc == 2, "output directory");
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    setenv("STORM_METAL_MODERN_LIGHTING", "1", 1);
    require(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto *window = SDL_CreateWindow("Frustum", 0, 0, 64, 64,
                                   SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN);
    require(window, "hidden window");
    storm_render::Presentation presentation{};
    presentation.width = presentation.height = 64;
    presentation.colorFormat = storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device *api = nullptr;
    require(storm_render::succeeded(storm_render::createMetalDevice(window, &presentation, &api)), "device");
    auto *device = static_cast<Device *>(api);
    storm_render::Matrix identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1;
    api->SetTransform(storm_render::TransformWorld, &identity);
    api->SetTransform(storm_render::TransformView, &identity);
    api->SetTransform(storm_render::TransformProjection, &identity);
    api->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
    api->SetRenderState(storm_render::RenderLighting, true);
    api->SetRenderState(storm_render::RenderDepthTest, true);
    api->SetRenderState(storm_render::RenderDepthWrite, true);
    api->SetRenderState(storm_render::RenderAlphaTest, false);
    api->SetRenderState(storm_render::RenderBlend, false);
    storm_render::Light sun{};
    sun.Type=storm_render::LightKind::Directional;
    sun.Direction={0,0,1}; sun.Diffuse={1,1,1,1};
    api->SetLight(0,&sun); api->LightEnable(0,true);
    api->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    api->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    api->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    storm_render::Surface *target = nullptr, *readback = nullptr;
    api->GetRenderTarget(&target);
    api->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &readback);
    struct Vertex { float x,y,z,nx,ny,nz,u0,v0,u1,v1,u2,v2; };
    const uint16_t indices[] = {0,1,2};
    struct Case { const char *name; float x,z; bool sail, shadow; float second=0; };
    for (const Case c : {Case{"sail-visible",0,.5,true,true},
                         Case{"sail-outside",3,.5,true,true},
                         Case{"sail-edge",1,.5,true,true},
                         Case{"sail-near",0,-.5,true,true},
                         Case{"rigging-outside",3,.5,false,false},
                         Case{"rigging-visible",0,.5,false,false},
                         Case{"sun-empty",0,.8,true,true,-1},
                         Case{"sun-reach",0,.8,true,true,3},
                         Case{"sun-miss",0,.8,true,true,6}}) {
        api->Clear(storm_render::clear::color | storm_render::clear::depth, 0xff102030, 1);
        api->BeginScene();
        device->landShadow.resetFrame();
        device->landShadow.enabled = true;
        device->landShadow.locationActive = false;
        device->landShadow.scope = 2;
        api->SetTransform(storm_render::TransformWorld,&identity);
        sun.Direction={c.second ? -5.f : 0.f,0,1};
        api->SetLight(0,&sun);
        device->riggingMaterialDraw = !c.shadow;
        Vertex vertices[] = {{c.x-.4f,-.4f,c.z,0,0,-1,0,0,0,0,0,0},
                             {c.x+.4f,-.4f,c.z,0,0,-1,1,0,0,0,0,0},
                             {c.x,.4f,c.z,0,0,-1,.5f,1,0,0,0,0}};
        const unsigned stride = c.sail ? 48 : 32;
        unsigned char bytes[144]{};
        for (unsigned i=0;i<3;++i) memcpy(bytes+i*stride,vertices+i,stride);
        storm_render::VertexBuffer *vb=nullptr;
        storm_render::IndexBuffer *ib=nullptr;
        api->CreateVertexBuffer(3*stride, c.sail ? storm_render::usage::dynamic : 0, 0, &vb);
        api->CreateIndexBuffer(sizeof(indices), c.sail ? storm_render::usage::dynamic : 0,
                              storm_render::IndexFormat::UInt16, &ib);
        void *data=nullptr;
        vb->Lock(0,0,&data,0); memcpy(data,bytes,3*stride); vb->Unlock();
        ib->Lock(0,0,&data,0); memcpy(data,indices,sizeof(indices)); ib->Unlock();
        api->SetFVF(c.sail ? 0x312 : 0x112);
        api->SetStreamSource(0,vb,0,stride); api->SetIndices(ib);
        const auto before = device->telemetryReceiverDraws;
        require(storm_render::succeeded(api->DrawIndexedPrimitive(
            storm_render::PrimitiveTriangleList,0,0,3,0,1)), c.name);
        if(c.second>0){
            auto world=identity;world._41=c.second;world._43=-.6f;
            api->SetTransform(storm_render::TransformWorld,&world);
            require(storm_render::succeeded(api->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,3,0,1)),"hidden solar caster");
        }
        const auto submitted = device->telemetryReceiverDraws-before;
        const auto packets = device->landShadow.packets.size();
        fprintf(stderr,"observed %s receivers=%u packets=%zu\n", c.name,unsigned(submitted),packets);
#ifndef BEFORE
        const bool hidden=std::string(c.name).find("outside")!=std::string::npos||std::string(c.name).find("near")!=std::string::npos;
        require(submitted==unsigned(!hidden),"shared color visibility");
        if(packets)require(device->landShadow.packets[0].receiverVisible==!hidden,"shared receiver visibility");
#endif
        require(packets == unsigned(c.shadow)+unsigned(c.second>0), "caster population preserved");
        device->resolveTraversalShadow();
        require(storm_render::succeeded(api->GetRenderTargetData(target,readback)), "pixels");
        storm_render::LockedRect locked{};readback->LockRect(&locked,nullptr,storm_render::lock::readOnly);
        std::vector<uint32_t> pixels(4096);
        for(unsigned y=0;y<64;++y) memcpy(pixels.data()+64*y,(char*)locked.pBits+y*locked.Pitch,256);
        readback->UnlockRect();
        std::ofstream output(std::string(argv[1])+"/"+c.name+".bin",std::ios::binary);
        output.write(reinterpret_cast<const char*>(pixels.data()),pixels.size()*4);
        printf("%s submitted=%u casters=%zu\n",c.name,unsigned(submitted),packets);
        api->SetStreamSource(0,nullptr,0,0);api->SetIndices(nullptr);vb->Release();ib->Release();
    }
    readback->Release(); target->Release(); api->Release();
    SDL_DestroyWindow(window); SDL_Quit();
    require(ExistingLightingMain()==0,"existing full native lighting cases");
    return 0;
} }

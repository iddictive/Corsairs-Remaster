#include <SDL.h>
#include "render_device_api.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gpu_skinning_bridge.hpp"

extern "C" bool StormMetalSetShipAuxiliaryDetail(void*,bool);
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" unsigned StormTestCasterDraws(void*);
extern "C" unsigned StormTestPackets(void*);
extern "C" unsigned StormTestRegistry(void*);
extern "C" bool StormMetalBeginShadowFrame(void*,uint64_t,uint64_t,const float*);
extern "C" bool StormMetalBeginShadowPass(void*,unsigned,unsigned,uint64_t,const float*,float,float*,float*);
extern "C" void StormMetalEndShadowPass(void*);
extern "C" bool StormMetalEndShadowFrame(void*);
extern "C" bool StormMetalRenderPointShadowCube(void*,uint64_t,const float*,float);
extern "C" bool StormMetalBeginOffscreenPointCaster(void*,const float*,float);
extern "C" void StormMetalEndOffscreenPointCaster(void*);
extern "C" bool StormMetalSceneLight(void*, uint64_t, const storm_render::Light*);
extern "C" unsigned StormMetalBeginSceneModel(void*);
extern "C" void StormMetalEndLandModel(void*, unsigned);
extern "C" void StormMetalSetLightIdentity(void*, unsigned, uint64_t);
extern "C" uint64_t StormMetalRawWorldDraws(void*);
extern "C" uint64_t StormMetalRawSkinnedDraws(void*);
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*);
extern "C" uint64_t StormMetalPointShadowIndirectFaceSubmissions(void*);
extern "C" bool StormMetalLandShadowResolved(void*);

static void need(bool ok, const char* label) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); } }
struct Vertex { float x,y,z,nx,ny,nz; uint32_t color; float u,v; };
struct FramePixels { uint32_t red{},blue{}; unsigned packets{}, registry{}; };
struct SunFrame { std::array<uint32_t,64*64> pixels{}; bool resolved=false; };

int main() {
    setenv("STORM_METAL_DYNAMIC_LIGHTING", "1", 1);
    setenv("STORM_METAL_MODERN_LIGHTING", "1", 1);
    need(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL");
    auto* window=SDL_CreateWindow("Sea point shadows",0,0,64,64,SDL_WINDOW_HIDDEN|SDL_WINDOW_METAL); need(window,"window");
     storm_render::Presentation pp{}; pp.width=pp.height=64;pp.colorFormat=storm_render::AssetFormat::A8R8G8B8;
    storm_render::Device* d=nullptr; need(storm_render::succeeded(storm_render::createMetalDevice(window, &pp, &d)),"device");
    storm_render::Surface *target=nullptr,*read=nullptr; d->GetRenderTarget(&target); need(storm_render::succeeded(d->CreateOffscreenPlainSurface(64, 64, storm_render::AssetFormat::A8R8G8B8, &read)),"readback");
    const uint16_t ix[]={0,1,2,0,2,3}; storm_render::VertexBuffer* meshVb=nullptr;storm_render::IndexBuffer* ib=nullptr;storm_render::Texture* alphaTexture=nullptr;
    need(storm_render::succeeded(d->CreateVertexBuffer(sizeof(Vertex)*4, storm_render::usage::writeOnly, 0, &meshVb)),"shared mesh VB");need(storm_render::succeeded(d->CreateIndexBuffer(sizeof(ix), storm_render::usage::writeOnly, storm_render::IndexFormat::UInt16, &ib)),"IB");need(storm_render::succeeded(d->CreateTexture(1, 1, 1, 0, storm_render::AssetFormat::A8R8G8B8, &alphaTexture)),"alpha texture");void* p=nullptr;ib->Lock(0,0,&p,0);std::memcpy(p,ix,sizeof(ix));ib->Unlock();Vertex q[]={{-1,-1,0,0,0,-1,0xffffffff,0,0},{1,-1,0,0,0,-1,0xffffffff,1,0},{1,1,0,0,0,-1,0xffffffff,1,1},{-1,1,0,0,0,-1,0xffffffff,0,1}};meshVb->Lock(0,0,&p,0);std::memcpy(p,q,sizeof(q));meshVb->Unlock();storm_render::LockedRect alphaLock{};alphaTexture->LockRect(0,&alphaLock,nullptr,0);*static_cast<uint32_t*>(alphaLock.pBits)=0xffffffff;alphaTexture->UnlockRect(0);
    storm_render::Matrix identity{};identity._11=identity._22=identity._33=identity._44=1;d->SetTransform(storm_render::TransformWorld,&identity);d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&identity);d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetIndices(ib);d->SetRenderState(storm_render::RenderCullMode,storm_render::CullNone);d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderVertexColor,false);d->SetRenderState(storm_render::RenderDiffuseMaterialSource,storm_render::MaterialSourceMaterial);d->SetRenderState(storm_render::RenderAmbientMaterialSource,storm_render::MaterialSourceMaterial);storm_render::Material material{};material.Diffuse={1,1,1,1};d->SetMaterial(&material);
    bool casterDetail=true,receiverDetail=true;
    auto frame=[&](bool blocker,bool red,bool blue,float translation=0.f,unsigned catalogMode=0){
        d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();
        storm_render::Matrix view=identity;view._41=-translation;d->SetTransform(storm_render::TransformView,&view);
        storm_render::Light redLamp{};redLamp.Type=storm_render::LightKind::Point;redLamp.Position={translation-.6f,0,-.4f};redLamp.Diffuse={1,0,0,1};redLamp.Range=3;redLamp.Attenuation0=1;d->SetLight(1,&redLamp);d->LightEnable(1,red);StormMetalSetLightIdentity(d,1,71);
        storm_render::Light blueLamp=redLamp;blueLamp.Position={translation+.6f,0,-.4f};blueLamp.Diffuse={0,0,1,1};d->SetLight(2,&blueLamp);d->LightEnable(2,blue);StormMetalSetLightIdentity(d,2,72);d->LightEnable(0,false);
        if(catalogMode){if(red)need(StormMetalSceneLight(d,71,&redLamp),"sea source published before shadow frame");if(blue)need(StormMetalSceneLight(d,72,&blueLamp),"second sea source");if(catalogMode==1){d->LightEnable(1,false);d->LightEnable(2,false);}}
        auto draw=[&](float x,float half,float z){storm_render::Matrix world=identity;world._11=world._22=half;world._41=x+translation;world._43=z;d->SetTransform(storm_render::TransformWorld,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"raw sea geometry");};
        unsigned scope=StormMetalBeginSceneModel(d);auto previous=StormMetalSetShipAuxiliaryDetail(d,casterDetail);if(blocker&&red)draw(-.1f,.14f,.1f);if(blocker&&blue)draw(.05f,.14f,.1f);StormMetalSetShipAuxiliaryDetail(d,receiverDetail);draw(0,.9f,.6f);StormMetalSetShipAuxiliaryDetail(d,previous);StormMetalEndLandModel(d,scope);storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);d->EndScene();
        need(storm_render::succeeded(d->GetRenderTargetData(target,read)),"readback");storm_render::LockedRect lock{};read->LockRect(&lock,nullptr,storm_render::lock::readOnly);FramePixels value{};value.packets=StormTestPackets(d);value.registry=StormTestRegistry(d);std::memcpy(&value.red,static_cast<char*>(lock.pBits)+32*lock.Pitch+48*4,4);std::memcpy(&value.blue,static_cast<char*>(lock.pBits)+32*lock.Pitch+16*4,4);read->UnlockRect();need(storm_render::succeeded(d->Present()),"frame reset");return value;
    };
    const auto before=StormMetalRawWorldDraws(d);const auto redLit=frame(false,true,false),redShadow=frame(true,true,false),blueLit=frame(false,false,true),blueShadow=frame(true,false,true),twoShadow=frame(true,true,true),off=frame(true,false,false);
    const auto catalogOnly=frame(false,true,false,0,1),catalogWithSlots=frame(false,true,false,0,2);
    need(((catalogOnly.red>>16)&255)>16,"catalog lights the hull without receiver-owned D3D slots");
    need(std::abs(int((catalogOnly.red>>16)&255)-int((catalogWithSlots.red>>16)&255))<=2,"catalog plus D3D slot counts a physical source once");
    const auto expiredCatalog=frame(false,false,false);
    need(((expiredCatalog.red>>16)&255)<8,"off source does not leak from the previous catalog frame");
    // Translating the camera and its illuminated geometry together must preserve
    // lamp occlusion, including beyond the former 30-unit world-origin cutoff.
    const auto translatedLit=frame(false,true,false,40),translatedShadow=frame(true,true,false,40);
    need(int((translatedLit.red>>16)&255)>int((translatedShadow.red>>16)&255)+8,"translated visible receiver retains its lamp shadow");
    need(std::abs(int((redShadow.red>>16)&255)-int((translatedShadow.red>>16)&255))<=3,"point shadows are covariant under world/view translation");
    d->SetTransform(storm_render::TransformView,&identity);
    // ShipLights clears the live D3D slot after the deck receiver. This imitates
    // the later DECK_CAMERA/Sailors draw: a valid earlier packet must admit its
    // GPU-skinned caster, while an empty frame must not create a point shadow.
    auto lateSkinnedFrame=[&](bool precedingSnapshot,bool lateBlocker){
        d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();
        storm_render::Light lamp{};lamp.Type=storm_render::LightKind::Point;lamp.Position={-.6f,0,-.4f};lamp.Diffuse={1,0,0,1};lamp.Range=3;lamp.Attenuation0=1;
        d->SetLight(1,&lamp);d->LightEnable(1,precedingSnapshot);StormMetalSetLightIdentity(d,1,precedingSnapshot?81:0);d->LightEnable(0,false);
        auto drawDeck=[&](){storm_render::Matrix world=identity;world._11=world._22=.9f;world._43=.6f;d->SetTransform(storm_render::TransformWorld,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"deck receiver");};
        unsigned scope=StormMetalBeginSceneModel(d);
        if(precedingSnapshot)drawDeck();
        d->LightEnable(1,false);StormMetalSetLightIdentity(d,1,0);
        if(lateBlocker){
            using namespace storm::metal::skinning;
            // `landSkin` negates the palette X axis. Mirror input so this
            // skinned blocker occupies the same x=[-.24,.04] footprint as the
            // static positive-control blocker after skinning.
            const AnimatedVertex actor[]={
                {{ .24f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,0}},
                {{-.04f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,0}},
                {{-.04f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,1}},
                {{ .24f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,1}},
            };
            storm_render::Matrix actorWorld=identity;d->SetTransform(storm_render::TransformWorld,&actorWorld);
            Matrix4x4 palette[1]{};palette[0].elements[0]=palette[0].elements[5]=palette[0].elements[10]=palette[0].elements[15]=1;
            need(acceptVertices(actor,4,1),"late actor accepted");need(beginPose(d,palette,1),"late actor pose");
            auto* actorVb=static_cast<storm_render::VertexBuffer*>(bindVertices(actor,0,4,4));need(actorVb,"late actor vertices");
            need(storm_render::succeeded(d->SetStreamSource(0,actorVb,0,36)),"late actor stream");need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"late actor draw");endPose();
            invalidateVertices(actor);
        }
        StormMetalEndLandModel(d,scope);storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);d->EndScene();
        need(storm_render::succeeded(d->GetRenderTargetData(target,read)),"late readback");storm_render::LockedRect lock{};read->LockRect(&lock,nullptr,storm_render::lock::readOnly);FramePixels value{};value.packets=StormTestPackets(d);value.registry=StormTestRegistry(d);std::memcpy(&value.red,static_cast<char*>(lock.pBits)+32*lock.Pitch+48*4,4);read->UnlockRect();need(storm_render::succeeded(d->Present()),"late frame reset");return value;
    };
    const auto rawSkinnedBefore=StormMetalRawSkinnedDraws(d);const auto lateLit=lateSkinnedFrame(true,false),lateShadow=lateSkinnedFrame(true,true);const auto noSourceCulls=StormMetalPointShadowGpuCullDispatches(d);const auto noSource=lateSkinnedFrame(false,false),noSourceLateActor=lateSkinnedFrame(false,true);
    // The live ship/deck receiver is submitted while Weather owns slot 3.
    // ShipLights then changes slot 0 before DECK_CAMERA draws the
    // animated actor. Both caster admission and resolve must use the immutable
    // directional snapshot carried by the earlier receiver packet.
    auto lateSunFrame=[&](bool precedingSnapshot,bool earlyCutout,bool lateBlocker){
        d->Clear(storm_render::clear::color|storm_render::clear::depth, 0xff000000, 1);d->BeginScene();
        storm_render::Light sun{};sun.Type=storm_render::LightKind::Directional;sun.Direction={1,0,1};sun.Diffuse={1,1,1,1};
        d->SetLight(3,&sun);d->LightEnable(3,false);
        auto drawDeck=[&](){storm_render::Matrix world=identity;world._11=world._22=.9f;world._43=.6f;d->SetTransform(storm_render::TransformWorld,&world);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"sun deck receiver");};
        unsigned shipScope=StormMetalBeginSceneModel(d);
        if(earlyCutout){
            d->SetTexture(0,alphaTexture);d->SetRenderState(storm_render::RenderAlphaTest,true);d->SetRenderState(storm_render::RenderAlphaCompare,storm_render::CompareGreater);d->SetRenderState(storm_render::RenderAlphaReference,24);d->SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);d->SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::texture);
            storm_render::Matrix mast=identity;mast._11=mast._22=.14f;mast._41=.48f;mast._43=.1f;d->SetTransform(storm_render::TransformWorld,&mast);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"pre-source alpha-cutout mast");
            d->SetRenderState(storm_render::RenderAlphaTest,false);d->SetTexture(0,nullptr);
        }
        d->LightEnable(3,precedingSnapshot);if(precedingSnapshot)drawDeck();d->LightEnable(3,false);
        StormMetalEndLandModel(d,shipScope);
        storm_render::Light replacement=sun;replacement.Direction={0,1,0};replacement.Diffuse={0,1,0,1};d->LightEnable(0,false);d->SetLight(0,&replacement);
        if(lateBlocker){
            const bool previous=StormMetalSetShipAuxiliaryDetail(d,casterDetail);
            unsigned scope=StormMetalBeginSceneModel(d);
            using namespace storm::metal::skinning;
            const AnimatedVertex actor[]={
                {{ .24f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,0}},
                {{-.04f,-.18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,0}},
                {{-.04f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{1,1}},
                {{ .24f, .18f,.1f},1,0,{0,0,-1},int32_t(0xffffffff),{0,1}},
            };
            storm_render::Matrix actorWorld=identity;d->SetTransform(storm_render::TransformWorld,&actorWorld);
            Matrix4x4 palette[1]{};palette[0].elements[0]=palette[0].elements[5]=palette[0].elements[10]=palette[0].elements[15]=1;
            need(acceptVertices(actor,4,1),"late sun actor accepted");need(beginPose(d,palette,1),"late sun actor pose");
            auto* actorVb=static_cast<storm_render::VertexBuffer*>(bindVertices(actor,0,4,4));need(actorVb,"late sun actor vertices");
            need(storm_render::succeeded(d->SetStreamSource(0,actorVb,0,36)),"late sun actor stream");need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"late sun actor draw");endPose();
            invalidateVertices(actor);
            StormMetalEndLandModel(d,scope);
            StormMetalSetShipAuxiliaryDetail(d,previous);
        }
        storm_render::markSceneBoundary(storm_render::SceneBoundary::Hud);d->EndScene();
        SunFrame value{};value.resolved=StormMetalLandShadowResolved(d);need(storm_render::succeeded(d->GetRenderTargetData(target,read)),"sun readback");storm_render::LockedRect lock{};read->LockRect(&lock,nullptr,storm_render::lock::readOnly);for(int y=0;y<64;y++)std::memcpy(value.pixels.data()+y*64,static_cast<char*>(lock.pBits)+y*lock.Pitch,64*4);read->UnlockRect();need(storm_render::succeeded(d->Present()),"sun frame reset");return value;
    };
    const auto sunLit=lateSunFrame(true,false,false),sunMastShadow=lateSunFrame(true,true,false),sunShadow=lateSunFrame(true,true,true),noSun=lateSunFrame(false,false,false),noSunActors=lateSunFrame(false,true,true);
    auto channel=[](uint32_t c,unsigned shift){return int((c>>shift)&255);};
    unsigned mastDarkened=0,actorDarkened=0,noSunChanged=0;for(int y=16;y<48;y++)for(int x=0;x<64;x++){const auto index=size_t(y*64+x);mastDarkened+=channel(sunLit.pixels[index],16)>channel(sunMastShadow.pixels[index],16)+8;actorDarkened+=channel(sunMastShadow.pixels[index],16)>channel(sunShadow.pixels[index],16)+8;noSunChanged+=noSun.pixels[index]!=noSunActors.pixels[index];}
    std::fprintf(stderr,"[sea-point] red=%d/%d blue=%d/%d other-blue=%d/%d two=(red=%d blue=%d) off=(%d,%d) late-red=%d/%d no-source=%d/%d mast-darkened=%u actor-darkened=%u no-sun-changed=%u raw=%llu cull=%llu faces=%llu\n",channel(redLit.red,16),channel(redShadow.red,16),channel(blueLit.blue,0),channel(blueShadow.blue,0),channel(blueLit.red,0),channel(twoShadow.red,0),channel(twoShadow.red,16),channel(twoShadow.blue,0),channel(off.red,16),channel(off.blue,0),channel(lateLit.red,16),channel(lateShadow.red,16),channel(noSource.red,16),channel(noSourceLateActor.red,16),mastDarkened,actorDarkened,noSunChanged,(unsigned long long)StormMetalRawWorldDraws(d),(unsigned long long)StormMetalPointShadowGpuCullDispatches(d),(unsigned long long)StormMetalPointShadowIndirectFaceSubmissions(d));
    need(StormMetalRawWorldDraws(d)>before,"scene-model scope captures raw sea geometry");need(channel(redLit.red,16)>channel(redShadow.red,16)+8,"same-frame point cube removes the red lamp direct term behind its caster");need(channel(blueLit.blue,0)>channel(blueShadow.blue,0)+8,"blue lamp has an independent cube shadow");need(channel(twoShadow.red,0)>=channel(blueLit.red,0)-3,"red correction preserves the nonselected blue direct term");need(channel(twoShadow.blue,0)>=channel(blueShadow.blue,0)-3,"combined lamps retain the blue shadow result");need(channel(off.red,16)<8&&channel(off.blue,0)<8,"disabled lamps create neither light nor correction");need(StormMetalRawSkinnedDraws(d)>rawSkinnedBefore,"late GPU-skinned actor uses the raw Metal path");need(channel(lateLit.red,16)>channel(lateShadow.red,16)+8,"preceding lamp snapshot admits a late skinned actor as a deck caster");need(StormMetalPointShadowGpuCullDispatches(d)==noSourceCulls,"no preceding point snapshot does not dispatch a point-shadow cube");need(noSource.red==noSourceLateActor.red,"no preceding point snapshot keeps a late skinned actor out of the registry");need(sunLit.resolved&&sunMastShadow.resolved&&sunShadow.resolved,"frozen directional snapshot resolves after slot 0 is replaced");need(mastDarkened>8,"pre-source SELECTARG1 ship cutout casts a directional shadow");need(actorDarkened>8,"late GPU-skinned sailor casts an additional directional shadow");need(!noSun.resolved&&!noSunActors.resolved,"next frame without a directional source does not resolve stale sun");need(noSunChanged==0,"next frame without a directional source does not apply stale sun");

    const auto validBefore=frame(true,true,false);
    casterDetail=false;const auto culled=frame(true,true,false);
    need(culled.red==redLit.red,"LOD false removes same-visible caster shadow immediately after a valid frame");
    need(culled.packets==validBefore.packets&&culled.registry+1==validBefore.registry,"receiver packets retained while prior-valid caster retires");
    casterDetail=true;receiverDetail=false;const auto receiving=frame(true,true,false);
    need(receiving.red==validBefore.red,"LOD false receiver still receives independent caster shadow");
    receiverDetail=true;const auto restored=frame(true,true,false);
    need(restored.red==validBefore.red&&restored.registry==validBefore.registry,"zoom/near scope restores prior-valid caster");
    casterDetail=false;const auto sunCulled=lateSunFrame(true,true,true);casterDetail=true;
    unsigned restoredSun=0;for(unsigned i=0;i<4096;i++)if(channel(sunMastShadow.pixels[i],16)>channel(sunShadow.pixels[i],16)+8)restoredSun+=std::abs(channel(sunCulled.pixels[i],16)-channel(sunMastShadow.pixels[i],16))<=3;
    need(restoredSun>8,"LOD false suppresses skinned sun caster while keeping its ordinary model draw");
    need(StormMetalSetShipAuxiliaryDetail(nullptr,false),"null device defaults to detail");
    int foreign=0;need(StormMetalSetShipAuxiliaryDetail(&foreign,false),"foreign device defaults to detail");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"detail starts enabled");
    need(!StormMetalSetShipAuxiliaryDetail(d,true),"nested scope returns suppressed state");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"restore outer scope returns nested state");
    need(!StormMetalSetShipAuxiliaryDetail(d,false),"outer scope restored");
    need(storm_render::succeeded(d->Present()),"present while suppressed");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"Present clears leaked LOD state");
    StormMetalLandLocation(d,true);need(StormMetalSetShipAuxiliaryDetail(d,false),"location transition clears leaked LOD state");
    StormMetalLandLocation(d,false);need(StormMetalSetShipAuxiliaryDetail(d,true),"sea transition clears leaked LOD state");
    target->Release();target=nullptr;
    StormMetalSetShipAuxiliaryDetail(d,false);need(storm_render::succeeded(d->Reset(&pp)),"valid device Reset");
    need(StormMetalSetShipAuxiliaryDetail(d,true),"Reset clears leaked LOD state");d->GetRenderTarget(&target);
    std::fprintf(stderr,"[aux-lod] valid=%08x culled=%08x receiver=%08x restored=%08x packets=%u/%u registry=%u/%u sun-restored=%u\n",validBefore.red,culled.red,receiving.red,restored.red,validBefore.packets,culled.packets,validBefore.registry,culled.registry,restoredSun);
    d->Clear(storm_render::clear::color|storm_render::clear::depth,0xff000000,1);d->BeginScene();StormMetalLandLocation(d,true);
    const float focus[]={0,0,0},ray[]={0,0,1};float shadowView[16],shadowProjection[16];
    need(StormMetalBeginShadowFrame(d,99001,1,focus),"depth test frame");
    need(StormMetalBeginShadowPass(d,0,0,9,ray,96,shadowView,shadowProjection),"depth test pass");
    d->SetTransform(storm_render::TransformWorld,&identity);d->SetTransform(storm_render::TransformView,reinterpret_cast<storm_render::Matrix*>(shadowView));d->SetTransform(storm_render::TransformProjection,reinterpret_cast<storm_render::Matrix*>(shadowProjection));
    d->SetRenderState(storm_render::RenderLighting,true);d->SetRenderState(storm_render::RenderDepthWrite,true);d->SetRenderState(storm_render::RenderBlend,false);d->SetFVF(storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1);d->SetStreamSource(0,meshVb,0,sizeof(Vertex));d->SetIndices(ib);
    auto depthScope=StormMetalBeginSceneModel(d);StormMetalSetShipAuxiliaryDetail(d,false);
    need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"suppressed direct depth returns success");need(StormTestCasterDraws(d)==0,"suppressed direct depth submits no caster");
    StormMetalSetShipAuxiliaryDetail(d,true);need(storm_render::succeeded(d->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList,0,0,4,0,2)),"restored direct depth draw");need(StormTestCasterDraws(d)==1,"restored direct depth submits caster");StormMetalEndLandModel(d,depthScope);StormMetalEndShadowPass(d);StormMetalEndShadowFrame(d);d->EndScene();d->Present();
    d->Clear(storm_render::clear::color|storm_render::clear::depth,0xff000000,1);d->BeginScene();d->SetTransform(storm_render::TransformWorld,&identity);d->SetTransform(storm_render::TransformView,&identity);d->SetTransform(storm_render::TransformProjection,&identity);
    need(StormMetalBeginShadowFrame(d,99002,1,focus),"offscreen test frame");storm_render::Light nearby{};nearby.Type=storm_render::LightKind::Point;nearby.Diffuse={1,1,1,1};nearby.Range=10;nearby.Attenuation0=1;need(StormMetalSceneLight(d,71,&nearby),"offscreen source");StormMetalRenderPointShadowCube(d,71,focus,10);StormMetalEndShadowFrame(d);
    StormMetalSetShipAuxiliaryDetail(d,false);need(!StormMetalBeginOffscreenPointCaster(d,focus,1),"false LOD rejects otherwise-valid offscreen caster");
    StormMetalSetShipAuxiliaryDetail(d,true);need(StormMetalBeginOffscreenPointCaster(d,focus,1),"restored LOD admits same offscreen caster");StormMetalEndOffscreenPointCaster(d);StormMetalLandLocation(d,false);d->EndScene();d->Present();
    std::puts("PASS direct sun depth0/1 and offscreen admission false/true");
    alphaTexture->Release();ib->Release();meshVb->Release();read->Release();target->Release();d->Release();SDL_DestroyWindow(window);SDL_Quit();std::puts("PASS sea point and mixed-order deck shadows preserve source ownership");
}

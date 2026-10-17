#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "ship_fire_bridge.hpp"
#include "ship_fire_shaders.hpp"
#include "frame_arena.hpp"

namespace storm_metal {
inline constexpr uint32_t shipFirePointLimit=48;
struct ShipFireSurfaceUniform {
    simd_float4x4 fromModel{};
    simd_float4x4 inverseWorld{};
    simd_uint4 counts{};
    // Surface-only upload reuses seed for prepared light flutter. Scope and
    // volume records retain their original variation seed and 32-byte ABI.
    StormMetalShipFirePoint points[shipFirePointLimit]{};
};
static_assert(sizeof(ShipFireSurfaceUniform)==1680);
struct ShipFireSurfaceScope {
    simd_float4x4 inverseWorld{};
    std::array<StormMetalShipFirePoint,shipFirePointLimit> points{};
    uint32_t count=0;
    void clear(){count=0;}
    static bool finiteMatrix(const float*world){if(!world)return false;for(unsigned i=0;i<16;i++)if(!std::isfinite(world[i]))return false;return true;}
    static bool validPoint(const StormMetalShipFirePoint&p){
        for(float value:p.position)if(!std::isfinite(value))return false;
        return std::isfinite(p.radius)&&p.radius>.05f&&p.radius<=100.f&&
            std::isfinite(p.intensity)&&std::isfinite(p.charred)&&std::isfinite(p.heat)&&std::isfinite(p.seed);
    }
    static StormMetalShipFirePoint normalized(StormMetalShipFirePoint p){
        p.intensity=std::clamp(p.intensity,0.f,1.f);p.charred=std::clamp(p.charred,0.f,1.f);p.heat=std::clamp(p.heat,0.f,1.f);return p;
    }
    void begin(const float*world,const StormMetalShipFirePoint*source,uint32_t size){
        clear();if(!finiteMatrix(world)||!source||size>shipFirePointLimit)return;
        simd_float4x4 matrix;std::memcpy(&matrix,world,sizeof(matrix));
        const float determinant=simd_determinant(matrix);if(!std::isfinite(determinant)||std::abs(determinant)<1.e-6f)return;
        inverseWorld=simd_inverse(matrix);
        for(uint32_t i=0;i<size;i++)if(validPoint(source[i])&&(source[i].charred>0.f||source[i].intensity>0.f||source[i].heat>0.f))points[count++]=normalized(source[i]);
    }
    void fill(ShipFireSurfaceUniform&uniform,const simd_float4x4&model,float seconds)const{
        if(!count)return;uniform.fromModel=simd_mul(inverseWorld,model);uniform.inverseWorld=inverseWorld;uniform.counts.x=count;
        std::copy_n(points.begin(),count,uniform.points);
        for(uint32_t i=0;i<count;i++){
            const float seed=points[i].seed;
            uniform.points[i].seed=points[i].intensity>.001f?
                .86f+.10f*std::sin(seconds*7.3f+seed)+.04f*std::sin(seconds*15.1f-seed*.37f):0.f;
        }
    }
};
struct ShipFireVolume {
    simd_float4 sourceRadius,burn,windTime,boundsMin,boundsMax,clipRect,lod;
};
static_assert(sizeof(ShipFireVolume)==112);
struct ShipFireCamera {
    simd_float4x4 inverseViewProjection,viewProjection;
    simd_float4 eye,viewport,fogColor,fogParams,lighting;
};
static_assert(sizeof(ShipFireCamera)==208);

struct ShipFireVisualBudget {
    // Keep useful integration samples and reduce backing pixels across the
    // entire fleet. Cutting complex volumes to eight samples caused stipple.
    // requested/admittedWork remain estimates, never scene FPS evidence.
    static constexpr double targetSampleWork=8.0e6;
    double requestedWork=0,admittedWork=0;
    float renderScale=1.f;
    static float sourceSamples(float){return 32.f;}
    void apply(std::vector<ShipFireVolume>&volumes,simd_float4 viewport){
        auto area=[&](const ShipFireVolume&v){return double(v.clipRect.z-v.clipRect.x)*double(v.clipRect.w-v.clipRect.y)*double(viewport.z)*double(viewport.w)*.25;};
        requestedWork=0;for(const auto&v:volumes)requestedWork+=area(v)*v.lod.x;
        renderScale=1.f;
        // Spend the existing budget continuously: power-of-two drops threw
        // away up to three quarters of it and enlarged the fire into blocks.
        if(requestedWork>targetSampleWork)
            renderScale=float(std::clamp(std::sqrt(targetSampleWork/requestedWork),.125,1.0));
        admittedWork=requestedWork*renderScale*renderScale;
    }
};

// One queue per frame, consumed after opaque ship/rigging/sea traversal and
// before HUD. There is no retained emitter/simulation registry to survive saves.
struct ShipFireRenderer {
    id<MTLLibrary> library=nil;
    id<MTLRenderPipelineState> volumePipeline=nil,emberPipeline=nil,compositePipeline=nil;
    id<MTLTexture> volumeColor=nil,volumeFront=nil;
    MTLPixelFormat format=MTLPixelFormatInvalid;
    bool failed=false,cameraValid=false;
    ShipFireCamera camera{};
    ShipFireVisualBudget budget{};
    std::vector<ShipFireVolume> volumes;
    void clear(){volumes.clear();cameraValid=false;}
    void reset(){clear();library=nil;volumePipeline=emberPipeline=compositePipeline=nil;volumeColor=volumeFront=nil;format=MTLPixelFormatInvalid;failed=false;}
    bool empty()const{return volumes.empty();}
    bool prepare(id<MTLDevice>device,MTLPixelFormat targetFormat){
        if(failed)return false;if(volumePipeline&&emberPipeline&&compositePipeline&&format==targetFormat)return true;
        NSError*error=nil;
        if(!library){std::string source=std::string(shipFireCommonMSL)+shipFireVolumeMSL;
            library=[device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];}
        if(!library){std::fprintf(stderr,"[StormMetal] ship fire shader: %s\n",error.localizedDescription.UTF8String);failed=true;return false;}
        auto make=[&](NSString*vertex,NSString*fragment,bool volume){
            auto descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=[library newFunctionWithName:vertex];descriptor.fragmentFunction=[library newFunctionWithName:fragment];
            if(!descriptor.vertexFunction||!descriptor.fragmentFunction){std::fprintf(stderr,"[StormMetal] ship fire entrypoint missing: %s / %s\n",vertex.UTF8String,fragment.UTF8String);return (id<MTLRenderPipelineState>)nil;}
            descriptor.colorAttachments[0].pixelFormat=volume?MTLPixelFormatRGBA16Float:targetFormat;descriptor.colorAttachments[0].blendingEnabled=YES;
            descriptor.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorOne;descriptor.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
            descriptor.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorOne;descriptor.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
            if(volume){descriptor.colorAttachments[1].pixelFormat=MTLPixelFormatR32Float;descriptor.colorAttachments[1].blendingEnabled=YES;descriptor.colorAttachments[1].rgbBlendOperation=MTLBlendOperationMin;descriptor.colorAttachments[1].sourceRGBBlendFactor=descriptor.colorAttachments[1].destinationRGBBlendFactor=MTLBlendFactorOne;}
            return [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        };
        volumePipeline=make(@"ship_fire_volume_vs",@"ship_fire_volume_fs",true);emberPipeline=make(@"ship_fire_ember_vs",@"ship_fire_ember_fs",false);compositePipeline=make(@"ship_fire_composite_vs",@"ship_fire_composite_fs",false);format=targetFormat;
        if(!volumePipeline||!emberPipeline||!compositePipeline){std::fprintf(stderr,"[StormMetal] ship fire pipeline: %s\n",error.localizedDescription.UTF8String);failed=true;return false;}return true;
    }
    bool append(const float*world,const StormMetalShipFirePoint*points,uint32_t count,
        const float*wind,float seconds,simd_float3 origin,const simd_float4x4&view,const simd_float4x4&projection,
        simd_float4 viewport,simd_float4 fogColor,simd_float4 fogParams,float daylight){
        if(!ShipFireSurfaceScope::finiteMatrix(world)||!points||count>shipFirePointLimit||!wind||!std::isfinite(seconds))return false;
        for(unsigned i=0;i<3;i++)if(!std::isfinite(wind[i]))return false;
        if(viewport.z<=0.f||viewport.w<=0.f)return false;
        const auto viewProjection=simd_mul(projection,view);
        if(!cameraValid){camera={simd_inverse(viewProjection),viewProjection,simd_inverse(view).columns[3],viewport,fogColor,fogParams,{std::clamp(daylight,0.f,1.f),0,0,0}};cameraValid=true;}
        else if(std::memcmp(&camera.viewProjection,&viewProjection,sizeof(viewProjection))||std::memcmp(&camera.viewport,&viewport,sizeof(viewport)))return false;
        simd_float4x4 root;std::memcpy(&root,world,sizeof(root));
        const float scale=std::max({simd_length(root.columns[0].xyz),simd_length(root.columns[1].xyz),simd_length(root.columns[2].xyz)});
        if(!std::isfinite(scale)||scale<.001f)return false;
        simd_float3 airflow={wind[0],wind[1],wind[2]};float windSpeed=simd_length(airflow);if(windSpeed>25.f)airflow*=25.f/windSpeed;
        for(uint32_t i=0;i<count;i++){
            if(!ShipFireSurfaceScope::validPoint(points[i]))continue;auto point=ShipFireSurfaceScope::normalized(points[i]);
            if(point.intensity<.001f&&point.heat<.01f)continue;
            // 48 zones per hull are bounded by simulation; render admission only
            // culls geometry outside this view, never changes the fire state.
            ShipFireVolume volume{};simd_float3 source=simd_mul(root,simd_make_float4(point.position[0],point.position[1],point.position[2],1.f)).xyz+origin;
            const float radius=point.radius*scale,height=radius*(7.f+9.f*std::max(point.intensity,point.heat*.4f));
            const simd_float3 extent={radius*5.2f,height,radius*5.2f};
            const float y=height/radius;const simd_float3 drift={airflow.x*(y*y/(4.f+y))*.11f*radius,0,airflow.z*(y*y/(4.f+y))*.11f*radius};
            const simd_float3 low=source+simd_min(simd_make_float3(0),drift)-simd_make_float3(extent.x,radius*.16f,extent.z),high=source+simd_max(simd_make_float3(0),drift)+extent;
            simd_float2 lo={1.f,1.f},hi={-1.f,-1.f};bool crossedNear=false;unsigned visible=0;
            for(unsigned corner=0;corner<8;corner++){
                simd_float3 p={corner&1?high.x:low.x,corner&2?high.y:low.y,corner&4?high.z:low.z};auto clip=simd_mul(viewProjection,simd_make_float4(p,1.f));
                if(clip.w<=.001f||clip.z<0.f){crossedNear=true;continue;}simd_float2 xy=clip.xy/clip.w;lo=simd_min(lo,xy);hi=simd_max(hi,xy);++visible;
            }
            const bool eyeInside=simd_all(camera.eye.xyz>=low)&&simd_all(camera.eye.xyz<=high);
            if(!visible&&!eyeInside)continue;if(crossedNear){lo={-1,-1};hi={1,1};}
            lo=simd_max(lo,simd_make_float2(-1.f,-1.f));hi=simd_min(hi,simd_make_float2(1.f,1.f));if(hi.x<=lo.x||hi.y<=lo.y)continue;
            const auto sourceView=simd_mul(view,simd_make_float4(source,1.f));
            const float footprint=radius*std::abs(projection.columns[1].y)*viewport.w/std::max(std::abs(sourceView.z),radius);
            // Keep distant fire readable while reducing only sampling/detail.
            if(footprint<.7f)continue;
            const float samples=ShipFireVisualBudget::sourceSamples(footprint);
            volume.sourceRadius=simd_make_float4(source,radius);volume.burn={point.intensity,point.charred,point.heat,point.seed};volume.windTime=simd_make_float4(airflow,seconds);
            volume.boundsMin=simd_make_float4(low,0.f);volume.boundsMax=simd_make_float4(high,0.f);volume.clipRect={lo.x,lo.y,hi.x,hi.y};volume.lod={samples,simd_length(source-camera.eye.xyz),footprint,0};volumes.push_back(volume);
        }return true;
    }
    bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>target,id<MTLTexture>depth,FrameArena&arena){
        if(empty())return true;
        if(!cameraValid||!target||!depth||depth.pixelFormat!=MTLPixelFormatDepth32Float||target.width!=depth.width||target.height!=depth.height||!prepare(device,target.pixelFormat)){clear();return false;}
        budget.apply(volumes,camera.viewport);
        camera.lighting.y=budget.renderScale;
        const NSUInteger renderWidth=std::max(NSUInteger(1),NSUInteger(std::ceil(target.width*budget.renderScale))),renderHeight=std::max(NSUInteger(1),NSUInteger(std::ceil(target.height*budget.renderScale)));
        // Keep capacity across camera/fire changes. Logical backing still uses
        // renderScale; only growth or a smaller scene target reallocates.
        if(!volumeColor||volumeColor.width<renderWidth||volumeColor.height<renderHeight||volumeColor.width>target.width||volumeColor.height>target.height){
            const NSUInteger capacityWidth=std::min(target.width,std::max(volumeColor?volumeColor.width:0,((renderWidth+63)/64)*64));
            const NSUInteger capacityHeight=std::min(target.height,std::max(volumeColor?volumeColor.height:0,((renderHeight+63)/64)*64));
            auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float width:capacityWidth height:capacityHeight mipmapped:NO];descriptor.storageMode=MTLStorageModePrivate;descriptor.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;volumeColor=[device newTextureWithDescriptor:descriptor];descriptor.pixelFormat=MTLPixelFormatR32Float;volumeFront=[device newTextureWithDescriptor:descriptor];
        }
        if(!volumeColor||!volumeFront){clear();return false;}
        std::stable_sort(volumes.begin(),volumes.end(),[](const auto&a,const auto&b){return a.lod.y>b.lod.y;});
        auto upload=arena.allocate(device,volumes.size()*sizeof(ShipFireVolume));if(!upload.buffer){clear();return false;}std::memcpy(upload.data,volumes.data(),upload.length);
        auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=volumeColor;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
        pass.colorAttachments[1].texture=volumeFront;pass.colorAttachments[1].loadAction=MTLLoadActionClear;pass.colorAttachments[1].storeAction=MTLStoreActionStore;pass.colorAttachments[1].clearColor=MTLClearColorMake(1,1,1,1);
        // Depth is sampled after the preceding encoder has stored it. It is
        // deliberately NOT attached here: no read/write alias and no depth write.
        auto encoder=[command renderCommandEncoderWithDescriptor:pass];if(!encoder){clear();return false;}
        [encoder setLabel:@"Ship fire: coherent FP16 volume and first-contribution depth"];
        [encoder setViewport:MTLViewport{camera.viewport.x*budget.renderScale,camera.viewport.y*budget.renderScale,camera.viewport.z*budget.renderScale,camera.viewport.w*budget.renderScale,0,1}];[encoder setCullMode:MTLCullModeNone];
        [encoder setVertexBuffer:upload.buffer offset:upload.offset atIndex:0];[encoder setFragmentBuffer:upload.buffer offset:upload.offset atIndex:0];
        [encoder setVertexBytes:&camera length:sizeof(camera) atIndex:1];[encoder setFragmentBytes:&camera length:sizeof(camera) atIndex:1];[encoder setFragmentTexture:depth atIndex:0];
        [encoder setRenderPipelineState:volumePipeline];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:volumes.size()];
        [encoder endEncoding];
        pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        encoder=[command renderCommandEncoderWithDescriptor:pass];if(!encoder){clear();return false;}
        [encoder setLabel:@"Ship fire: depth-aware full-size composition and embers"];
        [encoder setViewport:MTLViewport{camera.viewport.x,camera.viewport.y,camera.viewport.z,camera.viewport.w,0,1}];[encoder setCullMode:MTLCullModeNone];
        [encoder setFragmentBytes:&camera length:sizeof(camera) atIndex:1];[encoder setFragmentTexture:volumeColor atIndex:0];[encoder setFragmentTexture:volumeFront atIndex:1];[encoder setFragmentTexture:depth atIndex:2];
        [encoder setRenderPipelineState:compositePipeline];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder setVertexBuffer:upload.buffer offset:upload.offset atIndex:0];[encoder setVertexBytes:&camera length:sizeof(camera) atIndex:1];[encoder setFragmentTexture:depth atIndex:0];
        [encoder setRenderPipelineState:emberPipeline];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:volumes.size()*24];
        [encoder endEncoding];clear();return true;
    }
};
}

#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "ship_fire_shaders.hpp"
#include "phonk_ordnance_shaders.hpp"

namespace storm_metal
{
struct PhonkNuclearUniforms
{
    simd_float4x4 inverseViewProjection, viewProjection;
    simd_float4 eye, sourceRadius, params, viewport;
};
static_assert(sizeof(PhonkNuclearUniforms) == 192);
class PhonkNuclearRenderer
{
    id<MTLLibrary> library = nil;
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLTexture> noise = nil;
    MTLPixelFormat format = MTLPixelFormatInvalid;
    bool prepare(id<MTLDevice> device, MTLPixelFormat requested)
    {
        if (pipeline && requested == format) return true;
        NSError *error = nil;
        if (!noise) {
            // One immutable 64^3 scalar lattice replaces repeated ALU hashes
            // in both the view and light marches; no simulation/history state.
            constexpr unsigned side=64;
            std::vector<uint8_t> values(side*side*side);
            uint32_t seed=0x6d2b79f5u;
            for(auto &value:values) { seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;value=uint8_t(seed>>24); }
            auto descriptor=[MTLTextureDescriptor new];descriptor.textureType=MTLTextureType3D;
            descriptor.pixelFormat=MTLPixelFormatR8Unorm;descriptor.width=descriptor.height=descriptor.depth=side;
            descriptor.storageMode=MTLStorageModeShared;descriptor.usage=MTLTextureUsageShaderRead;
            noise=[device newTextureWithDescriptor:descriptor];if(!noise)return false;
            [noise replaceRegion:MTLRegionMake3D(0,0,0,side,side,side) mipmapLevel:0 slice:0
                withBytes:values.data() bytesPerRow:side bytesPerImage:side*side];
        }
        if (!library)
        {
            const auto source = std::string(shipFireCommonMSL) + phonkNuclearMSL;
            library = [device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];
            if (!library) { fprintf(stderr,"[StormMetal] nuclear shader: %s\n",error.localizedDescription.UTF8String); return false; }
        }
        auto descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.vertexFunction = [library newFunctionWithName:@"phonk_nuclear_vs"];
        descriptor.fragmentFunction = [library newFunctionWithName:@"phonk_nuclear_fs"];
        auto color = descriptor.colorAttachments[0];
        color.pixelFormat = requested;
        color.blendingEnabled = YES;
        color.sourceRGBBlendFactor = color.sourceAlphaBlendFactor = MTLBlendFactorOne;
        color.destinationRGBBlendFactor = color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!pipeline) { fprintf(stderr,"[StormMetal] nuclear pipeline: %s\n",error.localizedDescription.UTF8String); return false; }
        format = requested;
        return true;
    }
  public:
    bool encode(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> target,id<MTLTexture> depth,
                const simd_float4x4 &view,const simd_float4x4 &projection,simd_float3 worldOrigin,
                simd_float4 viewport,const float *center,float radius,float age)
    {
        if (!device || !command || !target || !depth || !center || target.sampleCount != 1 ||
            depth.sampleCount != 1 || target.width != depth.width || target.height != depth.height ||
            !std::isfinite(radius) || radius <= 0.f || !std::isfinite(age) || age < 0.f || age >= 9.f) return false;
        for (unsigned n=0;n<3;++n) if (!std::isfinite(center[n])) return false;
        PhonkNuclearUniforms uniform{};
        uniform.viewProjection = simd_mul(projection,view);
        const float determinant = simd_determinant(uniform.viewProjection);
        if (!std::isfinite(determinant) || std::abs(determinant)<1.e-12f) return false;
        uniform.inverseViewProjection = simd_inverse(uniform.viewProjection);
        uniform.eye = simd_inverse(view).columns[3];
        uniform.sourceRadius = simd_make_float4(simd_make_float3(center[0],center[1],center[2])+worldOrigin,radius);
        uniform.params = {age,0,0,0};
        uniform.viewport = viewport;
        if (viewport.z<=0.f || viewport.w<=0.f || !prepare(device,target.pixelFormat)) return false;
        float minX=viewport.x+viewport.z,minY=viewport.y+viewport.w,maxX=viewport.x,maxY=viewport.y;
        bool crossesNear=false;
        for (float x:{-3.2f,3.2f}) for (float y:{-.25f,3.7f}) for (float z:{-3.2f,3.2f})
        {
            const auto corner=uniform.sourceRadius.xyz+simd_make_float3(x,y,z)*radius;
            const auto clip=simd_mul(uniform.viewProjection,simd_make_float4(corner,1.f));
            if (clip.w<=1.e-5f || clip.z<=0.f) { crossesNear=true; continue; }
            const float px=viewport.x+(clip.x/clip.w*.5f+.5f)*viewport.z;
            const float py=viewport.y+(.5f-clip.y/clip.w*.5f)*viewport.w;
            minX=std::min(minX,px);minY=std::min(minY,py);maxX=std::max(maxX,px);maxY=std::max(maxY,py);
        }
        if (crossesNear) {minX=viewport.x;minY=viewport.y;maxX=viewport.x+viewport.z;maxY=viewport.y+viewport.w;}
        const auto left=NSUInteger(std::clamp(std::floor(minX),0.f,float(target.width)));
        const auto top=NSUInteger(std::clamp(std::floor(minY),0.f,float(target.height)));
        const auto right=NSUInteger(std::clamp(std::ceil(maxX),0.f,float(target.width)));
        const auto bottom=NSUInteger(std::clamp(std::ceil(maxY),0.f,float(target.height)));
        if (right<=left || bottom<=top) return true;
        auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=target;
        pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        auto encoder=[command renderCommandEncoderWithDescriptor:pass];
        if (!encoder) return false;
        [encoder setRenderPipelineState:pipeline];
        [encoder setViewport:MTLViewport{viewport.x,viewport.y,viewport.z,viewport.w,0,1}];
        [encoder setScissorRect:MTLScissorRect{left,top,right-left,bottom-top}];
        [encoder setFragmentBytes:&uniform length:sizeof(uniform) atIndex:0];
        [encoder setFragmentTexture:depth atIndex:0];
        [encoder setFragmentTexture:noise atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding];
        return true;
    }
};
}

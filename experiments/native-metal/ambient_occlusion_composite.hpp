#pragma once
#import <Metal/Metal.h>
#include "ambient_occlusion.hpp"
#include <vector>
namespace storm_metal {
inline constexpr const char*ambientCompositeSource=R"MSL(
#include <metal_stdlib>
using namespace metal;struct V{float4 p[[position]];float2 uv;};
vertex V ao_comp_vs(uint i[[vertex_id]]){float2 q=float2((i<<1)&2,i&2);return{float4(q*float2(2,-2)+float2(-1,1),0,1),q};}
fragment half4 ao_comp_fs(V v[[stage_in]],texture2d<half>scene[[texture(0)]],texture2d<float>ao[[texture(1)]],constant float&cap[[buffer(0)]],sampler s[[sampler(0)]]){half4 c=scene.sample(s,v.uv);half obstruction=half(1.-ao.sample(s,v.uv).r);half3 ambient=min(c.rgb,half3(half(cap)));return half4(c.rgb-ambient*obstruction,c.a);}
)MSL";
struct AmbientOcclusionComposite{sm::AmbientOcclusionPass pass;id<MTLRenderPipelineState>pipeline=nil;id<MTLSamplerState>sampler=nil;id<MTLTexture>coverage=nil;bool ready=false;
 bool prepare(id<MTLDevice>d,NSUInteger w,NSUInteger h,MTLPixelFormat f){NSError*e=nil;if(!ready){if(!pass.initialize(d,&e))return false;auto l=[d newLibraryWithSource:[NSString stringWithUTF8String:ambientCompositeSource] options:nil error:&e];if(!l)return false;auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[l newFunctionWithName:@"ao_comp_vs"];p.fragmentFunction=[l newFunctionWithName:@"ao_comp_fs"];p.colorAttachments[0].pixelFormat=f;pipeline=[d newRenderPipelineStateWithDescriptor:p error:&e];auto sd=[MTLSamplerDescriptor new];sd.minFilter=sd.magFilter=MTLSamplerMinMagFilterLinear;sd.sAddressMode=sd.tAddressMode=MTLSamplerAddressModeClampToEdge;sampler=[d newSamplerStateWithDescriptor:sd];ready=pipeline&&sampler;}if(!ready)return false;if(!coverage||coverage.width!=w||coverage.height!=h){auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm width:w height:h mipmapped:NO];td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageShaderRead;coverage=[d newTextureWithDescriptor:td];std::vector<uint8_t>opaque(w*h,255);[coverage replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:opaque.data() bytesPerRow:w];}return coverage!=nil;}
 bool encode(id<MTLDevice>d,id<MTLCommandBuffer>c,id<MTLTexture>depth,id<MTLTexture>scene,id<MTLTexture>target,simd_float4x4 inverseProjection,float cap){if(!prepare(d,target.width,target.height,target.pixelFormat))return false;auto ao=pass.encode(c,depth,coverage,inverseProjection);if(!ao)return false;auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=target;p.colorAttachments[0].loadAction=MTLLoadActionDontCare;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto r=[c renderCommandEncoderWithDescriptor:p];[r setRenderPipelineState:pipeline];[r setFragmentTexture:scene atIndex:0];[r setFragmentTexture:ao atIndex:1];[r setFragmentBytes:&cap length:sizeof(cap) atIndex:0];[r setFragmentSamplerState:sampler atIndex:0];[r drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[r endEncoding];return true;}
};}

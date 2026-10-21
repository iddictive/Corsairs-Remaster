#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "volumetric_light_shafts.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void need(bool value,const char*message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static uint64_t energy(id<MTLTexture> texture){std::vector<uint8_t> bytes(texture.width*texture.height*4);[texture getBytes:bytes.data() bytesPerRow:texture.width*4 fromRegion:MTLRegionMake2D(0,0,texture.width,texture.height) mipmapLevel:0];uint64_t value=0;for(size_t i=0;i<bytes.size();i+=4)value+=bytes[i]+bytes[i+1]+bytes[i+2];return value;}
static void clear(id<MTLTexture> texture){std::vector<uint8_t> bytes(texture.width*texture.height*4);[texture replaceRegion:MTLRegionMake2D(0,0,texture.width,texture.height) mipmapLevel:0 withBytes:bytes.data() bytesPerRow:texture.width*4];}

int main(){@autoreleasepool{
 using namespace storm_metal;
 need(!lightShaftsEnabled(true,true,0,true),"no authored mesh is exact bypass");need(!lightShaftsEnabled(true,false,1,true),"outdoor is exact bypass");need(!lightShaftsEnabled(true,true,1,false),"missing scene depth is exact bypass");
 LightShaftBudget budget;for(int i=0;i<48;i++)budget.observeFrameGPU(10.);need(budget.tier()>=2,"sustained GPU pressure reduces bounded quality");for(int i=0;i<24;i++)budget.observeFrameGPU(10.);need(!budget.quality().enabled,"sustained pressure disables optional shafts");
 id<MTLDevice> device=MTLCreateSystemDefaultDevice();need(device!=nil,"Metal device");id<MTLCommandQueue> queue=[device newCommandQueue];need(queue!=nil,"Metal queue");
 constexpr unsigned W=256,H=144;auto dd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:W height:H mipmapped:NO];dd.storageMode=MTLStorageModeShared;dd.usage=MTLTextureUsageShaderRead;id<MTLTexture> depth=[device newTextureWithDescriptor:dd];auto cd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:W height:H mipmapped:NO];cd.storageMode=MTLStorageModeShared;cd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;id<MTLTexture> color=[device newTextureWithDescriptor:cd];need(depth&&color,"probe textures");
 const std::array<float,9> vertices{{-.16f,-.28f,.45f,.18f,-.22f,.45f,-.02f,.27f,.45f}};const std::array<uint16_t,3> indices{{0,1,2}};LightShaftMesh mesh;mesh.authoredId=0x57494e444f57ull;mesh.vertexCount=3;mesh.indexCount=3;mesh.positions=[device newBufferWithBytes:vertices.data() length:sizeof(vertices) options:MTLResourceStorageModeShared];mesh.indices=[device newBufferWithBytes:indices.data() length:sizeof(indices) options:MTLResourceStorageModeShared];std::array<LightShaftMesh,1> meshes{{mesh}};
 std::vector<float> z(W*H,.98f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];VolumetricLightShafts shafts;clear(color);double gpuMs=0;for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer> command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,meshes,matrix_identity_float4x4,simd_make_float3(-.3f,-.2f,-1.f),true,true),"encode exact authored triangle");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"GPU completes shaft passes");if(command.GPUStartTime>0)gpuMs+=(command.GPUEndTime-command.GPUStartTime)*1000.;}uint64_t openEnergy=energy(color);need(openEnergy>0,"exact authored triangle produces shaft energy");
 z.assign(W*H,.1f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];clear(color);shafts.reset();for(unsigned frame=0;frame<3;frame++){id<MTLCommandBuffer> command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,meshes,matrix_identity_float4x4,simd_make_float3(-.3f,-.2f,-1.f),true,true),"encode depth-occluded triangle");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"occluded GPU pass completes");}uint64_t blockedEnergy=energy(color);need(blockedEnergy*8<openEnergy,"foreground depth suppresses authored aperture");
 id<MTLCommandBuffer> bypass=[queue commandBuffer];need(!shafts.encode(device,bypass,depth,color,{},matrix_identity_float4x4,simd_make_float3(0,-1,0),true,true),"empty mesh list does not encode");
 std::printf("PASS: exact indexed aperture mesh, depth occlusion, temporal accumulation, bounded quality; avg GPU %.3f ms at %ux%u\n",gpuMs/8.,W,H);
}}

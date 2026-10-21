#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <map>
#include <cassert>
#include <cstdio>
#include <chrono>
// Inspection access only; all production algorithms stay exactly as included.
#define private public
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/point_shadow_registry.hpp"
#undef private
struct Binding {uintptr_t object=0;NSUInteger offset=0;char kind=0;bool operator==(const Binding&)const=default;};
using Bindings=std::map<NSUInteger,Binding>;
@interface Bytes : NSObject { @public std::vector<uint8_t> data; }
@property(readonly) void* contents;
@property(readonly) NSUInteger length;
+(instancetype)bytes:(NSUInteger)n;
@end
@implementation Bytes
+(instancetype)bytes:(NSUInteger)n {auto b=[Bytes new];b->data.resize(n);return b;}
-(void*)contents{return data.data();}
-(NSUInteger)length{return data.size();}
@end
@interface Allocator : NSObject { @public size_t allocations; }
-(id)newBufferWithLength:(NSUInteger)n options:(MTLResourceOptions)options;
@end
@implementation Allocator
-(id)newBufferWithLength:(NSUInteger)n options:(MTLResourceOptions)options{(void)options;++allocations;return [Bytes bytes:n];}
@end
@interface Encoder : NSObject { @public uintptr_t current;std::map<uintptr_t,Bindings> records;size_t textures,buffers,commands,pipelines;bool capture; }
@property(readonly) NSUInteger encodedLength;
-(void)setArgumentBuffer:(id)buffer offset:(NSUInteger)offset;
-(void)setBuffer:(id)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index;
-(void)setTexture:(id)texture atIndex:(NSUInteger)index;
-(void)setIndirectCommandBuffer:(id)commands atIndex:(NSUInteger)index;
-(void)setRenderPipelineState:(id)pipeline atIndex:(NSUInteger)index;
@end
@implementation Encoder
-(NSUInteger)encodedLength{return 65536;}
-(void)setArgumentBuffer:(id)buffer offset:(NSUInteger)offset{assert(offset==0);current=(uintptr_t)(__bridge void*)buffer;}
-(void)setBuffer:(id)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index{++buffers;if(capture)records[current][index]={(uintptr_t)(__bridge void*)buffer,offset,'b'};}
-(void)setTexture:(id)texture atIndex:(NSUInteger)index{++textures;if(capture)records[current][index]={(uintptr_t)(__bridge void*)texture,0,'t'};}
-(void)setIndirectCommandBuffer:(id)commandBuffer atIndex:(NSUInteger)index{++commands;if(capture)records[current][index]={(uintptr_t)(__bridge void*)commandBuffer,0,'c'};}
-(void)setRenderPipelineState:(id)pipeline atIndex:(NSUInteger)index{++pipelines;if(capture)records[current][index]={(uintptr_t)(__bridge void*)pipeline,0,'p'};}
@end
using namespace storm_metal::point_shadow;
uint64_t hashBytes(const void*p,size_t n){uint64_t h=1469598103934665603ull;auto*b=(const uint8_t*)p;while(n--){h^=*b++;h*=1099511628211ull;}return h;}
struct Evidence{uint64_t draws,bounds;Bindings materials,geometry;};
Evidence image(Registry::FrameResources&f,Encoder*m,Encoder*r,size_t n){auto geom=r->records.at((uintptr_t)(__bridge void*)f.resourceTable);geom.erase(0);geom.erase(3);return {hashBytes(f.drawBuffer.contents,n*sizeof(GpuDraw)),hashBytes(f.boundsBuffer.contents,n*sizeof(GpuBounds)),m->records.at((uintptr_t)(__bridge void*)f.materialTable),geom};}
bool equal(const Evidence&a,const Evidence&b){return a.draws==b.draws&&a.bounds==b.bounds&&a.materials==b.materials&&a.geometry==b.geometry;}
void run(size_t n){Registry registry;auto allocator=[Allocator new];auto material=[Encoder new];auto resource=[Encoder new];material->capture=resource->capture=true;
 registry.device_=(id<MTLDevice>)allocator;registry.materialEncoder=(id<MTLArgumentEncoder>)material;registry.resourceEncoder=(id<MTLArgumentEncoder>)resource;registry.opaque_=(id<MTLRenderPipelineState>)[NSObject new];registry.cutout_=(id<MTLRenderPipelineState>)[NSObject new];
 id vertices=[Bytes bytes:4096],indices=[Bytes bytes:4096],alpha=[NSObject new],palette=[Bytes bytes:4096];
 std::vector<Registry::Draw> draws(n);for(size_t i=0;i<n;i++){auto&d=draws[i];d.key.vertexIdentity=i+1;d.key.indexIdentity=i+10000;d.key.geometryState=1;d.key.transformState=1;d.vertices=(id<MTLBuffer>)vertices;d.indices=(id<MTLBuffer>)indices;d.indexCount=3;d.vertexOffset=NSUInteger(i*44);d.indexOffset=NSUInteger(i*6);d.bounds={simd_make_float3(-1,-1,-1),simd_make_float3(1,1,1)};d.alphaTexture=(id<MTLTexture>)(i%3?nil:alpha);}
 registry.beginScene(501,1);for(const auto&d:draws)assert(registry.observe(d));registry.finishScene();
 simd_float3 positions[8];uint64_t old[8];for(unsigned i=0;i<8;i++){positions[i]=simd_make_float3(float(i)*.1f,0,0);old[i]=registry.cubeSignature(positions[i],10);}
 // Independent nearest negative: unchanged static snapshot yields eight cache hits.
 registry.beginScene(501,2);for(const auto&d:draws)assert(registry.observe(d));registry.finishScene();for(unsigned i=0;i<8;i++)assert(registry.cubeSignature(positions[i],10)==old[i]);
 // No NPC/pose needed: change one static transform inside the eight local spheres.
 ++draws[0].key.transformState;draws[0].world.columns[3].x=.2f;registry.beginScene(501,3);for(const auto&d:draws)assert(registry.observe(d));registry.finishScene();for(unsigned i=0;i<8;i++)assert(registry.cubeSignature(positions[i],10)!=old[i]);
 Registry::FrameResources frames[8];for(auto&f:frames)f.icb=(id<MTLIndirectCommandBuffer>)[NSObject new];
 auto start=std::chrono::steady_clock::now();for(auto&f:frames)assert(registry.buildGpuTables(f));auto coldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 auto first=image(frames[0],material,resource,n);for(unsigned i=1;i<8;i++)assert(equal(first,image(frames[i],material,resource,n)));
 assert(material->textures==8*n);assert(resource->buffers==8*(3*n+1));assert(resource->commands==8&&resource->pipelines==16);
 const size_t sourceSetters=material->textures+resource->buffers-8;
 assert(sourceSetters==32*n);assert(allocator->allocations==8*5);assert(registry.dirty);
 // The implementation doesn't consume dirty: false still regenerates/rebinds all N.
 registry.dirty=false;auto before=resource->buffers+material->textures;assert(registry.buildGpuTables(frames[0]));assert(resource->buffers+material->textures-before==4*n+1);assert(!registry.dirty);
 // Binding lifetime is NOT covered by the geometry/pose key. A new frame palette
 // slice with equal pose bytes changes resource-table inputs even at equal keys.
 draws[0].skinPalette=(id<MTLBuffer>)palette;draws[0].boneCount=1;draws[0].key.poseState=77;registry.beginScene(501,4);for(const auto&d:draws)assert(registry.observe(d));registry.finishScene();assert(registry.buildGpuTables(frames[0]));auto posed=image(frames[0],material,resource,n);auto posedSig=registry.cubeSignature(positions[0],10);
 draws[0].skinPaletteOffset=256;registry.beginScene(501,5);for(const auto&d:draws)assert(registry.observe(d));registry.finishScene();assert(registry.cubeSignature(positions[0],10)==posedSig);assert(registry.buildGpuTables(frames[0]));auto nextSlice=image(frames[0],material,resource,n);assert(posed.draws==nextSlice.draws&&posed.bounds==nextSlice.bounds&&posed.materials==nextSlice.materials&&posed.geometry!=nextSlice.geometry);
 // Out-of-range geometry does not invalidate local lamp depth signatures.
 auto distant=draws.back();++distant.key.transformState;distant.bounds={simd_make_float3(99,99,99),simd_make_float3(101,101,101)};registry.beginScene(501,6);for(size_t i=0;i+1<draws.size();++i)assert(registry.observe(draws[i]));assert(registry.observe(distant));registry.finishScene();auto localSig=registry.cubeSignature(positions[0],10);++distant.key.transformState;registry.beginScene(501,7);for(size_t i=0;i+1<draws.size();++i)assert(registry.observe(draws[i]));assert(registry.observe(distant));registry.finishScene();assert(registry.cubeSignature(positions[0],10)==localSig);
 // Timings are for the fake CPU allocator/dispatch, NOT Metal or player FPS.
 material->capture=resource->capture=false;constexpr unsigned repeats=80;start=std::chrono::steady_clock::now();for(unsigned pass=0;pass<repeats;pass++)for(auto&f:frames)assert(registry.buildGpuTables(f));double eightMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/repeats;
 start=std::chrono::steady_clock::now();for(unsigned pass=0;pass<repeats;pass++)assert(registry.buildGpuTables(frames[0]));double oneMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/repeats;
 printf("N=%zu K=8: semantic tables identical; setters=%zu vs one=%zu; CPU copied=%zu vs one=%zu bytes; GpuDraw=%zu Bounds=%zu; fake cold=%.3fms seeded8=%.3fms seeded1=%.3fms\n",n,sourceSetters,4*n,8*n*(sizeof(GpuDraw)+sizeof(GpuBounds)),n*(sizeof(GpuDraw)+sizeof(GpuBounds)),sizeof(GpuDraw),sizeof(GpuBounds),coldMs,eightMs,oneMs);
 puts("PASS unchanged/outside-caster cube reuse; static transform invalidates8; dirtyfalse still rebuilds; equal pose/different palette offset invalidates binding snapshot");
}
int main(){@autoreleasepool{run(256);run(2048);}puts("PASS no Metal device, command queue, game or GPU calls; exact production buildGpuTables/lifecycle/cubeSignature used");}

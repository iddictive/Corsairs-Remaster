#import <Metal/Metal.h>
#include "point_shadow_registry.hpp"
#include <cstdio>
#include <cstdlib>

static void need(bool value,const char* label){if(!value){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}

int main(){
 using namespace storm_metal::point_shadow;
 LifecycleModel registry;
 Key opaque{1,2,3,4,5,6,7};
 Bounds near{{-1,-1,-1},{1,1,1}},far{{50,50,50},{51,51,51}};
 registry.begin(11,1);need(registry.observe(opaque,near),"first insert");need(registry.finish()==1,"first generation retained");
 registry.begin(11,2);need(!registry.observe(opaque,near),"stable identity reused");need(registry.finish()==1,"stable generation retained");
 auto moved=opaque;++moved.transformState;registry.begin(11,3);need(registry.observe(moved,near),"transform revision invalidates");need(registry.finish()==1,"stale transform swept");
 auto geometry=moved;++geometry.vertexRevision;registry.begin(11,4);need(registry.observe(geometry,near),"vertex revision invalidates");need(registry.finish()==1,"stale geometry swept");
 auto alpha=geometry;++alpha.materialState;registry.begin(11,5);need(registry.observe(alpha,near),"alpha material revision invalidates");need(registry.finish()==1,"stale material swept");
 need(LifecycleModel::intersects(near,{0,0,0},2),"near bounds admitted");need(!LifecycleModel::intersects(far,{0,0,0},2),"far bounds rejected");
 registry.begin(12,6);need(registry.entries.empty(),"location transition evicts registry");

 id<MTLDevice> device=MTLCreateSystemDefaultDevice();need(device!=nil,"Metal device");
 const char* shader="#include <metal_stdlib>\nusing namespace metal;struct O{float4 p[[position]];};vertex O v(uint i[[vertex_id]]){O o;o.p=float4(0);return o;}fragment void f(){}";
 NSError* error=nil;id<MTLLibrary> library=[device newLibraryWithSource:@(shader) options:nil error:&error];need(library!=nil,"fixture library");
 auto makePipeline=[&](){auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[library newFunctionWithName:@"v"];d.fragmentFunction=[library newFunctionWithName:@"f"];d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;return [device newRenderPipelineStateWithDescriptor:d error:&error];};
 id<MTLRenderPipelineState> opaquePipeline=makePipeline(),cutoutPipeline=makePipeline();need(opaquePipeline&&cutoutPipeline,"fixture pipelines");
 Registry gpu;need(gpu.prepare(device,opaquePipeline,cutoutPipeline),"GPU compute/ICB owner compiles");
 auto s=gpu.statistics();need(s.cpuFaceTraversals==0&&s.cpuShadowDraws==0,"zero CPU point-shadow counters");
 std::puts("PASS persistent point-shadow lifecycle, bounds, alpha/dynamic invalidation, and GPU ICB compilation");
}

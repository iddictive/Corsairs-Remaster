#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

namespace storm_metal::point_shadow {

// Point-shadow casters are discovered while the ordinary scene is submitted.
// The registry is persistent: the six cube faces consume this compact snapshot
// and never ask Location/MODELR to render the scene again.
struct Key {
    uint64_t vertexIdentity{}, vertexRevision{}, indexIdentity{}, indexRevision{};
    uint64_t geometryState{}, transformState{}, materialState{}, poseState{};
    bool operator==(const Key&) const = default;
};

struct KeyHash {
    size_t operator()(const Key& k) const noexcept {
        uint64_t h = 1469598103934665603ull;
        const uint64_t words[] = {k.vertexIdentity, k.vertexRevision, k.indexIdentity,
                                  k.indexRevision, k.geometryState, k.transformState,
                                  k.materialState, k.poseState};
        for (auto v : words) { h ^= v; h *= 1099511628211ull; }
        return size_t(h);
    }
};

struct Bounds { simd_float3 minimum{}, maximum{}; };

struct LifecycleModel {
    struct Entry { Key key{}; Bounds bounds{}; uint64_t seenGeneration{}; };
    std::unordered_map<Key, Entry, KeyHash> entries;
    uint64_t scene{};

    void begin(uint64_t newScene, uint64_t generation) {
        if (scene != newScene) { entries.clear(); scene = newScene; }
        for (auto& [_, entry] : entries) entry.seenGeneration = 0;
        currentGeneration = generation;
    }
    bool observe(const Key& key, Bounds bounds) {
        auto [it, inserted] = entries.try_emplace(key, Entry{key, bounds, currentGeneration});
        it->second.bounds = bounds; it->second.seenGeneration = currentGeneration;
        return inserted;
    }
    size_t finish() {
        std::erase_if(entries, [&](const auto& pair) {
            return pair.second.seenGeneration != currentGeneration;
        });
        return entries.size();
    }
    static bool intersects(Bounds b, simd_float3 p, float range) {
        simd_float3 q = simd_clamp(p, b.minimum, b.maximum);
        simd_float3 d = p - q;
        return simd_dot(d, d) <= range * range;
    }
private:
    uint64_t currentGeneration{};
};

struct GpuBounds { simd_float4 minimum, maximum; };
struct GpuDraw {
    uint64_t vertexAddress{}, indexAddress{};
    uint32_t vertexOffset{}, indexOffset{}, indexCount{}, indexType{};
    uint32_t materialIndex{}, primitive{}, cull{}, stride{};
    uint32_t uvOffset{}, colorOffset{}, useColorAlpha{}, skinned{};
    uint32_t boneCount{};int32_t baseVertex{};uint32_t padding{};
    simd_float4 alpha{};
    simd_float4x4 world{};
};
struct GpuFace { simd_float4x4 viewProjection; simd_float4 lightRange; };
struct Statistics {
    uint64_t registryGeneration{}, registryInvalidations{}, gpuCullDispatches{};
    uint64_t indirectFaceSubmissions{}, cpuFaceTraversals{}, cpuShadowDraws{};
    uint64_t frameResourceExhaustions{};
    uint32_t registeredCasters{}, candidateCommands{};
};

// Metal execution owner. Geometry/material registration occurs in the normal
// draw path; encodeCube performs one compute dispatch plus one ICB execution per
// face. It deliberately exposes counters whose accepted value is exactly zero
// for CPU face traversal and CPU shadow draw submission.
class Registry {
public:
    static constexpr uint32_t MaxCasters = 2048;
    struct Draw {
        Key key{}; Bounds bounds{};
        id<MTLBuffer> vertices=nil, indices=nil;
        NSUInteger vertexOffset{}, indexOffset{}, indexCount{};
        MTLIndexType indexType=MTLIndexTypeUInt32;
        MTLPrimitiveType primitive=MTLPrimitiveTypeTriangle;
        MTLCullMode cull=MTLCullModeBack;
        simd_float4x4 world=matrix_identity_float4x4;
        id<MTLTexture> alphaTexture=nil;
        id<MTLBuffer> skinPalette=nil;NSUInteger skinPaletteOffset=0;
        float alphaReference{}; uint32_t alphaFunction{}, alphaTest{};
        uint32_t stride=112,uvOffset=32,colorOffset=16,useColorAlpha=1,boneCount=0;
        int32_t baseVertex{};
    };

    void beginScene(uint64_t scene, uint64_t generation) {
        model.begin(scene, generation); generation_ = generation;
        if (scene_ != scene) { draws.clear(); lookup.clear(); scene_=scene; dirty=true; ++stats.registryInvalidations; }
        for (auto& item : draws) item.seen=false;
    }

    bool observe(const Draw& draw) {
        if (!draw.vertices || !draw.indices || !draw.indexCount) return false;
        model.observe(draw.key, draw.bounds);
        auto found=lookup.find(draw.key);
        if(found==lookup.end()) {
            // A stale entry may be recycled across frames, but a second current-frame
            // instance with the same backing mesh must remain a distinct cube caster.
            auto stable=std::find_if(draws.begin(),draws.end(),[&](const Stored&item){const auto&k=item.draw.key;return !item.seen&&k.vertexIdentity==draw.key.vertexIdentity&&k.indexIdentity==draw.key.indexIdentity&&k.geometryState==draw.key.geometryState;});
            if(stable!=draws.end()){model.entries.erase(stable->draw.key);stable->draw=draw;stable->seen=true;dirty=true;lookup.clear();for(size_t i=0;i<draws.size();++i)lookup.emplace(draws[i].draw.key,i);}
            else {if(draws.size()>=MaxCasters)return false;lookup.emplace(draw.key,draws.size());draws.push_back({draw,true});dirty=true;}
        } else {
            auto&stored=draws[found->second];
            const bool transformChanged=std::memcmp(&stored.draw.world,&draw.world,sizeof(draw.world))!=0;
            const bool boundsChanged=std::memcmp(&stored.draw.bounds,&draw.bounds,sizeof(draw.bounds))!=0;
            stored.draw=draw;stored.seen=true;
            // World rebasing and animated bounds affect the GPU cull table
            // even when the geometry key and pose key remain unchanged.
            if(transformChanged||boundsChanged||draw.key.poseState||draw.skinPalette)dirty=true;
        }
        return true;
    }

    void finishScene() {
        model.finish();
        const auto old=draws.size();
        std::erase_if(draws, [](const Stored& item){return !item.seen;});
        if(draws.size()!=old) dirty=true;
        if(dirty){lookup.clear();for(size_t i=0;i<draws.size();++i)lookup.emplace(draws[i].draw.key,i);}
        stats.registeredCasters=uint32_t(draws.size()); stats.registryGeneration=generation_;
    }

    const Statistics& statistics() const { return stats; }
    const LifecycleModel& lifecycle() const { return model; }
    bool empty() const { return draws.empty(); }

    bool prepare(id<MTLDevice> device, id<MTLRenderPipelineState> = nil,
                 id<MTLRenderPipelineState> = nil) {
        if (!device) return false;
        if (device_==device && compute && opaque_ && cutout_) return true;
        for(auto& frame:framePool)frame.reset();frameCursor=0;
        device_=device;
        NSError* error=nil;
        id<MTLLibrary> lib=[device newLibraryWithSource:@(shaderSource) options:nil error:&error];
        if(!lib){fprintf(stderr,"[StormMetal] point shadow registry shader: %s\n",error.localizedDescription.UTF8String);return false;}
        id<MTLFunction> function=[lib newFunctionWithName:@"point_shadow_encode"];
        compute=[device newComputePipelineStateWithFunction:function error:&error];
        if(!compute){fprintf(stderr,"[StormMetal] point shadow registry compute: %s\n",error.localizedDescription.UTF8String);return false;}
        auto render=[MTLRenderPipelineDescriptor new];render.supportIndirectCommandBuffers=YES;render.vertexFunction=[lib newFunctionWithName:@"point_shadow_vertex"];render.fragmentFunction=[lib newFunctionWithName:@"point_shadow_opaque"];render.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;opaque_=[device newRenderPipelineStateWithDescriptor:render error:&error];render.fragmentFunction=[lib newFunctionWithName:@"point_shadow_cutout"];cutout_=[device newRenderPipelineStateWithDescriptor:render error:&error];if(!opaque_||!cutout_){fprintf(stderr,"[StormMetal] point shadow registry render: %s\n",error.localizedDescription.UTF8String);return false;}
        auto depth=[MTLDepthStencilDescriptor new];depth.depthCompareFunction=MTLCompareFunctionLess;depth.depthWriteEnabled=YES;depthState=[device newDepthStencilStateWithDescriptor:depth];
        resourceEncoder=[function newArgumentEncoderWithBufferIndex:3];
        id<MTLFunction> cutoutFunction=[lib newFunctionWithName:@"point_shadow_cutout"];
        materialEncoder=[cutoutFunction newArgumentEncoderWithBufferIndex:3];
        return resourceEncoder!=nil&&materialEncoder!=nil&&depthState!=nil;
    }

    bool encodeCube(id<MTLCommandBuffer> command, id<MTLTexture> cube,
                    const std::array<GpuFace,6>& faces) {
        if(!command||!cube||!compute||!resourceEncoder||!materialEncoder||draws.empty())return false;
        auto frame=acquireFrame();
        if(!frame){++stats.frameResourceExhaustions;return false;}
        if(!buildGpuTables(*frame)){frame->inFlight.store(false,std::memory_order_release);return false;}
        std::memcpy(frame->faceBuffer.contents,faces.data(),sizeof(GpuFace)*6);
        auto blit=[command blitCommandEncoder];
        [blit resetCommandsInBuffer:frame->icb withRange:NSMakeRange(0,draws.size()*6)];[blit endEncoding];
        auto c=[command computeCommandEncoder];[c setComputePipelineState:compute];
        [c setBuffer:frame->drawBuffer offset:0 atIndex:0];[c setBuffer:frame->boundsBuffer offset:0 atIndex:1];
        [c setBuffer:frame->faceBuffer offset:0 atIndex:2];[c setBuffer:frame->resourceTable offset:0 atIndex:3];uint32_t casterCount=uint32_t(draws.size());[c setBytes:&casterCount length:sizeof(casterCount) atIndex:4];
        [c useResource:frame->icb usage:MTLResourceUsageWrite];
        [c useResource:frame->resourceTable usage:MTLResourceUsageRead];
        [c useResource:frame->materialTable usage:MTLResourceUsageRead];
        for(const auto& item:draws){[c useResource:item.draw.vertices usage:MTLResourceUsageRead];[c useResource:item.draw.indices usage:MTLResourceUsageRead];if(item.draw.alphaTexture)[c useResource:item.draw.alphaTexture usage:MTLResourceUsageRead];if(item.draw.skinPalette)[c useResource:item.draw.skinPalette usage:MTLResourceUsageRead];}
        [c dispatchThreads:MTLSizeMake(draws.size(),6,1) threadsPerThreadgroup:MTLSizeMake(std::min<size_t>(32,draws.size()),1,1)];[c endEncoding];
        ++stats.gpuCullDispatches;stats.candidateCommands=uint32_t(draws.size()*6);
        std::vector<id<MTLResource>> residency{frame->icb,frame->drawBuffer,frame->boundsBuffer,frame->faceBuffer,frame->resourceTable,frame->materialTable};for(const auto&item:draws){residency.push_back(item.draw.vertices);residency.push_back(item.draw.indices);if(item.draw.alphaTexture)residency.push_back(item.draw.alphaTexture);if(item.draw.skinPalette)residency.push_back(item.draw.skinPalette);}
        for(unsigned face=0;face<6;++face){auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=cube;pass.depthAttachment.slice=face;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setViewport:MTLViewport{0,0,double(cube.width),double(cube.height),0,1}];[e setFrontFacingWinding:MTLWindingClockwise];[e setDepthStencilState:depthState];[e setDepthBias:0 slopeScale:1 clamp:0];[e useResources:residency.data() count:residency.size() usage:MTLResourceUsageRead stages:MTLRenderStageVertex|MTLRenderStageFragment];[e executeCommandsInBuffer:frame->icb withRange:NSMakeRange(face*draws.size(),draws.size())];[e endEncoding];++stats.indirectFaceSubmissions;}
        [command addCompletedHandler:^(id<MTLCommandBuffer> completed){(void)completed;frame->inFlight.store(false,std::memory_order_release);}];
        return true;
    }

private:
    struct FrameResources {
        id<MTLIndirectCommandBuffer> icb=nil;
        id<MTLBuffer> resourceTable=nil,materialTable=nil,drawBuffer=nil,boundsBuffer=nil,faceBuffer=nil;
        std::atomic_bool inFlight{false};
    };
    struct Stored { Draw draw; bool seen; };
    LifecycleModel model; std::vector<Stored> draws; std::unordered_map<Key,size_t,KeyHash> lookup;
    uint64_t scene_{},generation_{};bool dirty=true;Statistics stats{};
    id<MTLDevice> device_=nil;id<MTLComputePipelineState> compute=nil;
    id<MTLRenderPipelineState> opaque_=nil,cutout_=nil;
    id<MTLArgumentEncoder> resourceEncoder=nil;
    id<MTLArgumentEncoder> materialEncoder=nil;id<MTLDepthStencilState> depthState=nil;
    // The traversal submits at most eight point cubes per frame and the renderer
    // permits three frames in flight. Every cube needs a distinct mutable ICB
    // until its owning command buffer completes.
    std::array<std::shared_ptr<FrameResources>,24> framePool{};size_t frameCursor=0;

    std::shared_ptr<FrameResources> acquireFrame(){
        for(size_t attempt=0;attempt<framePool.size();++attempt){
            const size_t slot=(frameCursor+attempt)%framePool.size();
            if(!framePool[slot])framePool[slot]=std::make_shared<FrameResources>();
            bool expected=false;
            if(framePool[slot]->inFlight.compare_exchange_strong(expected,true,std::memory_order_acq_rel)){
                frameCursor=(slot+1)%framePool.size();return framePool[slot];
            }
        }
        return {};
    }

    bool buildGpuTables(FrameResources& frame){
        std::vector<GpuDraw> gpuDraws(draws.size());std::vector<GpuBounds> bounds(draws.size());
        for(size_t i=0;i<draws.size();++i){const auto&d=draws[i].draw;gpuDraws[i].vertexOffset=uint32_t(d.vertexOffset);gpuDraws[i].indexOffset=uint32_t(d.indexOffset);gpuDraws[i].indexCount=uint32_t(d.indexCount);gpuDraws[i].indexType=d.indexType==MTLIndexTypeUInt32;gpuDraws[i].materialIndex=uint32_t(i);gpuDraws[i].primitive=uint32_t(d.primitive);gpuDraws[i].cull=uint32_t(d.cull);gpuDraws[i].stride=d.stride;gpuDraws[i].uvOffset=d.uvOffset;gpuDraws[i].colorOffset=d.colorOffset;gpuDraws[i].useColorAlpha=d.useColorAlpha;gpuDraws[i].skinned=d.skinPalette!=nil;gpuDraws[i].boneCount=d.boneCount;gpuDraws[i].baseVertex=d.baseVertex;gpuDraws[i].alpha={d.alphaReference,float(d.alphaFunction),float(d.alphaTest),0};gpuDraws[i].world=d.world;bounds[i]={simd_make_float4(d.bounds.minimum,1),simd_make_float4(d.bounds.maximum,1)};}
        if(!frame.drawBuffer)frame.drawBuffer=[device_ newBufferWithLength:MaxCasters*sizeof(GpuDraw) options:MTLResourceStorageModeShared];
        if(!frame.boundsBuffer)frame.boundsBuffer=[device_ newBufferWithLength:MaxCasters*sizeof(GpuBounds) options:MTLResourceStorageModeShared];
        if(!frame.faceBuffer)frame.faceBuffer=[device_ newBufferWithLength:sizeof(GpuFace)*6 options:MTLResourceStorageModeShared];
        if(!frame.drawBuffer||!frame.boundsBuffer||!frame.faceBuffer)return false;
        std::memcpy(frame.drawBuffer.contents,gpuDraws.data(),gpuDraws.size()*sizeof(GpuDraw));
        std::memcpy(frame.boundsBuffer.contents,bounds.data(),bounds.size()*sizeof(GpuBounds));
        if(!frame.icb){auto descriptor=[MTLIndirectCommandBufferDescriptor new];descriptor.commandTypes=MTLIndirectCommandTypeDrawIndexed;descriptor.inheritPipelineState=NO;descriptor.inheritBuffers=NO;descriptor.maxVertexBufferBindCount=5;descriptor.maxFragmentBufferBindCount=1;frame.icb=[device_ newIndirectCommandBufferWithDescriptor:descriptor maxCommandCount:MaxCasters*6 options:MTLResourceStorageModePrivate];}
        if(!frame.resourceTable)frame.resourceTable=[device_ newBufferWithLength:resourceEncoder.encodedLength options:MTLResourceStorageModeShared];
        if(!frame.materialTable)frame.materialTable=[device_ newBufferWithLength:materialEncoder.encodedLength options:MTLResourceStorageModeShared];
        if(!frame.icb||!frame.resourceTable||!frame.materialTable)return false;
        [materialEncoder setArgumentBuffer:frame.materialTable offset:0];
        [resourceEncoder setArgumentBuffer:frame.resourceTable offset:0];
        [resourceEncoder setIndirectCommandBuffer:frame.icb atIndex:0];
        [resourceEncoder setRenderPipelineState:opaque_ atIndex:1];
        [resourceEncoder setRenderPipelineState:cutout_ atIndex:2];
        [resourceEncoder setBuffer:frame.materialTable offset:0 atIndex:3];
        for(size_t i=0;i<draws.size();++i){const auto&d=draws[i].draw;[materialEncoder setTexture:d.alphaTexture atIndex:i];[resourceEncoder setBuffer:d.vertices offset:0 atIndex:4+i];[resourceEncoder setBuffer:d.indices offset:0 atIndex:4+MaxCasters+i];[resourceEncoder setBuffer:d.skinPalette offset:d.skinPaletteOffset atIndex:4+MaxCasters*2+i];}
        return true;
    }

    static constexpr const char* shaderSource=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct Draw {ulong vertexAddress,indexAddress;uint vertexOffset,indexOffset,indexCount,indexType,materialIndex,primitive,cull,stride,uvOffset,colorOffset,useColorAlpha,skinned,boneCount;int baseVertex;uint padding;float4 alpha;float4x4 world;};
struct Bounds {float4 minimum,maximum;};
struct Face {float4x4 viewProjection;float4 lightRange;};
struct Materials {array<texture2d<float>,2048> alpha [[id(0)]];};
struct Resources {command_buffer commands [[id(0)]];render_pipeline_state opaque [[id(1)]];render_pipeline_state cutout [[id(2)]];constant Materials*materials [[id(3)]];array<device uchar*,2048> vertices [[id(4)]];array<device uchar*,2048> indices [[id(2052)]];array<device float4x4*,2048> palettes [[id(4100)]];};
struct O {float4 position [[position]];float2 uv;float alpha;uint material [[flat]];float alphaRef;uint alphaFunc;uint alphaTest;};
vertex O point_shadow_vertex(uint vertexId [[vertex_id]],constant Draw&d[[buffer(1)]],constant Face&face[[buffer(2)]],const device uchar*vertices[[buffer(0)]],constant Resources&resources[[buffer(4)]]){const device uchar*p=vertices+vertexId*d.stride;float4 local=float4(*(const device packed_float3*)p,1);if(d.skinned&&d.boneCount){uint packed=*(const device uint*)(p+16),a=min(packed&255u,d.boneCount-1),b=min((packed>>8)&255u,d.boneCount-1);float w=*(const device float*)(p+12);float4x4 skin=resources.palettes[d.materialIndex][a]*w+resources.palettes[d.materialIndex][b]*(1.-w);skin[0].x=-skin[0].x;skin[1].x=-skin[1].x;skin[2].x=-skin[2].x;skin[3].x=-skin[3].x;local=skin*local;}O o;o.position=face.viewProjection*d.world*local;o.uv=*(const device packed_float2*)(p+d.uvOffset);o.alpha=d.useColorAlpha?float((*(const device uint*)(p+d.colorOffset))>>24)/255.:1;o.material=d.materialIndex;o.alphaRef=d.alpha.x;o.alphaFunc=uint(d.alpha.y);o.alphaTest=uint(d.alpha.z);return o;}
fragment void point_shadow_opaque(){}
fragment void point_shadow_cutout(O o[[stage_in]],constant Materials&materials[[buffer(3)]]){if(o.alphaTest){constexpr sampler s(coord::normalized,address::repeat,filter::linear);float a=o.alpha*materials.alpha[o.material].sample(s,o.uv).a;if(o.alphaFunc==5?a<=o.alphaRef:a<o.alphaRef)discard_fragment();}}
kernel void point_shadow_encode(const device Draw*draws[[buffer(0)]],const device Bounds*bounds[[buffer(1)]],constant Face*faces[[buffer(2)]],constant Resources&resources[[buffer(3)]],constant uint&casterCount[[buffer(4)]],uint2 tid[[thread_position_in_grid]]){
 uint drawIndex=tid.x,face=tid.y;if(drawIndex>=casterCount||face>=6)return;Draw d=draws[drawIndex];Bounds b=bounds[drawIndex];float3 p=faces[face].lightRange.xyz;float3 q=clamp(p,b.minimum.xyz,b.maximum.xyz);if(distance_squared(p,q)>faces[face].lightRange.w*faces[face].lightRange.w)return;
 bool outside[6]={true,true,true,true,true,true};for(uint corner=0;corner<8;corner++){float3 w=float3(corner&1?b.maximum.x:b.minimum.x,corner&2?b.maximum.y:b.minimum.y,corner&4?b.maximum.z:b.minimum.z);float4 clip=faces[face].viewProjection*float4(w,1);outside[0]&=clip.x < -clip.w;outside[1]&=clip.x > clip.w;outside[2]&=clip.y < -clip.w;outside[3]&=clip.y > clip.w;outside[4]&=clip.z < 0;outside[5]&=clip.z > clip.w;}for(uint plane=0;plane<6;plane++)if(outside[plane])return;
 render_command command(resources.commands,face*casterCount+drawIndex);command.set_render_pipeline_state(d.alpha.z?resources.cutout:resources.opaque);command.set_vertex_buffer(resources.vertices[drawIndex],d.vertexOffset,0);command.set_vertex_buffer(draws+drawIndex,0,1);command.set_vertex_buffer(faces+face,0,2);command.set_vertex_buffer(&resources,0,4);if(d.alpha.z)command.set_fragment_buffer(resources.materials,3);if(d.indexType)command.draw_indexed_primitives(primitive_type::triangle,d.indexCount,(device uint*)(resources.indices[drawIndex]+d.indexOffset),1,d.baseVertex,0);else command.draw_indexed_primitives(primitive_type::triangle,d.indexCount,(device ushort*)(resources.indices[drawIndex]+d.indexOffset),1,d.baseVertex,0);
}
)MSL";
};

} // namespace storm_metal::point_shadow

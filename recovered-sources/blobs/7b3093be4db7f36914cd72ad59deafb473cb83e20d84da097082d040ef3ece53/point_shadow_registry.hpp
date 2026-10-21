#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace storm_metal::point_shadow {

inline constexpr const char* SamplingMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
float samplePoint(depthcube<float> map,float3 ray,float receiverMajor,float far,float radius) {
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 float3 direction=ray/max(receiverMajor,.00001f),a=abs(direction);
 float3 u=a.x>=a.y&&a.x>=a.z?float3(0,0,1):float3(1,0,0);
 float3 v=a.y>a.x&&a.y>=a.z?float3(0,0,1):float3(0,1,0);
 float step=2.f*radius/float(map.get_width());
 float depth=far/(far-.1f)-(.1f*far/(far-.1f))/receiverMajor-.0012f;
 float visibility=0.f;
 for(uint i=0;i<4;i++) {
  float2 offset=float2(i&1u? .5f:-.5f,i&2u? .5f:-.5f)*step;
  visibility+=map.sample_compare(comparison,normalize(direction+u*offset.x+v*offset.y),depth);
 }
 return visibility*.25f;
}
)MSL";

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

struct GeometryKey {
    uint64_t vertexIdentity{}, indexIdentity{}, geometryState{};
    bool operator==(const GeometryKey&) const = default;
};

struct GeometryKeyHash {
    size_t operator()(const GeometryKey& k) const noexcept {
        uint64_t h = 1469598103934665603ull;
        const uint64_t words[] = {k.vertexIdentity, k.indexIdentity, k.geometryState};
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
struct GpuCluster {
    simd_float4 minimum{}, maximum{};
    uint32_t drawIndex{}, firstIndex{}, indexCount{}, flags{};
};
static_assert(sizeof(GpuCluster)==48);
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
    static constexpr uint32_t MaxClusterRows = 8192;
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
        bool staticClusterEligible=false;
    };

    void beginScene(uint64_t scene, uint64_t generation) {
        ++observationEpoch;
        model.begin(scene, generation); generation_ = generation;
        if (scene_ != scene) { draws.clear(); lookup.clear(); availableSlots.clear(); clearClusterCache(); scene_=scene; dirty=true; ++stats.registryInvalidations; }
        availableSlots.clear();
        for (size_t i = 0; i < draws.size(); ++i) {
            draws[i].seen = false;
            draws[i].nextAvailable = UINT32_MAX;
            const auto& k = draws[i].draw.key;
            const GeometryKey gk{k.vertexIdentity, k.indexIdentity, k.geometryState};
            auto& q = availableSlots[gk];
            if (q.tail == UINT32_MAX) {
                q.head = q.tail = uint32_t(i);
            } else {
                draws[q.tail].nextAvailable = uint32_t(i);
                q.tail = uint32_t(i);
            }
        }
    }

    bool observe(const Draw& draw) {
        if (!draw.vertices || !draw.indices || !draw.indexCount) return false;
        ++observationEpoch;
        model.observe(draw.key, draw.bounds);
        auto found=lookup.find(draw.key);
        if(found==lookup.end()) {
            // A stale entry may be recycled across frames, but a second current-frame
            // instance with the same backing mesh must remain a distinct cube caster.
            uint32_t recycledSlot = UINT32_MAX;
            const GeometryKey gk{draw.key.vertexIdentity, draw.key.indexIdentity, draw.key.geometryState};
            auto qIt = availableSlots.find(gk);
            if (qIt != availableSlots.end()) {
                while (qIt->second.head != UINT32_MAX) {
                    const uint32_t candidate = qIt->second.head;
                    qIt->second.head = draws[candidate].nextAvailable;
                    if (!draws[candidate].seen) {
                        recycledSlot = candidate;
                        break;
                    }
                }
            }
            if (recycledSlot != UINT32_MAX) {
                auto& stored = draws[recycledSlot];
                model.entries.erase(stored.draw.key);
                lookup.erase(stored.draw.key);
                stored.draw = draw;
                stored.seen = true;
                dirty = true;
                lookup.emplace(draw.key, size_t(recycledSlot));
            } else {
                if (draws.size() >= MaxCasters) return false;
                lookup.emplace(draw.key, draws.size());
                draws.push_back({draw, true});
                dirty = true;
            }
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
        ++observationEpoch;
        model.finish();
        const auto old=draws.size();
        std::erase_if(draws, [](const Stored& item){return !item.seen;});
        if(draws.size()!=old) { dirty=true; lookup.clear(); for(size_t i=0;i<draws.size();++i)lookup.emplace(draws[i].draw.key,i); }
        availableSlots.clear();
        stats.registeredCasters=uint32_t(draws.size()); stats.registryGeneration=generation_;
    }

    const Statistics& statistics() const { return stats; }
    const LifecycleModel& lifecycle() const { return model; }
    bool empty() const { return draws.empty(); }
    // Private probe control; -1 returns to the process environment selection.
    void setProbeClusterOverride(int mode) { probeClusterOverride=mode==0||mode==128||mode==512?mode:-1; }
    void setProbeOptimizeOverride(int value){probeOptimizeOverride=value;}

    // Only occluders intersecting this light can invalidate its depth cube.
    // Intensity/exposure and the camera do not change shadow geometry.
    uint64_t cubeSignature(simd_float3 position, float range) const {
        uint64_t h = (1469598103934665603ull ^ scene_) * 1099511628211ull;
        for (const auto& item : draws) {
            const auto& d = item.draw;
            if (!LifecycleModel::intersects(d.bounds, position, range)) continue;
            h = (h ^ KeyHash{}(d.key)) * 1099511628211ull;
        }
        return h;
    }

    bool prepare(id<MTLDevice> device, id<MTLRenderPipelineState> = nil,
                 id<MTLRenderPipelineState> = nil) {
        if (!device) return false;
        if (device_==device && compute && clusterCompute && opaque_ && cutout_) return true;
        preparedCommand=nil; preparedFrame.reset();
        for(auto& frame:framePool)frame.reset();frameCursor=0;
        device_=device;
        NSError* error=nil;
        id<MTLLibrary> lib=[device newLibraryWithSource:@(shaderSource) options:nil error:&error];
        if(!lib){fprintf(stderr,"[StormMetal] point shadow registry shader: %s\n",error.localizedDescription.UTF8String);return false;}
        id<MTLFunction> function=[lib newFunctionWithName:@"point_shadow_encode"];
        compute=[device newComputePipelineStateWithFunction:function error:&error];
        if(!compute){fprintf(stderr,"[StormMetal] point shadow registry compute: %s\n",error.localizedDescription.UTF8String);return false;}
        clusterCompute=[device newComputePipelineStateWithFunction:[lib newFunctionWithName:@"point_cluster_encode"] error:&error];
        if(!clusterCompute){fprintf(stderr,"[StormMetal] point shadow cluster compute: %s\n",error.localizedDescription.UTF8String);return false;}
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
        const int mode=selectedClusterMode();
        frame->tableSource.reset();
        if(preparedCommand==command&&preparedEpoch==observationEpoch&&preparedMode==mode&&preparedFrame){
            // Geometry is immutable inside this command/observation snapshot.
            // The destination keeps its own ICB, face constants and argument table.
            frame->rowCount=preparedFrame->rowCount;
            if(!ensureFrameResources(*frame,frame->rowCount)){frame->inFlight.store(false,std::memory_order_release);return false;}
            frame->tableSource=preparedFrame;
            std::memcpy(frame->resourceTable.contents,preparedFrame->resourceTable.contents,resourceEncoder.encodedLength);
            [resourceEncoder setArgumentBuffer:frame->resourceTable offset:0];
            [resourceEncoder setIndirectCommandBuffer:frame->icb atIndex:0];
        }else{
            if(!buildGpuTables(*frame,mode)){frame->inFlight.store(false,std::memory_order_release);return false;}
            preparedCommand=command;preparedEpoch=observationEpoch;preparedMode=mode;preparedFrame=frame;
        }
        const auto& tables=frame->tableSource?*frame->tableSource:*frame;
        std::memcpy(frame->faceBuffer.contents,faces.data(),sizeof(GpuFace)*6);
        // Materials/instances often share buffers and textures. Residency is a
        // resource set, not a draw list; declare each object once per encoder.
        std::vector<id<MTLResource>> residency{frame->icb,tables.drawBuffer,tables.boundsBuffer,frame->faceBuffer,frame->resourceTable,tables.materialTable};
        if(mode)residency.push_back(tables.clusterBuffer);
        residency.insert(residency.end(),tables.geometryResources.begin(),tables.geometryResources.end());
        auto blit=[command blitCommandEncoder];
        const bool optimizeCommands=selectedOptimizeMode();
        [blit resetCommandsInBuffer:frame->icb withRange:NSMakeRange(0,optimizeCommands?frame->icbCapacity:frame->rowCount*6)];[blit endEncoding];
        auto c=[command computeCommandEncoder];[c setComputePipelineState:mode?clusterCompute:compute];
        [c setBuffer:tables.drawBuffer offset:0 atIndex:0];[c setBuffer:tables.boundsBuffer offset:0 atIndex:1];
        [c setBuffer:frame->faceBuffer offset:0 atIndex:2];[c setBuffer:frame->resourceTable offset:0 atIndex:3];uint32_t casterCount=uint32_t(draws.size());[c setBytes:&casterCount length:sizeof(casterCount) atIndex:4];
        if(mode){[c setBuffer:tables.clusterBuffer offset:0 atIndex:5];[c setBytes:&frame->rowCount length:sizeof(frame->rowCount) atIndex:6];}
        [c useResource:frame->icb usage:MTLResourceUsageWrite];
        [c useResource:frame->resourceTable usage:MTLResourceUsageRead];
        [c useResource:tables.materialTable usage:MTLResourceUsageRead];
        [c useResources:residency.data()+6 count:residency.size()-6 usage:MTLResourceUsageRead];
        [c dispatchThreads:MTLSizeMake(frame->rowCount,6,1) threadsPerThreadgroup:MTLSizeMake(std::min<size_t>(32,frame->rowCount),1,1)];[c endEncoding];
        ++stats.gpuCullDispatches;stats.candidateCommands=frame->rowCount*6;
        if(optimizeCommands){
            auto optimize=[command blitCommandEncoder];
            for(unsigned face=0;face<6;++face)
                [optimize optimizeIndirectCommandBuffer:frame->icb withRange:NSMakeRange(face*frame->rowCount,frame->rowCount)];
            [optimize endEncoding];
        }
        for(unsigned face=0;face<6;++face){auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=cube;pass.depthAttachment.slice=face;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setViewport:MTLViewport{0,0,double(cube.width),double(cube.height),0,1}];[e setFrontFacingWinding:MTLWindingClockwise];[e setDepthStencilState:depthState];[e setDepthBias:0 slopeScale:1 clamp:0];[e useResources:residency.data() count:residency.size() usage:MTLResourceUsageRead stages:MTLRenderStageVertex|MTLRenderStageFragment];[e executeCommandsInBuffer:frame->icb withRange:NSMakeRange(face*frame->rowCount,frame->rowCount)];[e endEncoding];++stats.indirectFaceSubmissions;}
        if(mode)reportClusterHighWater(*frame);
        [command addCompletedHandler:^(id<MTLCommandBuffer> completed){(void)completed;frame->geometryResources.clear();frame->tableSource.reset();frame->inFlight.store(false,std::memory_order_release);}];
        return true;
    }

private:
    struct FrameResources {
        id<MTLIndirectCommandBuffer> icb=nil;
        id<MTLBuffer> resourceTable=nil,materialTable=nil,drawBuffer=nil,boundsBuffer=nil,faceBuffer=nil,clusterBuffer=nil;
        uint32_t rowCount{},icbCapacity{},clusterCapacity{};
        std::vector<id<MTLResource>> geometryResources;
        std::shared_ptr<FrameResources> tableSource;
        std::atomic_bool inFlight{false};
    };
    struct Stored { Draw draw{}; bool seen{false}; uint32_t nextAvailable{UINT32_MAX}; };
    struct SlotQueue { uint32_t head{UINT32_MAX}; uint32_t tail{UINT32_MAX}; };
    LifecycleModel model;
    std::vector<Stored> draws;
    std::unordered_map<Key,size_t,KeyHash> lookup;
    std::unordered_map<GeometryKey,SlotQueue,GeometryKeyHash> availableSlots;
    uint64_t scene_{},generation_{};bool dirty=true;Statistics stats{};
    id<MTLDevice> device_=nil;id<MTLComputePipelineState> compute=nil,clusterCompute=nil;
    id<MTLRenderPipelineState> opaque_=nil,cutout_=nil;
    id<MTLArgumentEncoder> resourceEncoder=nil;
    id<MTLArgumentEncoder> materialEncoder=nil;id<MTLDepthStencilState> depthState=nil;
    // The traversal submits at most eight point cubes per frame and the renderer
    // permits three frames in flight. Every cube needs a distinct mutable ICB
    // until its owning command buffer completes.
    std::array<std::shared_ptr<FrameResources>,24> framePool{};size_t frameCursor=0;
    uint64_t observationEpoch{},preparedEpoch{};
    id<MTLCommandBuffer> preparedCommand=nil;
    std::shared_ptr<FrameResources> preparedFrame;
    int probeClusterOverride=-1,preparedMode=-1,probeOptimizeOverride=-1;
    bool selectedOptimizeMode()const{
        if(probeOptimizeOverride>=0)return probeOptimizeOverride!=0;
        const char*value=getenv("STORM_PROBE_SHADOW_ICB_OPTIMIZE");
        return value&&std::strcmp(value,"1")==0;
    }
    struct LocalRange { simd_float4 minimum{},maximum{};uint32_t firstIndex{},indexCount{}; };
    struct ClusterKey {
        std::array<uint64_t,17> words{};
        bool operator==(const ClusterKey&) const = default;
    };
    struct ClusterKeyHash {
        size_t operator()(const ClusterKey& key)const noexcept{
            uint64_t h=1469598103934665603ull;for(auto word:key.words){h^=word;h*=1099511628211ull;}return size_t(h);
        }
    };
    std::unordered_map<ClusterKey,std::vector<LocalRange>,ClusterKeyHash> clusterCache;
    static constexpr size_t MaxClusterCacheBytes=8*1024*1024;
    static constexpr size_t MaxClusterCacheEntries=2048;
    // Include a deliberately generous node/allocator allowance, plus buckets
    // and the vector's actual capacity. Cache entries never escape into frames.
    static constexpr size_t CacheEntryCharge=sizeof(ClusterKey)+sizeof(std::vector<LocalRange>)+256;
    size_t clusterCacheBytes{},highWaterCacheBytes{},highWaterIcbBytes{};
    uint32_t highWaterRows{};

    int selectedClusterMode()const{
        if(probeClusterOverride>=0)return probeClusterOverride;
        const char* value=std::getenv("STORM_PROBE_SHADOW_CLUSTERS");
        return value&&std::strcmp(value,"128")==0?128:value&&std::strcmp(value,"512")==0?512:0;
    }
    void clearClusterCache(){clusterCache.clear();clusterCacheBytes=clusterCache.bucket_count()*sizeof(void*);}
    static bool finiteAffine(const simd_float4x4& world){
        for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)if(!std::isfinite(world.columns[c][r]))return false;
        return world.columns[0].w==0&&world.columns[1].w==0&&world.columns[2].w==0&&world.columns[3].w==1;
    }
    const std::vector<LocalRange>* localRanges(const Draw& d,int mode){
        if(!d.staticClusterEligible||d.skinPalette||d.primitive!=MTLPrimitiveTypeTriangle||!finiteAffine(d.world)||
           d.vertices.storageMode!=MTLStorageModeShared||d.indices.storageMode!=MTLStorageModeShared||
           !d.vertices.contents||!d.indices.contents||d.stride<12||d.indexCount%3||
           (d.indexType!=MTLIndexTypeUInt16&&d.indexType!=MTLIndexTypeUInt32)||
           d.vertexOffset>UINT32_MAX||d.indexOffset>UINT32_MAX||d.indexCount>UINT32_MAX)return nullptr;
        const size_t width=d.indexType==MTLIndexTypeUInt32?4:2;
        if(d.indexOffset%width||d.indexOffset>d.indices.length||d.indexCount>(d.indices.length-d.indexOffset)/width||
           d.indexCount>(UINT32_MAX-d.indexOffset)/width)return nullptr;
        ClusterKey key{{d.key.vertexIdentity,d.key.vertexRevision,d.key.indexIdentity,d.key.indexRevision,d.key.geometryState,
            uint64_t((uintptr_t)(__bridge void*)d.vertices),uint64_t((uintptr_t)(__bridge void*)d.indices),
            d.vertexOffset,d.indexOffset,d.indexCount,uint64_t(d.indexType),d.stride,uint64_t(uint32_t(d.baseVertex)),
            uint64_t(d.primitive),uint64_t(mode),d.vertices.length,d.indices.length}};
        const auto found=clusterCache.find(key);if(found!=clusterCache.end())return &found->second;
        const size_t rangeCount=(size_t(d.indexCount)+size_t(mode)*3-1)/(size_t(mode)*3);
        if(!rangeCount||rangeCount>MaxClusterRows||clusterCache.size()>=MaxClusterCacheEntries)return nullptr;
        if(clusterCache.empty()&&clusterCache.bucket_count()<MaxClusterCacheEntries){
            clusterCache.reserve(MaxClusterCacheEntries);clusterCacheBytes=clusterCache.bucket_count()*sizeof(void*);
        }
        const size_t charge=CacheEntryCharge+rangeCount*sizeof(LocalRange);
        if(clusterCacheBytes>MaxClusterCacheBytes||charge>MaxClusterCacheBytes-clusterCacheBytes)return nullptr;
        std::vector<LocalRange> ranges; ranges.reserve(rangeCount);
        const auto* indexBytes=static_cast<const uint8_t*>(d.indices.contents)+d.indexOffset;
        const auto* vertexBytes=static_cast<const uint8_t*>(d.vertices.contents);
        for(size_t first=0;first<d.indexCount;first+=size_t(mode)*3){
            LocalRange range;range.firstIndex=uint32_t(first);range.indexCount=uint32_t(std::min<size_t>(size_t(mode)*3,d.indexCount-first));
            simd_float3 minimum=simd_make_float3(INFINITY,INFINITY,INFINITY),maximum=-minimum;
            for(size_t n=first;n<first+range.indexCount;++n){
                uint32_t index=0;if(width==4)std::memcpy(&index,indexBytes+n*width,4);else{uint16_t small;std::memcpy(&small,indexBytes+n*width,2);index=small;}
                const int64_t vertex=int64_t(index)+int64_t(d.baseVertex);if(vertex<0)return nullptr;
                if(d.vertexOffset>d.vertices.length||d.vertices.length-d.vertexOffset<12||
                   uint64_t(vertex)>(d.vertices.length-d.vertexOffset-12)/d.stride)return nullptr;
                float p[3];std::memcpy(p,vertexBytes+d.vertexOffset+size_t(vertex)*d.stride,12);
                if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))return nullptr;
                const auto position=simd_make_float3(p[0],p[1],p[2]);minimum=simd_min(minimum,position);maximum=simd_max(maximum,position);
            }
            range.minimum=simd_make_float4(minimum,1);range.maximum=simd_make_float4(maximum,1);ranges.push_back(range);
        }
        const size_t actualCharge=CacheEntryCharge+ranges.capacity()*sizeof(LocalRange);
        if(actualCharge>MaxClusterCacheBytes-clusterCacheBytes)return nullptr;
        auto [inserted,success]=clusterCache.emplace(key,std::move(ranges));
        if(success)clusterCacheBytes+=actualCharge;
        return &inserted->second;
    }
    void buildClusterRows(std::vector<GpuCluster>& rows,int mode){
        rows.reserve(MaxClusterRows);
        // drawIndex is rebuilt only from the final compacted draw order.
        for(size_t i=0;i<draws.size();++i){
            const auto& d=draws[i].draw;const auto* ranges=localRanges(d,mode);
            const size_t remaining=draws.size()-i-1;
            if(ranges&&ranges->size()<=MaxClusterRows-rows.size()-remaining){
                for(const auto& range:*ranges)rows.push_back({range.minimum,range.maximum,uint32_t(i),range.firstIndex,range.indexCount,1});
            }else rows.push_back({{}, {},uint32_t(i),0,uint32_t(d.indexCount),0});
        }
    }
    void reportClusterHighWater(const FrameResources& frame){
        const size_t bytes=frame.icb.allocatedSize;
        if(frame.rowCount<=highWaterRows&&clusterCacheBytes<=highWaterCacheBytes&&bytes<=highWaterIcbBytes)return;
        highWaterRows=std::max(highWaterRows,frame.rowCount);highWaterCacheBytes=std::max(highWaterCacheBytes,clusterCacheBytes);highWaterIcbBytes=std::max(highWaterIcbBytes,bytes);
        fprintf(stderr,"[StormMetal] point clusters high-water rows=%u cachedBytes=%zu icbAllocatedBytes=%zu icbCommands=%u\n",highWaterRows,highWaterCacheBytes,highWaterIcbBytes,frame.icbCapacity);
    }

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

    bool ensureFrameResources(FrameResources& frame,uint32_t rowCount){
        if(!frame.faceBuffer)frame.faceBuffer=[device_ newBufferWithLength:sizeof(GpuFace)*6 options:MTLResourceStorageModeShared];
        const uint32_t required=std::max(MaxCasters,rowCount)*6;
        // Called only after this completion-owned slot was acquired as free.
        if(frame.icbCapacity<required){auto descriptor=[MTLIndirectCommandBufferDescriptor new];descriptor.commandTypes=MTLIndirectCommandTypeDrawIndexed;descriptor.inheritPipelineState=NO;descriptor.inheritBuffers=NO;descriptor.maxVertexBufferBindCount=5;descriptor.maxFragmentBufferBindCount=1;frame.icb=[device_ newIndirectCommandBufferWithDescriptor:descriptor maxCommandCount:required options:MTLResourceStorageModePrivate];frame.icbCapacity=frame.icb?required:0;}
        if(!frame.resourceTable)frame.resourceTable=[device_ newBufferWithLength:resourceEncoder.encodedLength options:MTLResourceStorageModeShared];
        return frame.faceBuffer&&frame.icb&&frame.resourceTable;
    }

    bool buildGpuTables(FrameResources& frame,int mode){
        std::vector<GpuCluster> clusters;if(mode)buildClusterRows(clusters,mode);
        frame.rowCount=mode?uint32_t(clusters.size()):uint32_t(draws.size());
        std::vector<GpuDraw> gpuDraws(draws.size());std::vector<GpuBounds> bounds(draws.size());
        for(size_t i=0;i<draws.size();++i){const auto&d=draws[i].draw;gpuDraws[i].vertexOffset=uint32_t(d.vertexOffset);gpuDraws[i].indexOffset=uint32_t(d.indexOffset);gpuDraws[i].indexCount=uint32_t(d.indexCount);gpuDraws[i].indexType=d.indexType==MTLIndexTypeUInt32;gpuDraws[i].materialIndex=uint32_t(i);gpuDraws[i].primitive=uint32_t(d.primitive);gpuDraws[i].cull=uint32_t(d.cull);gpuDraws[i].stride=d.stride;gpuDraws[i].uvOffset=d.uvOffset;gpuDraws[i].colorOffset=d.colorOffset;gpuDraws[i].useColorAlpha=d.useColorAlpha;gpuDraws[i].skinned=d.skinPalette!=nil;gpuDraws[i].boneCount=d.boneCount;gpuDraws[i].baseVertex=d.baseVertex;gpuDraws[i].alpha={d.alphaReference,float(d.alphaFunction),float(d.alphaTest),0};gpuDraws[i].world=d.world;bounds[i]={simd_make_float4(d.bounds.minimum,1),simd_make_float4(d.bounds.maximum,1)};}
        if(!frame.drawBuffer)frame.drawBuffer=[device_ newBufferWithLength:MaxCasters*sizeof(GpuDraw) options:MTLResourceStorageModeShared];
        if(!frame.boundsBuffer)frame.boundsBuffer=[device_ newBufferWithLength:MaxCasters*sizeof(GpuBounds) options:MTLResourceStorageModeShared];
        if(!frame.drawBuffer||!frame.boundsBuffer||!ensureFrameResources(frame,frame.rowCount))return false;
        if(mode){
            if(frame.clusterCapacity<frame.rowCount){frame.clusterBuffer=[device_ newBufferWithLength:frame.rowCount*sizeof(GpuCluster) options:MTLResourceStorageModeShared];frame.clusterCapacity=frame.clusterBuffer?frame.rowCount:0;}
            if(!frame.clusterBuffer)return false;
            std::memcpy(frame.clusterBuffer.contents,clusters.data(),clusters.size()*sizeof(GpuCluster));
        }
        std::memcpy(frame.drawBuffer.contents,gpuDraws.data(),gpuDraws.size()*sizeof(GpuDraw));
        std::memcpy(frame.boundsBuffer.contents,bounds.data(),bounds.size()*sizeof(GpuBounds));
        if(!frame.materialTable)frame.materialTable=[device_ newBufferWithLength:materialEncoder.encodedLength options:MTLResourceStorageModeShared];
        if(!frame.icb||!frame.resourceTable||!frame.materialTable)return false;
        [materialEncoder setArgumentBuffer:frame.materialTable offset:0];
        [resourceEncoder setArgumentBuffer:frame.resourceTable offset:0];
        [resourceEncoder setIndirectCommandBuffer:frame.icb atIndex:0];
        [resourceEncoder setRenderPipelineState:opaque_ atIndex:1];
        [resourceEncoder setRenderPipelineState:cutout_ atIndex:2];
        [resourceEncoder setBuffer:frame.materialTable offset:0 atIndex:3];
        for(size_t i=0;i<draws.size();++i){const auto&d=draws[i].draw;[materialEncoder setTexture:d.alphaTexture atIndex:i];[resourceEncoder setBuffer:d.vertices offset:0 atIndex:4+i];[resourceEncoder setBuffer:d.indices offset:0 atIndex:4+MaxCasters+i];[resourceEncoder setBuffer:d.skinPalette offset:d.skinPaletteOffset atIndex:4+MaxCasters*2+i];}
        frame.geometryResources.clear();frame.geometryResources.reserve(draws.size()*4);
        for(const auto&item:draws){frame.geometryResources.push_back(item.draw.vertices);frame.geometryResources.push_back(item.draw.indices);if(item.draw.alphaTexture)frame.geometryResources.push_back(item.draw.alphaTexture);if(item.draw.skinPalette)frame.geometryResources.push_back(item.draw.skinPalette);}
        std::sort(frame.geometryResources.begin(),frame.geometryResources.end(),std::less<id<MTLResource>>{});
        frame.geometryResources.erase(std::unique(frame.geometryResources.begin(),frame.geometryResources.end()),frame.geometryResources.end());
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
struct Cluster {float4 minimum,maximum;uint drawIndex,firstIndex,indexCount,flags;};
bool clusterOutside(Cluster cluster,float4x4 world,float4x4 faceVP){
 // Each clip component is a sum of 16 products (two four-term dots),
 // whether evaluated as (F*W)*L or F*(W*L). 64*epsilon exceeds the
 // operation/association roundoff bound, including both plane terms;
 // the absolute floor admits subnormal/near-plane numerical uncertainty.
 float4x4 magnitudeWorld=float4x4(abs(world[0]),abs(world[1]),abs(world[2]),abs(world[3]));
 float4x4 magnitudeFace=float4x4(abs(faceVP[0]),abs(faceVP[1]),abs(faceVP[2]),abs(faceVP[3]));
 float4x4 combined=faceVP*world;
 bool outside[6]={true,true,true,true,true,true};
 for(uint corner=0;corner<8;corner++){
  float4 local=float4(corner&1?cluster.maximum.x:cluster.minimum.x,corner&2?cluster.maximum.y:cluster.minimum.y,corner&4?cluster.maximum.z:cluster.minimum.z,1);
  float4 clip=combined*local;
  float4 magnitude=magnitudeFace*(magnitudeWorld*abs(local));
  float4 error=magnitude*(64.f*1.1920928955078125e-7f)+float4(1.e-6f);
  if(!all(isfinite(clip))||!all(isfinite(magnitude))||!all(isfinite(error)))return false;
  outside[0]&=clip.x+clip.w < -(error.x+error.w);
  outside[1]&=clip.x-clip.w > error.x+error.w;
  outside[2]&=clip.y+clip.w < -(error.y+error.w);
  outside[3]&=clip.y-clip.w > error.y+error.w;
  outside[4]&=clip.z < -error.z;
  outside[5]&=clip.z-clip.w > error.z+error.w;
 }
 for(uint plane=0;plane<6;plane++)if(outside[plane])return true;
 return false;
}
kernel void point_cluster_encode(const device Draw*draws[[buffer(0)]],const device Bounds*bounds[[buffer(1)]],constant Face*faces[[buffer(2)]],constant Resources&resources[[buffer(3)]],constant uint&casterCount[[buffer(4)]],const device Cluster*clusters[[buffer(5)]],constant uint&clusterCount[[buffer(6)]],uint2 tid[[thread_position_in_grid]]){
 uint clusterIndex=tid.x,face=tid.y;if(clusterIndex>=clusterCount||face>=6)return;
 Cluster cluster=clusters[clusterIndex];uint drawIndex=cluster.drawIndex;if(drawIndex>=casterCount)return;
 Draw d=draws[drawIndex];Bounds b=bounds[drawIndex];float3 p=faces[face].lightRange.xyz;float3 q=clamp(p,b.minimum.xyz,b.maximum.xyz);if(distance_squared(p,q)>faces[face].lightRange.w*faces[face].lightRange.w)return;
 // Retain both original whole-draw tests. The sphere is deliberately never
 // tightened to a cluster: the reference rasterizes cube-face depth beyond it.
 bool outside[6]={true,true,true,true,true,true};for(uint corner=0;corner<8;corner++){float3 w=float3(corner&1?b.maximum.x:b.minimum.x,corner&2?b.maximum.y:b.minimum.y,corner&4?b.maximum.z:b.minimum.z);float4 clip=faces[face].viewProjection*float4(w,1);outside[0]&=clip.x < -clip.w;outside[1]&=clip.x > clip.w;outside[2]&=clip.y < -clip.w;outside[3]&=clip.y > clip.w;outside[4]&=clip.z < 0;outside[5]&=clip.z > clip.w;}for(uint plane=0;plane<6;plane++)if(outside[plane])return;
 if((cluster.flags&1u)&&clusterOutside(cluster,d.world,faces[face].viewProjection))return;
 render_command command(resources.commands,face*clusterCount+clusterIndex);command.set_render_pipeline_state(d.alpha.z?resources.cutout:resources.opaque);command.set_vertex_buffer(resources.vertices[drawIndex],d.vertexOffset,0);command.set_vertex_buffer(draws+drawIndex,0,1);command.set_vertex_buffer(faces+face,0,2);command.set_vertex_buffer(&resources,0,4);if(d.alpha.z)command.set_fragment_buffer(resources.materials,3);
 uint indexCount=(cluster.flags&1u)?cluster.indexCount:d.indexCount;
 uint indexOffset=d.indexOffset+((cluster.flags&1u)?cluster.firstIndex*(d.indexType?4u:2u):0u);
 if(d.indexType)command.draw_indexed_primitives(primitive_type::triangle,indexCount,(device uint*)(resources.indices[drawIndex]+indexOffset),1,d.baseVertex,0);else command.draw_indexed_primitives(primitive_type::triangle,indexCount,(device ushort*)(resources.indices[drawIndex]+indexOffset),1,d.baseVertex,0);
}
)MSL";
};

} // namespace storm_metal::point_shadow

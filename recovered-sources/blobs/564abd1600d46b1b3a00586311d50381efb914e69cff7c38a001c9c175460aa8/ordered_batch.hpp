// Private native backend prototype. Included once for types and once in Device.
#ifndef STORM_ORDERED_BATCH_DEVICE
#include "ordered_indices.hpp"
#include <set>
namespace ordered_batch {
struct Table {
 MTLGPUAddress vertices,uniform,raw,sampling,fog;
 int32_t baseVertex;uint32_t padding;
 MTLResourceID textures[8],samplers[8],cubes[8];
};
static_assert(sizeof(Table)==240 && offsetof(Table,baseVertex)==40);
static_assert(offsetof(Table,textures)==48 && offsetof(Table,samplers)==112 && offsetof(Table,cubes)==176);
struct Counters {
 uint64_t considered=0,admitted=0,batches=0,originals=0,indices=0,indexUploads=0,indexBytes=0,cacheHits=0,genericPackets=0,barriers=0;
 uint64_t reject[12]{};
 ~Counters(){fprintf(stderr,"[StormMetal ordered] considered=%llu admitted=%llu batches=%llu originals=%llu indices=%llu uploads=%llu bytes=%llu hits=%llu generic=%llu barriers=%llu rejects=",considered,admitted,batches,originals,indices,indexUploads,indexBytes,cacheHits,genericPackets,barriers);for(auto n:reject)fprintf(stderr,"%llu,",n);fprintf(stderr,"\n");}
};
// Binding-only derivation is generated below from the actual shader owners.
inline std::string source(std::string original);
}
#else
 bool orderedEnabled=[](){const char*v=getenv("STORM_METAL_ORDERED_BATCH");return v&&strcmp(v,"1")==0;}();
 bool orderedFailed=false,orderedFlushing=false,orderedSkyFog=false;
 id<MTLLibrary> orderedLibrary=nil;
 ordered_batch::Counters orderedStats;
 ordered_indices::Cache orderedTopology;
 struct OrderedDraw {
  std::array<uint32_t,256> renderState;storm_render::Viewport viewport;uint32_t vertexFormat;
  Uniform uniform;RawLandUniform raw;LandShadow::Sampling sampling;storm_metal::SkyFogDraw fog;
  id<MTLBuffer> vertices,indices;NSUInteger vertexOffset,indexOffset;int32_t base;uint32_t count;MTLIndexType indexType;
  uint64_t indexIdentity,indexRevision;std::shared_ptr<const std::vector<uint8_t>> source;
  std::array<id<MTLTexture>,16> textures{};std::array<id<MTLSamplerState>,8> samplers{};
  std::array<id<MTLTexture>,12> shared{};
 };
 std::vector<OrderedDraw> orderedDraws;
 struct OrderedResident {id<MTLBuffer> buffer;uint64_t used;};
 std::map<std::shared_ptr<const ordered_indices::Indices>,OrderedResident,std::owner_less<std::shared_ptr<const ordered_indices::Indices>>> orderedResident;
 uint64_t orderedClock=0;size_t orderedResidentBytes=0;
 std::array<std::vector<id>,FrameFlightRing::count> orderedRetained;
 bool orderedFogActive()const{return cloudFieldCurrent()&&(rs[storm_render::RenderFog]||skyFogActive||(localWeather.current&&rs[storm_render::RenderDepthTest]&&!interfaceBackSceneActive));}
 bool prepareOrdered(){
  if(!orderedEnabled||orderedFailed)return false;
  if(metal.argumentBuffersSupport!=MTLArgumentBuffersTier2){++orderedStats.reject[0];return false;}
  if(!orderedLibrary){NSError*error=nil;auto src=ordered_batch::source(materialShaderSource);if(src.empty()){orderedFailed=true;++orderedStats.reject[1];return false;}
   orderedLibrary=[metal newLibraryWithSource:[NSString stringWithUTF8String:src.c_str()] options:nil error:&error];
   if(!orderedLibrary){orderedFailed=true;++orderedStats.reject[1];fprintf(stderr,"[StormMetal ordered] shader fallback: %s\n",error.localizedDescription.UTF8String);return false;}}
  if(orderedFogActive()&&!prepareLocalWeather()){++orderedStats.reject[2];return false;}
  return true;
 }
 // Keep the exact SkyFogDraw bytes produced by the ordinary binding owner.
 storm_metal::SkyFogDraw orderedFogSnapshot(){
  simd_float4x4 view,projection;memcpy(&view,&matrices[storm_render::TransformView],64);memcpy(&projection,&matrices[storm_render::TransformProjection],64);
  auto draw=storm_metal::makeSkyFogDraw(simd_inverse(simd_mul(projection,view)),screenViewport(),cloudFieldCurrent(),skyFogActive);
  draw.options.z=localWeather.current;auto absolute=matrix_identity_float4x4;absolute.columns[3]=simd_make_float4(-landShadow.worldOrigin,1);
  draw.volumeClipFromWorld=simd_mul(localWeather.clipFromWorld,absolute);draw.volumeCameraRange=simd_make_float4(localWeather.cameraRange.xyz+landShadow.worldOrigin,localWeather.cameraRange.w);draw.volumeAngles=localWeather.angles;
  draw.options.w=rs[storm_render::RenderBlend]?(rs[storm_render::RenderDestinationBlend]==storm_render::BlendOne?1.f:rs[storm_render::RenderSourceBlend]==storm_render::BlendOne&&rs[storm_render::RenderDestinationBlend]==storm_render::BlendInverseSourceAlpha?2.f:0.f):0.f;
  draw.options.z*=float(cloudFieldCurrent()&&!(landShadow.locationActive&&landShadow.indoor));return draw;
 }
 bool enqueueOrdered(const Uniform&u,const RawLandUniform&r,const LandShadow::Sampling&s,id<MTLBuffer>vertices,NSUInteger vo,id<MTLBuffer>indices,NSUInteger io,int32_t base,uint32_t count,MTLIndexType type){
  const size_t width=type==MTLIndexTypeUInt16?2:4;
  if(!count||count%3||io>ib->bytes.size()||size_t(count)*width>ib->bytes.size()-io){++orderedStats.reject[3];return false;}
  if(type==MTLIndexTypeUInt32){for(uint32_t i=0;i<count;i++){uint32_t index;memcpy(&index,ib->bytes.data()+io+size_t(i)*4,4);if(index>0xfffffu){++orderedStats.reject[4];return false;}}}
  size_t references=count;for(const auto&draw:orderedDraws)references+=draw.count;
  if(orderedDraws.size()==4096||references>32u*1024u*1024u/4u)flushOrdered();
  if(!orderedDraws.empty()){const auto&previous=orderedDraws.back();bool changed=memcmp(&previous.viewport,&vp,sizeof(vp));for(auto field:{storm_render::RenderBlend,storm_render::RenderSourceBlend,storm_render::RenderDestinationBlend,storm_render::RenderSeparateAlphaBlend,storm_render::RenderSourceBlendAlpha,storm_render::RenderDestinationBlendAlpha,storm_render::RenderBlendFactor,storm_render::RenderDepthTest,storm_render::RenderDepthWrite,storm_render::RenderDepthCompare,storm_render::RenderCullMode,storm_render::RenderDepthBias,storm_render::RenderSlopeScaleDepthBias})changed|=previous.renderState[field]!=rs[field];if(changed)flushOrdered();}
  std::array<id<MTLTexture>,12> shared{};shared[0]=landShadow.map;shared[1]=landShadow.farMap;for(unsigned i=0;i<8;i++)shared[i+2]=landShadow.prepass.points[i].map;shared[10]=volumetricSky.fogEnvironment;shared[11]=localWeather.transport?localWeather.transport:volumetricSky.shape;
  const bool fog=orderedFogActive();
  if(!orderedDraws.empty()&&(orderedSkyFog!=fog||orderedDraws.back().shared!=shared))flushOrdered();
  if(!encoder)begin(); // An encoder exists before deferred packets own this pass.
  OrderedDraw p{};p.uniform=u;p.raw=r;p.sampling=s;p.fog=orderedFogSnapshot();p.vertices=vertices;p.indices=indices;p.vertexOffset=vo;p.indexOffset=io;p.base=base;p.count=count;p.indexType=type;p.indexIdentity=ib->cacheIdentity;p.indexRevision=ib->cacheRevision;p.shared=shared;
  // The topology adapter reads immutable captured bytes, never a borrowed IB
  // after SetIndices/Release or Lock/Unlock has changed its CPU owner.
  // Copy once per source revision, not per packet (see orderedIndexSources).
  for(unsigned i=0;i<8;i++){p.textures[i]=boundTextures[i].flat?boundTextures[i].flat->gpu():nil;p.textures[8+i]=boundTextures[i].cube?boundTextures[i].cube->gpu():nil;p.samplers[i]=samplerFor(i);if(!p.samplers[i]){++orderedStats.reject[5];return false;}}
  const auto key=std::make_pair(p.indexIdentity,p.indexRevision);
  if(!orderedIndexSources.count(key))orderedIndexSources[key]=std::make_shared<const std::vector<uint8_t>>(ib->bytes);
  p.source=orderedIndexSources.at(key);p.renderState=rs;p.viewport=vp;p.vertexFormat=fvf;
  orderedDraws.push_back(std::move(p));orderedSkyFog=fog;++orderedStats.admitted;++orderedStats.genericPackets;return true;
 }
 std::map<std::pair<uint64_t,uint64_t>,std::shared_ptr<const std::vector<uint8_t>>> orderedIndexSources;
 void orderedBindShared(const OrderedDraw&p){for(unsigned i=0;i<12;i++)[encoder setFragmentTexture:p.shared[i] atIndex:16+i];}
 void orderedOriginals(){
  for(const auto&p:orderedDraws){
   auto result=applyState(orderedSkyFog?0x72001ull:0x72000ull,orderedLibrary,@"vs_landraw",orderedSkyFog?@"fs_landlit_skyfog":@"fs_landlit");if(storm_render::failed(result)){fprintf(stderr,"[StormMetal ordered] original replay pipeline failed\n");continue;}
   [encoder setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];bindFixedFunction(p.uniform);[encoder setVertexBytes:&p.raw length:sizeof(p.raw) atIndex:3];bindLandSampling(p.sampling,4);
   for(unsigned i=0;i<16;i++)[encoder setFragmentTexture:p.textures[i] atIndex:i];for(unsigned i=0;i<8;i++)[encoder setFragmentSamplerState:p.samplers[i] atIndex:i];orderedBindShared(p);if(orderedSkyFog)[encoder setFragmentBytes:&p.fog length:sizeof(p.fog) atIndex:9];
   [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:p.count indexType:p.indexType indexBuffer:p.indices indexBufferOffset:p.indexOffset instanceCount:1 baseVertex:p.base baseInstance:0];++orderedStats.originals;
  }
 }
 void flushOrdered(){
  if(orderedDraws.empty()||orderedFlushing)return;orderedFlushing=true;++orderedStats.barriers;
  // Bridge/external helpers can write raster fields directly. Encoding uses the
  // captured run state, not mutable state belonging to the following draw.
  const auto savedState=rs;const auto savedViewport=vp;const auto savedFVF=fvf;
  rs=orderedDraws.front().renderState;vp=orderedDraws.front().viewport;fvf=orderedDraws.front().vertexFormat;
  std::vector<ordered_indices::Source> sources;sources.reserve(orderedDraws.size());for(const auto&p:orderedDraws){sources.push_back({p.indexIdentity,p.indexRevision,p.indexOffset,p.count,p.indexType==MTLIndexTypeUInt16?2u:4u,*p.source});}
  auto topology=orderedTopology.get(sources);id<MTLBuffer> indices=nil;
  if(topology.indices){auto found=orderedResident.find(topology.indices);if(found!=orderedResident.end()){indices=found->second.buffer;found->second.used=++orderedClock;++orderedStats.cacheHits;}else{
   const size_t bytes=topology.indices->size()*4;indices=[metal newBufferWithBytes:topology.indices->data() length:bytes options:MTLResourceStorageModeShared];
   if(indices&&profile.enabled){++profile.buffers;profile.bytes+=bytes;}
   if(indices){while(orderedResidentBytes+bytes>32u*1024u*1024u&&!orderedResident.empty()){auto oldest=std::min_element(orderedResident.begin(),orderedResident.end(),[](const auto&a,const auto&b){return a.second.used<b.second.used;});orderedResidentBytes-=oldest->second.buffer.length;orderedResident.erase(oldest);}orderedResident.emplace(topology.indices,OrderedResident{indices,++orderedClock});orderedResidentBytes+=bytes;++orderedStats.indexUploads;orderedStats.indexBytes+=bytes;}
  }}
  static_assert(sizeof(Uniform)==1328&&sizeof(RawLandUniform)==1280&&sizeof(LandShadow::Sampling)==3488&&sizeof(storm_metal::SkyFogDraw)==192);
  const uint64_t previousArenaAllocations=frameArena.allocations;
  const size_t count=orderedDraws.size();auto table=frameArena.allocate(metal,count*sizeof(ordered_batch::Table));
  auto metadata=frameArena.allocate(metal,count*(1536+1280+3584+256));bool okay=indices&&table.buffer&&metadata.buffer;
  // Count only new Metal blocks, not every arena slice or reused cache hit.
  if(profile.enabled)profile.buffers+=frameArena.allocations-previousArenaAllocations;
  std::vector<id<MTLResource>> resources;std::set<void*> seen;auto retain=[&](id value){if(!value)return;orderedRetained[frameArena.active].push_back(value);};
  auto resident=[&](id<MTLResource>value){if(value&&seen.insert((__bridge void*)value).second){resources.push_back(value);retain(value);}};
  if(okay){auto*entries=static_cast<ordered_batch::Table*>(table.data);NSUInteger at=metadata.offset;auto*destination=static_cast<uint8_t*>(metadata.data);
   for(size_t i=0;i<count;i++){const auto&p=orderedDraws[i];auto&t=entries[i];t={};t.vertices=p.vertices.gpuAddress+p.vertexOffset;t.baseVertex=p.base;
    auto copy=[&](const void*data,size_t length,size_t padded){const auto address=metadata.buffer.gpuAddress+at;memcpy(destination,data,length);destination+=padded;at+=padded;return address;};
    t.uniform=copy(&p.uniform,sizeof(p.uniform),1536);t.raw=copy(&p.raw,sizeof(p.raw),1280);t.sampling=copy(&p.sampling,sizeof(p.sampling),3584);t.fog=copy(&p.fog,sizeof(p.fog),256);
    resident(p.vertices);for(unsigned j=0;j<8;j++){t.textures[j]=p.textures[j].gpuResourceID;t.cubes[j]=p.textures[j+8].gpuResourceID;t.samplers[j]=p.samplers[j].gpuResourceID;resident(p.textures[j]);resident(p.textures[j+8]);retain(p.samplers[j]);}for(auto value:p.shared)resident(value);
   }
   // Upload accounting follows actual initialized bytes; arena alignment gaps
   // reserve memory but are not uploaded. Count these even if PSO fallback runs.
   if(profile.enabled)profile.bytes+=count*(sizeof(ordered_batch::Table)+sizeof(Uniform)+sizeof(RawLandUniform)+sizeof(LandShadow::Sampling)+sizeof(storm_metal::SkyFogDraw));
   resident(metadata.buffer);resident(table.buffer);resident(indices);
   auto result=applyState(orderedSkyFog?0x71001ull:0x71000ull,orderedLibrary,@"vs_landraw_batch",orderedSkyFog?@"fs_landlit_skyfog_batch":@"fs_landlit_batch");okay=!storm_render::failed(result);
   if(okay){orderedBindShared(orderedDraws.front());[encoder useResources:resources.data() count:resources.size() usage:MTLResourceUsageRead stages:MTLRenderStageVertex|MTLRenderStageFragment];[encoder setVertexBuffer:table.buffer offset:table.offset atIndex:10];[encoder setFragmentBuffer:table.buffer offset:table.offset atIndex:10];[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:topology.indices->size() indexType:MTLIndexTypeUInt32 indexBuffer:indices indexBufferOffset:0];++orderedStats.batches;orderedStats.indices+=topology.indices->size();}
  }
  if(!okay){++orderedStats.reject[topology.indices?7:6];orderedOriginals();}
  orderedDraws.clear();rs=savedState;vp=savedViewport;fvf=savedFVF;encoderBindings={};activeMaterialPipeline=nullptr;activeMaterialGeneric=nil;orderedFlushing=false;
  // Bound immutable CPU snapshots; packets themselves retain active revisions.
  size_t retained=0;for(const auto&entry:orderedIndexSources)retained+=entry.second->size();
  while(retained>32u*1024u*1024u&&!orderedIndexSources.empty()){auto entry=orderedIndexSources.begin();retained-=entry->second->size();orderedIndexSources.erase(entry);}
 }
#endif

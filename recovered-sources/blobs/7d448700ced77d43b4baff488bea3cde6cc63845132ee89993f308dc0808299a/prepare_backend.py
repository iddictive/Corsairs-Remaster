"""Generate one private backend from current owning sources; no compilation/GPU.

Run from any directory. Root may compile backend.mm with the ordinary backend
compile command, adding -I<experiments/native-metal> and this directory. The
same resulting binary selects Off/On using STORM_METAL_ORDERED_BATCH=0|1.
"""
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
ROOT = next(p for p in HERE.parents if (p / "experiments/native-metal/backend.mm").is_file())
SRC = ROOT / "experiments/native-metal"
original = (SRC / "backend.mm").read_text()
shadow = (SRC / "land_shadow.hpp").read_text()
resources = (SRC / "resources.hpp").read_text()
backend = original
changes = []


def once(old, new):
    global backend
    assert backend.count(old) == 1, (old[:120], backend.count(old))
    backend = backend.replace(old, new)
    changes.append(hashlib.sha256((old + "\0" + new).encode()).hexdigest())


def function(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    level, end = 1, brace + 1
    while level:
        level += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


table = '''
struct OrderedPacket {
 const device uchar* vertices [[id(0)]];constant U* uniform [[id(1)]];
 constant RawLandU* raw [[id(2)]];constant LandSampling* sampling [[id(3)]];
 constant SkyFogDraw* fog [[id(4)]];int baseVertex [[id(5)]];uint padding [[id(6)]];
 array<texture2d<float>,8> t [[id(7)]];array<sampler,8> sam [[id(15)]];array<texturecube<float>,8> cubes [[id(23)]];
};
'''
vertex = function(original, "vertex O vs_landraw(")
body = vertex[vertex.index("{")+1:]
assert body.count("O o;o.p=u.m*") == 1
vertex = '''vertex O vs_landraw_batch(uint encoded [[vertex_id]],constant OrderedPacket*packets [[buffer(10)]]) {
 uint packetID=encoded>>20;constant OrderedPacket&packet=packets[packetID];
 uint i=uint(int(encoded&0xfffffu)+packet.baseVertex);const device uchar*bytes=packet.vertices;
 constant U&u=*packet.uniform;constant RawLandU&r=*packet.raw;constant LandSampling&s=*packet.sampling;
''' + body.replace("O o;o.p=u.m*", "O o;o.packetID=packetID;o.p=u.m*")
fragments = []
for fog in (False, True):
    name = "fs_landlit_skyfog" if fog else "fs_landlit"
    fragment = function(shadow, "fragment float4 " + name + "(")
    signature, body = fragment[:fragment.index("{")], fragment[fragment.index("{")+1:]
    signature = signature.replace(name + "(", name + "_batch(")
    for old in ("constant U&u [[buffer(1)]],", "constant LandSampling&s [[buffer(2)]],", "array<texture2d<float>,8>t [[texture(0)]],", "array<sampler,8>sam [[sampler(0)]],", "array<texturecube<float>,8>cubes [[texture(8)]],", "constant SkyFogDraw&fogDraw [[buffer(9)]],"):
        if old in signature:
            assert signature.count(old) == 1
            signature = signature.replace(old, "")
    signature = signature.replace("O v [[stage_in]],", "O v [[stage_in]],constant OrderedPacket*packets [[buffer(10)]],")
    assert body.count("v,u,s,t,sam,cubes,") == 1
    fragments.append(signature + "{constant OrderedPacket&packet=packets[v.packetID];constant U&u=*packet.uniform;constant LandSampling&s=*packet.sampling;constant SkyFogDraw&fogDraw=*packet.fog;" + body.replace("v,u,s,t,sam,cubes,", "v,u,s,packet.t,packet.sam,packet.cubes,"))
suffix = table + vertex + "\n" + "\n".join(fragments)
resource_candidate = resources
assert resource_candidate.count("namespace sm {") == 2
resource_candidate = resource_candidate.replace("namespace sm {", "namespace sm {\nvoid orderedResourceWrite(storm_render::Device*,id<MTLResource>);", 1)
assert resource_candidate.count(" void upload(){") == 1
resource_candidate = resource_candidate.replace(" void upload(){", " void upload(){if(owner)orderedResourceWrite(owner,gpu);")
assert resource_candidate.count("[gpu replaceRegion:MTLRegionMake3D") == 1
resource_candidate = resource_candidate.replace("[gpu replaceRegion:MTLRegionMake3D", "if(owner)orderedResourceWrite(owner,gpu);[gpu replaceRegion:MTLRegionMake3D")
once('#include "resources.hpp"', resource_candidate)
source_function = '''
namespace ordered_batch {inline std::string source(std::string original){
 const std::string needle="struct O { ";auto at=original.find(needle);if(at==std::string::npos||original.find(needle,at+1)!=std::string::npos)return {};
 original.replace(at,needle.size(),"struct O { uint packetID [[flat]]; ");return original+R"ORDEREDMSL(
''' + suffix + ''')ORDEREDMSL";}}
'''
once("using namespace sm;", '#include "ordered_batch.hpp"\n' + source_function + "\nusing namespace sm;")
once("struct Device : storm_render::Device {", '''struct Device : storm_render::Device {
#define STORM_ORDERED_BATCH_DEVICE 1
#include "ordered_batch.hpp"
#undef STORM_ORDERED_BATCH_DEVICE
''')
once("void finish(bool finalizeShadowRegistry=false){if(encoder)", "void finish(bool finalizeShadowRegistry=false){flushOrdered();if(encoder)")
once("void clearBindings(){if(drained)return;drained=true;synchronize();", "void clearBindings(){if(drained)return;drained=true;synchronize();orderedResident.clear();orderedResidentBytes=0;orderedIndexSources.clear();orderedLibrary=nil;")
once("void retainDeviceResource(storm_render::Device* d){", '''void orderedResourceWrite(storm_render::Device* owner,id<MTLResource> resource){
 if(!owner||!resource)return;auto*d=static_cast<Device*>(owner);if(!d->orderedEnabled)return;
 bool referenced=false;for(const auto&p:d->orderedDraws){for(auto value:p.textures)referenced|=value==resource;for(auto value:p.shared)referenced|=value==resource;}
 for(const auto&slot:d->orderedRetained)for(id value:slot)referenced|=value==resource;
 if(referenced)d->synchronize();
}
void retainDeviceResource(storm_render::Device* d){''')
once("frameArena.rewindAfterCompletion(slot,flight.command);flight={};", "frameArena.rewindAfterCompletion(slot,flight.command);orderedRetained[slot].clear();flight={};")
once(" if(++p.frames<120)return;", " if(++p.frames<120)return;\n if(orderedEnabled)orderedStats.report();")
once("auto sampler=[MTLSamplerDescriptor new];sampler.minFilter", "auto sampler=[MTLSamplerDescriptor new];sampler.supportArgumentBuffers=YES;sampler.minFilter")
once("Uniform prepareFixedFunction(){", "Uniform prepareFixedFunction(bool bindResources=true){")
once("if(preparedMaterialsEnabled){\n for(unsigned i=0;i<16;i++)bindMaterialTexture", "if(bindResources&&preparedMaterialsEnabled){\n for(unsigned i=0;i<16;i++)bindMaterialTexture")
once("}else{\n [encoder setFragmentTextures:shaderTextures", "}else if(bindResources){\n [encoder setFragmentTextures:shaderTextures")
once("storm_render::Result applyState(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){", "storm_render::Result applyState(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){\n  flushOrdered();")
once("d.fragmentFunction=fixedFunctionMaterial?", "d.fragmentFunction=(fixedFunctionMaterial||shaderLibrary==orderedLibrary)?")
once("storm_render::Result applyStateLegacy(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){ begin();", "storm_render::Result applyStateLegacy(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){flushOrdered();begin();")
once("void begin(bool clear=false,uint32_t flags=0,uint32_t c=0,float z=1){prepareSceneFrame();", "void begin(bool clear=false,uint32_t flags=0,uint32_t c=0,float z=1){if(clear)flushOrdered();prepareSceneFrame();")
once("SetViewport(const storm_render::Viewport*p)override{vp=*p;", "SetViewport(const storm_render::Viewport*p)override{if(memcmp(&vp,p,sizeof(vp)))flushOrdered();vp=*p;")
# Only raster/global render state splits a material run. Material fog/alpha/light
# values are captured by Uniform/RawLandUniform, so they remain heterogeneous.
once("if(s>=256)return storm_render::result::invalidCall;if(s==storm_render::RenderAmbient)", "if(s>=256)return storm_render::result::invalidCall;if(rs[s]!=v&&(s==storm_render::RenderBlend||s==storm_render::RenderSourceBlend||s==storm_render::RenderDestinationBlend||s==storm_render::RenderSeparateAlphaBlend||s==storm_render::RenderSourceBlendAlpha||s==storm_render::RenderDestinationBlendAlpha||s==storm_render::RenderBlendFactor||s==storm_render::RenderDepthTest||s==storm_render::RenderDepthWrite||s==storm_render::RenderDepthCompare||s==storm_render::RenderCullMode||s==storm_render::RenderDepthBias||s==storm_render::RenderSlopeScaleDepthBias))flushOrdered();if(s==storm_render::RenderAmbient)")
once("if(s==storm_render::TransformWorld)authoredWorldValid=false;", "if((s==storm_render::TransformView||s==storm_render::TransformProjection)&&memcmp(&matrices[s],m,sizeof(*m)))flushOrdered();if(s==storm_render::TransformWorld)authoredWorldValid=false;")
once("storm_render::Result UpdateSurface(storm_render::Surface* source,const storm_render::Rect* sr,storm_render::Surface* destination,const storm_render::Point* point)override{", "storm_render::Result UpdateSurface(storm_render::Surface* source,const storm_render::Rect* sr,storm_render::Surface* destination,const storm_render::Point* point)override{flushOrdered();")
once("bool boundIndexed=false,const IndexedSpanFrameCache::Key* validatedKey=nullptr){bool depthPass=", "bool boundIndexed=false,const IndexedSpanFrameCache::Key* validatedKey=nullptr){if(orderedEnabled){++orderedStats.considered;if(landShadow.prepass.inPass||offscreenPointCasterDepth||landShadow.locationActive||declaration||vertexShader||pixelShader||!boundIndexed||!vb||!ib||t!=storm_render::PrimitiveTriangleList){++orderedStats.reject[8];flushOrdered();}}bool depthPass=")
once("if(rawOutdoor||rawIndoor||rawSeaLit||rawDynamicSail||rawWorld||rawSkinned||rawWdmModel){", '''// Rejection slots: 0 tier,1 shader,2 weather,3 range,4 index20,
// 5 sampler,6 topology,7 GPU/allocation,8 early family,9 raw family,
// 10 fire/aim/rigging/depth,11 dynamic/topology. Off executes the old path.
// MODELR hull traversal sets dynamicDraw but its rawWorld source is immutable.
// Both families use the same raw vertex/material shader; keep final payloads
// and traversal registry capture below this admission boundary unchanged.
bool orderedCandidate=orderedEnabled&&(rawWorld||rawSeaLit)&&!rawDynamicSail&&!rawSkinned&&!rawWdmModel&&!landShadow.locationActive;
if(orderedEnabled){
 if(!orderedCandidate)++orderedStats.reject[9];
 if(shipFireSurface.count||aimModelScope.captured||aimDirectPass||riggingMaterialDraw||depthPass||offscreenPointCasterDepth||interfaceBackSceneActive){orderedCandidate=false;++orderedStats.reject[10];}
 if(t!=storm_render::PrimitiveTriangleList||dynamicSource||vb&&vb->locked||ib&&ib->locked||!rawIndexRange){orderedCandidate=false;++orderedStats.reject[11];}
 if(orderedCandidate&&!prepareOrdered())orderedCandidate=false;
 if(orderedCandidate)++orderedStats.qualifiedFamily[rawWorld?0:1];
 if(!orderedCandidate){flushOrdered();previousAllocations=frameArena.allocations;}
}
if(rawOutdoor||rawIndoor||rawSeaLit||rawDynamicSail||rawWorld||rawSkinned||rawWdmModel){''')
once("if(!offscreenPointCasterDepth){auto stateResult=applyState((rawSkinned?", "if(!offscreenPointCasterDepth&&!orderedCandidate){auto stateResult=applyState((rawSkinned?")
once(" Uniform u=prepareFixedFunction();\n uint32_t colorOffset=rawSkinned?", " Uniform u=prepareFixedFunction(!orderedCandidate);\n uint32_t colorOffset=rawSkinned?")
once(" if(!offscreenPointCasterDepth){[encoder setVertexBuffer:rawVertices offset:rawVertexOffset atIndex:0];bindFixedFunction(u,rawDynamicSail);", ''' const bool orderedQueued=orderedCandidate&&enqueueOrdered(u,raw,lighting,rawVertices,rawVertexOffset,rawIndices,rawIndexOffset,rawBaseVertex,drawCount,fmt==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16);
 if(orderedQueued){const unsigned family=rawWorld?0:1;++orderedStats.admittedFamily[family];orderedStats.familyIndices[family]+=drawCount;++orderedStats.admittedTarget[family][target==back?0:1];}
 if(orderedCandidate)++telemetryReceiverDraws;
 if(orderedCandidate&&!orderedQueued){flushOrdered();auto result=applyState(0x1800ull,library,@"vs_landraw",@"fs_landlit");if(storm_render::failed(result))return result;prepareFixedFunction();}
 if(!offscreenPointCasterDepth&&!orderedQueued){[encoder setVertexBuffer:rawVertices offset:rawVertexOffset atIndex:0];bindFixedFunction(u,rawDynamicSail);''')
once("if(!offscreenPointCasterDepth)[encoder drawIndexedPrimitives:rawType", "if(!offscreenPointCasterDepth&&!orderedQueued)[encoder drawIndexedPrimitives:rawType")
assert "landShadow.append(packet,num);" in backend
# Admission never rebuilds lighting or reconstructs a registry packet. These
# existing owners must remain byte-for-byte intact in the private source.
assert function(original, " if(captureRawShadow)") in backend
lighting_start = original.index(" uint32_t colorOffset=rawSkinned?")
lighting_end = original.index(" const bool captureRawShadow=", lighting_start)
assert original[lighting_start:lighting_end] in backend
(HERE / "backend.mm").write_text(backend)
hashes = {"source_backend_sha256": hashlib.sha256(original.encode()).hexdigest(),
          "source_land_shadow_sha256": hashlib.sha256(shadow.encode()).hexdigest(),
          "source_resources_sha256": hashlib.sha256(resources.encode()).hexdigest(),
          "candidate_backend_sha256": hashlib.sha256(backend.encode()).hexdigest(),
          "shader_suffix_sha256": hashlib.sha256(suffix.encode()).hexdigest(),
          "ordered_batch_hpp_sha256": hashlib.sha256((HERE / "ordered_batch.hpp").read_bytes()).hexdigest(),
          "ordered_indices_hpp_sha256": hashlib.sha256((HERE / "ordered_indices.hpp").read_bytes()).hexdigest(),
          "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          "patch_transform_sha256": hashlib.sha256("".join(changes).encode()).hexdigest()}
print(json.dumps(hashes, indent=2))

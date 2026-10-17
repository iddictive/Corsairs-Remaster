// Editable inspection scene. Reuses the current production fire renderer and
// material burn field; sea, lighting, camera and lifecycle are demo presentation.
// Build outside the shared engine cache:
// xcrun clang++ -std=c++20 -O2 -fobjc-arc ship_fire_demo.mm -framework Cocoa
//   -framework Metal -framework MetalKit -o <owned-temp>/ship-fire-demo
// Run: ship-fire-demo <ship.json> <output.png> side|rear|overhead <seconds> <backend.mm>
// Batch: replace output.png with an output directory, then append frames step.
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <simd/simd.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "ship_fire_render.hpp"

namespace {
constexpr unsigned width=1280,height=720;
struct MeshVertex {float position[3],normal[3],uv[2];};
static_assert(sizeof(MeshVertex)==32);
struct DemoCamera {simd_float4x4 viewProjection,inverseViewProjection;simd_float4 eye,viewport,sun,time;};
struct Draw {unsigned first,count;bool cloth;id<MTLTexture> texture;};
struct ClothContact {simd_float3 position;float radius,seed,distance;};
const char*sceneMSL=R"MSL(
struct DemoCamera {float4x4 viewProjection,inverseViewProjection;float4 eye,viewport,sun,time;};
struct DemoVertex {packed_float3 position,normal;packed_float2 uv;};
struct DemoMeshO {float4 p [[position]];float3 local,normal;float2 uv;};
vertex DemoMeshO demo_mesh_vs(uint id [[vertex_id]],const device DemoVertex*vertices [[buffer(0)]],constant DemoCamera&c [[buffer(1)]]){
 DemoVertex v=vertices[id];DemoMeshO o;o.local=v.position;o.normal=v.normal;o.uv=v.uv;o.p=c.viewProjection*float4(o.local,1);return o;
}
fragment float4 demo_mesh_fs(DemoMeshO o [[stage_in]],constant DemoCamera&c [[buffer(1)]],constant ShipFireSurface&surface [[buffer(2)]],constant uint&cloth [[buffer(3)]],texture2d<float>tex [[texture(0)]],sampler repeatSampler [[sampler(0)]]){
 float4 albedo=tex.sample(repeatSampler,o.uv);if(albedo.a<.18f)discard_fragment();
 ShipFireEvaluation fire=shipFireEvaluate(o.local,normalize(o.normal),surface);
 float4 burn=fire.surface;albedo.rgb=shipFireMaterialAlbedo(albedo.rgb,burn,o.local);
 float3 normal=normalize(o.normal),sun=normalize(c.sun.xyz);float lit=max(dot(normal,sun),0.f);if(cloth)lit=abs(dot(normal,sun))*.70f+.12f;
 float sky=.5f+.5f*normal.y;float3 illumination=float3(.22f,.26f,.29f)+float3(.55f,.48f,.36f)*lit+float3(.05f,.07f,.09f)*sky;
 float3 eye=normalize(c.eye.xyz-o.local),halfway=normalize(eye+sun);
 float spec=pow(max(dot(normal,halfway),0.f),28.f)*.10f*(1.f-burn.x*.97f)*(cloth?0.f:1.f);
 float3 color=albedo.rgb*illumination*(1.f-burn.x*.25f)+float3(spec);
 color+=fire.irradiance*albedo.rgb*(1.f-burn.x*.25f)+float3(2.2f,.55f,.03f)*burn.w;
 return float4(color,1);
}
struct SceneO {float4 p [[position]];};
struct ScenePixel {float4 color [[color(0)]];float depth [[depth(any)]];};
vertex SceneO demo_scene_vs(uint id [[vertex_id]]){float2 q=float2((id<<1)&2,id&2);return {float4(q*float2(2,-2)+float2(-1,1),0,1)};}
float3 demoRay(float2 pixel,constant DemoCamera&c){float2 ndc=pixel/c.viewport.zw*float2(2,-2)+float2(-1,1);float4 p=c.inverseViewProjection*float4(ndc,0,1);return normalize(p.xyz/p.w-c.eye.xyz);}
fragment ScenePixel demo_scene_fs(SceneO o [[stage_in]],constant DemoCamera&c [[buffer(1)]]){
 float3 ray=demoRay(o.p.xy,c),horizon=float3(.40f,.47f,.50f),zenith=float3(.075f,.14f,.22f);
 float3 sky=mix(horizon,zenith,smoothstep(-.03f,.55f,ray.y));
 float clouds=shipFireTurbulence(ray*7.f+float3(0,1.7f,0));sky+=float3(.045f)*smoothstep(.58f,.78f,clouds)*smoothstep(.06f,.32f,ray.y);
 if(ray.y>=-.0001f)return {float4(sky,1),1};
 float distance=(-.35f-c.eye.y)/ray.y;float3 p=c.eye.xyz+ray*distance;
 float2 wave=p.xz*.16f;float time=c.time.x;
 float a=sin(wave.x*.87f+wave.y*.39f-time*.8f),b=sin(wave.y*1.13f-wave.x*.33f+time*.6f);
 float3 normal=normalize(float3(-a*.14f,1,-b*.12f));float fresnel=pow(1.f-saturate(dot(normal,-ray)),4.f);
 float sheen=pow(max(dot(normal,normalize(-ray+normalize(c.sun.xyz))),0.f),100.f);
 float3 water=mix(float3(.035f,.10f,.13f),horizon*.65f,fresnel)+sheen*float3(.35f,.31f,.21f);
 water+=float3(.025f,.03f,.033f)*(a*.5f+b*.5f);
 float haze=1.f-exp(-distance*.0008f);water=mix(water,horizon,haze);
 float4 clip=c.viewProjection*float4(p,1);float depth=clamp(clip.z/clip.w,0.f,1.f);
 return {float4(water,1),depth};
}
)MSL";

simd_float3 vec3(id value){NSArray*array=value;if(![array isKindOfClass:[NSArray class]]||array.count!=3)throw std::runtime_error("Expected three coordinates");return {[array[0] floatValue],[array[1] floatValue],[array[2] floatValue]};}
simd_float4x4 lookAt(simd_float3 eye,simd_float3 target){
 auto z=simd_normalize(target-eye),x=simd_normalize(simd_cross(simd_make_float3(0,1,0),z)),y=simd_cross(z,x);
 return (simd_float4x4){{{x.x,y.x,z.x,0},{x.y,y.y,z.y,0},{x.z,y.z,z.z,0},{-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1}}};
}
simd_float4x4 perspective(){float scale=1.f/std::tan(.72f*.5f),near=.3f,far=3000.f;return (simd_float4x4){{{scale/(float(width)/height),0,0,0},{0,scale,0,0},{0,0,far/(far-near),1},{0,0,-near*far/(far-near),0}}};}
NSData*readFile(NSString*path){NSData*data=[NSData dataWithContentsOfFile:path];if(!data)throw std::runtime_error(std::string("Cannot read ")+path.UTF8String);return data;}
NSString*relative(NSString*base,id path){if(![path isKindOfClass:[NSString class]])throw std::runtime_error("Missing asset filename");return [path isAbsolutePath]?path:[base stringByAppendingPathComponent:path];}
id<MTLRenderPipelineState> pipeline(id<MTLDevice>device,id<MTLLibrary>library,NSString*vertex,NSString*fragment){
 auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[library newFunctionWithName:vertex];d.fragmentFunction=[library newFunctionWithName:fragment];d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
 NSError*error=nil;auto result=[device newRenderPipelineStateWithDescriptor:d error:&error];if(!result)throw std::runtime_error(error.localizedDescription.UTF8String);return result;
}
std::string materialHelper(const char*backend){
 NSData*data=readFile([NSString stringWithUTF8String:backend]);std::string source(static_cast<const char*>(data.bytes),data.length);
 auto begin=source.find("float3 shipFireMaterialAlbedo(");if(begin==std::string::npos)throw std::runtime_error("Current backend material helper missing");auto end=source.find("\n}",begin);if(end==std::string::npos)throw std::runtime_error("Material helper boundary missing");return source.substr(begin,end+2-begin);
}
void savePNG(id<MTLTexture>texture,NSString*path){
 NSBitmapImageRep*image=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:width pixelsHigh:height bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:0 bytesPerRow:width*4 bitsPerPixel:32];
 std::vector<uint8_t>bgra(size_t(width)*height*4);[texture getBytes:bgra.data() bytesPerRow:width*4 fromRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0];auto*rgba=image.bitmapData;
 for(size_t i=0;i<bgra.size();i+=4){rgba[i]=bgra[i+2];rgba[i+1]=bgra[i+1];rgba[i+2]=bgra[i];rgba[i+3]=255;}
 NSData*png=[image representationUsingType:NSBitmapImageFileTypePNG properties:@{}];if(![png writeToFile:path atomically:YES])throw std::runtime_error("Cannot save rendered PNG");
}
float smooth(float a,float b,float x){float t=std::clamp((x-a)/(b-a),0.f,1.f);return t*t*(3.f-2.f*t);}
}

int main(int argc,const char**argv){@autoreleasepool{try{
 if(argc!=6&&argc!=8){std::fprintf(stderr,"Usage: ship-fire-demo ship.json output.png|directory side|rear|overhead seconds backend.mm [frames step]\n");return 2;}
 NSString*manifestPath=[NSString stringWithUTF8String:argv[1]],*base=manifestPath.stringByDeletingLastPathComponent;
 NSError*error=nil;NSDictionary*manifest=[NSJSONSerialization JSONObjectWithData:readFile(manifestPath) options:0 error:&error];if(![manifest isKindOfClass:[NSDictionary class]])throw std::runtime_error("Invalid asset manifest");
 NSData*vertexData=readFile(relative(base,manifest[@"vertices"])),*indexData=readFile(relative(base,manifest[@"indices"]));
 if(vertexData.length%sizeof(MeshVertex)||indexData.length%4)throw std::runtime_error("Mesh buffers have wrong packed layout");
 const auto*vertices=static_cast<const MeshVertex*>(vertexData.bytes);const auto*indices=static_cast<const uint32_t*>(indexData.bytes);size_t vertexCount=vertexData.length/sizeof(MeshVertex),indexCount=indexData.length/4;
 for(size_t i=0;i<indexCount;i++)if(indices[i]>=vertexCount)throw std::runtime_error("Asset index outside vertex buffer");
 auto device=MTLCreateSystemDefaultDevice();if(!device)throw std::runtime_error("Metal device unavailable");auto queue=[device newCommandQueue];
 auto loader=[[MTKTextureLoader alloc] initWithDevice:device];std::vector<Draw>draws;NSMutableDictionary<NSString*,id<MTLTexture>>*textures=[NSMutableDictionary dictionary];
 for(NSDictionary*item in manifest[@"draws"]){
  NSString*path=relative(base,item[@"texture"]);auto texture=textures[path];if(!texture){texture=[loader newTextureWithContentsOfURL:[NSURL fileURLWithPath:path] options:@{MTKTextureLoaderOptionSRGB:@NO,MTKTextureLoaderOptionGenerateMipmaps:@YES,MTKTextureLoaderOptionOrigin:MTKTextureLoaderOriginTopLeft} error:&error];if(!texture)throw std::runtime_error(std::string("Texture decode: ")+path.UTF8String);textures[path]=texture;}
  unsigned first=[item[@"firstIndex"] unsignedIntValue],count=[item[@"indexCount"] unsignedIntValue];if(uint64_t(first)+count>indexCount||count%3)throw std::runtime_error("Material range outside index buffer");draws.push_back({first,count,[item[@"cloth"] boolValue],texture});
 }
 auto vertexBuffer=[device newBufferWithBytes:vertexData.bytes length:vertexData.length options:MTLResourceStorageModeShared],indexBuffer=[device newBufferWithBytes:indexData.bytes length:indexData.length options:MTLResourceStorageModeShared];
 float seconds=std::strtof(argv[4],nullptr);if(!std::isfinite(seconds)||seconds<0)throw std::runtime_error("Invalid demo time");
 const float startSeconds=seconds,step=argc==8?std::strtof(argv[7],nullptr):0.f;
 const unsigned frameCount=argc==8?unsigned(std::strtoul(argv[6],nullptr,10)):1;
 if(!frameCount||frameCount>120||(argc==8&&(!std::isfinite(step)||step<=0.f||step>2.f)))throw std::runtime_error("Invalid bounded frame sequence");
 NSString*output=[NSString stringWithUTF8String:argv[2]];
 if(argc==8&&![[NSFileManager defaultManager] createDirectoryAtPath:output withIntermediateDirectories:YES attributes:nil error:&error])throw std::runtime_error("Cannot create frame output directory");
 simd_float3 lower=vec3(manifest[@"bounds"][@"min"]),upper=vec3(manifest[@"bounds"][@"max"]),center=(lower+upper)*.5f;
 float length=std::max(upper.z-lower.z,upper.x-lower.x),tall=upper.y-std::max(lower.y,0.f);auto target=simd_make_float3(center.x,std::max(upper.y*.43f,5.f),center.z);
 simd_float3 eye;std::string cameraName=argv[3];
 if(cameraName=="side")eye=target+simd_make_float3(length*1.60f,tall*.16f,length*.28f);
 else if(cameraName=="rear")eye=target+simd_make_float3(length*.25f,tall*.21f,-length*1.63f);
 else if(cameraName=="overhead")eye=target+simd_make_float3(length*.44f,length*1.54f,length*.29f);
 else throw std::runtime_error("Unknown camera");
 auto view=lookAt(eye,target),projection=perspective(),vp=simd_mul(projection,view);DemoCamera camera{vp,simd_inverse(vp),simd_make_float4(eye,1),{0,0,width,height},{-.45f,.62f,-.42f,0},{seconds,0,0,0}};
 std::vector<StormMetalShipFirePoint>deckSources;
 for(NSDictionary*item in manifest[@"burnPoints"]){if(deckSources.size()>=48)break;auto p=vec3(item[@"position"]);
  deckSources.push_back({{p.x,p.y,p.z},[item[@"radius"] floatValue],0,0,0,[item[@"seed"] floatValue]});
 }
 // Explicit presentation timeline, not the game's spread/balance simulation.
 // Find local ignition contacts on actual exported cloth triangles nearest
 // deck fuel. Early fire reaches two lower cells; the late inferno is separate.
 std::vector<ClothContact>clothContacts;
 for(NSDictionary*item in manifest[@"clothPoints"]){
  unsigned drawIndex=[item[@"drawIndex"] unsignedIntValue];if(drawIndex>=draws.size()||!draws[drawIndex].cloth)throw std::runtime_error("Cloth cell has no native material draw");
  const auto&draw=draws[drawIndex];float closest=INFINITY;simd_float3 contact{};
  for(unsigned triangle=draw.first;triangle<draw.first+draw.count;triangle+=3){
   simd_float3 center{};for(unsigned corner=0;corner<3;corner++){const auto&p=vertices[indices[triangle+corner]].position;center+=simd_make_float3(p[0],p[1],p[2])/3.f;}
   for(const auto&deck:deckSources){float distance=simd_length_squared(center-simd_make_float3(deck.position[0],deck.position[1],deck.position[2]));if(distance<closest){closest=distance;contact=center;}}
  }
  clothContacts.push_back({contact,std::clamp([item[@"radius"] floatValue]*.28f,.8f,1.65f),[item[@"seed"] floatValue],closest});
 }
 std::stable_sort(clothContacts.begin(),clothContacts.end(),[](const auto&a,const auto&b){return a.distance<b.distance;});
 std::vector<StormMetalShipFirePoint>points;
 auto updatePoints=[&](float seconds){points.clear();unsigned n=0;
 for(auto point:deckSources){float delay=float(n%4)*.36f;
  float growth=smooth(.25f+delay,4.6f+delay,seconds);point.intensity=growth*(1.f-smooth(9.7f,13.2f,seconds));
  point.charred=smooth(.4f+delay,8.f+delay,seconds)*.97f;point.heat=growth*(1.f-smooth(13.f,20.f,seconds));points.push_back(point);++n;
 }
 for(unsigned rank=0;rank<clothContacts.size()&&points.size()<48;rank++){
  const auto&cell=clothContacts[rank];auto p=cell.position;float delay=rank<2?3.8f+float(rank)*1.1f:12.f+float(rank-2)*.55f;
  float growth=smooth(delay,delay+3.6f,seconds),extinction=1.f-smooth(rank<2?11.f:23.f,rank<2?15.5f:28.f,seconds),intensity=growth*extinction*.74f;
  float charred=smooth(delay+1.f,delay+7.f,seconds)*.90f,heat=growth*(1.f-smooth(rank<2?15.f:28.f,rank<2?22.f:35.f,seconds));
  points.push_back({{p.x,p.y,p.z},cell.radius,intensity,charred,heat,cell.seed});
 }
 };
 updatePoints(seconds);
 if(points.empty())throw std::runtime_error("Demo requires real ship fire anchors");
 storm_metal::ShipFireSurfaceScope scope;auto root=matrix_identity_float4x4;storm_metal::ShipFireSurfaceUniform surface{};
 std::string source=std::string(storm_metal::shipFireCommonMSL)+materialHelper(argv[5])+sceneMSL;
 auto library=[device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];if(!library)throw std::runtime_error(error.localizedDescription.UTF8String);
 auto meshPipeline=pipeline(device,library,@"demo_mesh_vs",@"demo_mesh_fs"),scenePipeline=pipeline(device,library,@"demo_scene_vs",@"demo_scene_fs");
 auto textureDescriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];textureDescriptor.storageMode=MTLStorageModeShared;textureDescriptor.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;auto color=[device newTextureWithDescriptor:textureDescriptor];textureDescriptor.pixelFormat=MTLPixelFormatDepth32Float;textureDescriptor.storageMode=MTLStorageModePrivate;auto depth=[device newTextureWithDescriptor:textureDescriptor];
 auto samplerDescriptor=[MTLSamplerDescriptor new];samplerDescriptor.minFilter=samplerDescriptor.magFilter=MTLSamplerMinMagFilterLinear;samplerDescriptor.mipFilter=MTLSamplerMipFilterLinear;samplerDescriptor.sAddressMode=samplerDescriptor.tAddressMode=MTLSamplerAddressModeRepeat;auto sampler=[device newSamplerStateWithDescriptor:samplerDescriptor];
 auto depthDescriptor=[MTLDepthStencilDescriptor new];depthDescriptor.depthCompareFunction=MTLCompareFunctionLessEqual;depthDescriptor.depthWriteEnabled=YES;auto depthState=[device newDepthStencilStateWithDescriptor:depthDescriptor];
 storm_metal::ShipFireRenderer fire;FrameArena arena;const float wind[]={3.4f,0,1.1f};std::vector<double>gpuMilliseconds;double peakRequested=0,peakAdmitted=0;
 for(unsigned frame=0;frame<frameCount;frame++){@autoreleasepool{
 seconds=startSeconds+float(frame)*step;camera.time.x=seconds;updatePoints(seconds);surface={};scope.begin(reinterpret_cast<const float*>(&root),points.data(),uint32_t(points.size()));scope.fill(surface,root,seconds);
 auto command=[queue commandBuffer];auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(.1,.15,.2,1);pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;
 auto encoder=[command renderCommandEncoderWithDescriptor:pass];[encoder setViewport:MTLViewport{0,0,width,height,0,1}];[encoder setCullMode:MTLCullModeNone];[encoder setDepthStencilState:depthState];[encoder setRenderPipelineState:scenePipeline];[encoder setFragmentBytes:&camera length:sizeof(camera) atIndex:1];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
 [encoder setRenderPipelineState:meshPipeline];[encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];[encoder setVertexBytes:&camera length:sizeof(camera) atIndex:1];[encoder setFragmentBytes:&surface length:sizeof(surface) atIndex:2];[encoder setFragmentSamplerState:sampler atIndex:0];
 for(const auto&draw:draws){uint32_t cloth=draw.cloth;surface.counts.y=cloth;[encoder setFragmentBytes:&surface length:sizeof(surface) atIndex:2];[encoder setFragmentBytes:&cloth length:sizeof(cloth) atIndex:3];[encoder setFragmentTexture:draw.texture atIndex:0];[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:draw.count indexType:MTLIndexTypeUInt32 indexBuffer:indexBuffer indexBufferOffset:size_t(draw.first)*4];}[encoder endEncoding];
 if(!fire.append(reinterpret_cast<const float*>(&root),points.data(),uint32_t(points.size()),wind,seconds,{0,0,0},view,projection,{0,0,width,height},{.4f,.47f,.5f,1},{0,0,0,0},.72f)||!fire.encode(device,command,color,depth,arena))throw std::runtime_error("Production fire renderer rejected demo frame");
 [command commit];[command waitUntilCompleted];if(command.error)throw std::runtime_error(command.error.localizedDescription.UTF8String);
 NSString*frameOutput=argc==8?[output stringByAppendingPathComponent:[NSString stringWithFormat:@"frame-%04u.png",frame]]:output;savePNG(color,frameOutput);
 gpuMilliseconds.push_back((command.GPUEndTime-command.GPUStartTime)*1000.);peakRequested=std::max(peakRequested,fire.budget.requestedWork);peakAdmitted=std::max(peakAdmitted,fire.budget.admittedWork);
 if(!arena.rewindAfterCompletion(0,command))throw std::runtime_error("Completed demo frame did not release upload arena");
 }}
 std::sort(gpuMilliseconds.begin(),gpuMilliseconds.end());
 std::printf("GPU command min/median/max %.3f/%.3f/%.3f ms (includes demo geometry/background/effect; not gameplay FPS or a comparative performance test)\n",gpuMilliseconds.front(),gpuMilliseconds[gpuMilliseconds.size()/2],gpuMilliseconds.back());
 std::printf("Demo %s t=%.2f..%.2f: %u frames, %zu real-mesh vertices, %zu triangles, %zu authored anchors; output %s; peak predicted fire sample work %.0f -> %.0f (not game FPS)\n",argv[3],startSeconds,seconds,frameCount,vertexCount,indexCount/3,points.size(),argv[2],peakRequested,peakAdmitted);
 return 0;
 }catch(const std::exception&error){std::fprintf(stderr,"Ship fire demo: %s\n",error.what());return 1;}}}

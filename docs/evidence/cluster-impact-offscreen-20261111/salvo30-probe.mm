// Standalone, synthetic Metal fixture. Does not launch or read the game.
// clang++ -std=c++17 -fobjc-arc -framework Metal -framework AppKit cluster_impact_offscreen.mm -o /tmp/cluster-impact-probe
// /tmp/cluster-impact-probe /absolute/output/directory
#import <Metal/Metal.h>
#import <AppKit/AppKit.h>
#include "cluster_impact_render.hpp"
#include <cstdio>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace storm_metal;
static const char *sceneMSL=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct U { float4x4 inverseVP, vp; float4 eye,viewport; float land; };
vertex float4 scene_vs(uint i [[vertex_id]]) { constexpr float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)}; return float4(p[i],0,1); }
struct Out { float4 color [[color(0)]]; float depth [[depth(any)]]; };
fragment Out scene_fs(float4 pix [[position]],constant U &u [[buffer(0)]]) {
 float2 ndc=(pix.xy/u.viewport.xy)*float2(2,-2)+float2(-1,1);
 float4 q=u.inverseVP*float4(ndc,1,1); float3 ray=normalize(q.xyz/q.w-u.eye.xyz);
 float t=-u.eye.y/ray.y; Out o;
 // Two synthetic 60m hull boxes for a scale comparison, not game models.
 if(u.land<.5) for(int k=0;k<2;k++) {
  float3 c=float3(k==0?-37.:37.,4.,-45.); float3 h=float3(30.,4.,7.);
  float3 safe=select(ray,select(float3(-1.e-6),float3(1.e-6),ray>=0),abs(ray)<1.e-6);
  float3 a=(c-h-u.eye.xyz)/safe,b=(c+h-u.eye.xyz)/safe;float3 lo=min(a,b),hi=max(a,b);
  float enter=max(0.,max(lo.x,max(lo.y,lo.z))),leave=min(hi.x,min(hi.y,hi.z));
  if(leave>enter && (t<=0 || enter<t)) {float3 pos=u.eye.xyz+ray*enter;float4 clip=u.vp*float4(pos,1);o.depth=clip.z/clip.w;o.color=float4(.20,.13,.085,1);return o;}
 }

 if(t>0 && ray.y<-.0001) { float3 p=u.eye.xyz+ray*t; float4 clip=u.vp*float4(p,1); o.depth=clip.z/clip.w;
  float wave=.5+.5*sin(p.x*2.2+sin(p.z*1.4))*sin(p.z*2.1);
  o.color=float4(mix(float3(.025,.09,.14)+wave*.035,float3(.20,.16,.105)+wave*.035,u.land),1);
 } else { o.depth=1; float k=clamp(ray.y*1.8,0.,1.); o.color=float4(mix(float3(.48,.55,.58),float3(.20,.29,.38),k),1); }
 return o;
}
)MSL";
struct SceneUniform { simd_float4x4 inverseVP,vp; simd_float4 eye,viewport; float land; };
static simd_float4x4 look(simd_float3 eye,simd_float3 target) {
 auto z=simd_normalize(eye-target); auto x=simd_normalize(simd_cross(simd_make_float3(0,1,0),z)); auto y=simd_cross(z,x);
 return simd_matrix(simd_make_float4(x.x,y.x,z.x,0),simd_make_float4(x.y,y.y,z.y,0),simd_make_float4(x.z,y.z,z.z,0),simd_make_float4(-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1));
}
static simd_float4x4 perspective(float aspect) {
 float f=1.f/tanf(55.f*3.14159265f/360.f), n=.1f, far=2000.f;
 return simd_matrix(simd_make_float4(f/aspect,0,0,0),simd_make_float4(0,f,0,0),simd_make_float4(0,0,far/(n-far),-1),simd_make_float4(0,0,n*far/(n-far),0));
}
static bool png(id<MTLTexture> texture, NSString *path) {
 NSUInteger w=texture.width,h=texture.height; std::vector<unsigned char> bytes(w*h*4);
 [texture getBytes:bytes.data() bytesPerRow:w*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0];
 NSBitmapImageRep *im=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil pixelsWide:w pixelsHigh:h bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:w*4 bitsPerPixel:32];
 memcpy(im.bitmapData,bytes.data(),bytes.size()); return [[im representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES];
}
int main(int argc,char **argv) { @autoreleasepool {
 if(argc<2||argc>4) return 2; bool cascade=argc>=3 && strcmp(argv[2],"cascade")==0; bool spread=cascade||(argc>=3 && strcmp(argv[2],"spread")==0); unsigned warhead=argc==4?unsigned(atoi(argv[3])):1; if(warhead<1||warhead>4)return 2; NSString *dir=[NSString stringWithUTF8String:argv[1]];
 id<MTLDevice> device=MTLCreateSystemDefaultDevice(); if(!device)return 2;
 NSError *err=nil; auto lib=[device newLibraryWithSource:[NSString stringWithUTF8String:sceneMSL] options:nil error:&err];
 if(!lib){fprintf(stderr,"scene shader: %s\n",err.localizedDescription.UTF8String);return 1;}
 auto pd=[MTLRenderPipelineDescriptor new]; pd.vertexFunction=[lib newFunctionWithName:@"scene_vs"]; pd.fragmentFunction=[lib newFunctionWithName:@"scene_fs"]; pd.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm; pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
 auto pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&err]; if(!pipeline){fprintf(stderr,"scene pipeline: %s\n",err.localizedDescription.UTF8String);return 1;}
 auto ds=[MTLDepthStencilDescriptor new];ds.depthWriteEnabled=YES;ds.depthCompareFunction=MTLCompareFunctionAlways; auto state=[device newDepthStencilStateWithDescriptor:ds];
 constexpr int w=640,h=360; auto cd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];cd.storageMode=MTLStorageModeShared;cd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
 auto zd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:w height:h mipmapped:NO];zd.storageMode=MTLStorageModePrivate;zd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
 auto color=[device newTextureWithDescriptor:cd],depth=[device newTextureWithDescriptor:zd]; auto queue=[device newCommandQueue]; ClusterImpactRenderer renderer;
 auto loadAtlas=[&](NSString *asset)->id<MTLTexture> {
  NSBitmapImageRep *bitmap=[[NSBitmapImageRep alloc] initWithData:[NSData dataWithContentsOfFile:asset]];CGImageRef image=bitmap.CGImage;if(!image){fprintf(stderr,"missing atlas %s\n",asset.UTF8String);return nil;}
  size_t aw=CGImageGetWidth(image),ah=CGImageGetHeight(image);std::vector<unsigned char> pixels(aw*ah*4);auto space=CGColorSpaceCreateDeviceRGB();
  auto ctx=CGBitmapContextCreate(pixels.data(),aw,ah,8,aw*4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);CGColorSpaceRelease(space);if(!ctx)return nil;
  CGContextDrawImage(ctx,CGRectMake(0,0,aw,ah),image);CGContextRelease(ctx);
  auto ad=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:aw height:ah mipmapped:NO];ad.storageMode=MTLStorageModeShared;ad.usage=MTLTextureUsageShaderRead;
  auto atlas=[device newTextureWithDescriptor:ad];[atlas replaceRegion:MTLRegionMake2D(0,0,aw,ah) mipmapLevel:0 withBytes:pixels.data() bytesPerRow:aw*4];return atlas;
 };
 auto e0=loadAtlas(@"src/assets/cluster-vfx/Explosion00_5x5.png"),e1=loadAtlas(@"src/assets/cluster-vfx/Explosion01_5x5.png"),e2=loadAtlas(@"src/assets/cluster-vfx/Explosion02_5x5.png"),spray=loadAtlas(@"src/assets/cluster-vfx/WispySmoke02_8x8.png");
 if(!e0||!e1||!e2||!spray)return 1;renderer.setImpactAtlases(e0,e1,e2,spray); auto begin=std::chrono::steady_clock::now(); NSData *volume=[NSData dataWithContentsOfFile:@"src/assets/cluster-vfx/ground-vdb/GroundExplosion64.rg8"]; if(!volume||renderer.setVolumeData(device,volume.bytes,volume.length-1)||!renderer.setVolumeData(device,volume.bytes,volume.length))return 3; printf("volume_load_cpu_ms %.3f\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
 simd_float3 eye={0,35.f,120.f}; if(getenv("VDB_SIDE"))eye={120.f,35.f,0}; if(getenv("VDB_FAR"))eye={0,80.f,300.f}; auto view=look(eye,simd_make_float3(0,6.f,0)), projection=perspective(float(w)/h),vp=simd_mul(projection,view);
 SceneUniform su{};su.vp=vp;su.inverseVP=simd_inverse(vp);su.eye=simd_make_float4(eye,1);su.viewport={float(w),float(h),0,0};
 NSString *csvPath=[dir stringByAppendingPathComponent:@"timings.csv"]; FILE *csv=fopen(csvPath.fileSystemRepresentation,"w");if(!csv)return 1; fprintf(csv,"surface,age,count,trial,gpu_ms\n");
 for(int land=0;land<2;land++) for(float age:{.06f,.18f,.40f,.80f,1.40f,2.40f}) for(int count:{0,1,8,30}) {
  su.land=float(land); std::vector<double> times,cpuTimes;
  if(warhead!=2 && count==8)continue;
  for(int trial=-1;trial<2;trial++) { @autoreleasepool {
   auto cpuStart=std::chrono::steady_clock::now(); auto cmd=[queue commandBuffer]; auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;
   auto e=[cmd renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:pipeline];[e setDepthStencilState:state];[e setFragmentBytes:&su length:sizeof(su) atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];
   bool young=count>0 && age-.025f*float(count-1)<.62f; if(!renderer.captureBackground(device,cmd,color,young))return 4; auto batch=count?renderer.beginBatch(device,cmd,color):nil;if(count&&!batch)return 4;
   for(int i=0;i<count;i++) {float a=float(i)*2.39996f;float r=i? .16f+.10f*float(i%3):0;simd_float3 center={cosf(a)*r,0,sinf(a)*r};float impactAge=age;
    if(spread && count>1) {center={float(i%4)*1.15f-1.72f,0,float(i/4)*1.1f-.55f};impactAge=std::max(.03f,age-.075f*float(i%4));}
    if(cascade && count>1) {center={float(i)*(110.f/29.f)-55.f,0,float((i*17)%41)-20.f};impactAge=age-.025f*float(i);if(impactAge<0)continue;}
    simd_float3 incoming=simd_make_float3(.65f*cosf(a),-1.f,.65f*sinf(a));
    if(!renderer.encode(device,cmd,color,depth,view,projection,{0,0,0},{0,0,w,h},center,(warhead==2?8.f:18.f),impactAge,land?ClusterImpactSurface::Land:ClusterImpactSurface::Sea,float(i)*.71f,incoming,warhead,batch)){fprintf(stderr,"impact encode failed\n");return 1;}}
   if(batch)[batch endEncoding]; [cmd commit];[cmd waitUntilCompleted];if(cmd.status!=MTLCommandBufferStatusCompleted){fprintf(stderr,"GPU error: %s\n",cmd.error.localizedDescription.UTF8String);return 1;}
   if(trial>=0){double ms=(cmd.GPUEndTime-cmd.GPUStartTime)*1000.;times.push_back(ms);cpuTimes.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-cpuStart).count());fprintf(csv,"%s,%.2f,%d,%d,%.6f\n",land?"land":"sea",age,count,trial,ms);}
   if(trial==0&&(count==0||count==1||count==8||count==30)){NSString *name=[NSString stringWithFormat:@"%@-%.2f-%d.png",land?@"land":@"sea",age,count];if(!png(color,[dir stringByAppendingPathComponent:name]))return 1;}
  }}
  std::sort(times.begin(),times.end());std::sort(cpuTimes.begin(),cpuTimes.end());printf("%s %.2f %d mean %.3f ms max %.3f ms wall %.3f ms\n",land?"land":"sea",age,count,(times[0]+times[1])*.5,times[1],(cpuTimes[0]+cpuTimes[1])*.5);fflush(stdout);
 }
 fclose(csv);return 0;
} }

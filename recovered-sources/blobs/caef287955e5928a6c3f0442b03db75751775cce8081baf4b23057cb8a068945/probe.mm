#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>
#import <VideoToolbox/VideoToolbox.h>
#import <CoreVideo/CoreVideo.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <unistd.h>
#include <mach/mach.h>

static size_t limitBytes=1024ull*1024*1024;
static NSString* exportRoot=nil;
static double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static void fail(const char*s){std::fprintf(stderr,"FAIL %s\n",s);std::exit(2);}
static size_t footprint(){task_vm_info_data_t v{};mach_msg_type_number_t n=TASK_VM_INFO_COUNT;task_info(mach_task_self(),TASK_VM_INFO,(task_info_t)&v,&n);return v.phys_footprint;}
struct Pixel {
 CVPixelBufferRef buffer=nullptr;CVMetalTextureRef wrapper=nullptr;id<MTLTexture> texture=nil;
 Pixel(id<MTLDevice>d,CVMetalTextureCacheRef cache,NSDictionary* attrs,MTLPixelFormat format){
  NSMutableDictionary* a=[attrs mutableCopy];a[(id)kCVPixelBufferMetalCompatibilityKey]=@YES;
  auto w=[a[(id)kCVPixelBufferWidthKey] unsignedLongValue],h=[a[(id)kCVPixelBufferHeightKey] unsignedLongValue];
  id formats=a[(id)kCVPixelBufferPixelFormatTypeKey];if([formats isKindOfClass:[NSArray class]])formats=[formats firstObject];a[(id)kCVPixelBufferPixelFormatTypeKey]=formats;auto f=[formats unsignedIntValue];
  if(CVPixelBufferCreate(kCFAllocatorDefault,w,h,f,(__bridge CFDictionaryRef)a,&buffer))fail("CVPixelBufferCreate");
  if(CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault,cache,buffer,nullptr,format,w,h,0,&wrapper))fail("CVMetalTextureCacheCreateTextureFromImage");
  texture=CVMetalTextureGetTexture(wrapper);if(!texture)fail("CVMetalTextureGetTexture");
 }
 ~Pixel(){if(wrapper)CFRelease(wrapper);if(buffer)CFRelease(buffer);}
};
static const char*msl=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V{float4 position[[position]];};
struct P{float w,h,t,hud;};
vertex V vs(uint i[[vertex_id]]){return {float4(i==0?float2(-1,-1):i==1?float2(3,-1):float2(-1,3),0,1)};}
float hash(float2 x){return fract(sin(dot(x,float2(127.1,311.7)))*43758.5453);}
float noise(float2 x){float2 i=floor(x),f=fract(x);f=f*f*(3-2*f);return mix(mix(hash(i),hash(i+float2(1,0)),f.x),mix(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
float4 scene(float2 p,constant P&u){
 float2 q=p-float2(4,2)*u.t;
 float3 rgb=.05+.42*float3(noise(q/17),noise(q/37+13),noise(q/53+29));
 float2 o=p-(float2(u.w*.35,u.h*.48)+float2(16,6)*u.t);
 if(abs(o.x)<u.w*.12&&abs(o.y)<u.h*.15)rgb=float3(.25+.65*noise(o/15+71),.08+.18*noise(o/23+31),.06+.15*noise(o/41+89));
 return float4(rgb,1);
}
float4 hud(float4 c,float2 p,constant P&u){
 if(p.x>u.w*.7&&p.x<u.w*.95&&p.y>u.h*.05&&p.y<u.h*.14){bool opaque=(int(p.x/8)%3==0)||p.y<u.h*.075;float3 h=opaque?float3(.95,.95,.95):float3(.05,.2,.05);return float4(opaque?h:mix(c.rgb,h,.75),1);}return c;
}
struct F{float4 color[[color(0)]];float depth[[depth(any)]];};
fragment float4 colorfs(V v[[stage_in]],constant P&u[[buffer(0)]]){float4 c=scene(v.position.xy,u);if(u.hud>0)c=hud(c,v.position.xy,u);return c;}
fragment F fs(V v[[stage_in]],constant P&u[[buffer(0)]]){
 float2 o=v.position.xy-(float2(u.w*.35,u.h*.48)+float2(16,6)*u.t);
 float d=(abs(o.x)<u.w*.12&&abs(o.y)<u.h*.15)?.5:.9;
 float4 c=scene(v.position.xy,u);if(u.hud>0)c=hud(c,v.position.xy,u);return {c,d};
}
kernel void convert(texture2d<float,access::read>s[[texture(0)]],texture2d<float,access::write>d[[texture(1)]],uint2 p[[thread_position_in_grid]]){if(p.x<d.get_width()&&p.y<d.get_height())d.write(s.read(p),p);}
kernel void overlay(texture2d<float,access::read>s[[texture(0)]],texture2d<float,access::write>d[[texture(1)]],constant P&u[[buffer(0)]],uint2 p[[thread_position_in_grid]]){if(p.x<d.get_width()&&p.y<d.get_height())d.write(hud(s.read(p),float2(p)+.5,u),p);}
kernel void flowup(texture2d<float>s[[texture(0)]],texture2d<float,access::write>d[[texture(1)]],constant float2&scale[[buffer(0)]],uint2 p[[thread_position_in_grid]]){if(p.x>=d.get_width()||p.y>=d.get_height())return;constexpr sampler sam(coord::normalized,address::clamp_to_edge,filter::linear);float2 v=s.sample(sam,(float2(p)+.5)/float2(d.get_width(),d.get_height())).xy*scale;d.write(float4(v,0,0),p);}
// Rec.709 Y'CbCr video range. Pad the native odd last row by edge replication;
// 2x downsampling averages each 2x2 block; chroma averages the four low pixels.
float3 nativeLow(texture2d<float,access::read>s,uint2 p){float3 c=0;for(uint y=0;y<2;y++)for(uint x=0;x<2;x++)c+=s.read(min(p*2+uint2(x,y),uint2(s.get_width()-1,s.get_height()-1))).rgb;return c*.25;}
float3 yuv(float3 c){float y=dot(c,float3(.2126,.7152,.0722));return float3((16+219*y)/255,(128+224*(c.b-y)/1.8556)/255,(128+224*(c.r-y)/1.5748)/255);}
kernel void rgbnv(texture2d<float,access::read>s[[texture(0)]],texture2d<float,access::write>y[[texture(1)]],texture2d<float,access::write>uv[[texture(2)]],uint2 p[[thread_position_in_grid]]){if(p.x>=y.get_width()||p.y>=y.get_height())return;y.write(float4(yuv(nativeLow(s,p)).x,0,0,1),p);if((p.x&1)==0&&(p.y&1)==0){float3 c=0;for(uint j=0;j<2;j++)for(uint i=0;i<2;i++)c+=nativeLow(s,p+uint2(i,j));float3 v=yuv(c*.25);uv.write(float4(v.yz,0,1),p/2);}}
kernel void nvrgb(texture2d<float,access::read>y[[texture(0)]],texture2d<float>uv[[texture(1)]],texture2d<float,access::write>d[[texture(2)]],uint2 p[[thread_position_in_grid]]){if(p.x>=d.get_width()||p.y>=d.get_height())return;constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);float l=(y.read(p).r*255-16)/219;float2 c=(uv.sample(s,(float2(p)+.5)/float2(y.get_width(),y.get_height())).rg*255-128)/224;float3 rgb=float3(l+1.5748*c.y,l-.187324*c.x-.468124*c.y,l+1.8556*c.x);d.write(float4(clamp(rgb,0.f,1.f),1),p);}
kernel void nvrgbscale(texture2d<float>y[[texture(0)]],texture2d<float>uv[[texture(1)]],texture2d<float,access::write>d[[texture(2)]],uint2 p[[thread_position_in_grid]]){if(p.x>=d.get_width()||p.y>=d.get_height())return;constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);float2 tc=(float2(p)+.5)/float2(y.get_width()*2,y.get_height()*2);float l=(y.sample(s,tc).r*255-16)/219;float2 c=(uv.sample(s,tc).rg*255-128)/224;float3 rgb=float3(l+1.5748*c.y,l-.187324*c.x-.468124*c.y,l+1.8556*c.x);d.write(float4(clamp(rgb,0.f,1.f),1),p);}
kernel void sentinel(texture2d<float,access::write>y[[texture(0)]],texture2d<float,access::write>uv[[texture(1)]],uint2 p[[thread_position_in_grid]]){if(p.x>=y.get_width()||p.y>=y.get_height())return;y.write(float4(96.f/255,0,0,1),p);if((p.x&1)==0&&(p.y&1)==0)uv.write(float4(64.f/255,192.f/255,0,1),p/2);}
)MSL";
struct Params{float w,h,t,hud;};
static id<MTLTexture> tex(id<MTLDevice>d,NSUInteger w,NSUInteger h,MTLPixelFormat f){auto q=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f width:w height:h mipmapped:NO];q.storageMode=MTLStorageModePrivate;q.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;return[d newTextureWithDescriptor:q];}
static void dispatch(id<MTLCommandBuffer>c,id<MTLComputePipelineState>p,id<MTLTexture>s,id<MTLTexture>d,const void*b=nullptr,size_t n=0){auto e=[c computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:s atIndex:0];[e setTexture:d atIndex:1];if(n)[e setBytes:b length:n atIndex:0];[e dispatchThreads:MTLSizeMake(d.width,d.height,1) threadsPerThreadgroup:MTLSizeMake(16,16,1)];[e endEncoding];}
static id<MTLRenderPipelineState> renderPipeline(id<MTLDevice>d,id<MTLLibrary>l,MTLPixelFormat f,bool depth){auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[l newFunctionWithName:@"vs"];p.fragmentFunction=[l newFunctionWithName:depth?@"fs":@"colorfs"];p.colorAttachments[0].pixelFormat=f;if(depth)p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;NSError*error=nil;auto r=[d newRenderPipelineStateWithDescriptor:p error:&error];if(!r){NSLog(@"%@",error);fail("render pipeline");}return r;}
static void render(id<MTLCommandBuffer>c,id<MTLRenderPipelineState>p,id<MTLDepthStencilState>ds,id<MTLTexture>t,id<MTLTexture>z,Params u){auto r=[MTLRenderPassDescriptor renderPassDescriptor];r.colorAttachments[0].texture=t;r.colorAttachments[0].loadAction=MTLLoadActionDontCare;r.colorAttachments[0].storeAction=MTLStoreActionStore;if(z){r.depthAttachment.texture=z;r.depthAttachment.loadAction=MTLLoadActionClear;r.depthAttachment.storeAction=MTLStoreActionStore;r.depthAttachment.clearDepth=1;}auto e=[c renderCommandEncoderWithDescriptor:r];[e setRenderPipelineState:p];if(z)[e setDepthStencilState:ds];[e setFragmentBytes:&u length:sizeof(u) atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];}
struct Readback{ id<MTLBuffer> b=nil;size_t pitch=0;Readback(id<MTLDevice>d,size_t w,size_t h,size_t bytes){pitch=((w*bytes+255)/256)*256;b=[d newBufferWithLength:pitch*h options:MTLResourceStorageModeShared];}void copy(id<MTLCommandBuffer>c,id<MTLTexture>t){auto e=[c blitCommandEncoder];[e copyFromTexture:t sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(t.width,t.height,1) toBuffer:b destinationOffset:0 destinationBytesPerRow:pitch destinationBytesPerImage:pitch*t.height];[e endEncoding];}float value(size_t x,size_t y,size_t ch,size_t channels){auto p=(const _Float16*)((const char*)b.contents+y*pitch);return(float)p[x*channels+ch];}};
static float mean(Readback&r,float x,float y,int channel){float sum=0;for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)sum+=r.value(size_t(x)+dx,size_t(y)+dy,channel,2);return sum/25;}
static void metrics(Readback&actual,Readback&truth,size_t w,size_t h){double sum=0,max=0;size_t bad=0,n=0;double hudsum=0;size_t hn=0;for(size_t y=0;y<h;y+=2)for(size_t x=0;x<w;x+=2)for(size_t k=0;k<3;k++){float a=actual.value(x,y,k,4),b=truth.value(x,y,k,4);if(!std::isfinite(a)){bad++;continue;}double d=std::abs(a-b);sum+=d*d;max=std::max(max,d);n++;if(x>w*.71&&x<w*.94&&y>h*.06&&y<h*.13){hudsum+=d*d;hn++;}}printf("metrics rmse=%.7f max_error=%.7f nonfinite=%zu hud_rmse=%.7f samples=%zu\n",sqrt(sum/std::max<size_t>(n,1)),max,bad,sqrt(hudsum/std::max<size_t>(hn,1)),n);}
struct NVPixel {
 CVPixelBufferRef buffer=nullptr;CVMetalTextureRef wy=nullptr,wuv=nullptr;id<MTLTexture> y=nil,uv=nil;
 NVPixel(CVMetalTextureCacheRef cache,NSDictionary*attrs){
  NSMutableDictionary*a=[attrs mutableCopy];a[(id)kCVPixelBufferMetalCompatibilityKey]=@YES;id f=a[(id)kCVPixelBufferPixelFormatTypeKey];if([f isKindOfClass:[NSArray class]])f=[f firstObject];a[(id)kCVPixelBufferPixelFormatTypeKey]=f;
  if([f unsignedIntValue]!=kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)fail("lowlat format must be420v");
  auto w=[a[(id)kCVPixelBufferWidthKey]unsignedLongValue],h=[a[(id)kCVPixelBufferHeightKey]unsignedLongValue];
  if(CVPixelBufferCreate(kCFAllocatorDefault,w,h,[f unsignedIntValue],(__bridge CFDictionaryRef)a,&buffer))fail("NV12 allocation");
  CVBufferSetAttachment(buffer,kCVImageBufferYCbCrMatrixKey,kCVImageBufferYCbCrMatrix_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);CVBufferSetAttachment(buffer,kCVImageBufferColorPrimariesKey,kCVImageBufferColorPrimaries_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);CVBufferSetAttachment(buffer,kCVImageBufferTransferFunctionKey,kCVImageBufferTransferFunction_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
  NSDictionary*tattrs=@{(id)kCVMetalTextureUsage:@(MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite)};
  if(CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault,cache,buffer,(__bridge CFDictionaryRef)tattrs,MTLPixelFormatR8Unorm,w,h,0,&wy))fail("NV12 Y wrapper");
  if(CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault,cache,buffer,(__bridge CFDictionaryRef)tattrs,MTLPixelFormatRG8Unorm,w/2,h/2,1,&wuv))fail("NV12 UV wrapper");
  y=CVMetalTextureGetTexture(wy);uv=CVMetalTextureGetTexture(wuv);if(!y||!uv)fail("NV12 textures nil");
 }
 ~NVPixel(){if(wy)CFRelease(wy);if(wuv)CFRelease(wuv);if(buffer)CFRelease(buffer);}
};
static void convertNV(id<MTLCommandBuffer>c,id<MTLComputePipelineState>p,id<MTLTexture>s,NVPixel&d){auto e=[c computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:s atIndex:0];[e setTexture:d.y atIndex:1];[e setTexture:d.uv atIndex:2];[e dispatchThreads:MTLSizeMake(d.y.width,d.y.height,1) threadsPerThreadgroup:MTLSizeMake(16,16,1)];[e endEncoding];}
static void decodeNV(id<MTLCommandBuffer>c,id<MTLComputePipelineState>p,NVPixel&s,id<MTLTexture>d){auto e=[c computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:s.y atIndex:0];[e setTexture:s.uv atIndex:1];[e setTexture:d atIndex:2];[e dispatchThreads:MTLSizeMake(d.width,d.height,1) threadsPerThreadgroup:MTLSizeMake(16,16,1)];[e endEncoding];}
static void hudMetrics(Readback&actual,Readback&under,size_t w,size_t h){double opaque=0,trans=0;size_t on=0,tn=0;double max=0;for(size_t y=0;y<h;y++)for(size_t x=0;x<w;x++){float px=x+.5f,py=y+.5f;if(!(px>w*.7f&&px<w*.95f&&py>h*.05f&&py<h*.14f))continue;bool o=(int(px/8)%3==0)||py<h*.075f;for(int k=0;k<3;k++){float hi=o?.95f:k==1?.2f:.05f;float expect=o?hi:.25f*under.value(x,y,k,4)+.75f*hi;double d=std::abs(actual.value(x,y,k,4)-expect);max=std::max(max,d);if(o){opaque+=d*d;on++;}else{trans+=d*d;tn++;}}}printf("hud_composition opaque_rmse=%.8f translucent_rmse=%.8f max=%.8f opaque_samples=%zu translucent_samples=%zu\n",sqrt(opaque/on),sqrt(trans/tn),max,on,tn);}
static void png(Readback&r,size_t w,size_t h,NSString*name){std::vector<uint8_t>rgba(w*h*4);for(size_t y=0;y<h;y++)for(size_t x=0;x<w;x++){for(size_t k=0;k<3;k++)rgba[(y*w+x)*4+k]=uint8_t(std::lround(std::clamp(r.value(x,y,k,4),0.f,1.f)*255));rgba[(y*w+x)*4+3]=255;}auto cs=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);auto provider=CGDataProviderCreateWithData(nullptr,rgba.data(),rgba.size(),nullptr);auto image=CGImageCreate(w,h,8,32,w*4,cs,kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast,provider,nullptr,false,kCGRenderingIntentDefault);auto path=[exportRoot stringByAppendingPathComponent:name];auto dest=CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:path],CFSTR("public.png"),1,nullptr);if(!dest||!image)fail("PNG creation");CGImageDestinationAddImage(dest,image,nullptr);if(!CGImageDestinationFinalize(dest))fail("PNG finalize");printf("png=%s\n",path.UTF8String);CFRelease(dest);CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(cs);}
static int runLowlat(int w,int h,int frames){@autoreleasepool{
 if(w!=2560||h!=1655||frames!=16)fail("admitted lowlat scope is2560x1655x16");alarm(60);setbuf(stdout,nullptr);printf("route=lowlat-spatial2 native=%dx%d input=1280x828 generated=2560x1656 crop_bottom=1 frames=%d warmup=4 limit_bytes=%zu pid=%d fixture=nonperiodic-value-noise\n",w,h,frames,limitBytes,getpid());
 auto d=MTLCreateSystemDefaultDevice();auto queue=[d newCommandQueue];NSError*error=nil;
 printf("support=%d max_input_dimension_spatial2=%ld max_input_pixels_spatial2=%ld\n",[VTLowLatencyFrameInterpolationConfiguration isSupported],(long)[VTLowLatencyFrameInterpolationConfiguration maximumDimensionForSpatialScaleFactor:2],(long)[VTLowLatencyFrameInterpolationConfiguration maximumPixelCountForSpatialScaleFactor:2]);
 auto config=[[VTLowLatencyFrameInterpolationConfiguration alloc]initWithFrameWidth:1280 frameHeight:828 spatialScaleFactor:2];if(!config)fail("lowlat spatial2 config nil");NSLog(@"source=%@ destination=%@",config.sourcePixelBufferAttributes,config.destinationPixelBufferAttributes);
 if([config.destinationPixelBufferAttributes[(id)kCVPixelBufferWidthKey]intValue]!=2560||[config.destinationPixelBufferAttributes[(id)kCVPixelBufferHeightKey]intValue]!=1656)fail("unexpected lowlat spatial output dimensions");
 auto processor=[VTFrameProcessor new];double t0=seconds();if(![processor startSessionWithConfiguration:config error:&error]){NSLog(@"%@",error);fail("lowlat session start");}printf("session_ms=%.4f footprint=%zu\n",(seconds()-t0)*1000,footprint());if(footprint()>limitBytes)fail("admitted memory exceeded after session");
 auto lib=[d newLibraryWithSource:[NSString stringWithUTF8String:msl]options:nil error:&error];if(!lib){NSLog(@"%@",error);fail("lowlat shaders");}
 auto encoder=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"rgbnv"]error:&error],decoder=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"nvrgb"]error:&error],over=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"overlay"]error:&error],convert=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"convert"]error:&error];if(!encoder||!decoder||!over||!convert)fail("lowlat compute pipelines");
 auto rawP=renderPipeline(d,lib,MTLPixelFormatBGRA8Unorm,false),truthP=renderPipeline(d,lib,MTLPixelFormatRGBA16Float,false);CVMetalTextureCacheRef cache=nullptr;if(CVMetalTextureCacheCreate(kCFAllocatorDefault,nullptr,d,nullptr,&cache))fail("lowlat cache");
 NVPixel prev(cache,config.sourcePixelBufferAttributes),curr(cache,config.sourcePixelBufferAttributes),mid(cache,config.destinationPixelBufferAttributes),scaledCurrent(cache,config.destinationPixelBufferAttributes);
 auto rawPrev=tex(d,w,h,MTLPixelFormatBGRA8Unorm),rawCurr=tex(d,w,h,MTLPixelFormatBGRA8Unorm),decoded=tex(d,w,h,MTLPixelFormatRGBA16Float),result=tex(d,w,h,MTLPixelFormatRGBA16Float),truth=tex(d,w,h,MTLPixelFormatRGBA16Float),hold=tex(d,w,h,MTLPixelFormatRGBA16Float),holdHUD=tex(d,w,h,MTLPixelFormatRGBA16Float);
 Readback actualRead(d,w,h,8),truthRead(d,w,h,8),underRead(d,w,h,8),holdRead(d,w,h,8);std::vector<double>gpu,wall;size_t peakMetal=d.currentAllocatedSize,peakPhys=footprint();double start=seconds();
 for(int f=0;f<frames;f++){@autoreleasepool{
  if(seconds()-start>55)fail("lowlat route deadline");auto cb=[queue commandBuffer];cb.label=[NSString stringWithFormat:@"bounded lowlat spatial2 %d",f];double begin=seconds();
  render(cb,rawP,nil,rawPrev,nil,{float(w),float(h),float(f),0});render(cb,rawP,nil,rawCurr,nil,{float(w),float(h),float(f+1),0});convertNV(cb,encoder,rawPrev,prev);convertNV(cb,encoder,rawCurr,curr);
  auto previous=[[VTFrameProcessorFrame alloc]initWithBuffer:prev.buffer presentationTimeStamp:CMTimeMake(f,30)],current=[[VTFrameProcessorFrame alloc]initWithBuffer:curr.buffer presentationTimeStamp:CMTimeMake(f+1,30)],middle=[[VTFrameProcessorFrame alloc]initWithBuffer:mid.buffer presentationTimeStamp:CMTimeMake(2*f+1,60)],scaled=[[VTFrameProcessorFrame alloc]initWithBuffer:scaledCurrent.buffer presentationTimeStamp:CMTimeMake(f+1,30)];
  auto parameters=[[VTLowLatencyFrameInterpolationParameters alloc]initWithSourceFrame:current previousFrame:previous interpolationPhase:@[@.5] destinationFrames:@[middle,scaled]];if(!parameters)fail("lowlat spatial parameters nil");[processor processWithCommandBuffer:cb parameters:parameters];decodeNV(cb,decoder,mid,decoded);Params hud{float(w),float(h),float(f)+.5f,1};dispatch(cb,over,decoded,result,&hud,sizeof(hud));
  if(f==frames-1){render(cb,truthP,nil,truth,nil,hud);dispatch(cb,convert,rawCurr,hold);dispatch(cb,over,hold,holdHUD,&hud,sizeof(hud));actualRead.copy(cb,result);truthRead.copy(cb,truth);underRead.copy(cb,decoded);holdRead.copy(cb,holdHUD);}
  [cb commit];[cb waitUntilCompleted];if(cb.status==MTLCommandBufferStatusError){NSLog(@"GPU error %@",cb.error);fail("lowlat GPU command error");}double gt=(cb.GPUEndTime-cb.GPUStartTime)*1000,wt=(seconds()-begin)*1000;peakMetal=std::max(peakMetal,size_t(d.currentAllocatedSize));peakPhys=std::max(peakPhys,footprint());printf("frame=%d phase=%s gpu_ms=%.4f wall_ms=%.4f metal_bytes=%lu footprint_bytes=%zu status=%lu\n",f,f<4?"warmup":f==frames-1?"readback":"warm",gt,wt,(unsigned long)d.currentAllocatedSize,footprint(),(unsigned long)cb.status);if(f>=4&&f<frames-1){gpu.push_back(gt);wall.push_back(wt);}if(peakMetal>limitBytes||peakPhys>limitBytes)fail("admitted lowlat memory exceeded");
 }}
 printf("generated_midpoint ");metrics(actualRead,truthRead,w,h);printf("hold_current_baseline ");metrics(holdRead,truthRead,w,h);hudMetrics(actualRead,underRead,w,h);png(actualRead,w,h,@"lowlat-actual-midpoint-t0.516667.png");png(truthRead,w,h,@"lowlat-truth-midpoint-t0.516667.png");png(holdRead,w,h,@"lowlat-native-current-t0.533333.png");
 std::sort(gpu.begin(),gpu.end());std::sort(wall.begin(),wall.end());struct rusage ru{};getrusage(RUSAGE_SELF,&ru);printf("summary warm_samples=%zu gpu_median_ms=%.4f gpu_max_ms=%.4f wall_median_ms=%.4f metal_peak_bytes=%zu footprint_peak_bytes=%zu maxrss_bytes=%ld route_s=%.4f\n",gpu.size(),gpu[gpu.size()/2],gpu.back(),wall[wall.size()/2],peakMetal,peakPhys,ru.ru_maxrss,seconds()-start);[processor endSession];CFRelease(cache);alarm(0);return 0;
}}
static void byteStats(const char*label,Readback&r,size_t widthBytes,size_t height,int sentinelValue=-1,bool uv=false){unsigned lo=255,hi=0;size_t changed=0,n=0;for(size_t y=0;y<height;y++){auto*p=(const uint8_t*)r.b.contents+y*r.pitch;for(size_t x=0;x<widthBytes;x++){lo=std::min(lo,unsigned(p[x]));hi=std::max(hi,unsigned(p[x]));if(sentinelValue>=0&&p[x]!=(uv?(x%2?192:64):sentinelValue))changed++;n++;}}printf("%s min=%u max=%u changed_from_sentinel=%zu total=%zu\n",label,lo,hi,changed,n);}
static int runLowlatTemporal(int w,int h,int frames){@autoreleasepool{
 if(w!=2560||h!=1655||frames!=16)fail("admitted temporal scope is2560x1655x16");alarm(60);setbuf(stdout,nullptr);printf("route=lowlat-temporal native=%dx%d NV12=1280x828 native_real_untouched=1 frames=%d warmup=4 limit_bytes=%zu pid=%d fixture=nonperiodic first_control=identical_inputs\n",w,h,frames,limitBytes,getpid());
 auto d=MTLCreateSystemDefaultDevice();auto queue=[d newCommandQueue];NSError*error=nil;
 auto md=[VTLowLatencyFrameInterpolationConfiguration maximumDimensionForSpatialScaleFactor:1],mp=[VTLowLatencyFrameInterpolationConfiguration maximumPixelCountForSpatialScaleFactor:1];printf("support=%d temporal_maxdimension=%ld temporal_maxpixels=%ld requested_pixels=%d\n",[VTLowLatencyFrameInterpolationConfiguration isSupported],(long)md,(long)mp,1280*828);if(1280>md||828>md||1280*828>mp)fail("temporal requested dimensions outside supported bounds");
 auto config=[[VTLowLatencyFrameInterpolationConfiguration alloc]initWithFrameWidth:1280 frameHeight:828 numberOfInterpolatedFrames:1];if(!config)fail("temporal configuration nil");NSLog(@"temporal source=%@ destination=%@",config.sourcePixelBufferAttributes,config.destinationPixelBufferAttributes);
 if([config.sourcePixelBufferAttributes[(id)kCVPixelBufferWidthKey]intValue]!=1280||[config.sourcePixelBufferAttributes[(id)kCVPixelBufferHeightKey]intValue]!=828||[config.destinationPixelBufferAttributes[(id)kCVPixelBufferWidthKey]intValue]!=1280||[config.destinationPixelBufferAttributes[(id)kCVPixelBufferHeightKey]intValue]!=828)fail("temporal source/destination attributes mismatch");
 auto processor=[VTFrameProcessor new];double t0=seconds();if(![processor startSessionWithConfiguration:config error:&error]){NSLog(@"%@",error);fail("temporal session start");}printf("session_ms=%.4f footprint=%zu\n",(seconds()-t0)*1000,footprint());if(footprint()>limitBytes)fail("temporal memory after startup");
 auto lib=[d newLibraryWithSource:[NSString stringWithUTF8String:msl]options:nil error:&error];if(!lib){NSLog(@"%@",error);fail("temporal shader compile");}
 auto enc=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"rgbnv"]error:&error],dec=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"nvrgbscale"]error:&error],over=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"overlay"]error:&error],convert=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"convert"]error:&error],seed=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"sentinel"]error:&error];if(!enc||!dec||!over||!convert||!seed)fail("temporal compute pipeline");
 auto rawP=renderPipeline(d,lib,MTLPixelFormatBGRA8Unorm,false),truthP=renderPipeline(d,lib,MTLPixelFormatRGBA16Float,false);CVMetalTextureCacheRef cache=nullptr;if(CVMetalTextureCacheCreate(kCFAllocatorDefault,nullptr,d,nullptr,&cache))fail("temporal texture cache");NVPixel prev(cache,config.sourcePixelBufferAttributes),curr(cache,config.sourcePixelBufferAttributes),mid(cache,config.destinationPixelBufferAttributes);
 auto rawPrev=tex(d,w,h,MTLPixelFormatBGRA8Unorm),rawCurr=tex(d,w,h,MTLPixelFormatBGRA8Unorm),decoded=tex(d,w,h,MTLPixelFormatBGRA8Unorm),result=tex(d,w,h,MTLPixelFormatRGBA16Float),truth=tex(d,w,h,MTLPixelFormatRGBA16Float),hold=tex(d,w,h,MTLPixelFormatRGBA16Float),holdHUD=tex(d,w,h,MTLPixelFormatRGBA16Float),under=tex(d,w,h,MTLPixelFormatRGBA16Float);
 Readback actualRead(d,w,h,8),truthRead(d,w,h,8),underRead(d,w,h,8),holdRead(d,w,h,8),sourceY(d,1280,828,1),middleY(d,1280,828,1),middleUV(d,640,414,2);std::vector<double>gpu,wall;size_t peakMetal=d.currentAllocatedSize,peakPhys=footprint();double start=seconds();
 for(int f=0;f<frames;f++){@autoreleasepool{
  if(seconds()-start>55)fail("temporal route deadline");auto cb=[queue commandBuffer];cb.label=[NSString stringWithFormat:@"bounded temporal %d",f];double begin=seconds();float currentTime=f?float(f+1):0.f,midTime=f?float(f)+.5f:0.f;
  render(cb,rawP,nil,rawPrev,nil,{float(w),float(h),float(f),0});render(cb,rawP,nil,rawCurr,nil,{float(w),float(h),currentTime,0});convertNV(cb,enc,rawPrev,prev);convertNV(cb,enc,rawCurr,curr);
  auto se=[cb computeCommandEncoder];[se setComputePipelineState:seed];[se setTexture:mid.y atIndex:0];[se setTexture:mid.uv atIndex:1];[se dispatchThreads:MTLSizeMake(1280,828,1)threadsPerThreadgroup:MTLSizeMake(16,16,1)];[se endEncoding];
  auto previous=[[VTFrameProcessorFrame alloc]initWithBuffer:prev.buffer presentationTimeStamp:CMTimeMake(f,30)],current=[[VTFrameProcessorFrame alloc]initWithBuffer:curr.buffer presentationTimeStamp:CMTimeMake(f+1,30)],middle=[[VTFrameProcessorFrame alloc]initWithBuffer:mid.buffer presentationTimeStamp:CMTimeMake(2*f+1,60)];auto parameters=[[VTLowLatencyFrameInterpolationParameters alloc]initWithSourceFrame:current previousFrame:previous interpolationPhase:@[@.5]destinationFrames:@[middle]];if(!parameters||parameters.destinationFrames.count!=1||parameters.interpolationPhase.count!=1)fail("temporal one-destination parameters invalid");
  [processor processWithCommandBuffer:cb parameters:parameters];decodeNV(cb,dec,mid,decoded);Params hud{float(w),float(h),midTime,1};dispatch(cb,over,decoded,result,&hud,sizeof(hud));
  if(f==0||f==frames-1){render(cb,truthP,nil,truth,nil,hud);dispatch(cb,convert,rawCurr,hold);dispatch(cb,over,hold,holdHUD,&hud,sizeof(hud));dispatch(cb,convert,decoded,under);actualRead.copy(cb,result);truthRead.copy(cb,truth);underRead.copy(cb,under);holdRead.copy(cb,holdHUD);sourceY.copy(cb,curr.y);middleY.copy(cb,mid.y);middleUV.copy(cb,mid.uv);}
  [cb commit];[cb waitUntilCompleted];if(cb.status==MTLCommandBufferStatusError){NSLog(@"temporal GPU error %@",cb.error);fail("temporal GPU command error");}double gt=(cb.GPUEndTime-cb.GPUStartTime)*1000,wt=(seconds()-begin)*1000;peakMetal=std::max(peakMetal,size_t(d.currentAllocatedSize));peakPhys=std::max(peakPhys,footprint());printf("frame=%d phase=%s gpu_ms=%.4f wall_ms=%.4f metal_bytes=%lu footprint_bytes=%zu status=%lu\n",f,f<4?"warmup":f==frames-1?"readback":"warm",gt,wt,(unsigned long)d.currentAllocatedSize,footprint(),(unsigned long)cb.status);if(f>=4&&f<frames-1){gpu.push_back(gt);wall.push_back(wt);}if(peakMetal>limitBytes||peakPhys>limitBytes)fail("admitted temporal memory exceeded");
  if(f==0||f==frames-1){printf("control=%s\n",f==0?"identical-inputs":"moving-inputs");byteStats("source_Y",sourceY,1280,828);byteStats("destination_Y",middleY,1280,828,96);byteStats("destination_UV",middleUV,1280,414,64,true);printf("generated ");metrics(actualRead,truthRead,w,h);printf("hold_current ");metrics(holdRead,truthRead,w,h);hudMetrics(actualRead,underRead,w,h);if(f==0){png(actualRead,w,h,@"temporal-identical-actual-t0.png");png(truthRead,w,h,@"temporal-identical-truth-t0.png");}else{png(actualRead,w,h,@"temporal-actual-midpoint-t0.516667.png");png(truthRead,w,h,@"temporal-truth-midpoint-t0.516667.png");png(holdRead,w,h,@"temporal-native-current-t0.533333.png");}}
 }}
 std::sort(gpu.begin(),gpu.end());std::sort(wall.begin(),wall.end());struct rusage ru{};getrusage(RUSAGE_SELF,&ru);printf("summary warm_samples=%zu gpu_median_ms=%.4f gpu_max_ms=%.4f wall_median_ms=%.4f metal_peak_bytes=%zu footprint_peak_bytes=%zu maxrss_bytes=%ld route_s=%.4f\n",gpu.size(),gpu[gpu.size()/2],gpu.back(),wall[wall.size()/2],peakMetal,peakPhys,ru.ru_maxrss,seconds()-start);[processor endSession];CFRelease(cache);alarm(0);return 0;
}}
int main(int argc,char**argv){@autoreleasepool{
 exportRoot=[[NSString stringWithUTF8String:argv[0]]stringByDeletingLastPathComponent];
 if(argc<2)fail("usage probe flow|frc|lowlat|lowlat-temporal [width height frames memoryMiB]");std::string route=argv[1];if(route!="flow"&&route!="frc"&&route!="lowlat"&&route!="lowlat-temporal")fail("unknown route");
 int w=argc>2?atoi(argv[2]):2560,h=argc>3?atoi(argv[3]):1655,frames=argc>4?atoi(argv[4]):16;if(w<=0||h<=0||frames<4||frames>32)fail("bounds");
 int memoryMiB=argc>5?atoi(argv[5]):1024;if(memoryMiB<512||memoryMiB>3072)fail("memoryMiB must be512..3072");limitBytes=size_t(memoryMiB)*1024*1024;if(route=="lowlat")return runLowlat(w,h,frames);if(route=="lowlat-temporal")return runLowlatTemporal(w,h,frames);
 alarm(60);setbuf(stdout,nullptr);printf("route=%s size=%dx%d frames=%d pid=%d\n",route.c_str(),w,h,frames,getpid());
 auto device=MTLCreateSystemDefaultDevice();auto queue=[device newCommandQueue];NSError*error=nil;auto library=[device newLibraryWithSource:[NSString stringWithUTF8String:msl] options:nil error:&error];if(!library){NSLog(@"%@",error);fail("shader compile");}
 CVMetalTextureCacheRef cache=nullptr;if(CVMetalTextureCacheCreate(kCFAllocatorDefault,nullptr,device,nullptr,&cache))fail("texture cache");
 auto converter=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"convert"] error:&error];auto overlay=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"overlay"] error:&error];auto flowup=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"flowup"] error:&error];if(!converter||!overlay||!flowup)fail("compute pipeline");
 auto rawP=renderPipeline(device,library,MTLPixelFormatBGRA8Unorm,false),rawD=renderPipeline(device,library,MTLPixelFormatBGRA8Unorm,true),truthP=renderPipeline(device,library,MTLPixelFormatRGBA16Float,false);
 auto dsdesc=[MTLDepthStencilDescriptor new];dsdesc.depthCompareFunction=MTLCompareFunctionAlways;dsdesc.depthWriteEnabled=YES;auto ds=[device newDepthStencilStateWithDescriptor:dsdesc];
 auto processor=[VTFrameProcessor new];id<VTFrameProcessorConfiguration> config=nil;
 if(route=="flow")config=[[VTOpticalFlowConfiguration alloc]initWithFrameWidth:w frameHeight:h qualityPrioritization:VTOpticalFlowConfigurationQualityPrioritizationNormal revision:VTOpticalFlowConfiguration.defaultRevision];
 else config=[[VTFrameRateConversionConfiguration alloc]initWithFrameWidth:w frameHeight:h usePrecomputedFlow:NO qualityPrioritization:VTFrameRateConversionConfigurationQualityPrioritizationNormal revision:VTFrameRateConversionConfiguration.defaultRevision];
 if(!config)fail("configuration nil");NSLog(@"source=%@ destination=%@",[(id)config sourcePixelBufferAttributes],[(id)config destinationPixelBufferAttributes]);
 double start=seconds();if(![processor startSessionWithConfiguration:config error:&error]){NSLog(@"%@",error);fail("startSession");}printf("session_ms=%.3f\n",(seconds()-start)*1000);
 auto sourceAttrs=[(id)config sourcePixelBufferAttributes];Pixel prev(device,cache,sourceAttrs,MTLPixelFormatRGBA16Float),curr(device,cache,sourceAttrs,MTLPixelFormatRGBA16Float);
 NSDictionary*fullAttrs=@{(id)kCVPixelBufferWidthKey:@(w),(id)kCVPixelBufferHeightKey:@(h),(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_64RGBAHalf),(id)kCVPixelBufferIOSurfacePropertiesKey:@{}};
 Pixel out(device,cache,fullAttrs,MTLPixelFormatRGBA16Float);
 auto rawPrev=tex(device,w,h,MTLPixelFormatBGRA8Unorm),rawCurr=tex(device,w,h,MTLPixelFormatBGRA8Unorm),truth=tex(device,w,h,MTLPixelFormatRGBA16Float),composed=tex(device,w,h,MTLPixelFormatRGBA16Float),result=tex(device,w,h,MTLPixelFormatRGBA16Float);
 auto zdesc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:w height:h mipmapped:NO];zdesc.storageMode=MTLStorageModePrivate;zdesc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;auto depth=[device newTextureWithDescriptor:zdesc];
 Pixel*forward=nullptr;Pixel*backward=nullptr;id<MTLTexture>motion=nil,fiOut=nil;id<MTLFXFrameInterpolator>fi=nil;Readback*flowRead=nullptr;
 if(route=="flow"){
  auto a=[(id)config destinationPixelBufferAttributes];forward=new Pixel(device,cache,a,MTLPixelFormatRG16Float);backward=new Pixel(device,cache,a,MTLPixelFormatRG16Float);motion=tex(device,w,h,MTLPixelFormatRG16Float);flowRead=new Readback(device,backward->texture.width,backward->texture.height,4);
  auto d=[MTLFXFrameInterpolatorDescriptor new];d.inputWidth=w;d.inputHeight=h;d.outputWidth=w;d.outputHeight=h;d.colorTextureFormat=MTLPixelFormatRGBA16Float;d.outputTextureFormat=MTLPixelFormatRGBA16Float;d.depthTextureFormat=MTLPixelFormatDepth32Float;d.motionTextureFormat=MTLPixelFormatRG16Float;d.uiTextureFormat=MTLPixelFormatRGBA16Float;d.requiresPrevColorTexture=YES;
  fi=[d newFrameInterpolatorWithDevice:device];if(!fi)fail("FI creation");fiOut=tex(device,w,h,MTLPixelFormatRGBA16Float);fi.depthReversed=NO;fi.nearPlane=1;fi.farPlane=100;fi.fieldOfView=65;fi.aspectRatio=float(w)/h;fi.deltaTime=1.f/30;fi.motionVectorScaleX=fi.motionVectorScaleY=1;fi.colorTexture=curr.texture;fi.prevColorTexture=prev.texture;fi.depthTexture=depth;fi.motionTexture=motion;fi.outputTexture=fiOut;fi.uiTexture=composed;fi.uiTextureComposited=YES;
 }
 Readback outputRead(device,w,h,8),truthRead(device,w,h,8);std::vector<double>gpu,cpu;float scale[2]={8,8};size_t peakMetal=device.currentAllocatedSize,peakPhys=footprint();double routeStart=seconds();
 for(int f=0;f<frames;f++){@autoreleasepool{
  if(seconds()-routeStart>55)fail("route deadline");auto command=[queue commandBuffer];command.label=[NSString stringWithFormat:@"bounded %@ %d",[NSString stringWithUTF8String:route.c_str()],f];double begin=seconds();
  render(command,rawP,ds,rawPrev,nil,{float(w),float(h),float(f),0});render(command,rawD,ds,rawCurr,depth,{float(w),float(h),float(f+1),0});
  dispatch(command,converter,rawPrev,prev.texture);dispatch(command,converter,rawCurr,curr.texture);
  auto p=[[VTFrameProcessorFrame alloc]initWithBuffer:prev.buffer presentationTimeStamp:CMTimeMake(f,30)];auto n=[[VTFrameProcessorFrame alloc]initWithBuffer:curr.buffer presentationTimeStamp:CMTimeMake(f+1,30)];
  if(route=="flow"){
   auto pair=[[VTFrameProcessorOpticalFlow alloc]initWithForwardFlow:forward->buffer backwardFlow:backward->buffer];auto parameters=[[VTOpticalFlowParameters alloc]initWithSourceFrame:p nextFrame:n submissionMode:f?VTOpticalFlowParametersSubmissionModeSequential:VTOpticalFlowParametersSubmissionModeRandom destinationOpticalFlow:pair];if(!parameters)fail("flow parameters nil");[processor processWithCommandBuffer:command parameters:parameters];
   if(f==0)flowRead->copy(command,backward->texture);
   else{dispatch(command,flowup,backward->texture,motion,scale,sizeof(scale));Params hud{float(w),float(h),float(f+1),1};dispatch(command,overlay,curr.texture,composed,&hud,sizeof(hud));fi.shouldResetHistory=f==1;[fi encodeToCommandBuffer:command];}
  }else{
   auto dest=[[VTFrameProcessorFrame alloc]initWithBuffer:out.buffer presentationTimeStamp:CMTimeMake(f*2+1,60)];auto parameters=[[VTFrameRateConversionParameters alloc]initWithSourceFrame:p nextFrame:n opticalFlow:nil interpolationPhase:@[@0.5] submissionMode:f?VTFrameRateConversionParametersSubmissionModeSequential:VTFrameRateConversionParametersSubmissionModeRandom destinationFrames:@[dest]];if(!parameters)fail("FRC parameters nil");[processor processWithCommandBuffer:command parameters:parameters];
   Params hud{float(w),float(h),float(f)+.5f,1};dispatch(command,overlay,out.texture,result,&hud,sizeof(hud));
  }
  if(f==frames-1){render(command,truthP,ds,truth,nil,{float(w),float(h),float(f)+.5f,1});outputRead.copy(command,route=="flow"?fiOut:result);truthRead.copy(command,truth);}
  [command commit];[command waitUntilCompleted];if(command.status==MTLCommandBufferStatusError){NSLog(@"GPU error %@",command.error);fail("command error");}
  double elapsed=(seconds()-begin)*1000,gt=(command.GPUEndTime-command.GPUStartTime)*1000;printf("frame=%d gpu_ms=%.4f wall_ms=%.4f status=%lu\n",f,gt,elapsed,(unsigned long)command.status);
  if(f>=3&&f<frames-1){gpu.push_back(gt);cpu.push_back(elapsed);}peakMetal=std::max(peakMetal,size_t(device.currentAllocatedSize));peakPhys=std::max(peakPhys,footprint());printf("memory metal_bytes=%lu footprint_bytes=%zu\n",(unsigned long)device.currentAllocatedSize,footprint());if(peakMetal>limitBytes||peakPhys>limitBytes)fail("1GiB budget exceeded");
  if(route=="flow"&&f==0){float fw=backward->texture.width,fh=backward->texture.height;float ox=mean(*flowRead,fw*.35f+16.f*fw/w,fh*.48f+6.f*fh/h,0),oy=mean(*flowRead,fw*.35f+16.f*fw/w,fh*.48f+6.f*fh/h,1);float bx=mean(*flowRead,fw*.8f,fh*.65f,0),by=mean(*flowRead,fw*.8f,fh*.65f,1);printf("backward_flow dims=%lux%lu object=(%.6f,%.6f) expected=(-16,-6) background=(%.6f,%.6f) expected=(-4,-2)\n",(unsigned long)backward->texture.width,(unsigned long)backward->texture.height,ox,oy,bx,by);
   if(!std::isfinite(ox)||!std::isfinite(oy)||std::abs(ox)<.01||std::abs(oy)<.01)fail("invalid flow samples");
   printf("flow_scale=(%.1f,%.1f) UNVERIFIED no one-sample inference; background_scaled=(%.4f,%.4f)\n",scale[0],scale[1],bx*scale[0],by*scale[1]);
  }
 }}
 metrics(outputRead,truthRead,w,h);std::sort(gpu.begin(),gpu.end());std::sort(cpu.begin(),cpu.end());struct rusage ru{};getrusage(RUSAGE_SELF,&ru);printf("summary gpu_median_ms=%.4f gpu_max_ms=%.4f wall_median_ms=%.4f metal_peak_bytes=%zu footprint_peak_bytes=%zu maxrss_bytes=%ld route_s=%.3f\n",gpu[gpu.size()/2],gpu.back(),cpu[cpu.size()/2],peakMetal,peakPhys,ru.ru_maxrss,seconds()-routeStart);
 [processor endSession];delete flowRead;delete forward;delete backward;CFRelease(cache);alarm(0);return 0;
}}

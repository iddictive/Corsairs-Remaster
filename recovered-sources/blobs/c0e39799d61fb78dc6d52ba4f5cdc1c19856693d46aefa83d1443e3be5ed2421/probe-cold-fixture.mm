#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>
#import <VideoToolbox/VideoToolbox.h>
#import <CoreVideo/CoreVideo.h>
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

static constexpr size_t limitBytes=1024ull*1024*1024;
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
float4 scene(float2 p,constant P&u){
 float2 q=p-float2(4,2)*u.t;
 float c=((int(floor(q.x/20))^int(floor(q.y/20)))&1)?.25:.08;
 float3 rgb=float3(c+.06*sin(q.x*.07),c+.04*cos(q.y*.09),c+.1);
 float2 o=p-(float2(u.w*.35,u.h*.48)+float2(16,6)*u.t);
 if(abs(o.x)<u.w*.12&&abs(o.y)<u.h*.15){float z=((int(floor(o.x/12))^int(floor(o.y/12)))&1)?.8:.35;rgb=float3(z,.12+.09*cos(o.x*.1),.1+.1*sin(o.y*.1));}
 return float4(rgb,1);
}
float4 hud(float4 c,float2 p,constant P&u){
 if(p.x>u.w*.7&&p.x<u.w*.95&&p.y>u.h*.05&&p.y<u.h*.14){float3 h=(int(p.x/8)%3==0)?float3(.95,.95,.95):float3(.05,.2,.05);return float4(mix(c.rgb,h,.75),1);}return c;
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
)MSL";
struct Params{float w,h,t,hud;};
static id<MTLTexture> tex(id<MTLDevice>d,NSUInteger w,NSUInteger h,MTLPixelFormat f){auto q=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f width:w height:h mipmapped:NO];q.storageMode=MTLStorageModePrivate;q.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;return[d newTextureWithDescriptor:q];}
static void dispatch(id<MTLCommandBuffer>c,id<MTLComputePipelineState>p,id<MTLTexture>s,id<MTLTexture>d,const void*b=nullptr,size_t n=0){auto e=[c computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:s atIndex:0];[e setTexture:d atIndex:1];if(n)[e setBytes:b length:n atIndex:0];[e dispatchThreads:MTLSizeMake(d.width,d.height,1) threadsPerThreadgroup:MTLSizeMake(16,16,1)];[e endEncoding];}
static id<MTLRenderPipelineState> renderPipeline(id<MTLDevice>d,id<MTLLibrary>l,MTLPixelFormat f,bool depth){auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[l newFunctionWithName:@"vs"];p.fragmentFunction=[l newFunctionWithName:depth?@"fs":@"colorfs"];p.colorAttachments[0].pixelFormat=f;if(depth)p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;NSError*error=nil;auto r=[d newRenderPipelineStateWithDescriptor:p error:&error];if(!r){NSLog(@"%@",error);fail("render pipeline");}return r;}
static void render(id<MTLCommandBuffer>c,id<MTLRenderPipelineState>p,id<MTLDepthStencilState>ds,id<MTLTexture>t,id<MTLTexture>z,Params u){auto r=[MTLRenderPassDescriptor renderPassDescriptor];r.colorAttachments[0].texture=t;r.colorAttachments[0].loadAction=MTLLoadActionDontCare;r.colorAttachments[0].storeAction=MTLStoreActionStore;if(z){r.depthAttachment.texture=z;r.depthAttachment.loadAction=MTLLoadActionClear;r.depthAttachment.storeAction=MTLStoreActionStore;r.depthAttachment.clearDepth=1;}auto e=[c renderCommandEncoderWithDescriptor:r];[e setRenderPipelineState:p];if(z)[e setDepthStencilState:ds];[e setFragmentBytes:&u length:sizeof(u) atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];}
struct Readback{ id<MTLBuffer> b=nil;size_t pitch=0;Readback(id<MTLDevice>d,size_t w,size_t h,size_t bytes){pitch=((w*bytes+255)/256)*256;b=[d newBufferWithLength:pitch*h options:MTLResourceStorageModeShared];}void copy(id<MTLCommandBuffer>c,id<MTLTexture>t){auto e=[c blitCommandEncoder];[e copyFromTexture:t sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(t.width,t.height,1) toBuffer:b destinationOffset:0 destinationBytesPerRow:pitch destinationBytesPerImage:pitch*t.height];[e endEncoding];}float value(size_t x,size_t y,size_t ch,size_t channels){auto p=(const _Float16*)((const char*)b.contents+y*pitch);return(float)p[x*channels+ch];}};
static float mean(Readback&r,float x,float y,int channel){float sum=0;for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)sum+=r.value(size_t(x)+dx,size_t(y)+dy,channel,2);return sum/25;}
static void metrics(Readback&actual,Readback&truth,size_t w,size_t h){double sum=0,max=0;size_t bad=0,n=0;double hudsum=0;size_t hn=0;for(size_t y=0;y<h;y+=2)for(size_t x=0;x<w;x+=2)for(size_t k=0;k<3;k++){float a=actual.value(x,y,k,4),b=truth.value(x,y,k,4);if(!std::isfinite(a)){bad++;continue;}double d=std::abs(a-b);sum+=d*d;max=std::max(max,d);n++;if(x>w*.71&&x<w*.94&&y>h*.06&&y<h*.13){hudsum+=d*d;hn++;}}printf("metrics rmse=%.7f max_error=%.7f nonfinite=%zu hud_rmse=%.7f samples=%zu\n",sqrt(sum/std::max<size_t>(n,1)),max,bad,sqrt(hudsum/std::max<size_t>(hn,1)),n);}
int main(int argc,char**argv){@autoreleasepool{
 if(argc<2)fail("usage probe flow|frc [width height frames]");std::string route=argv[1];if(route!="flow"&&route!="frc")fail("unknown route");
 int w=argc>2?atoi(argv[2]):2560,h=argc>3?atoi(argv[3]):1655,frames=argc>4?atoi(argv[4]):16;if(w<=0||h<=0||frames<4||frames>32)fail("bounds");
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
   float sx=-16.f/ox,sy=-6.f/oy;float candidates[]={-2560,-1655,-8,-4,-2,-1,1,2,4,8,1655,2560};auto nearest=[&](float s){float best=candidates[0];for(float c:candidates)if(std::abs(c-s)<std::abs(best-s))best=c;return best;};scale[0]=nearest(sx);scale[1]=nearest(sy);printf("flow_scale inferred=(%.6f,%.6f) selected=(%.1f,%.1f) background_scaled=(%.4f,%.4f)\n",sx,sy,scale[0],scale[1],bx*scale[0],by*scale[1]);
  }
 }}
 metrics(outputRead,truthRead,w,h);std::sort(gpu.begin(),gpu.end());std::sort(cpu.begin(),cpu.end());struct rusage ru{};getrusage(RUSAGE_SELF,&ru);printf("summary gpu_median_ms=%.4f gpu_max_ms=%.4f wall_median_ms=%.4f metal_peak_bytes=%zu footprint_peak_bytes=%zu maxrss_bytes=%ld route_s=%.3f\n",gpu[gpu.size()/2],gpu.back(),cpu[cpu.size()/2],peakMetal,peakPhys,ru.ru_maxrss,seconds()-routeStart);
 [processor endSession];delete flowRead;delete forward;delete backward;CFRelease(cache);alarm(0);return 0;
}}

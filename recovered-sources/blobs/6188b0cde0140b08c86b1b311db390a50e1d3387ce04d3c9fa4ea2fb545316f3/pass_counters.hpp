#pragma once
#import <Metal/Metal.h>
#import <objc/runtime.h>
#include <dlfcn.h>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <string>
namespace pass_probe {
struct Pass {unsigned start;unsigned width,height,samples;std::string label;};
struct Frame {id<MTLCounterSampleBuffer> samples=nil;unsigned id=0,next=0;std::vector<Pass> passes;};
inline std::mutex mutex;
inline std::map<void*,std::shared_ptr<Frame>> frames;
inline std::map<void*,std::pair<std::shared_ptr<Frame>,unsigned>> encoders;
inline unsigned sequence=0;
inline FILE* output=nullptr;
inline bool attempted=false;
inline IMP originalRender=nullptr;
inline std::map<Class,IMP> originalEnd;
void install(id<MTLCommandQueue>);
inline std::shared_ptr<Frame> prepare(id<MTLCommandBuffer> cb) {
 std::lock_guard guard(mutex);auto found=frames.find((__bridge void*)cb);if(found!=frames.end())return found->second;
 if(!attempted){attempted=true;const char*path=getenv("STORM_GPU_PASS_TRACE");if(path)output=fopen(path,"w");if(output)fprintf(output,"command,pass,width,height,samples,label,vertex_start,vertex_end,fragment_start,fragment_end,command_gpu_start_s,command_gpu_end_s\n");}
 if(!output||![cb.device supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary])return {};
 id<MTLCounterSet> timestamps=nil;for(id<MTLCounterSet> set in cb.device.counterSets)if([set.name isEqualToString:MTLCommonCounterSetTimestamp])timestamps=set;
 if(!timestamps)return {};
 auto descriptor=[MTLCounterSampleBufferDescriptor new];descriptor.counterSet=timestamps;descriptor.storageMode=MTLStorageModeShared;descriptor.sampleCount=4096;NSError*error=nil;
 auto state=std::make_shared<Frame>();state->samples=[cb.device newCounterSampleBufferWithDescriptor:descriptor error:&error];state->id=sequence++;
 if(!state->samples){fprintf(stderr,"PASS_COUNTER_ERROR %s\n",error.localizedDescription.UTF8String);return {};}
 frames[(__bridge void*)cb]=state;
 [cb addCompletedHandler:^(id<MTLCommandBuffer> done){
  auto data=[state->samples resolveCounterRange:NSMakeRange(0,state->next)];
  std::lock_guard guard(mutex);
  if(data.length>=state->next*sizeof(MTLCounterResultTimestamp)){
   const auto*values=(const MTLCounterResultTimestamp*)data.bytes;
   for(size_t i=0;i<state->passes.size();i++){const auto&p=state->passes[i];fprintf(output,"%u,%zu,%u,%u,%u,%s,%llu,%llu,%llu,%llu,%.9f,%.9f\n",state->id,i,p.width,p.height,p.samples,p.label.c_str(),values[p.start].timestamp,values[p.start+1].timestamp,values[p.start+2].timestamp,values[p.start+3].timestamp,done.GPUStartTime,done.GPUEndTime);}
   fflush(output);
  }else fprintf(stderr,"PASS_COUNTER_RESOLVE_ERROR command=%u count=%u bytes=%lu\n",state->id,state->next,(unsigned long)data.length);
  frames.erase((__bridge void*)done);
 }];
 return state;
}
}
@interface NSObject (PassCounterProbe)
-(id<MTLRenderCommandEncoder>)probeRenderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor*)pass;
-(void)probeEndEncoding;
@end
@implementation NSObject (PassCounterProbe)
-(id<MTLRenderCommandEncoder>)probeRenderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor*)pass {
 auto cb=(id<MTLCommandBuffer>)self;auto state=pass_probe::prepare(cb);unsigned index=0;
 if(state&&state->next+4<=4096){index=state->next;state->next+=4;auto attachment=pass.sampleBufferAttachments[0];attachment.sampleBuffer=state->samples;attachment.startOfVertexSampleIndex=index;attachment.endOfVertexSampleIndex=index+1;attachment.startOfFragmentSampleIndex=index+2;attachment.endOfFragmentSampleIndex=index+3;}
 else state.reset();
 auto encoder=((id<MTLRenderCommandEncoder>(*)(id,SEL,MTLRenderPassDescriptor*))pass_probe::originalRender)(cb,@selector(renderCommandEncoderWithDescriptor:),pass);
 if(encoder&&state){Dl_info caller{};dladdr(__builtin_return_address(0),&caller);auto texture=pass.colorAttachments[0].texture?:pass.depthAttachment.texture;std::lock_guard guard(pass_probe::mutex);auto n=unsigned(state->passes.size());state->passes.push_back({index,unsigned(texture.width),unsigned(texture.height),unsigned(texture.sampleCount),caller.dli_sname?caller.dli_sname:"unlabelled"});pass_probe::encoders[(__bridge void*)encoder]={state,n};
  Class cls=object_getClass(encoder);if(!pass_probe::originalEnd.count(cls)){auto method=class_getInstanceMethod(cls,@selector(endEncoding));auto replacement=class_getInstanceMethod([NSObject class],@selector(probeEndEncoding));pass_probe::originalEnd[cls]=method_getImplementation(method);if(!class_addMethod(cls,@selector(endEncoding),method_getImplementation(replacement),method_getTypeEncoding(method)))method_setImplementation(method,method_getImplementation(replacement));}
 }
 return encoder;
}
-(void)probeEndEncoding {
 {std::lock_guard guard(pass_probe::mutex);auto found=pass_probe::encoders.find((__bridge void*)self);if(found!=pass_probe::encoders.end()){auto name=((id<MTLRenderCommandEncoder>)self).label;if(name)found->second.first->passes[found->second.second].label=name.UTF8String;pass_probe::encoders.erase(found);}}
 ((void(*)(id,SEL))pass_probe::originalEnd.at(object_getClass(self)))(self,@selector(endEncoding));
}
@end
namespace pass_probe {
inline void install(id<MTLCommandQueue> queue){auto cb=[queue commandBuffer];Class cls=object_getClass(cb);auto method=class_getInstanceMethod(cls,@selector(renderCommandEncoderWithDescriptor:));auto replacement=class_getInstanceMethod([NSObject class],@selector(probeRenderCommandEncoderWithDescriptor:));originalRender=method_getImplementation(method);if(!class_addMethod(cls,@selector(renderCommandEncoderWithDescriptor:),method_getImplementation(replacement),method_getTypeEncoding(method)))method_setImplementation(method,method_getImplementation(replacement));fprintf(stderr,"PASS_COUNTER_INSTALLED class=%s stage_boundary=%d\n",class_getName(cls),[queue.device supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary]);}
}

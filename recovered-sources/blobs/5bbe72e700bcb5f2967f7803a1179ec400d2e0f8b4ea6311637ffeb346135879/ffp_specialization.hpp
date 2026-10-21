#pragma once
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace storm_metal {
// Only dispatch state is constant. Texture presence, alpha coverage, fog,
// colours, transforms and lighting continue to consume the final Uniform.
inline constexpr const char* ffpSpecializationMSL=R"MSL(
constant uint ffpStageCount [[function_constant(0)]];
constant uint ffpAlphaMode [[function_constant(1)]];
constant uint ffpSpecularMode [[function_constant(2)]];
constant uint4 ffp0_color [[function_constant(4)]];
constant uint4 ffp0_alpha [[function_constant(5)]];
constant uint ffp0_coord [[function_constant(6)]];
constant uint ffp0_textured [[function_constant(7)]];
constant uint ffp0_result [[function_constant(8)]];
constant uint ffp0_transformFlags [[function_constant(9)]];
constant uint ffp0_stochastic [[function_constant(10)]];
constant uint4 ffp1_color [[function_constant(11)]];
constant uint4 ffp1_alpha [[function_constant(12)]];
constant uint ffp1_coord [[function_constant(13)]];
constant uint ffp1_textured [[function_constant(14)]];
constant uint ffp1_result [[function_constant(15)]];
constant uint ffp1_transformFlags [[function_constant(16)]];
constant uint ffp1_stochastic [[function_constant(17)]];
Stage ffpStaticStage(uint i,Stage st){
 if(i==0){
  if(is_function_constant_defined(ffp0_color))st.color=ffp0_color;
  if(is_function_constant_defined(ffp0_alpha))st.alpha=ffp0_alpha;
  if(is_function_constant_defined(ffp0_coord))st.coord=ffp0_coord;
  if(is_function_constant_defined(ffp0_textured))st.textured=ffp0_textured;
  if(is_function_constant_defined(ffp0_result))st.result=ffp0_result;
  if(is_function_constant_defined(ffp0_transformFlags))st.transformFlags=ffp0_transformFlags;
  if(is_function_constant_defined(ffp0_stochastic))st.stochastic=ffp0_stochastic;
 }else if(i==1){
  if(is_function_constant_defined(ffp1_color))st.color=ffp1_color;
  if(is_function_constant_defined(ffp1_alpha))st.alpha=ffp1_alpha;
  if(is_function_constant_defined(ffp1_coord))st.coord=ffp1_coord;
  if(is_function_constant_defined(ffp1_textured))st.textured=ffp1_textured;
  if(is_function_constant_defined(ffp1_result))st.result=ffp1_result;
  if(is_function_constant_defined(ffp1_transformFlags))st.transformFlags=ffp1_transformFlags;
  if(is_function_constant_defined(ffp1_stochastic))st.stochastic=ffp1_stochastic;
 }
 return st;
}
)MSL";

class FfpSpecialization {
public:
 using Material=std::array<uint32_t,29>; // count, alpha/specular + two stages
 static bool wide(){static const bool value=std::getenv("STORM_PROBE_FFP_WIDE")!=nullptr;return value;}
private:
 using Key=std::pair<uint64_t,Material>;
 struct Entry {id<MTLRenderPipelineState> pipeline=nil;};
 struct State {
  std::mutex mutex;
  std::map<Key,Entry> entries;
  bool cancelled=false;
 };
 std::shared_ptr<State> state=std::make_shared<State>();
 dispatch_queue_t compiler=dispatch_queue_create("storm.metal.material-compiler",DISPATCH_QUEUE_SERIAL);
 bool enabled=[](){const char*mode=std::getenv("STORM_METAL_FFP_SPECIALIZATION");return !mode||std::strcmp(mode,"0")!=0;}();
 uint64_t frames=0,used=0,fallback=0;
public:
 template<class Uniform> static Material material(const Uniform&u){
  Material result{};uint32_t count=0;
  while(count<8&&u.stages[count].color.x!=1)++count;
  result[0]=count;
  if(wide()){result[1]=u.alphaTest;result[2]=u.specularEnable;}
  if(count>2)return result;
  for(uint32_t i=0;i<count;++i){const auto&s=u.stages[i];auto*p=result.data()+3+i*13;
   for(unsigned q=0;q<4;++q){p[q]=s.color[q];p[4+q]=s.alpha[q];}
   p[8]=s.coord;p[9]=s.result;p[10]=s.transformFlags;
   if(wide()){p[11]=s.textured;p[12]=s.stochastic;}
  }
  return result;
 }
 static MTLFunctionConstantValues*constants(const Material&material){
  auto*values=[MTLFunctionConstantValues new];
  if(material[0]>2)return values;
  [values setConstantValue:&material[0] type:MTLDataTypeUInt atIndex:0];
  if(wide()){
   [values setConstantValue:&material[1] type:MTLDataTypeUInt atIndex:1];
   [values setConstantValue:&material[2] type:MTLDataTypeUInt atIndex:2];
  }
  for(unsigned i=0;i<material[0];++i){const auto*p=material.data()+3+i*13;const NSUInteger index=4+i*7;
   [values setConstantValue:p type:MTLDataTypeUInt4 atIndex:index];
   [values setConstantValue:p+4 type:MTLDataTypeUInt4 atIndex:index+1];
   [values setConstantValue:p+8 type:MTLDataTypeUInt atIndex:index+2];
   [values setConstantValue:p+9 type:MTLDataTypeUInt atIndex:index+4];
   [values setConstantValue:p+10 type:MTLDataTypeUInt atIndex:index+5];
   if(wide()){
    [values setConstantValue:p+11 type:MTLDataTypeUInt atIndex:index+3];
    [values setConstantValue:p+12 type:MTLDataTypeUInt atIndex:index+6];
   }
  }
  return values;
 }
 bool active()const{return enabled;}
 void drain(){dispatch_sync(compiler,^{});}
 void reset(){
  {std::lock_guard lock(state->mutex);state->cancelled=true;}
  drain();state=std::make_shared<State>();frames=used=fallback=0;
 }
 ~FfpSpecialization(){{std::lock_guard lock(state->mutex);state->cancelled=true;}drain();}
 id<MTLRenderPipelineState> pipeline(id<MTLDevice>device,id<MTLLibrary>library,
     MTLRenderPipelineDescriptor*base,uint64_t baseKey,const Material&material){
  if(!enabled||material[0]>2)return nil;
  const Key key{baseKey,material};const auto owner=state;
  {
   std::lock_guard lock(owner->mutex);
   auto found=owner->entries.find(key);
   if(found!=owner->entries.end()){
    if(found->second.pipeline){++used;return found->second.pipeline;}
    ++fallback;return nil;
   }
   ++fallback;
   // Pending and failed entries count too; a material flood cannot keep
   // allocating PSOs or retry a compiler failure every draw.
   if(owner->entries.size()>=128)return nil;
   owner->entries.emplace(key,Entry{});
  }
  MTLRenderPipelineDescriptor*descriptor=[base copy];NSString*name=base.fragmentFunction.name;
  dispatch_async(compiler, ^{
   @autoreleasepool {
    {std::lock_guard lock(owner->mutex);if(owner->cancelled)return;}
    auto values=constants(key.second);
    NSError*error=nil;
    descriptor.fragmentFunction=[library newFunctionWithName:name constantValues:values error:&error];
    id<MTLRenderPipelineState> compiled=descriptor.fragmentFunction?
        [device newRenderPipelineStateWithDescriptor:descriptor error:&error]:nil;
    if(!compiled)std::fprintf(stderr,"[StormMetal] material specialization %s: %s; generic retained\n",name.UTF8String,error.localizedDescription.UTF8String);
    // No Device pointer escapes. Reset/destruction retires this state while
    // the queued compile safely owns its Metal inputs and old cache alone.
    std::lock_guard lock(owner->mutex);
    if(!owner->cancelled)owner->entries.at(key).pipeline=compiled;
   }
  });
  return nil;
 }
 void report(bool profiling){
  if(!profiling||++frames<120)return;
  size_t ready=0,total=0;
  {std::lock_guard lock(state->mutex);total=state->entries.size();for(const auto&entry:state->entries)ready+=entry.second.pipeline!=nil;}
  std::fprintf(stderr,"[StormMetal] ffp-specialize enabled=%d frames=%llu used=%llu fallback=%llu ready=%zu variants=%zu\n",int(enabled),(unsigned long long)frames,(unsigned long long)used,(unsigned long long)fallback,ready,total);
  frames=used=fallback=0;
 }
};
}

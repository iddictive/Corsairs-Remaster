#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/frame_interpolation.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
#include <mach/mach.h>
#include <unistd.h>

static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void require(bool ok,const char* message) { if(!ok){fprintf(stderr,"FAIL %s\n",message);exit(1);} }
static size_t footprint() { task_vm_info_data_t v{};mach_msg_type_number_t n=TASK_VM_INFO_COUNT;task_info(mach_task_self(),TASK_VM_INFO,(task_info_t)&v,&n);return v.phys_footprint; }
static void ready(sm::FrameInterpolation& fi) {
    double start=now();while(fi.status()==sm::FrameInterpolation::Status::Loading&&now()-start<35)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    require(fi.status()==sm::FrameInterpolation::Status::Ready,sm::FrameInterpolation::statusName(fi.status()));
    printf("ready_ms=%.3f footprint=%zu\n",(now()-start)*1000,footprint());
}
static void completed(id<MTLCommandBuffer> cb) { [cb waitUntilCompleted];require(cb.status==MTLCommandBufferStatusCompleted,"command completed without GPU error"); }
int main() { @autoreleasepool {
    alarm(60);setbuf(stdout,nullptr);auto device=MTLCreateSystemDefaultDevice();require(device!=nil,"device");auto queue=[device newCommandQueue];
    printf("pid=%d device=%s measurement=incremental_generation_only no_game_fps_claim\n",getpid(),device.name.UTF8String);
    sm::FrameInterpolation fi;
    require(fi.status()==sm::FrameInterpolation::Status::Disabled,"initial disabled");
    fi.prepare(device,2560,1655,false);require(fi.status()==sm::FrameInterpolation::Status::Disabled,"disabled does not initialize");
    fi.prepare(device,2560,1655);fi.reset();require(fi.status()==sm::FrameInterpolation::Status::Disabled,"reset cancels pending setup");
    fi.prepare(device,8192,8192);double limit=now()+35;while(fi.status()==sm::FrameInterpolation::Status::Loading&&now()<limit)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    require(fi.status()==sm::FrameInterpolation::Status::Unavailable,"over-cap input falls back before textures/session");
    fi.prepare(device,2560,1655);ready(fi);
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:2560 height:1655 mipmapped:NO];
    descriptor.storageMode=MTLStorageModeShared;descriptor.usage=MTLTextureUsageShaderRead;
    std::array<id<MTLTexture>,3> inputs{};std::vector<uint32_t> pixels(2560*1655);
    for(unsigned s=0;s<3;s++) {
        for(unsigned y=0;y<1655;y++)for(unsigned x=0;x<2560;x++) {
            bool shape=x>650+s*8&&x<1000+s*8&&y>600&&y<1000;
            unsigned r=shape?220:20+x*100/2560,g=shape?35:30+y*100/1655,b=shape?30:80+(x/80+y/80)%2*35;
            pixels[y*2560+x]=0xff000000u|r<<16|g<<8|b;
        }
        inputs[s]=[device newTextureWithDescriptor:descriptor];require(inputs[s]!=nil,"native input allocation");
        [inputs[s] replaceRegion:MTLRegionMake2D(0,0,2560,1655) mipmapLevel:0 withBytes:pixels.data() bytesPerRow:2560*4];
    }
    std::array<id<MTLCommandBuffer>,3> flights{};std::vector<id<MTLCommandBuffer>> measured;std::vector<double> cpu;
    auto seed=[queue commandBuffer];require(fi.encode(seed,inputs[0],0,1.0,false)==nil,"first input seeds history");[seed commit];completed(seed);
    auto gap=[queue commandBuffer];require(fi.encode(gap,inputs[1],1,1.5)==nil,"time gap seeds native");[gap commit];completed(gap);
    fi.invalidateHistory();auto invalidated=[queue commandBuffer];require(fi.encode(invalidated,inputs[2],2,1.516)==nil,"invalidated history seeds native");[invalidated commit];completed(invalidated);
    fi.invalidateHistory();double start=now();id<MTLTexture> last=nil;
    for(unsigned f=0;f<72;f++) { @autoreleasepool {
        const unsigned slot=f%3;if(flights[slot])completed(flights[slot]);
        auto cb=[queue commandBuffer];double t=now();last=fi.encode(cb,inputs[slot],slot,2.0+double(f)/60.0,f!=0);double cost=(now()-t)*1000;
        require((f==0)==(last==nil),"midpoint only after seed");[cb commit];flights[slot]=cb;
        if(f>=12){measured.push_back(cb);cpu.push_back(cost);}
        require(footprint()<3ull*1024*1024*1024,"bounded probe footprint");
    }}
    for(auto cb:flights)if(cb)completed(cb);double elapsed=now()-start;
    std::vector<double> gpu,intervals;double previous=0;
    for(auto cb:measured){require(cb.GPUEndTime>cb.GPUStartTime&&cb.GPUStartTime>0,"GPU timing available");gpu.push_back((cb.GPUEndTime-cb.GPUStartTime)*1000);if(previous>0)intervals.push_back((cb.GPUEndTime-previous)*1000);previous=cb.GPUEndTime;}
    auto report=[](const char* label,std::vector<double> v){std::sort(v.begin(),v.end());printf("%s n=%zu median_ms=%.4f p95_ms=%.4f p99_ms=%.4f max_ms=%.4f\n",label,v.size(),(v[(v.size()-1)/2]+v[v.size()/2])*.5,v[size_t(ceil(v.size()*.95))-1],v[size_t(ceil(v.size()*.99))-1],v.back());};
    report("incremental_generation_GPU",gpu);report("generation_completion_interval",intervals);report("CPU_encode",cpu);
    auto readback=[device newBufferWithLength:2560*1655*4 options:MTLResourceStorageModeShared];auto copy=[queue commandBuffer];
    last=fi.encode(copy,inputs[0],0,2.0+72.0/60.0);require(last!=nil,"final generation before in-flight reset");auto blit=[copy blitCommandEncoder];
    [blit copyFromTexture:last sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(2560,1655,1) toBuffer:readback destinationOffset:0 destinationBytesPerRow:2560*4 destinationBytesPerImage:2560*1655*4];[blit endEncoding];[copy commit];
    // Reset while an encoded command still owns the session/resources.
    fi.reset();require(fi.status()==sm::FrameInterpolation::Status::Disabled,"reset releases public state");completed(copy);
    auto* output=(uint32_t*)readback.contents;unsigned lo=255,hi=0;size_t opaque=0;
    for(size_t p=0;p<2560*1655;p+=11){lo=std::min(lo,unsigned(output[p]&255));hi=std::max(hi,unsigned(output[p]&255));opaque+=(output[p]>>24)==255;}
    require(hi>lo+20&&opaque>380000,"generated image is nonuniform and opaque");
    printf("PASS samples=%zu elapsed_s=%.3f output_blue_range=%u..%u opaque_samples=%zu peak_at_end_footprint=%zu metal_bytes=%lu\n",measured.size(),elapsed,lo,hi,opaque,footprint(),(unsigned long)device.currentAllocatedSize);
    alarm(0);return 0;
}}

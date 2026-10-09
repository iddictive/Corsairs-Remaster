#include "cloud_noise_assets.hpp"
#include <cstring>
#include <span>

#ifndef STORM_SKY_NOISE_DIR
#error "The native build must bind its verified sky input directory"
#endif

// Mach-O embeds the original source bytes; neither source paths nor a generated
// multi-megabyte C++ initializer become a runtime dependency.
__asm__(".section __TEXT,__const\n"
        ".balign 16\n.globl _storm_sky_shape\n_storm_sky_shape:\n.incbin \"" STORM_SKY_NOISE_DIR "/perlworlnoise.tga\"\n"
        ".globl _storm_sky_shape_end\n_storm_sky_shape_end:\n"
        ".balign 16\n.globl _storm_sky_detail\n_storm_sky_detail:\n.incbin \"" STORM_SKY_NOISE_DIR "/worlnoise.bmp\"\n"
        ".globl _storm_sky_detail_end\n_storm_sky_detail_end:\n"
        ".balign 16\n.globl _storm_sky_weather\n_storm_sky_weather:\n.incbin \"" STORM_SKY_NOISE_DIR "/weather.bmp\"\n"
        ".globl _storm_sky_weather_end\n_storm_sky_weather_end:\n");
extern "C" const uint8_t storm_sky_shape[],storm_sky_shape_end[],storm_sky_detail[],storm_sky_detail_end[],storm_sky_weather[],storm_sky_weather_end[];

namespace storm_metal {
namespace {
uint32_t u16(const uint8_t*p){return p[0]|uint32_t(p[1])<<8;}
uint32_t u32(const uint8_t*p){return u16(p)|u16(p+2)<<16;}
bool tgaVolume(std::span<const uint8_t>data,std::vector<uint8_t>&out){
    constexpr unsigned side=128,width=side*side,count=side*side*side;
    if(data.size()<18||data[1]!=0||data[2]!=10||u16(data.data()+12)!=width||u16(data.data()+14)!=side||data[16]!=32)return false;
    out.resize(count*4);size_t at=18+data[0],pixel=0;
    auto emit=[&](const uint8_t*p){unsigned x=pixel%width,y=pixel/width;if(!(data[17]&32))y=side-1-y;if(data[17]&16)x=width-1-x;size_t dest=((x/side)*side*side+y*side+x%side)*4;out[dest]=p[2];out[dest+1]=p[1];out[dest+2]=p[0];out[dest+3]=p[3];++pixel;};
    while(pixel<count){if(at>=data.size())return false;uint8_t header=data[at++];unsigned n=(header&127)+1;if(n>count-pixel)return false;if(header&128){if(at+4>data.size())return false;for(unsigned i=0;i<n;++i)emit(data.data()+at);at+=4;}else{if(at+n*4>data.size())return false;for(unsigned i=0;i<n;++i){emit(data.data()+at);at+=4;}}}
    return true;
}
bool bitmap(std::span<const uint8_t>data,unsigned side,unsigned slices,std::vector<uint8_t>&out){
    unsigned width=side*slices,height=side;
    if(data.size()<54||data[0]!='B'||data[1]!='M'||u32(data.data()+14)!=40||u32(data.data()+18)!=width||u32(data.data()+22)!=height||u16(data.data()+26)!=1||u16(data.data()+28)!=24||u32(data.data()+30)!=0)return false;
    size_t offset=u32(data.data()+10),pitch=(width*3+3)&~size_t(3);if(offset+pitch*height>data.size())return false;
    out.resize(side*side*slices*4);
    for(unsigned y=0;y<side;++y)for(unsigned x=0;x<width;++x){const uint8_t*p=data.data()+offset+(side-1-y)*pitch+x*3;size_t dest=((x/side)*side*side+y*side+x%side)*4;out[dest]=p[2];out[dest+1]=p[1];out[dest+2]=p[0];out[dest+3]=255;}
    return true;
}
}
bool loadCloudNoise(std::vector<uint8_t>&shape,std::vector<uint8_t>&detail,std::vector<uint8_t>&weather){
    const auto span=[](const uint8_t*start,const uint8_t*end){return std::span<const uint8_t>(start,size_t(reinterpret_cast<uintptr_t>(end)-reinterpret_cast<uintptr_t>(start)));};
    return tgaVolume(span(storm_sky_shape,storm_sky_shape_end),shape)&&bitmap(span(storm_sky_detail,storm_sky_detail_end),32,32,detail)&&bitmap(span(storm_sky_weather,storm_sky_weather_end),512,1,weather);
}
}

#pragma once
#include "authored_sky.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace storm_metal {
struct AuthoredSolarSource {
    float x=0.f, y=0.f, z=0.f, gain=0.f;
};

// Run only when an authored texture set changes. Equal-solid-angle cells make
// coherence independent of the cube face size and avoid promoting a night
// image's relative maximum (or an isolated bright texel) into a second sun.
template<class Sample> AuthoredSolarSource findAuthoredSolarSource(Sample sample)
{
    constexpr int width=128, height=64, count=width*height;
    constexpr float floor=.6f, pi=3.14159265358979323846f;
    std::array<float,count> luminance{};
    std::array<unsigned char,count> seen{};
    std::array<int,count> queue{};
    const auto ray=[](int index) {
        const float y=(float(index/width)+.5f)/height;
        const float phi=(float(index%width)+.5f)*(2.f*pi/width);
        const float r=std::sqrt(1.f-y*y);
        return AuthoredSolarSource{r*std::cos(phi),y,r*std::sin(phi),0.f};
    };
    std::array<float,256> linear{};
    for(int i=0;i<256;++i)linear[i]=std::pow(float(i)/255.f,2.2f);
    for(int i=0;i<count;++i) {
        const auto d=ray(i);
        const auto c=sample(authoredSkyCoordinate(d.x,d.y,d.z));
        luminance[i]=.2126f*linear[(c>>16)&255]+.7152f*linear[(c>>8)&255]+.0722f*linear[c&255];
    }
    AuthoredSolarSource best{};
    float bestFlux=0.f;
    for(int start=0;start<count;++start) {
        if(seen[start]||luminance[start]<floor)continue;
        int head=0,tail=0;
        queue[tail++]=start;seen[start]=1;
        float flux=0.f,peak=0.f,x=0.f,y=0.f,z=0.f;
        while(head<tail) {
            const int cell=queue[head++],col=cell%width,row=cell/width;
            const auto d=ray(cell);
            const float weight=luminance[cell]-floor;
            flux+=weight;peak=std::max(peak,luminance[cell]);
            x+=d.x*weight;y+=d.y*weight;z+=d.z*weight;
            const int neighbors[]={row*width+(col+1)%width,row*width+(col+width-1)%width,
                                   row>0?cell-width:-1,row+1<height?cell+width:-1};
            for(const int next:neighbors)
                if(next>=0&&!seen[next]&&luminance[next]>=floor) {
                    seen[next]=1;queue[tail++]=next;
                }
        }
        if(tail<4||flux<=bestFlux)continue;
        const float length=std::sqrt(x*x+y*y+z*z);
        if(length<=0.f)continue;
        const float confidence=std::clamp((peak-floor)/(.85f-floor),0.f,1.f);
        best={x/length,y/length,z/length,confidence*confidence*(3.f-2.f*confidence)};
        bestFlux=flux;
    }
    return best;
}
}

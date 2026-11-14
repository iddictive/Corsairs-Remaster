#pragma once
// PAC-3 MSE-inspired arcade silhouette. +z is forward; nose is at z=0.
// The caller transforms these local vertices and owns flight and hit-to-kill.
#include <cmath>
#include <cstdint>

namespace storm_metal::patriot_visual {
struct Vec3 { float x, y, z; };
struct Vertex { Vec3 position; uint32_t color; };
constexpr unsigned type = 7;
constexpr float tailZ = -5.2f;
constexpr float bodyRadius = .42f;
constexpr float outerRadius = .91f;

template<class Emit> void geometry(Emit emit) {
    constexpr unsigned sides=12;
    constexpr float pi=3.14159265359f;
    constexpr float z[]={0.f,-.78f,-1.35f,-4.85f,tailZ};
    constexpr float r[]={0.f,.30f,bodyRadius,bodyRadius,.34f};
    constexpr unsigned light[]={222,239,250,242,216,188,164,153,164,184,201,215};
    auto shade=[](uint32_t c,unsigned l) { return 0xff000000u | (((c>>16&255u)*l/255u)<<16) |
        (((c>>8&255u)*l/255u)<<8) | ((c&255u)*l/255u); };
    auto point=[](float radius,float a,float depth) -> Vec3 { return {radius*std::cos(a),radius*std::sin(a),depth}; };
    for(unsigned j=0;j<4;j++) for(unsigned i=0;i<sides;i++) {
        const float a=2.f*pi*float(i)/sides,b=2.f*pi*float(i+1)/sides;
        const uint32_t c=shade(j==0?0xffdeddd4u:j==1?0xffd1d0c5u:0xffbbbdb6u,light[i]);
        const Vertex p{point(r[j],a,z[j]),c},q{point(r[j],b,z[j]),c};
        const Vertex s{point(r[j+1],a,z[j+1]),c},t{point(r[j+1],b,z[j+1]),c};
        if(j) emit(p,s,q);
        emit(q,s,t);
    }
    // Four clipped, swept tail fins, two-sided; no decorative oversized wings.
    for(unsigned i=0;i<4;i++) {
        const float a=(float(i)+.5f)*pi*.5f;
        const uint32_t c=shade(0xffa6aaa5u,light[(i*3)%sides]);
        const Vertex p{point(.39f,a,-3.82f),c},q{point(outerRadius,a,-4.67f),c};
        const Vertex s{point(.88f,a,-5.08f),c},t{point(.35f,a,-4.96f),c};
        emit(p,q,s); emit(p,s,t); emit(s,q,p); emit(t,s,p);
    }
}
}

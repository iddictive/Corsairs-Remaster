#pragma once
#include "planar_geometry.hpp"
#include <algorithm>
#include <cmath>

namespace storm_sea_cutout {
using storm_world_cut::PlanarRing;
using storm_world_cut::PlanarPolygon;
inline double area(const PlanarRing&ring){double value=0;for(size_t n=0;n<ring.size();++n){const auto&a=ring[n],&b=ring[(n+1)%ring.size()];value+=a[0]*b[1]-b[0]*a[1];}return value*.5;}
inline void positive(PlanarRing&ring){if(area(ring)<0)std::reverse(ring.begin(),ring.end());}
// Exact integer planar subtraction, with barycentric transport of every wave
// channel. Unchanged triangles retain their original indexed vertices.
template<class Vertex>bool subtract(const std::array<Vertex,3>&source,float offsetX,float offsetZ,
    const std::vector<PlanarRing>&hulls,std::vector<Vertex>&output,bool&changed)
{
    changed=false;
    PlanarRing triangle;for(const auto&v:source)triangle.push_back({double(v.vPos.x)+offsetX,double(v.vPos.z)+offsetZ});
    const double signedArea=area(triangle);if(std::abs(signedArea)<1.e-10)return true;
    auto subject=triangle;positive(subject);std::vector<PlanarPolygon>outside;
    if(!storm_world_cut::differencePlanar({subject},hulls,outside))return false;
    if(outside.size()==1&&outside[0].size()==1&&outside[0][0].size()==3){
        bool same=true;for(const auto&p:outside[0][0])same&=std::any_of(triangle.begin(),triangle.end(),[&](const auto&q){return std::abs(p[0]-q[0])<=.000002&&std::abs(p[1]-q[1])<=.000002;});
        if(same)return true;
    }
    std::vector<Vertex>next;
    for(const auto&polygon:outside){std::vector<uint32_t>indices;if(!storm_world_cut::integerTriangulation(polygon,indices))return false;
        std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&p:ring){
            const double x=p[0]-triangle[0][0],z=p[1]-triangle[0][1];
            const double u=(x*(triangle[2][1]-triangle[0][1])-z*(triangle[2][0]-triangle[0][0]))/(2*signedArea);
            const double v=((triangle[1][0]-triangle[0][0])*z-(triangle[1][1]-triangle[0][1])*x)/(2*signedArea);
            const float a=float(1-u-v),b=float(u),c=float(v);Vertex out=source[0];
            out.vPos=source[0].vPos*a+source[1].vPos*b+source[2].vPos*c;
            out.vNormal=source[0].vNormal*a+source[1].vNormal*b+source[2].vNormal*c;
            out.tu=source[0].tu*a+source[1].tu*b+source[2].tu*c;out.tv=source[0].tv*a+source[1].tv*b+source[2].tv*c;
            for(unsigned n=0;n<2;++n)out.eventMaterial[n]=source[0].eventMaterial[n]*a+source[1].eventMaterial[n]*b+source[2].eventMaterial[n]*c;
            vertices.push_back(out);
        }
        for(size_t n=0;n<indices.size();n+=3){auto a=indices[n],b=indices[n+1],c=indices[n+2];
            const auto&x=vertices[a].vPos,&y=vertices[b].vPos,&z=vertices[c].vPos;
            if(((y.x-x.x)*(z.z-x.z)-(y.z-x.z)*(z.x-x.x))*signedArea<0)std::swap(b,c);
            next.push_back(vertices[a]);next.push_back(vertices[b]);next.push_back(vertices[c]);
        }
    }
    output.insert(output.end(),next.begin(),next.end());changed=true;return true;
}
}

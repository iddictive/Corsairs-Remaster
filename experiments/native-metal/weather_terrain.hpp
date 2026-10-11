#pragma once
#include "island_geometry.hpp"

namespace storm_metal {
// One current island, captured once from its authored opaque GM triangles.
// No collision queries, vertex copies or terrain rasterisation in a frame.
struct WeatherTerrain {
    // Largest authored islands span 58 km. A 256 grid made each terrain
    // sample larger than several ships; 1024 keeps that spacing below 60 m.
    static constexpr unsigned side=1024;
    const void* owner=nullptr;
    uint64_t revision=0;
    std::array<float,4> bounds{}; // min X/Z, extent X/Z in authored world metres
    std::vector<float> heights;
    void clear(const void* source){if(owner==source){owner=nullptr;heights.clear();++revision;}}
    void capture(const void* source,const std::vector<island_geometry::Face>&faces){
        if(faces.empty())return;
        float xmin=INFINITY,zmin=INFINITY,xmax=-INFINITY,zmax=-INFINITY;
        for(const auto&f:faces)for(const auto&v:f.v){xmin=std::min(xmin,v.p[0]);xmax=std::max(xmax,v.p[0]);zmin=std::min(zmin,v.p[2]);zmax=std::max(zmax,v.p[2]);}
        if(!std::isfinite(xmin)||xmax-xmin<1||zmax-zmin<1)return;
        bounds={xmin,zmin,xmax-xmin,zmax-zmin};heights.assign(side*side,0.f);
        for(const auto&f:faces){
            std::array<std::array<float,3>,3> p;
            for(unsigned j=0;j<3;j++)p[j]={(f.v[j].p[0]-xmin)/bounds[2]*(side-1),f.v[j].p[1],(f.v[j].p[2]-zmin)/bounds[3]*(side-1)};
            const float area=(p[1][2]-p[2][2])*(p[0][0]-p[2][0])+(p[2][0]-p[1][0])*(p[0][2]-p[2][2]);
            if(std::abs(area)<1.e-6f)continue;
            int x0=std::max(0,int(std::floor(std::min({p[0][0],p[1][0],p[2][0]})))),x1=std::min(int(side-1),int(std::ceil(std::max({p[0][0],p[1][0],p[2][0]}))));
            int z0=std::max(0,int(std::floor(std::min({p[0][2],p[1][2],p[2][2]})))),z1=std::min(int(side-1),int(std::ceil(std::max({p[0][2],p[1][2],p[2][2]}))));
            for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){
                float a=((p[1][2]-p[2][2])*(x-p[2][0])+(p[2][0]-p[1][0])*(z-p[2][2]))/area;
                float b=((p[2][2]-p[0][2])*(x-p[2][0])+(p[0][0]-p[2][0])*(z-p[2][2]))/area,c=1-a-b;
                if(std::min({a,b,c})>=-1.e-5f)heights[z*side+x]=std::max(heights[z*side+x],a*p[0][1]+b*p[1][1]+c*p[2][1]);
            }
        }
        owner=source;++revision;
    }
};
inline WeatherTerrain weatherTerrain;
}

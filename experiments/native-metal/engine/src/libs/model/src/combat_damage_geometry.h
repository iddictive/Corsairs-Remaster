#pragma once
#include "ship_laser_geometry.hpp"

// Geometry-only portion shared by the native model and windowless real-GM
// replay. Anchor channels: source triangle in uv[2], barycentrics in uv[3..5].
namespace storm::combat
{
using namespace storm_ship_hull;
using Mesh=storm_ship_laser::Mesh;
inline size_t triangleCount(const Mesh &mesh)
{
    size_t n=0; for (const auto &entry:mesh) n+=entry.second.size(); return n;
}
inline Mesh wound(const std::vector<Triangle> &posed, Vec3 hit, Vec3 normal, bool blade)
{
    const auto basis=frame(normal);
    Mesh result;
    const std::array<float,3> radii=blade ? std::array<float,3>{.055f,.026f,.009f} : std::array<float,3>{.043f,.024f,.008f};
    const std::array<uint32_t,3> colors={0xff48101bu,0xff811b25u,0xff160a0cu};
    for (unsigned layer=0;layer<3;++layer) for (const auto &triangle:posed)
    {
        const auto faceNormal=normalized(cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position));
        if (dot(faceNormal,normal)<.25f) continue;
        std::vector<Vertex> polygon(triangle.begin(),triangle.end()),inside,outside;
        for (unsigned edge=0;edge<16 && !polygon.empty();++edge)
        {
            const float angle=float(edge)*6.28318530718f/16;
            auto axis=basis.tangent*std::cos(angle)+basis.bitangent*std::sin(angle);
            const float torn=1.f+.16f*std::sin(angle*3.f+1.7f)+.09f*std::sin(angle*7.f+.6f);
            const float radius=radii[layer]*torn*(blade ? (std::abs(std::sin(angle))*.35f+.65f) : 1.f);
            storm_ship_laser::linearSplit(polygon,{axis,dot(axis,hit)+radius},inside,outside); polygon.swap(inside);
        }
        for (float sign:{-1.f,1.f})
        {
            storm_ship_laser::linearSplit(polygon,{normal*sign,dot(normal*sign,hit)+.035f},inside,outside); polygon.swap(inside);
        }
        if (polygon.size()<3) continue;
        auto center=polygon[0];
        for (size_t n=1;n<polygon.size();++n) center=interpolate(center,polygon[n],1.f/float(n+1));
        const auto shade=[&](Vertex v) {
            const auto delta=v.position-hit;
            const float radius=std::sqrt(dot(delta,basis.tangent)*dot(delta,basis.tangent)+dot(delta,basis.bitangent)*dot(delta,basis.bitangent));
            const float alpha=std::clamp((1.12f-radius/radii[layer])*3.f,0.f,1.f)*(layer==0 ? 205.f : 255.f);
            v.color=(colors[layer]&0x00ffffffu)|(uint32_t(alpha)<<24);v.uv[6]=float(layer);return v;
        };
        center=shade(center);
        for (size_t n=0;n<polygon.size();++n) result[-1].push_back({center,shade(polygon[n]),shade(polygon[(n+1)%polygon.size()])});
        if (triangleCount(result)>384) return {};
    }
    return result;
}

inline storm_ship_laser::Split sever(const Mesh &source,const storm_ship_laser::Plane &plane)
{
    // The neck subtree's actual skin influence is the anatomical field.
    // A global horizontal slab would also remove a raised shoulder/collar.
    // Interpolate this field on each source triangle, retaining both sides.
    storm_ship_laser::Split cut;
    for (const auto &object:source) for (const auto &triangle:object.second)
    {
        std::vector<Vertex> body,part,edge;
        for (unsigned n=0;n<3;++n)
        {
            const auto &a=triangle[n], &b=triangle[(n+1)%3];
            const float da=a.uv[7]-.5f,db=b.uv[7]-.5f;
            (da<=0 ? body : part).push_back(a);
            if ((da<0 && db>0)||(da>0 && db<0))
            {
                auto v=interpolate(a,b,da/(da-db));
                body.push_back(v);part.push_back(v);edge.push_back(v);
            }
            else if (std::abs(da)<1.e-7f) edge.push_back(a);
        }
        const auto emit=[&](const std::vector<Vertex> &polygon,std::vector<Triangle> &target,float &surfaceArea){
            const auto first=target.size();fan(polygon,target);
            for (size_t n=first;n<target.size();++n) surfaceArea+=area(target[n]);
        };
        emit(body,cut.kept[object.first],cut.keptArea);emit(part,cut.detached[object.first],cut.detachedArea);
        // A zero-field edge belongs to its kept-side triangle, as in the
        // planar splitter. Taking it from both sides duplicates the cap seam.
        const bool keptSide=std::any_of(triangle.begin(),triangle.end(),[](const Vertex &v){return v.uv[7]<.5f;});
        if (keptSide && edge.size()==2 && !storm_ship_laser::near(edge[0].position,edge[1].position)) cut.seam.push_back({edge[0],edge[1],object.first});
    }
    if (cut.keptArea<.01f || cut.detachedArea<.001f || cut.seam.empty()) return {};
    std::map<storm_ship_laser::PositionKey,Vertex> anchors;
    for (auto &edge:cut.seam)
    {
        anchors.emplace(storm_ship_laser::positionKey(edge.a.position),edge.a);
        anchors.emplace(storm_ship_laser::positionKey(edge.b.position),edge.b);
        edge.object=-2;
    }
    Mesh bodyCap,partCap; storm_ship_laser::cap(cut.seam,plane,bodyCap,partCap);
    if (triangleCount(bodyCap)==0 || triangleCount(partCap)==0) return {};
    using Key=storm_ship_laser::PositionKey;
    using Edge=std::pair<Key,Key>;
    const auto edgeKey=[](Vec3 a,Vec3 b) {
        auto first=storm_ship_laser::positionKey(a),second=storm_ship_laser::positionKey(b);
        if (second<first) std::swap(first,second);
        return Edge{first,second};
    };
    std::set<Edge> boundary;
    for (const auto &edge:cut.seam) boundary.insert(edgeKey(edge.a.position,edge.b.position));
    for (const auto *cap:{&bodyCap,&partCap})
    {
        std::map<Edge,unsigned> incidence;
        for (const auto &object:*cap) for (const auto &triangle:object.second) for (unsigned n=0;n<3;++n)
            ++incidence[edgeKey(triangle[n].position,triangle[(n+1)%3].position)];
        for (const auto &edge:boundary) if (incidence[edge]!=1) return {};
        for (const auto &edge:incidence)
            if (edge.second!=2 && !(edge.second==1 && boundary.count(edge.first))) return {};
    }
    for (auto *cap:{&bodyCap,&partCap}) for (auto &object:*cap) for (auto &triangle:object.second) for (auto &v:triangle)
    {
        const auto found=anchors.find(storm_ship_laser::positionKey(v.position));
        if (found==anchors.end()) return {};
        v.uv=found->second.uv;
        const float tissue=.5f+.5f*std::sin(v.position.x*139.f+v.position.z*173.f);
        v.color=0xff000000u|(uint32_t(105+65*tissue)<<16)|(uint32_t(19+15*tissue)<<8)|uint32_t(27+17*tissue);
    }
    cut.kept[-2]=std::move(bodyCap[-2]); cut.detached[-2]=std::move(partCap[-2]);
    return cut;
}
}

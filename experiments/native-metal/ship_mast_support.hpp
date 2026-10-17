#pragma once
#include "ship_laser_geometry.hpp"

// Authored hull/mast joints live in the cut value worker. Bounds only select
// candidate faces; a joint requires a finite region of actual triangle contact.
namespace storm::ship::mast_support {
using namespace storm_ship_laser;
constexpr float tolerance = .00015f; // Same geometric tolerance as SupportTopology.
struct Region { std::vector<Vec3> points; Bounds bounds{}; int object=0; };
struct Contact {
    std::shared_ptr<const SurfaceBase> hull, mast;
    std::vector<Region> regions;
};
template<class Boundary> struct Index {
    const Boundary &surface;
    struct Face { unsigned role; int object; uint32_t triangle; };
    std::vector<Face> faces;
    storm_ship_hull::Broadphase broadphase;
    explicit Index(const Boundary &source):surface(source) {
        std::vector<Bounds> boxes;
        const Vec3 margin{tolerance,tolerance,tolerance};
        for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(source,role))for(uint32_t n=0;n<o.second.size();++n){
            storm_geometry_work::checkpoint();const auto&t=o.second[n];Bounds b{t[0].position,t[0].position};
            for(unsigned c=1;c<3;++c)b=storm_ship_hull::merge(b,{t[c].position,t[c].position});
            b.lo=b.lo-margin;b.hi=b.hi+margin;boxes.push_back(b);faces.push_back({role,o.first,n});
        }
        broadphase.build(boxes);
    }
    const Triangle &face(uint32_t n)const{const auto&r=faces[n];return surfaceRole(surface,r.role).at(r.object)[r.triangle];}
};
inline std::vector<Vec3> intersection(const Triangle&a,const Triangle&b) {
    if(storm_ship_hull::length(storm_ship_hull::cross(a[1].position-a[0].position,a[2].position-a[0].position))<1.e-12f||
       storm_ship_hull::length(storm_ship_hull::cross(b[1].position-b[0].position,b[2].position-b[0].position))<1.e-12f)return {};
    std::vector<Vec3> points;
    const auto append=[&](Vec3 point){for(auto old:points)if(storm_ship_hull::length(old-point)<=tolerance)return;points.push_back(point);};
    for(const auto&v:a)if(storm_ship_hull::length(v.position-closestFacePoint(v.position,b))<=tolerance)append(v.position);
    for(const auto&v:b)if(storm_ship_hull::length(v.position-closestFacePoint(v.position,a))<=tolerance)append(v.position);
    const auto crosses=[&](Vec3 p,Vec3 q,const Triangle&t){
        const auto normal=storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position);
        const double x=storm_ship_hull::dot(normal,p-t[0].position),y=storm_ship_hull::dot(normal,q-t[0].position);
        if((x<0)==(y<0)||std::abs(x-y)<1.e-20)return;
        const auto point=p+(q-p)*float(x/(x-y));if(storm_ship_hull::length(point-closestFacePoint(point,t))<=tolerance)append(point);
    };
    for(unsigned i=0;i<3;++i){
        crosses(a[i].position,a[(i+1)%3].position,b);crosses(b[i].position,b[(i+1)%3].position,a);
        // Coplanar crossing edges need their own finite segment intersection.
        for(unsigned j=0;j<3;++j){
            const auto p=a[i].position,q=b[j].position,u=a[(i+1)%3].position-p,v=b[(j+1)%3].position-q,w=p-q;
            const double aa=storm_ship_hull::dot(u,u),bb=storm_ship_hull::dot(u,v),cc=storm_ship_hull::dot(v,v),dd=storm_ship_hull::dot(u,w),ee=storm_ship_hull::dot(v,w);
            if(aa<1.e-16||cc<1.e-16)continue;const double determinant=aa*cc-bb*bb;
            double s=determinant>1.e-20?std::clamp((bb*ee-cc*dd)/determinant,0.,1.):0.,t=(bb*s+ee)/cc;
            if(t<0){t=0;s=std::clamp(-dd/aa,0.,1.);}else if(t>1){t=1;s=std::clamp((bb-dd)/aa,0.,1.);}
            const auto first=p+u*float(s),last=q+v*float(t);if(storm_ship_hull::length(first-last)<=tolerance)append((first+last)*.5f);
        }
    }
    if(points.size()<2)return {};
    size_t first=0,last=1;float length=0;
    for(size_t i=0;i<points.size();++i)for(size_t j=i+1;j<points.size();++j){const float d=storm_ship_hull::length(points[j]-points[i]);if(d>length){length=d;first=i;last=j;}}
    if(length<=tolerance)return {};
    const auto axis=(points[last]-points[first])*(1.f/length);float width=0;
    for(auto point:points)width=std::max(width,storm_ship_hull::length(storm_ship_hull::cross(axis,point-points[first])));
    if(width<=tolerance)return {points[first],points[last]};
    Vec3 center{};for(auto point:points)center=center+point;center=center*(1.f/float(points.size()));
    const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(a[1].position-a[0].position,a[2].position-a[0].position));
    const auto side=storm_ship_hull::cross(normal,axis);
    std::sort(points.begin(),points.end(),[&](Vec3 p,Vec3 q){return std::atan2(storm_ship_hull::dot(p-center,side),storm_ship_hull::dot(p-center,axis))<std::atan2(storm_ship_hull::dot(q-center,side),storm_ship_hull::dot(q-center,axis));});
    return points;
}
inline bool segmentSupported(Vec3 a,Vec3 b,const Triangle&t) {
    const auto raw=storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position);
    if(storm_ship_hull::length(raw)<1.e-12f)return false;const auto normal=storm_ship_hull::normalized(raw);
    if(std::abs(storm_ship_hull::dot(normal,a-t[0].position))>tolerance||std::abs(storm_ship_hull::dot(normal,b-t[0].position))>tolerance)return false;
    for(unsigned n=0;n<3;++n){const auto edge=t[(n+1)%3].position-t[n].position;
        const auto inward=storm_ship_hull::normalized(storm_ship_hull::cross(normal,edge));
        const float x=storm_ship_hull::dot(a-t[n].position,inward),y=storm_ship_hull::dot(b-t[n].position,inward);
        if(x<0&&y<0)return false;if((x<0)!=(y<0)){const auto point=a+(b-a)*(x/(x-y));if(x<0)a=point;else b=point;}
    }
    return storm_ship_hull::length(b-a)>tolerance;
}
template<class Transform> inline std::shared_ptr<const Contact> capture(const std::shared_ptr<const SurfaceBase>&hull,
    const std::shared_ptr<const SurfaceBase>&mast,const Index<SurfaceBase>&index,const std::map<int,float>&hullWood,
    bool receiverMast,Transform transform) {
    auto contact=std::make_shared<Contact>();contact->hull=hull;contact->mast=mast;std::vector<uint32_t>nearby;
    // The native pMasts record, rather than the hull timber texture profile,
    // admits this assembly. Its authored outer boundary is the real joint.
    for(const auto&o:mast->outer)for(auto triangle:o.second){
        storm_geometry_work::checkpoint();for(auto&v:triangle)v.position=transform(v.position);
        Bounds box{triangle[0].position,triangle[0].position};for(unsigned n=1;n<3;++n)box=storm_ship_hull::merge(box,{triangle[n].position,triangle[n].position});
        const Vec3 margin{tolerance,tolerance,tolerance};box.lo=box.lo-margin;box.hi=box.hi+margin;index.broadphase.queryBounds(box,nearby);
        for(auto id:nearby){if(receiverMast?index.faces[id].role!=0:!hullWood.count(index.faces[id].object))continue;auto points=intersection(triangle,index.face(id));if(points.empty())continue;
            Region region;region.object=index.faces[id].object;region.bounds={points.front(),points.front()};
            for(auto p:points)region.bounds=storm_ship_hull::merge(region.bounds,{p,p});region.points=std::move(points);contact->regions.push_back(std::move(region));}
    }
    return contact;
}
inline bool lost(const Contact&contact,const Index<Surface>&retained) {
    if(contact.regions.empty())return false; // No authored joint is no license to invent one.
    std::vector<uint32_t>nearby;const Vec3 margin{tolerance,tolerance,tolerance};
    for(const auto&region:contact.regions){storm_geometry_work::checkpoint();Bounds box=region.bounds;box.lo=box.lo-margin;box.hi=box.hi+margin;retained.broadphase.queryBounds(box,nearby);
        for(auto id:nearby){if(retained.faces[id].object!=region.object)continue;const auto&t=retained.face(id);
            if(region.points.size()==2){if(segmentSupported(region.points[0],region.points[1],t))return false;}
            else for(size_t n=1;n+1<region.points.size();++n){Triangle joint{};joint[0].position=region.points[0];joint[1].position=region.points[n];joint[2].position=region.points[n+1];if(!intersection(joint,t).empty())return false;}
        }
    }
    return true;
}
}

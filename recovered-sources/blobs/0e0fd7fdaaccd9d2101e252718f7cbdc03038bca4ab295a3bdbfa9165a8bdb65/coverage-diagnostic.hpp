#include <iostream>
#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_airburst_hull_distance.hpp"
#include "world_cut_coverage.hpp"
#include <set>
#include <tuple>
#include <string_view>

// Static-world Boolean fields. Geometry owns reconstruction and rejection;
// scene code owns world/local transforms and persistent ordered records.
namespace storm_world_cut {
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_laser::Mesh;
using storm_ship_laser::PositionKey;
using storm_ship_laser::PositionEdge;
using storm_ship_laser::Segment;
using Plane = storm_ship_hull::Plane;
inline constexpr size_t MaxCuts=64, MaxTriangles=1000000;
using PlanarPoint=std::array<double,2>;
using PlanarRing=std::vector<PlanarPoint>;
using PlanarPolygon=std::vector<PlanarRing>;
// Private geometry adapter; Clipper types never enter the shared game API.
bool unitePlanar(const std::vector<PlanarRing>&input,std::vector<PlanarPolygon>&output);
bool differencePlanar(const std::vector<PlanarRing>&subject,const std::vector<PlanarRing>&clip,std::vector<PlanarPolygon>&output);
bool integerTriangulation(const PlanarPolygon&polygon,std::vector<uint32_t>&indices);
// World solids cover every shipped ground/shore material family. The PN
// renderer's smaller refinement allowlist is a separate visual policy.
inline bool terrainTextureStem(std::string_view stem) {
    for(std::string_view terrain:{"rockk1","rockk1a","rockk2","rockk3","rockk4","rockk4s","rockk5","rocksh1",
        "jungle_gray","jungle1","beach","beach1","beach2","reef1","sandtile"})if(stem==terrain)return true;
    return false;
}
struct Cut {
    enum Kind : uint32_t { Halfspace=0, Sphere=1 };
    Kind kind=Halfspace;
    Vec3 normal{0,1,0}; float distance=0;
    Vec3 center{}; float radius=1;
    uint32_t field=0; // Derived binding identity; never part of shape equality.
    std::array<Vec3,3>axes{{{1,0,0},{0,1,0},{0,0,1}}};
};
inline bool valid(const Cut&c) {
    if(c.kind==Cut::Halfspace)return storm_ship_hull::finite(c.normal)&&std::isfinite(c.distance)&&
        std::abs(storm_ship_hull::length(c.normal)-1)<.001f&&std::abs(c.distance)<=10000000;
    if(c.kind!=Cut::Sphere||!storm_ship_hull::finite(c.center)||!std::isfinite(c.radius)||c.radius<.05f||c.radius>5000||
        std::max({std::abs(c.center.x),std::abs(c.center.y),std::abs(c.center.z)})>10000000)return false;
    for(unsigned n=0;n<3;++n){if(!storm_ship_hull::finite(c.axes[n])||std::abs(storm_ship_hull::length(c.axes[n])-1)>.001f)return false;
        for(unsigned p=0;p<n;++p)if(std::abs(storm_ship_hull::dot(c.axes[n],c.axes[p]))>.001f)return false;}
    return true;
}
inline bool equal(const Cut&a,const Cut&b) {
    if(a.kind!=b.kind)return false;
    if(a.kind==Cut::Halfspace)return a.normal.x==b.normal.x&&a.normal.y==b.normal.y&&a.normal.z==b.normal.z&&a.distance==b.distance;
    if(a.center.x!=b.center.x||a.center.y!=b.center.y||a.center.z!=b.center.z||a.radius!=b.radius)return false;
    for(unsigned n=0;n<3;++n)if(a.axes[n].x!=b.axes[n].x||a.axes[n].y!=b.axes[n].y||a.axes[n].z!=b.axes[n].z)return false;return true;
}
inline size_t triangleCount(const Mesh&m){size_t n=0;for(const auto&o:m)n+=o.second.size();return n;}
inline bool affects(const storm_ship_hull::Bounds&b,const std::vector<Cut>&cuts) {
    for(const auto&c:cuts){if(c.kind==Cut::Halfspace){const Vec3 support{c.normal.x>=0?b.hi.x:b.lo.x,c.normal.y>=0?b.hi.y:b.lo.y,c.normal.z>=0?b.hi.z:b.lo.z};
            if(double(c.normal.x)*support.x+double(c.normal.y)*support.y+double(c.normal.z)*support.z>c.distance)return true;
        }else {const Vec3 reach{c.radius*(std::abs(c.axes[0].x)+std::abs(c.axes[1].x)+std::abs(c.axes[2].x)),c.radius*(std::abs(c.axes[0].y)+std::abs(c.axes[1].y)+std::abs(c.axes[2].y)),c.radius*(std::abs(c.axes[0].z)+std::abs(c.axes[1].z)+std::abs(c.axes[2].z))};
            if(c.center.x+reach.x>=b.lo.x&&c.center.x-reach.x<=b.hi.x&&c.center.y+reach.y>=b.lo.y&&c.center.y-reach.y<=b.hi.y&&c.center.z+reach.z>=b.lo.z&&c.center.z-reach.z<=b.hi.z)return true;}}
    return false;
}
inline bool finite(const Mesh&m) {
    if(triangleCount(m)>MaxTriangles)return false;
    for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t)
        if(!storm_ship_hull::finite(v.position)||!storm_ship_hull::finite(v.normal)||
           std::max({std::abs(v.position.x),std::abs(v.position.y),std::abs(v.position.z)})>10000000)return false;
    return true;
}
struct Boundary {Vertex a,b;int object=0;unsigned count=0;int winding=0;};
inline bool boundaries(const Mesh&mesh,std::vector<Segment>&out) {
    std::unordered_map<PositionEdge,Boundary,storm_ship_laser::PositionEdgeHash>edges;
    edges.reserve(triangleCount(mesh)*2);
    for(const auto&o:mesh)for(const auto&t:o.second)for(unsigned n=0;n<3;++n){
        auto a=storm_ship_laser::positionKey(t[n].position),b=storm_ship_laser::positionKey(t[(n+1)%3].position);if(a==b)continue;
        const int direction=a<b?1:-1;auto ordered=std::minmax(a,b);auto&e=edges[{ordered.first,ordered.second}];
        if(!e.count){e.a=t[n];e.b=t[(n+1)%3];e.object=o.first;}++e.count;e.winding+=direction;
    }
    for(const auto&e:edges){if(e.second.count>2||(e.second.count==2&&e.second.winding))return false;
        if(e.second.count==1)out.push_back({e.second.a,e.second.b,e.second.object});}
    // Deterministic reconstruction also fixes cap material ownership.
    std::sort(out.begin(),out.end(),[](const Segment&a,const Segment&b){return
        std::make_tuple(storm_ship_laser::positionKey(a.a.position),storm_ship_laser::positionKey(a.b.position),a.object)<
        std::make_tuple(storm_ship_laser::positionKey(b.a.position),storm_ship_laser::positionKey(b.b.position),b.object);});
    return true;
}
inline bool caps(const std::vector<Segment>&seam,const Plane&plane,Mesh&out) {
    if(seam.empty())return true;
    using Point=std::array<double,2>;
    struct Incidence{Segment edge;int winding=0;};std::map<PositionEdge,Incidence>incidence;
    for(const auto&e:seam){auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);if(a==b)continue;
        auto ordered=std::minmax(a,b);auto&entry=incidence[{ordered.first,ordered.second}];entry.edge=e;entry.winding+=a<b?1:-1;}
    std::map<PositionKey,std::vector<size_t>>outgoing;std::map<PositionKey,int>balance;std::set<PositionEdge>unique;std::vector<Segment>edges;
    for(const auto&entry:incidence){if(!entry.second.winding)continue;if(std::abs(entry.second.winding)!=1)return false;auto e=entry.second.edge;
        auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);if((a<b)!=(entry.second.winding>0)){std::swap(e.a,e.b);std::swap(a,b);}
        unique.insert(entry.first);outgoing[a].push_back(edges.size());++balance[a];--balance[b];edges.push_back(e);}
    if(unique.empty())return true;if(unique.size()>65536)return false;
    for(const auto&p:balance)if(p.second)return false;
    const auto basis=storm_ship_hull::frame(plane.normal);const auto origin=edges.front().a.position;
    const double positionTolerance=std::max(1.e-6,2.*std::numeric_limits<float>::epsilon()*
        std::max({std::abs(double(origin.x)),std::abs(double(origin.y)),std::abs(double(origin.z)),std::abs(double(plane.distance)),1.}));
    const auto project=[&](Vec3 p){const double x=double(p.x)-origin.x,y=double(p.y)-origin.y,z=double(p.z)-origin.z;return Point{
        x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z};};
    struct Ring{std::vector<Vertex>vertices;std::vector<Point>points;double area=0;int object=0,parent=-1,depth=0;};
    std::vector<Ring>rings;std::vector<bool>used(edges.size());
    for(size_t start=0;start<edges.size();++start){if(used[start])continue;Ring ring;ring.object=edges[start].object;
        const auto first=storm_ship_laser::positionKey(edges[start].a.position);auto current=first;size_t next=start;
        do{if(used[next])return false;used[next]=true;const auto&e=edges[next];ring.vertices.push_back(e.a);current=storm_ship_laser::positionKey(e.b.position);if(current==first)break;
            const auto incoming=project(e.a.position),point=project(e.b.position);double best=-4;next=edges.size();
            for(auto candidate:outgoing[current])if(!used[candidate]){const auto end=project(edges[candidate].b.position);
                const double ax=point[0]-incoming[0],ay=point[1]-incoming[1],bx=end[0]-point[0],by=end[1]-point[1];
                const double turn=std::atan2(ax*by-ay*bx,ax*bx+ay*by);if(next==edges.size()||turn>best){best=turn;next=candidate;}}
            if(next==edges.size())return false;
        }while(ring.vertices.size()<=edges.size());
        if(current!=first||ring.vertices.size()<3)return false;
        for(const auto&v:ring.vertices)ring.points.push_back(project(v.position));
        for(size_t n=0;n<ring.points.size();++n){const auto a=ring.points[n],b=ring.points[(n+1)%ring.points.size()];ring.area+=a[0]*b[1]-b[0]*a[1];}
        ring.area=std::abs(ring.area)*.5;if(ring.area<=1.e-8)return false;rings.push_back(std::move(ring));}
    const auto contains=[](const std::vector<Point>&ring,Point q){bool inside=false;for(size_t n=0,j=ring.size()-1;n<ring.size();j=n++){const auto a=ring[n],b=ring[j];if((a[1]>q[1])!=(b[1]>q[1])&&q[0]<(b[0]-a[0])*(q[1]-a[1])/(b[1]-a[1])+a[0])inside=!inside;}return inside;};
    for(size_t n=0;n<rings.size();++n){double area=std::numeric_limits<double>::infinity();for(size_t j=0;j<rings.size();++j)
        if(j!=n&&rings[j].area>rings[n].area&&rings[j].area<area&&contains(rings[j].points,rings[n].points.front())){rings[n].parent=int(j);area=rings[j].area;}}
    for(auto&ring:rings)for(int p=ring.parent;p>=0;p=rings[p].parent)++ring.depth;
    for(size_t n=0;n<rings.size();++n){if(rings[n].depth%2)continue;
        std::vector<std::vector<Point>>polygon;std::vector<Vertex>vertices;std::vector<uint32_t>cornerIndices;std::vector<std::pair<size_t,size_t>>ranges;double expected=0;
        const auto add=[&](const Ring&r,bool hole){const size_t base=vertices.size();ranges.emplace_back(base,r.vertices.size());vertices.insert(vertices.end(),r.vertices.begin(),r.vertices.end());expected+=hole?-r.area:r.area;
            std::vector<size_t>corners(r.points.size());std::iota(corners.begin(),corners.end(),0);bool changed=true;
            while(changed&&corners.size()>3){changed=false;for(size_t k=0;k<corners.size();++k){const auto a=r.points[corners[(k+corners.size()-1)%corners.size()]],b=r.points[corners[k]],c=r.points[corners[(k+1)%corners.size()]];
                const double x=c[0]-a[0],y=c[1]-a[1],along=(b[0]-a[0])*x+(b[1]-a[1])*y;
                bool linear=along>=0&&along<=x*x+y*y;
                for(size_t q=(corners[(k+corners.size()-1)%corners.size()]+1)%r.points.size();linear&&q!=corners[(k+1)%corners.size()];q=(q+1)%r.points.size()){
                    const auto p=r.points[q];const double step=(p[0]-a[0])*x+(p[1]-a[1])*y;
                    linear=step>=0&&step<=x*x+y*y&&std::abs(x*(p[1]-a[1])-y*(p[0]-a[0]))<=positionTolerance*std::hypot(x,y);}
                if(linear){
                    corners.erase(corners.begin()+k);changed=true;break;}}}
            polygon.emplace_back();for(auto k:corners){polygon.back().push_back(r.points[k]);cornerIndices.push_back(uint32_t(base+k));}
        };
        add(rings[n],false);for(size_t j=0;j<rings.size();++j)if(rings[j].parent==int(n))add(rings[j],true);
        auto indices=mapbox::earcut<uint32_t>(polygon);double measured=0;
        for(auto&index:indices)index=cornerIndices[index];
        std::vector<uint32_t>nondegenerate;nondegenerate.reserve(indices.size());
        for(size_t k=0;k+2<indices.size();k+=3)if(storm_ship_hull::area({vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]})>1.e-10f)
            nondegenerate.insert(nondegenerate.end(),indices.begin()+k,indices.begin()+k+3);
        indices=std::move(nondegenerate);
        std::map<std::pair<uint32_t,uint32_t>,unsigned>incidence;
        for(size_t k=0;k+2<indices.size();k+=3)for(unsigned c=0;c<3;++c){auto a=indices[k+c],b=indices[k+(c+1)%3];if(b<a)std::swap(a,b);++incidence[{a,b}];}
        for(size_t k=0;k+2<indices.size();k+=3){std::vector<Vertex>boundary;bool expanded=false;
            const auto pa=project(vertices[indices[k]].position),pb=project(vertices[indices[k+1]].position),pc=project(vertices[indices[k+2]].position);
            const bool reverse=(pb[0]-pa[0])*(pc[1]-pa[1])-(pb[1]-pa[1])*(pc[0]-pa[0])<0;
            for(unsigned c=0;c<3;++c){const uint32_t a=indices[k+c],b=indices[k+(c+1)%3];boundary.push_back(vertices[a]);auto ordered=std::minmax(a,b);
                if(incidence[{ordered.first,ordered.second}]!=1)continue;
                for(const auto&range:ranges){if(a<range.first||a>=range.first+range.second||b<range.first||b>=range.first+range.second)continue;
                    for(int direction:{1,-1}){std::vector<Vertex>between;size_t current=a;bool linear=true;
                        const auto pa=project(vertices[a].position),pb=project(vertices[b].position);const double dx=pb[0]-pa[0],dy=pb[1]-pa[1],length=std::hypot(dx,dy);
                        for(size_t step=0;step<range.second;++step){current=range.first+(current-range.first+range.second+direction)%range.second;if(current==b)break;const auto p=project(vertices[current].position);
                            const double along=(p[0]-pa[0])*dx+(p[1]-pa[1])*dy;
                            if(std::abs(dx*(p[1]-pa[1])-dy*(p[0]-pa[0]))>length*positionTolerance||along<=0||along>=length*length){linear=false;break;}between.push_back(vertices[current]);}
                        if(linear){boundary.insert(boundary.end(),between.begin(),between.end());expanded|=!between.empty();break;}}
                }
            }
            std::vector<Triangle>faces;
            if(!expanded)faces.push_back({vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]});
            else{auto center=storm_ship_hull::interpolate(storm_ship_hull::interpolate(vertices[indices[k]],vertices[indices[k+1]],.5f),vertices[indices[k+2]],1.f/3.f);
                for(size_t p=0;p<boundary.size();++p)faces.push_back({center,boundary[p],boundary[(p+1)%boundary.size()]});}
            for(auto t:faces){const auto a=project(t[0].position),b=project(t[1].position),c=project(t[2].position);const double signedArea=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
                if(reverse)std::swap(t[1],t[2]);const double area=std::abs(signedArea)*.5;if(area<=1.e-10)continue;measured+=area;
                for(auto&v:t){v.normal=plane.normal;const auto uv=project(v.position);for(unsigned channel=0;channel<4;++channel){v.uv[channel*2]=float(uv[0]*.12);v.uv[channel*2+1]=float(uv[1]*.12);}}out[rings[n].object].push_back(t);}
        }
        if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4))return false;
    }
    // Every contour must appear in the output boundary, including holes.
    std::vector<Segment>actual;if(!boundaries(out,actual))return false;
    std::set<PositionEdge>capEdges;
    for(const auto&e:actual){auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);auto ordered=std::minmax(a,b);capEdges.insert({ordered.first,ordered.second});}
    return capEdges==unique;
}
inline void surfaceNormal(Triangle&t) {
    const auto n=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
    for(auto&v:t)v.normal=n;
}
// Connected closed components stay authored. Only open stone/town components
// acquire the shared shell implementation, preserving doors and other openings.
inline std::vector<Mesh> components(const Mesh&source) {
    struct Ref{int object;size_t triangle;};std::vector<Ref>refs;std::vector<size_t>parent;
    std::unordered_map<PositionEdge,size_t,storm_ship_laser::PositionEdgeHash>edges;
    const auto root=[&](size_t a){while(parent[a]!=a){parent[a]=parent[parent[a]];a=parent[a];}return a;};
    for(const auto&o:source)for(size_t n=0;n<o.second.size();++n){const size_t id=refs.size();refs.push_back({o.first,n});parent.push_back(id);const auto&t=o.second[n];
        for(unsigned c=0;c<3;++c){auto a=storm_ship_laser::positionKey(t[c].position),b=storm_ship_laser::positionKey(t[(c+1)%3].position);if(a==b)continue;auto ordered=std::minmax(a,b);
            auto inserted=edges.emplace(PositionEdge{ordered.first,ordered.second},id);if(!inserted.second){const size_t x=root(id),y=root(inserted.first->second);parent[x]=y;}}}
    std::map<size_t,Mesh>groups;for(size_t n=0;n<refs.size();++n)groups[root(n)][refs[n].object].push_back(source.at(refs[n].object)[refs[n].triangle]);
    std::vector<Mesh>result;result.reserve(groups.size());for(auto&entry:groups)result.push_back(std::move(entry.second));return result;
}
inline std::map<PositionKey,float> stoneClearance(const Mesh&source,float maximum,std::map<PositionKey,Vec3>&offsetNormals) {
    struct Point{Vec3 position{},normal{};float depth=0;std::vector<Vec3>faces;};std::map<PositionKey,Point>points;
    struct Feature{Vec3 normal{};unsigned count=0;bool sharp=false;float length=0,altitude=std::numeric_limits<float>::infinity();};
    std::map<PositionEdge,Feature>features;std::vector<const Triangle*>triangles;std::vector<storm_ship_hull::Bounds>boxes;
    for(const auto&o:source)for(const auto&t:o.second){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
        storm_ship_hull::Bounds box{t[0].position,t[0].position};for(unsigned c=0;c<3;++c){const auto&v=t[c];auto inserted=points.try_emplace(storm_ship_laser::positionKey(v.position),Point{v.position,{},maximum});
            const float opposite=storm_ship_hull::length(t[(c+1)%3].position-t[(c+2)%3].position);
            if(opposite>1.e-8f)inserted.first->second.depth=std::min(inserted.first->second.depth,storm_ship_hull::area(t)*.25f/opposite);
            inserted.first->second.normal=inserted.first->second.normal+normal;inserted.first->second.faces.push_back(normal);box=storm_ship_hull::merge(box,{v.position,v.position});}
        triangles.push_back(&t);boxes.push_back(box);
        for(unsigned n=0;n<3;++n){auto a=storm_ship_laser::positionKey(t[n].position),b=storm_ship_laser::positionKey(t[(n+1)%3].position);if(a==b)continue;auto ordered=std::minmax(a,b);auto&edge=features[{ordered.first,ordered.second}];
            if(!edge.count){edge.normal=normal;edge.length=storm_ship_hull::length(t[n].position-t[(n+1)%3].position);}else edge.sharp|=storm_ship_hull::dot(edge.normal,normal)<.7f;
            edge.altitude=std::min(edge.altitude,2*storm_ship_hull::area(t)/edge.length);++edge.count;}}
    // Incident altitudes bound local displacement; physical opening/sharp edges
    // and nonincident clearance further bound the material's maximum depth.
    for(const auto&e:features)if(e.second.count==1||e.second.sharp)for(const auto&p:{e.first.first,e.first.second})points.at(p).depth=std::min(points.at(p).depth,std::min(e.second.length,e.second.altitude)*.25f);
    storm_ship_hull::Broadphase broadphase;broadphase.build(boxes);std::vector<uint32_t>candidates;std::map<PositionKey,float>limits;
    for(auto&entry:points){auto&point=entry.second;auto outward=storm_ship_hull::normalized(point.normal);
        // Lighting normals are not a solid offset field. Project the geometric
        // average into the incident face cone so every offset moves inward.
        for(unsigned iteration=0;iteration<64;++iteration){bool valid=true;for(const auto&normal:point.faces){const float missing=-storm_ship_hull::dot(outward,normal);if(missing>0){outward=outward+normal*missing;valid=false;}}outward=storm_ship_hull::normalized(outward);if(valid)break;}
        if(storm_ship_hull::length(outward)<1.e-6f)return {};
        for(const auto&normal:point.faces)if(storm_ship_hull::dot(outward,normal)<-1.e-5f)return {};
        offsetNormals.emplace(entry.first,outward);const auto direction=outward*-1.f;
        const float epsilon=std::max(.0001f,2*std::numeric_limits<float>::epsilon()*std::max({std::abs(point.position.x),std::abs(point.position.y),std::abs(point.position.z),1.f}));
        const auto start=point.position+direction*epsilon,end=point.position+direction*(maximum*4);
        broadphase.queryRay(start,end,candidates);
        for(auto index:candidates){const auto&t=*triangles[index];bool incident=false;for(const auto&v:t)incident|=storm_ship_laser::positionKey(v.position)==entry.first;if(incident)continue;
            float a=0,b=0;const float hit=storm_ship_laser::trace(t,start,end,a,b);if(hit<=1)point.depth=std::min(point.depth,(epsilon+hit*(maximum*4-epsilon))*.25f);}
        const Vec3 reach{maximum*4,maximum*4,maximum*4};broadphase.queryBounds({point.position-reach,point.position+reach},candidates);
        const auto precise=[](Vec3 p){return storm_airburst_hull::Point{p.x,p.y,p.z};};
        for(auto index:candidates){const auto&t=*triangles[index];bool incident=false;for(const auto&v:t)incident|=storm_ship_laser::positionKey(v.position)==entry.first;if(incident)continue;
            const auto p=precise(point.position),q=storm_airburst_hull::closestTriangle(p,precise(t[0].position),precise(t[1].position),precise(t[2].position));
            const double distance=std::sqrt(storm_airburst_hull::dot(p-q,p-q));point.depth=std::min(point.depth,float(distance*.25));}
        limits.emplace(entry.first,point.depth);
    }return limits;
}
inline std::vector<Plane> spherePlanes(const Cut&c) {
    std::vector<Plane>planes;planes.reserve(26);
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z){if(!x&&!y&&!z)continue;
        const auto n=storm_ship_hull::normalized(c.axes[0]*float(x)+c.axes[1]*float(y)+c.axes[2]*float(z));planes.push_back({n,storm_ship_hull::dot(n,c.center)+c.radius});}
    return planes;
}
inline bool crossSection(const Mesh&source,const Plane&plane,Mesh&cap) {
    // Discover only intersections. Re-evaluating an interpolated float against
    // a plane with a fixed epsilon loses seams on kilometre-scale island GMs.
    const auto side=[&](Vec3 p){return double(plane.normal.x)*p.x+double(plane.normal.y)*p.y+double(plane.normal.z)*p.z-plane.distance;};
    const auto before=[](Vec3 a,Vec3 b){return a.x<b.x||(a.x==b.x&&(a.y<b.y||(a.y==b.y&&a.z<b.z)));};
    std::vector<Segment>seam;
    for(const auto&o:source)for(const auto&t:o.second){double d[3]={side(t[0].position),side(t[1].position),side(t[2].position)};
        if(*std::min_element(d,d+3)>=0||*std::max_element(d,d+3)<0)continue;
        std::vector<Vertex>edge;
        for(unsigned n=0;n<3;++n){const auto&a=t[n];const auto&b=t[(n+1)%3];const double da=d[n],db=d[(n+1)%3];
            if(da==0)edge.push_back(a);
            if((da<0&&db>0)||(da>0&&db<0)){const auto&first=before(a.position,b.position)?a:b;const auto&last=before(a.position,b.position)?b:a;
                const double f=side(first.position);edge.push_back(storm_ship_hull::interpolate(first,last,float(std::clamp(f/(f-side(last.position)),0.,1.))));}}
        if(edge.size()==2&&storm_ship_laser::positionKey(edge[0].position)!=storm_ship_laser::positionKey(edge[1].position)){
            const auto face=storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position);
            if(storm_ship_hull::dot(edge[1].position-edge[0].position,storm_ship_hull::cross(plane.normal,face))<0)std::swap(edge[0],edge[1]);
            seam.push_back({edge[0],edge[1],o.first});}
    }
    return caps(seam,plane,cap);
}
inline bool subtractSphereSurface(const Mesh&source,const Cut&cut,Mesh&out) {
    const auto planes=spherePlanes(cut);
    for(const auto&o:source){auto&destination=out[o.first];for(const auto&t:o.second){
        // Convex subtraction preserves untouched triangles byte-for-byte.
        bool far=false;for(const auto&p:planes){bool outside=true;for(const auto&v:t)outside&=storm_ship_hull::dot(p.normal,v.position)>p.distance;if(outside){far=true;break;}}
        if(far){destination.push_back(t);continue;}
        std::vector<Vertex>remaining(t.begin(),t.end()),inside,outside;
        for(const auto&p:planes){storm_ship_laser::linearSplit(remaining,p,inside,outside);storm_ship_hull::fan(outside,destination);remaining.swap(inside);if(remaining.size()<3)break;}
        if(triangleCount(out)>MaxTriangles)return false;}}
    return finite(out);
}
inline bool retainHalfspaceSurface(const Mesh&source,const Plane&plane,Mesh&out) {
    for(const auto&o:source){auto&destination=out[o.first];for(const auto&t:o.second){
        bool inside=true;for(const auto&v:t)inside&=storm_ship_hull::dot(plane.normal,v.position)<=plane.distance;
        if(inside){destination.push_back(t);continue;}
        std::vector<Vertex>a,b;storm_ship_laser::linearSplit(std::vector<Vertex>(t.begin(),t.end()),plane,a,b);storm_ship_hull::fan(a,destination);
    }}return finite(out);
}
inline bool applySurfaceCuts(Mesh source,const std::vector<Cut>&cuts,size_t exclude,Mesh&out,const Plane*boundary=nullptr) {
    for(size_t n=0;n<cuts.size();++n){if(n==exclude)continue;Mesh next;const auto&c=cuts[n];
        if(boundary){
            const auto same=[](const Plane&a,const Plane&b){return a.normal.x==b.normal.x&&a.normal.y==b.normal.y&&a.normal.z==b.normal.z&&a.distance==b.distance;};
            if(c.kind==Cut::Halfspace){const Plane plane{c.normal,c.distance},opposite{c.normal*-1.f,-c.distance};
                if(same(*boundary,opposite)||(same(*boundary,plane)&&n<exclude)){source.clear();break;}
                if(same(*boundary,plane))continue;
            }else if(n>exclude){bool shared=false;for(const auto&p:spherePlanes(c))shared|=same(*boundary,{p.normal*-1.f,-p.distance});
                // A common cavity face belongs to the first record. The later
                // record removes its overlap, preserving one solid wall.
                if(shared)continue;
            }
        }
        if(!(c.kind==Cut::Halfspace?retainHalfspaceSurface(source,{c.normal,c.distance},next):subtractSphereSurface(source,c,next)))return false;
        source=std::move(next);}
    out=std::move(source);return true;
}
// The authored terrain is a top sheet, not a globally manifold mesh: shipped
// seabeds have overlapping projected coasts and vertically stacked shore strips.
// Intersect local triangle columns with each cutter face and normalize their
// planar union. No invented global basement contour can reject the whole scene.
struct WorldPrism {std::array<Vec3,6>vertices;int object=0;};
inline bool removedComponent(const Mesh&source,const std::vector<Cut>&cuts,float shellMargin) {
    for(const auto&cut:cuts){bool inside=true;const auto planes=cut.kind==Cut::Sphere?spherePlanes(cut):std::vector<Plane>{{cut.normal,cut.distance}};
        for(const auto&o:source)for(const auto&t:o.second)for(const auto&v:t)for(const auto&p:planes){
            const double d=double(p.normal.x)*v.position.x+double(p.normal.y)*v.position.y+double(p.normal.z)*v.position.z-p.distance;
            inside&=cut.kind==Cut::Sphere?d<=-shellMargin:d>=shellMargin;}
        if(inside)return true;}
    return false;
}
inline bool prismSection(const WorldPrism&prism,const Plane&plane,const std::vector<Plane>&limits,
    std::vector<PlanarRing>&sections,const storm_ship_hull::Frame&basis,Vec3 origin) {
    // A consistent three-tetrahedron subdivision also supports varying inward
    // depths without replacing a non-convex swept triangle by its convex hull.
    constexpr unsigned tetra[3][4]={{0,1,2,5},{0,1,4,5},{0,3,4,5}};
    const auto side=[&](Vec3 p){return double(plane.normal.x)*p.x+double(plane.normal.y)*p.y+double(plane.normal.z)*p.z-plane.distance;};
    for(const auto&indices:tetra){std::vector<Vertex>points;
        for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const auto x=prism.vertices[indices[a]],y=prism.vertices[indices[b]];const double dx=side(x),dy=side(y);
            Vec3 p;if(dx==0)p=x;else if(dy==0)p=y;else if((dx<0)==(dy<0))continue;else {const double t=dx/(dx-dy);p={float(x.x+(double(y.x)-x.x)*t),float(x.y+(double(y.y)-x.y)*t),float(x.z+(double(y.z)-x.z)*t)};}
            bool duplicate=false;for(const auto&v:points)duplicate|=storm_ship_laser::positionKey(v.position)==storm_ship_laser::positionKey(p);if(!duplicate){Vertex v{};v.position=p;points.push_back(v);}}
        if(points.size()<3)continue;
        Vec3 center{};for(const auto&v:points)center=center+v.position*(1.f/float(points.size()));
        std::sort(points.begin(),points.end(),[&](const Vertex&a,const Vertex&b){const auto x=a.position-center,y=b.position-center;
            return std::atan2(storm_ship_hull::dot(x,basis.bitangent),storm_ship_hull::dot(x,basis.tangent))<std::atan2(storm_ship_hull::dot(y,basis.bitangent),storm_ship_hull::dot(y,basis.tangent));});
        for(const auto&limit:limits){std::vector<Vertex>inside,outside;storm_ship_laser::linearSplit(points,limit,inside,outside);points=std::move(inside);if(points.size()<3)break;}
        if(points.size()<3)continue;PlanarRing ring;
        for(const auto&v:points){const double x=double(v.position.x)-origin.x,y=double(v.position.y)-origin.y,z=double(v.position.z)-origin.z;
            ring.push_back({x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z});}
        double area=0;for(size_t n=0;n<ring.size();++n){const auto&a=ring[n],&b=ring[(n+1)%ring.size()];area+=a[0]*b[1]-b[0]*a[1];}
        if(std::abs(area)<=1.e-10)continue;if(area<0)std::reverse(ring.begin(),ring.end());sections.push_back(std::move(ring));
        if(sections.size()>MaxTriangles*3)return false;
    }return true;
}
struct CapAppearance {float density=0;uint32_t color=0;std::array<float,2>anchor{};};
inline CapAppearance capAppearance(const std::vector<Triangle>&source) {
    CapAppearance result;if(source.empty())return result;result.color=source.front()[0].color;result.anchor={source.front()[0].uv[0],source.front()[0].uv[1]};
    std::vector<float>densities;double weight=0,color[4]{};
    for(const auto&t:source){const double area=storm_ship_hull::area(t);if(area<=1.e-10)continue;
        const double uv=std::abs((double(t[1].uv[0])-t[0].uv[0])*(double(t[2].uv[1])-t[0].uv[1])-(double(t[1].uv[1])-t[0].uv[1])*(double(t[2].uv[0])-t[0].uv[0]));
        const double density=std::sqrt(uv/(2*area));if(std::isfinite(density)&&density>0)densities.push_back(float(density));
        weight+=area;for(const auto&v:t)for(unsigned c=0;c<4;++c)color[c]+=double((v.color>>(c*8))&255u)*area/3;
    }
    if(!densities.empty()){auto middle=densities.begin()+densities.size()/2;std::nth_element(densities.begin(),middle,densities.end());result.density=*middle;}
    if(weight>0){result.color=0;for(unsigned c=0;c<4;++c)result.color|=uint32_t(std::clamp(std::lround(color[c]/weight),0l,255l))<<(c*8);}
    return result;
}
inline bool triangulatePlanarAttempt(const std::vector<PlanarPolygon>&polygons,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance,bool integer) {
    const auto basis=storm_ship_hull::frame(plane.normal);Mesh result;
    for(const auto&polygon:polygons){std::vector<PlanarPoint>points;double expected=0;
        for(size_t r=0;r<polygon.size();++r){const auto&ring=polygon[r];if(ring.size()<3)return false;double area=0;for(size_t n=0;n<ring.size();++n){const auto&a=ring[n],&b=ring[(n+1)%ring.size()];area+=a[0]*b[1]-b[0]*a[1];}expected+=(r?-1:1)*std::abs(area)*.5;points.insert(points.end(),ring.begin(),ring.end());}
        std::vector<uint32_t>indices;if(integer){if(!integerTriangulation(polygon,indices))return false;}else indices=mapbox::earcut<uint32_t>(polygon);double measured=0;
        using Edge=std::pair<PlanarPoint,PlanarPoint>;std::map<Edge,unsigned>incidence;
        for(size_t n=0;n+2<indices.size();n+=3){Triangle t{};double area=0;const auto&a=points[indices[n]],&b=points[indices[n+1]],&c=points[indices[n+2]];
            area=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);measured+=std::abs(area)*.5;if(std::abs(area)<=1.e-12)continue;
            for(unsigned e=0;e<3;++e){auto x=points[indices[n+e]],y=points[indices[n+(e+1)%3]];if(y<x)std::swap(x,y);if(++incidence[{x,y}]>2)return false;}
            for(unsigned v=0;v<3;++v){const auto&p=points[indices[n+v]];auto&vertex=t[v];vertex.position={float(origin.x+p[0]*basis.tangent.x+p[1]*basis.bitangent.x),float(origin.y+p[0]*basis.tangent.y+p[1]*basis.bitangent.y),float(origin.z+p[0]*basis.tangent.z+p[1]*basis.bitangent.z)};vertex.normal=plane.normal;vertex.color=appearance.color;
                // New solid faces have no authored island-mask atlas. Their
                // separate base-only draw uses the source texture's density.
                vertex.uv[0]=float(p[0]*appearance.density)+appearance.anchor[0];vertex.uv[1]=float(p[1]*appearance.density)+appearance.anchor[1];}
            if(area<0)std::swap(t[1],t[2]);result[object].push_back(t);}
        if(expected<0||std::abs(measured-expected)>std::max(1.e-7,expected*1.e-6))return false;
        std::vector<Edge>wanted,actual;for(const auto&ring:polygon)for(size_t e=0;e<ring.size();++e)wanted.emplace_back(ring[e],ring[(e+1)%ring.size()]);
        for(const auto&e:incidence)if(e.second==1)actual.push_back(e.first);
        // Earcut may drop collinear vertices. Compare covered segments, not
        // index spelling, and reject either missing or invented boundary edges.
        const auto covered=[](const Edge&edge,const std::vector<Edge>&by){
 const auto key=[](const PlanarPoint&p){return std::array<int64_t,2>{std::llround(p[0]*1000000.),std::llround(p[1]*1000000.)};};auto a=key(edge.first),b=key(edge.second);const int64_t dx=b[0]-a[0],dy=b[1]-a[1];if(!dx&&!dy)return true;const __int128 length=__int128(dx)*dx+__int128(dy)*dy;std::vector<std::pair<__int128,__int128>>intervals;
 for(const auto&candidate:by){auto x=key(candidate.first),y=key(candidate.second);if(__int128(dx)*(x[1]-a[1])-__int128(dy)*(x[0]-a[0])||__int128(dx)*(y[1]-a[1])-__int128(dy)*(y[0]-a[0]))continue;
 __int128 p=__int128(x[0]-a[0])*dx+__int128(x[1]-a[1])*dy,q=__int128(y[0]-a[0])*dx+__int128(y[1]-a[1])*dy;intervals.emplace_back(std::min(p,q),std::max(p,q));}
 std::sort(intervals.begin(),intervals.end());__int128 end=0;for(const auto&i:intervals){if(i.first>end)break;end=std::max(end,i.second);if(end>=length)return true;}return false;};
 for(const auto&e:wanted)if(!covered(e,actual)){std::cerr.precision(17);std::cerr<<"missing edge integer="<<integer<<" "<<e.first[0]<<","<<e.first[1]<<" -> "<<e.second[0]<<","<<e.second[1]<<" expected="<<expected<<" measured="<<measured<<std::endl;for(const auto&a:actual)std::cerr<<"actual "<<a.first[0]<<","<<a.first[1]<<" -> "<<a.second[0]<<","<<a.second[1]<<std::endl;for(const auto&w:wanted)std::cerr<<"wanted "<<w.first[0]<<","<<w.first[1]<<" -> "<<w.second[0]<<","<<w.second[1]<<std::endl;return false;}for(const auto&e:actual)if(!covered(e,wanted)){std::cerr<<"invented edge integer="<<integer<<std::endl;return false;}
        if(triangleCount(cap)+triangleCount(result)>MaxTriangles)return false;
    }if(!finite(result))return false;storm_ship_laser::appendMesh(cap,result);return true;
}
inline bool triangulatePlanar(const std::vector<PlanarPolygon>&polygons,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance) {
    return triangulatePlanarAttempt(polygons,plane,object,origin,cap,appearance,false)||triangulatePlanarAttempt(polygons,plane,object,origin,cap,appearance,true);
}
inline bool planarCap(const std::vector<PlanarRing>&sections,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance) {
    if(sections.empty())return true;std::vector<PlanarPolygon>polygons;if(!unitePlanar(sections,polygons))return false;
    return triangulatePlanar(polygons,plane,object,origin,cap,appearance);
}
struct CapPatch {
    uint32_t field=0,face=0;
    Plane plane{};Vec3 origin{};int object=0;bool reverse=false;
    CapAppearance appearance;Mesh surface;
};
inline bool reconstructWorld(const Mesh&original,const std::set<int>&terrainObjects,const std::vector<Cut>&cuts,Mesh&out,Mesh*outerOutput=nullptr,Mesh*capOutput=nullptr,std::vector<CapPatch>*patchesOutput=nullptr) {
    if(cuts.size()>MaxCuts||!finite(original))return false;for(const auto&c:cuts)if(!valid(c))return false;
    std::vector<Cut>unique;for(const auto&c:cuts){bool duplicate=false;for(const auto&p:unique)duplicate|=equal(c,p);if(!duplicate)unique.push_back(c);}
    Mesh surface,stone,closed;std::vector<WorldPrism>prisms;std::map<int,CapAppearance>appearances;
    for(const auto&o:original)appearances.emplace(o.first,capAppearance(o.second));
    const float floor=storm_ship_laser::bounds(original).lo.y-10032.f;
    const auto addPrism=[&](const Triangle&t,const std::array<Vec3,3>&inner,int object){std::array<unsigned,3>order{0,1,2};std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return storm_ship_laser::positionKey(t[a].position)<storm_ship_laser::positionKey(t[b].position);});
        WorldPrism prism;prism.object=object;for(unsigned n=0;n<3;++n){prism.vertices[n]=t[order[n]].position;prism.vertices[n+3]=inner[order[n]];}prisms.push_back(prism);};
    for(const auto&o:original){surface.try_emplace(o.first);if(!terrainObjects.count(o.first)){stone[o.first]=o.second;continue;}
        surface[o.first]=o.second;for(const auto&t:o.second){auto box=storm_ship_hull::Bounds{t[0].position,t[0].position};for(const auto&v:t)box=storm_ship_hull::merge(box,{v.position,v.position});box.lo.y=floor;
            if(!affects(box,unique))continue;std::array<Vec3,3>inner;for(unsigned n=0;n<3;++n){inner[n]=t[n].position;inner[n].y=floor;}addPrism(t,inner,o.first);}}
    for(const auto&component:components(stone)){auto box=storm_ship_laser::bounds(component);box.lo=box.lo-Vec3{1,1,1};box.hi=box.hi+Vec3{1,1,1};
        if(!affects(box,unique)){storm_ship_laser::appendMesh(surface,component);continue;}
        if(removedComponent(component,unique,1.f))continue;
        std::vector<Segment>boundary;if(!boundaries(component,boundary))return false;
        if(boundary.empty()){storm_ship_laser::appendMesh(surface,component);storm_ship_laser::appendMesh(closed,component);continue;}
        std::map<PositionKey,Vec3>normals;const auto depths=stoneClearance(component,1,normals);if(depths.empty())return false;
        std::map<int,float>thickness;for(const auto&o:component)thickness[o.first]=1;
        auto shell=storm_ship_laser::shellForRender(component,thickness,&depths,&normals);storm_ship_laser::appendMesh(surface,shell);
        for(const auto&o:component)for(const auto&t:o.second){std::array<Vec3,3>inner;for(unsigned n=0;n<3;++n){const auto key=storm_ship_laser::positionKey(t[n].position);inner[n]=t[n].position-normals.at(key)*depths.at(key);}addPrism(t,inner,o.first);}
        if(triangleCount(surface)>MaxTriangles)return false;
    }
    Mesh outer,allCaps;std::vector<CapPatch>patches;if(!applySurfaceCuts(surface,unique,unique.size(),outer))return false;
    for(size_t n=0;n<unique.size();++n){const auto&c=unique[n];const auto planes=c.kind==Cut::Sphere?spherePlanes(c):std::vector<Plane>{{c.normal,c.distance}};
        for(size_t side=0;side<planes.size();++side){const auto&plane=planes[side];const auto basis=storm_ship_hull::frame(plane.normal);const Vec3 origin=plane.normal*plane.distance;
            std::vector<Plane>limits;if(c.kind==Cut::Sphere)for(size_t p=0;p<planes.size();++p)if(p!=side)limits.push_back(planes[p]);
            std::vector<PlanarRing>sections;int object=original.empty()?0:original.begin()->first;
            for(const auto&prism:prisms){const auto before=sections.size();if(!prismSection(prism,plane,limits,sections,basis,origin))return false;if(sections.size()!=before)object=prism.object;}
            // Closed authored components use their own oriented cross-section,
            // preserving cavities/windows rather than filling a convex hull.
            if(triangleCount(closed)){Mesh cap;if(!crossSection(closed,plane,cap))return false;
                for(const auto&limit:limits){Mesh clipped;if(!retainHalfspaceSurface(cap,limit,clipped))return false;cap=std::move(clipped);}
                for(const auto&o:cap)for(const auto&t:o.second){PlanarRing ring;for(const auto&v:t){auto p=v.position-origin;ring.push_back({storm_ship_hull::dot(p,basis.tangent),storm_ship_hull::dot(p,basis.bitangent)});}double area=0;for(unsigned j=0;j<3;++j)area+=ring[j][0]*ring[(j+1)%3][1]-ring[(j+1)%3][0]*ring[j][1];if(area<0)std::reverse(ring.begin(),ring.end());sections.push_back(std::move(ring));object=o.first;}}
            if(sections.empty())continue;Mesh cap;if(!planarCap(sections,plane,object,origin,cap,appearances.at(object)))return false;
            if(c.kind==Cut::Sphere)for(auto&o:cap)for(auto&t:o.second){std::swap(t[1],t[2]);for(auto&v:t)v.normal=plane.normal*-1.f;}
            const Plane boundary=c.kind==Cut::Sphere?Plane{plane.normal*-1.f,-plane.distance}:plane;Mesh retained;
            if(!applySurfaceCuts(std::move(cap),unique,n,retained,&boundary))return false;
            if(patchesOutput&&triangleCount(retained)){CapPatch patch;patch.field=c.field;patch.face=uint32_t(side);patch.plane=plane;patch.origin=origin;patch.object=object;patch.reverse=c.kind==Cut::Sphere;patch.appearance=appearances.at(object);patch.surface=retained;patches.push_back(std::move(patch));}
            storm_ship_laser::appendMesh(allCaps,retained);if(triangleCount(outer)+triangleCount(allCaps)>MaxTriangles)return false;
        }}
    for(const auto&o:original)outer.try_emplace(o.first);Mesh result=outer;storm_ship_laser::appendMesh(result,allCaps);if(!finite(result))return false;
    out=std::move(result);if(outerOutput)*outerOutput=std::move(outer);if(capOutput)*capOutput=std::move(allCaps);if(patchesOutput)*patchesOutput=std::move(patches);return true;
}
inline bool resolveCapCoverage(const std::vector<CapPatch>&patches,const std::vector<CapTriangle>&earlier,Mesh&capOutput,std::vector<CapTriangle>&acceptedOutput) {
    if(earlier.size()>MaxTriangles)return false;
    using Key=std::pair<uint32_t,uint32_t>;std::map<Key,std::vector<const CapTriangle*>>byKey;
    for(const auto&t:earlier){for(const auto&p:t.positions)for(float v:p)if(!std::isfinite(v)||std::abs(v)>10000000)return false;byKey[{t.field,t.face}].push_back(&t);}
    Mesh caps;std::vector<CapTriangle>accepted;
    for(const auto&patch:patches){Mesh retained;const auto found=byKey.find({patch.field,patch.face});
        if(found==byKey.end())retained=patch.surface;
        else {std::cerr<<"resolve face="<<patch.face<<std::endl;const auto basis=storm_ship_hull::frame(patch.plane.normal);std::vector<PlanarRing>subject,clip;
            const auto project=[&](Vec3 p){const double x=double(p.x)-patch.origin.x,y=double(p.y)-patch.origin.y,z=double(p.z)-patch.origin.z;
                return PlanarPoint{x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z};};
            const auto positive=[](PlanarRing&ring){double area=0;for(unsigned n=0;n<3;++n)area+=ring[n][0]*ring[(n+1)%3][1]-ring[(n+1)%3][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());};
            for(const auto&o:patch.surface)for(const auto&t:o.second){PlanarRing ring;for(const auto&v:t)ring.push_back(project(v.position));positive(ring);subject.push_back(std::move(ring));}
            for(const auto*t:found->second){PlanarRing ring;for(const auto&p:t->positions){const Vec3 point{p[0],p[1],p[2]};
                    const double tolerance=std::max(1.e-4,8.*std::numeric_limits<float>::epsilon()*std::max({std::abs(double(point.x)),std::abs(double(point.y)),std::abs(double(point.z)),std::abs(double(patch.plane.distance)),1.}));
                    const double distance=double(patch.plane.normal.x)*point.x+double(patch.plane.normal.y)*point.y+double(patch.plane.normal.z)*point.z-patch.plane.distance;
                    if(std::abs(distance)>tolerance)return false;ring.push_back(project(point));}positive(ring);clip.push_back(std::move(ring));}
            std::vector<PlanarPolygon>polygons;if(!differencePlanar(subject,clip,polygons)||!triangulatePlanar(polygons,patch.plane,patch.object,patch.origin,retained,patch.appearance))return false;
            if(patch.reverse)for(auto&o:retained)for(auto&t:o.second){std::swap(t[1],t[2]);for(auto&v:t)v.normal=patch.plane.normal*-1.f;}
        }
        for(const auto&o:retained)for(const auto&t:o.second){CapTriangle record;record.field=patch.field;record.face=patch.face;
            for(unsigned n=0;n<3;++n)record.positions[n]={t[n].position.x,t[n].position.y,t[n].position.z};accepted.push_back(record);}
        storm_ship_laser::appendMesh(caps,retained);if(accepted.size()>MaxTriangles)return false;
    }
    if(!finite(caps))return false;capOutput=std::move(caps);acceptedOutput=std::move(accepted);return true;
}
} // namespace storm_world_cut

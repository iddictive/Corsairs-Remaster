#pragma once
#include "geometry_work_budget.hpp"
#include "ship_laser_geometry.hpp"
#include "ship_airburst_hull_distance.hpp"
#include "ship_hull_material.hpp"
#include "world_cut_coverage.hpp"
#include <set>
#include <tuple>
#include <string_view>
#include <memory>

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
// MaxCuts bounds one legacy restore / admitted operation, not an island's life.
inline constexpr size_t MaxCuts=64, MaxPendingCuts=8, MaxTriangles=1000000;
inline constexpr size_t MaxCheckpointBytes=128*1024*1024;
using PlanarPoint=std::array<double,2>;
using PlanarRing=std::vector<PlanarPoint>;
using PlanarPolygon=std::vector<PlanarRing>;
inline constexpr int64_t PlanarUnitsPerMeter=storm_ship_laser::PlanarUnitsPerMeter;
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
    enum Kind : uint32_t { Halfspace=0, Sphere=1, Beam=2 };
    Kind kind=Halfspace;
    Vec3 normal{0,1,0}; float distance=0;
    Vec3 center{}; float radius=1;
    uint32_t field=0; // Derived binding identity; never part of shape equality.
    std::array<Vec3,3>axes{{{1,0,0},{0,1,0},{0,0,1}}};
    std::vector<storm_ship_laser::BeamSample> beams;
    uint32_t faceOffset=0; // Derived identity of a sample within a saved stroke.
};
inline bool valid(const Cut&c) {
    if(c.kind==Cut::Beam){if(c.beams.empty()||c.beams.size()>storm_hull_damage::MaxBeamSamples)return false;
        for(const auto&b:c.beams)if(!storm_ship_laser::valid(b))return false;return true;}
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
    if(a.kind==Cut::Beam){if(a.beams.size()!=b.beams.size())return false;for(size_t n=0;n<a.beams.size();++n){const auto&x=a.beams[n];const auto&y=b.beams[n];
        if(x.start!=y.start||x.direction!=y.direction||x.tangent!=y.tangent||x.radius!=y.radius||x.depth!=y.depth||
           x.crossSection!=y.crossSection||x.backDepth!=y.backDepth||x.specificWork!=y.specificWork||x.contactStrength!=y.contactStrength)return false;}return true;}
    if(a.kind==Cut::Halfspace)return a.normal.x==b.normal.x&&a.normal.y==b.normal.y&&a.normal.z==b.normal.z&&a.distance==b.distance;
    if(a.center.x!=b.center.x||a.center.y!=b.center.y||a.center.z!=b.center.z||a.radius!=b.radius)return false;
    for(unsigned n=0;n<3;++n)if(a.axes[n].x!=b.axes[n].x||a.axes[n].y!=b.axes[n].y||a.axes[n].z!=b.axes[n].z)return false;return true;
}
inline size_t triangleCount(const Mesh&m){size_t n=0;for(const auto&o:m){storm_geometry_work::checkpoint();n+=o.second.size();}return n;}
inline bool affects(const storm_ship_hull::Bounds&b,const std::vector<Cut>&cuts) {
    for(const auto&c:cuts){storm_geometry_work::checkpoint();if(c.kind==Cut::Beam){for(const auto&beam:c.beams){storm_geometry_work::checkpoint();const auto a=storm_ship_laser::beamStart(beam),z=a+storm_ship_laser::beamDirection(beam)*beam.depth;
            const float r=beam.radius*1.083f+.025f;const Vec3 lo{std::min(a.x,z.x)-r,std::min(a.y,z.y)-r,std::min(a.z,z.z)-r},hi{std::max(a.x,z.x)+r,std::max(a.y,z.y)+r,std::max(a.z,z.z)+r};
            if(hi.x>=b.lo.x&&lo.x<=b.hi.x&&hi.y>=b.lo.y&&lo.y<=b.hi.y&&hi.z>=b.lo.z&&lo.z<=b.hi.z)return true;}}
        else if(c.kind==Cut::Halfspace){const Vec3 support{c.normal.x>=0?b.hi.x:b.lo.x,c.normal.y>=0?b.hi.y:b.lo.y,c.normal.z>=0?b.hi.z:b.lo.z};
            if(double(c.normal.x)*support.x+double(c.normal.y)*support.y+double(c.normal.z)*support.z>c.distance)return true;
        }else {const Vec3 reach{c.radius*(std::abs(c.axes[0].x)+std::abs(c.axes[1].x)+std::abs(c.axes[2].x)),c.radius*(std::abs(c.axes[0].y)+std::abs(c.axes[1].y)+std::abs(c.axes[2].y)),c.radius*(std::abs(c.axes[0].z)+std::abs(c.axes[1].z)+std::abs(c.axes[2].z))};
            if(c.center.x+reach.x>=b.lo.x&&c.center.x-reach.x<=b.hi.x&&c.center.y+reach.y>=b.lo.y&&c.center.y-reach.y<=b.hi.y&&c.center.z+reach.z>=b.lo.z&&c.center.z-reach.z<=b.hi.z)return true;}}
    return false;
}
inline bool finite(const Mesh&m) {
    if(triangleCount(m)>MaxTriangles)return false;
    for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t){storm_geometry_work::checkpoint();
        if(!storm_ship_hull::finite(v.position)||!storm_ship_hull::finite(v.normal)||
           std::max({std::abs(v.position.x),std::abs(v.position.y),std::abs(v.position.z)})>10000000)return false;
    }
    return true;
}
struct Boundary {Vertex a,b;int object=0;unsigned count=0;int winding=0;};
inline bool boundaries(const Mesh&mesh,std::vector<Segment>&out) {
    std::unordered_map<PositionEdge,Boundary,storm_ship_laser::PositionEdgeHash>edges;
    edges.reserve(triangleCount(mesh)*2);
    for(const auto&o:mesh)for(const auto&t:o.second)for(unsigned n=0;n<3;++n){
        storm_geometry_work::checkpoint();
        auto a=storm_ship_laser::positionKey(t[n].position),b=storm_ship_laser::positionKey(t[(n+1)%3].position);if(a==b)continue;
        const int direction=a<b?1:-1;auto ordered=std::minmax(a,b);auto&e=edges[{ordered.first,ordered.second}];
        if(!e.count){e.a=t[n];e.b=t[(n+1)%3];e.object=o.first;}++e.count;e.winding+=direction;
    }
    for(const auto&e:edges){storm_geometry_work::checkpoint();if(e.second.count>2||(e.second.count==2&&e.second.winding))return false;
        if(e.second.count==1)out.push_back({e.second.a,e.second.b,e.second.object});}
    // Deterministic reconstruction also fixes cap material ownership.
    std::sort(out.begin(),out.end(),[](const Segment&a,const Segment&b){storm_geometry_work::checkpoint();return
        std::make_tuple(storm_ship_laser::positionKey(a.a.position),storm_ship_laser::positionKey(a.b.position),a.object)<
        std::make_tuple(storm_ship_laser::positionKey(b.a.position),storm_ship_laser::positionKey(b.b.position),b.object);});
    return true;
}
inline bool caps(const std::vector<Segment>&seam,const Plane&plane,Mesh&out) {
    if(seam.empty())return true;
    using Point=std::array<double,2>;
    struct Incidence{Segment edge;int winding=0;};std::map<PositionEdge,Incidence>incidence;
    for(const auto&e:seam){storm_geometry_work::checkpoint();auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);if(a==b)continue;
        auto ordered=std::minmax(a,b);auto&entry=incidence[{ordered.first,ordered.second}];entry.edge=e;entry.winding+=a<b?1:-1;}
    std::map<PositionKey,std::vector<size_t>>outgoing;std::map<PositionKey,int>balance;std::set<PositionEdge>unique;std::vector<Segment>edges;
    for(const auto&entry:incidence){storm_geometry_work::checkpoint();if(!entry.second.winding)continue;if(std::abs(entry.second.winding)!=1)return false;auto e=entry.second.edge;
        auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);if((a<b)!=(entry.second.winding>0)){std::swap(e.a,e.b);std::swap(a,b);}
        unique.insert(entry.first);outgoing[a].push_back(edges.size());++balance[a];--balance[b];edges.push_back(e);}
    if(unique.empty())return true;if(unique.size()>65536)return false;
    for(const auto&p:balance){storm_geometry_work::checkpoint();if(p.second)return false;}
    const auto basis=storm_ship_hull::frame(plane.normal);const auto origin=edges.front().a.position;
    const double positionTolerance=std::max(1.e-6,2.*std::numeric_limits<float>::epsilon()*
        std::max({std::abs(double(origin.x)),std::abs(double(origin.y)),std::abs(double(origin.z)),std::abs(double(plane.distance)),1.}));
    const auto project=[&](Vec3 p){const double x=double(p.x)-origin.x,y=double(p.y)-origin.y,z=double(p.z)-origin.z;return Point{
        x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z};};
    struct Ring{std::vector<Vertex>vertices;std::vector<Point>points;double area=0;int object=0,parent=-1,depth=0;};
    std::vector<Ring>rings;std::vector<bool>used(edges.size());
    for(size_t start=0;start<edges.size();++start){storm_geometry_work::checkpoint();if(used[start])continue;Ring ring;ring.object=edges[start].object;
        const auto first=storm_ship_laser::positionKey(edges[start].a.position);auto current=first;size_t next=start;
        do{storm_geometry_work::checkpoint();if(used[next])return false;used[next]=true;const auto&e=edges[next];ring.vertices.push_back(e.a);current=storm_ship_laser::positionKey(e.b.position);if(current==first)break;
            const auto incoming=project(e.a.position),point=project(e.b.position);double best=-4;next=edges.size();
            for(auto candidate:outgoing[current]){storm_geometry_work::checkpoint();if(!used[candidate]){const auto end=project(edges[candidate].b.position);
                const double ax=point[0]-incoming[0],ay=point[1]-incoming[1],bx=end[0]-point[0],by=end[1]-point[1];
                const double turn=std::atan2(ax*by-ay*bx,ax*bx+ay*by);if(next==edges.size()||turn>best){best=turn;next=candidate;}}}
            if(next==edges.size())return false;
        }while(ring.vertices.size()<=edges.size());
        if(current!=first||ring.vertices.size()<3)return false;
        for(const auto&v:ring.vertices){storm_geometry_work::checkpoint();ring.points.push_back(project(v.position));}
        for(size_t n=0;n<ring.points.size();++n){storm_geometry_work::checkpoint();const auto a=ring.points[n],b=ring.points[(n+1)%ring.points.size()];ring.area+=a[0]*b[1]-b[0]*a[1];}
        ring.area=std::abs(ring.area)*.5;if(ring.area<=1.e-8)return false;rings.push_back(std::move(ring));}
    const auto contains=[](const std::vector<Point>&ring,Point q){bool inside=false;for(size_t n=0,j=ring.size()-1;n<ring.size();j=n++){storm_geometry_work::checkpoint();const auto a=ring[n],b=ring[j];if((a[1]>q[1])!=(b[1]>q[1])&&q[0]<(b[0]-a[0])*(q[1]-a[1])/(b[1]-a[1])+a[0])inside=!inside;}return inside;};
    for(size_t n=0;n<rings.size();++n){double area=std::numeric_limits<double>::infinity();for(size_t j=0;j<rings.size();++j){storm_geometry_work::checkpoint();
        if(j!=n&&rings[j].area>rings[n].area&&rings[j].area<area&&contains(rings[j].points,rings[n].points.front())){rings[n].parent=int(j);area=rings[j].area;}}}
    for(auto&ring:rings)for(int p=ring.parent;p>=0;p=rings[p].parent){storm_geometry_work::checkpoint();++ring.depth;}
    for(size_t n=0;n<rings.size();++n){storm_geometry_work::checkpoint();if(rings[n].depth%2)continue;
        std::vector<std::vector<Point>>polygon;std::vector<Vertex>vertices;std::vector<uint32_t>cornerIndices;std::vector<std::pair<size_t,size_t>>ranges;double expected=0;
        const auto add=[&](const Ring&r,bool hole){const size_t base=vertices.size();ranges.emplace_back(base,r.vertices.size());vertices.insert(vertices.end(),r.vertices.begin(),r.vertices.end());expected+=hole?-r.area:r.area;
            std::vector<size_t>corners(r.points.size());std::iota(corners.begin(),corners.end(),0);bool changed=true;
            while(changed&&corners.size()>3){changed=false;for(size_t k=0;k<corners.size();++k){storm_geometry_work::checkpoint();const auto a=r.points[corners[(k+corners.size()-1)%corners.size()]],b=r.points[corners[k]],c=r.points[corners[(k+1)%corners.size()]];
                const double x=c[0]-a[0],y=c[1]-a[1],along=(b[0]-a[0])*x+(b[1]-a[1])*y;
                bool linear=along>=0&&along<=x*x+y*y;
                for(size_t q=(corners[(k+corners.size()-1)%corners.size()]+1)%r.points.size();linear&&q!=corners[(k+1)%corners.size()];q=(q+1)%r.points.size()){
                    storm_geometry_work::checkpoint();
                    const auto p=r.points[q];const double step=(p[0]-a[0])*x+(p[1]-a[1])*y;
                    linear=step>=0&&step<=x*x+y*y&&std::abs(x*(p[1]-a[1])-y*(p[0]-a[0]))<=positionTolerance*std::hypot(x,y);}
                if(linear){
                    corners.erase(corners.begin()+k);changed=true;break;}}}
            polygon.emplace_back();for(auto k:corners){storm_geometry_work::checkpoint();polygon.back().push_back(r.points[k]);cornerIndices.push_back(uint32_t(base+k));}
        };
        add(rings[n],false);for(size_t j=0;j<rings.size();++j){storm_geometry_work::checkpoint();if(rings[j].parent==int(n))add(rings[j],true);}
        storm_geometry_work::checkpoint(true);auto indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);double measured=0;
        for(auto&index:indices){storm_geometry_work::checkpoint();index=cornerIndices[index];}
        std::vector<uint32_t>nondegenerate;nondegenerate.reserve(indices.size());
        for(size_t k=0;k+2<indices.size();k+=3){storm_geometry_work::checkpoint();if(storm_ship_hull::area({vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]})>1.e-10f)
            nondegenerate.insert(nondegenerate.end(),indices.begin()+k,indices.begin()+k+3);
        }
        indices=std::move(nondegenerate);
        std::map<std::pair<uint32_t,uint32_t>,unsigned>incidence;
        for(size_t k=0;k+2<indices.size();k+=3)for(unsigned c=0;c<3;++c){storm_geometry_work::checkpoint();auto a=indices[k+c],b=indices[k+(c+1)%3];if(b<a)std::swap(a,b);++incidence[{a,b}];}
        for(size_t k=0;k+2<indices.size();k+=3){storm_geometry_work::checkpoint();std::vector<Vertex>boundary;bool expanded=false;
            const auto pa=project(vertices[indices[k]].position),pb=project(vertices[indices[k+1]].position),pc=project(vertices[indices[k+2]].position);
            const bool reverse=(pb[0]-pa[0])*(pc[1]-pa[1])-(pb[1]-pa[1])*(pc[0]-pa[0])<0;
            for(unsigned c=0;c<3;++c){const uint32_t a=indices[k+c],b=indices[k+(c+1)%3];boundary.push_back(vertices[a]);auto ordered=std::minmax(a,b);
                if(incidence[{ordered.first,ordered.second}]!=1)continue;
                for(const auto&range:ranges){storm_geometry_work::checkpoint();if(a<range.first||a>=range.first+range.second||b<range.first||b>=range.first+range.second)continue;
                    for(int direction:{1,-1}){std::vector<Vertex>between;size_t current=a;bool linear=true;
                        const auto pa=project(vertices[a].position),pb=project(vertices[b].position);const double dx=pb[0]-pa[0],dy=pb[1]-pa[1],length=std::hypot(dx,dy);
                        for(size_t step=0;step<range.second;++step){storm_geometry_work::checkpoint();current=range.first+(current-range.first+range.second+direction)%range.second;if(current==b)break;const auto p=project(vertices[current].position);
                            const double along=(p[0]-pa[0])*dx+(p[1]-pa[1])*dy;
                            if(std::abs(dx*(p[1]-pa[1])-dy*(p[0]-pa[0]))>length*positionTolerance||along<=0||along>=length*length){linear=false;break;}between.push_back(vertices[current]);}
                        if(linear){boundary.insert(boundary.end(),between.begin(),between.end());expanded|=!between.empty();break;}}
                }
            }
            std::vector<Triangle>faces;
            if(!expanded)faces.push_back({vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]});
            else{auto center=storm_ship_hull::interpolate(storm_ship_hull::interpolate(vertices[indices[k]],vertices[indices[k+1]],.5f),vertices[indices[k+2]],1.f/3.f);
                for(size_t p=0;p<boundary.size();++p){storm_geometry_work::checkpoint();faces.push_back({center,boundary[p],boundary[(p+1)%boundary.size()]});}}
            for(auto t:faces){storm_geometry_work::checkpoint();const auto a=project(t[0].position),b=project(t[1].position),c=project(t[2].position);const double signedArea=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
                if(reverse)std::swap(t[1],t[2]);const double area=std::abs(signedArea)*.5;if(area<=1.e-10)continue;measured+=area;
                for(auto&v:t){v.normal=plane.normal;const auto uv=project(v.position);for(unsigned channel=0;channel<4;++channel){v.uv[channel*2]=float(uv[0]*.12);v.uv[channel*2+1]=float(uv[1]*.12);}}out[rings[n].object].push_back(t);}
        }
        if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4))return false;
    }
    // Every contour must appear in the output boundary, including holes.
    std::vector<Segment>actual;if(!boundaries(out,actual))return false;
    std::set<PositionEdge>capEdges;
    for(const auto&e:actual){storm_geometry_work::checkpoint();auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);auto ordered=std::minmax(a,b);capEdges.insert({ordered.first,ordered.second});}
    return capEdges==unique;
}
inline void surfaceNormal(Triangle&t) {
    const auto n=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
    for(auto&v:t)v.normal=n;
}
// Connected closed components stay authored. Only open stone/town components
// acquire the shared shell implementation, preserving doors and other openings.
using storm_ship_laser::components;
inline std::map<PositionKey,float> stoneClearance(const Mesh&source,float maximum,std::map<PositionKey,Vec3>&offsetNormals) {
    struct Point{Vec3 position{},normal{};float depth=0;std::vector<Vec3>faces;};std::map<PositionKey,Point>points;
    struct Feature{Vec3 normal{};unsigned count=0;bool sharp=false;float length=0,altitude=std::numeric_limits<float>::infinity();};
    std::map<PositionEdge,Feature>features;std::vector<const Triangle*>triangles;std::vector<storm_ship_hull::Bounds>boxes;
    for(const auto&o:source)for(const auto&t:o.second){storm_geometry_work::checkpoint();const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
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
    for(const auto&e:features){storm_geometry_work::checkpoint();if(e.second.count==1||e.second.sharp)for(const auto&p:{e.first.first,e.first.second})points.at(p).depth=std::min(points.at(p).depth,std::min(e.second.length,e.second.altitude)*.25f);}
    storm_ship_hull::Broadphase broadphase;broadphase.build(boxes);std::vector<uint32_t>candidates;std::map<PositionKey,float>limits;
    for(auto&entry:points){storm_geometry_work::checkpoint();auto&point=entry.second;auto outward=storm_ship_hull::normalized(point.normal);
        // Lighting normals are not a solid offset field. Project the geometric
        // average into the incident face cone so every offset moves inward.
        for(unsigned iteration=0;iteration<64;++iteration){bool valid=true;for(const auto&normal:point.faces){storm_geometry_work::checkpoint();const float missing=-storm_ship_hull::dot(outward,normal);if(missing>0){outward=outward+normal*missing;valid=false;}}outward=storm_ship_hull::normalized(outward);if(valid)break;}
        if(storm_ship_hull::length(outward)<1.e-6f)return {};
        for(const auto&normal:point.faces){storm_geometry_work::checkpoint();if(storm_ship_hull::dot(outward,normal)<-1.e-5f)return {};}
        offsetNormals.emplace(entry.first,outward);const auto direction=outward*-1.f;
        const float epsilon=std::max(.0001f,2*std::numeric_limits<float>::epsilon()*std::max({std::abs(point.position.x),std::abs(point.position.y),std::abs(point.position.z),1.f}));
        const auto start=point.position+direction*epsilon,end=point.position+direction*(maximum*4);
        broadphase.queryRay(start,end,candidates);
        for(auto index:candidates){storm_geometry_work::checkpoint();const auto&t=*triangles[index];bool incident=false;for(const auto&v:t)incident|=storm_ship_laser::positionKey(v.position)==entry.first;if(incident)continue;
            float a=0,b=0;const float hit=storm_ship_laser::trace(t,start,end,a,b);if(hit<=1)point.depth=std::min(point.depth,(epsilon+hit*(maximum*4-epsilon))*.25f);}
        const Vec3 reach{maximum*4,maximum*4,maximum*4};broadphase.queryBounds({point.position-reach,point.position+reach},candidates);
        const auto precise=[](Vec3 p){return storm_airburst_hull::Point{p.x,p.y,p.z};};
        for(auto index:candidates){storm_geometry_work::checkpoint();const auto&t=*triangles[index];bool incident=false;for(const auto&v:t)incident|=storm_ship_laser::positionKey(v.position)==entry.first;if(incident)continue;
            const auto p=precise(point.position),q=storm_airburst_hull::closestTriangle(p,precise(t[0].position),precise(t[1].position),precise(t[2].position));
            const double distance=std::sqrt(storm_airburst_hull::dot(p-q,p-q));point.depth=std::min(point.depth,float(distance*.25));}
        limits.emplace(entry.first,point.depth);
    }return limits;
}
inline std::vector<Plane> spherePlanes(const Cut&c) {
    if(c.kind==Cut::Beam){const auto planes=storm_ship_laser::beamPlanes(c.beams.front());return {planes.begin(),planes.end()};}
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
    for(const auto&o:source)for(const auto&t:o.second){storm_geometry_work::checkpoint();double d[3]={side(t[0].position),side(t[1].position),side(t[2].position)};
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
    storm_ship_hull::Bounds beamBounds{};
    if(cut.kind==Cut::Beam){const auto&beam=cut.beams.front();const auto a=storm_ship_laser::beamStart(beam),z=a+storm_ship_laser::beamDirection(beam)*beam.depth;
        const float r=beam.radius*(1.083f+.15625f);beamBounds={{std::min(a.x,z.x)-r,std::min(a.y,z.y)-r,std::min(a.z,z.z)-r},{std::max(a.x,z.x)+r,std::max(a.y,z.y)+r,std::max(a.z,z.z)+r}};}
    for(const auto&o:source){auto&destination=out[o.first];for(const auto&t:o.second){storm_geometry_work::checkpoint();
        // The octagon lies inside this circumradius. Reject remote triangles
        // before evaluating its ten planes; copied triangles remain exact.
        if(cut.kind==Cut::Beam){bool remote=false;for(unsigned axis=0;axis<3;++axis){const auto coord=[&](Vec3 p){return axis==0?p.x:axis==1?p.y:p.z;};
            const float lo=coord(beamBounds.lo),hi=coord(beamBounds.hi);remote|=(coord(t[0].position)<lo&&coord(t[1].position)<lo&&coord(t[2].position)<lo)||(coord(t[0].position)>hi&&coord(t[1].position)>hi&&coord(t[2].position)>hi);}
            if(remote){destination.push_back(t);continue;}}
        // Convex subtraction preserves untouched triangles byte-for-byte.
        bool far=false;for(const auto&p:planes){bool outside=true;for(const auto&v:t)outside&=storm_ship_hull::dot(p.normal,v.position)>p.distance;if(outside){far=true;break;}}
        if(far){destination.push_back(t);continue;}
        std::vector<Vertex>remaining(t.begin(),t.end()),inside,outside;
        for(const auto&p:planes){storm_geometry_work::checkpoint();storm_ship_laser::linearSplit(remaining,p,inside,outside);storm_ship_hull::fan(outside,destination);remaining.swap(inside);if(remaining.size()<3)break;}
        if(triangleCount(out)>MaxTriangles)return false;}}
    return finite(out);
}
inline bool retainHalfspaceSurface(const Mesh&source,const Plane&plane,Mesh&out) {
    for(const auto&o:source){auto&destination=out[o.first];for(const auto&t:o.second){storm_geometry_work::checkpoint();
        bool inside=true;for(const auto&v:t)inside&=storm_ship_hull::dot(plane.normal,v.position)<=plane.distance;
        if(inside){destination.push_back(t);continue;}
        std::vector<Vertex>a,b;storm_ship_laser::linearSplit(std::vector<Vertex>(t.begin(),t.end()),plane,a,b);storm_ship_hull::fan(a,destination);
    }}return finite(out);
}
inline bool applySurfaceCuts(Mesh source,const std::vector<Cut>&cuts,size_t exclude,Mesh&out,const Plane*boundary=nullptr) {
    for(size_t n=0;n<cuts.size();++n){storm_geometry_work::checkpoint();if(n==exclude)continue;Mesh next;const auto&c=cuts[n];
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
    for(const auto&cut:cuts){storm_geometry_work::checkpoint();bool inside=true;const auto planes=cut.kind!=Cut::Halfspace?spherePlanes(cut):std::vector<Plane>{{cut.normal,cut.distance}};
        for(const auto&o:source)for(const auto&t:o.second)for(const auto&v:t)for(const auto&p:planes){
            storm_geometry_work::checkpoint();
            const double d=double(p.normal.x)*v.position.x+double(p.normal.y)*v.position.y+double(p.normal.z)*v.position.z-p.distance;
            inside&=cut.kind!=Cut::Halfspace?d<=-shellMargin:d>=shellMargin;}
        if(inside)return true;}
    return false;
}
inline bool prismSection(const WorldPrism&prism,const Plane&plane,const std::vector<Plane>&limits,
    std::vector<PlanarRing>&sections,const storm_ship_hull::Frame&basis,Vec3 origin) {
    storm_geometry_work::checkpoint();
    // A consistent three-tetrahedron subdivision also supports varying inward
    // depths without replacing a non-convex swept triangle by its convex hull.
    constexpr unsigned tetra[3][4]={{0,1,2,5},{0,1,4,5},{0,3,4,5}};
    // Linear limits bound the whole swept prism, including all tetrahedra.
    // A tiny bore must not allocate/clip every remote triangle in a fort.
    for(const auto&limit:limits){bool outside=true;for(const auto&p:prism.vertices)outside&=double(limit.normal.x)*p.x+double(limit.normal.y)*p.y+double(limit.normal.z)*p.z>limit.distance;if(outside)return true;}
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
    for(const auto&t:source){storm_geometry_work::checkpoint();const double area=storm_ship_hull::area(t);if(area<=1.e-10)continue;
        const double uv=std::abs((double(t[1].uv[0])-t[0].uv[0])*(double(t[2].uv[1])-t[0].uv[1])-(double(t[1].uv[1])-t[0].uv[1])*(double(t[2].uv[0])-t[0].uv[0]));
        const double density=std::sqrt(uv/(2*area));if(std::isfinite(density)&&density>0)densities.push_back(float(density));
        weight+=area;for(const auto&v:t)for(unsigned c=0;c<4;++c)color[c]+=double((v.color>>(c*8))&255u)*area/3;
    }
    if(!densities.empty()){auto middle=densities.begin()+densities.size()/2;std::nth_element(densities.begin(),middle,densities.end(),[](float a,float b){storm_geometry_work::checkpoint();return a<b;});result.density=*middle;}
    if(weight>0){result.color=0;for(unsigned c=0;c<4;++c)result.color|=uint32_t(std::clamp(std::lround(color[c]/weight),0l,255l))<<(c*8);}
    return result;
}
inline bool triangulatePlanarAttempt(const std::vector<PlanarPolygon>&polygons,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance,bool integer) {
    const auto basis=storm_ship_hull::frame(plane.normal);Mesh result;
    for(const auto&polygon:polygons){storm_geometry_work::checkpoint();std::vector<PlanarPoint>points;double expected=0;
        for(size_t r=0;r<polygon.size();++r){storm_geometry_work::checkpoint();const auto&ring=polygon[r];if(ring.size()<3)return false;double area=0;for(size_t n=0;n<ring.size();++n){storm_geometry_work::checkpoint();const auto&a=ring[n],&b=ring[(n+1)%ring.size()];area+=a[0]*b[1]-b[0]*a[1];}expected+=(r?-1:1)*std::abs(area)*.5;points.insert(points.end(),ring.begin(),ring.end());}
        storm_geometry_work::checkpoint(true);std::vector<uint32_t>indices;if(integer){if(!integerTriangulation(polygon,indices))return false;}else indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);double measured=0;
        using Edge=std::pair<PlanarPoint,PlanarPoint>;std::map<Edge,unsigned>incidence;
        for(size_t n=0;n+2<indices.size();n+=3){storm_geometry_work::checkpoint();Triangle t{};double area=0;const auto&a=points[indices[n]],&b=points[indices[n+1]],&c=points[indices[n+2]];
            area=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);measured+=std::abs(area)*.5;if(std::abs(area)<=1.e-12)continue;
            for(unsigned e=0;e<3;++e){auto x=points[indices[n+e]],y=points[indices[n+(e+1)%3]];if(y<x)std::swap(x,y);if(++incidence[{x,y}]>2)return false;}
            for(unsigned v=0;v<3;++v){const auto&p=points[indices[n+v]];auto&vertex=t[v];vertex.position={float(origin.x+p[0]*basis.tangent.x+p[1]*basis.bitangent.x),float(origin.y+p[0]*basis.tangent.y+p[1]*basis.bitangent.y),float(origin.z+p[0]*basis.tangent.z+p[1]*basis.bitangent.z)};vertex.normal=plane.normal;vertex.color=appearance.color;
                // New solid faces have no authored island-mask atlas. Their
                // separate base-only draw uses the source texture's density.
                vertex.uv[0]=float(p[0]*appearance.density)+appearance.anchor[0];vertex.uv[1]=float(p[1]*appearance.density)+appearance.anchor[1];}
            if(area<0)std::swap(t[1],t[2]);result[object].push_back(t);}
        if(expected<0||std::abs(measured-expected)>std::max(1.e-7,expected*1.e-6))return false;
        std::vector<Edge>wanted,actual;for(const auto&ring:polygon)for(size_t e=0;e<ring.size();++e){storm_geometry_work::checkpoint();wanted.emplace_back(ring[e],ring[(e+1)%ring.size()]);}
        for(const auto&e:incidence){storm_geometry_work::checkpoint();if(e.second==1)actual.push_back(e.first);}
        // Earcut may drop collinear vertices. Compare covered segments, not
        // index spelling, and reject either missing or invented boundary edges.
        // The pinned integer triangulator may repair a rounding intersection
        // by at most one grid unit (RemoveIntersection). Check that exact
        // contract; sub-grid double collinearity is a different predicate.
        const auto covered=[integer](const Edge&edge,const std::vector<Edge>&by){
            storm_geometry_work::checkpoint();
            if(!integer){const double dx=edge.second[0]-edge.first[0],dy=edge.second[1]-edge.first[1],length=std::hypot(dx,dy);if(length<=1.e-7)return true;
                std::vector<std::pair<double,double>>intervals;for(const auto&candidate:by){storm_geometry_work::checkpoint();double t[2];bool linear=true;unsigned n=0;
                    for(const auto&p:{candidate.first,candidate.second}){linear&=std::abs(dx*(p[1]-edge.first[1])-dy*(p[0]-edge.first[0]))<=length*1.e-7;t[n++]=((p[0]-edge.first[0])*dx+(p[1]-edge.first[1])*dy)/(length*length);}
                    if(linear)intervals.emplace_back(std::min(t[0],t[1]),std::max(t[0],t[1]));}
                std::sort(intervals.begin(),intervals.end(),[](const auto&a,const auto&b){storm_geometry_work::checkpoint();return a<b;});double end=0;for(const auto&i:intervals){storm_geometry_work::checkpoint();if(i.first>end+1.e-7/length)break;end=std::max(end,i.second);if(end>=1-1.e-7/length)return true;}return false;}
            const auto key=[](const PlanarPoint&p){return std::array<int64_t,2>{std::llround(p[0]*PlanarUnitsPerMeter),std::llround(p[1]*PlanarUnitsPerMeter)};};
            const auto a=key(edge.first),b=key(edge.second);const int64_t dx=b[0]-a[0],dy=b[1]-a[1];if(!dx&&!dy)return true;
            const __int128 squared=__int128(dx)*dx+__int128(dy)*dy;const int64_t reach=std::abs(dx)+std::abs(dy);const long double length=static_cast<long double>(squared),unit=std::sqrt(length);
            std::vector<std::pair<long double,long double>>intervals;
            const auto add=[&](long double lo,long double hi){lo=std::max(lo,0.L);hi=std::min(hi,length);if(lo<=hi)intervals.emplace_back(lo,hi);};
            for(const auto&candidate:by){storm_geometry_work::checkpoint();const auto x=key(candidate.first),y=key(candidate.second);
                const __int128 px=__int128(x[0]-a[0])*dx+__int128(x[1]-a[1])*dy,py=__int128(y[0]-a[0])*dx+__int128(y[1]-a[1])*dy;
                const __int128 cx=__int128(dx)*(x[1]-a[1])-__int128(dy)*(x[0]-a[0]),cy=__int128(dx)*(y[1]-a[1])-__int128(dy)*(y[0]-a[0]);
                // Clip the finite candidate to the one-unit strip before
                // testing coverage; its distant endpoints are irrelevant.
                if(cx==cy){const auto cross=cx<0?-cx:cx;if(cross<=reach&&cross*cross<=squared)add(std::min(px,py),std::max(px,py));}
                else {const long double cross=static_cast<long double>(cx),step=static_cast<long double>(cy-cx);long double lo=(-unit-cross)/step,hi=(unit-cross)/step;if(hi<lo)std::swap(lo,hi);lo=std::max(lo,0.L);hi=std::min(hi,1.L);
                    if(lo<=hi){const long double start=static_cast<long double>(px),delta=static_cast<long double>(py-px);const auto p=start+delta*lo,q=start+delta*hi;add(std::min(p,q),std::max(p,q));}}
                for(const auto&endpoint:{std::make_pair(px,cx),std::make_pair(py,cy)}){const auto cross=endpoint.second<0?-endpoint.second:endpoint.second;
                    if(cross>reach||cross*cross>squared)continue;const long double delta=std::sqrt(static_cast<long double>(squared-cross*cross)),center=static_cast<long double>(endpoint.first);add(center-delta,center+delta);}}
            std::sort(intervals.begin(),intervals.end(),[](const auto&a,const auto&b){storm_geometry_work::checkpoint();return a<b;});long double end=0;for(const auto&i:intervals){storm_geometry_work::checkpoint();if(i.first>end)break;end=std::max(end,i.second);if(end>=length)return true;}return false;};
        for(const auto&e:wanted){storm_geometry_work::checkpoint();if(!covered(e,actual))return false;}for(const auto&e:actual){storm_geometry_work::checkpoint();if(!covered(e,wanted))return false;}
        if(triangleCount(cap)+triangleCount(result)>MaxTriangles)return false;
    }if(!finite(result))return false;storm_ship_laser::appendMesh(cap,result);return true;
}
inline bool triangulatePlanar(const std::vector<PlanarPolygon>&polygons,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance,bool integerFallback=false) {
    return triangulatePlanarAttempt(polygons,plane,object,origin,cap,appearance,false)||(integerFallback&&triangulatePlanarAttempt(polygons,plane,object,origin,cap,appearance,true));
}
inline bool planarCap(const std::vector<PlanarRing>&sections,const Plane&plane,int object,Vec3 origin,Mesh&cap,const CapAppearance&appearance) {
    if(sections.empty())return true;storm_geometry_work::checkpoint(true);std::vector<PlanarPolygon>polygons;if(!unitePlanar(sections,polygons))return false;storm_geometry_work::checkpoint(true);
    return triangulatePlanar(polygons,plane,object,origin,cap,appearance);
}
struct CapPatch {
    uint32_t field=0,face=0;
    Plane plane{};Vec3 origin{};int object=0;bool reverse=false;
    CapAppearance appearance;Mesh surface;
};
inline bool reconstructWorld(const Mesh&original,const std::set<int>&terrainObjects,const std::vector<Cut>&cuts,Mesh&out,Mesh*outerOutput=nullptr,Mesh*capOutput=nullptr,std::vector<CapPatch>*patchesOutput=nullptr) {
    storm_geometry_work::checkpoint(true);
    if(cuts.size()>MaxCuts||!finite(original))return false;for(const auto&c:cuts)if(!valid(c))return false;
    std::vector<Cut>unique;for(const auto&record:cuts){storm_geometry_work::checkpoint();std::vector<Cut>expanded;
        if(record.kind==Cut::Beam){for(size_t n=0;n<record.beams.size();++n){storm_geometry_work::checkpoint();auto part=record;part.beams={record.beams[n]};part.faceOffset=uint32_t(n)*10;expanded.push_back(std::move(part));}}
        else expanded.push_back(record);
        for(const auto&c:expanded){storm_geometry_work::checkpoint();bool duplicate=false;for(const auto&p:unique){storm_geometry_work::checkpoint();duplicate|=equal(c,p);}if(!duplicate)unique.push_back(c);}}
    Mesh surface,stone,closed;std::vector<WorldPrism>prisms;std::map<int,CapAppearance>appearances;
    for(const auto&o:original){storm_geometry_work::checkpoint();appearances.emplace(o.first,capAppearance(o.second));}
    const float floor=storm_ship_laser::bounds(original).lo.y-10032.f;
    const auto addPrism=[&](const Triangle&t,const std::array<Vec3,3>&inner,int object){std::array<unsigned,3>order{0,1,2};std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return storm_ship_laser::positionKey(t[a].position)<storm_ship_laser::positionKey(t[b].position);});
        WorldPrism prism;prism.object=object;for(unsigned n=0;n<3;++n){prism.vertices[n]=t[order[n]].position;prism.vertices[n+3]=inner[order[n]];}prisms.push_back(prism);};
    for(const auto&o:original){storm_geometry_work::checkpoint();surface.try_emplace(o.first);if(!terrainObjects.count(o.first)){stone[o.first]=o.second;continue;}
        surface[o.first]=o.second;for(const auto&t:o.second){storm_geometry_work::checkpoint();auto box=storm_ship_hull::Bounds{t[0].position,t[0].position};for(const auto&v:t)box=storm_ship_hull::merge(box,{v.position,v.position});box.lo.y=floor;
            if(!affects(box,unique))continue;std::array<Vec3,3>inner;for(unsigned n=0;n<3;++n){inner[n]=t[n].position;inner[n].y=floor;}addPrism(t,inner,o.first);}}
    for(const auto&component:components(stone)){storm_geometry_work::checkpoint();auto box=storm_ship_laser::bounds(component);box.lo=box.lo-Vec3{1,1,1};box.hi=box.hi+Vec3{1,1,1};
        if(!affects(box,unique)){storm_ship_laser::appendMesh(surface,component);continue;}
        if(removedComponent(component,unique,1.f))continue;
        std::vector<Segment>boundary;if(!boundaries(component,boundary))return false;
        if(boundary.empty()){storm_ship_laser::appendMesh(surface,component);storm_ship_laser::appendMesh(closed,component);continue;}
        std::map<PositionKey,Vec3>normals;const auto depths=stoneClearance(component,1,normals);if(depths.empty())return false;
        std::map<int,float>thickness;for(const auto&o:component){storm_geometry_work::checkpoint();thickness[o.first]=1;}
        auto shell=storm_ship_laser::shellForRender(component,thickness,&depths,&normals);storm_ship_laser::appendMesh(surface,shell);
        for(const auto&o:component)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::array<Vec3,3>inner;for(unsigned n=0;n<3;++n){const auto key=storm_ship_laser::positionKey(t[n].position);inner[n]=t[n].position-normals.at(key)*depths.at(key);}addPrism(t,inner,o.first);}
        if(triangleCount(surface)>MaxTriangles)return false;
    }
    Mesh outer,allCaps;std::vector<CapPatch>patches;if(!applySurfaceCuts(surface,unique,unique.size(),outer))return false;
    for(size_t n=0;n<unique.size();++n){storm_geometry_work::checkpoint();const auto&c=unique[n];const auto planes=c.kind!=Cut::Halfspace?spherePlanes(c):std::vector<Plane>{{c.normal,c.distance}};
        for(size_t side=0;side<planes.size();++side){storm_geometry_work::checkpoint();const auto&plane=planes[side];const auto basis=storm_ship_hull::frame(plane.normal);const Vec3 origin=plane.normal*plane.distance;
            std::vector<Plane>limits;if(c.kind!=Cut::Halfspace)for(size_t p=0;p<planes.size();++p)if(p!=side)limits.push_back(planes[p]);
            std::vector<PlanarRing>sections;int object=original.empty()?0:original.begin()->first;
            for(const auto&prism:prisms){storm_geometry_work::checkpoint();const auto before=sections.size();if(!prismSection(prism,plane,limits,sections,basis,origin))return false;if(sections.size()!=before)object=prism.object;}
            // Closed authored components use their own oriented cross-section,
            // preserving cavities/windows rather than filling a convex hull.
            if(triangleCount(closed)){Mesh cap;if(!crossSection(closed,plane,cap))return false;
                for(const auto&limit:limits){storm_geometry_work::checkpoint();Mesh clipped;if(!retainHalfspaceSurface(cap,limit,clipped))return false;cap=std::move(clipped);}
                for(const auto&o:cap)for(const auto&t:o.second){storm_geometry_work::checkpoint();PlanarRing ring;for(const auto&v:t){auto p=v.position-origin;ring.push_back({storm_ship_hull::dot(p,basis.tangent),storm_ship_hull::dot(p,basis.bitangent)});}double area=0;for(unsigned j=0;j<3;++j)area+=ring[j][0]*ring[(j+1)%3][1]-ring[(j+1)%3][0]*ring[j][1];if(area<0)std::reverse(ring.begin(),ring.end());sections.push_back(std::move(ring));object=o.first;}}
            if(sections.empty())continue;Mesh cap;if(!planarCap(sections,plane,object,origin,cap,appearances.at(object)))return false;
            if(c.kind!=Cut::Halfspace)for(auto&o:cap)for(auto&t:o.second){storm_geometry_work::checkpoint();std::swap(t[1],t[2]);for(auto&v:t)v.normal=plane.normal*-1.f;}
            const Plane boundary=c.kind!=Cut::Halfspace?Plane{plane.normal*-1.f,-plane.distance}:plane;Mesh retained;
            if(!applySurfaceCuts(std::move(cap),unique,n,retained,&boundary))return false;
            if(patchesOutput&&triangleCount(retained)){CapPatch patch;patch.field=c.field;patch.face=c.faceOffset+uint32_t(side);patch.plane=plane;patch.origin=origin;patch.object=object;patch.reverse=c.kind!=Cut::Halfspace;patch.appearance=appearances.at(object);patch.surface=retained;patches.push_back(std::move(patch));}
            storm_ship_laser::appendMesh(allCaps,retained);if(triangleCount(outer)+triangleCount(allCaps)>MaxTriangles)return false;
        }}
    for(const auto&o:original){storm_geometry_work::checkpoint();outer.try_emplace(o.first);}Mesh result=outer;storm_ship_laser::appendMesh(result,allCaps);if(!finite(result))return false;
    out=std::move(result);if(outerOutput)*outerOutput=std::move(outer);if(capOutput)*capOutput=std::move(allCaps);if(patchesOutput)*patchesOutput=std::move(patches);return true;
}
inline bool resolveCapCoverage(const std::vector<CapPatch>&patches,const std::vector<CapTriangle>&earlier,Mesh&capOutput,std::vector<CapTriangle>&acceptedOutput) {
    storm_geometry_work::checkpoint(true);
    if(earlier.size()>MaxTriangles)return false;
    using Key=std::pair<uint32_t,uint32_t>;std::map<Key,std::vector<const CapTriangle*>>byKey;
    for(const auto&t:earlier){storm_geometry_work::checkpoint();for(const auto&p:t.positions)for(float v:p)if(!std::isfinite(v)||std::abs(v)>10000000)return false;byKey[{t.field,t.face}].push_back(&t);}
    Mesh caps;std::vector<CapTriangle>accepted;
    for(const auto&patch:patches){storm_geometry_work::checkpoint();Mesh retained;const auto found=byKey.find({patch.field,patch.face});
        if(found==byKey.end())retained=patch.surface;
        else {const auto basis=storm_ship_hull::frame(patch.plane.normal);std::vector<PlanarRing>subject,clip;
            const auto project=[&](Vec3 p){const double x=double(p.x)-patch.origin.x,y=double(p.y)-patch.origin.y,z=double(p.z)-patch.origin.z;
                return PlanarPoint{x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z};};
            const auto positive=[](PlanarRing&ring){double area=0;for(unsigned n=0;n<3;++n)area+=ring[n][0]*ring[(n+1)%3][1]-ring[(n+1)%3][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());};
            for(const auto&o:patch.surface)for(const auto&t:o.second){storm_geometry_work::checkpoint();PlanarRing ring;for(const auto&v:t)ring.push_back(project(v.position));positive(ring);subject.push_back(std::move(ring));}
            for(const auto*t:found->second){storm_geometry_work::checkpoint();PlanarRing ring;for(const auto&p:t->positions){const Vec3 point{p[0],p[1],p[2]};
                    const double tolerance=std::max(1.e-4,8.*std::numeric_limits<float>::epsilon()*std::max({std::abs(double(point.x)),std::abs(double(point.y)),std::abs(double(point.z)),std::abs(double(patch.plane.distance)),1.}));
                    const double distance=double(patch.plane.normal.x)*point.x+double(patch.plane.normal.y)*point.y+double(patch.plane.normal.z)*point.z-patch.plane.distance;
                    if(std::abs(distance)>tolerance)return false;ring.push_back(project(point));}positive(ring);clip.push_back(std::move(ring));}
            storm_geometry_work::checkpoint(true);std::vector<PlanarPolygon>polygons;if(!differencePlanar(subject,clip,polygons))return false;storm_geometry_work::checkpoint(true);
            if(!triangulatePlanar(polygons,patch.plane,patch.object,patch.origin,retained,patch.appearance,true))return false;
            if(patch.reverse)for(auto&o:retained)for(auto&t:o.second){storm_geometry_work::checkpoint();std::swap(t[1],t[2]);for(auto&v:t)v.normal=patch.plane.normal*-1.f;}
        }
        for(const auto&o:retained)for(const auto&t:o.second){storm_geometry_work::checkpoint();CapTriangle record;record.field=patch.field;record.face=patch.face;
            for(unsigned n=0;n<3;++n)record.positions[n]={t[n].position.x,t[n].position.y,t[n].position.z};accepted.push_back(record);}
        storm_ship_laser::appendMesh(caps,retained);if(accepted.size()>MaxTriangles)return false;
    }
    if(!finite(caps))return false;capOutput=std::move(caps);acceptedOutput=std::move(accepted);return true;
}
// Immutable world cells use the existing finite-solid Boolean and SAVE codec.
// Only touched cells materialize a mesh; terrain columns keep six vertices.
struct Cell {
    WorldPrism prism{};Triangle appearance{};
    std::shared_ptr<const Mesh> closed;
};
struct Source {
    Mesh outer;
    std::vector<Cell> cells;
    std::vector<storm_ship_hull::Bounds> boxes;
    storm_ship_hull::Broadphase index;
    std::map<int,CapAppearance> appearances;
    uint64_t fingerprint=0;
};
struct RegionSource {
    std::vector<Cell> cells;
    std::vector<storm_ship_hull::Bounds> boxes;
    storm_ship_hull::Broadphase index;
    Mesh shell;
};
struct Region {
    std::shared_ptr<const RegionSource> source;
    std::map<uint32_t,std::shared_ptr<const Mesh>> changed;
};
struct Material {
    std::shared_ptr<const Source> source;
    std::map<uint32_t,std::shared_ptr<const Region>> changed;
    Mesh outer;
    std::vector<CapPatch> caps;
};
inline Mesh cellMesh(const Cell&cell) {
    if(cell.closed)return *cell.closed;
    Mesh result;const auto&prism=cell.prism;
    const Vec3 center=(prism.vertices[0]+prism.vertices[1]+prism.vertices[2]+prism.vertices[3]+prism.vertices[4]+prism.vertices[5])*(1.f/6.f);
    constexpr unsigned faces[8][3]={{0,1,2},{3,5,4},{0,3,4},{0,4,1},{1,4,5},{1,5,2},{2,5,0},{0,5,3}};
    for(const auto&face:faces){Triangle t;for(unsigned n=0;n<3;++n){t[n]=cell.appearance[face[n]%3];t[n].position=prism.vertices[face[n]];}
        if(storm_ship_hull::dot(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position),center-t[0].position)>0)std::swap(t[1],t[2]);
        surfaceNormal(t);if(storm_ship_hull::area(t)>1.e-10f)result[prism.object].push_back(t);}
    return result;
}
inline Cell prismCell(const Triangle&t,const std::array<Vec3,3>&inner,int object) {
    Cell cell;cell.prism.object=object;cell.appearance=t;
    std::array<unsigned,3>order{0,1,2};std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return storm_ship_laser::positionKey(t[a].position)<storm_ship_laser::positionKey(t[b].position);});
    for(unsigned n=0;n<3;++n){cell.prism.vertices[n]=t[order[n]].position;cell.prism.vertices[n+3]=inner[order[n]];}return cell;
}
inline storm_ship_hull::Bounds cellBounds(const Cell&cell) {
    if(cell.closed)return storm_ship_laser::bounds(*cell.closed);
    auto box=storm_ship_hull::Bounds{cell.prism.vertices[0],cell.prism.vertices[0]};
    for(const auto&p:cell.prism.vertices)box=storm_ship_hull::merge(box,{p,p});return box;
}
// A descriptor is only classified/solidified after a cutter reaches it. Remote
// open or nonmanifold architecture cannot reject a terrain cut or gain shells.
inline std::shared_ptr<const RegionSource> activate(const Cell&cell) {
    auto region=std::make_shared<RegionSource>();
    if(!cell.closed)region->cells.push_back(cell);
    else {const auto&component=*cell.closed;std::vector<Segment>boundary;if(!boundaries(component,boundary))return {};
        if(boundary.empty())region->cells.push_back(cell);
        else {std::map<PositionKey,Vec3>normals;const auto depths=stoneClearance(component,1,normals);if(depths.empty())return {};
            std::map<int,float>thickness;for(const auto&o:component)thickness[o.first]=1;
            region->shell=storm_ship_laser::shellForRender(component,thickness,&depths,&normals);if(!finite(region->shell))return {};
            for(const auto&o:component)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::array<Vec3,3>inner;
                for(unsigned n=0;n<3;++n){const auto key=storm_ship_laser::positionKey(t[n].position);inner[n]=t[n].position-normals.at(key)*depths.at(key);}region->cells.push_back(prismCell(t,inner,o.first));}}}
    for(const auto&part:region->cells)region->boxes.push_back(cellBounds(part));region->index.build(region->boxes);return region;
}
inline void activateOuter(Material&state,const Cell&descriptor,const RegionSource&region) {
    if(region.shell.empty())return;
    using Key=std::array<std::array<uint32_t,15>,3>;
    for(const auto&o:*descriptor.closed){std::set<Key>authored;
        for(const auto&t:o.second)authored.insert({storm::ship::material::vertexWords(t[0]),storm::ship::material::vertexWords(t[1]),storm::ship::material::vertexWords(t[2])});
        auto&triangles=state.outer[o.first];triangles.erase(std::remove_if(triangles.begin(),triangles.end(),[&](const Triangle&t){storm_geometry_work::checkpoint();
            return authored.count({storm::ship::material::vertexWords(t[0]),storm::ship::material::vertexWords(t[1]),storm::ship::material::vertexWords(t[2])})!=0;}),triangles.end());}
    storm_ship_laser::appendMesh(state.outer,region.shell);
}
inline std::shared_ptr<const Source> materialSource(const Mesh&original,const std::set<int>&terrain) {
    if(!finite(original))return {};auto source=std::make_shared<Source>();source->outer=original;Mesh stone;
    source->fingerprint=storm::ship::material::fingerprint(original,{});
    for(int object:terrain)source->fingerprint=storm::ship::material::hashBytes(&object,sizeof(object),source->fingerprint);
    const float floor=storm_ship_laser::bounds(original).lo.y-10032.f;
    const auto add=[&](Cell cell,storm_ship_hull::Bounds box){source->cells.push_back(std::move(cell));source->boxes.push_back(box);};
    for(const auto&o:original){storm_geometry_work::checkpoint();source->appearances.emplace(o.first,capAppearance(o.second));
        if(!terrain.count(o.first)){stone[o.first]=o.second;continue;}
        for(const auto&t:o.second){storm_geometry_work::checkpoint();std::array<Vec3,3>inner;for(unsigned n=0;n<3;++n){inner[n]=t[n].position;inner[n].y=floor;}
            auto cell=prismCell(t,inner,o.first);const auto box=cellBounds(cell);add(std::move(cell),box);}}
    for(const auto&component:components(stone)){storm_geometry_work::checkpoint();Cell cell;cell.closed=std::make_shared<const Mesh>(component);
        auto box=storm_ship_laser::bounds(component);box.lo=box.lo-Vec3{1,1,1};box.hi=box.hi+Vec3{1,1,1};add(std::move(cell),box);}
    if(source->cells.size()>MaxTriangles)return {};
    source->index.build(source->boxes);return source;
}
inline void affectedCells(const std::vector<storm_ship_hull::Bounds>&boxes,const storm_ship_hull::Broadphase&index,const Cut&cut,std::vector<uint32_t>&ids) {
    if(cut.kind==Cut::Beam)index.queryBounds(storm_ship_laser::beamBounds(cut.beams.front()),ids);
    else if(cut.kind==Cut::Sphere){const Vec3 r{cut.radius*1.733f,cut.radius*1.733f,cut.radius*1.733f};index.queryBounds({cut.center-r,cut.center+r},ids);}
    else for(uint32_t n=0;n<boxes.size();++n)if(affects(boxes[n],{cut}))ids.push_back(n);
}
inline bool updateMaterial(Material&state,const std::vector<Cut>&cuts,const char**failure=nullptr) {
    for(const auto&record:cuts){std::vector<Cut>parts;
        if(record.kind==Cut::Beam)for(size_t n=0;n<record.beams.size();++n){auto part=record;part.beams={record.beams[n]};part.faceOffset=uint32_t(n)*10;parts.push_back(std::move(part));}
        else parts.push_back(record);
        for(const auto&cut:parts){storm_geometry_work::checkpoint(true);std::vector<uint32_t>ids;
            // Fixed authored BVH: a cut only visits intersecting material cells.
            affectedCells(state.source->boxes,state.source->index,cut,ids);
            const auto planes=cut.kind==Cut::Halfspace?std::vector<Plane>{{cut.normal*-1.f,-cut.distance}}:spherePlanes(cut);
            std::vector<std::vector<PlanarRing>>sections(planes.size());std::vector<int>objects(planes.size(),0);
            for(auto id:ids){storm_geometry_work::checkpoint();const auto found=state.changed.find(id);Region region;
                if(found!=state.changed.end())region=*found->second;
                else {if(failure)*failure="touched architecture solidification";region.source=activate(state.source->cells[id]);if(!region.source)return false;activateOuter(state,state.source->cells[id],*region.source);}
                std::vector<uint32_t>parts;affectedCells(region.source->boxes,region.source->index,cut,parts);
                for(auto part:parts){const auto previous=region.changed.find(part);Mesh solid=previous==region.changed.end()?cellMesh(region.source->cells[part]):*previous->second;
                if(solid.empty())continue;
                for(size_t side=0;side<planes.size();++side){std::vector<Plane>limits;if(cut.kind!=Cut::Halfspace)for(size_t p=0;p<planes.size();++p)if(p!=side)limits.push_back(planes[p]);
                    // Cell boundaries can be internal to the world union. Do
                    // not discard a section merely because it meets that face.
                    if(failure)*failure="retained cell cross-section";Mesh inside;if(!crossSection(solid,planes[side],inside))return false;
                    for(const auto&limit:limits){Mesh clipped;if(!retainHalfspaceSurface(inside,limit,clipped))return false;inside=std::move(clipped);}
                    const auto basis=storm_ship_hull::frame(planes[side].normal);const auto origin=planes[side].normal*planes[side].distance;
                    for(const auto&o:inside)for(const auto&t:o.second){PlanarRing ring;for(const auto&v:t){const auto p=v.position-origin;ring.push_back({storm_ship_hull::dot(p,basis.tangent),storm_ship_hull::dot(p,basis.bitangent)});}
                        double area=0;for(unsigned n=0;n<3;++n)area+=ring[n][0]*ring[(n+1)%3][1]-ring[(n+1)%3][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());sections[side].push_back(std::move(ring));objects[side]=o.first;}}
                if(failure)*failure="retained cell subtraction";Mesh kept;if(!storm::ship::material::clipCell(solid,planes,kept)||!finite(kept))return false;
                region.changed[part]=std::make_shared<const Mesh>(std::move(kept));}
                state.changed[id]=std::make_shared<const Region>(std::move(region));}
            if(failure)*failure="visible outer subtraction";Mesh outer;if(!applySurfaceCuts(std::move(state.outer),{cut},1,outer))return false;state.outer=std::move(outer);
            for(auto&patch:state.caps){Mesh retained;const Plane boundary=patch.reverse?Plane{patch.plane.normal*-1.f,-patch.plane.distance}:patch.plane;
                // Put the existing cap first so coincident empty bores retain
                // their current wall instead of erasing it and regenerating it.
                Cut prior;prior.kind=Cut::Halfspace;prior.normal=boundary.normal;prior.distance=boundary.distance;
                if(failure)*failure="existing cavity wall subtraction";if(!applySurfaceCuts(std::move(patch.surface),{prior,cut},0,retained,&boundary))return false;patch.surface=std::move(retained);}
            state.caps.erase(std::remove_if(state.caps.begin(),state.caps.end(),[](const CapPatch&p){return !triangleCount(p.surface);}),state.caps.end());
            for(size_t side=0;side<planes.size();++side){if(sections[side].empty())continue;CapPatch patch;patch.field=cut.field;patch.face=cut.faceOffset+uint32_t(side);
                patch.plane=planes[side];patch.origin=patch.plane.normal*patch.plane.distance;patch.object=objects[side];patch.reverse=true;patch.appearance=state.source->appearances.at(patch.object);
                std::vector<PlanarRing>existing;const auto basis=storm_ship_hull::frame(patch.plane.normal);
                const auto mask=[&](const Mesh&mesh){for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();
                    const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
                    if(storm_ship_hull::dot(normal,patch.plane.normal)>-.999f)continue;bool coplanar=true;PlanarRing ring;
                    for(const auto&v:t){const auto p=v.position-patch.origin;coplanar&=std::abs(double(patch.plane.normal.x)*p.x+double(patch.plane.normal.y)*p.y+double(patch.plane.normal.z)*p.z)<=std::max(.0001,4.*std::numeric_limits<float>::epsilon()*std::max({std::abs(double(v.position.x)),std::abs(double(v.position.y)),std::abs(double(v.position.z)),1.}));
                        ring.push_back({storm_ship_hull::dot(p,basis.tangent),storm_ship_hull::dot(p,basis.bitangent)});}if(!coplanar)continue;
                    double area=0;for(unsigned n=0;n<3;++n)area+=ring[n][0]*ring[(n+1)%3][1]-ring[(n+1)%3][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());existing.push_back(std::move(ring));}};
                mask(state.outer);for(const auto&old:state.caps)mask(old.surface);
                if(failure)*failure="new cavity wall union/triangulation";std::vector<PlanarPolygon>polygons;if(!differencePlanar(sections[side],existing,polygons)||!triangulatePlanar(polygons,patch.plane,patch.object,patch.origin,patch.surface,patch.appearance,true))return false;
                for(auto&o:patch.surface)for(auto&t:o.second){std::swap(t[1],t[2]);for(auto&v:t)v.normal=patch.plane.normal*-1.f;}state.caps.push_back(std::move(patch));}
            size_t total=triangleCount(state.outer);for(const auto&patch:state.caps)total+=triangleCount(patch.surface);
            for(const auto&region:state.changed)for(const auto&cell:region.second->changed)total+=triangleCount(*cell.second);
            if(failure)*failure="retained geometry budget";if(total>MaxTriangles||state.caps.size()>MaxTriangles)return false;
        }}return true;
}
inline std::string encodeMaterial(const Material&state) {
    using storm::ship::material::Writer;Writer writer;writer.word(0x32574d43);writer.wide(state.source->fingerprint);writer.mesh(state.outer);
    writer.word(uint32_t(state.caps.size()));
    const auto scalar=[&](float value){uint32_t word;std::memcpy(&word,&value,4);writer.word(word);};
    for(const auto&patch:state.caps){writer.word(patch.field);writer.word(patch.face);writer.word(uint32_t(patch.object));
        for(float value:{patch.plane.normal.x,patch.plane.normal.y,patch.plane.normal.z,patch.plane.distance})scalar(value);writer.mesh(patch.surface);
        if(writer.bytes.size()>MaxCheckpointBytes/2)return {};}
    writer.word(uint32_t(state.changed.size()));for(const auto&region:state.changed){writer.word(region.first);writer.word(uint32_t(region.second->changed.size()));
        for(const auto&cell:region.second->changed){writer.word(cell.first);writer.mesh(*cell.second);if(writer.bytes.size()>MaxCheckpointBytes/2)return {};}}
    writer.wide(storm::ship::material::hashBytes(writer.bytes.data(),writer.bytes.size()));
    if(writer.bytes.size()>MaxCheckpointBytes/2)return {};constexpr char digits[]="0123456789abcdef";std::string result(writer.bytes.size()*2,'0');
    for(size_t n=0;n<writer.bytes.size();++n){storm_geometry_work::checkpoint();result[n*2]=digits[writer.bytes[n]>>4];result[n*2+1]=digits[writer.bytes[n]&15];}return result;
}
inline bool decodeMaterial(const std::string&payload,std::shared_ptr<const Source>source,Material&out) {
    if(payload.size()<40||payload.size()%2||payload.size()>MaxCheckpointBytes)return false;storm::ship::material::Reader reader;reader.bytes.reserve(payload.size()/2);
    const auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t n=0;n<payload.size();n+=2){storm_geometry_work::checkpoint();const auto a=digit(payload[n]),b=digit(payload[n+1]);if(a<0||b<0)return false;reader.bytes.push_back(uint8_t(a*16+b));}
    if(reader.word()!=0x32574d43||reader.wide()!=source->fingerprint)return false;Material next;next.source=std::move(source);
    if(!reader.mesh(next.outer)||!finite(next.outer)||next.outer.size()!=next.source->outer.size())return false;
    for(const auto&o:next.outer)if(!next.source->outer.count(o.first))return false;
    const auto scalar=[&](){const auto word=reader.word();float value;std::memcpy(&value,&word,4);return value;};
    const auto count=reader.word();if(count>MaxTriangles||uint64_t(count)*32>reader.bytes.size()-reader.at)return false;size_t total=triangleCount(next.outer);
    for(uint32_t n=0;n<count;++n){CapPatch patch;patch.field=reader.word();patch.face=reader.word();patch.object=int32_t(reader.word());
        patch.plane.normal={scalar(),scalar(),scalar()};patch.plane.distance=scalar();Cut plane;plane.normal=patch.plane.normal;plane.distance=patch.plane.distance;
        if(!valid(plane)||!next.source->appearances.count(patch.object)||!reader.mesh(patch.surface)||!finite(patch.surface))return false;
        patch.origin=patch.plane.normal*patch.plane.distance;patch.reverse=true;patch.appearance=next.source->appearances.at(patch.object);total+=triangleCount(patch.surface);next.caps.push_back(std::move(patch));}
    const auto changed=reader.word();if(changed>next.source->cells.size())return false;
    for(uint32_t n=0;n<changed;++n){const auto id=reader.word();if(id>=next.source->cells.size()||next.changed.count(id))return false;
        Region region;region.source=activate(next.source->cells[id]);if(!region.source)return false;const auto count=reader.word();if(count>region.source->cells.size())return false;
        for(uint32_t p=0;p<count;++p){const auto part=reader.word();Mesh mesh;if(part>=region.source->cells.size()||region.changed.count(part)||!reader.mesh(mesh)||!finite(mesh))return false;
            total+=triangleCount(mesh);region.changed[part]=std::make_shared<const Mesh>(std::move(mesh));}
        next.changed[id]=std::make_shared<const Region>(std::move(region));}
    const auto end=reader.at;const auto checksum=reader.wide();if(total>MaxTriangles||!reader.valid||reader.at!=reader.bytes.size()||checksum!=storm::ship::material::hashBytes(reader.bytes.data(),end))return false;
    out=std::move(next);return true;
}
inline bool contains(const Mesh&mesh,Vec3 point) {
    double angle=0;for(const auto&o:mesh)for(const auto&t:o.second){if(storm_ship_laser::pointOnFace(point,t))return true;
        const auto a=t[0].position-point,b=t[1].position-point,c=t[2].position-point;
        const double x=storm_ship_hull::length(a),y=storm_ship_hull::length(b),z=storm_ship_hull::length(c);
        angle+=2*std::atan2(double(storm_ship_hull::dot(a,storm_ship_hull::cross(b,c))),x*y*z+storm_ship_hull::dot(a,b)*z+storm_ship_hull::dot(b,c)*x+storm_ship_hull::dot(c,a)*y);}
    return std::abs(angle)>6.283185307179586;
}
inline bool containsCell(const Cell&cell,Vec3 point) {
    if(cell.closed)return contains(*cell.closed,point);
    // The same three tetrahedra as prismSection; no temporary mesh is built on
    // the grass/fort support-query path, including before a cell is cut.
    constexpr unsigned tetra[3][4]={{0,1,2,5},{0,1,4,5},{0,3,4,5}};
    for(const auto&indices:tetra){const auto&p=cell.prism.vertices;const auto a=p[indices[0]],b=p[indices[1]],c=p[indices[2]],d=p[indices[3]];
        if(std::abs(storm_ship_hull::dot(b-a,storm_ship_hull::cross(c-a,d-a)))<1.e-12f)continue;
        bool inside=true;
        for(unsigned side=0;side<4;++side){const auto x=p[indices[(side+1)%4]],y=p[indices[(side+2)%4]],z=p[indices[(side+3)%4]];
            const auto normal=storm_ship_hull::cross(y-x,z-x);const double reference=storm_ship_hull::dot(normal,p[indices[side]]-x),distance=storm_ship_hull::dot(normal,point-x);
            const double tolerance=1.e-5*storm_ship_hull::length(normal);inside&=reference>=0?distance>=-tolerance:distance<=tolerance;}
        if(inside)return true;}
    return false;
}
inline bool positionRetained(const Material&state,Vec3 point) {
    std::vector<uint32_t>ids;const Vec3 epsilon{.0001f,.0001f,.0001f};state.source->index.queryBounds({point-epsilon,point+epsilon},ids);
    bool removed=false;for(auto id:ids){const auto found=state.changed.find(id);if(found==state.changed.end())continue;
        const auto&region=*found->second;std::vector<uint32_t>parts;region.source->index.queryBounds({point-epsilon,point+epsilon},parts);
        for(auto part:parts){if(!containsCell(region.source->cells[part],point))continue;const auto cell=region.changed.find(part);
            if(cell==region.changed.end()||contains(*cell->second,point))return true;removed=true;}}
    // Overlapping untouched columns may still own this point.
    if(removed)for(auto id:ids)if(!state.changed.count(id)&&containsCell(state.source->cells[id],point))return true;
    return !removed;
}
// Immutable captured geometry is shared with CPU jobs. No engine entity,
// renderer, attribute tree or live geometry pointer crosses this boundary.
struct Input {
    std::shared_ptr<const Mesh> original;
    std::set<int> terrainObjects;
    std::vector<Cut> cuts;
    std::shared_ptr<const Material> material;
    std::string checkpoint;
    bool active=false;
};
struct Projection {
    Mesh retained,outer,caps;
    std::vector<CapPatch> rawCaps;
    std::vector<Cut> cuts;
    std::shared_ptr<const Material> material;
    std::string checkpoint;
    storm_ship_hull::Broadphase broadphase;
    std::vector<std::pair<int,uint32_t>> references;
    bool active=false;
};
inline bool prepare(const Input&input,Projection&out,const char**failure=nullptr) {
    Projection result;result.active=input.active;result.cuts=input.cuts;
    if(failure)*failure="invalid finite cutter";
    if(input.cuts.size()>MaxCuts)return false;for(const auto&cut:input.cuts)if(!valid(cut))return false;
    if(result.active){if(failure)*failure="missing authored geometry";if(!input.original)return false;Material state;
        if(input.material)state=*input.material;
        else {if(failure)*failure="authored source capture";state.source=materialSource(*input.original,input.terrainObjects);if(!state.source)return false;state.outer=state.source->outer;}
        if(failure)*failure="saved material checkpoint";if(!input.checkpoint.empty()&&!decodeMaterial(input.checkpoint,state.source,state))return false;
        if(!updateMaterial(state,input.cuts,failure))return false;
        if(failure)*failure="material checkpoint size/encoding";result.outer=state.outer;result.rawCaps=state.caps;result.checkpoint=encodeMaterial(state);if(result.checkpoint.empty())return false;
        result.material=std::make_shared<const Material>(std::move(state));}
    out=std::move(result);if(failure)*failure=nullptr;return true;
}
inline bool finishCoverage(Projection&projection,const std::vector<CapTriangle>&earlier,std::vector<CapTriangle>&accepted) {
    if(!projection.active){accepted.clear();return true;}
    Mesh caps;if(!resolveCapCoverage(projection.rawCaps,earlier,caps,accepted))return false;
    Mesh retained=projection.outer;storm_ship_laser::appendMesh(retained,caps);if(!finite(retained))return false;
    std::vector<storm_ship_hull::Bounds>boxes;std::vector<std::pair<int,uint32_t>>references;
    boxes.reserve(triangleCount(retained));references.reserve(boxes.capacity());
    for(const auto&o:retained)for(size_t n=0;n<o.second.size();++n){storm_geometry_work::checkpoint();const auto&t=o.second[n];auto box=storm_ship_hull::Bounds{t[0].position,t[0].position};
        for(unsigned c=1;c<3;++c)box=storm_ship_hull::merge(box,{t[c].position,t[c].position});boxes.push_back(box);references.emplace_back(o.first,uint32_t(n));}
    storm_ship_hull::Broadphase broadphase;broadphase.build(boxes);
    projection.retained=std::move(retained);projection.caps=std::move(caps);projection.broadphase=std::move(broadphase);projection.references=std::move(references);return true;
}
} // namespace storm_world_cut

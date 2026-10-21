#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_airburst_hull_distance.hpp"
#include <set>
#include <iostream>
#include <tuple>

// Static-world Boolean fields. Geometry owns reconstruction and rejection;
// scene code owns world/local transforms and persistent ordered records.
namespace storm_world_cut {
inline bool diagnosticFailure(int line){std::cerr<<"world failure line="<<line<<"\n";return false;}
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_laser::Mesh;
using storm_ship_laser::PositionKey;
using storm_ship_laser::PositionEdge;
using storm_ship_laser::Segment;
using Plane = storm_ship_hull::Plane;
inline constexpr size_t MaxCuts=64, MaxTriangles=1000000;
struct Cut {
    enum Kind : uint32_t { Halfspace=0, Sphere=1 };
    Kind kind=Halfspace;
    Vec3 normal{0,1,0}; float distance=0;
    Vec3 center{}; float radius=1;
};
inline bool valid(const Cut&c) {
    if(c.kind==Cut::Halfspace)return storm_ship_hull::finite(c.normal)&&std::isfinite(c.distance)&&
        std::abs(storm_ship_hull::length(c.normal)-1)<.001f&&std::abs(c.distance)<=10000000;
    return c.kind==Cut::Sphere&&storm_ship_hull::finite(c.center)&&std::isfinite(c.radius)&&c.radius>=.05f&&c.radius<=5000&&
        std::max({std::abs(c.center.x),std::abs(c.center.y),std::abs(c.center.z)})<=10000000;
}
inline bool equal(const Cut&a,const Cut&b) {
    if(a.kind!=b.kind)return diagnosticFailure(__LINE__);
    if(a.kind==Cut::Halfspace)return a.normal.x==b.normal.x&&a.normal.y==b.normal.y&&a.normal.z==b.normal.z&&a.distance==b.distance;
    return a.center.x==b.center.x&&a.center.y==b.center.y&&a.center.z==b.center.z&&a.radius==b.radius;
}
inline size_t triangleCount(const Mesh&m){size_t n=0;for(const auto&o:m)n+=o.second.size();return n;}
inline bool affects(const storm_ship_hull::Bounds&b,const std::vector<Cut>&cuts) {
    for(const auto&c:cuts){if(c.kind==Cut::Halfspace){const Vec3 support{c.normal.x>=0?b.hi.x:b.lo.x,c.normal.y>=0?b.hi.y:b.lo.y,c.normal.z>=0?b.hi.z:b.lo.z};
            if(double(c.normal.x)*support.x+double(c.normal.y)*support.y+double(c.normal.z)*support.z>c.distance)return true;
        }else if(c.center.x+c.radius>=b.lo.x&&c.center.x-c.radius<=b.hi.x&&c.center.y+c.radius>=b.lo.y&&c.center.y-c.radius<=b.hi.y&&c.center.z+c.radius>=b.lo.z&&c.center.z-c.radius<=b.hi.z)return true;}
    return diagnosticFailure(__LINE__);
}
inline bool finite(const Mesh&m) {
    if(triangleCount(m)>MaxTriangles)return diagnosticFailure(__LINE__);
    for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t)
        if(!storm_ship_hull::finite(v.position)||!storm_ship_hull::finite(v.normal)||
           std::max({std::abs(v.position.x),std::abs(v.position.y),std::abs(v.position.z)})>10000000)return diagnosticFailure(__LINE__);
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
    for(const auto&e:edges){if(e.second.count>2||(e.second.count==2&&e.second.winding))return diagnosticFailure(__LINE__);
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
    std::map<PositionKey,std::vector<size_t>>adjacent;std::set<PositionEdge>unique;std::vector<Segment>edges;
    for(const auto&e:seam){auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);if(a==b)continue;
        auto ordered=std::minmax(a,b);if(!unique.insert({ordered.first,ordered.second}).second)continue;
        adjacent[a].push_back(edges.size());adjacent[b].push_back(edges.size());edges.push_back(e);}
    if(unique.empty())return true;if(unique.size()>65536)return diagnosticFailure(__LINE__);
    for(const auto&p:adjacent)if(p.second.size()!=2)return diagnosticFailure(__LINE__);
    const auto basis=storm_ship_hull::frame(plane.normal);const auto origin=edges.front().a.position;
    const double positionTolerance=std::max(1.e-6,2.*std::numeric_limits<float>::epsilon()*
        std::max({std::abs(double(origin.x)),std::abs(double(origin.y)),std::abs(double(origin.z)),std::abs(double(plane.distance)),1.}));
    const auto project=[&](Vec3 p){const double x=double(p.x)-origin.x,y=double(p.y)-origin.y,z=double(p.z)-origin.z;return Point{
        x*basis.tangent.x+y*basis.tangent.y+z*basis.tangent.z,x*basis.bitangent.x+y*basis.bitangent.y+z*basis.bitangent.z};};
    struct Ring{std::vector<Vertex>vertices;std::vector<Point>points;double area=0;int object=0,parent=-1,depth=0;};
    std::vector<Ring>rings;std::vector<bool>used(edges.size());
    for(size_t start=0;start<edges.size();++start){if(used[start])continue;Ring ring;ring.object=edges[start].object;
        const auto first=storm_ship_laser::positionKey(edges[start].a.position);auto current=first;size_t next=start;
        do{if(used[next])return diagnosticFailure(__LINE__);used[next]=true;const auto&e=edges[next];const bool forward=storm_ship_laser::positionKey(e.a.position)==current;
            ring.vertices.push_back(forward?e.a:e.b);current=storm_ship_laser::positionKey((forward?e.b:e.a).position);if(current==first)break;
            const auto&neighbor=adjacent.at(current);next=used[neighbor[0]]?neighbor[1]:neighbor[0];
        }while(ring.vertices.size()<=edges.size());
        if(current!=first||ring.vertices.size()<3)return diagnosticFailure(__LINE__);
        for(const auto&v:ring.vertices)ring.points.push_back(project(v.position));
        for(size_t n=0;n<ring.points.size();++n){const auto a=ring.points[n],b=ring.points[(n+1)%ring.points.size()];ring.area+=a[0]*b[1]-b[0]*a[1];}
        ring.area=std::abs(ring.area)*.5;if(ring.area<=1.e-8)return diagnosticFailure(__LINE__);rings.push_back(std::move(ring));}
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
        if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4)){
std::cerr<<"cap area expected="<<expected<<" measured="<<measured<<" tolerance="<<positionTolerance<<" vertices="<<vertices.size()<<"\n";
unsigned crossings=0;const auto cross=[](Point a,Point b,Point c){return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);};for(size_t ri=0;ri<polygon.size();++ri)for(size_t i=0;i<polygon[ri].size();++i)for(size_t rj=ri;rj<polygon.size();++rj)for(size_t j=0;j<polygon[rj].size();++j){if(ri==rj&&(i==j||(i+1)%polygon[ri].size()==j||(j+1)%polygon[ri].size()==i||j<i))continue;auto a=polygon[ri][i],b=polygon[ri][(i+1)%polygon[ri].size()],c=polygon[rj][j],d=polygon[rj][(j+1)%polygon[rj].size()];if(cross(a,b,c)*cross(a,b,d)<0&&cross(c,d,a)*cross(c,d,b)<0){++crossings;std::cerr<<"proper crossing rings="<<ri<<","<<rj<<" edges="<<i<<","<<j<<" pts="<<a[0]<<","<<a[1]<<" "<<b[0]<<","<<b[1]<<" "<<c[0]<<","<<c[1]<<" "<<d[0]<<","<<d[1]<<"\n";}}std::cerr<<"crossings="<<crossings<<"\n";
return diagnosticFailure(__LINE__);}
    }
    // Every contour must appear in the output boundary, including holes.
    std::vector<Segment>actual;if(!boundaries(out,actual))return diagnosticFailure(__LINE__);
    std::set<PositionEdge>capEdges;
    for(const auto&e:actual){auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);auto ordered=std::minmax(a,b);capEdges.insert({ordered.first,ordered.second});}
    if(capEdges!=unique){std::cerr<<"cap mismatch expected="<<unique.size()<<" actual="<<capEdges.size()<<"\n";return diagnosticFailure(__LINE__);}return true;
}
inline void surfaceNormal(Triangle&t) {
    const auto n=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
    for(auto&v:t)v.normal=n;
}
// Terrain is an authored top sheet. Close its actual coast, below all terrain,
// without changing any exposed vertex, UV, color or original material.
inline bool terrainSolid(const Mesh&source,float floor,Mesh&out) {
    std::vector<Segment>coast;if(!boundaries(source,coast))return diagnosticFailure(__LINE__);
    out=source;if(coast.empty())return true;
    std::vector<Segment>bottom;bottom.reserve(coast.size());
    for(const auto&e:coast){auto a=e.a,b=e.b,da=a,db=b;da.position.y=db.position.y=floor;
        Triangle first{b,a,da},second{b,da,db};surfaceNormal(first);surfaceNormal(second);
        if(storm_ship_hull::area(first)>1.e-8f)out[e.object].push_back(first);
        if(storm_ship_hull::area(second)>1.e-8f)out[e.object].push_back(second);
        bottom.push_back({da,db,e.object});}
    Mesh base;if(!caps(bottom,{{0,-1,0},-floor},base))return diagnosticFailure(__LINE__);
    storm_ship_laser::appendMesh(out,base);return finite(out);
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
inline std::map<PositionKey,float> stoneClearance(const Mesh&source,float maximum) {
    struct Point{Vec3 position{},normal{};float depth=0;};std::map<PositionKey,Point>points;
    struct Feature{Vec3 normal{};unsigned count=0;bool sharp=false;float length=0,altitude=std::numeric_limits<float>::infinity();};
    std::map<PositionEdge,Feature>features;std::vector<const Triangle*>triangles;std::vector<storm_ship_hull::Bounds>boxes;
    for(const auto&o:source)for(const auto&t:o.second){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
        storm_ship_hull::Bounds box{t[0].position,t[0].position};for(unsigned c=0;c<3;++c){const auto&v=t[c];auto inserted=points.try_emplace(storm_ship_laser::positionKey(v.position),Point{v.position,{},maximum});
            const float opposite=storm_ship_hull::length(t[(c+1)%3].position-t[(c+2)%3].position);
            if(opposite>1.e-8f)inserted.first->second.depth=std::min(inserted.first->second.depth,storm_ship_hull::area(t)*.25f/opposite);
            inserted.first->second.normal=inserted.first->second.normal+(storm_ship_hull::length(v.normal)>1.e-6f?storm_ship_hull::normalized(v.normal):normal);box=storm_ship_hull::merge(box,{v.position,v.position});}
        triangles.push_back(&t);boxes.push_back(box);
        for(unsigned n=0;n<3;++n){auto a=storm_ship_laser::positionKey(t[n].position),b=storm_ship_laser::positionKey(t[(n+1)%3].position);if(a==b)continue;auto ordered=std::minmax(a,b);auto&edge=features[{ordered.first,ordered.second}];
            if(!edge.count){edge.normal=normal;edge.length=storm_ship_hull::length(t[n].position-t[(n+1)%3].position);}else edge.sharp|=storm_ship_hull::dot(edge.normal,normal)<.7f;
            edge.altitude=std::min(edge.altitude,2*storm_ship_hull::area(t)/edge.length);++edge.count;}}
    // Smooth tessellation edges do not set a material thickness. Only actual
    // openings and sharp feature edges constrain the local offset radius.
    for(const auto&e:features)if(e.second.count==1||e.second.sharp)for(const auto&p:{e.first.first,e.first.second})points.at(p).depth=std::min(points.at(p).depth,std::min(e.second.length,e.second.altitude)*.25f);
    storm_ship_hull::Broadphase broadphase;broadphase.build(boxes);std::vector<uint32_t>candidates;std::map<PositionKey,float>limits;
    for(auto&entry:points){auto&point=entry.second;const auto direction=storm_ship_hull::normalized(point.normal)*-1.f;
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
inline bool stoneSolid(const Mesh&source,Mesh&out,float thickness=1) {
    for(auto&component:components(source)){std::vector<Segment>boundary;if(!boundaries(component,boundary))return diagnosticFailure(__LINE__);
        if(boundary.empty())storm_ship_laser::appendMesh(out,component);
        else {std::map<int,float>depth;for(const auto&o:component)depth[o.first]=thickness;
            const auto limits=stoneClearance(component,thickness);
            auto solid=storm_ship_laser::shellForRender(component,depth,&limits);std::vector<Segment>remaining;
            if(!boundaries(solid,remaining)||!remaining.empty())return diagnosticFailure(__LINE__);
            storm_ship_laser::appendMesh(out,solid);}
        if(triangleCount(out)>MaxTriangles)return diagnosticFailure(__LINE__);}
    for(const auto&o:source)out.try_emplace(o.first);return finite(out);
}
inline bool prepare(const Mesh&original,const std::set<int>&terrainObjects,const std::vector<Cut>&cuts,Mesh&solid,Mesh&untouched) {
    Mesh terrain,stone;for(const auto&o:original)(terrainObjects.count(o.first)?terrain:stone)[o.first]=o.second;
    for(unsigned kind=0;kind<2;++kind)for(auto&component:components(kind?stone:terrain)){
        auto bounds=storm_ship_laser::bounds(component);const float floor=bounds.lo.y-10032.f;
        if(!kind)bounds.lo.y=floor;
        if(!affects(bounds,cuts)){storm_ship_laser::appendMesh(untouched,component);continue;}
        Mesh prepared;if(!(kind?stoneSolid(component,prepared):terrainSolid(component,floor,prepared)))return diagnosticFailure(__LINE__);
        storm_ship_laser::appendMesh(solid,prepared);if(triangleCount(solid)+triangleCount(untouched)>MaxTriangles)return diagnosticFailure(__LINE__);
    }
    for(const auto&o:original){solid.try_emplace(o.first);untouched.try_emplace(o.first);}return finite(solid)&&finite(untouched);
}
inline std::vector<Plane> spherePlanes(const Cut&c) {
    std::vector<Plane>planes;planes.reserve(26);
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z){if(!x&&!y&&!z)continue;
        const auto n=storm_ship_hull::normalized(Vec3{float(x),float(y),float(z)});planes.push_back({n,storm_ship_hull::dot(n,c.center)+c.radius});}
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
        if(edge.size()==2&&storm_ship_laser::positionKey(edge[0].position)!=storm_ship_laser::positionKey(edge[1].position))seam.push_back({edge[0],edge[1],o.first});
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
        if(triangleCount(out)>MaxTriangles)return diagnosticFailure(__LINE__);}}
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
        if(!(c.kind==Cut::Halfspace?retainHalfspaceSurface(source,{c.normal,c.distance},next):subtractSphereSurface(source,c,next)))return diagnosticFailure(__LINE__);
        source=std::move(next);}
    out=std::move(source);return true;
}
inline bool reconstruct(const Mesh&original,const std::vector<Cut>&cuts,Mesh&out) {
    if(cuts.size()>MaxCuts||!finite(original))return diagnosticFailure(__LINE__);
    for(const auto&cut:cuts)if(!valid(cut))return diagnosticFailure(__LINE__);
    std::vector<Cut>unique;for(const auto&c:cuts){bool found=false;for(const auto&p:unique)found|=equal(c,p);if(!found)unique.push_back(c);}
    Mesh result;if(!applySurfaceCuts(original,unique,unique.size(),result))return diagnosticFailure(__LINE__);
    // Boundary of A minus union(B): retain A's exposed faces; each cutter face
    // is intersected with immutable A and removed by every other B. Never feed
    // a previous triangulated cap back into contour discovery.
    for(size_t n=0;n<unique.size();++n){const auto&c=unique[n];const auto planes=c.kind==Cut::Sphere?spherePlanes(c):std::vector<Plane>{{c.normal,c.distance}};
        for(size_t side=0;side<planes.size();++side){Mesh cap;if(!crossSection(original,planes[side],cap)){std::cerr<<"plane side="<<side<<" n="<<planes[side].normal.x<<","<<planes[side].normal.y<<","<<planes[side].normal.z<<" d="<<planes[side].distance<<"\n";return diagnosticFailure(__LINE__);}
            if(c.kind==Cut::Sphere){
                for(size_t p=0;p<planes.size();++p)if(p!=side){Mesh clipped;if(!retainHalfspaceSurface(cap,planes[p],clipped))return diagnosticFailure(__LINE__);cap=std::move(clipped);}
                for(auto&o:cap)for(auto&t:o.second){std::swap(t[1],t[2]);for(auto&v:t)v.normal=planes[side].normal*-1.f;}
            }
            const Plane boundary=c.kind==Cut::Sphere?Plane{planes[side].normal*-1.f,-planes[side].distance}:planes[side];
            Mesh retained;if(!applySurfaceCuts(std::move(cap),unique,n,retained,&boundary))return diagnosticFailure(__LINE__);
            storm_ship_laser::appendMesh(result,retained);if(triangleCount(result)>MaxTriangles)return diagnosticFailure(__LINE__);
        }
    }
    if(!finite(result))return diagnosticFailure(__LINE__);out=std::move(result);return true;
}
} // namespace storm_world_cut

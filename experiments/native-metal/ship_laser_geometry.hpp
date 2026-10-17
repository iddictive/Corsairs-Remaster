#pragma once
#include "ship_hull_geometry.hpp"
// Mapbox earcut.hpp v3.2.3, c68c8835ccff2b7532d31d8fa8dfcf398f629498 (ISC).
#include "third_party/mapbox-earcut/earcut.hpp"
#include <unordered_map>
#include "ship_hull_damage_bridge.hpp"
#include <map>
#include <limits>
#include <numeric>
#include <memory>
#include <set>
#include <utility>

namespace storm_ship_structure { struct State; }

// A deterministic, node-local half-space split of the actual static GM mesh.
// Vertex interpolation/layout remain owned by ship_hull_geometry.hpp.
namespace storm_ship_laser {
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_hull::Bounds;
using Mesh = std::map<int,std::vector<Triangle>>;
inline std::vector<Mesh> components(const Mesh&);
using FacePoint=std::array<double,2>;
using FaceRing=std::vector<FacePoint>;
using FacePolygon=std::vector<FaceRing>;
inline constexpr int64_t PlanarUnitsPerMeter=1000000;
inline constexpr double PlanarUnit=1./double(PlanarUnitsPerMeter);
bool differenceFace(const FaceRing&,const FaceRing&,std::vector<FacePolygon>&);
bool differenceFaces(const FaceRing&,const std::vector<FaceRing>&,std::vector<FacePolygon>&);
bool unionFaces(const std::vector<FaceRing>&,std::vector<FacePolygon>&);
using PositionKey=std::array<int64_t,3>;
using PositionEdge=std::pair<PositionKey,PositionKey>;
struct PositionEdgeHash {size_t operator()(const PositionEdge&e)const {size_t h=0;for(const auto&point:{e.first,e.second})for(auto q:point)h^=std::hash<int64_t>{}(q)+0x9e3779b9u+(h<<6)+(h>>2);return h;}};
inline PositionKey positionKey(Vec3 p) {storm_geometry_work::checkpoint();return {int64_t(std::llround(double(p.x)*10000)),int64_t(std::llround(double(p.y)*10000)),int64_t(std::llround(double(p.z)*10000))};}
inline bool topologyTriangle(const Triangle&t){const auto a=positionKey(t[0].position),b=positionKey(t[1].position),c=positionKey(t[2].position);const PositionKey u{b[0]-a[0],b[1]-a[1],b[2]-a[2]},v{c[0]-a[0],c[1]-a[1],c[2]-a[2]};return __int128(u[0])*v[1]!=__int128(u[1])*v[0]||__int128(u[1])*v[2]!=__int128(u[2])*v[1]||__int128(u[2])*v[0]!=__int128(u[0])*v[2];}
using BeamSample=storm_hull_damage::BeamSample;
inline Vec3 beamStart(const BeamSample&b){return {b.start[0],b.start[1],b.start[2]};}
inline Vec3 beamDirection(const BeamSample&b){return {b.direction[0],b.direction[1],b.direction[2]};}
inline Vec3 beamTangent(const BeamSample&b){return b.tangent==std::array<float,3>{} ? storm_ship_hull::frame(beamDirection(b)).tangent : Vec3{b.tangent[0],b.tangent[1],b.tangent[2]};}
inline bool valid(const BeamSample&b){return storm_ship_hull::finite(beamStart(b))&&storm_ship_hull::finite(beamDirection(b))&&
    std::abs(storm_ship_hull::length(beamDirection(b))-1.f)<.001f&&std::isfinite(b.radius)&&b.radius>0&&
    std::isfinite(b.depth)&&b.depth>0&&storm_ship_hull::finite(beamTangent(b))&&
    std::abs(storm_ship_hull::length(beamTangent(b))-1.f)<.001f&&std::abs(storm_ship_hull::dot(beamTangent(b),beamDirection(b)))<.001f&&storm_hull_damage::validCrossSection(b);}
struct BeamPlanes {
    std::array<storm_ship_hull::Plane,10> planes{};unsigned count=10;
    auto begin()const{return planes.begin();}auto end()const{return planes.begin()+count;}
    auto begin(){return planes.begin();}auto end(){return planes.begin()+count;}
    size_t size()const{return count;}
    storm_ship_hull::Plane&operator[](size_t n){return planes[n];}const storm_ship_hull::Plane&operator[](size_t n)const{return planes[n];}
};
inline BeamPlanes beamPlanes(const BeamSample&b){
    const auto start=beamStart(b),direction=beamDirection(b);const storm_ship_hull::Frame basis{beamTangent(b),storm_ship_hull::cross(direction,beamTangent(b))};
    BeamPlanes result;result.count=unsigned(b.crossSection.empty()?10:b.crossSection.size()+2);
    result[0]={direction,storm_ship_hull::dot(direction,start)+b.depth};
    result[1]={direction*-1.f,-storm_ship_hull::dot(direction,start)+(b.backDepth<0?b.radius*.15625f:b.backDepth)};
    if(!b.crossSection.empty()){for(unsigned n=0;n<b.crossSection.size();++n){const auto&a=b.crossSection[n],&c=b.crossSection[(n+1)%b.crossSection.size()];
        const auto normal=storm_ship_hull::normalized(basis.tangent*(c[1]-a[1])+basis.bitangent*(a[0]-c[0]));
        result[n+2]={normal,storm_ship_hull::dot(normal,start+basis.tangent*a[0]+basis.bitangent*a[1])};}return result;}
    constexpr float diagonal=.7071067811865475f;
    constexpr std::array<std::array<float,2>,8> octagon{{{1,0},{diagonal,diagonal},{0,1},{-diagonal,diagonal},
        {-1,0},{-diagonal,-diagonal},{0,-1},{diagonal,-diagonal}}};
    for(unsigned n=0;n<8;++n){const auto normal=basis.tangent*octagon[n][0]+basis.bitangent*octagon[n][1];
        result[n+2]={normal,storm_ship_hull::dot(normal,start)+b.radius};}
    return result;
}
// A queued trace can still hit a bore's old entrance. Replay its centre ray
// through that actual empty prism, including small changes of aim/ship pose.
// A neighboring stroke or a ray leaving through a side gets no free depth.
inline void continueBeamDose(BeamSample&sample,const std::vector<BeamSample>&committed,size_t count=std::numeric_limits<size_t>::max()){
    if(!valid(sample)||!sample.crossSection.empty()||sample.specificWork>0||sample.backDepth>=0)return;
    for(size_t i=0;i<std::min(count,committed.size());++i){const auto&prior=committed[i];
        if(!valid(prior)||!prior.crossSection.empty()||prior.specificWork>0||prior.backDepth>=0||std::abs(sample.radius-prior.radius)>.00001f)continue;
        const auto start=beamStart(sample),direction=beamDirection(sample),axis=beamDirection(prior),delta=start-beamStart(prior);
        const float tolerance=std::max(.0001f,2*std::numeric_limits<float>::epsilon()*std::max({std::abs(start.x),std::abs(start.y),std::abs(start.z),1.f}));
        const float along=storm_ship_hull::dot(delta,axis),forward=storm_ship_hull::dot(direction,axis);
        if(along>prior.depth+tolerance||forward<=1.e-6f)continue;
        const float travel=(prior.depth-along)/forward;
        if(travel<=tolerance)continue;
        const auto end=start+direction*travel;const auto planes=beamPlanes(prior);bool inside=true;
        for(unsigned n=1;n<planes.size();++n)
            if(storm_ship_hull::dot(planes[n].normal,start)>planes[n].distance+tolerance||
               storm_ship_hull::dot(planes[n].normal,end)>planes[n].distance+tolerance){inside=false;break;}
        if(inside)sample.start={end.x,end.y,end.z};
    }
}
inline void continueBeamDose(std::vector<BeamSample>&next,const std::vector<BeamSample>&committed){
    for(size_t n=0;n<next.size();++n){
        continueBeamDose(next[n],committed);
        // A predecessor may move the first sample past its siblings' stale
        // starts. Rechain only earlier doses of this same atomic request.
        continueBeamDose(next[n],next,n);
    }
}
inline float beamDistance(const BeamSample&b,Vec3 point){float inside=std::numeric_limits<float>::infinity();
    for(const auto&p:beamPlanes(b))inside=std::min(inside,p.distance-storm_ship_hull::dot(p.normal,point));return inside;}
inline bool beamIntersectsBox(const BeamSample&b,Vec3 center,Vec3 halfSize){
    if(!valid(b))return false;for(const auto&p:beamPlanes(b)){
        const float reach=std::abs(p.normal.x)*halfSize.x+std::abs(p.normal.y)*halfSize.y+std::abs(p.normal.z)*halfSize.z;
        if(storm_ship_hull::dot(p.normal,center)-reach>p.distance)return false;}return true;
}
struct Plane { Vec3 normal; float distance=0; bool charred=true; Vec3 grain{0,1,0}; float roughness=0,spacing=1,phase=0;
    std::vector<BeamSample> beams; bool releaseEnclosed=true; };
inline bool valid(const Plane&p) {
    if(!p.beams.empty()){if(p.beams.size()>storm_hull_damage::MaxBeamSamples)return false;
        for(const auto&b:p.beams)if(!valid(b))return false;return p.roughness==0;}
    const float n=storm_ship_hull::length(p.normal),g=storm_ship_hull::length(p.grain);
    return storm_ship_hull::finite(p.normal)&&std::isfinite(p.distance)&&n>.999f&&n<1.001f&&
        std::isfinite(p.roughness)&&p.roughness>=0&&p.roughness<=1000&&std::isfinite(p.spacing)&&p.spacing>=.01f&&p.spacing<=10000&&
        std::isfinite(p.phase)&&std::abs(p.phase)<=1000000&&storm_ship_hull::finite(p.grain)&&
        (p.roughness==0||(g>.999f&&g<1.001f&&std::abs(storm_ship_hull::dot(p.normal,p.grain))<1.e-5f));
}
// Stateless lattice values: phase is a grain-coordinate translation in metres,
// not a random seed. Both halves and saved/replayed cuts evaluate the same field.
inline float offset(const Plane&p,Vec3 v) {
    if(p.roughness==0||!valid(p)||!storm_ship_hull::finite(v))return 0;
    const double coordinate=(double(storm_ship_hull::dot(p.grain,v))+p.phase)/p.spacing,cell=std::floor(coordinate);
    if(!std::isfinite(coordinate))return 0;
    const auto sample=[](double cell){uint32_t h=uint32_t(std::fmod(std::fmod(cell,4294967296.)+4294967296.,4294967296.));h^=0x9e3779b9u;h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;return float(h&0xffffffu)*(2.f/16777215.f)-1.f;};
    const float a=sample(cell),b=sample(cell+1);return p.roughness*(a+(b-a)*float(coordinate-cell));
}
inline float distance(const Plane&p,Vec3 v) { if(!p.beams.empty()){float result=-std::numeric_limits<float>::infinity();
    for(const auto&b:p.beams)result=std::max(result,beamDistance(b,v));return result;}
    return storm_ship_hull::dot(p.normal,v)-p.distance-offset(p,v); }
struct Segment { Vertex a,b; int object=0; };
// Open authored GM shells have no validated volume. These are measured surface
// properties (a uniform surface-density proxy), never a claim of solid wood mass.
struct Fragment { uint64_t fragment=0; Bounds bounds{}; Vec3 centroid{}; float surfaceArea=0; std::array<Vec3,14>support{}; };
struct Opening { Vec3 point; float area=0; uint64_t source=UINT64_MAX; };
struct Result { bool changed=false,stateChanged=false; uint64_t fragment=0; float removedFraction=0; Bounds keptBounds{}; bool volumeBased=false; std::vector<Segment>seam; std::vector<Fragment>fragments; bool supportSevered=false; std::vector<Opening>openings; };
struct Split { Mesh kept,detached; float keptArea=0,detachedArea=0; std::vector<Segment>seam; bool complete=true; };
inline bool near(Vec3 a,Vec3 b) { auto d=a-b;return storm_ship_hull::dot(d,d)<1.e-8f; }
inline Bounds bounds(const Mesh&m) { Bounds b{};bool first=true;for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t){storm_geometry_work::checkpoint();if(first){b={v.position,v.position};first=false;}else b=storm_ship_hull::merge(b,{v.position,v.position});}return b; }
inline Fragment fragmentProperties(const Mesh&mesh) {
    Fragment result;result.bounds=bounds(mesh);double area=0,x=0,y=0,z=0;bool first=true;
    const std::array<Vec3,14>directions={{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},
        {1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},{-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1}}};
    for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();const double a=storm_ship_hull::area(t);area+=a;
        x+=a*(double(t[0].position.x)+t[1].position.x+t[2].position.x)/3.;
        y+=a*(double(t[0].position.y)+t[1].position.y+t[2].position.y)/3.;
        z+=a*(double(t[0].position.z)+t[1].position.z+t[2].position.z)/3.;
        for(const auto&v:t){for(size_t n=0;n<directions.size();++n)
            if(first||storm_ship_hull::dot(v.position,directions[n])>storm_ship_hull::dot(result.support[n],directions[n]))result.support[n]=v.position;
            first=false;}}
    result.surfaceArea=float(area);result.centroid=area>1.e-12?Vec3{float(x/area),float(y/area),float(z/area)}:(result.bounds.lo+result.bounds.hi)*.5f;return result;
}
// Thickness for explicitly classified structural wood. Ordinary canonical
// collision and structural accounting keep the authored shell; laser material
// queries include its inner skin. Reconstruction must not grow another shell.
inline Mesh shellForRender(const Mesh&mesh,const std::map<int,float>&thicknesses,const std::map<PositionKey,float>*depthLimits=nullptr,const std::map<PositionKey,Vec3>*offsetNormals=nullptr) {
    Mesh result=mesh;
    using Point=std::array<int64_t,3>;using Edge=std::pair<Point,Point>;
    const auto key=positionKey;
    const auto darken=[](Vertex&v,float factor){uint32_t color=v.color&0xff000000u;for(unsigned c=0;c<3;++c)color|=uint32_t(float((v.color>>(c*8))&255u)*factor)<<(c*8);v.color=color;};
    struct Offset {Vec3 normal{};double thickness=0;size_t count=0;};std::map<Point,Offset>offsets;
    // One inward position at shared corners, including UV/material seams. Using
    // each face's own shading normal there would tear the new inner shell open.
    for(const auto&o:mesh){const auto selected=thicknesses.find(o.first);if(selected==thicknesses.end()||!std::isfinite(selected->second)||selected->second<=0)continue;
        for(const auto&t:o.second){storm_geometry_work::checkpoint();const auto fallback=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
            for(const auto&v:t){auto&offset=offsets[key(v.position)];offset.normal=offset.normal+(storm_ship_hull::length(v.normal)>1.e-6f?storm_ship_hull::normalized(v.normal):fallback);offset.thickness+=selected->second;++offset.count;}}}
    const auto inward=[&](const Vertex&v){const auto&offset=offsets.at(key(v.position));auto normal=storm_ship_hull::length(offset.normal)>1.e-6f?storm_ship_hull::normalized(offset.normal):storm_ship_hull::normalized(v.normal);
        if(offsetNormals){const auto found=offsetNormals->find(key(v.position));if(found!=offsetNormals->end())normal=found->second;}
        float depth=float(offset.thickness/offset.count);if(depthLimits){const auto limit=depthLimits->find(key(v.position));if(limit!=depthLimits->end())depth=std::min(depth,std::max(0.f,limit->second));}
        return v.position-normal*depth;};
    struct Boundary{Vertex a,b;int object=0;unsigned count=0;};std::map<Edge,Boundary>edges;
    for(const auto&o:mesh){const auto selected=thicknesses.find(o.first);if(selected==thicknesses.end()||!std::isfinite(selected->second)||selected->second<=0)continue;
        auto&out=result[o.first];
        for(const auto&t:o.second){storm_geometry_work::checkpoint();const auto fallback=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));Triangle inner=t;
            for(unsigned c=0;c<3;++c){const auto normal=storm_ship_hull::length(t[c].normal)>1.e-6f?storm_ship_hull::normalized(t[c].normal):fallback;
                inner[c].position=inward(t[c]);inner[c].normal=normal*-1.f;darken(inner[c],.62f);
                auto a=key(t[c].position),b=key(t[(c+1)%3].position);if(a==b)continue;if(b<a)std::swap(a,b);auto&edge=edges[{a,b}];if(!edge.count){edge.a=t[c];edge.b=t[(c+1)%3];edge.object=o.first;
                    if(storm_ship_hull::length(edge.a.normal)<=1.e-6f)edge.a.normal=fallback;if(storm_ship_hull::length(edge.b.normal)<=1.e-6f)edge.b.normal=fallback;}++edge.count;}
            std::swap(inner[1],inner[2]);out.push_back(inner);}}
        for(const auto&entry:edges){const auto&edge=entry.second;if(edge.count!=1)continue;auto&out=result[edge.object];auto a=edge.a,b=edge.b,c=b,d=a;
            const float thickness=depthLimits?(storm_ship_hull::length(inward(a)-a.position)+storm_ship_hull::length(inward(b)-b.position))*.5f:thicknesses.at(edge.object);
            c.position=inward(b);d.position=inward(a);
            const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(d.position-a.position,c.position-a.position));
            a.normal=b.normal=c.normal=d.normal=normal;darken(a,.78f);darken(b,.78f);darken(c,.78f);darken(d,.78f);
            const float length=storm_ship_hull::length(b.position-a.position);if(length>1.e-6f)for(unsigned uv=0;uv<4;++uv){const float du=(b.uv[uv*2]-a.uv[uv*2])*thickness/length,dv=(b.uv[uv*2+1]-a.uv[uv*2+1])*thickness/length;
                c.uv[uv*2]-=dv;d.uv[uv*2]-=dv;c.uv[uv*2+1]+=du;d.uv[uv*2+1]+=du;}
            Triangle first={a,d,c},second={a,c,b};if(storm_ship_hull::area(first)>1.e-10f)out.push_back(first);if(storm_ship_hull::area(second)>1.e-10f)out.push_back(second);}
    return result;
}
inline float cross2(Vec3 a,Vec3 b,Vec3 c,Vec3 normal) { return storm_ship_hull::dot(storm_ship_hull::cross(b-a,c-a),normal); }
inline double closedVolume(const Mesh&mesh) {
    using Point=PositionKey;std::unordered_map<PositionEdge,std::pair<unsigned,int>,PositionEdgeHash>edges;double volume=0;
    size_t triangles=0;for(const auto&o:mesh)triangles+=o.second.size();edges.reserve(triangles*3);
    const auto key=positionKey;
    for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();const Point points[3]={key(t[0].position),key(t[1].position),key(t[2].position)};if(points[0]==points[1]||points[0]==points[2]||points[1]==points[2])continue;
        volume+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6.;
        for(unsigned n=0;n<3;++n){auto a=points[n],b=points[(n+1)%3];int direction=1;if(b<a){std::swap(a,b);direction=-1;}auto&e=edges[{a,b}];++e.first;e.second+=direction;}}
    for(const auto&e:edges)if(e.second.first!=2||e.second.second!=0)return 0;return std::abs(volume);
}
// A polygon boolean can subdivide one side of a shared edge without changing
// the material boundary. Validate directed interval coverage, rather than
// treating every harmless T-junction as an open solid.
inline double closedCutVolume(const Mesh&mesh){
    std::map<PositionEdge,std::pair<unsigned,int>>edges;double volume=0;for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();if(!topologyTriangle(t))continue;const PositionKey points[3]{positionKey(t[0].position),positionKey(t[1].position),positionKey(t[2].position)};volume+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6.;
        for(unsigned n=0;n<3;++n){auto a=points[n],b=points[(n+1)%3];int sign=1;if(b<a){std::swap(a,b);sign=-1;}auto&edge=edges[{a,b}];++edge.first;edge.second+=sign;}}
    std::vector<PositionEdge>unmatched;std::map<PositionKey,Vec3>points;for(const auto&edge:edges)if(edge.second.first!=2||edge.second.second!=0){unmatched.push_back(edge.first);for(const auto&point:{edge.first.first,edge.first.second})points.emplace(point,Vec3{float(point[0])*.0001f,float(point[1])*.0001f,float(point[2])*.0001f});}
    if(unmatched.empty())return std::abs(volume);if(unmatched.size()>4096)return 0;
    for(const auto&edge:unmatched){const auto a=points.at(edge.first),b=points.at(edge.second),line=b-a;const double length2=storm_ship_hull::dot(line,line),length=std::sqrt(length2),tolerance=.00015/length;struct Event{double at;int count,balance;};std::vector<Event>events;
        for(const auto&candidate:unmatched){storm_geometry_work::checkpoint();const auto first=points.at(candidate.first)-a,last=points.at(candidate.second)-a;const double x=storm_ship_hull::dot(first,line)/length2,y=storm_ship_hull::dot(last,line)/length2,low=std::max(0.,std::min(x,y)),high=std::min(1.,std::max(x,y));if(high<=low||std::abs(y-x)<1.e-14)continue;const auto start=first+(last-first)*float((low-x)/(y-x)),end=first+(last-first)*float((high-x)/(y-x));if(storm_ship_hull::length(storm_ship_hull::cross(line,start))>.00015*length||storm_ship_hull::length(storm_ship_hull::cross(line,end))>.00015*length)continue;
            const auto value=edges.at(candidate);const int sign=y>=x?1:-1;events.push_back({low,int(value.first),value.second*sign});events.push_back({high,-int(value.first),-value.second*sign});}
        std::sort(events.begin(),events.end(),[](const auto&a,const auto&b){return a.at<b.at;});int count=0,balance=0;double end=0;for(size_t n=0;n<events.size();){const double at=events[n].at;if(at>end+tolerance&&(count!=2||balance!=0))return 0;size_t last=n;while(last<events.size()&&events[last].at<=at+tolerance){count+=events[last].count;balance+=events[last].balance;++last;}end=events[last-1].at;n=last;}if(end<1-tolerance||count||balance)return 0;}
    return std::abs(volume);
}
// Split an actual boundary-adjacent surface strip, retaining its original UVs,
// normals and material. No decal, floating strip or whole-piece black tint.
inline void charBand(const std::vector<Vertex>&polygon,const Plane&p,bool kept,bool scorch,std::vector<Triangle>&out) {
    if(!scorch){storm_ship_hull::fan(polygon,out);return;}
    const float width=.08f;storm_ship_hull::Plane inner{kept?p.normal:p.normal*-1.f,(kept?p.distance:-p.distance)-width};
    std::vector<Vertex>deep,band;storm_ship_hull::split(polygon,inner,deep,band);storm_ship_hull::fan(deep,out);
    if(scorch)for(auto&v:band){uint32_t color=v.color&0xff000000u;for(unsigned channel=0;channel<3;++channel)color|=uint32_t(float((v.color>>(channel*8))&255u)*.22f)<<(channel*8);v.color=color;}
    storm_ship_hull::fan(band,out);
}

// GM surfaces can have seams, disconnected shells and open deck/sail sheets.
// Weld positions only for contour discovery, never surface UVs/normals. Cap only
// closed, simple loops; an open authored sheet must not acquire a fabricated disk.
inline void capLegacy(std::vector<Segment> segments,const Plane&p,Mesh&kept,Mesh&detached) {
    std::vector<bool>used(segments.size());
    for(size_t start=0;start<segments.size();++start){
        if(used[start])continue;used[start]=true;std::vector<Vertex>loop{segments[start].a,segments[start].b};const int object=segments[start].object;
        bool closed=false;
        for(size_t steps=0;steps<segments.size();++steps){
            if(near(loop.back().position,loop.front().position)){closed=true;loop.pop_back();break;}
            size_t next=segments.size();bool reverse=false;
            for(size_t n=0;n<segments.size();++n)if(!used[n]){storm_geometry_work::checkpoint();
                if(near(loop.back().position,segments[n].a.position)){next=n;break;}
                if(near(loop.back().position,segments[n].b.position)){next=n;reverse=true;break;}
            }
            if(next==segments.size())break;used[next]=true;loop.push_back(reverse?segments[next].a:segments[next].b);
        }
        if(!closed||loop.size()<3)continue;
        float winding=0;for(size_t n=1;n+1<loop.size();++n)winding+=cross2(loop[0].position,loop[n].position,loop[n+1].position,p.normal);
        if(winding<0)std::reverse(loop.begin(),loop.end());
        auto basis=storm_ship_hull::frame(p.normal);for(auto&v:loop){v.normal=p.normal;if(p.charred)v.color=(v.color&0xff000000u)|0x00645140u;for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=storm_ship_hull::dot(v.position,basis.tangent)*.12f;v.uv[uv*2+1]=storm_ship_hull::dot(v.position,basis.bitangent)*.12f;}}
        std::vector<Triangle>triangles;auto polygon=loop;
        // Ear selection uses corners. Restore all source-edge subdivisions below
        // so collinear GM triangles cannot stall triangulation or leave T-junctions.
        bool reduced=true;while(reduced&&polygon.size()>3){reduced=false;for(size_t n=0;n<polygon.size();++n){storm_geometry_work::checkpoint();size_t a=(n+polygon.size()-1)%polygon.size(),b=(n+1)%polygon.size();if(std::abs(cross2(polygon[a].position,polygon[n].position,polygon[b].position,p.normal))<1.e-7f){polygon.erase(polygon.begin()+n);reduced=true;break;}}}
        while(polygon.size()>3){bool ear=false;for(size_t n=0;n<polygon.size();++n){storm_geometry_work::checkpoint();size_t a=(n+polygon.size()-1)%polygon.size(),c=(n+1)%polygon.size();const auto A=polygon[a].position,B=polygon[n].position,C=polygon[c].position;
            if(cross2(A,B,C,p.normal)<=1.e-8f)continue;bool contains=false;
            for(size_t j=0;j<polygon.size();++j)if(j!=a&&j!=n&&j!=c){storm_geometry_work::checkpoint();auto q=polygon[j].position;if(cross2(A,B,q,p.normal)>=-1.e-8f&&cross2(B,C,q,p.normal)>=-1.e-8f&&cross2(C,A,q,p.normal)>=-1.e-8f){contains=true;break;}}
            if(contains)continue;triangles.push_back({polygon[a],polygon[n],polygon[c]});polygon.erase(polygon.begin()+n);ear=true;break;}
            if(!ear){triangles.clear();break;}}
        if(polygon.size()==3)triangles.push_back({polygon[0],polygon[1],polygon[2]});
        for(const auto&point:loop){const size_t count=triangles.size();for(size_t n=0;n<count;++n)for(unsigned edge=0;edge<3;++edge){storm_geometry_work::checkpoint();auto triangle=triangles[n];const unsigned b=(edge+1)%3,c=(edge+2)%3;auto aPos=triangle[edge].position,bPos=triangle[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;
            auto line=bPos-aPos,offset=point.position-aPos;const float length2=storm_ship_hull::dot(line,line),along=storm_ship_hull::dot(offset,line);if(along<=0||along>=length2||storm_ship_hull::length(storm_ship_hull::cross(line,offset))>1.e-5f*std::sqrt(length2))continue;
            triangles[n]={triangle[edge],point,triangle[c]};triangles.push_back({point,triangle[b],triangle[c]});break;}}
        for(auto triangle:triangles){kept[object].push_back(triangle);std::swap(triangle[1],triangle[2]);for(auto&v:triangle)v.normal=p.normal*-1.f;detached[object].push_back(triangle);}
    }
}
// Render timber contours use polygon holes; canonical outer-mesh caps retain
// their established policy. Unknown materials are independent authored solids.
inline void cap(std::vector<Segment> segments,const Plane&p,Mesh&kept,Mesh&detached,const std::map<int,float>*wood=nullptr) {
    if(!wood){capLegacy(std::move(segments),p,kept,detached);return;}
    std::vector<Segment>timber;std::map<int,std::vector<Segment>>other;
    for(auto&edge:segments)(wood->count(edge.object)?timber:other[edge.object]).push_back(edge);
    for(auto&entry:other)capLegacy(std::move(entry.second),p,kept,detached);
    using Point=std::array<double,2>;using Edge=std::pair<PositionKey,PositionKey>;
    std::map<PositionKey,std::vector<size_t>>adjacent;std::map<Edge,size_t>unique;
    std::vector<Segment>edges;
    for(const auto&edge:timber){auto a=positionKey(edge.a.position),b=positionKey(edge.b.position);if(a==b)continue;auto ordered=std::minmax(a,b);if(!unique.emplace(Edge{ordered.first,ordered.second},edges.size()).second)continue;adjacent[a].push_back(edges.size());adjacent[b].push_back(edges.size());edges.push_back(edge);}
    const auto basis=storm_ship_hull::frame(p.normal);
    const auto project=[&](Vec3 v){return Point{double(storm_ship_hull::dot(v,basis.tangent)),double(storm_ship_hull::dot(v,basis.bitangent))};};
    struct Ring {std::vector<Vertex>vertices;std::vector<Point>points;double area=0;int object=0,parent=-1,depth=0;};std::vector<Ring>rings;std::vector<bool>used(edges.size());
    for(size_t start=0;start<edges.size();++start){if(used[start])continue;Ring ring;ring.object=edges[start].object;auto first=positionKey(edges[start].a.position),current=first;size_t next=start;bool closed=false;
        for(size_t steps=0;steps<=edges.size();++steps){if(used[next])break;used[next]=true;const auto&e=edges[next];const bool forward=positionKey(e.a.position)==current;ring.vertices.push_back(forward?e.a:e.b);current=positionKey((forward?e.b:e.a).position);if(current==first){closed=true;break;}const auto found=adjacent.find(current);if(found==adjacent.end()||found->second.size()!=2)break;next=used[found->second[0]]?found->second[1]:found->second[0];}
        if(!closed||ring.vertices.size()<3)continue;
        for(const auto&v:ring.vertices)ring.points.push_back(project(v.position));
        for(size_t n=0;n<ring.points.size();++n){const auto a=ring.points[n],b=ring.points[(n+1)%ring.points.size()];ring.area+=a[0]*b[1]-b[0]*a[1];}ring.area=std::abs(ring.area)*.5;if(ring.area>1.e-8)rings.push_back(std::move(ring));
    }
    const auto contains=[](const std::vector<Point>&ring,Point q){bool inside=false;for(size_t n=0,j=ring.size()-1;n<ring.size();j=n++){auto a=ring[n],b=ring[j];if((a[1]>q[1])!=(b[1]>q[1])&&q[0]<(b[0]-a[0])*(q[1]-a[1])/(b[1]-a[1])+a[0])inside=!inside;}return inside;};
    for(size_t n=0;n<rings.size();++n){double enclosing=std::numeric_limits<double>::infinity();for(size_t j=0;j<rings.size();++j)if(j!=n&&rings[j].area>rings[n].area&&rings[j].area<enclosing&&contains(rings[j].points,rings[n].points.front())){rings[n].parent=int(j);enclosing=rings[j].area;}}
    for(auto&ring:rings)for(int parent=ring.parent;parent>=0;parent=rings[parent].parent)++ring.depth;
    for(size_t n=0;n<rings.size();++n){if(rings[n].depth%2)continue;std::vector<std::vector<Point>>polygon;std::vector<Vertex>vertices;double expected=0;
        const auto append=[&](const Ring&ring,bool hole){polygon.push_back(ring.points);expected+=hole?-ring.area:ring.area;vertices.insert(vertices.end(),ring.vertices.begin(),ring.vertices.end());};append(rings[n],false);
        for(size_t j=0;j<rings.size();++j)if(rings[j].parent==int(n)&&rings[j].depth%2)append(rings[j],true);
        storm_geometry_work::checkpoint(true);auto indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);std::vector<Triangle>triangles;double measured=0;
        for(size_t k=0;k+2<indices.size();k+=3){Triangle t={vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]};if(cross2(t[0].position,t[1].position,t[2].position,p.normal)<0)std::swap(t[1],t[2]);const double a=storm_ship_hull::area(t);if(a>1.e-10){measured+=a;triangles.push_back(t);}}
        // Invalid authored/self-crossing rings must not fill a hold or port.
        if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4))continue;
        // Restore collinear source subdivisions omitted by earcut. This keeps
        // the cap conforming to the original surface on the next saved cut.
        for(const auto&point:vertices){const size_t count=triangles.size();for(size_t k=0;k<count;++k)for(unsigned edge=0;edge<3;++edge){storm_geometry_work::checkpoint();auto t=triangles[k];const unsigned b=(edge+1)%3,c=(edge+2)%3;const auto aPos=t[edge].position,bPos=t[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;const auto line=bPos-aPos,delta=point.position-aPos;const float length2=storm_ship_hull::dot(line,line),along=storm_ship_hull::dot(delta,line);if(along<=0||along>=length2||storm_ship_hull::length(storm_ship_hull::cross(line,delta))>1.e-5f*std::sqrt(length2))continue;triangles[k]={t[edge],point,t[c]};triangles.push_back({point,t[b],t[c]});break;}}
        for(auto t:triangles){for(auto&v:t){v.normal=p.normal;if(p.charred)v.color=(v.color&0xff000000u)|0x00645140u;for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=storm_ship_hull::dot(v.position,basis.tangent)*.12f;v.uv[uv*2+1]=storm_ship_hull::dot(v.position,basis.bitangent)*.12f;}}kept[rings[n].object].push_back(t);std::swap(t[1],t[2]);for(auto&v:t)v.normal=p.normal*-1.f;detached[rings[n].object].push_back(t);}
    }
}

// Shared edges may be traversed in opposite directions. Use one endpoint order
// for their intersections so repeated fractures do not open float-sized cracks.
inline double planarSide(const storm_ship_hull::Plane&p,Vec3 v){
    const double d=double(p.normal.x)*v.x+double(p.normal.y)*v.y+double(p.normal.z)*v.z-p.distance;
    return std::abs(d)<=PlanarUnit?0.:d;
}
inline void linearSplit(const std::vector<Vertex>&polygon,const storm_ship_hull::Plane&p,std::vector<Vertex>&inside,std::vector<Vertex>&outside) {storm_geometry_work::checkpoint();
    inside.clear();outside.clear();
    const auto side=[&](Vec3 v){return planarSide(p,v);};
    const auto before=[](Vec3 a,Vec3 b){return a.x<b.x||(a.x==b.x&&(a.y<b.y||(a.y==b.y&&a.z<b.z)));};
    for(size_t n=0;n<polygon.size();++n){storm_geometry_work::checkpoint();const auto&a=polygon[n];const auto&b=polygon[(n+1)%polygon.size()];const double da=side(a.position),db=side(b.position);const bool ai=da<=0,bi=db<=0;(ai?inside:outside).push_back(a);
        if(ai!=bi){const auto&first=before(a.position,b.position)?a:b;const auto&last=before(a.position,b.position)?b:a;const double d=side(first.position);auto vertex=storm_ship_hull::interpolate(first,last,float(std::clamp(d/(d-side(last.position)),0.,1.)));inside.push_back(vertex);outside.push_back(vertex);}}
}
inline Split planarSplit(const Mesh&mesh,const Plane&p,bool capped=true,bool stable=false,const std::map<int,float>*wood=nullptr) {
    Split result;std::vector<Segment>segments;
    for(const auto&o:mesh){result.kept.try_emplace(o.first);result.detached.try_emplace(o.first);
        for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<Vertex>inside,outside;if(stable)linearSplit(std::vector<Vertex>(t.begin(),t.end()),{p.normal,p.distance},inside,outside);else storm_ship_hull::split(std::vector<Vertex>(t.begin(),t.end()),{p.normal,p.distance},inside,outside);
            bool positive=false,negative=false;for(const auto&v:t){positive|=distance(p,v.position)>1.e-6f;negative|=distance(p,v.position)<-1.e-6f;}
            size_t first=result.kept[o.first].size();charBand(inside,p,true,p.charred&&positive&&negative,result.kept[o.first]);for(size_t n=first;n<result.kept[o.first].size();++n)result.keptArea+=storm_ship_hull::area(result.kept[o.first][n]);
            first=result.detached[o.first].size();charBand(outside,p,false,p.charred&&positive&&negative,result.detached[o.first]);for(size_t n=first;n<result.detached[o.first].size();++n)result.detachedArea+=storm_ship_hull::area(result.detached[o.first][n]);
            // A subsequent plane can coincide with a previously triangulated
            // edge. Its negative-side triangle still contributes that seam.
            if(!negative)continue;std::vector<Vertex>edge;for(const auto&v:inside)if(std::abs(distance(p,v.position))<1.e-4f&&(edge.empty()||!near(edge.back().position,v.position)))edge.push_back(v);
            if(edge.size()>=2&&!near(edge[0].position,edge[1].position))segments.push_back({edge[0],edge[1],o.first});
        }
    }
    if(result.detachedArea<=1.e-7f)return result;
    result.seam=segments;if(capped)cap(std::move(segments),p,result.kept,result.detached,wood);return result;
}
// A piecewise-linear shear has determinant one because grain is perpendicular
// to normal. Refinement at its knots makes planar clipping exact, without moving
// the original hull surface. Refine neighboring triangles too: otherwise a knot
// on a shared edge would leave a T-junction outside the actual fracture slab.
inline bool refine(const Triangle&t,const Plane&p,std::vector<Triangle>&out,size_t&count) {
    constexpr size_t budget=262144;constexpr double maxKnots=256;
    double low=storm_ship_hull::dot(p.grain,t[0].position),high=low;
    for(const auto&v:t){if(!storm_ship_hull::finite(v.position))return false;const double q=storm_ship_hull::dot(p.grain,v.position);low=std::min(low,q);high=std::max(high,q);}
    const double first=std::floor((low+p.phase)/p.spacing)+1,last=std::ceil((high+p.phase)/p.spacing)-1;
    if(!std::isfinite(first)||!std::isfinite(last)||last-first+1>maxKnots||std::abs(first)>1000000000.||std::abs(last)>1000000000.)return false;
    std::vector<Vertex>remaining(t.begin(),t.end()),inside,outside;
    const auto append=[&](const std::vector<Vertex>&polygon){const size_t before=out.size();storm_ship_hull::fan(polygon,out);count+=out.size()-before;return count<=budget;};
    for(double cell=first;cell<=last;++cell){const float knot=float(cell*p.spacing-p.phase);linearSplit(remaining,{p.grain,knot},inside,outside);if(!append(inside))return false;remaining.swap(outside);}
    return append(remaining);
}
// Only the conservative fracture slab needs grain knots. Share knots through
// original geometric edges so neighboring far faces remain conforming without
// refining every cannon, rope and distant hull triangle on every ram.
inline bool refineNearCut(const Mesh&mesh,const Plane&p,Mesh&out,size_t&count) {
    for(const auto&o:mesh)out.try_emplace(o.first);
    std::unordered_map<PositionEdge,std::vector<float>,PositionEdgeHash>edgeKnots;
    std::vector<bool>active;size_t face=0;
    const auto grain=[&](Vec3 v){return double(p.grain.x)*v.x+double(p.grain.y)*v.y+double(p.grain.z)*v.z;};
    for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();float lo=std::numeric_limits<float>::infinity(),hi=-lo;for(const auto&v:t){const float d=storm_ship_hull::dot(p.normal,v.position)-p.distance;lo=std::min(lo,d);hi=std::max(hi,d);}const bool local=lo<=p.roughness&&hi>=-p.roughness;active.push_back(local);if(!local)continue;
        for(unsigned c=0;c<3;++c){auto a=positionKey(t[c].position),b=positionKey(t[(c+1)%3].position);if(a==b)continue;if(b<a)std::swap(a,b);auto inserted=edgeKnots.try_emplace({a,b});if(!inserted.second)continue;double low=grain(t[c].position),high=grain(t[(c+1)%3].position);if(high<low)std::swap(low,high);const double first=std::floor((low+p.phase)/p.spacing)+1,last=std::ceil((high+p.phase)/p.spacing)-1;if(last-first+1>256)return false;for(double cell=first;cell<=last;++cell)inserted.first->second.push_back(float(cell*p.spacing-p.phase));}}
    for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();auto&destination=out[o.first];if(active[face++]){if(!refine(t,p,destination,count))return false;continue;}
        bool neighboring=false;for(unsigned c=0;c<3;++c){auto a=positionKey(t[c].position),b=positionKey(t[(c+1)%3].position);if(b<a)std::swap(a,b);auto found=edgeKnots.find({a,b});neighboring|=found!=edgeKnots.end()&&!found->second.empty();}if(!neighboring){destination.push_back(t);if(++count>262144)return false;continue;}
        std::vector<Vertex>boundary;bool divided=false;
        for(unsigned c=0;c<3;++c){const auto&a=t[c];const auto&b=t[(c+1)%3];boundary.push_back(a);auto ka=positionKey(a.position),kb=positionKey(b.position);if(kb<ka)std::swap(ka,kb);auto found=edgeKnots.find({ka,kb});if(found==edgeKnots.end()||found->second.empty())continue;
            const bool before=a.position.x<b.position.x||(a.position.x==b.position.x&&(a.position.y<b.position.y||(a.position.y==b.position.y&&a.position.z<b.position.z)));const auto&first=before?a:b;const auto&last=before?b:a;const double fa=grain(first.position),fb=grain(last.position);std::vector<std::pair<double,Vertex>>points;
            for(float knot:found->second){const double t=(fa-knot)/(fa-fb);if(t<=0||t>=1)continue;auto v=storm_ship_hull::interpolate(first,last,float(t));points.push_back({before?t:1-t,v});}std::sort(points.begin(),points.end(),[](const auto&a,const auto&b){return a.first<b.first;});for(const auto&point:points)boundary.push_back(point.second);divided|=!points.empty();}
        if(!divided){destination.push_back(t);++count;}else{const auto center=storm_ship_hull::interpolate(storm_ship_hull::interpolate(t[0],t[1],.5f),t[2],1.f/3.f);for(size_t n=0;n<boundary.size();++n){Triangle face={center,boundary[n],boundary[(n+1)%boundary.size()]};if(storm_ship_hull::area(face)>1.e-10){destination.push_back(face);++count;}}}if(count>262144)return false;
    }return true;
}

inline Split split(const Mesh&mesh,const Plane&p,const std::map<int,float>*wood=nullptr,bool capped=true) {
    const auto unchanged=[&](bool complete=false){Split result;result.complete=complete;result.kept=mesh;for(const auto&o:mesh)for(const auto&t:o.second)result.keptArea+=storm_ship_hull::area(t);return result;};
    if(!valid(p))return unchanged();
    if(p.roughness==0)return planarSplit(mesh,p,capped,false,wood);
    Mesh refined,warped,farKept,farDetached;size_t count=0;
    if(!refineNearCut(mesh,p,refined,count))return unchanged();
    for(auto&o:refined)for(auto&t:o.second){float low=std::numeric_limits<float>::infinity(),high=-low;for(const auto&v:t){const float d=storm_ship_hull::dot(p.normal,v.position)-p.distance;low=std::min(low,d);high=std::max(high,d);}if(high<-p.roughness)farKept[o.first].push_back(std::move(t));else if(low>p.roughness)farDetached[o.first].push_back(std::move(t));else{for(auto&v:t)v.position=v.position-p.normal*offset(p,v.position);warped[o.first].push_back(std::move(t));}}
    const Plane flat{p.normal,p.distance,false};auto result=planarSplit(warped,flat,false,true);
    // An empty object suppresses its original GM draw. Keep that override even
    // when the object has no near-slab faces or was removed by an earlier cut.
    for(const auto&o:mesh){result.kept.try_emplace(o.first);result.detached.try_emplace(o.first);}
    if(result.seam.size()>4096)return unchanged();
    Mesh keptCap,detachedCap;if(capped)cap(result.seam,flat,keptCap,detachedCap,wood);
    const auto restore=[&](Mesh&m){for(auto&o:m)for(auto&t:o.second)for(auto&v:t)v.position=v.position+p.normal*offset(p,v.position);};
    restore(result.kept);restore(result.detached);
    for(auto&o:farKept){auto&out=result.kept[o.first];out.insert(out.end(),std::make_move_iterator(o.second.begin()),std::make_move_iterator(o.second.end()));}
    for(auto&o:farDetached){auto&out=result.detached[o.first];out.insert(out.end(),std::make_move_iterator(o.second.begin()),std::make_move_iterator(o.second.end()));}
    result.keptArea=result.detachedArea=0;
    for(const auto&o:result.kept)for(const auto&t:o.second)result.keptArea+=storm_ship_hull::area(t);
    for(const auto&o:result.detached)for(const auto&t:o.second)result.detachedArea+=storm_ship_hull::area(t);
    if(result.detachedArea<=1.e-7f)return unchanged(true);
    const auto appendCap=[&](const Mesh&caps,Mesh&destination){for(const auto&o:caps)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<Triangle>triangles;if(!refine(t,p,triangles,count))return false;
        for(auto triangle:triangles){for(auto&v:triangle)v.position=v.position+p.normal*offset(p,v.position);const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position));for(auto&v:triangle)v.normal=normal;destination[o.first].push_back(triangle);}}return true;};
    if(!appendCap(keptCap,result.kept)||!appendCap(detachedCap,result.detached))return unchanged();
    size_t total=0;for(const auto*piece:{&result.kept,&result.detached})for(const auto&o:*piece){total+=o.second.size();if(total>262144)return unchanged();}
    for(auto&edge:result.seam){edge.a.position=edge.a.position+p.normal*offset(p,edge.a.position);edge.b.position=edge.b.position+p.normal*offset(p,edge.b.position);}
    const double originalVolume=capped?closedVolume(mesh):0;
    if(originalVolume>0){const double keptVolume=closedVolume(result.kept),detachedVolume=closedVolume(result.detached);if((result.keptArea>1.e-7f&&keptVolume<=0)||(result.detachedArea>1.e-7f&&detachedVolume<=0)||std::abs(keptVolume+detachedVolume-originalVolume)>std::max(.001,originalVolume*1.e-5))return unchanged();}
    return result;
}
// The outer authored surface is shared by damage/collision and rendering.
// Caps and timber interiors have separate policies; tagged clipping visits their
// union once rather than independently clipping two complete ship meshes.
struct FaceIndex {storm_ship_hull::Broadphase broadphase;std::vector<std::pair<int,uint32_t>>references;Bounds bounds{};};
inline FaceIndex indexFaces(const Mesh&mesh){FaceIndex result;std::vector<Bounds>boxes;for(const auto&o:mesh)for(size_t n=0;n<o.second.size();++n){storm_geometry_work::checkpoint();const auto&t=o.second[n];Bounds box{t[0].position,t[0].position};for(unsigned c=1;c<3;++c)box=storm_ship_hull::merge(box,{t[c].position,t[c].position});boxes.push_back(box);result.references.emplace_back(o.first,uint32_t(n));}result.bounds=bounds(mesh);result.broadphase.build(boxes);return result;}
struct SurfaceBase {Mesh outer,canonicalCaps,interior,finiteWalls;FaceIndex index;};
struct MaterialPrism {std::array<Vec3,6>vertices;int object=0;};
struct MaterialCellSource {std::vector<std::shared_ptr<const Mesh>>solids;std::vector<Bounds>bounds;storm_ship_hull::Broadphase index;};
struct MaterialCells {std::shared_ptr<const MaterialCellSource>source;std::map<uint32_t,std::shared_ptr<const Mesh>>changed;};
inline const Mesh& materialCell(const MaterialCells&cells,uint32_t id){const auto found=cells.changed.find(id);return found==cells.changed.end()?*cells.source->solids.at(id):*found->second;}
bool timberSectionCaps(const std::vector<MaterialPrism>&,const std::vector<Plane>&,const storm_ship_hull::Plane&,const std::vector<storm_ship_hull::Plane>&,Vec3,bool,Mesh&,Mesh&);
struct Surface {Mesh outer,canonicalCaps,interior,finiteWalls;std::shared_ptr<const std::vector<Mesh>>material;std::shared_ptr<const MaterialCells>cells;std::shared_ptr<const SurfaceBase>original,supportOriginal;std::shared_ptr<const storm_ship_structure::State>structure;std::vector<BeamSample>cutters;std::vector<std::shared_ptr<const Mesh>>released;std::shared_ptr<const std::vector<MaterialPrism>>timber;std::vector<Plane>legacy;bool checkpointed=false;};
struct SupportSpan {Vec3 base{},tip{};};
struct Input {std::shared_ptr<const Surface>surface;std::shared_ptr<const storm_ship_structure::State>structure;std::map<int,float>wood;Plane plane;unsigned fragmentLimit=0,materialTailCount=0;SupportSpan support{};bool hasSupport=false,enclosedFragments=true,structuralFragmentsOnly=false,physicalContact=true;double closedVolume=0,structuralArea=0;uint64_t generation=0,revision=0,materialRevision=0,fingerprint=0;};
struct Projection {std::shared_ptr<const Surface>kept;std::shared_ptr<const storm_ship_structure::State>structure;Mesh canonical,render;std::set<int>changedObjects;std::vector<Mesh>fragmentRender;Result result;FaceIndex collision,materialCollision,retainedSupport;std::vector<uint32_t>materialCandidates,supportCandidates,supportNearby;std::vector<FaceIndex>releasedSupport;Plane plane;double keptVolume=0;uint64_t generation=0,revision=0,materialRevision=0;bool enclosedFragments=true;std::string materialPayload;};
struct FragmentInput {Input input;uint64_t owner=0,generation=0,token=0;unsigned successorLimit=0;};
struct FragmentPiece {Fragment properties;std::shared_ptr<const Surface>surface;FaceIndex collision;};
struct FragmentProjection {uint64_t owner=0,generation=0,token=0;bool changed=false;std::vector<FragmentPiece>successors;};
inline bool prepare(const Input&,Projection&);
inline void appendMesh(Mesh&destination,const Mesh&source){for(const auto&o:source){auto&out=destination[o.first];out.insert(out.end(),o.second.begin(),o.second.end());}}
inline Mesh canonicalMesh(const Surface&surface){Mesh result=surface.outer;appendMesh(result,surface.canonicalCaps);appendMesh(result,surface.finiteWalls);return result;}
inline Mesh renderMesh(const Surface&surface){Mesh result=surface.outer;appendMesh(result,surface.interior);appendMesh(result,surface.finiteWalls);return result;}
inline Bounds beamBounds(const BeamSample&beam){
    const auto start=beamStart(beam),direction=beamDirection(beam),tangent=beamTangent(beam),bitangent=storm_ship_hull::cross(direction,tangent);
    Bounds result{start,start};const float back=beam.backDepth<0?beam.radius*.15625f:beam.backDepth;
    for(float depth:{-back,beam.depth})for(unsigned n=0;n<(beam.crossSection.empty()?8u:unsigned(beam.crossSection.size()));++n){const float angle=.3926990816987241f+float(n)*.7853981633974483f,corner=beam.radius/.9238795325112867f;
        const auto xy=beam.crossSection.empty()?std::array<float,2>{corner*std::cos(angle),corner*std::sin(angle)}:beam.crossSection[n];
        const auto p=start+direction*depth+tangent*xy[0]+bitangent*xy[1];result=storm_ship_hull::merge(result,{p,p});}return result;
}
inline Surface timberSurface(Mesh mesh,const std::map<int,float>&wood){Surface result;auto rendered=shellForRender(mesh,wood);std::vector<MaterialPrism>prisms;for(auto&o:rendered){const auto&source=mesh.at(o.first);const size_t original=source.size();if(o.second.size()>original){result.interior[o.first].assign(o.second.begin()+original,o.second.end());for(size_t n=0;n<original;++n){MaterialPrism prism;prism.object=o.first;for(unsigned c=0;c<3;++c){prism.vertices[c]=source[n][c].position;prism.vertices[c+3]=o.second[original+n][c==0?0:3-c].position;}prisms.push_back(prism);}}}if(!prisms.empty())result.timber=std::make_shared<const std::vector<MaterialPrism>>(std::move(prisms));result.outer=std::move(mesh);return result;}
inline bool surfaceCaps(std::vector<Segment>seam,const Plane&p,Mesh&kept,Mesh&detached,const std::map<int,float>*wood=nullptr){
    if(p.roughness==0){cap(std::move(seam),p,kept,detached,wood);return true;}
    for(auto&e:seam){e.a.position=e.a.position-p.normal*offset(p,e.a.position);e.b.position=e.b.position-p.normal*offset(p,e.b.position);}
    Mesh a,b;cap(std::move(seam),{p.normal,p.distance,false},a,b,wood);size_t count=0;
    const auto append=[&](const Mesh&source,Mesh&destination){for(const auto&o:source)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<Triangle>triangles;if(!refine(t,p,triangles,count))return false;for(auto triangle:triangles){for(auto&v:triangle)v.position=v.position+p.normal*offset(p,v.position);const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position));for(auto&v:triangle)v.normal=normal;destination[o.first].push_back(triangle);}}return true;};
    return append(a,kept)&&append(b,detached);
}
struct SurfaceSplit {Surface kept,detached;float keptArea=0,detachedArea=0;std::vector<Segment>seam;std::vector<double>detachedVolumes;bool complete=true;};
// A finite cutter intersects real material cross-sections. Nested contours of
// one authored object describe a cavity, not two independent filled disks.
inline bool finiteSurfaceCaps(const std::vector<Segment>&seam,const Plane&p,Mesh&kept,Mesh&detached,const std::map<int,float>*wood=nullptr){
    std::vector<Segment>timber;std::map<int,std::vector<Segment>>objects;
    for(const auto&edge:seam)(wood&&wood->count(edge.object)?timber:objects[edge.object]).push_back(edge);
    if(!timber.empty()&&!surfaceCaps(std::move(timber),p,kept,detached,wood))return false;
    for(auto&entry:objects){const std::map<int,float>solid{{entry.first,1.f}};if(!surfaceCaps(std::move(entry.second),p,kept,detached,&solid))return false;}return true;
}
inline bool subtractFaces(const Triangle&t,const std::vector<std::vector<Vertex>>&removed,std::vector<Triangle>&outside,bool authored=false){
    if(storm_ship_hull::area(t)<=1.e-10f){outside.push_back(t);return true;}
    const auto origin=t[0].position;std::array<double,3>u{double(t[1].position.x)-origin.x,double(t[1].position.y)-origin.y,double(t[1].position.z)-origin.z},w{double(t[2].position.x)-origin.x,double(t[2].position.y)-origin.y,double(t[2].position.z)-origin.z};
    std::array<double,3>n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};const double ul=std::sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]),nl=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(nl<1.e-14||ul<1.e-14){outside.push_back(t);return true;}for(unsigned k=0;k<3;++k){u[k]/=ul;n[k]/=nl;}const std::array<double,3>v{n[1]*u[2]-n[2]*u[1],n[2]*u[0]-n[0]*u[2],n[0]*u[1]-n[1]*u[0]};const Vec3 normal{float(n[0]),float(n[1]),float(n[2])};
    // Boolean coordinates belong to the source triangle. Its original edges
    // are exactly the unit simplex, avoiding rounded metric diagonal ribbons.
    const auto project=[&](Vec3 point){const std::array<double,3>d{double(point.x)-origin.x,double(point.y)-origin.y,double(point.z)-origin.z};const double x=d[0]*u[0]+d[1]*u[1]+d[2]*u[2],y=d[0]*v[0]+d[1]*v[1]+d[2]*v[2];if(!authored)return FacePoint{x,y};const double wx=w[0]*u[0]+w[1]*u[1]+w[2]*u[2],wy=w[0]*v[0]+w[1]*v[1]+w[2]*v[2];double c=y/wy,b=(x-c*wx)/ul;
        // Float intersections on a skinny source triangle amplify metric
        // rounding in barycentric space. Preserve its exact simplex edges at
        // the boolean's metric grid, including b+c=1 after integer rounding.
        if(std::abs(b)*nl/std::hypot(wx,wy)<PlanarUnit)b=0;
        if(std::abs(c)*nl/ul<PlanarUnit)c=0;
        if(std::abs(1-b-c)*nl/std::hypot(wx-ul,wy)<PlanarUnit){const auto q=std::llround(b*PlanarUnitsPerMeter);b=double(q)/PlanarUnitsPerMeter;c=double(PlanarUnitsPerMeter-q)/PlanarUnitsPerMeter;}
        return FacePoint{b,c};};
    FaceRing triangle;std::vector<FaceRing>clips;std::vector<std::pair<FacePoint,Vertex>>clipVertices;for(const auto&point:t)triangle.push_back(project(point.position));for(const auto&ring:removed){FaceRing clip;for(const auto&point:ring){const auto p=project(point.position);clip.push_back(p);clipVertices.push_back({p,point});}clips.push_back(std::move(clip));}
    std::vector<FacePolygon>polygons;if(!differenceFaces(triangle,clips,polygons))return false;
    const double denominator=triangle[1][0]*triangle[2][1]-triangle[1][1]*triangle[2][0];if(std::abs(denominator)<1.e-14)return false;
    const auto vertex=[&](FacePoint point){for(unsigned n=0;n<3;++n)if(std::hypot(point[0]-triangle[n][0],point[1]-triangle[n][1])<1.e-6)return t[n];
        for(const auto&clip:clipVertices)if(std::hypot(point[0]-clip.first[0],point[1]-clip.first[1])<1.e-6)return clip.second;
        const float b=float((point[0]*triangle[2][1]-point[1]*triangle[2][0])/denominator),c=float((triangle[1][0]*point[1]-triangle[1][1]*point[0])/denominator);
        Vertex result=t[0];const float a=1-b-c;result.position=t[0].position*a+t[1].position*b+t[2].position*c;result.normal=t[0].normal*a+t[1].normal*b+t[2].normal*c;
        for(unsigned n=0;n<8;++n)result.uv[n]=t[0].uv[n]*a+t[1].uv[n]*b+t[2].uv[n]*c;result.color=0;for(unsigned n=0;n<4;++n){float value=float((t[0].color>>(n*8))&255u)*a+float((t[1].color>>(n*8))&255u)*b+float((t[2].color>>(n*8))&255u)*c;result.color|=uint32_t(std::clamp(std::lround(value),0l,255l))<<(n*8);}return result;};
    for(const auto&polygon:polygons){storm_geometry_work::checkpoint();std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&point:ring)vertices.push_back(vertex(point));storm_geometry_work::checkpoint(true);auto indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);
        std::vector<Triangle>faces;for(size_t n=0;n+2<indices.size();n+=3){Triangle face{vertices[indices[n]],vertices[indices[n+1]],vertices[indices[n+2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(face[1].position-face[0].position,face[2].position-face[0].position),normal)<0)std::swap(face[1],face[2]);if(storm_ship_hull::area(face)>1.e-10f)faces.push_back(face);}
        if(!authored)for(const auto&point:vertices){const size_t count=faces.size();for(size_t n=0;n<count;++n)for(unsigned edge=0;edge<3;++edge){storm_geometry_work::checkpoint();auto face=faces[n];const unsigned b=(edge+1)%3,c=(edge+2)%3;const auto aPos=face[edge].position,bPos=face[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;const auto line=bPos-aPos,delta=point.position-aPos;const float length2=storm_ship_hull::dot(line,line),along=storm_ship_hull::dot(delta,line);if(along<=0||along>=length2||storm_ship_hull::length(storm_ship_hull::cross(line,delta))>1.e-6f*std::sqrt(length2))continue;faces[n]={face[edge],point,face[c]};faces.push_back({point,face[b],face[c]});break;}}
        for(const auto&face:faces)if(topologyTriangle(face))outside.push_back(face);}
    return true;
}
inline bool subtractFace(const Triangle&t,const std::vector<Vertex>&removed,std::vector<Triangle>&outside){return subtractFaces(t,{removed},outside);}
inline bool clipOriginalMesh(const Mesh&original,const std::vector<BeamSample>&cutters,Mesh&result,const std::vector<std::shared_ptr<const Mesh>>&released={}){
    std::vector<BeamPlanes>volumes;for(const auto&beam:cutters)volumes.push_back(beamPlanes(beam));Mesh out;
    for(const auto&o:original){auto&destination=out[o.first];for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<std::vector<Vertex>>removed;
        for(const auto&faces:volumes){bool far=false;for(const auto&face:faces){bool beyond=true;for(const auto&v:t)beyond&=planarSide(face,v.position)>0;if(beyond){far=true;break;}}if(far)continue;std::vector<Vertex>remaining(t.begin(),t.end()),a,b;for(const auto&face:faces){linearSplit(remaining,face,a,b);remaining.swap(a);if(remaining.size()<3)break;}std::vector<Triangle>area;storm_ship_hull::fan(remaining,area);if(!area.empty())removed.push_back(std::move(remaining));}
        if(!released.empty()){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));for(const auto&piece:released){auto found=piece->find(o.first);if(found==piece->end())continue;for(const auto&face:found->second){storm_geometry_work::checkpoint();bool coplanar=true;for(const auto&v:face)coplanar&=std::abs(storm_ship_hull::dot(normal,v.position-t[0].position))<1.e-5f;if(coplanar)removed.emplace_back(face.begin(),face.end());}}}
        if(removed.empty())destination.push_back(t);else if(!subtractFaces(t,removed,destination,true))return false;}}
    result=std::move(out);return true;
}
inline std::vector<Mesh> materialSolids(const Surface&surface,const std::set<int>&affected,const std::map<int,float>&wood){
    std::vector<Mesh>result;Mesh authored;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.finiteWalls})for(const auto&o:*mesh)if(!wood.count(o.first))appendMesh(authored,Mesh{{o.first,o.second}});
    std::map<std::vector<int>,Mesh>groups;for(auto&solid:components(authored)){bool touched=false;std::vector<int>objects;for(const auto&o:solid){touched|=affected.count(o.first)>0;objects.push_back(o.first);}if(touched&&closedCutVolume(solid)>0)appendMesh(groups[objects],solid);}for(auto&group:groups)result.push_back(std::move(group.second));return result;
}
template<class Planes> inline bool subtractVolume(const Mesh&mesh,const Planes&faces,Mesh&outside,Mesh&inside,std::set<int>*affected=nullptr){
    for(const auto&o:mesh){outside.try_emplace(o.first);for(const auto&t:o.second){storm_geometry_work::checkpoint();const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));bool far=false;for(const auto&face:faces){bool beyond=true,coplanar=true;for(const auto&v:t){const auto side=planarSide(face,v.position);beyond&=side>0;coplanar&=side==0;}
        // A coincident wall whose wood lies outside this cutter remains a
        // boundary. An identical empty bore must not erase its old walls.
        if(beyond||(coplanar&&storm_ship_hull::dot(normal,face.normal)<-.999f)){far=true;break;}}if(far){outside[o.first].push_back(t);continue;}
        std::vector<Vertex>remaining(t.begin(),t.end()),a,b;for(const auto&face:faces){linearSplit(remaining,face,a,b);remaining.swap(a);if(remaining.size()<3)break;}std::vector<Triangle>removed;storm_ship_hull::fan(remaining,removed);if(removed.empty()){outside[o.first].push_back(t);continue;}
        if(!subtractFace(t,remaining,outside[o.first]))return false;if(affected)affected->insert(o.first);auto&in=inside[o.first];in.insert(in.end(),removed.begin(),removed.end());}}
    return true;
}
inline bool solidSectionCaps(const Mesh&solid,const storm_ship_hull::Plane&plane,const std::vector<storm_ship_hull::Plane>&limits,Vec3 origin,bool charred,Mesh&inside,Mesh&outside){
    const auto box=bounds(solid);origin=(box.lo+box.hi)*.5f;
    origin=origin+plane.normal*(plane.distance-storm_ship_hull::dot(plane.normal,origin));
    const auto basis=storm_ship_hull::frame(plane.normal);std::vector<FaceRing>sections;
    const auto side=[&](Vec3 p){return planarSide(plane,p);};
    std::vector<FaceRing>coplanar;
    for(const auto&o:solid)for(const auto&t:o.second){storm_geometry_work::checkpoint();bool same=true;for(const auto&v:t)same&=side(v.position)==0;if(same){FaceRing ring;for(const auto&v:t){const auto p=v.position-origin;ring.push_back({double(storm_ship_hull::dot(p,basis.tangent)),double(storm_ship_hull::dot(p,basis.bitangent))});}coplanar.push_back(std::move(ring));}}
    for(const auto&o:solid)for(const auto&t:o.second){storm_geometry_work::checkpoint();const Vec3 a=t[0].position-origin,b=t[1].position-origin,c=t[2].position-origin;const double signedVolume=double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x);if(std::abs(signedVolume)<1.e-12)continue;
        const std::array<Vec3,4>tetra{{origin,t[0].position,t[1].position,t[2].position}};bool inward=false;for(const auto&p:tetra)inward|=side(p)<0;if(!inward)continue;bool far=false;for(const auto&limit:limits){bool beyond=true;for(const auto&p:tetra)beyond&=planarSide(limit,p)>0;if(beyond){far=true;break;}}if(far)continue;
        std::vector<Vertex>points;for(unsigned x=0;x<4;++x)for(unsigned y=x+1;y<4;++y){const auto first=tetra[x],last=tetra[y];const double da=side(first),db=side(last);if(da!=0&&db!=0&&(da<0)==(db<0))continue;
            const double along=da==0?0:db==0?1:da/(da-db);Vertex vertex;vertex.position={float(first.x+(double(last.x)-first.x)*along),float(first.y+(double(last.y)-first.y)*along),float(first.z+(double(last.z)-first.z)*along)};bool duplicate=false;for(const auto&point:points)duplicate|=positionKey(point.position)==positionKey(vertex.position);if(!duplicate)points.push_back(vertex);}
        if(points.size()<3)continue;Vec3 center{};for(const auto&point:points)center=center+point.position*(1.f/points.size());std::sort(points.begin(),points.end(),[&](const auto&a,const auto&b){return std::atan2(storm_ship_hull::dot(a.position-center,basis.bitangent),storm_ship_hull::dot(a.position-center,basis.tangent))<std::atan2(storm_ship_hull::dot(b.position-center,basis.bitangent),storm_ship_hull::dot(b.position-center,basis.tangent));});
        for(const auto&limit:limits){std::vector<Vertex>in,out;linearSplit(points,limit,in,out);points=std::move(in);if(points.size()<3)break;}if(points.size()<3)continue;FaceRing ring;for(const auto&point:points){const auto p=point.position-origin;ring.push_back({double(storm_ship_hull::dot(p,basis.tangent)),double(storm_ship_hull::dot(p,basis.bitangent))});}double area=0;for(size_t n=0;n<ring.size();++n)area+=ring[n][0]*ring[(n+1)%ring.size()][1]-ring[(n+1)%ring.size()][0]*ring[n][1];if(std::abs(area)<1.e-12)continue;if((area<0)!=(signedVolume<0))std::reverse(ring.begin(),ring.end());sections.push_back(std::move(ring));}
    std::vector<FacePolygon>polygons;if(!unionFaces(sections,polygons))return false;const int object=solid.begin()->first;
    // A section touching an existing boundary is not a newly exposed wall.
    // That face already went through subtractVolume; a second cap would make
    // duplicate/opposing skins when a later beam shares the first beam's side.
    if(!coplanar.empty()){std::vector<FacePolygon>trimmed;for(const auto&polygon:polygons){storm_geometry_work::checkpoint();auto clips=coplanar;clips.insert(clips.end(),polygon.begin()+1,polygon.end());
        for(auto&ring:clips){double area=0;for(size_t n=0;n<ring.size();++n)area+=ring[n][0]*ring[(n+1)%ring.size()][1]-ring[(n+1)%ring.size()][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());}
        std::vector<FacePolygon>part;if(!differenceFaces(polygon.front(),clips,part))return false;trimmed.insert(trimmed.end(),std::make_move_iterator(part.begin()),std::make_move_iterator(part.end()));}polygons=std::move(trimmed);}
    for(const auto&polygon:polygons){storm_geometry_work::checkpoint();std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&p:ring){Vertex v;v.position={float(origin.x+p[0]*basis.tangent.x+p[1]*basis.bitangent.x),float(origin.y+p[0]*basis.tangent.y+p[1]*basis.bitangent.y),float(origin.z+p[0]*basis.tangent.z+p[1]*basis.bitangent.z)};v.normal=plane.normal;v.color=charred?0xff645140u:solid.begin()->second.front()[0].color;for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=float(p[0])*.12f;v.uv[uv*2+1]=float(p[1])*.12f;}vertices.push_back(v);}
        storm_geometry_work::checkpoint(true);const auto indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);for(size_t n=0;n+2<indices.size();n+=3){Triangle t{vertices[indices[n]],vertices[indices[n+1]],vertices[indices[n+2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position),plane.normal)<0)std::swap(t[1],t[2]);if(storm_ship_hull::area(t)<=1.e-10f)continue;inside[object].push_back(t);std::swap(t[1],t[2]);for(auto&v:t)v.normal=plane.normal*-1.f;outside[object].push_back(t);}}
    return true;
}
inline bool compactFiniteWalls(Mesh&mesh,const std::vector<BeamSample>&cutters){
    std::vector<storm_ship_hull::Plane>planes;for(const auto&beam:cutters)for(const auto&face:beamPlanes(beam)){planes.push_back(face);planes.push_back({face.normal*-1.f,-face.distance});}
    std::map<std::pair<int,size_t>,std::vector<Triangle>>groups;Mesh result;for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();size_t best=planes.size();float error=.0001f;for(size_t n=0;n<planes.size();++n){const auto&p=planes[n];if(storm_ship_hull::length(p.normal-t[0].normal)>1.e-5f)continue;float distance=0;for(const auto&v:t)distance=std::max(distance,std::abs(storm_ship_hull::dot(p.normal,v.position)-p.distance));if(distance<error){best=n;error=distance;}}
        if(best==planes.size()){result[o.first].push_back(t);continue;}groups[{o.first,best}].push_back(t);}
    for(const auto&group:groups){const auto&plane=planes[group.first.second];const auto basis=storm_ship_hull::frame(plane.normal);const auto origin=plane.normal*plane.distance;std::vector<FaceRing>triangles;std::vector<std::vector<Vertex>>opposite;
        for(const auto&other:groups){const auto&candidate=planes[other.first.second];if(other.first.first==group.first.first&&storm_ship_hull::length(candidate.normal+plane.normal)<1.e-6f&&std::abs(candidate.distance+plane.distance)<1.e-6f)for(const auto&t:other.second)opposite.emplace_back(t.begin(),t.end());}
        std::vector<Triangle>faces;if(opposite.empty())faces=group.second;else for(const auto&t:group.second)if(!subtractFaces(t,opposite,faces))return false;
        for(const auto&t:faces){FaceRing ring;for(const auto&v:t){const auto p=v.position-origin;ring.push_back({double(storm_ship_hull::dot(p,basis.tangent)),double(storm_ship_hull::dot(p,basis.bitangent))});}triangles.push_back(std::move(ring));}
        std::vector<FacePolygon>polygons;if(!unionFaces(triangles,polygons))return false;for(const auto&polygon:polygons){storm_geometry_work::checkpoint();std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&p:ring){auto v=group.second.front()[0];v.position={float(origin.x+p[0]*basis.tangent.x+p[1]*basis.bitangent.x),float(origin.y+p[0]*basis.tangent.y+p[1]*basis.bitangent.y),float(origin.z+p[0]*basis.tangent.z+p[1]*basis.bitangent.z)};for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=float(p[0])*.12f;v.uv[uv*2+1]=float(p[1])*.12f;}vertices.push_back(v);}storm_geometry_work::checkpoint(true);const auto indices=mapbox::earcut<uint32_t>(polygon);storm_geometry_work::checkpoint(true);for(size_t n=0;n+2<indices.size();n+=3){Triangle t{vertices[indices[n]],vertices[indices[n+1]],vertices[indices[n+2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position),plane.normal)<0)std::swap(t[1],t[2]);if(storm_ship_hull::area(t)>1.e-10f)result[group.first.first].push_back(t);}}}
    mesh=std::move(result);return true;
}
inline std::vector<Segment> planeSeam(const Mesh&mesh,const storm_ship_hull::Plane&plane){
    std::vector<Segment> result;
    for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();float d[3];bool positive=false,negative=false;
        for(unsigned n=0;n<3;++n){d[n]=storm_ship_hull::dot(plane.normal,t[n].position)-plane.distance;positive|=d[n]>0;negative|=d[n]<=0;}
        if(!positive||!negative)continue;std::vector<Vertex>edge;
        for(unsigned n=0;n<3;++n){unsigned next=(n+1)%3;if((d[n]>0)==(d[next]>0))continue;
            auto v=storm_ship_hull::interpolate(t[n],t[next],d[n]/(d[n]-d[next]));if(edge.empty()||!near(edge.front().position,v.position))edge.push_back(v);}
        if(edge.size()!=2)continue;
        const auto face=storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position);
        if(storm_ship_hull::dot(edge[1].position-edge[0].position,storm_ship_hull::cross(plane.normal,face))<0)std::swap(edge[0],edge[1]);
        result.push_back({edge[0],edge[1],o.first});}
    return result;
}
inline SurfaceSplit splitPlaneSurface(const Surface&surface,const Plane&p,const std::map<int,float>&wood){
    Mesh tagged;unsigned role=0;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior,&surface.finiteWalls}){for(const auto&o:*mesh)tagged[o.first*4+int(role)]=o.second;++role;}
    auto cut=split(tagged,p,nullptr,false);SurfaceSplit result;result.complete=cut.complete;if(!cut.complete)return result;
    const auto unpack=[](Mesh&mesh,Surface&out){for(auto&o:mesh){auto&destination=o.first%4==0?out.outer:o.first%4==1?out.canonicalCaps:o.first%4==2?out.interior:out.finiteWalls;destination[o.first/4]=std::move(o.second);}};unpack(cut.kept,result.kept);unpack(cut.detached,result.detached);
    const auto measured=[](const Surface&s){float total=0;for(const auto*mesh:{&s.outer,&s.canonicalCaps,&s.finiteWalls})for(const auto&o:*mesh)for(const auto&t:o.second)total+=storm_ship_hull::area(t);return total;};result.keptArea=measured(result.kept);result.detachedArea=measured(result.detached);
    std::vector<Segment>canonical,render;for(auto edge:cut.seam){const int role=edge.object%4;edge.object/=4;if(role!=2)canonical.push_back(edge);if(role!=1)render.push_back(edge);}result.seam=canonical;
    if(!surfaceCaps(std::move(canonical),p,result.kept.canonicalCaps,result.detached.canonicalCaps)||!surfaceCaps(std::move(render),p,result.kept.interior,result.detached.interior,&wood))result.complete=false;
    result.kept.timber=surface.timber;result.kept.legacy=surface.legacy;result.kept.legacy.push_back(p);result.kept.supportOriginal=surface.supportOriginal;result.kept.released=surface.released;result.kept.checkpointed=surface.checkpointed;
    result.kept.released.push_back(std::make_shared<const Mesh>(renderMesh(result.detached)));
    return result;
}
// Subtract each delivered prism, preserving every outside triangle and clipping
// only its actual boundary caps. Sequential halfspace solids would leave hidden
// full-size partition walls; here every new wall is bounded by all other faces.
inline SurfaceSplit splitSurface(const Surface&surface,const Plane&p,const std::map<int,float>&wood){storm_geometry_work::checkpoint();
    if(p.beams.empty())return splitPlaneSurface(surface,p,wood);
    SurfaceSplit result;result.kept=surface;if(!valid(p)){result.complete=false;return result;}
    if(!result.kept.material&&!result.kept.cells){std::set<int>objects;for(const auto&o:surface.outer)objects.insert(o.first);result.kept.material=std::make_shared<const std::vector<Mesh>>(materialSolids(surface,objects,wood));}
    if(!result.kept.original){auto original=std::make_shared<SurfaceBase>(SurfaceBase{surface.outer,surface.canonicalCaps,surface.interior,surface.finiteWalls});original->index=indexFaces(original->outer);result.kept.original=std::move(original);}
    for(const auto&beam:p.beams){bool redundant=false;for(const auto&previous:result.kept.cutters)redundant|=beam.start==previous.start&&beam.direction==previous.direction&&beam.tangent==previous.tangent&&beam.crossSection==previous.crossSection&&beam.backDepth==previous.backDepth&&beam.radius<=previous.radius&&beam.depth<=previous.depth;if(redundant)continue;
        const auto faces=beamPlanes(beam);const auto source=std::move(result.kept);Surface kept,detached;std::set<int>affected;
        if(!subtractVolume(source.outer,faces,kept.outer,detached.outer,&affected)||!subtractVolume(source.canonicalCaps,faces,kept.canonicalCaps,detached.canonicalCaps,&affected)||!subtractVolume(source.interior,faces,kept.interior,detached.interior,&affected)||!subtractVolume(source.finiteWalls,faces,kept.finiteWalls,detached.finiteWalls,&affected)){result.complete=false;return result;}
        kept.material=source.material;kept.cells=source.cells;kept.original=source.original;kept.supportOriginal=source.supportOriginal;kept.checkpointed=source.checkpointed;kept.cutters=source.cutters;kept.cutters.push_back(beam);kept.released=source.released;kept.timber=source.timber;kept.legacy=source.legacy;
        for(const auto*mesh:{&detached.outer,&detached.canonicalCaps,&detached.finiteWalls})for(const auto&o:*mesh)for(const auto&t:o.second)result.detachedArea+=storm_ship_hull::area(t);
        const auto local=[&](const Mesh&mesh){Mesh out;for(int object:affected){auto found=mesh.find(object);if(found!=mesh.end())out.emplace(object,found->second);}return out;};
        const auto outer=local(source.outer),caps=local(source.canonicalCaps),interior=local(source.interior);
        for(size_t side=0;side<faces.size()&&!affected.empty();++side){const auto&face=faces[side];std::vector<storm_ship_hull::Plane>limits;
            for(size_t n=0;n<faces.size();++n)if(n!=side)limits.push_back(faces[n]);
            // Only the newly created cross-section belongs to this boundary;
            // pre-existing cap/interior triangles are handled by subtract above.
            auto canonical=planeSeam(outer,face),rendered=canonical;
            auto prior=planeSeam(caps,face);canonical.insert(canonical.end(),prior.begin(),prior.end());
            prior=planeSeam(interior,face);rendered.insert(rendered.end(),prior.begin(),prior.end());
            Mesh inCap,outCap,inRender,outRender;
            if(source.cells){std::vector<uint32_t>ids;source.cells->source->index.queryBounds(beamBounds(beam),ids);for(auto id:ids){const auto&solid=materialCell(*source.cells,id);if(!solid.empty()&&!solidSectionCaps(solid,face,limits,beamStart(beam),p.charred,inCap,outCap)){result.complete=false;return result;}}}
            else{for(const auto&solid:*source.material)if(!solidSectionCaps(solid,face,limits,beamStart(beam),p.charred,inCap,outCap)){result.complete=false;return result;}
                if(source.timber&&!timberSectionCaps(*source.timber,source.legacy,face,limits,beamStart(beam),p.charred,inCap,outCap)){result.complete=false;return result;}}
            for(const auto&previous:source.cutters){const auto priorFaces=beamPlanes(previous);bool coincident=false;for(const auto&prior:priorFaces)coincident|=storm_ship_hull::length(prior.normal-face.normal)<1.e-6f&&std::abs(prior.distance-face.distance)<1.e-6f;if(coincident)continue;
                Mesh in,out,discard;if(!subtractVolume(inCap,priorFaces,in,discard)||!subtractVolume(outCap,priorFaces,out,discard)){result.complete=false;return result;}inCap=std::move(in);outCap=std::move(out);}
            for(const auto&released:source.released){Mesh mask,unused;if(!solidSectionCaps(*released,face,limits,beamStart(beam),false,mask,unused)){result.complete=false;return result;}for(const auto&o:mask)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::array<storm_ship_hull::Plane,3>edges;for(unsigned n=0;n<3;++n){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[(n+1)%3].position-t[n].position,face.normal));edges[n]={normal,storm_ship_hull::dot(normal,t[n].position)};}Mesh in,out,discard;if(!subtractVolume(inCap,edges,in,discard)||!subtractVolume(outCap,edges,out,discard)){result.complete=false;return result;}inCap=std::move(in);outCap=std::move(out);}}
            appendMesh(kept.finiteWalls,outCap);appendMesh(detached.finiteWalls,inCap);
            for(auto edge:canonical){bool alive=true;for(const auto&limit:limits){const float a=storm_ship_hull::dot(limit.normal,edge.a.position)-limit.distance,b=storm_ship_hull::dot(limit.normal,edge.b.position)-limit.distance;
                if(a>0&&b>0){alive=false;break;}if((a>0)!=(b>0)){auto intersection=storm_ship_hull::interpolate(edge.a,edge.b,a/(a-b));if(a>0)edge.a=intersection;else edge.b=intersection;}}
                if(alive&&!near(edge.a.position,edge.b.position))result.seam.push_back(edge);}
        }
        if(!compactFiniteWalls(kept.finiteWalls,kept.cutters)||!compactFiniteWalls(detached.finiteWalls,kept.cutters)){result.complete=false;return result;}
        const auto&baseline=*kept.original;
        if(!clipOriginalMesh(baseline.outer,kept.cutters,kept.outer,kept.released)||!clipOriginalMesh(baseline.canonicalCaps,kept.cutters,kept.canonicalCaps,kept.released)||!clipOriginalMesh(baseline.interior,kept.cutters,kept.interior,kept.released)){result.complete=false;return result;}
        // Adjacent doses are distinct physical solids. Their shared interface
        // can have different triangulation; validate before batching draws.
        const auto physical=canonicalMesh(detached);bool physicalFaces=false;for(const auto&o:physical)physicalFaces|=!o.second.empty();if(physicalFaces)result.detachedVolumes.push_back(closedCutVolume(physical));
        result.kept=std::move(kept);appendMesh(result.detached.outer,detached.outer);appendMesh(result.detached.canonicalCaps,detached.canonicalCaps);appendMesh(result.detached.interior,detached.interior);appendMesh(result.detached.finiteWalls,detached.finiteWalls);
    }
    for(const auto*mesh:{&result.kept.outer,&result.kept.canonicalCaps,&result.kept.finiteWalls})for(const auto&o:*mesh)for(const auto&t:o.second)result.keptArea+=storm_ship_hull::area(t);
    return result;
}
// Authored UV/material islands are attached parts, not independent wreck bodies.
// A mechanical secondary break clips the entire assembly through one shared
// spatial plane; lasers keep the detached assembly intact.
inline std::vector<Surface> surfaceFragments(Surface surface,const Plane&cut,unsigned limit,const std::map<int,float>&wood){
    if(!limit)return {};std::vector<Surface>result;
    if(!cut.charred&&limit>1){const auto b=bounds(surface.outer);const auto size=b.hi-b.lo;const bool longitudinal=size.z>=size.x;const float span=longitudinal?size.z:size.x,across=longitudinal?size.x:size.z;
        if(span>std::max(6.f,across*2.f)){const Vec3 normal=longitudinal?Vec3{0,0,1}:Vec3{1,0,0};const float low=longitudinal?b.lo.z:b.lo.x;auto divided=splitSurface(surface,{normal,low+span*.38f,false},wood);const float area=divided.keptArea+divided.detachedArea;if(divided.complete&&divided.keptArea>area*.1f&&divided.detachedArea>area*.1f){result.push_back(std::move(divided.kept));result.push_back(std::move(divided.detached));return result;}}}
    result.push_back(std::move(surface));return result;
}

inline std::vector<Mesh> components(const Mesh&source) {
    struct Ref{int object;size_t triangle;};std::vector<Ref>refs;std::vector<size_t>parent;
    std::unordered_map<PositionEdge,size_t,storm_ship_laser::PositionEdgeHash>edges;
    const auto root=[&](size_t a){while(parent[a]!=a){parent[a]=parent[parent[a]];a=parent[a];}return a;};
    for(const auto&o:source)for(size_t n=0;n<o.second.size();++n){storm_geometry_work::checkpoint();const size_t id=refs.size();refs.push_back({o.first,n});parent.push_back(id);const auto&t=o.second[n];
        for(unsigned c=0;c<3;++c){auto a=storm_ship_laser::positionKey(t[c].position),b=storm_ship_laser::positionKey(t[(c+1)%3].position);if(a==b)continue;auto ordered=std::minmax(a,b);
            auto inserted=edges.emplace(PositionEdge{ordered.first,ordered.second},id);if(!inserted.second){const size_t x=root(id),y=root(inserted.first->second);parent[x]=y;}}}
    std::map<size_t,Mesh>groups;for(size_t n=0;n<refs.size();++n)groups[root(n)][refs[n].object].push_back(source.at(refs[n].object)[refs[n].triangle]);
    std::vector<Mesh>result;result.reserve(groups.size());for(auto&entry:groups)result.push_back(std::move(entry.second));return result;
}
struct SurfaceTopology {
    struct Face {unsigned role;int object;size_t triangle;};
    std::vector<Face>faces;std::vector<size_t>parent;std::map<PositionKey,size_t>vertices;
    size_t root(size_t n){while(parent[n]!=n){parent[n]=parent[parent[n]];n=parent[n];}return n;}
    template<class Boundary> explicit SurfaceTopology(const Boundary&surface){std::map<PositionEdge,std::vector<size_t>>edges;unsigned role=0;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior,&surface.finiteWalls}){for(const auto&o:*mesh)for(size_t n=0;n<o.second.size();++n){storm_geometry_work::checkpoint();const size_t id=faces.size();faces.push_back({role,o.first,n});parent.push_back(id);const auto&t=o.second[n];for(unsigned c=0;c<3;++c){auto a=positionKey(t[c].position),b=positionKey(t[(c+1)%3].position);vertices.emplace(a,id);if(a==b)continue;if(b<a)std::swap(a,b);auto&edge=edges[{a,b}];if(!edge.empty())parent[root(id)]=root(edge.front());edge.push_back(id);}}++role;}
        struct Edge{Vec3 a,b;size_t face;};std::vector<Edge>boundary;const auto point=[](const PositionKey&key){return Vec3{float(key[0])*.0001f,float(key[1])*.0001f,float(key[2])*.0001f};};for(const auto&edge:edges)if(edge.second.size()==1)boundary.push_back({point(edge.first.first),point(edge.first.second),edge.second.front()});
        if(boundary.size()<=4096)for(size_t n=0;n<boundary.size();++n)for(size_t m=n+1;m<boundary.size();++m){storm_geometry_work::checkpoint();const auto&a=boundary[n];const auto&b=boundary[m];if(root(a.face)==root(b.face))continue;const auto line=a.b-a.a,first=b.a-a.a,last=b.b-a.a;const double length2=storm_ship_hull::dot(line,line),tolerance=.00015*std::sqrt(length2);if(storm_ship_hull::length(storm_ship_hull::cross(line,first))>tolerance||storm_ship_hull::length(storm_ship_hull::cross(line,last))>tolerance)continue;const double low=storm_ship_hull::dot(first,line)/length2,high=storm_ship_hull::dot(last,line)/length2;if(std::min(1.,std::max(low,high))-std::max(0.,std::min(low,high))>1.e-5)parent[root(a.face)]=root(b.face);}}
};
inline const Mesh&surfaceRole(const Surface&surface,unsigned role){return role==0?surface.outer:role==1?surface.canonicalCaps:role==2?surface.interior:surface.finiteWalls;}
inline const Mesh&surfaceRole(const SurfaceBase&surface,unsigned role){return role==0?surface.outer:role==1?surface.canonicalCaps:role==2?surface.interior:surface.finiteWalls;}
inline Mesh&surfaceRole(Surface&surface,unsigned role){return role==0?surface.outer:role==1?surface.canonicalCaps:role==2?surface.interior:surface.finiteWalls;}
inline bool pointOnFace(Vec3 point,const Triangle&t){storm_geometry_work::checkpoint();const auto u=t[1].position-t[0].position,v=t[2].position-t[0].position,w=point-t[0].position,n=storm_ship_hull::cross(u,v);const double length2=storm_ship_hull::dot(n,n);if(length2<1.e-16||std::abs(storm_ship_hull::dot(n,w))>1.e-5*std::sqrt(length2))return false;
    const double uu=storm_ship_hull::dot(u,u),uv=storm_ship_hull::dot(u,v),vv=storm_ship_hull::dot(v,v),wu=storm_ship_hull::dot(w,u),wv=storm_ship_hull::dot(w,v),d=uu*vv-uv*uv;if(d<=1.e-16)return false;const double a=(wu*vv-wv*uv)/d,b=(wv*uu-wu*uv)/d;return a>=-1.e-5&&b>=-1.e-5&&a+b<=1.00001;}
inline Vec3 closestFacePoint(Vec3 p,const Triangle&t){const auto a=t[0].position,b=t[1].position,c=t[2].position,ab=b-a,ac=c-a,ap=p-a;const float d1=storm_ship_hull::dot(ab,ap),d2=storm_ship_hull::dot(ac,ap);if(d1<=0&&d2<=0)return a;
    const auto bp=p-b;const float d3=storm_ship_hull::dot(ab,bp),d4=storm_ship_hull::dot(ac,bp);if(d3>=0&&d4<=d3)return b;const float vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return a+ab*(d1/(d1-d3));
    const auto cp=p-c;const float d5=storm_ship_hull::dot(ab,cp),d6=storm_ship_hull::dot(ac,cp);if(d6>=0&&d5<=d6)return c;const float vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return a+ac*(d2/(d2-d6));const float va=d3*d6-d5*d4;if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0)return b+(c-b)*((d4-d3)/((d4-d3)+(d5-d6)));const float sum=va+vb+vc;if(std::abs(sum)<1.e-20f)return a;return a+ab*(vb/sum)+ac*(vc/sum);}
inline bool supportRetained(const Surface&surface,const FaceIndex&retained,const std::vector<FaceIndex>&released,Vec3 point,std::vector<uint32_t>*candidateScratch=nullptr,std::vector<uint32_t>*nearbyScratch=nullptr){
    const auto authored=surface.supportOriginal?surface.supportOriginal:surface.original;
    if((!surface.checkpointed&&surface.released.empty())||!authored||!storm_ship_hull::finite(point))return true;const auto&source=*authored;const auto&index=source.index;if(index.references.empty())return true;
    std::vector<uint32_t>localCandidates,localNearby;auto&candidates=candidateScratch?*candidateScratch:localCandidates;auto&nearby=nearbyScratch?*nearbyScratch:localNearby;
    float radius=.25f,best=INFINITY;const float maximum=storm_ship_hull::length(index.bounds.hi-index.bounds.lo)+storm_ship_hull::length(point-(index.bounds.lo+index.bounds.hi)*.5f)+1.f;
    for(unsigned pass=0;pass<32;++pass){const Vec3 extent{radius,radius,radius};index.broadphase.queryBounds({point-extent,point+extent},candidates);for(uint32_t candidate:candidates){const auto ref=index.references[candidate];const auto delta=closestFacePoint(point,source.outer.at(ref.first)[ref.second])-point;best=std::min(best,storm_ship_hull::dot(delta,delta));}if(std::isfinite(best)&&radius>=std::sqrt(best)+.0002f)break;if(radius>=maximum)break;radius=std::min(maximum,std::isfinite(best)?std::max(radius*2,std::sqrt(best)+.0002f):radius*2);}
    if(!std::isfinite(best))return true;bool lost=false;const float nearest=std::sqrt(best)+.0002f;const Vec3 extent{.0002f,.0002f,.0002f};
    for(uint32_t candidate:candidates){const auto original=index.references[candidate];const auto witness=closestFacePoint(point,source.outer.at(original.first)[original.second]),delta=witness-point;if(storm_ship_hull::dot(delta,delta)>nearest*nearest)continue;
        retained.broadphase.queryBounds({witness-extent,witness+extent},nearby);for(uint32_t face:nearby){const auto ref=retained.references[face];if(ref.first==original.first&&pointOnFace(witness,surface.outer.at(ref.first)[ref.second]))return true;}
        if(surface.checkpointed){lost=true;continue;}
        bool detached=false;for(size_t n=0;n<released.size();++n){released[n].broadphase.queryBounds({witness-extent,witness+extent},nearby);for(uint32_t face:nearby){const auto ref=released[n].references[face];if(ref.first==original.first&&pointOnFace(witness,surface.released[n]->at(ref.first)[ref.second])){detached=true;break;}}if(detached)break;}if(!detached)return true;lost=true;}
    return !lost;
}
// A cut may split one prior physical component. Keep its largest surviving
// authored support area; pre-existing disconnected components each keep their
// own support owner rather than competing with the whole vessel.
inline SurfaceTopology materialTopology(const Surface&);
inline std::vector<Surface> detachEnclosed(const Surface&before,Surface&after){
    auto source=materialTopology(before),current=materialTopology(after);std::map<size_t,double>supportArea;
    for(size_t n=0;n<current.faces.size();++n){const auto&ref=current.faces[n];if(ref.role==0)supportArea[current.root(n)]+=storm_ship_hull::area(after.outer.at(ref.object)[ref.triangle]);}
    std::map<size_t,std::set<size_t>>parents;for(const auto&entry:current.vertices){auto old=source.vertices.find(entry.first);if(old!=source.vertices.end())parents[current.root(entry.second)].insert(source.root(old->second));}
    std::map<size_t,size_t>representatives;for(size_t n=0;n<current.faces.size();++n)if(current.faces[n].role==0)representatives.try_emplace(current.root(n),n);
    for(const auto&entry:representatives)if(parents[entry.first].empty()){const auto&ref=current.faces[entry.second];const auto&t=after.outer.at(ref.object)[ref.triangle];const Vec3 point=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);
        for(size_t n=0;n<source.faces.size();++n){const auto&old=source.faces[n];if(old.role==0&&old.object==ref.object&&pointOnFace(point,before.outer.at(old.object)[old.triangle]))parents[entry.first].insert(source.root(n));}}
    std::map<size_t,std::pair<double,size_t>>owners;for(const auto&entry:parents)for(size_t parent:entry.second){const auto candidate=std::make_pair(supportArea[entry.first],entry.first);auto found=owners.find(parent);if(found==owners.end()||candidate.first>found->second.first||(candidate.first==found->second.first&&candidate.second<found->second.second))owners[parent]=candidate;}
    std::set<size_t>released;for(const auto&entry:parents)if(supportArea[entry.first]>1.e-7&&!entry.second.empty()){bool supportedElsewhere=true;for(size_t parent:entry.second)supportedElsewhere&=owners.at(parent).second!=entry.first;if(supportedElsewhere)released.insert(entry.first);}
    if(released.empty())return {};Surface retained;retained.material=after.material;retained.cells=after.cells;retained.structure=after.structure;retained.original=after.original;retained.supportOriginal=after.supportOriginal;retained.checkpointed=after.checkpointed;retained.cutters=after.cutters;retained.released=after.released;retained.timber=after.timber;retained.legacy=after.legacy;for(const auto*mesh:{&after.outer,&after.canonicalCaps,&after.interior})for(const auto&o:*mesh)retained.outer.try_emplace(o.first);
    std::map<size_t,Surface>pieces;for(size_t n=0;n<current.faces.size();++n){const auto&ref=current.faces[n];const size_t component=current.root(n);auto&destination=released.count(component)?pieces[component]:retained;surfaceRole(destination,ref.role)[ref.object].push_back(surfaceRole(after,ref.role).at(ref.object)[ref.triangle]);}
    std::vector<Surface>result;for(auto&piece:pieces){retained.released.push_back(std::make_shared<const Mesh>(renderMesh(piece.second)));result.push_back(std::move(piece.second));}after=std::move(retained);return result;
}

inline bool supportTrianglesTouch(const Triangle&a,const Triangle&b){
    constexpr float tolerance=.00015f,tolerance2=tolerance*tolerance;
    const auto close=[](Vec3 a,Vec3 b){const auto d=a-b;return storm_ship_hull::dot(d,d);};
    for(const auto&v:a)if(close(v.position,closestFacePoint(v.position,b))<=tolerance2)return true;
    for(const auto&v:b)if(close(v.position,closestFacePoint(v.position,a))<=tolerance2)return true;
    const auto crosses=[&](Vec3 p,Vec3 q,const Triangle&t){const auto n=storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position);
        const double first=storm_ship_hull::dot(n,p-t[0].position),last=storm_ship_hull::dot(n,q-t[0].position);
        if((first<0)==(last<0)||std::abs(first-last)<1.e-20)return false;
        const auto point=p+(q-p)*float(first/(first-last));return close(point,closestFacePoint(point,t))<=tolerance2;};
    for(unsigned i=0;i<3;++i){if(crosses(a[i].position,a[(i+1)%3].position,b)||crosses(b[i].position,b[(i+1)%3].position,a))return true;
        for(unsigned j=0;j<3;++j){const auto p=a[i].position,q=b[j].position,u=a[(i+1)%3].position-p,v=b[(j+1)%3].position-q,w=p-q;
            const double aa=storm_ship_hull::dot(u,u),bb=storm_ship_hull::dot(u,v),cc=storm_ship_hull::dot(v,v),dd=storm_ship_hull::dot(u,w),ee=storm_ship_hull::dot(v,w);
            if(aa<1.e-16||cc<1.e-16)continue;const double determinant=aa*cc-bb*bb;
            double s=determinant>1.e-20?std::clamp((bb*ee-cc*dd)/determinant,0.,1.):0.,t=(bb*s+ee)/cc;
            if(t<0){t=0;s=std::clamp(-dd/aa,0.,1.);}else if(t>1){t=1;s=std::clamp((bb-dd)/aa,0.,1.);}
            if(close(p+u*float(s),q+v*float(t))<=tolerance2)return true;}}
    return false;
}
// Shared GM edges are not the only authored joints: overlapping pole sections
// carry load through real triangle contact. A bbox overlap alone carries none.
template<class Boundary> struct SupportTopology {
    const Boundary&surface;SurfaceTopology topology;storm_ship_hull::Broadphase index;
    const Triangle&face(size_t n)const{const auto&ref=topology.faces[n];return surfaceRole(surface,ref.role).at(ref.object)[ref.triangle];}
    explicit SupportTopology(const Boundary&source):surface(source),topology(source){
        const Vec3 margin{.00015f,.00015f,.00015f};std::vector<Bounds>boxes;boxes.reserve(topology.faces.size());
        for(size_t n=0;n<topology.faces.size();++n){const auto&t=face(n);Bounds box{t[0].position,t[0].position};for(unsigned c=1;c<3;++c)box=storm_ship_hull::merge(box,{t[c].position,t[c].position});box.lo=box.lo-margin;box.hi=box.hi+margin;boxes.push_back(box);}
        index.build(boxes);std::vector<uint32_t>nearby;
        for(size_t n=0;n<boxes.size();++n){storm_geometry_work::checkpoint();index.queryBounds(boxes[n],nearby);for(auto other:nearby){storm_geometry_work::checkpoint();if(other>n&&topology.root(n)!=topology.root(other)&&supportTrianglesTouch(face(n),face(other)))topology.parent[topology.root(other)]=topology.root(n);}}
    }
};
inline SurfaceTopology materialTopology(const Surface&surface){SupportTopology<Surface> physical(surface);return std::move(physical.topology);}
template<class Boundary> inline bool supportDisconnected(const Boundary&authored,const Surface&retained,const SupportSpan&span){
    if(!storm_ship_hull::finite(span.base)||!storm_ship_hull::finite(span.tip)||storm_ship_hull::length(span.tip-span.base)<.001f)return false;
    SupportTopology<Boundary> original(authored);SupportTopology<Surface> current(retained);
    struct Witness{Vec3 point;int object;size_t component;};std::vector<Witness>base,tip;
    const auto witnesses=[&](Vec3 point,std::vector<Witness>&out){float best=INFINITY;for(size_t n=0;n<original.topology.faces.size();++n){storm_geometry_work::checkpoint();const auto&ref=original.topology.faces[n];if(ref.role)continue;
        const auto witness=closestFacePoint(point,original.face(n)),delta=witness-point;const float distance=storm_ship_hull::dot(delta,delta);
        if(distance<best-1.e-8f){best=distance;out.clear();}if(distance<=best+1.e-8f)out.push_back({witness,ref.object,original.topology.root(n)});}};
    witnesses(span.base,base);witnesses(span.tip,tip);
    std::set<size_t>support;for(const auto&a:base)for(const auto&b:tip)if(a.component==b.component)support.insert(a.component);
    if(support.empty())return false; // No authored load path between the declared native endpoints.
    // Anchor the authored bottom contact region, not one nearest endpoint point.
    // A bore through one point inside a wide foot must not erase its other real
    // load paths; removing the entire foot leaves the surviving upper pole free.
    const auto axis=storm_ship_hull::normalized(span.tip-span.base);std::map<size_t,float>bottom;
    for(size_t n=0;n<original.topology.faces.size();++n){const auto&ref=original.topology.faces[n];const auto component=original.topology.root(n);if(ref.role||!support.count(component))continue;
        for(const auto&v:original.face(n)){const float height=storm_ship_hull::dot(v.position-span.base,axis);auto inserted=bottom.emplace(component,height);inserted.first->second=std::min(inserted.first->second,height);}}
    base.clear();for(size_t n=0;n<original.topology.faces.size();++n){const auto&ref=original.topology.faces[n];const auto component=original.topology.root(n);if(ref.role||!support.count(component))continue;
        for(const auto&v:original.face(n))if(storm_ship_hull::dot(v.position-span.base,axis)<=bottom.at(component)+.00015f)base.push_back({v.position,ref.object,component});}
    const auto roots=[&](const std::vector<Witness>&witnesses,size_t component){std::set<size_t>result;std::vector<uint32_t>nearby;const Vec3 margin{.0002f,.0002f,.0002f};
        for(const auto&w:witnesses)if(w.component==component){current.index.queryBounds({w.point-margin,w.point+margin},nearby);for(auto n:nearby){const auto&ref=current.topology.faces[n];if(ref.role==0&&ref.object==w.object&&pointOnFace(w.point,current.face(n)))result.insert(current.topology.root(n));}}return result;};
    for(size_t component:support){const auto low=roots(base,component),high=roots(tip,component);
        // Erasing a tip witness alone does not prove that the standing pole fell.
        if(high.empty())return false;for(auto root:low)if(high.count(root))return false;}
    return true;
}
inline bool beamSeversSupport(const Surface&before,const Surface&after,const SupportSpan&span){
    // Use the immutable authored boundary, not the last already-separated dose.
    const auto authored=before.supportOriginal?before.supportOriginal:before.original;
    return authored?supportDisconnected(*authored,after,span):supportDisconnected(before,after,span);
}
inline bool prepare(const Input&input,Projection&output){storm_geometry_work::checkpoint(true);
    if(!input.surface||!valid(input.plane))return false;Projection next;next.generation=input.generation;next.revision=input.revision;next.materialRevision=input.materialRevision;next.plane=input.plane;next.enclosedFragments=input.enclosedFragments;
    auto cut=splitSurface(*input.surface,input.plane,input.wood);if(!cut.complete)return false;
    float removedInterior=0;for(const auto&o:cut.detached.interior)for(const auto&t:o.second)removedInterior+=storm_ship_hull::area(t);
    if(input.hasSupport&&!input.plane.beams.empty()&&(cut.detachedArea>1.e-7f||removedInterior>1.e-7f||!input.surface->cutters.empty()||input.surface->checkpointed))
        next.result.supportSevered=beamSeversSupport(*input.surface,cut.kept,input.support);
    // A partial bore can remove inner timber before reaching the outer shell.
    // Publish that real material change without charging it as canonical damage.
    if((cut.detachedArea<=1.e-7f&&removedInterior<=1.e-7f)||cut.keptArea+cut.detachedArea<=1.e-7f){output=std::move(next);return true;}
    // An exhausted output budget still permits untouched nodes in an atomic cut.
    // Actual material removal requires a fragment slot and must remain unchanged.
    if(!input.fragmentLimit&&(!input.structuralFragmentsOnly||input.plane.beams.empty()))return false;
    std::vector<Surface>released;if(input.enclosedFragments&&!input.plane.beams.empty())released=detachEnclosed(*input.surface,cut.kept);
    auto groups=input.structuralFragmentsOnly&&!input.plane.beams.empty()?std::vector<Surface>{}:surfaceFragments(std::move(cut.detached),input.plane,input.fragmentLimit,input.wood);if(groups.size()+released.size()>input.fragmentLimit)return false;
    if(input.structuralFragmentsOnly&&!input.plane.beams.empty())for(const auto&o:cut.detached.outer)for(const auto&t:o.second)
        next.result.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});
    for(auto&piece:released){const auto mesh=canonicalMesh(piece);for(const auto&o:mesh)for(const auto&t:o.second)cut.detachedArea+=storm_ship_hull::area(t);groups.push_back(std::move(piece));}
    next.canonical=canonicalMesh(cut.kept);next.render=renderMesh(cut.kept);next.result.changed=true;next.result.keptBounds=bounds(next.canonical);next.result.seam=std::move(cut.seam);
    float keptArea=0;for(const auto&o:next.canonical)for(const auto&t:o.second)keptArea+=storm_ship_hull::area(t);const float total=keptArea+cut.detachedArea;next.result.removedFraction=std::clamp(cut.detachedArea/std::max(total,1.e-7f),0.f,1.f);
    double detachedVolume=0;if(input.closedVolume>1.e-6)for(double volume:cut.detachedVolumes){if(volume<=0)return false;detachedVolume+=volume;}
    next.result.fragments.reserve(groups.size());next.fragmentRender.reserve(groups.size());size_t groupIndex=0;for(const auto&piece:groups){auto canonical=canonicalMesh(piece);if(input.closedVolume>1.e-6&&(cut.detachedVolumes.empty()||groupIndex>0||input.structuralFragmentsOnly)){const double volume=closedCutVolume(canonical);if(volume<=0)return false;detachedVolume+=volume;}++groupIndex;
        next.result.fragments.push_back(fragmentProperties(canonical));next.fragmentRender.push_back(renderMesh(piece));if(!input.plane.beams.empty())for(const auto&o:piece.outer)for(const auto&t:o.second)next.result.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});}
    if(input.closedVolume>1.e-6){next.keptVolume=closedCutVolume(next.canonical);if((keptArea>1.e-7f&&next.keptVolume<=0)||std::abs(input.closedVolume-next.keptVolume-detachedVolume)>std::max(.001,input.closedVolume*1.e-5))return false;next.result.volumeBased=true;next.result.removedFraction=float(std::clamp(detachedVolume/input.closedVolume,0.,1.));}
    next.collision=indexFaces(next.canonical);next.materialCollision=indexFaces(next.render);next.materialCandidates.reserve(next.materialCollision.references.size());if(!cut.kept.released.empty()){next.retainedSupport=indexFaces(cut.kept.outer);for(const auto&piece:cut.kept.released)next.releasedSupport.push_back(indexFaces(*piece));}next.kept=std::make_shared<const Surface>(std::move(cut.kept));output=std::move(next);return true;
}

// A born fragment owns exactly the uploaded boundary, including inner timber
// skin and finite walls. Rebase its material/original history to that boundary:
// the ship's authored solid and thickness must never grow back on a later cut.
inline FragmentPiece standaloneFragment(Mesh physical){
    FragmentPiece piece;piece.properties=fragmentProperties(physical);piece.collision=indexFaces(physical);
    Surface surface;surface.material=std::make_shared<const std::vector<Mesh>>(std::vector<Mesh>{physical});surface.outer=std::move(physical);
    piece.surface=std::make_shared<const Surface>(std::move(surface));return piece;
}
inline bool traceFragment(const FragmentPiece&piece,Vec3 start,Vec3 end,float&fraction,Vec3&normal){
    fraction=2;normal={};if(!piece.surface||!storm_ship_hull::finite(start)||!storm_ship_hull::finite(end))return false;
    try{std::vector<uint32_t>candidates;piece.collision.broadphase.queryRay(start,end,candidates);const auto direction=end-start;double nearest=2;
        for(uint32_t candidate:candidates){const auto ref=piece.collision.references.at(candidate);const auto&t=piece.surface->outer.at(ref.first).at(ref.second);
            const auto edge1=t[1].position-t[0].position,edge2=t[2].position-t[0].position;
            const auto p=storm_ship_hull::cross(direction,edge2);const double determinant=storm_ship_hull::dot(edge1,p);if(std::abs(determinant)<1.e-12)continue;
            const auto delta=start-t[0].position;const double u=storm_ship_hull::dot(delta,p)/determinant;if(u<0||u>1)continue;
            const auto q=storm_ship_hull::cross(delta,edge1);const double v=storm_ship_hull::dot(direction,q)/determinant;if(v<0||u+v>1)continue;
            const double along=storm_ship_hull::dot(edge2,q)/determinant;if(along<0||along>1||along>=nearest)continue;
            nearest=along;normal=storm_ship_hull::normalized(storm_ship_hull::cross(edge1,edge2));}
        if(nearest>1)return false;fraction=float(nearest);return true;
    }catch(...){fraction=2;normal={};return false;}
}
inline bool prepareFragment(const FragmentInput&input,FragmentProjection&output){
    if(!input.owner||!input.generation||!input.token||!input.successorLimit||!input.input.surface)return false;
    try{Projection cut;if(!prepare(input.input,cut))return false;FragmentProjection next;next.owner=input.owner;next.generation=input.generation;next.token=input.token;next.changed=cut.result.changed;
        if(next.changed){next.successors.reserve(cut.fragmentRender.size()+1);
            const auto append=[&](Mesh mesh){bool nonempty=false;for(const auto&o:mesh)nonempty|=!o.second.empty();if(nonempty)next.successors.push_back(standaloneFragment(std::move(mesh)));};
            append(std::move(cut.render));for(auto&mesh:cut.fragmentRender)append(std::move(mesh));if(next.successors.size()>input.successorLimit)return false;
            const double volume=closedCutVolume(input.input.surface->outer);if(volume>0){double total=0;for(const auto&piece:next.successors){const double part=closedCutVolume(piece.surface->outer);if(part<=0)return false;total+=part;}if(std::abs(total-volume)>std::max(1.e-6,volume*1.e-5))return false;}}
        output=std::move(next);return true;
    }catch(...){return false;}
}

// Compatibility entry point for canonical-only callers; the spatial ownership
// policy is shared with the complete rendered assembly.
inline std::vector<Mesh> fragmentGroups(const Mesh&mesh,const Plane&cut,unsigned limit=storm_hull_damage::MaxFragmentGroups) {
    std::vector<Mesh>result;for(auto&surface:surfaceFragments({mesh,{},{}},cut,std::min(limit,storm_hull_damage::MaxFragmentGroups),{}))result.push_back(canonicalMesh(surface));return result;
}
struct Contour { bool valid=false;std::vector<std::vector<Vec3>>loops; };
// Clip the authored waterplane through the same mesh field as the visible hull.
// Return every connected boundary explicitly; the caller owns component policy.
inline Contour clipContour(const std::vector<Vec3>&polygon,const Plane&p) {
    if(valid(p)&&polygon.empty())return {true,{}};
    if(!valid(p)||polygon.size()<3||polygon.size()>128)return {};
    // A single outer waterplane cannot represent bore holes. Keep its conservative
    // broad-phase envelope; finite support and collision use the field/real mesh.
    if(!p.beams.empty())return {true,{polygon}};
    if(p.roughness==0){std::vector<Vec3>loop;
        for(size_t n=0;n<polygon.size();++n){storm_geometry_work::checkpoint();const auto a=polygon[n],b=polygon[(n+1)%polygon.size()];if(!storm_ship_hull::finite(a))return {};const float da=storm_ship_hull::dot(p.normal,a)-p.distance,db=storm_ship_hull::dot(p.normal,b)-p.distance;
            if(da<=0)loop.push_back(a);if((da<=0)!=(db<=0))loop.push_back(a+(b-a)*(da/(da-db)));}
        if(loop.size()>128)return {};if(loop.size()<3)return {true,{}};return {true,{std::move(loop)}};}
    const float height=polygon.front().y;std::vector<Segment>edges;
    for(size_t n=0;n<polygon.size();++n){storm_geometry_work::checkpoint();const auto a=polygon[n],b=polygon[(n+1)%polygon.size()];if(!storm_ship_hull::finite(a)||std::abs(a.y-height)>1.e-4f||near(a,b))return {};Vertex va,vb;va.position=a;vb.position=b;edges.push_back({va,vb,0});}
    Mesh sheet,back;cap(std::move(edges),{{0,1,0},height,false},sheet,back);
    if(sheet.empty()||sheet.begin()->second.empty())return {};
    auto cut=split(sheet,p);if(!cut.complete)return {};
    using Point=std::array<int64_t,3>;using Edge=std::pair<Point,Point>;
    const auto key=positionKey;
    struct Boundary {Vec3 a,b;unsigned count=0;};std::map<Edge,Boundary>boundary;
    for(const auto&o:cut.kept)for(const auto&t:o.second)for(unsigned n=0;n<3;++n){auto a=t[n].position,b=t[(n+1)%3].position;auto ka=key(a),kb=key(b);if(ka==kb)continue;if(kb<ka){std::swap(ka,kb);std::swap(a,b);}auto&edge=boundary[{ka,kb}];edge.a=a;edge.b=b;if(++edge.count>2)return {};}
    std::map<Point,std::vector<Point>>neighbors;std::map<Point,Vec3>positions;
    for(const auto&e:boundary)if(e.second.count==1){const auto a=e.first.first,b=e.first.second;neighbors[a].push_back(b);neighbors[b].push_back(a);positions[a]=e.second.a;positions[b]=e.second.b;}
    for(const auto&n:neighbors)if(n.second.size()!=2)return {};
    Contour result;size_t total=0;
    while(!neighbors.empty()){const auto start=neighbors.begin()->first;auto current=start,previous=start;std::vector<Vec3>loop;
        do{auto found=neighbors.find(current);if(found==neighbors.end()||++total>128)return {};loop.push_back(positions[current]);const auto next=found->second[0]==previous?found->second[1]:found->second[0];previous=current;current=next;neighbors.erase(found);}while(current!=start);
        if(loop.size()<3)return {};result.loops.push_back(std::move(loop));}
    result.valid=true;return result;
}
inline float trace(const Triangle&t,Vec3 start,Vec3 finish,float&a,float&b) {
    auto direction=finish-start,e1=t[1].position-t[0].position,e2=t[2].position-t[0].position,h=storm_ship_hull::cross(direction,e2);float det=storm_ship_hull::dot(e1,h);
    if(std::abs(det)<1.e-8f)return 2;float inv=1.f/det;auto s=start-t[0].position;a=inv*storm_ship_hull::dot(s,h);if(a<0||a>1)return 2;auto q=storm_ship_hull::cross(s,e1);b=inv*storm_ship_hull::dot(direction,q);if(b<0||a+b>1)return 2;float f=inv*storm_ship_hull::dot(e2,q);return f>=0&&f<=1?f:2;
}
}

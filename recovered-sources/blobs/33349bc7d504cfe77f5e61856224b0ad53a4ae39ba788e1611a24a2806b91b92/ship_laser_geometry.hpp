#pragma once
#include "ship_hull_geometry.hpp"
// Mapbox earcut.hpp v3.2.3, c68c8835ccff2b7532d31d8fa8dfcf398f629498 (ISC).
#include "third_party/mapbox-earcut/earcut.hpp"
#include <unordered_map>
#include "ship_hull_damage_bridge.hpp"
#include <map>
#include <limits>
#include <numeric>
#include <set>
#include <utility>

// A deterministic, node-local half-space split of the actual static GM mesh.
// Vertex interpolation/layout remain owned by ship_hull_geometry.hpp.
namespace storm_ship_laser {
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_hull::Bounds;
using Mesh = std::map<int,std::vector<Triangle>>;
using FacePoint=std::array<double,2>;
using FaceRing=std::vector<FacePoint>;
using FacePolygon=std::vector<FaceRing>;
bool differenceFace(const FaceRing&,const FaceRing&,std::vector<FacePolygon>&);
bool unionFaces(const std::vector<FaceRing>&,std::vector<FacePolygon>&);
using PositionKey=std::array<int64_t,3>;
using PositionEdge=std::pair<PositionKey,PositionKey>;
struct PositionEdgeHash {size_t operator()(const PositionEdge&e)const {size_t h=0;for(const auto&point:{e.first,e.second})for(auto q:point)h^=std::hash<int64_t>{}(q)+0x9e3779b9u+(h<<6)+(h>>2);return h;}};
inline PositionKey positionKey(Vec3 p) {return {int64_t(std::llround(double(p.x)*10000)),int64_t(std::llround(double(p.y)*10000)),int64_t(std::llround(double(p.z)*10000))};}
using BeamSample=storm_hull_damage::BeamSample;
inline Vec3 beamStart(const BeamSample&b){return {b.start[0],b.start[1],b.start[2]};}
inline Vec3 beamDirection(const BeamSample&b){return {b.direction[0],b.direction[1],b.direction[2]};}
inline Vec3 beamTangent(const BeamSample&b){return b.tangent==std::array<float,3>{} ? storm_ship_hull::frame(beamDirection(b)).tangent : Vec3{b.tangent[0],b.tangent[1],b.tangent[2]};}
inline bool valid(const BeamSample&b){return storm_ship_hull::finite(beamStart(b))&&storm_ship_hull::finite(beamDirection(b))&&
    std::abs(storm_ship_hull::length(beamDirection(b))-1.f)<.001f&&std::isfinite(b.radius)&&b.radius>=.01f&&b.radius<=2.f&&
    std::isfinite(b.depth)&&b.depth>=.001f&&b.depth<=1000.f&&storm_ship_hull::finite(beamTangent(b))&&
    std::abs(storm_ship_hull::length(beamTangent(b))-1.f)<.001f&&std::abs(storm_ship_hull::dot(beamTangent(b),beamDirection(b)))<.001f;}
inline std::array<storm_ship_hull::Plane,10> beamPlanes(const BeamSample&b){
    const auto start=beamStart(b),direction=beamDirection(b);const storm_ship_hull::Frame basis{beamTangent(b),storm_ship_hull::cross(direction,beamTangent(b))};
    std::array<storm_ship_hull::Plane,10> result{};
    result[0]={direction,storm_ship_hull::dot(direction,start)+b.depth};
    result[1]={direction*-1.f,-storm_ship_hull::dot(direction,start)+b.radius*.15625f};
    constexpr float diagonal=.7071067811865475f;
    constexpr std::array<std::array<float,2>,8> octagon{{{1,0},{diagonal,diagonal},{0,1},{-diagonal,diagonal},
        {-1,0},{-diagonal,-diagonal},{0,-1},{diagonal,-diagonal}}};
    for(unsigned n=0;n<8;++n){const auto normal=basis.tangent*octagon[n][0]+basis.bitangent*octagon[n][1];
        result[n+2]={normal,storm_ship_hull::dot(normal,start)+b.radius};}
    return result;
}
inline float beamDistance(const BeamSample&b,Vec3 point){float inside=std::numeric_limits<float>::infinity();
    for(const auto&p:beamPlanes(b))inside=std::min(inside,p.distance-storm_ship_hull::dot(p.normal,point));return inside;}
inline bool beamIntersectsBox(const BeamSample&b,Vec3 center,Vec3 halfSize){
    if(!valid(b))return false;for(const auto&p:beamPlanes(b)){
        const float reach=std::abs(p.normal.x)*halfSize.x+std::abs(p.normal.y)*halfSize.y+std::abs(p.normal.z)*halfSize.z;
        if(storm_ship_hull::dot(p.normal,center)-reach>p.distance)return false;}return true;
}
struct Plane { Vec3 normal; float distance=0; bool charred=true; Vec3 grain{0,1,0}; float roughness=0,spacing=1,phase=0;
    std::vector<BeamSample> beams; };
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
struct Opening { Vec3 point; float area=0; };
struct Result { bool changed=false; uint64_t fragment=0; float removedFraction=0; Bounds keptBounds{}; bool volumeBased=false; std::vector<Segment>seam; std::vector<Fragment>fragments; bool supportSevered=false; std::vector<Opening>openings; };
struct Split { Mesh kept,detached; float keptArea=0,detachedArea=0; std::vector<Segment>seam; bool complete=true; };
inline bool near(Vec3 a,Vec3 b) { auto d=a-b;return storm_ship_hull::dot(d,d)<1.e-8f; }
inline Bounds bounds(const Mesh&m) { Bounds b{};bool first=true;for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t){if(first){b={v.position,v.position};first=false;}else b=storm_ship_hull::merge(b,{v.position,v.position});}return b; }
inline Fragment fragmentProperties(const Mesh&mesh) {
    Fragment result;result.bounds=bounds(mesh);double area=0,x=0,y=0,z=0;bool first=true;
    const std::array<Vec3,14>directions={{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},
        {1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},{-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1}}};
    for(const auto&o:mesh)for(const auto&t:o.second){const double a=storm_ship_hull::area(t);area+=a;
        x+=a*(double(t[0].position.x)+t[1].position.x+t[2].position.x)/3.;
        y+=a*(double(t[0].position.y)+t[1].position.y+t[2].position.y)/3.;
        z+=a*(double(t[0].position.z)+t[1].position.z+t[2].position.z)/3.;
        for(const auto&v:t){for(size_t n=0;n<directions.size();++n)
            if(first||storm_ship_hull::dot(v.position,directions[n])>storm_ship_hull::dot(result.support[n],directions[n]))result.support[n]=v.position;
            first=false;}}
    result.surfaceArea=float(area);result.centroid=area>1.e-12?Vec3{float(x/area),float(y/area),float(z/area)}:(result.bounds.lo+result.bounds.hi)*.5f;return result;
}
// Render-only thickness for explicitly classified structural wood. Canonical
// cut/collision meshes never contain these faces, so reconstruction cannot grow
// another inner shell on every saved cut. Open holds and ports stay open.
inline Mesh shellForRender(const Mesh&mesh,const std::map<int,float>&thicknesses,const std::map<PositionKey,float>*depthLimits=nullptr,const std::map<PositionKey,Vec3>*offsetNormals=nullptr) {
    Mesh result=mesh;
    using Point=std::array<int64_t,3>;using Edge=std::pair<Point,Point>;
    const auto key=positionKey;
    const auto darken=[](Vertex&v,float factor){uint32_t color=v.color&0xff000000u;for(unsigned c=0;c<3;++c)color|=uint32_t(float((v.color>>(c*8))&255u)*factor)<<(c*8);v.color=color;};
    struct Offset {Vec3 normal{};double thickness=0;size_t count=0;};std::map<Point,Offset>offsets;
    // One inward position at shared corners, including UV/material seams. Using
    // each face's own shading normal there would tear the new inner shell open.
    for(const auto&o:mesh){const auto selected=thicknesses.find(o.first);if(selected==thicknesses.end()||!std::isfinite(selected->second)||selected->second<=0)continue;
        for(const auto&t:o.second){const auto fallback=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
            for(const auto&v:t){auto&offset=offsets[key(v.position)];offset.normal=offset.normal+(storm_ship_hull::length(v.normal)>1.e-6f?storm_ship_hull::normalized(v.normal):fallback);offset.thickness+=selected->second;++offset.count;}}}
    const auto inward=[&](const Vertex&v){const auto&offset=offsets.at(key(v.position));auto normal=storm_ship_hull::length(offset.normal)>1.e-6f?storm_ship_hull::normalized(offset.normal):storm_ship_hull::normalized(v.normal);
        if(offsetNormals){const auto found=offsetNormals->find(key(v.position));if(found!=offsetNormals->end())normal=found->second;}
        float depth=float(offset.thickness/offset.count);if(depthLimits){const auto limit=depthLimits->find(key(v.position));if(limit!=depthLimits->end())depth=std::min(depth,std::max(0.f,limit->second));}
        return v.position-normal*depth;};
    struct Boundary{Vertex a,b;int object=0;unsigned count=0;};std::map<Edge,Boundary>edges;
    for(const auto&o:mesh){const auto selected=thicknesses.find(o.first);if(selected==thicknesses.end()||!std::isfinite(selected->second)||selected->second<=0)continue;
        auto&out=result[o.first];
        for(const auto&t:o.second){const auto fallback=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));Triangle inner=t;
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
    for(const auto&o:mesh)for(const auto&t:o.second){const Point points[3]={key(t[0].position),key(t[1].position),key(t[2].position)};if(points[0]==points[1]||points[0]==points[2]||points[1]==points[2])continue;
        volume+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6.;
        for(unsigned n=0;n<3;++n){auto a=points[n],b=points[(n+1)%3];int direction=1;if(b<a){std::swap(a,b);direction=-1;}auto&e=edges[{a,b}];++e.first;e.second+=direction;}}
    for(const auto&e:edges)if(e.second.first!=2||e.second.second!=0)return 0;return std::abs(volume);
}
// A polygon boolean can subdivide one side of a shared edge without changing
// the material boundary. Validate directed interval coverage, rather than
// treating every harmless T-junction as an open solid.
inline double closedCutVolume(const Mesh&mesh){
    std::map<PositionEdge,std::pair<unsigned,int>>edges;double volume=0;for(const auto&o:mesh)for(const auto&t:o.second){const PositionKey points[3]{positionKey(t[0].position),positionKey(t[1].position),positionKey(t[2].position)};if(points[0]==points[1]||points[0]==points[2]||points[1]==points[2])continue;volume+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6.;
        for(unsigned n=0;n<3;++n){auto a=points[n],b=points[(n+1)%3];int sign=1;if(b<a){std::swap(a,b);sign=-1;}auto&edge=edges[{a,b}];++edge.first;edge.second+=sign;}}
    std::vector<PositionEdge>unmatched;std::map<PositionKey,Vec3>points;for(const auto&edge:edges)if(edge.second.first!=2||edge.second.second!=0){unmatched.push_back(edge.first);for(const auto&point:{edge.first.first,edge.first.second})points.emplace(point,Vec3{float(point[0])*.0001f,float(point[1])*.0001f,float(point[2])*.0001f});}
    if(unmatched.empty())return std::abs(volume);if(unmatched.size()>4096)return 0;
    std::map<PositionEdge,std::pair<unsigned,int>>subdivided;for(const auto&edge:unmatched){const auto a=points.at(edge.first),b=points.at(edge.second),line=b-a;const double length2=storm_ship_hull::dot(line,line);std::vector<std::pair<double,PositionKey>>knots{{0,edge.first},{1,edge.second}};
        for(const auto&point:points){if(point.first==edge.first||point.first==edge.second)continue;const auto delta=point.second-a;const double along=storm_ship_hull::dot(delta,line)/length2;if(along>0&&along<1&&storm_ship_hull::length(storm_ship_hull::cross(line,delta))<=.00015*std::sqrt(length2))knots.push_back({along,point.first});}
        std::sort(knots.begin(),knots.end());for(size_t n=1;n<knots.size();++n){auto first=knots[n-1].second,last=knots[n].second;if(first==last)continue;int sign=1;if(last<first){std::swap(first,last);sign=-1;}auto&part=subdivided[{first,last}];part.first+=edges.at(edge).first;part.second+=edges.at(edge).second*sign;}}
    for(const auto&edge:subdivided)if(edge.second.first!=2||edge.second.second!=0)return 0;return std::abs(volume);
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
            for(size_t n=0;n<segments.size();++n)if(!used[n]){
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
        bool reduced=true;while(reduced&&polygon.size()>3){reduced=false;for(size_t n=0;n<polygon.size();++n){size_t a=(n+polygon.size()-1)%polygon.size(),b=(n+1)%polygon.size();if(std::abs(cross2(polygon[a].position,polygon[n].position,polygon[b].position,p.normal))<1.e-7f){polygon.erase(polygon.begin()+n);reduced=true;break;}}}
        while(polygon.size()>3){bool ear=false;for(size_t n=0;n<polygon.size();++n){size_t a=(n+polygon.size()-1)%polygon.size(),c=(n+1)%polygon.size();const auto A=polygon[a].position,B=polygon[n].position,C=polygon[c].position;
            if(cross2(A,B,C,p.normal)<=1.e-8f)continue;bool contains=false;
            for(size_t j=0;j<polygon.size();++j)if(j!=a&&j!=n&&j!=c){auto q=polygon[j].position;if(cross2(A,B,q,p.normal)>=-1.e-8f&&cross2(B,C,q,p.normal)>=-1.e-8f&&cross2(C,A,q,p.normal)>=-1.e-8f){contains=true;break;}}
            if(contains)continue;triangles.push_back({polygon[a],polygon[n],polygon[c]});polygon.erase(polygon.begin()+n);ear=true;break;}
            if(!ear){triangles.clear();break;}}
        if(polygon.size()==3)triangles.push_back({polygon[0],polygon[1],polygon[2]});
        for(const auto&point:loop){const size_t count=triangles.size();for(size_t n=0;n<count;++n)for(unsigned edge=0;edge<3;++edge){auto triangle=triangles[n];const unsigned b=(edge+1)%3,c=(edge+2)%3;auto aPos=triangle[edge].position,bPos=triangle[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;
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
        auto indices=mapbox::earcut<uint32_t>(polygon);std::vector<Triangle>triangles;double measured=0;
        for(size_t k=0;k+2<indices.size();k+=3){Triangle t={vertices[indices[k]],vertices[indices[k+1]],vertices[indices[k+2]]};if(cross2(t[0].position,t[1].position,t[2].position,p.normal)<0)std::swap(t[1],t[2]);const double a=storm_ship_hull::area(t);if(a>1.e-10){measured+=a;triangles.push_back(t);}}
        // Invalid authored/self-crossing rings must not fill a hold or port.
        if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4))continue;
        // Restore collinear source subdivisions omitted by earcut. This keeps
        // the cap conforming to the original surface on the next saved cut.
        for(const auto&point:vertices){const size_t count=triangles.size();for(size_t k=0;k<count;++k)for(unsigned edge=0;edge<3;++edge){auto t=triangles[k];const unsigned b=(edge+1)%3,c=(edge+2)%3;const auto aPos=t[edge].position,bPos=t[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;const auto line=bPos-aPos,delta=point.position-aPos;const float length2=storm_ship_hull::dot(line,line),along=storm_ship_hull::dot(delta,line);if(along<=0||along>=length2||storm_ship_hull::length(storm_ship_hull::cross(line,delta))>1.e-5f*std::sqrt(length2))continue;triangles[k]={t[edge],point,t[c]};triangles.push_back({point,t[b],t[c]});break;}}
        for(auto t:triangles){for(auto&v:t){v.normal=p.normal;if(p.charred)v.color=(v.color&0xff000000u)|0x00645140u;for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=storm_ship_hull::dot(v.position,basis.tangent)*.12f;v.uv[uv*2+1]=storm_ship_hull::dot(v.position,basis.bitangent)*.12f;}}kept[rings[n].object].push_back(t);std::swap(t[1],t[2]);for(auto&v:t)v.normal=p.normal*-1.f;detached[rings[n].object].push_back(t);}
    }
}

// Shared edges may be traversed in opposite directions. Use one endpoint order
// for their intersections so repeated fractures do not open float-sized cracks.
inline void linearSplit(const std::vector<Vertex>&polygon,const storm_ship_hull::Plane&p,std::vector<Vertex>&inside,std::vector<Vertex>&outside) {
    inside.clear();outside.clear();
    const auto side=[&](Vec3 v){return double(p.normal.x)*v.x+double(p.normal.y)*v.y+double(p.normal.z)*v.z-p.distance;};
    const auto before=[](Vec3 a,Vec3 b){return a.x<b.x||(a.x==b.x&&(a.y<b.y||(a.y==b.y&&a.z<b.z)));};
    for(size_t n=0;n<polygon.size();++n){const auto&a=polygon[n];const auto&b=polygon[(n+1)%polygon.size()];const double da=side(a.position),db=side(b.position);const bool ai=da<=0,bi=db<=0;(ai?inside:outside).push_back(a);
        if(ai!=bi){const auto&first=before(a.position,b.position)?a:b;const auto&last=before(a.position,b.position)?b:a;const double d=side(first.position);auto vertex=storm_ship_hull::interpolate(first,last,float(std::clamp(d/(d-side(last.position)),0.,1.)));inside.push_back(vertex);outside.push_back(vertex);}}
}
inline Split planarSplit(const Mesh&mesh,const Plane&p,bool capped=true,bool stable=false,const std::map<int,float>*wood=nullptr) {
    Split result;std::vector<Segment>segments;
    for(const auto&o:mesh){result.kept.try_emplace(o.first);result.detached.try_emplace(o.first);
        for(const auto&t:o.second){std::vector<Vertex>inside,outside;if(stable)linearSplit(std::vector<Vertex>(t.begin(),t.end()),{p.normal,p.distance},inside,outside);else storm_ship_hull::split(std::vector<Vertex>(t.begin(),t.end()),{p.normal,p.distance},inside,outside);
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
    for(const auto&o:mesh)for(const auto&t:o.second){float lo=std::numeric_limits<float>::infinity(),hi=-lo;for(const auto&v:t){const float d=storm_ship_hull::dot(p.normal,v.position)-p.distance;lo=std::min(lo,d);hi=std::max(hi,d);}const bool local=lo<=p.roughness&&hi>=-p.roughness;active.push_back(local);if(!local)continue;
        for(unsigned c=0;c<3;++c){auto a=positionKey(t[c].position),b=positionKey(t[(c+1)%3].position);if(a==b)continue;if(b<a)std::swap(a,b);auto inserted=edgeKnots.try_emplace({a,b});if(!inserted.second)continue;double low=grain(t[c].position),high=grain(t[(c+1)%3].position);if(high<low)std::swap(low,high);const double first=std::floor((low+p.phase)/p.spacing)+1,last=std::ceil((high+p.phase)/p.spacing)-1;if(last-first+1>256)return false;for(double cell=first;cell<=last;++cell)inserted.first->second.push_back(float(cell*p.spacing-p.phase));}}
    for(const auto&o:mesh)for(const auto&t:o.second){auto&destination=out[o.first];if(active[face++]){if(!refine(t,p,destination,count))return false;continue;}
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
    const auto appendCap=[&](const Mesh&caps,Mesh&destination){for(const auto&o:caps)for(const auto&t:o.second){std::vector<Triangle>triangles;if(!refine(t,p,triangles,count))return false;
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
struct Surface {Mesh outer,canonicalCaps,interior;};
inline void appendMesh(Mesh&destination,const Mesh&source){for(const auto&o:source){auto&out=destination[o.first];out.insert(out.end(),o.second.begin(),o.second.end());}}
inline Mesh canonicalMesh(const Surface&surface){Mesh result=surface.outer;appendMesh(result,surface.canonicalCaps);return result;}
inline Mesh renderMesh(const Surface&surface){Mesh result=surface.outer;appendMesh(result,surface.interior);return result;}
inline Surface timberSurface(Mesh mesh,const std::map<int,float>&wood){Surface result;auto rendered=shellForRender(mesh,wood);for(auto&o:rendered){const size_t original=mesh.at(o.first).size();if(o.second.size()>original)result.interior[o.first].assign(o.second.begin()+original,o.second.end());}result.outer=std::move(mesh);return result;}
inline bool surfaceCaps(std::vector<Segment>seam,const Plane&p,Mesh&kept,Mesh&detached,const std::map<int,float>*wood=nullptr){
    if(p.roughness==0){cap(std::move(seam),p,kept,detached,wood);return true;}
    for(auto&e:seam){e.a.position=e.a.position-p.normal*offset(p,e.a.position);e.b.position=e.b.position-p.normal*offset(p,e.b.position);}
    Mesh a,b;cap(std::move(seam),{p.normal,p.distance,false},a,b,wood);size_t count=0;
    const auto append=[&](const Mesh&source,Mesh&destination){for(const auto&o:source)for(const auto&t:o.second){std::vector<Triangle>triangles;if(!refine(t,p,triangles,count))return false;for(auto triangle:triangles){for(auto&v:triangle)v.position=v.position+p.normal*offset(p,v.position);const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position));for(auto&v:triangle)v.normal=normal;destination[o.first].push_back(triangle);}}return true;};
    return append(a,kept)&&append(b,detached);
}
struct SurfaceSplit {Surface kept,detached;float keptArea=0,detachedArea=0;std::vector<Segment>seam;bool complete=true;};
// A finite cutter intersects real material cross-sections. Nested contours of
// one authored object describe a cavity, not two independent filled disks.
inline bool finiteSurfaceCaps(const std::vector<Segment>&seam,const Plane&p,Mesh&kept,Mesh&detached,const std::map<int,float>*wood=nullptr){
    std::vector<Segment>timber;std::map<int,std::vector<Segment>>objects;
    for(const auto&edge:seam)(wood&&wood->count(edge.object)?timber:objects[edge.object]).push_back(edge);
    if(!timber.empty()&&!surfaceCaps(std::move(timber),p,kept,detached,wood))return false;
    for(auto&entry:objects){const std::map<int,float>solid{{entry.first,1.f}};if(!surfaceCaps(std::move(entry.second),p,kept,detached,&solid))return false;}return true;
}
inline bool subtractFace(const Triangle&t,const std::vector<Vertex>&removed,std::vector<Triangle>&outside){
    if(storm_ship_hull::area(t)<=1.e-10f){outside.push_back(t);return true;}
    const auto origin=t[0].position;std::array<double,3>u{double(t[1].position.x)-origin.x,double(t[1].position.y)-origin.y,double(t[1].position.z)-origin.z},w{double(t[2].position.x)-origin.x,double(t[2].position.y)-origin.y,double(t[2].position.z)-origin.z};
    std::array<double,3>n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};const double ul=std::sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]),nl=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(nl<1.e-14||ul<1.e-14){outside.push_back(t);return true;}for(unsigned k=0;k<3;++k){u[k]/=ul;n[k]/=nl;}const std::array<double,3>v{n[1]*u[2]-n[2]*u[1],n[2]*u[0]-n[0]*u[2],n[0]*u[1]-n[1]*u[0]};const Vec3 normal{float(n[0]),float(n[1]),float(n[2])};
    const auto project=[&](Vec3 point){const std::array<double,3>d{double(point.x)-origin.x,double(point.y)-origin.y,double(point.z)-origin.z};return FacePoint{d[0]*u[0]+d[1]*u[1]+d[2]*u[2],d[0]*v[0]+d[1]*v[1]+d[2]*v[2]};};
    FaceRing triangle,clip;for(const auto&point:t)triangle.push_back(project(point.position));for(const auto&point:removed)clip.push_back(project(point.position));
    std::vector<FacePolygon>polygons;if(!differenceFace(triangle,clip,polygons))return false;
    const double denominator=triangle[1][0]*triangle[2][1]-triangle[1][1]*triangle[2][0];if(std::abs(denominator)<1.e-14)return false;
    const auto vertex=[&](FacePoint point){for(unsigned n=0;n<3;++n)if(std::hypot(point[0]-triangle[n][0],point[1]-triangle[n][1])<1.e-6)return t[n];
        for(size_t n=0;n<clip.size();++n)if(std::hypot(point[0]-clip[n][0],point[1]-clip[n][1])<1.e-6)return removed[n];
        const float b=float((point[0]*triangle[2][1]-point[1]*triangle[2][0])/denominator),c=float((triangle[1][0]*point[1]-triangle[1][1]*point[0])/denominator);
        Vertex result=t[0];const float a=1-b-c;result.position=t[0].position*a+t[1].position*b+t[2].position*c;result.normal=t[0].normal*a+t[1].normal*b+t[2].normal*c;
        for(unsigned n=0;n<8;++n)result.uv[n]=t[0].uv[n]*a+t[1].uv[n]*b+t[2].uv[n]*c;result.color=0;for(unsigned n=0;n<4;++n){float value=float((t[0].color>>(n*8))&255u)*a+float((t[1].color>>(n*8))&255u)*b+float((t[2].color>>(n*8))&255u)*c;result.color|=uint32_t(std::clamp(std::lround(value),0l,255l))<<(n*8);}return result;};
    for(const auto&polygon:polygons){std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&point:ring)vertices.push_back(vertex(point));auto indices=mapbox::earcut<uint32_t>(polygon);
        std::vector<Triangle>faces;for(size_t n=0;n+2<indices.size();n+=3){Triangle face{vertices[indices[n]],vertices[indices[n+1]],vertices[indices[n+2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(face[1].position-face[0].position,face[2].position-face[0].position),normal)<0)std::swap(face[1],face[2]);if(storm_ship_hull::area(face)>1.e-10f)faces.push_back(face);}
        // Earcut may elide collinear contour vertices; restore those edge
        // subdivisions so adjacent authored triangles and cut walls agree.
        for(const auto&point:vertices){const size_t count=faces.size();for(size_t n=0;n<count;++n)for(unsigned edge=0;edge<3;++edge){auto face=faces[n];const unsigned b=(edge+1)%3,c=(edge+2)%3;const auto aPos=face[edge].position,bPos=face[b].position;if(near(point.position,aPos)||near(point.position,bPos))continue;const auto line=bPos-aPos,delta=point.position-aPos;const float length2=storm_ship_hull::dot(line,line),along=storm_ship_hull::dot(delta,line);if(along<=0||along>=length2||storm_ship_hull::length(storm_ship_hull::cross(line,delta))>1.e-6f*std::sqrt(length2))continue;faces[n]={face[edge],point,face[c]};faces.push_back({point,face[b],face[c]});break;}}
        outside.insert(outside.end(),faces.begin(),faces.end());}
    return true;
}
inline std::vector<Mesh> materialSolids(const Surface&surface,const std::set<int>&affected,const std::map<int,float>&wood){
    std::vector<Mesh>result;Mesh timber;for(const auto&o:surface.outer)if(wood.count(o.first)){timber[o.first]=o.second;auto found=surface.interior.find(o.first);if(found!=surface.interior.end())timber[o.first].insert(timber[o.first].end(),found->second.begin(),found->second.end());}
    bool touched=false;for(int object:affected)touched|=wood.count(object)>0;
    if(touched&&closedCutVolume(timber)>0)result.push_back(std::move(timber));
    for(int object:affected)if(!wood.count(object)){Mesh solid;auto found=surface.outer.find(object);if(found!=surface.outer.end())solid[object]=found->second;found=surface.canonicalCaps.find(object);if(found!=surface.canonicalCaps.end())appendMesh(solid,Mesh{{object,found->second}});if(closedCutVolume(solid)>0)result.push_back(std::move(solid));}return result;
}
inline bool solidSectionCaps(const Mesh&solid,const storm_ship_hull::Plane&plane,const std::vector<storm_ship_hull::Plane>&limits,Vec3 origin,bool charred,Mesh&inside,Mesh&outside){
    origin=origin+plane.normal*(plane.distance-storm_ship_hull::dot(plane.normal,origin));
    const auto basis=storm_ship_hull::frame(plane.normal);std::vector<FaceRing>sections;
    const auto side=[&](Vec3 p){return double(plane.normal.x)*p.x+double(plane.normal.y)*p.y+double(plane.normal.z)*p.z-plane.distance;};
    for(const auto&o:solid)for(const auto&t:o.second){const Vec3 a=t[0].position-origin,b=t[1].position-origin,c=t[2].position-origin;const double signedVolume=double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x);if(std::abs(signedVolume)<1.e-12)continue;
        const std::array<Vec3,4>tetra{{origin,t[0].position,t[1].position,t[2].position}};bool inward=false;for(const auto&p:tetra)inward|=side(p)<0;if(!inward)continue;bool far=false;for(const auto&limit:limits){bool beyond=true;for(const auto&p:tetra)beyond&=double(limit.normal.x)*p.x+double(limit.normal.y)*p.y+double(limit.normal.z)*p.z>limit.distance;if(beyond){far=true;break;}}if(far)continue;
        std::vector<Vertex>points;for(unsigned x=0;x<4;++x)for(unsigned y=x+1;y<4;++y){const auto first=tetra[x],last=tetra[y];const double da=side(first),db=side(last);if(da!=0&&db!=0&&(da<0)==(db<0))continue;
            const double along=da==0?0:db==0?1:da/(da-db);Vertex vertex;vertex.position={float(first.x+(double(last.x)-first.x)*along),float(first.y+(double(last.y)-first.y)*along),float(first.z+(double(last.z)-first.z)*along)};bool duplicate=false;for(const auto&point:points)duplicate|=positionKey(point.position)==positionKey(vertex.position);if(!duplicate)points.push_back(vertex);}
        if(points.size()<3)continue;Vec3 center{};for(const auto&point:points)center=center+point.position*(1.f/points.size());std::sort(points.begin(),points.end(),[&](const auto&a,const auto&b){return std::atan2(storm_ship_hull::dot(a.position-center,basis.bitangent),storm_ship_hull::dot(a.position-center,basis.tangent))<std::atan2(storm_ship_hull::dot(b.position-center,basis.bitangent),storm_ship_hull::dot(b.position-center,basis.tangent));});
        for(const auto&limit:limits){std::vector<Vertex>in,out;linearSplit(points,limit,in,out);points=std::move(in);if(points.size()<3)break;}if(points.size()<3)continue;FaceRing ring;for(const auto&point:points){const auto p=point.position-origin;ring.push_back({double(storm_ship_hull::dot(p,basis.tangent)),double(storm_ship_hull::dot(p,basis.bitangent))});}double area=0;for(size_t n=0;n<ring.size();++n)area+=ring[n][0]*ring[(n+1)%ring.size()][1]-ring[(n+1)%ring.size()][0]*ring[n][1];if(std::abs(area)<1.e-12)continue;if((area<0)!=(signedVolume<0))std::reverse(ring.begin(),ring.end());sections.push_back(std::move(ring));}
    std::vector<FacePolygon>polygons;if(!unionFaces(sections,polygons))return false;const int object=solid.begin()->first;
    for(const auto&polygon:polygons){std::vector<Vertex>vertices;for(const auto&ring:polygon)for(const auto&p:ring){Vertex v;v.position={float(origin.x+p[0]*basis.tangent.x+p[1]*basis.bitangent.x),float(origin.y+p[0]*basis.tangent.y+p[1]*basis.bitangent.y),float(origin.z+p[0]*basis.tangent.z+p[1]*basis.bitangent.z)};v.normal=plane.normal;v.color=charred?0xff645140u:solid.begin()->second.front()[0].color;for(unsigned uv=0;uv<4;++uv){v.uv[uv*2]=float(p[0])*.12f;v.uv[uv*2+1]=float(p[1])*.12f;}vertices.push_back(v);}
        const auto indices=mapbox::earcut<uint32_t>(polygon);for(size_t n=0;n+2<indices.size();n+=3){Triangle t{vertices[indices[n]],vertices[indices[n+1]],vertices[indices[n+2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position),plane.normal)<0)std::swap(t[1],t[2]);if(storm_ship_hull::area(t)<=1.e-10f)continue;inside[object].push_back(t);std::swap(t[1],t[2]);for(auto&v:t)v.normal=plane.normal*-1.f;outside[object].push_back(t);}}
    return true;
}
inline std::vector<Segment> planeSeam(const Mesh&mesh,const storm_ship_hull::Plane&plane){
    std::vector<Segment> result;
    for(const auto&o:mesh)for(const auto&t:o.second){float d[3];bool positive=false,negative=false;
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
    Mesh tagged;unsigned role=0;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior}){for(const auto&o:*mesh)tagged[o.first*3+int(role)]=o.second;++role;}
    auto cut=split(tagged,p,nullptr,false);SurfaceSplit result;result.complete=cut.complete;if(!cut.complete)return result;
    const auto unpack=[](Mesh&mesh,Surface&out){for(auto&o:mesh){auto&destination=o.first%3==0?out.outer:o.first%3==1?out.canonicalCaps:out.interior;destination[o.first/3]=std::move(o.second);}};unpack(cut.kept,result.kept);unpack(cut.detached,result.detached);
    const auto measured=[](const Surface&s){float total=0;for(const auto*mesh:{&s.outer,&s.canonicalCaps})for(const auto&o:*mesh)for(const auto&t:o.second)total+=storm_ship_hull::area(t);return total;};result.keptArea=measured(result.kept);result.detachedArea=measured(result.detached);
    std::vector<Segment>canonical,render;for(auto edge:cut.seam){const int role=edge.object%3;edge.object/=3;if(role!=2)canonical.push_back(edge);if(role!=1)render.push_back(edge);}result.seam=canonical;
    if(!surfaceCaps(std::move(canonical),p,result.kept.canonicalCaps,result.detached.canonicalCaps)||!surfaceCaps(std::move(render),p,result.kept.interior,result.detached.interior,&wood))result.complete=false;
    return result;
}
// Subtract each delivered prism, preserving every outside triangle and clipping
// only its actual boundary caps. Sequential halfspace solids would leave hidden
// full-size partition walls; here every new wall is bounded by all other faces.
inline SurfaceSplit splitSurface(const Surface&surface,const Plane&p,const std::map<int,float>&wood){
    if(p.beams.empty())return splitPlaneSurface(surface,p,wood);
    SurfaceSplit result;result.kept=surface;if(!valid(p)){result.complete=false;return result;}
    const auto trim=[](Mesh mesh,const std::vector<storm_ship_hull::Plane>&planes){
        for(const auto&face:planes){Mesh next;for(const auto&o:mesh)for(const auto&t:o.second){std::vector<Vertex>in,out;
            linearSplit(std::vector<Vertex>(t.begin(),t.end()),face,in,out);storm_ship_hull::fan(in,next[o.first]);}mesh=std::move(next);}return mesh;};
    for(const auto&beam:p.beams){const auto faces=beamPlanes(beam);const auto source=std::move(result.kept);Surface kept,detached;std::set<int>affected;
        const auto subtract=[&](const Mesh&mesh,Mesh&outside,Mesh&inside){for(const auto&o:mesh){outside.try_emplace(o.first);for(const auto&t:o.second){
            bool far=false;for(const auto&face:faces){bool beyond=true;for(const auto&v:t)beyond&=storm_ship_hull::dot(face.normal,v.position)>face.distance;if(beyond){far=true;break;}}
            if(far){outside[o.first].push_back(t);continue;}std::vector<Vertex>remaining(t.begin(),t.end()),a,b;std::vector<Triangle>pieces,removed;
            for(const auto&face:faces){linearSplit(remaining,face,a,b);remaining.swap(a);if(remaining.size()<3)break;}
            storm_ship_hull::fan(remaining,removed);
            if(removed.empty()){outside[o.first].push_back(t);continue;}
            if(!subtractFace(t,remaining,pieces)){result.complete=false;return;}
            affected.insert(o.first);auto&out=outside[o.first];out.insert(out.end(),pieces.begin(),pieces.end());auto&in=inside[o.first];in.insert(in.end(),removed.begin(),removed.end());}}};
        subtract(source.outer,kept.outer,detached.outer);subtract(source.canonicalCaps,kept.canonicalCaps,detached.canonicalCaps);
        subtract(source.interior,kept.interior,detached.interior);
        if(!result.complete)return result;
        for(const auto*mesh:{&detached.outer,&detached.canonicalCaps})for(const auto&o:*mesh)for(const auto&t:o.second)result.detachedArea+=storm_ship_hull::area(t);
        const auto local=[&](const Mesh&mesh){Mesh out;for(int object:affected){auto found=mesh.find(object);if(found!=mesh.end())out.emplace(object,found->second);}return out;};
        const auto outer=local(source.outer),caps=local(source.canonicalCaps),interior=local(source.interior);
        const auto solids=materialSolids(source,affected,wood);
        for(size_t side=0;side<faces.size()&&!affected.empty();++side){const auto&face=faces[side];std::vector<storm_ship_hull::Plane>limits;
            for(size_t n=0;n<faces.size();++n)if(n!=side)limits.push_back(faces[n]);
            // Only the newly created cross-section belongs to this boundary;
            // pre-existing cap/interior triangles are handled by subtract above.
            auto canonical=planeSeam(outer,face),rendered=canonical;
            auto prior=planeSeam(caps,face);canonical.insert(canonical.end(),prior.begin(),prior.end());
            prior=planeSeam(interior,face);rendered.insert(rendered.end(),prior.begin(),prior.end());
            Mesh inCap,outCap,inRender,outRender;
            for(const auto&solid:solids)if(!solidSectionCaps(solid,face,limits,beamStart(beam),p.charred,inCap,outCap)){result.complete=false;return result;}
            appendMesh(kept.canonicalCaps,outCap);appendMesh(detached.canonicalCaps,inCap);
            appendMesh(kept.interior,outCap);appendMesh(detached.interior,inCap);
            for(auto edge:canonical){bool alive=true;for(const auto&limit:limits){const float a=storm_ship_hull::dot(limit.normal,edge.a.position)-limit.distance,b=storm_ship_hull::dot(limit.normal,edge.b.position)-limit.distance;
                if(a>0&&b>0){alive=false;break;}if((a>0)!=(b>0)){auto intersection=storm_ship_hull::interpolate(edge.a,edge.b,a/(a-b));if(a>0)edge.a=intersection;else edge.b=intersection;}}
                if(alive&&!near(edge.a.position,edge.b.position))result.seam.push_back(edge);}
        }
        result.kept=std::move(kept);appendMesh(result.detached.outer,detached.outer);appendMesh(result.detached.canonicalCaps,detached.canonicalCaps);appendMesh(result.detached.interior,detached.interior);
    }
    for(const auto*mesh:{&result.kept.outer,&result.kept.canonicalCaps})for(const auto&o:*mesh)for(const auto&t:o.second)result.keptArea+=storm_ship_hull::area(t);
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
    for(const auto&o:source)for(size_t n=0;n<o.second.size();++n){const size_t id=refs.size();refs.push_back({o.first,n});parent.push_back(id);const auto&t=o.second[n];
        for(unsigned c=0;c<3;++c){auto a=storm_ship_laser::positionKey(t[c].position),b=storm_ship_laser::positionKey(t[(c+1)%3].position);if(a==b)continue;auto ordered=std::minmax(a,b);
            auto inserted=edges.emplace(PositionEdge{ordered.first,ordered.second},id);if(!inserted.second){const size_t x=root(id),y=root(inserted.first->second);parent[x]=y;}}}
    std::map<size_t,Mesh>groups;for(size_t n=0;n<refs.size();++n)groups[root(n)][refs[n].object].push_back(source.at(refs[n].object)[refs[n].triangle]);
    std::vector<Mesh>result;result.reserve(groups.size());for(auto&entry:groups)result.push_back(std::move(entry.second));return result;
}
struct SurfaceTopology {
    struct Face {unsigned role;int object;size_t triangle;};
    std::vector<Face>faces;std::vector<size_t>parent;std::map<PositionKey,size_t>vertices;
    size_t root(size_t n){while(parent[n]!=n){parent[n]=parent[parent[n]];n=parent[n];}return n;}
    explicit SurfaceTopology(const Surface&surface){std::map<PositionEdge,std::vector<size_t>>edges;unsigned role=0;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior}){for(const auto&o:*mesh)for(size_t n=0;n<o.second.size();++n){const size_t id=faces.size();faces.push_back({role,o.first,n});parent.push_back(id);const auto&t=o.second[n];for(unsigned c=0;c<3;++c){auto a=positionKey(t[c].position),b=positionKey(t[(c+1)%3].position);vertices.emplace(a,id);if(a==b)continue;if(b<a)std::swap(a,b);auto&edge=edges[{a,b}];if(!edge.empty())parent[root(id)]=root(edge.front());edge.push_back(id);}}++role;}
        struct Edge{Vec3 a,b;size_t face;};std::vector<Edge>boundary;const auto point=[](const PositionKey&key){return Vec3{float(key[0])*.0001f,float(key[1])*.0001f,float(key[2])*.0001f};};for(const auto&edge:edges)if(edge.second.size()==1)boundary.push_back({point(edge.first.first),point(edge.first.second),edge.second.front()});
        if(boundary.size()<=4096)for(size_t n=0;n<boundary.size();++n)for(size_t m=n+1;m<boundary.size();++m){const auto&a=boundary[n];const auto&b=boundary[m];if(root(a.face)==root(b.face))continue;const auto line=a.b-a.a,first=b.a-a.a,last=b.b-a.a;const double length2=storm_ship_hull::dot(line,line),tolerance=.00015*std::sqrt(length2);if(storm_ship_hull::length(storm_ship_hull::cross(line,first))>tolerance||storm_ship_hull::length(storm_ship_hull::cross(line,last))>tolerance)continue;const double low=storm_ship_hull::dot(first,line)/length2,high=storm_ship_hull::dot(last,line)/length2;if(std::min(1.,std::max(low,high))-std::max(0.,std::min(low,high))>1.e-5)parent[root(a.face)]=root(b.face);}}
};
inline const Mesh&surfaceRole(const Surface&surface,unsigned role){return role==0?surface.outer:role==1?surface.canonicalCaps:surface.interior;}
inline Mesh&surfaceRole(Surface&surface,unsigned role){return role==0?surface.outer:role==1?surface.canonicalCaps:surface.interior;}
inline bool pointOnFace(Vec3 point,const Triangle&t){const auto u=t[1].position-t[0].position,v=t[2].position-t[0].position,w=point-t[0].position,n=storm_ship_hull::cross(u,v);const double length2=storm_ship_hull::dot(n,n);if(length2<1.e-16||std::abs(storm_ship_hull::dot(n,w))>1.e-5*std::sqrt(length2))return false;
    const double uu=storm_ship_hull::dot(u,u),uv=storm_ship_hull::dot(u,v),vv=storm_ship_hull::dot(v,v),wu=storm_ship_hull::dot(w,u),wv=storm_ship_hull::dot(w,v),d=uu*vv-uv*uv;if(d<=1.e-16)return false;const double a=(wu*vv-wv*uv)/d,b=(wv*uu-wu*uv)/d;return a>=-1.e-5&&b>=-1.e-5&&a+b<=1.00001;}
// Local carving may release a newly enclosed patch, but never reclassifies
// authored disconnected objects as wreckage. Each source component retains its
// actual extremal boundary witnesses; ambiguous transverse splits stay attached.
inline std::vector<Surface> detachEnclosed(const Surface&before,Surface&after){
    SurfaceTopology source(before),current(after);std::map<size_t,std::array<PositionKey,6>>witnesses;std::map<size_t,std::array<float,6>>extrema;
    const std::array<Vec3,6>directions{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    for(const auto&entry:source.vertices){const size_t component=source.root(entry.second);const Vec3 point{float(entry.first[0])*.0001f,float(entry.first[1])*.0001f,float(entry.first[2])*.0001f};auto inserted=extrema.try_emplace(component);if(inserted.second)inserted.first->second.fill(-INFINITY);
        for(unsigned n=0;n<6;++n){const float value=storm_ship_hull::dot(point,directions[n]);if(value>inserted.first->second[n]){inserted.first->second[n]=value;witnesses[component][n]=entry.first;}}}
    std::set<size_t>anchored;std::map<size_t,std::set<size_t>>sourceAnchors;
    for(const auto&entry:witnesses)for(const auto&point:entry.second){auto found=current.vertices.find(point);if(found!=current.vertices.end()){const size_t component=current.root(found->second);anchored.insert(component);sourceAnchors[entry.first].insert(component);}}
    std::map<size_t,std::set<size_t>>parents;for(const auto&entry:current.vertices){auto old=source.vertices.find(entry.first);if(old!=source.vertices.end())parents[current.root(entry.second)].insert(source.root(old->second));}
    std::map<size_t,size_t>representatives;for(size_t n=0;n<current.faces.size();++n)if(current.faces[n].role==0)representatives.try_emplace(current.root(n),n);
    for(const auto&entry:representatives)if(!anchored.count(entry.first)&&parents[entry.first].empty()){const auto&ref=current.faces[entry.second];const auto&t=after.outer.at(ref.object)[ref.triangle];const Vec3 point=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);
        for(size_t n=0;n<source.faces.size();++n){const auto&old=source.faces[n];if(old.role==0&&old.object==ref.object&&pointOnFace(point,before.outer.at(old.object)[old.triangle]))parents[entry.first].insert(source.root(n));}}
    std::set<size_t>released;for(const auto&entry:parents)if(!anchored.count(entry.first)&&!entry.second.empty()){bool supportedElsewhere=true;for(size_t parent:entry.second)supportedElsewhere&=!sourceAnchors[parent].empty();if(supportedElsewhere)released.insert(entry.first);}
    if(released.empty())return {};Surface retained;for(const auto*mesh:{&after.outer,&after.canonicalCaps,&after.interior})for(const auto&o:*mesh)retained.outer.try_emplace(o.first);
    std::map<size_t,Surface>pieces;for(size_t n=0;n<current.faces.size();++n){const auto&ref=current.faces[n];const size_t component=current.root(n);auto&destination=released.count(component)?pieces[component]:retained;surfaceRole(destination,ref.role)[ref.object].push_back(surfaceRole(after,ref.role).at(ref.object)[ref.triangle]);}
    after=std::move(retained);std::vector<Surface>result;for(auto&piece:pieces)result.push_back(std::move(piece.second));return result;
}

inline bool beamSeversSupport(const Mesh&before,const Mesh&after,const std::vector<BeamSample>&samples,Vec3 axis){
    if(std::abs(storm_ship_hull::length(axis)-1)> .001f)return false;
    const auto authored=components(before);
    for(const auto&beam:samples){const auto direction=beamDirection(beam);const auto center=beamStart(beam)+direction*(beam.depth*.5f);
        const float middle=storm_ship_hull::dot(center,axis),reach=std::abs(storm_ship_hull::dot(direction,axis))*beam.depth*.5f+beam.radius*1.083f+.01f;
        bool spanning=false;for(const auto&component:authored){float lo=INFINITY,hi=-INFINITY;for(const auto&o:component)for(const auto&t:o.second)for(const auto&v:t){const float position=storm_ship_hull::dot(v.position,axis);lo=std::min(lo,position);hi=std::max(hi,position);}spanning|=lo<middle-reach&&hi>middle+reach;}
        // Overlapping authored pole sections need not share GM edges. Require
        // disappearance of the entire actual support section, never a bbox hit
        // or the disconnection of one decorative component.
        if(spanning&&!planeSeam(before,{axis,middle}).empty()&&planeSeam(after,{axis,middle}).empty())return true;
    }return false;
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
        for(size_t n=0;n<polygon.size();++n){const auto a=polygon[n],b=polygon[(n+1)%polygon.size()];if(!storm_ship_hull::finite(a))return {};const float da=storm_ship_hull::dot(p.normal,a)-p.distance,db=storm_ship_hull::dot(p.normal,b)-p.distance;
            if(da<=0)loop.push_back(a);if((da<=0)!=(db<=0))loop.push_back(a+(b-a)*(da/(da-db)));}
        if(loop.size()>128)return {};if(loop.size()<3)return {true,{}};return {true,{std::move(loop)}};}
    const float height=polygon.front().y;std::vector<Segment>edges;
    for(size_t n=0;n<polygon.size();++n){const auto a=polygon[n],b=polygon[(n+1)%polygon.size()];if(!storm_ship_hull::finite(a)||std::abs(a.y-height)>1.e-4f||near(a,b))return {};Vertex va,vb;va.position=a;vb.position=b;edges.push_back({va,vb,0});}
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

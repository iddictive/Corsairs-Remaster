#pragma once
#include "ship_hull_geometry.hpp"
// Mapbox earcut.hpp v3.2.3, c68c8835ccff2b7532d31d8fa8dfcf398f629498 (ISC).
#include "third_party/mapbox-earcut/earcut.hpp"
#include <unordered_map>
#include "ship_hull_damage_bridge.hpp"
#include <map>
#include <limits>
#include <numeric>
#include <utility>

// A deterministic, node-local half-space split of the actual static GM mesh.
// Vertex interpolation/layout remain owned by ship_hull_geometry.hpp.
namespace storm_ship_laser {
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_hull::Bounds;
using Mesh = std::map<int,std::vector<Triangle>>;
using PositionKey=std::array<int64_t,3>;
using PositionEdge=std::pair<PositionKey,PositionKey>;
struct PositionEdgeHash {size_t operator()(const PositionEdge&e)const {size_t h=0;for(const auto&point:{e.first,e.second})for(auto q:point)h^=std::hash<int64_t>{}(q)+0x9e3779b9u+(h<<6)+(h>>2);return h;}};
inline PositionKey positionKey(Vec3 p) {return {int64_t(std::llround(double(p.x)*10000)),int64_t(std::llround(double(p.y)*10000)),int64_t(std::llround(double(p.z)*10000))};}
struct Plane { Vec3 normal; float distance=0; bool charred=true; Vec3 grain{0,1,0}; float roughness=0,spacing=1,phase=0; };
inline bool valid(const Plane&p) {
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
inline float distance(const Plane&p,Vec3 v) { return storm_ship_hull::dot(p.normal,v)-p.distance-offset(p,v); }
struct Segment { Vertex a,b; int object=0; };
// Open authored GM shells have no validated volume. These are measured surface
// properties (a uniform surface-density proxy), never a claim of solid wood mass.
struct Fragment { uint64_t fragment=0; Bounds bounds{}; Vec3 centroid{}; float surfaceArea=0; std::array<Vec3,14>support{}; };
struct Result { bool changed=false; uint64_t fragment=0; float removedFraction=0; Bounds keptBounds{}; bool volumeBased=false; std::vector<Segment>seam; std::vector<Fragment>fragments; };
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
inline Mesh shellForRender(const Mesh&mesh,const std::map<int,float>&thicknesses) {
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
    const auto inward=[&](const Vertex&v){const auto&offset=offsets.at(key(v.position));const auto normal=storm_ship_hull::length(offset.normal)>1.e-6f?storm_ship_hull::normalized(offset.normal):storm_ship_hull::normalized(v.normal);
        return v.position-normal*float(offset.thickness/offset.count);};
    struct Boundary{Vertex a,b;int object=0;unsigned count=0;};std::map<Edge,Boundary>edges;
    for(const auto&o:mesh){const auto selected=thicknesses.find(o.first);if(selected==thicknesses.end()||!std::isfinite(selected->second)||selected->second<=0)continue;
        auto&out=result[o.first];
        for(const auto&t:o.second){const auto fallback=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));Triangle inner=t;
            for(unsigned c=0;c<3;++c){const auto normal=storm_ship_hull::length(t[c].normal)>1.e-6f?storm_ship_hull::normalized(t[c].normal):fallback;
                inner[c].position=inward(t[c]);inner[c].normal=normal*-1.f;darken(inner[c],.62f);
                auto a=key(t[c].position),b=key(t[(c+1)%3].position);if(a==b)continue;if(b<a)std::swap(a,b);auto&edge=edges[{a,b}];if(!edge.count){edge.a=t[c];edge.b=t[(c+1)%3];edge.object=o.first;
                    if(storm_ship_hull::length(edge.a.normal)<=1.e-6f)edge.a.normal=fallback;if(storm_ship_hull::length(edge.b.normal)<=1.e-6f)edge.b.normal=fallback;}++edge.count;}
            std::swap(inner[1],inner[2]);out.push_back(inner);}}
        for(const auto&entry:edges){const auto&edge=entry.second;if(edge.count!=1)continue;auto&out=result[edge.object];const float thickness=thicknesses.at(edge.object);auto a=edge.a,b=edge.b,c=b,d=a;
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
inline SurfaceSplit splitSurface(const Surface&surface,const Plane&p,const std::map<int,float>&wood){
    Mesh tagged;unsigned role=0;for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior}){for(const auto&o:*mesh)tagged[o.first*3+int(role)]=o.second;++role;}
    auto cut=split(tagged,p,nullptr,false);SurfaceSplit result;result.complete=cut.complete;if(!cut.complete)return result;
    const auto unpack=[](Mesh&mesh,Surface&out){for(auto&o:mesh){auto&destination=o.first%3==0?out.outer:o.first%3==1?out.canonicalCaps:out.interior;destination[o.first/3]=std::move(o.second);}};unpack(cut.kept,result.kept);unpack(cut.detached,result.detached);
    const auto measured=[](const Surface&s){float total=0;for(const auto*mesh:{&s.outer,&s.canonicalCaps})for(const auto&o:*mesh)for(const auto&t:o.second)total+=storm_ship_hull::area(t);return total;};result.keptArea=measured(result.kept);result.detachedArea=measured(result.detached);
    std::vector<Segment>canonical,render;for(auto edge:cut.seam){const int role=edge.object%3;edge.object/=3;if(role!=2)canonical.push_back(edge);if(role!=1)render.push_back(edge);}result.seam=canonical;
    if(!surfaceCaps(std::move(canonical),p,result.kept.canonicalCaps,result.detached.canonicalCaps)||!surfaceCaps(std::move(render),p,result.kept.interior,result.detached.interior,&wood))result.complete=false;
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

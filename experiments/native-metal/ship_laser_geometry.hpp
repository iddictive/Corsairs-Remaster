#pragma once
#include "ship_hull_geometry.hpp"
#include <map>
#include <utility>

// A deterministic, node-local half-space split of the actual static GM mesh.
// Vertex interpolation/layout remain owned by ship_hull_geometry.hpp.
namespace storm_ship_laser {
using storm_ship_hull::Vec3;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using storm_ship_hull::Bounds;
using Mesh = std::map<int,std::vector<Triangle>>;
struct Plane { Vec3 normal; float distance=0; bool charred=true; };
inline bool valid(const Plane&p) { const float n=storm_ship_hull::length(p.normal);return storm_ship_hull::finite(p.normal)&&std::isfinite(p.distance)&&n>.999f&&n<1.001f; }
inline float distance(const Plane&p,Vec3 v) { return storm_ship_hull::dot(p.normal,v)-p.distance; }
struct Segment { Vertex a,b; int object=0; };
struct Result { bool changed=false; uint64_t fragment=0; float removedFraction=0; Bounds keptBounds{}; bool volumeBased=false; std::vector<Segment>seam; };
struct Split { Mesh kept,detached; float keptArea=0,detachedArea=0; std::vector<Segment>seam; };
inline bool near(Vec3 a,Vec3 b) { auto d=a-b;return storm_ship_hull::dot(d,d)<1.e-8f; }
inline Bounds bounds(const Mesh&m) { Bounds b{};bool first=true;for(const auto&o:m)for(const auto&t:o.second)for(const auto&v:t){if(first){b={v.position,v.position};first=false;}else b=storm_ship_hull::merge(b,{v.position,v.position});}return b; }
inline float cross2(Vec3 a,Vec3 b,Vec3 c,Vec3 normal) { return storm_ship_hull::dot(storm_ship_hull::cross(b-a,c-a),normal); }
inline double closedVolume(const Mesh&mesh) {
    using Point=std::array<int64_t,3>;using Edge=std::pair<Point,Point>;std::map<Edge,std::pair<unsigned,int>>edges;double volume=0;
    const auto key=[](Vec3 p)->Point{return {int64_t(std::llround(double(p.x)*10000)),int64_t(std::llround(double(p.y)*10000)),int64_t(std::llround(double(p.z)*10000))};};
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
inline void cap(std::vector<Segment> segments,const Plane&p,Mesh&kept,Mesh&detached) {
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
inline Split split(const Mesh&mesh,const Plane&p) {
    Split result;std::vector<Segment>segments;
    for(const auto&o:mesh){result.kept.try_emplace(o.first);result.detached.try_emplace(o.first);
        for(const auto&t:o.second){std::vector<Vertex>inside,outside;storm_ship_hull::split(std::vector<Vertex>(t.begin(),t.end()),{p.normal,p.distance},inside,outside);
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
    result.seam=segments;cap(std::move(segments),p,result.kept,result.detached);return result;
}
inline float trace(const Triangle&t,Vec3 start,Vec3 finish,float&a,float&b) {
    auto direction=finish-start,e1=t[1].position-t[0].position,e2=t[2].position-t[0].position,h=storm_ship_hull::cross(direction,e2);float det=storm_ship_hull::dot(e1,h);
    if(std::abs(det)<1.e-8f)return 2;float inv=1.f/det;auto s=start-t[0].position;a=inv*storm_ship_hull::dot(s,h);if(a<0||a>1)return 2;auto q=storm_ship_hull::cross(s,e1);b=inv*storm_ship_hull::dot(direction,q);if(b<0||a+b>1)return 2;float f=inv*storm_ship_hull::dot(e2,q);return f>=0&&f<=1?f:2;
}
}

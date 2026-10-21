#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>

// Static render geometry only. Original vertices/BSP remain the authority for
// collision. New points are cubic PN edge midpoints; UVs never cross a face.
namespace island_geometry {
using P = std::array<float,3>;
inline P add(P a,P b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline P sub(P a,P b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline P mul(P a,float s){return {a[0]*s,a[1]*s,a[2]*s};}
inline float dot(P a,P b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline float length(P a){return std::sqrt(dot(a,a));}
inline P unit(P a){float l=length(a);return l>.00001f?mul(a,1/l):P{0,1,0};}
struct Vertex {
    P p,n; uint32_t color; std::array<float,8> uv{};
};
static_assert(sizeof(Vertex)==60,"RDF static vertex prefix and UV layout");
struct Face {std::array<Vertex,3> v;int object,material;};
struct Group {P sum{},normal{};int material=-1;bool boundary=false,hard=false;std::vector<P> normals;};
struct Surface {
    std::map<P,Group> groups;
    std::map<std::pair<P,P>,unsigned> edgeCount;
    explicit Surface(const std::vector<Face>& faces){
        for(const auto&f:faces){
            for(const auto&v:f.v){auto&g=groups[v.p];g.sum=add(g.sum,unit(v.n));g.normals.push_back(unit(v.n));if(g.material==-1)g.material=f.material;else if(g.material!=f.material)g.material=-2;}
            for(unsigned i=0;i<3;i++){P a=f.v[i].p,b=f.v[(i+1)%3].p;if(b<a)std::swap(a,b);++edgeCount[{a,b}];}
        }
        for(auto&entry:groups){auto&g=entry.second;g.normal=unit(g.sum);for(P n:g.normals)if(dot(n,g.normal)<.75f)g.hard=true;g.normals.clear();}
        for(const auto&e:edgeCount)if(e.second!=2){groups[e.first.first].boundary=true;groups[e.first.second].boundary=true;}
    }
    P midpoint(const Vertex&a,const Vertex&b)const{
        const P straight=mul(add(a.p,b.p),.5f),edge=sub(b.p,a.p);float extent=length(edge);
        // Island sea datum is zero; the entire 32-unit shore band is immutable.
        // Small triangles gain no subdivisions, and original corner positions
        // remain exact even on eligible slopes.
        if(a.p[1]<=32||b.p[1]<=32||extent<48)return straight;
        const auto&ga=groups.at(a.p);const auto&gb=groups.at(b.p);
        if(ga.boundary||gb.boundary||ga.hard||gb.hard||ga.material<0||ga.material!=gb.material)return straight;
        // Cubic PN Bezier edge at t=.5. Shared endpoint normals make this
        // exactly the same geometric point on both sides of a UV seam.
        P delta=mul(sub(mul(gb.normal,dot(edge,gb.normal)),mul(ga.normal,dot(edge,ga.normal))),.125f);
        float d=length(delta),limit=std::min(30.f,extent*.125f);if(d>limit)delta=mul(delta,limit/d);
        return add(straight,delta);
    }
    Vertex middle(const Vertex&a,const Vertex&b)const{
        Vertex v{};v.p=midpoint(a,b);v.n=unit(add(a.n,b.n));
        for(unsigned i=0;i<8;i++)v.uv[i]=(a.uv[i]+b.uv[i])*.5f;
        for(unsigned shift=0;shift<32;shift+=8)v.color|=(((a.color>>shift&255)+(b.color>>shift&255)+1)/2)<<shift;
        return v;
    }
    std::vector<Vertex> refine(const Face&f)const{
        const auto&a=f.v[0];const auto&b=f.v[1];const auto&c=f.v[2];
        Vertex ab=middle(a,b),bc=middle(b,c),ca=middle(c,a);
        float change=length(sub(ab.p,mul(add(a.p,b.p),.5f)))+length(sub(bc.p,mul(add(b.p,c.p),.5f)))+length(sub(ca.p,mul(add(c.p,a.p),.5f)));
        if(change<.05f)return {a,b,c};
        return {a,ab,ca, ab,b,bc, ca,bc,c, ab,bc,ca};
    }
};
struct Chunk {std::vector<Vertex> vertices;std::vector<uint16_t> indices;};
inline std::vector<Chunk> chunks(const Surface&surface,const std::vector<Face>&faces,int object){
    std::vector<Chunk> result;std::map<std::array<uint32_t,15>,uint16_t> lookup;
    for(const auto&f:faces)if(f.object==object){auto refined=surface.refine(f);for(size_t i=0;i<refined.size();i+=3){
        if(result.empty()||result.back().vertices.size()+3>60000){result.emplace_back();lookup.clear();}
        auto&chunk=result.back();for(size_t j=i;j<i+3;j++){std::array<uint32_t,15> key;std::memcpy(key.data(),&refined[j],60);auto found=lookup.find(key);if(found==lookup.end()){uint16_t index=static_cast<uint16_t>(chunk.vertices.size());chunk.vertices.push_back(refined[j]);lookup.emplace(key,index);chunk.indices.push_back(index);}else chunk.indices.push_back(found->second);}
    }}return result;
}
} // namespace island_geometry

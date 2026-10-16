#pragma once
#include "island_forest_bridge.hpp"
#include "island_geometry.hpp"
#include "engine/src/libs/location/src/forest_vendor/proctree.h"
#include <limits>

namespace storm_island_forest {
inline uint32_t hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16); }
inline float unit(uint32_t x) { return float(hash(x) >> 8) / 16777216.f; }
inline float smooth(float a, float b, float x) { float t=std::clamp((x-a)/(b-a),0.f,1.f);return t*t*(3.f-2.f*t); }
inline float groundSuitability(std::string_view texture) {
    const auto stem=island_geometry::textureStem(texture);
    // Material roles, not island names. Rock/reef/beach/town/masonry never grow
    // trees merely because they happen to be part of the primary island GM.
    return stem.starts_with("jungle") ? 1.f : 0.f;
}
inline bool terrainMaterial(std::string_view texture) {
    const auto stem=island_geometry::textureStem(texture);
    return island_geometry::refinableTerrainTexture(texture) || stem.starts_with("rockk") || stem.starts_with("jungle");
}
inline bool islandForestConsumer(std::string_view path, std::string_view texture) {
    std::string p(path);for(char &c:p)c=c=='\\' ? '/' : char(std::tolower(static_cast<unsigned char>(c)));
    return p.find("/islands/")!=std::string::npos && p.ends_with(".grs") && island_geometry::textureStem(texture)=="junglesprites";
}
struct Triangle { Float3 p[3]; float normalY=0, suitability=0; bool terrain=false; };
struct Cell { float y=-INFINITY, normalY=0, suitability=0; };

// Bounded, resumable top-surface raster. The same jittered point is tested
// against every triangle, so an opaque roof above soil rejects that root.
class Placement {
    static constexpr unsigned MaximumCells=262144, MaximumRoots=32768;
    std::vector<Triangle> faces;
    std::vector<Cell> cells;
    Float3 low{INFINITY,INFINITY,INFINITY}, high{-INFINITY,-INFINITY,-INFINITY};
    float summit=0, spacing=0;
    uint32_t seed=0;
    unsigned nx=0,nz=0,face=0,finalCell=0;
    int x0=0,x1=-1,z0=0,z1=-1,x=0,z=0;
    bool rasterActive=false, sealed=false, complete=false;
    Float2 point(unsigned ix,unsigned iz) const {
        const uint32_t key=seed ^ hash(ix+nx*iz);
        return {low[0]+(float(ix)+.16f+.68f*unit(key))*spacing,
                low[2]+(float(iz)+.16f+.68f*unit(key^0x8f53u))*spacing};
    }
public:
    std::vector<Instance> roots;
    void add(const Triangle &t) {
        if(sealed)return;faces.push_back(t);
        for(const auto&p:t.p){for(unsigned j=0;j<3;j++){low[j]=std::min(low[j],p[j]);high[j]=std::max(high[j],p[j]);}if(t.terrain)summit=std::max(summit,p[1]);}
    }
    void seal(uint32_t identity) {
        if(sealed)return;sealed=true;seed=hash(identity);
        if(faces.empty()||!std::isfinite(summit)||summit<=0){complete=true;return;}
        spacing=std::max(42.f,std::sqrt(std::max(1.f,(high[0]-low[0])*(high[2]-low[2]))/float(MaximumCells-2048)));
        nx=std::max(1u,unsigned(std::ceil((high[0]-low[0])/spacing)));nz=std::max(1u,unsigned(std::ceil((high[2]-low[2])/spacing)));
        while(uint64_t(nx)*nz>MaximumCells){spacing*=1.01f;nx=std::max(1u,unsigned(std::ceil((high[0]-low[0])/spacing)));nz=std::max(1u,unsigned(std::ceil((high[2]-low[2])/spacing)));}
        cells.resize(size_t(nx)*nz);roots.reserve(std::min(size_t(MaximumRoots),cells.size()/4));
    }
    bool ready() const { return complete; }
    size_t triangleCount() const { return faces.size(); }
    float terrainSummit() const { return summit; }
    // Each unit is one triangle/cell relation or one finalized root, not a
    // whole large triangle. A single island face cannot monopolize a frame.
    void advance(unsigned budget) {
        if(!sealed||complete)return;
        while(budget&&face<faces.size()) {
            const auto&t=faces[face];
            if(!rasterActive){
                x0=std::clamp(int(std::floor((std::min({t.p[0][0],t.p[1][0],t.p[2][0]})-low[0])/spacing)),0,int(nx)-1);
                x1=std::clamp(int(std::floor((std::max({t.p[0][0],t.p[1][0],t.p[2][0]})-low[0])/spacing)),0,int(nx)-1);
                z0=std::clamp(int(std::floor((std::min({t.p[0][2],t.p[1][2],t.p[2][2]})-low[2])/spacing)),0,int(nz)-1);
                z1=std::clamp(int(std::floor((std::max({t.p[0][2],t.p[1][2],t.p[2][2]})-low[2])/spacing)),0,int(nz)-1);x=x0;z=z0;rasterActive=true;
            }
            const auto p=point(unsigned(x),unsigned(z));const auto&a=t.p[0];const auto&b=t.p[1];const auto&c=t.p[2];
            const float den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
            if(std::abs(den)>1.e-6f){
                const float u=((b[2]-c[2])*(p[0]-c[0])+(c[0]-b[0])*(p[1]-c[2]))/den;
                const float v=((c[2]-a[2])*(p[0]-c[0])+(a[0]-c[0])*(p[1]-c[2]))/den,w=1.f-u-v;
                if(std::min({u,v,w})>=-1.e-5f){const float y=u*a[1]+v*b[1]+w*c[1];auto&cell=cells[size_t(z)*nx+x];if(y>cell.y){cell={y,t.normalY,t.suitability};}}
            }
            --budget;if(++x>x1){x=x0;if(++z>z1){++face;rasterActive=false;}}
        }
        while(budget&&face==faces.size()&&finalCell<cells.size()) {
            const unsigned i=finalCell++,ix=i%nx,iz=i/nx;const auto&c=cells[i];--budget;
            if(c.suitability<=0||!std::isfinite(c.y)||c.y<=0||c.normalY<=.62f)continue;
            float coast=smooth(6.f,30.f,c.y);
            for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++){
                const int xx=int(ix)+dx,zz=int(iz)+dz;
                if(xx<0||zz<0||xx>=int(nx)||zz>=int(nz)||cells[size_t(zz)*nx+xx].y<=0)coast*=.42f;
            }
            const float elevation=1.f-smooth(.43f,.72f,c.y/std::max(1.f,summit));
            const float slope=smooth(.62f,.91f,c.normalY);
            const uint32_t key=seed^hash(i);const float density=c.suitability*coast*elevation*slope;
            if(unit(key^0x26c1u)>=density||roots.size()>=MaximumRoots)continue;
            const auto p=point(ix,iz);const float height=(46.f+30.f*unit(key^0x3752u))*(.85f+.15f*elevation);
            roots.push_back({{p[0],c.y-.15f,p[1]},height,unit(key^0x4a39u)*6.283185307f,unit(key^0x937du)*6.283185307f,.86f+.23f*unit(key^0x81edu),1.f});
        }
        if(finalCell==cells.size()){complete=true;std::vector<Triangle>().swap(faces);std::vector<Cell>().swap(cells);}
    }
};

struct Mesh { std::vector<Vertex> branches,leaves; };
inline Mesh makeMesh(unsigned variant,unsigned lod) {
    Proctree::Tree tree;auto&p=tree.mProperties;p.mSeed=262+int(variant)*173;p.mSegments=lod?4:6;p.mLevels=lod?2:3;p.mTreeSteps=lod?2:3;
    p.mTrunkLength=variant?2.1f:2.4f;p.mInitialBranchLength=variant?.64f:.55f;p.mTwigScale=variant?.54f:.48f;p.mClimbRate=.38f;p.mGrowAmount=.24f;tree.generate();
    float height=0;for(int i=0;i<tree.mVertCount;i++)height=std::max(height,tree.mVert[i].y);for(int i=0;i<tree.mTwigVertCount;i++)height=std::max(height,tree.mTwigVert[i].y);height=std::max(.01f,height);
    Mesh mesh;
    auto append=[&](auto*positions,auto*normals,auto*uvs,auto*indices,int count,bool leaf){
        auto&out=leaf?mesh.leaves:mesh.branches;out.reserve(size_t(count)*3);
        for(int i=0;i<count;i++){if(leaf&&i%4>=2)continue;const int ids[]={indices[i].x,indices[i].y,indices[i].z};for(int id:ids){const auto&v=positions[id];const auto&n=normals[id];const auto&uv=uvs[id];
            Vertex vertex{{v.x/height,v.y/height,v.z/height},{n.x,n.y,n.z},{uv.u,uv.v},leaf?0xffe5f0d5u:0xffb4a695u};
            // Visually inspected leafPalms broadleaf (first lower oval), not an
            // entire palm/tree sprite or the opaque kanion ground tile.
            if(leaf)vertex.uv={(4.f+uv.u*244.f)/2048.f,(323.f+uv.v*124.f)/512.f};
            out.push_back(vertex);
        }}
    };
    append(tree.mVert,tree.mNormal,tree.mUV,tree.mFace,tree.mFaceCount,false);append(tree.mTwigVert,tree.mTwigNormal,tree.mTwigUV,tree.mTwigFace,tree.mTwigFaceCount,true);return mesh;
}
}

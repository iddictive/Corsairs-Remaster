#include "island_geometry.hpp"
#include <cstdio>
#include <cstdlib>
using namespace island_geometry;
static void need(bool value,const char*message){if(!value){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
static P cross(P a,P b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static bool finite(Vertex v){for(float x:v.p)if(!std::isfinite(x))return false;for(float x:v.n)if(!std::isfinite(x))return false;return true;}
int main(){
    need(refinableTerrainTexture("RESOURCE\\Textures\\rockK2.tga.tx"),"rock texture qualifies as opaque terrain");
    need(refinableTerrainTexture("jungle_gray.tga"),"opaque jungle cover qualifies as terrain");
    for(const char* path:{"Islands/PuertoRico/PuertoRico.gm","RESOURCE\\MODELS\\Islands\\Nevis\\Nevis.gm","/models/islands/jamaica/jamaica.gm"})
        need(islandModelPath(path),"island component qualifies independent of path root and separators");
    for(const char* path:{"Locations/Town_PuertoRico/Town/SanJuan.gm","WorldMap/islands.gm","Ships/IslandsTrader/ship.gm","Islands/Nevis/Nevis_refl.gm","Islands/Nevis/Nevis_seabed.gm","Islands/Nevis/Nevis_locators.gm","Islands/Nevis/Nevis_fort1.gm"})
        need(!islandModelPath(path),"reflection, seabed, locator, fort and non-island roles stay authored");
    for(const char* texture:{"treePalms.tga","trees.tga","PalmsSprites.tga","rockKeep.tga","rockK.tga","rockK2_alpha.tga","beach.tga","reef1.tga","fortstone1.tga"})
        need(!refinableTerrainTexture(texture),"foliage, shore, reef and structure draws stay authored");
    std::vector<Face> faces;
    auto vertex=[](int x,int z){Vertex v{};float px=(x-2)*100.f,pz=(z-2)*100.f;v.p={px,400.f-.002f*(px*px+pz*pz),pz};v.n=unit({.004f*px,1,.004f*pz});v.color=0xff204060;v.uv={x*.5f,z*.5f,0,0,0,0,0,0};return v;};
    for(int z=0;z<4;z++)for(int x=0;x<4;x++){
        auto a=vertex(x,z),b=vertex(x+1,z),c=vertex(x+1,z+1),d=vertex(x,z+1);
        faces.push_back({{a,b,c},0,0});faces.push_back({{a,c,d},0,0});
    }
    Surface surface(faces);auto a=vertex(1,2),b=vertex(2,2);P midpoint=surface.midpoint(a,b);
    need(length(sub(midpoint,mul(add(a.p,b.p),.5f)))>2,"PN changes a straight geometric silhouette");
    auto seamA=a,seamB=b;seamA.uv[0]+=10;seamB.uv[0]+=10;
    need(surface.midpoint(seamB,seamA)==midpoint,"shared PN geometry is independent of UV seam and edge direction");
    auto originalMid=surface.middle(a,b),seamMid=surface.middle(seamA,seamB);
    need(seamMid.uv[0]-originalMid.uv[0]==10,"UV interpolation stays inside authored seam");
    need(originalMid.color==a.color,"authored packed color and alpha preserved");
    auto border=vertex(0,2);need(surface.midpoint(border,a)==mul(add(border.p,a.p),.5f),"open boundary is immutable");
    auto shore=a;shore.p[1]=0;need(surface.midpoint(shore,b)==mul(add(shore.p,b.p),.5f),"shoreline edge remains linear");
    size_t refinedCount=0;for(const auto&face:faces){auto out=surface.refine(face);refinedCount+=out.size()/3;P parent=cross(sub(face.v[1].p,face.v[0].p),sub(face.v[2].p,face.v[0].p));for(const auto&v:face.v){bool found=false;for(const auto&r:out)if(std::memcmp(&v,&r,sizeof(v))==0)found=true;need(found,"original corners remain byte exact");}for(const auto&r:out){need(finite(r),"generated attributes are finite");need(std::abs(length(r.n)-1)<1e-4f,"generated normal is unit length");need(r.p[1]>=std::min({face.v[0].p[1],face.v[1].p[1],face.v[2].p[1]})&&r.p[1]<=std::max({face.v[0].p[1],face.v[1].p[1],face.v[2].p[1]}),"new vertices stay inside authored vertical envelope");}for(size_t i=0;i<out.size();i+=3){P child=cross(sub(out[i+1].p,out[i].p),sub(out[i+2].p,out[i].p));need(length(child)>1e-4f&&dot(parent,child)>0,"child triangles retain finite area and winding");}}
    need(refinedCount>faces.size()&&refinedCount<faces.size()*4,"adaptive refinement avoids blanket 4x expansion");
    auto packed=chunks(surface,faces,0);for(const auto&chunk:packed){need(chunk.vertices.size()<=60000,"16-bit draw capacity");for(auto i:chunk.indices)need(i<chunk.vertices.size(),"render indices valid");}
    auto mixed=faces;for(auto&face:mixed)if(face.v[0].p[0]>=0)face.material=1;Surface materialBoundary(mixed);
    const auto&group=materialBoundary.groups.at(vertex(2,2).p);need(group.material==-2,"material seam detected");
    need(materialBoundary.midpoint(a,b)==mul(add(a.p,b.p),.5f),"material boundary geometry preserved");
    // Regression for the former face-sum split: a face may split another edge,
    // but a shared edge below the edge-owned threshold must stay unsplit on both
    // incident faces.  This forbids the non-collinear T-junction construction.
    auto q0=vertex(1,1),q1=vertex(2,1),q2=vertex(2,2),q3=vertex(1,2);
    std::vector<Face> pair{{{q0,q1,q2},0,0},{{q0,q2,q3},0,0}};Surface pairSurface(pair);
    for(const auto&face:pair)for(unsigned i=0;i<3;i++){const auto&x=face.v[i];const auto&y=face.v[(i+1)%3];need(pairSurface.splitEdge(x,y)==pairSurface.splitEdge(y,x),"undirected edge owns one split decision");}
    std::printf("PASS island conforming PN edges: %zu -> %zu triangles, curved midpoint %.3f units; fixed topology has no LOD transition and gameplay geometry is outside transform API\n",faces.size(),refinedCount,length(sub(midpoint,mul(add(a.p,b.p),.5f))));
}

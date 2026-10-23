// Offline only. Native island depth/AIFlowGraph and PtcData serialization owners
// remain in engine/src/libs/{island,sea_ai,location}; no runtime cache builder.
#include "../experiments/native-metal/ship_hull_geometry.hpp"
#include "clipper2/clipper.engine.h"
#include "clipper2/clipper.minkowski.h"
#include "clipper2/clipper.triangulation.h"
#include "clipper2/clipper.h"
#include "../experiments/native-metal/engine/src/libs/location/include/ptc.h"
#include <fstream>
#include <cfenv>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>

using namespace storm_ship_hull;
using namespace Clipper2Lib;
using Face = std::array<Vec3,3>;
template<class T> void read(std::istream& s,T& v) { s.read(reinterpret_cast<char*>(&v),sizeof(v)); if(!s) throw std::runtime_error("Truncated asset input"); }
template<class T> void write(std::ostream& s,const T& v) { s.write(reinterpret_cast<const char*>(&v),sizeof(v)); }
struct Body {
    float cx,cz,sx,sz,ex,ez;
    std::vector<Face> faces;
    std::vector<Vec3> points;
    std::vector<std::array<uint32_t,2>> edges;
    Broadphase index;
    explicit Body(const std::string& path) {
        std::ifstream s(path,std::ios::binary); uint64_t magic; read(s,magic);
        if(magic!=0x32303056414e5353ull) throw std::runtime_error("Unknown collision input");
        read(s,cx);read(s,cz);read(s,sx);read(s,sz);read(s,ex);read(s,ez);
        uint32_t n;read(s,n);if(n>4000000)throw std::runtime_error("Collision input exceeds finite budget");
        faces.resize(n);std::vector<Bounds> boxes;boxes.reserve(n);
        for(auto& t:faces){read(s,t);Bounds b{t[0],t[0]};for(auto p:t){if(!finite(p))throw std::runtime_error("Nonfinite collision face");b=merge(b,{p,p});}boxes.push_back(b);}
        read(s,n);if(n>65535)throw std::runtime_error("Graph point budget exceeded");points.resize(n);for(auto& p:points)read(s,p);
        read(s,n);if(n>10000000)throw std::runtime_error("Graph edge budget exceeded");edges.resize(n);for(auto& e:edges){read(s,e);if(e[0]>=points.size()||e[1]>=points.size())throw std::runtime_error("Graph endpoint outside source");}
        index.build(boxes);
    }
    // GEOM::TraceQuery uses double Moller/Trumbore, strict negative-det
    // barycentrics, and open segment endpoints. The ship laser's inclusive
    // float kernel differs at precisely those boundaries; reuse its broadphase
    // but retain the island's actual admission contract here.
    float trace(Vec3 start,Vec3 finish,Vec3* normal=nullptr) const {
        struct D {double x,y,z;D operator-(D b)const{return{x-b.x,y-b.y,z-b.z};}};
        auto cv=[](Vec3 p){return D{p.x,p.y,p.z};};
        auto cross=[](D a,D b){return D{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
        auto dot=[](D a,D b){return a.x*b.x+a.y*b.y+a.z*b.z;};
        std::vector<uint32_t> candidates;index.queryRay(start,finish,candidates);
        double nearest=2;uint32_t best=UINT32_MAX;D direction=cv(finish)-cv(start);
        for(auto i:candidates){const auto& t=faces[i];D a=cv(t[1])-cv(t[0]),b=cv(t[2])-cv(t[0]),p=cross(direction,b),c=cv(start)-cv(t[0]);double det=dot(a,p),u=dot(c,p),v=dot(direction,cross(c,a));
            bool inside=det<0?(u<0&&u>det&&v<0&&u+v>det):(det>0&&u>=0&&u<=det&&v>=0&&u+v<=det);
            if(!inside)continue;double at=dot(b,cross(c,a))/det;
            if(at>0&&at<1&&(at<nearest||(at==nearest&&i<best))){nearest=at;best=i;}
        }
        if(normal&&best!=UINT32_MAX){const auto& t=faces[best];*normal=normalized(storm_ship_hull::cross(t[1]-t[0],t[2]-t[0]));}
        return float(nearest);
    }
    bool land(Vec3 p) const {
        unsigned inward=0;constexpr float tau=6.2831853071795864769f;
        for(unsigned i=0;i<8;i++){float angle=float(i)/8.f*tau;Vec3 direction{std::cos(angle)*100.f,0,std::sin(angle)*100.f},normal;
            if(trace(p,p+direction,&normal)<=1&&dot(normal,normalized(direction))>0&&++inward>1)return true;
        }return false;
    }
};

// One owner for native LocalDepth admission. ftoi uses current-direction
// rounding (nearest/even in the source runtime); it does not truncate. Find
// transitions in the actual monotone float mapping, including half ties and
// reciprocal rounding, rather than assuming nominal [sample,sample+1) cells.
struct NativeAxis {
    float center,extent,step,inverse;
    std::vector<double> cuts;
    int cell(float local) const {return int(std::nearbyint(float(local*inverse+1024.f)));}
    static uint32_t ordered(float v){uint32_t bits;std::memcpy(&bits,&v,4);return bits&0x80000000u?~bits:bits^0x80000000u;}
    static float value(uint32_t key){uint32_t bits=key&0x80000000u?key^0x80000000u:~key;float v;std::memcpy(&v,&bits,4);return v;}
    NativeAxis(float c,float size,float realExtent):center(c),extent(realExtent),step(size/2048.f),inverse(1.f/step){
        if(std::fegetround()!=FE_TONEAREST)throw std::runtime_error("Native depth preparation requires nearest/even rounding");
        uint64_t first=ordered(std::nextafter(-extent,INFINITY)),end=uint64_t(ordered(std::nextafter(extent,-INFINITY)))+1;
        if(cell(value(uint32_t(first)))<0||cell(value(uint32_t(end-1)))>=2048)throw std::runtime_error("Native real bounds admit an unclamped out-of-range depth index");
        for(int index=0;index<=2048;index++){
            uint64_t lo=first,hi=end;while(lo<hi){auto mid=(lo+hi)/2;if(cell(value(uint32_t(mid)))<index)lo=mid+1;else hi=mid;}
            double local=lo==first?-double(extent):lo==end?double(extent):(double(value(uint32_t(lo-1)))+value(uint32_t(lo)))*.5;
            cuts.push_back(double(center)+local);
        }
    }
    int at(float p)const{float local=p-center;if(std::abs(local)>=extent)return -1;int result=cell(local);if(result<0||result>=2048)throw std::runtime_error("Native depth admission exceeds MapZipper's unclamped grid");return result;}
};
struct NativeGrid {NativeAxis x,z;explicit NativeGrid(const Body& b):x(b.cx,b.sx,b.ex),z(b.cz,b.sz,b.ez){};};

void depth(const std::string& input,const std::string& prefix) {
    Body b(input);NativeGrid grid(b);constexpr uint32_t size=2048;std::vector<uint8_t> pixels(size*size,255);
    float dx=b.sx/size,dz=b.sz/size;uint32_t land=0;
    for(uint32_t z=0;z<size;z++)for(uint32_t x=0;x<size;x++){
        Vec3 p{(float(x)-float(size)/2)*dx+b.cx,0,(float(z)-float(size)/2)*dz+b.cz};
        float hit=b.trace(p,{p.x,-500,p.z+.001f});uint8_t value=255;
        if(hit<=1){float height=std::min(20.f,length(Vec3{0,-500,.001f}*hit));value=b.land(p)?2:uint8_t(2.f+253.f*height/20.f);}
        else if(b.trace(p,{p.x,1500,p.z+.001f})<=1||b.land(p))value=2;
        pixels[x+z*size]=value;land+=value==2;
    }
    std::ofstream tga(prefix+".tga",std::ios::binary);std::array<uint8_t,18> h{};h[2]=3;h[12]=h[14]=0;h[13]=h[15]=8;h[16]=8;h[17]=8;tga.write(reinterpret_cast<char*>(h.data()),h.size());tga.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
    std::vector<uint16_t> table;std::vector<uint8_t> blocks;
    for(uint32_t z=0;z<size;z+=8)for(uint32_t x=0;x<size;x+=8){uint8_t value=pixels[x+z*size];bool same=true;for(uint32_t j=0;j<8;j++)for(uint32_t i=0;i<8;i++)same&=pixels[x+i+(z+j)*size]==value;
        if(same)table.push_back(uint16_t(0x8000|value));else{if(blocks.size()/64>=0x8000)throw std::runtime_error("Native MapZipper 15-bit block limit exceeded; no valid ZAP emitted");table.push_back(uint16_t(blocks.size()/64));for(uint32_t j=0;j<8;j++)for(uint32_t i=0;i<8;i++)blocks.push_back(pixels[x+i+(z+j)*size]);}
    }
    std::ofstream zap(prefix+".tga.zap",std::ios::binary);for(uint32_t v:{size,256u,8u,3u,8u,uint32_t(blocks.size()/64)})write(zap,v);zap.write(reinterpret_cast<char*>(table.data()),table.size()*2);zap.write(reinterpret_cast<char*>(blocks.data()),blocks.size());
    auto water=[&](Vec3 p){int x=grid.x.at(p.x),z=grid.z.at(p.z);if(x<0||z<0)return b.trace(p,{p.x,1500,p.z+.001f})>1&&!b.land(p);return pixels[x+z*size]>2&&!b.land(p);};
    std::vector<int> ids(b.points.size(),-1);unsigned n=0;for(size_t i=0;i<ids.size();i++)if(water(b.points[i]))ids[i]=int(n++);
    std::vector<std::vector<std::array<uint32_t,2>>> adjacency(n);unsigned kept=0;
    for(auto e:b.edges)if(ids[e[0]]>=0&&ids[e[1]]>=0&&b.trace(b.points[e[0]],b.points[e[1]])>1){std::array<uint32_t,2> edge{uint32_t(ids[e[0]]),uint32_t(ids[e[1]])};adjacency[edge[0]].push_back(edge);adjacency[edge[1]].push_back(edge);kept++;}
    std::vector<unsigned> parent(n);for(unsigned i=0;i<n;i++)parent[i]=i;
    auto find=[&](unsigned p){while(parent[p]!=p){parent[p]=parent[parent[p]];p=parent[p];}return p;};
    unsigned components=n;for(unsigned p=0;p<n;p++)for(auto e:adjacency[p]){auto a=find(e[0]),c=find(e[1]);if(a!=c){parent[a]=c;components--;}}
    unsigned authoredComponents=components;
    // Every intersected native sample cell must admit a join, including both
    // cells along a grid edge and all four at a corner. Outside this body's
    // native depth extent remains open water, but physical collision still
    // checks the complete unbounded segment.
    auto clearGrid=[&](Vec3 a,Vec3 c){
        double vx=double(c.x)-a.x,vz=double(c.z)-a.z;
        std::vector<double> events{0,1};
        if(vx)for(auto x:grid.x.cuts){double t=(x-a.x)/vx;if(t>0&&t<1)events.push_back(t);}
        if(vz)for(auto z:grid.z.cuts){double t=(z-a.z)/vz;if(t>0&&t<1)events.push_back(t);}
        std::sort(events.begin(),events.end());
        auto touched=[](const NativeAxis& axis,double p){std::vector<unsigned> result;double below=p,above=p;for(int i=0;i<4;i++){below=std::nextafter(below,-INFINITY);above=std::nextafter(above,INFINITY);} // cover event division/multiply/add rounding at shared boundaries
            auto first=std::lower_bound(axis.cuts.begin(),axis.cuts.end(),below),last=std::upper_bound(axis.cuts.begin(),axis.cuts.end(),above);auto lo=first-axis.cuts.begin(),hi=last-axis.cuts.begin();for(auto i=std::max<std::ptrdiff_t>(0,lo-1);i<std::min<std::ptrdiff_t>(2048,hi);i++)if(axis.cuts[i]<axis.cuts[i+1])result.push_back(unsigned(i));return result;};
        auto admit=[&](double t){auto xs=touched(grid.x,double(a.x)+vx*t),zs=touched(grid.z,double(a.z)+vz*t);for(auto x:xs)for(auto z:zs)if(pixels[x+z*size]<=2)return false;return true;};
        for(size_t i=0;i<events.size();i++){if(!admit(events[i]))return false;if(i&& !admit((events[i-1]+events[i])*.5))return false;}return true;
    };
    struct Join{double distance;uint32_t a,c;};std::vector<Join> candidates,joins;
    if(components>1){for(unsigned a=0;a<ids.size();a++)if(ids[a]>=0)for(unsigned c=a+1;c<ids.size();c++)if(ids[c]>=0&&find(ids[a])!=find(ids[c])){auto d=b.points[c]-b.points[a];candidates.push_back({double(d.x)*d.x+double(d.z)*d.z,a,c});}
        std::sort(candidates.begin(),candidates.end(),[](const Join&a,const Join&c){return std::tie(a.distance,a.a,a.c)<std::tie(c.distance,c.a,c.c);});
        for(auto j:candidates){auto a=find(ids[j.a]),c=find(ids[j.c]);if(a==c)continue;if(b.trace(b.points[j.a],b.points[j.c])<=1||!clearGrid(b.points[j.a],b.points[j.c]))continue;
            std::array<uint32_t,2> edge{uint32_t(ids[j.a]),uint32_t(ids[j.c])};adjacency[edge[0]].push_back(edge);adjacency[edge[1]].push_back(edge);parent[a]=c;joins.push_back(j);if(--components==1)break;
        }
    }
    std::ofstream graph(prefix+".graph");graph.precision(9);graph<<"[GraphPoints]\n";
    for(size_t i=0;i<ids.size();i++)if(ids[i]>=0){graph<<"pnt"<<ids[i]<<"="<<b.points[i].x<<","<<b.points[i].z<<","<<adjacency[ids[i]].size()<<",";for(auto e:adjacency[ids[i]])graph<<e[0]<<","<<e[1]<<",";graph<<"\n";}
    std::cout.precision(9);std::cout<<"{\"collision_triangles\":"<<b.faces.size()<<",\"land_samples\":"<<land<<",\"real_blocks\":"<<blocks.size()/64<<",\"graph_points\":"<<n<<",\"graph_edges\":"<<kept+joins.size()<<",\"authored_graph_components\":"<<authoredComponents<<",\"graph_components\":"<<components<<",\"rejected_graph_points\":"<<ids.size()-n<<",\"rejected_graph_edges\":"<<b.edges.size()-kept<<",\"bridge_edges\":[";
    for(size_t i=0;i<joins.size();i++){auto j=joins[i];if(i)std::cout<<",";std::cout<<"["<<ids[j.a]<<","<<ids[j.c]<<","<<b.points[j.a].x<<","<<b.points[j.a].z<<","<<b.points[j.c].x<<","<<b.points[j.c].z<<"]";}std::cout<<"]}\n";
}

Paths64 unite(const Paths64& paths){Clipper64 clip;clip.AddSubject(paths);Paths64 out;if(!clip.Execute(ClipType::Union,FillRule::NonZero,out))throw std::runtime_error("Coast union failed");return out;}
Paths64 booleanPaths(ClipType operation,const Paths64& subject,const Paths64& clipPaths){Clipper64 clip;clip.AddSubject(subject);clip.AddClip(clipPaths);Paths64 out;if(!clip.Execute(operation,FillRule::NonZero,out))throw std::runtime_error("Navigation containment operation failed");return out;}
Path64 filledSupport(const Path64& support,Point64 anchor){Path64 seed;for(auto p:support)seed.push_back(p+anchor);if(!IsPositive(seed))std::reverse(seed.begin(),seed.end());return seed;}
Paths64 expand(const Paths64& paths,const Path64& support,const Paths64& extra={}){
    Paths64 result=paths;for(const auto& path:paths){if(path.empty())continue;
        // Clipper's MinkowskiSum unions edge-edge sweep quads, not the filled
        // pattern. Seed a boundary support so tiny contours and holes also
        // receive the complete filled hull footprint.
        result.push_back(filledSupport(support,path.front()));auto shell=MinkowskiSum(support,path,true);result.insert(result.end(),shell.begin(),shell.end());
    }result.insert(result.end(),extra.begin(),extra.end());return unite(result);
}
void writePaths(std::ostream& out,const Paths64& paths){write(out,uint32_t(paths.size()));for(const auto& path:paths){write(out,uint32_t(path.size()));for(auto p:path){write(out,p.x);write(out,p.y);}}}
Paths64 readPaths(std::istream& in){uint32_t n;read(in,n);if(n>4000000)throw std::runtime_error("Prepared obstruction path budget exceeded");Paths64 result(n);size_t total=0;for(auto& path:result){uint32_t size;read(in,size);total+=size;if(total>4000000)throw std::runtime_error("Prepared obstruction vertex budget exceeded");path.resize(size);for(auto& p:path){read(in,p.x);read(in,p.y);}}return result;}
Path64 projection(const Face& t,double tx,double tz,double units){
    std::vector<Vec3> polygon(t.begin(),t.end()),clipped;
    // The saturated native depth map promises only20m. Treat every collision
    // surface above that plane as an obstacle, including submerged reef faces.
    for(size_t i=0;i<polygon.size();i++){auto a=polygon[i],b=polygon[(i+1)%polygon.size()];bool ia=a.y>=-20,ib=b.y>=-20;if(ia)clipped.push_back(a);if(ia!=ib)clipped.push_back(a+(b-a)*((-20-a.y)/(b.y-a.y)));}
    Path64 path;for(auto p:clipped)path.emplace_back(int64_t(std::llround((p.x+tx)*units)),int64_t(std::llround((p.z+tz)*units)));if(path.size()>=3&&!IsPositive(path))std::reverse(path.begin(),path.end());return path;
}

void navigation(const std::string& input,const std::string& output,bool inspect=false,bool envelope=false,bool prepared=false,bool obstructionOnly=false){
    std::ifstream config(input,std::ios::binary);double minX,minZ,maxX,maxZ,radius,maxExtra;unsigned l,w,count;
    Paths64 nativeUnsafe,blocked;unsigned sourceWalls=0,uncoveredWalls=0;
    if(prepared){uint64_t magic;read(config,magic);if(magic!=0x31304f56414e5353ull)throw std::runtime_error("Unknown prepared obstruction bundle");for(auto p:{&minX,&minZ,&maxX,&maxZ,&radius,&maxExtra})read(config,*p);for(auto p:{&l,&w,&count,&sourceWalls,&uncoveredWalls})read(config,*p);blocked=readPaths(config);nativeUnsafe=readPaths(config);}
    else config>>minX>>minZ>>maxX>>maxZ>>l>>w>>radius>>count>>maxExtra;
    if(!config||minX>=maxX||minZ>=maxZ||!l||!w||!std::isfinite(radius)||radius<0)throw std::runtime_error("Invalid navigation configuration");
    // Use the finest common binary grid representable by every native float
    // vertex in the preserved PTC extent. Integer millimetres would collapse
    // distinct far-map vertices when PtcVertex stores them as float.
    double mapScale=std::ldexp(1.,23-std::ilogb(std::max({std::abs(minX),std::abs(minZ),std::abs(maxX),std::abs(maxZ)})/50));
    double units=mapScale/50;
    // Existing Clipper2 vendor has no offset.cpp. Its maintained Minkowski
    // facility supplies the actual configured WDM hull's conservative support.
    //32-gon support includes three integer projection/boolean rounding events;
    // this is representational error, not coastline simplification/padding.
    Path64 support;constexpr unsigned sides=32;double r=(radius+3*std::sqrt(2.)/units)/std::cos(3.141592653589793/sides);
    for(unsigned i=0;i<sides;i++){double a=(i+.5)*6.283185307179586/sides;support.emplace_back(int64_t(std::llround(std::cos(a)*r*units)),int64_t(std::llround(std::sin(a)*r*units)));}
    if(!prepared){
    Paths64 islands;
    Paths64 lineObstacles;
    for(unsigned i=0;i<count;i++){
        std::string file;double tx,tz;config>>file>>tx>>tz;if(!config)throw std::runtime_error("Truncated body list");Body b(file);Paths64 projected,lines;Path64 points;
        for(const auto& t:b.faces){auto p=projection(t,tx,tz,units);if(p.empty())continue;
            if(p.size()>=3&&Area(p)!=0)projected.push_back(std::move(p));
            else {p.erase(std::unique(p.begin(),p.end()),p.end());if(p.size()>1)lines.push_back(std::move(p));else points.push_back(p[0]);}
        }
        auto body=unite(projected);sourceWalls+=unsigned(lines.size());
        {
            std::string depthPath;config>>depthPath;std::ifstream depthFile(depthPath,std::ios::binary);std::array<uint8_t,18> header{};read(depthFile,header);
            unsigned size=header[12]+(unsigned(header[13])<<8),height=header[14]+(unsigned(header[15])<<8);
            if(header[2]!=3||header[16]!=8||size!=2048||height!=2048)throw std::runtime_error("Inspection requires prepared native2048 depth TGA");
            std::vector<uint8_t> pixels(size*size);depthFile.read(reinterpret_cast<char*>(pixels.data()),pixels.size());if(!depthFile)throw std::runtime_error("Truncated prepared depth TGA");
            Paths64 runs;NativeGrid grid(b);
            for(unsigned z=0;z<size;z++){unsigned x=0;while(x<size){while(x<size&&pixels[x+z*size]==255)x++;unsigned first=x;while(x<size&&pixels[x+z*size]!=255)x++;if(first==x)continue;
                if(grid.x.cuts[first]>=grid.x.cuts[x]||grid.z.cuts[z]>=grid.z.cuts[z+1])continue;
                int64_t x0=int64_t(std::floor((grid.x.cuts[first]+tx)*units)),x1=int64_t(std::ceil((grid.x.cuts[x]+tx)*units));
                int64_t z0=int64_t(std::floor((grid.z.cuts[z]+tz)*units)),z1=int64_t(std::ceil((grid.z.cuts[z+1]+tz)*units));runs.push_back({{x0,z0},{x1,z0},{x1,z1},{x0,z1}});
            }}
            auto unsafe=unite(runs);nativeUnsafe.insert(nativeUnsafe.end(),unsafe.begin(),unsafe.end());
            // Native GetDepthFast admits the complete sample cell, including
            // camomile land not covered by an upward/downward triangle. That
            // source exclusion is part of the obstruction before hull support.
            body.insert(body.end(),unsafe.begin(),unsafe.end());body=unite(body);
        }
        // Exact redundancy removal. If L is covered by the land projection P,
        // then L+support is already contained in P+support. Clip open wall
        // paths against P before expanding, without dropping uncovered walls.
        Clipper64 outside;outside.AddOpenSubject(lines);outside.AddClip(body);Paths64 ignored,uncovered;
        if(!outside.Execute(ClipType::Difference,FillRule::NonZero,ignored,uncovered))throw std::runtime_error("Projected wall coverage difference failed");
        uncoveredWalls+=unsigned(uncovered.size());Paths64 expanded;
        for(const auto& p:uncovered){if(p.empty())continue;expanded.push_back(filledSupport(support,p.front()));expanded.push_back(filledSupport(support,p.back()));auto shell=MinkowskiSum(support,p,false);expanded.insert(expanded.end(),shell.begin(),shell.end());}
        for(auto point:points){int winding=0;bool boundary=false;for(const auto& polygon:body){auto at=PointInPolygon(point,polygon);if(at==PointInPolygonResult::IsOn){boundary=true;break;}if(at==PointInPolygonResult::IsInside)winding+=IsPositive(polygon)?1:-1;}if(boundary||winding)continue;Path64 translated;for(auto q:support)translated.push_back(q+point);expanded.push_back(std::move(translated));}
        auto walls=unite(expanded);lineObstacles.insert(lineObstacles.end(),walls.begin(),walls.end());islands.insert(islands.end(),body.begin(),body.end());
        std::cerr<<"coast "<<i+1<<"/"<<count<<" active="<<b.faces.size()<<" wall_paths="<<lines.size()<<" uncovered="<<uncovered.size()<<"\n";
    }
    blocked=unite(islands);
    blocked=expand(blocked,support,lineObstacles);
    }
    if(obstructionOnly){
        nativeUnsafe=unite(nativeUnsafe);std::ofstream out(output,std::ios::binary);write(out,uint64_t(0x31304f56414e5353ull));for(auto p:{&minX,&minZ,&maxX,&maxZ,&radius,&maxExtra})write(out,*p);for(auto p:{&l,&w,&count,&sourceWalls,&uncoveredWalls})write(out,*p);writePaths(out,blocked);writePaths(out,nativeUnsafe);if(!out)throw std::runtime_error("Prepared obstruction bundle write failed");
        size_t vertices=0;for(const auto& p:blocked)vertices+=p.size();std::cout<<"{\"required_obstruction_paths\":"<<blocked.size()<<",\"required_obstruction_vertices\":"<<vertices<<",\"source_wall_paths\":"<<sourceWalls<<",\"uncovered_wall_paths\":"<<uncoveredWalls<<"}\n";return;
    }
    const Paths64 required=blocked;Paths64 upper=required;size_t requiredVertices=0;for(const auto& p:required)requiredVertices+=p.size();
    if(envelope){
        // Candidate quality bound, not a user-required tolerance. Clipper's
        // integer intersection truncation and scanbeam correction each admit
        // a conservative 2*sqrt(2) lattice-cell event budget. Eight stages need
        // ceil(8*2*sqrt(2))+1 =24 cells. Use separate inner/outer allowances so
        // strict containment never compares identical quantized boundaries.
        constexpr int64_t pad=24;
        if(!std::isfinite(maxExtra)||maxExtra*units<=4*pad+3*std::sqrt(2.))throw std::runtime_error("Invalid configured far-envelope allowance");
        auto allowance=[&](double radius){Path64 path;double r=radius*units-3*std::sqrt(2.);for(unsigned i=0;i<sides;i++){double a=i*6.283185307179586/sides;path.emplace_back(int64_t(std::llround(std::cos(a)*r)),int64_t(std::llround(std::sin(a)*r)));}return path;};
        auto inner=expand(required,allowance(maxExtra-4*pad/units));upper=expand(required,allowance(maxExtra-pad/units));
        auto simplified=unite(SimplifyPaths(inner,maxExtra*units*.5,true));simplified.insert(simplified.end(),required.begin(),required.end());
        blocked=booleanPaths(ClipType::Intersection,unite(simplified),inner);
        blocked=expand(blocked,Path64{{-pad,-pad},{pad,-pad},{pad,pad},{-pad,pad}});
        auto requiredMissing=booleanPaths(ClipType::Difference,required,blocked),upperMissing=booleanPaths(ClipType::Difference,required,upper),outside=booleanPaths(ClipType::Difference,blocked,upper);
        auto area=[&](const Paths64& paths){double sum=0;for(const auto& p:paths)sum+=Area(p);return sum/units/units;};
        if(inspect){
            auto save=[&](const std::string& suffix,const Paths64& paths){std::ofstream out(output+suffix,std::ios::binary);write(out,uint64_t(0x31304756414e5353ull));write(out,units);writePaths(out,paths);if(!out)throw std::runtime_error("Far containment evidence write failed");};
            save(".candidate",blocked);save(".upper",upper);save(".required-missing",requiredMissing);save(".upper-missing-required",upperMissing);save(".outside-upper",outside);
            size_t candidateVertices=0,upperVertices=0;for(const auto& p:blocked)candidateVertices+=p.size();for(const auto& p:upper)upperVertices+=p.size();
            std::cout.precision(17);std::cout<<"{\"required_missing_paths\":"<<requiredMissing.size()<<",\"required_missing_square_metres\":"<<area(requiredMissing)<<",\"upper_missing_required_paths\":"<<upperMissing.size()<<",\"upper_missing_required_square_metres\":"<<area(upperMissing)<<",\"candidate_outside_upper_paths\":"<<outside.size()<<",\"candidate_outside_upper_square_metres\":"<<area(outside)<<",\"required_vertices\":"<<requiredVertices<<",\"candidate_vertices\":"<<candidateVertices<<",\"upper_vertices\":"<<upperVertices<<",\"allowance_metres\":"<<maxExtra<<",\"grid_metres\":"<<1/units<<"}\n";return;
        }
        if(!requiredMissing.empty()||!upperMissing.empty()||!outside.empty())throw std::runtime_error("Far containment failed: O\\Q="+std::to_string(requiredMissing.size())+", O\\U="+std::to_string(upperMissing.size())+", Q\\U="+std::to_string(outside.size()));
        std::cerr<<"far envelope exact containment passed; allowance_metres="<<maxExtra<<" required_vertices="<<requiredVertices<<"\n";
    }
    Path64 rectangle{{int64_t(std::llround(minX*units)),int64_t(std::llround(minZ*units))},{int64_t(std::llround(maxX*units)),int64_t(std::llround(minZ*units))},{int64_t(std::llround(maxX*units)),int64_t(std::llround(maxZ*units))},{int64_t(std::llround(minX*units)),int64_t(std::llround(maxZ*units))}};
    Clipper64 clip;clip.AddSubject({rectangle});clip.AddClip(blocked);Paths64 water;if(!clip.Execute(ClipType::Difference,FillRule::NonZero,water))throw std::runtime_error("Water difference failed");water=unite(water);
    nativeUnsafe=unite(nativeUnsafe);Clipper64 missed;missed.AddSubject(nativeUnsafe);missed.AddClip(water);Paths64 missing;if(!missed.Execute(ClipType::Intersection,FillRule::NonZero,missing))throw std::runtime_error("Native depth admission intersection failed");
    double missingArea=0;for(const auto& p:missing)missingArea+=Area(p);if(!missing.empty())throw std::runtime_error("Prepared global water intersects native unsafe depth cells");
    std::cerr<<"coast union complete; triangulating "<<water.size()<<" water boundaries\n";
    Paths64 mesh;if(Triangulate(water,mesh,true)!=TriangulateResult::success)throw std::runtime_error("Clipper2 constrained triangulation failed");
    size_t waterVertices=0,obstacleVertices=0;std::set<std::pair<int64_t,int64_t>> distinctVertices;for(const auto& p:water)waterVertices+=p.size();for(const auto& p:blocked)obstacleVertices+=p.size();for(const auto& p:mesh)for(auto v:p)distinctVertices.emplace(v.x,v.y);
    std::cerr<<"triangulation vertices="<<distinctVertices.size()<<" triangles="<<mesh.size()<<" water_vertices="<<waterVertices<<" obstruction_vertices="<<obstacleVertices<<" candidate_path_table_bytes="<<mesh.size()*((mesh.size()+3)/4)<<"\n";
    if(inspect){
        auto save=[&](const std::string& suffix,const Paths64& paths){std::ofstream file(output+suffix,std::ios::binary);uint64_t magic=0x31304756414e5353ull;write(file,magic);write(file,units);write(file,uint32_t(paths.size()));for(const auto& p:paths){write(file,uint32_t(p.size()));for(auto v:p){write(file,v.x);write(file,v.y);}}if(!file)throw std::runtime_error("Inspection geometry output failed");};
        save(".obstacles",blocked);save(".water",water);save(".native-unsafe",nativeUnsafe);save(".missing-native",missing);
        std::cout.precision(17);std::cout<<"{\"triangles\":"<<mesh.size()<<",\"vertices\":"<<distinctVertices.size()<<",\"native_triangle_limit\":32768,\"water_boundaries\":"<<water.size()<<",\"water_vertices\":"<<waterVertices<<",\"obstruction_vertices\":"<<obstacleVertices<<",\"native_unsafe_water_paths\":"<<missing.size()<<",\"native_unsafe_water_square_metres\":"<<missingArea/units/units<<",\"source_wall_paths\":"<<sourceWalls<<",\"uncovered_wall_paths\":"<<uncoveredWalls<<",\"clearance_metres\":"<<radius<<",\"grid_metres\":"<<1/units<<"}\n";return;
    }
    if(mesh.empty()||mesh.size()>32768)throw std::runtime_error("Native PTC signed16 triangle-neighbor limit exceeded: "+std::to_string(mesh.size())+" triangles, "+std::to_string(distinctVertices.size())+" vertices; no patch emitted");
    Paths64 positiveMesh=mesh;for(auto& triangle:positiveMesh)if(!IsPositive(triangle))std::reverse(triangle.begin(),triangle.end());
    if(!booleanPaths(ClipType::Intersection,positiveMesh,required).empty())throw std::runtime_error("Final water triangles intersect exact required obstruction");
    if(!booleanPaths(ClipType::Intersection,positiveMesh,nativeUnsafe).empty())throw std::runtime_error("Final water triangles intersect native unsafe depth cells");
    std::map<std::pair<int64_t,int64_t>,uint16_t> ids;std::vector<PtcVertex> vertices;std::vector<PtcTriangle> triangles;
    for(auto t:mesh){if(t.size()!=3)throw std::runtime_error("Nontriangle in triangulator output");if(IsPositive(t))std::reverse(t.begin(),t.end());PtcTriangle row{};row.nb[0]=row.nb[1]=row.nb[2]=-1;
        for(unsigned c=0;c<3;c++){auto key=std::make_pair(t[c].x,t[c].y);auto found=ids.find(key);if(found==ids.end()){if(vertices.size()>=65536)throw std::runtime_error("Native PTC vertex limit exceeded");auto id=uint16_t(vertices.size());ids[key]=id;PtcVertex v{float(t[c].x/mapScale),0,float(t[c].y/mapScale)};if(double(v.x)*mapScale!=t[c].x||double(v.z)*mapScale!=t[c].y)throw std::runtime_error("Native float PTC vertex changed admitted geometry");vertices.push_back(v);row.i[c]=id;}else row.i[c]=found->second;}triangles.push_back(row);
    }
    std::map<std::pair<uint16_t,uint16_t>,std::pair<int,unsigned>> edges;for(unsigned i=0;i<triangles.size();i++)for(unsigned e=0;e<3;e++){auto a=triangles[i].i[e],b=triangles[i].i[(e+1)%3];auto key=std::minmax(a,b);auto inserted=edges.emplace(key,std::make_pair(int(i),e));if(!inserted.second){auto other=inserted.first->second;if(triangles[other.first].nb[other.second]>=0)throw std::runtime_error("Nonmanifold PTC edge");triangles[i].nb[e]=int16_t(other.first);triangles[other.first].nb[other.second]=int16_t(i);}}
    unsigned n=unsigned(triangles.size()),line=(n+3)/4;std::vector<uint8_t> table(size_t(n)*line,255);std::vector<PtcVertex> centers(n);for(unsigned i=0;i<n;i++){for(auto id:triangles[i].i){centers[i].x+=vertices[id].x/3;centers[i].z+=vertices[id].z/3;}}
    std::cerr<<"PTC "<<n<<" triangles; preparing native next-edge rows\n";
    unsigned components=0;std::vector<int> component(n,-1);
    for(unsigned source=0;source<n;source++){
        std::vector<double> dist(n,INFINITY);std::vector<uint8_t> first(n,3);using Item=std::pair<double,unsigned>;std::priority_queue<Item,std::vector<Item>,std::greater<Item>> queue;dist[source]=0;queue.push({0,source});
        while(!queue.empty()){auto [d,u]=queue.top();queue.pop();if(d!=dist[u])continue;for(unsigned e=0;e<3;e++){int v=triangles[u].nb[e];if(v<0)continue;double next=d+std::hypot(centers[u].x-centers[v].x,centers[u].z-centers[v].z);if(next<dist[v]){dist[v]=next;first[v]=u==source?uint8_t(e):first[u];queue.push({next,unsigned(v)});}}}
        if(component[source]<0){for(unsigned j=0;j<n;j++)if(std::isfinite(dist[j]))component[j]=int(components);components++;}
        for(unsigned j=0;j<n;j++)table[size_t(source)*line+j/4]&=uint8_t(~(uint8_t(3-first[j])<<((j&3)*2)));
    }
    std::vector<PtcMap> map;std::vector<uint16_t> indices;map.reserve(l*w);
    for(unsigned z=0;z<l;z++)for(unsigned x=0;x<w;x++){PtcMap cell{int32_t(indices.size()),0};double x0=(minX+(maxX-minX)*x/w)/50,x1=(minX+(maxX-minX)*(x+1)/w)/50,z0=(minZ+(maxZ-minZ)*z/l)/50,z1=(minZ+(maxZ-minZ)*(z+1)/l)/50;
        for(unsigned i=0;i<n;i++){const auto& t=triangles[i];float lx=INFINITY,hx=-INFINITY,lz=INFINITY,hz=-INFINITY;for(auto id:t.i){lx=std::min(lx,vertices[id].x);hx=std::max(hx,vertices[id].x);lz=std::min(lz,vertices[id].z);hz=std::max(hz,vertices[id].z);}if(lx<=x1&&hx>=x0&&lz<=z1&&hz>=z0){indices.push_back(uint16_t(i));cell.size++;}}map.push_back(cell);
    }
    PtcHeader h{};h.id=PTC_ID;h.space0=h.space1=h.space2=PTC_SPACEFLD;h.ver=PTC_VERSION;h.isSea=PTC_WITHSEA;h.numTriangles=int(n);h.numVerteces=int(vertices.size());h.numNormals=1;h.mapL=int(l);h.mapW=int(w);h.numIndeces=int(indices.size());h.lineSize=int(line);h.minX=float(minX/50);h.minZ=float(minZ/50);h.maxX=float(maxX/50);h.maxZ=float(maxZ/50);
    std::ofstream out(output,std::ios::binary);write(out,h);for(auto t:triangles)write(out,t);for(auto v:vertices)write(out,v);write(out,PtcNormal{0,1,0});for(auto c:map)write(out,c);for(auto id:indices)write(out,id);out.write(reinterpret_cast<char*>(table.data()),table.size());PtcMaterials materials{};materials.numMaterials=1;std::memcpy(materials.material[0],"sea",3);write(out,materials);if(!out)throw std::runtime_error("PTC output write failed");
    std::cout<<"{\"triangles\":"<<n<<",\"vertices\":"<<vertices.size()<<",\"components\":"<<components<<",\"path_table_bytes\":"<<table.size()<<",\"collision_indices\":"<<indices.size()<<",\"clearance_metres\":"<<radius<<",\"far_envelope\":"<<(envelope?"true":"false")<<",\"far_extra_exclusion_limit_metres\":"<<(envelope?maxExtra:0)<<",\"required_obstruction_vertices\":"<<requiredVertices<<",\"required_containment_failed_paths\":0,\"upper_containment_failed_paths\":0,\"triangles_intersecting_required_paths\":0,\"native_float_grid_metres\":"<<1/units<<",\"source_wall_paths\":"<<sourceWalls<<",\"uncovered_wall_paths\":"<<uncoveredWalls<<",\"native_unsafe_water_paths\":"<<missing.size()<<",\"native_unsafe_water_square_metres\":"<<missingArea/units/units<<"}\n";
}
int main(int argc,char** argv){try{if(argc!=4)throw std::runtime_error("Expected depth|obstruction|navigation|navigation-envelope|navigation-prepared|navigation-envelope-prepared|inspect-prepared|inspect-envelope-prepared|inspect input output-prefix");std::string mode=argv[1];if(mode=="depth")depth(argv[2],argv[3]);else if(mode=="navigation"||mode=="navigation-envelope"||mode=="inspect"||mode=="obstruction"||mode=="navigation-prepared"||mode=="navigation-envelope-prepared"||mode=="inspect-prepared"||mode=="inspect-envelope-prepared")navigation(argv[2],argv[3],mode=="inspect"||mode=="inspect-prepared"||mode=="inspect-envelope-prepared",mode=="navigation-envelope"||mode=="navigation-envelope-prepared"||mode=="inspect-envelope-prepared",mode=="navigation-prepared"||mode=="navigation-envelope-prepared"||mode=="inspect-prepared"||mode=="inspect-envelope-prepared",mode=="obstruction");else throw std::runtime_error("Unknown offline preparation stage");return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}

// Offline chart polygon preparation. Reuse the navigation owner's Clipper2.
#include "clipper2/clipper.h"
#include "clipper2/clipper.triangulation.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace Clipper2Lib;
template<class T> void read(std::istream& s,T& v) {
    s.read(reinterpret_cast<char*>(&v),sizeof(v));
    if(!s) throw std::runtime_error("Truncated chart polygons");
}
int main(int argc,char** argv) {
    try {
        if(argc!=4) throw std::runtime_error("Expected union|triangulate input output");
        std::ifstream in(argv[2],std::ios::binary);
        uint32_t count;read(in,count);
        if(count>1000000) throw std::runtime_error("Chart polygon budget exceeded");
        Paths64 paths(count);size_t total=0;
        for(auto& path:paths) {
            uint32_t size;read(in,size);total+=size;
            if(total>4000000) throw std::runtime_error("Chart vertex budget exceeded");
            path.resize(size);for(auto& p:path){read(in,p.x);read(in,p.y);}
        }
        Paths64 result;const std::string mode=argv[1];
        if(mode=="union") {
            Clipper64 clip;clip.AddSubject(paths);
            if(!clip.Execute(ClipType::Union,FillRule::NonZero,result))
                throw std::runtime_error("Chart land union failed");
        } else if(mode=="triangulate") {
            if(Triangulate(paths,result)!=TriangulateResult::success)
                throw std::runtime_error("Chart land triangulation failed");
        } else if(mode=="difference"||mode=="intersection"||mode=="clip-mesh") {
            read(in,count);Paths64 clips(count);
            for(auto& path:clips){uint32_t size;read(in,size);total+=size;
                if(total>4000000)throw std::runtime_error("Chart clip budget exceeded");
                path.resize(size);for(auto& p:path){read(in,p.x);read(in,p.y);}}
            if(mode=="clip-mesh") {
                for(size_t i=0;i<paths.size();i++) {
                    Clipper64 clip;clip.AddSubject({paths[i]});clip.AddClip(clips);Paths64 pieces,triangles;PolyTree64 tree;
                    if(!clip.Execute(ClipType::Intersection,FillRule::NonZero,tree))
                        throw std::runtime_error("Chart triangle clip failed");
                    const auto collect=[&](const auto& self,const PolyPath64& parent)->void{
                        for(const auto& child:parent){
                            auto piece=TrimCollinear(child->Polygon());
                            // Hierarchy owns holes: tiny integer-grid slivers
                            // can emerge with reversed winding after clipping.
                            if(IsPositive(piece)==child->IsHole())std::reverse(piece.begin(),piece.end());
                            pieces.push_back(std::move(piece));self(self,*child);
                        }
                    };collect(collect,tree);
                    pieces.erase(std::remove_if(pieces.begin(),pieces.end(),[](const Path64& p){return p.size()<3||Area(p)==0;}),pieces.end());
                    if(pieces.empty())continue;
                    bool convex=true;
                    for(const auto& p:pieces){int sign=0;
                        if(!IsPositive(p)){convex=false;break;}
                        for(size_t k=0;k<p.size();k++){
                            const auto a=p[k],b=p[(k+1)%p.size()],c=p[(k+2)%p.size()];
                            const __int128 turn=__int128(b.x-a.x)*(c.y-b.y)-__int128(b.y-a.y)*(c.x-b.x);
                            const int next=turn>0?1:turn<0?-1:0;
                            if(next&&sign&&next!=sign){convex=false;break;}if(next)sign=next;
                        }
                        if(!convex)break;
                    }
                    // Existing GM preparers fan convex clipped faces. Avoid
                    // a sweep triangulation for an already convex triangle.
                    TriangulateResult status=TriangulateResult::success;
                    if(convex){for(const auto& p:pieces)for(size_t k=1;k+1<p.size();k++)triangles.push_back({p[0],p[k],p[k+1]});}
                    else status=Triangulate(pieces,triangles);
                    if(status!=TriangulateResult::success)
                        throw std::runtime_error("Clipped chart triangle triangulation failed at face "+std::to_string(i)+" status "+std::to_string(int(status)));
                    for(auto& triangle:triangles){triangle.insert(triangle.begin(),Point64(int64_t(i),int64_t(-1)));result.push_back(std::move(triangle));}
                }
            } else {
                Clipper64 clip;clip.AddSubject(paths);clip.AddClip(clips);
                if(!clip.Execute(mode=="difference"?ClipType::Difference:ClipType::Intersection,FillRule::NonZero,result))
                    throw std::runtime_error("Chart intersection failed");
            }
        } else throw std::runtime_error("Unknown chart operation");
        std::ofstream out(argv[3],std::ios::binary);
        auto write=[&](auto v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));};
        write(uint32_t(result.size()));
        for(const auto& path:result){write(uint32_t(path.size()));for(auto p:path){write(p.x);write(p.y);}}
        if(!out)throw std::runtime_error("Chart polygons write failed");
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

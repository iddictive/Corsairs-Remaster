#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_hull_structure.hpp"
#include "geometry_work_parallel.hpp"
#include <tuple>

// Derived physical cells. SAVE continues to own the exact current boundaries;
// this cache contains no operation history and is rebuilt only by a worker.
namespace storm::ship::material::convex {
using namespace storm_ship_laser;
using storm_ship_laser::Plane;
using Halfspace=storm_ship_hull::Plane;
struct Face {std::vector<Vertex>vertices;Halfspace plane{};int object=0;bool fresh=false,partition=false;};
struct Piece {std::vector<Face>faces;Bounds bounds{};double volume=0;uint32_t supportCell=storm_ship_structure::NoCell,authoredOwner=UINT32_MAX;};
struct Key {uint32_t cell=0,piece=0;bool operator<(const Key&o)const{return std::tie(cell,piece)<std::tie(o.cell,o.piece);}bool operator==(const Key&o)const{return cell==o.cell&&piece==o.piece;}};
struct Component {uint32_t authoredOwner=0;std::vector<Key>pieces;double volume=0;};
struct Cell {std::shared_ptr<const Mesh>boundary;std::shared_ptr<const std::vector<Piece>>pieces;std::vector<std::vector<Key>>edges;};
struct Source {std::shared_ptr<const MaterialCellSource>material;std::shared_ptr<const storm_ship_structure::Layout>layout;std::vector<std::shared_ptr<const std::vector<Piece>>>pieces;std::vector<std::vector<uint32_t>>supportSources;};
struct Measure {Bounds bounds{};Vec3 moment{};double volume=0,area=0;};
struct State {std::shared_ptr<const Source>source;std::vector<std::shared_ptr<const Cell>>cells;std::vector<Component>components;std::vector<Measure>supportMeasures;std::set<uint32_t>pendingConnectivity;const MaterialCells*owner=nullptr;};
struct Limits {size_t candidateCells=SIZE_MAX,convexPieces=SIZE_MAX,planeSplits=SIZE_MAX,contactPairs=SIZE_MAX,graphVisits=SIZE_MAX;};
struct Budget {Limits left;bool complete=true;explicit Budget(Limits limits={}):left(limits){}bool take(size_t&value,size_t n=1){if(n>value){complete=false;return false;}value-=n;return true;}};
inline bool overlapBounds(Bounds a,Bounds b,float pad=0){return a.lo.x<=b.hi.x+pad&&a.hi.x+pad>=b.lo.x&&a.lo.y<=b.hi.y+pad&&a.hi.y+pad>=b.lo.y&&a.lo.z<=b.hi.z+pad&&a.hi.z+pad>=b.lo.z;}
inline float polygonArea(const std::vector<Vertex>&v){float area=0;for(size_t n=1;n+1<v.size();++n)area+=storm_ship_hull::area({v[0],v[n],v[n+1]});return area;}
inline void measure(Piece&p){bool first=true;p.volume=0;Vec3 origin{};if(!p.faces.empty()&&!p.faces.front().vertices.empty())origin=p.faces.front().vertices.front().position;
    for(const auto&f:p.faces){for(const auto&v:f.vertices){p.bounds=first?Bounds{v.position,v.position}:storm_ship_hull::merge(p.bounds,{v.position,v.position});first=false;}for(size_t n=1;n+1<f.vertices.size();++n)p.volume+=double(storm_ship_hull::dot(f.vertices[0].position-origin,storm_ship_hull::cross(f.vertices[n].position-origin,f.vertices[n+1].position-origin)))/6.;}p.volume=std::abs(p.volume);}
inline Vec3 center(const Piece&p){if(p.faces.empty()||p.faces.front().vertices.empty())return {};const auto origin=p.faces.front().vertices.front().position;double volume=0;Vec3 moment{};for(const auto&f:p.faces)for(size_t n=1;n+1<f.vertices.size();++n){const auto a=f.vertices[0].position,b=f.vertices[n].position,c=f.vertices[n+1].position;const double v=double(storm_ship_hull::dot(a-origin,storm_ship_hull::cross(b-origin,c-origin)))/6.;volume+=v;moment=moment+(origin+a+b+c)*float(v*.25);}return std::abs(volume)>1.e-15?moment*float(1./volume):origin;}
inline std::vector<std::vector<Vertex>>outsideFace(std::vector<Vertex>polygon,const Piece&solid){std::vector<std::vector<Vertex>>result;for(const auto&f:solid.faces){std::vector<Vertex>in,out;linearSplit(polygon,f.plane,in,out);if(out.size()>=3&&polygonArea(out)>1.e-12f)result.push_back(std::move(out));polygon=std::move(in);if(polygon.size()<3)break;}return result;}
inline bool aligned(const Face&a,const Piece&b){for(const auto&f:b.faces)if(storm_ship_hull::dot(a.plane.normal,f.plane.normal)>.99999f&&std::abs(a.plane.distance-f.plane.distance)<1.e-6f)return true;return false;}
inline storm_ship_hull::Broadphase pieceIndex(const std::vector<Piece>&pieces){std::vector<Bounds>boxes;boxes.reserve(pieces.size());for(const auto&p:pieces)boxes.push_back(p.bounds);storm_ship_hull::Broadphase index;index.build(boxes);return index;}
inline std::vector<std::vector<Vertex>>externalFace(const std::vector<Piece>&pieces,size_t n,const Face&f,const storm_ship_hull::Broadphase*index=nullptr){std::vector<std::vector<Vertex>>fragments{f.vertices};std::vector<uint32_t>nearby;if(index){Bounds box{f.vertices.front().position,f.vertices.front().position};for(const auto&v:f.vertices)box=storm_ship_hull::merge(box,{v.position,v.position});index->queryBounds(box,nearby);}else{nearby.resize(pieces.size());std::iota(nearby.begin(),nearby.end(),0);}for(auto other:nearby){if(fragments.empty())break;if(other==n||!overlapBounds(pieces[n].bounds,pieces[other].bounds))continue;if(other>n&&aligned(f,pieces[other]))continue;std::vector<std::vector<Vertex>>next;for(auto&polygon:fragments){auto outside=outsideFace(std::move(polygon),pieces[other]);next.insert(next.end(),std::make_move_iterator(outside.begin()),std::make_move_iterator(outside.end()));}fragments=std::move(next);}return fragments;}
inline Mesh mesh(const std::vector<Piece>&pieces){Mesh result;const auto index=pieceIndex(pieces);for(size_t n=0;n<pieces.size();++n)for(const auto&f:pieces[n].faces)for(const auto&polygon:externalFace(pieces,n,f,&index))storm_ship_hull::fan(polygon,result[f.object]);return result;}
inline std::vector<Halfspace>planes(const Piece&p){std::vector<Halfspace>out;for(const auto&f:p.faces)out.push_back(f.plane);return out;}
inline bool clip(const Piece&source,Halfspace plane,Piece&out,Budget&budget,bool charred=false){
    if(!budget.take(budget.left.planeSplits))return false;bool low=false,high=false;for(const auto&f:source.faces)for(const auto&v:f.vertices){const auto side=planarSide(plane,v.position);low|=side<0;high|=side>0;}
    if(!high){out=source;return true;}if(!low){out={};return true;}
    Piece next;next.supportCell=source.supportCell;next.authoredOwner=source.authoredOwner;std::vector<Vertex>ring;
    for(const auto&face:source.faces){std::vector<Vertex>inside,outside;linearSplit(face.vertices,plane,inside,outside);if(inside.size()<3||polygonArea(inside)<=1.e-12f)continue;
        for(const auto&v:inside)if(planarSide(plane,v.position)==0){bool found=false;for(const auto&p:ring){const auto d=v.position-p.position;if(storm_ship_hull::dot(d,d)<1.e-12f){found=true;break;}}if(!found)ring.push_back(v);}
        next.faces.push_back({std::move(inside),face.plane,face.object,face.fresh,face.partition});}
    if(ring.size()>=3){Vec3 midpoint{};for(const auto&v:ring)midpoint=midpoint+v.position*(1.f/ring.size());const auto basis=storm_ship_hull::frame(plane.normal);
        std::sort(ring.begin(),ring.end(),[&](const auto&a,const auto&b){const auto x=a.position-midpoint,y=b.position-midpoint;return std::atan2(storm_ship_hull::dot(x,basis.bitangent),storm_ship_hull::dot(x,basis.tangent))<std::atan2(storm_ship_hull::dot(y,basis.bitangent),storm_ship_hull::dot(y,basis.tangent));});
        if(storm_ship_hull::dot(storm_ship_hull::cross(ring[1].position-ring[0].position,ring[2].position-ring[0].position),plane.normal)<0)std::reverse(ring.begin(),ring.end());
        for(auto&v:ring){v.normal=plane.normal;if(charred)v.color=(v.color&0xff000000u)|0x00645140u;capUV(v,plane.normal);}
        next.faces.push_back({std::move(ring),plane,source.faces.front().object,true});}
    measure(next);if(next.volume<=1.e-12)next={};out=std::move(next);return true;
}
template<class Planes>inline bool subtract(const Piece&source,const Planes&field,std::vector<Piece>&out,Budget&budget,std::vector<Face>*caps=nullptr,bool charred=false){
    Piece inside=source;std::vector<Piece>outside;
    for(const auto&p:field){Piece low,high;if(!clip(inside,p,low,budget,charred)||!clip(inside,{p.normal*-1.f,-p.distance},high,budget,charred))return false;
        if(high.volume>1.e-12)outside.push_back(std::move(high));inside=std::move(low);if(inside.volume<=1.e-12)break;}
    if(inside.volume<=1.e-12){out.push_back(source);return true;}
    if(caps)for(const auto&p:outside)for(const auto&f:p.faces)if(f.fresh)for(const auto&plane:field)if(storm_ship_hull::dot(f.plane.normal,plane.normal)<-.99999f&&std::abs(f.plane.distance+plane.distance)<1.e-5f){caps->push_back(f);break;}
    if(!budget.take(budget.left.convexPieces,outside.size()))return false;out.insert(out.end(),std::make_move_iterator(outside.begin()),std::make_move_iterator(outside.end()));return true;
}
inline std::vector<Piece>prismPieces(const MaterialPrism&prism,const Mesh&appearance){
    constexpr unsigned tetra[3][4]={{0,1,2,5},{0,1,4,5},{0,3,4,5}},faces[4][4]={{0,1,2,3},{0,3,1,2},{0,2,3,1},{1,3,2,0}};std::vector<Piece>result;
    if(appearance.empty()||appearance.begin()->second.empty())return result;const auto vertex=appearance.begin()->second.front()[0];
    for(const auto&indices:tetra){Piece piece;for(const auto&f:faces){Face face;face.object=prism.object;for(unsigned n=0;n<3;++n){auto v=vertex;v.position=prism.vertices[indices[f[n]]];face.vertices.push_back(v);}auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(face.vertices[1].position-face.vertices[0].position,face.vertices[2].position-face.vertices[0].position));if(storm_ship_hull::dot(normal,prism.vertices[indices[f[3]]]-face.vertices[0].position)>0){std::swap(face.vertices[1],face.vertices[2]);normal=normal*-1.f;}face.plane={normal,storm_ship_hull::dot(normal,face.vertices.front().position)};for(auto&v:face.vertices)v.normal=normal;piece.faces.push_back(std::move(face));}measure(piece);if(piece.volume>1.e-12)result.push_back(std::move(piece));}return result;
}
inline bool inside(const Mesh&boundary,Vec3 point){double angle=0;for(const auto&o:boundary)for(const auto&t:o.second){const auto a=t[0].position-point,b=t[1].position-point,c=t[2].position-point;const double la=storm_ship_hull::length(a),lb=storm_ship_hull::length(b),lc=storm_ship_hull::length(c);const double denominator=la*lb*lc+storm_ship_hull::dot(a,b)*lc+storm_ship_hull::dot(b,c)*la+storm_ship_hull::dot(c,a)*lb;angle+=2*std::atan2(double(storm_ship_hull::dot(a,storm_ship_hull::cross(b,c))),denominator);}return std::abs(angle)>6.283185307179586;}
// Old checkpoints contain boundaries, not convex decomposition. Partition only
// this changed source cell once. An explicit finite cache failure leaves its
// exact old boundary available to the worker compatibility path.
inline bool decompose(const std::vector<Piece>&authored,const Mesh&current,std::vector<Piece>&out){
    if(current.empty())return true;std::vector<Halfspace>cuts;for(const auto&o:current)for(const auto&t:o.second){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));if(storm_ship_hull::length(normal)<.5f)continue;Halfspace plane{normal,storm_ship_hull::dot(normal,t[0].position)};bool known=false;for(const auto&p:cuts)known|=(storm_ship_hull::dot(p.normal,plane.normal)>.99999f&&std::abs(p.distance-plane.distance)<1.e-6f)||(storm_ship_hull::dot(p.normal,plane.normal)<-.99999f&&std::abs(p.distance+plane.distance)<1.e-6f);if(!known)cuts.push_back(plane);}
    Limits limits;limits.planeSplits=262144;limits.convexPieces=4096;Budget budget(limits);std::vector<Piece>parts=authored;
    for(const auto&plane:cuts){storm_geometry_work::checkpoint();std::vector<Piece>next;for(const auto&p:parts){Piece low,high;if(!clip(p,plane,low,budget)||!clip(p,{plane.normal*-1.f,-plane.distance},high,budget))return false;if(low.volume>1.e-12)next.push_back(std::move(low));if(high.volume>1.e-12)next.push_back(std::move(high));if(next.size()>4096)return false;}parts=std::move(next);}
    for(auto&p:parts)if(inside(current,center(p))){for(auto&f:p.faces)f.fresh=false;out.push_back(std::move(p));}return true;
}
inline bool touching(const Piece&a,const Piece&b,Budget&budget){if(!overlapBounds(a.bounds,b.bounds,.000002f))return false;if(!budget.take(budget.left.contactPairs))return false;
    // Positive material overlap, or a finite shared face. Merely sharing an
    // expanded bbox/nearby origin never establishes a structural connection.
    // Authored partitions usually share a face. Prove that cheap alternative
    // before constructing a complete solid Boolean merely to find zero volume.
    for(const auto&fa:a.faces)for(const auto&fb:b.faces)if(storm_ship_hull::dot(fa.plane.normal,fb.plane.normal)<-.99999f&&std::abs(fa.plane.distance+fb.plane.distance)<=.000002f){auto polygon=fa.vertices;for(const auto&f:b.faces){std::vector<Vertex>in,out;linearSplit(polygon,f.plane,in,out);polygon=std::move(in);if(polygon.size()<3)break;}if(polygonArea(polygon)>1.e-10f)return true;}
    Piece overlap=a;for(const auto&f:b.faces){Piece next;if(!clip(overlap,f.plane,next,budget))return false;overlap=std::move(next);if(overlap.volume<=1.e-12)break;}return overlap.volume>1.e-12;
}
inline bool mergeConvex(const Piece&a,const Piece&b,Piece&merged){if(a.supportCell!=b.supportCell||a.authoredOwner!=b.authoredOwner||!overlapBounds(a.bounds,b.bounds))return false;bool shared=false;for(const auto&fa:a.faces)for(const auto&fb:b.faces)shared|=storm_ship_hull::dot(fa.plane.normal,fb.plane.normal)<-.99999f&&std::abs(fa.plane.distance+fb.plane.distance)<1.e-6f;if(!shared)return false;
    const std::vector<Piece>pair{a,b};Piece next;next.supportCell=a.supportCell;next.authoredOwner=a.authoredOwner;for(size_t n=0;n<2;++n)for(const auto&face:pair[n].faces)for(auto polygon:externalFace(pair,n,face)){Face outside=face;outside.vertices=std::move(polygon);next.faces.push_back(std::move(outside));}
    if(next.faces.empty())return false;for(const auto&face:next.faces)for(const auto&other:next.faces)for(const auto&v:other.vertices)if(planarSide(face.plane,v.position)>0)return false;measure(next);
    if(std::abs(next.volume-a.volume-b.volume)>std::max(1.e-10,(a.volume+b.volume)*1.e-5))return false;merged=std::move(next);return true;
}
// Convex decomposition is a cache, not growing cutter history. Cancel obsolete
// internal partitions whenever their exact union is convex; never bridge wood.
inline bool coalesce(std::vector<Piece>&pieces,Budget&budget){bool changed=true;while(changed){changed=false;const auto index=pieceIndex(pieces);std::vector<uint32_t>nearby;for(uint32_t a=0;a<pieces.size()&&!changed;++a){index.queryBounds(pieces[a].bounds,nearby);for(auto b:nearby)if(b>a&&pieces[a].supportCell==pieces[b].supportCell){if(!budget.take(budget.left.contactPairs))return false;Piece merged;if(!mergeConvex(pieces[a],pieces[b],merged))continue;pieces[a]=std::move(merged);pieces.erase(pieces.begin()+b);changed=true;break;}}}return true;}
inline std::shared_ptr<const Mesh>boundary(const MaterialCells&cells,uint32_t id){const auto found=cells.changed.find(id);return found==cells.changed.end()?cells.source->solids.at(id):found->second;}
inline std::shared_ptr<Source>buildSource(std::shared_ptr<const MaterialCellSource>material,std::shared_ptr<const storm_ship_structure::Layout>layout={}){auto source=std::make_shared<Source>();source->material=std::move(material);source->layout=std::move(layout);if(source->layout)source->supportSources.resize(source->layout->cells.size());source->pieces.resize(source->material->solids.size());
    for(uint32_t id=0;id<source->material->prisms.size();++id){storm_geometry_work::checkpoint();auto parts=prismPieces(source->material->prisms[id],*source->material->solids[id]);if(source->layout){std::vector<Piece>partition;std::vector<uint32_t>ids;Budget budget;for(const auto&part:parts){storm_ship_structure::queryCells(*source->layout,part.bounds,ids);for(auto cell:ids){Piece inside=part;const auto box=source->layout->cells[cell].bounds;for(unsigned axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){Piece next;if(!clip(inside,{storm_ship_structure::axisVector(axis,sign),sign*storm_ship_structure::coordinate(sign<0?box.lo:box.hi,axis)},next,budget))return {};inside=std::move(next);}if(inside.volume>1.e-12){inside.supportCell=cell;for(auto&f:inside.faces){f.partition=f.fresh;f.fresh=false;}partition.push_back(std::move(inside));source->supportSources[cell].push_back(id);}}}parts=std::move(partition);}source->pieces[id]=std::make_shared<const std::vector<Piece>>(std::move(parts));}
    for(auto&ids:source->supportSources){std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());}
    return source;
}
inline void refreshSupportMeasures(State&state,const std::set<uint32_t>&changed){
    if(!state.source->layout)return;
    const auto&layout=*state.source->layout;std::set<uint32_t>supports;
    if(state.supportMeasures.size()!=layout.cells.size()){
        state.supportMeasures.assign(layout.cells.size(),{});
        for(uint32_t id=0;id<layout.cells.size();++id)supports.insert(id);
    }else for(auto id:changed)if(const auto&parts=state.source->pieces[id])
        for(const auto&piece:*parts)if(piece.supportCell<layout.cells.size())supports.insert(piece.supportCell);
    for(auto support:supports){auto&value=state.supportMeasures[support];value={};
        for(auto id:state.source->supportSources[support])if(const auto&cell=state.cells[id];cell&&cell->pieces)
            for(const auto&piece:*cell->pieces)if(piece.supportCell==support){
                value.bounds=value.volume?storm_ship_hull::merge(value.bounds,piece.bounds):piece.bounds;
                value.volume+=piece.volume;value.moment=value.moment+center(piece)*float(piece.volume);
            }
        const auto&authored=layout.cells[support];value.area=authored.volume>0?authored.area*value.volume/authored.volume:0;
    }
}
inline bool components(State&state,Budget&budget){state.components.clear();
    std::vector<size_t>offsets(state.cells.size()+1);
    for(size_t id=0;id<state.cells.size();++id)offsets[id+1]=offsets[id]+(state.cells[id]&&state.cells[id]->pieces?state.cells[id]->pieces->size():0);
    std::vector<uint8_t>seen(offsets.back());
    const auto visit=[&](Key key){if(key.cell>=state.cells.size()||key.piece>=offsets[key.cell+1]-offsets[key.cell])return false;auto&mark=seen[offsets[key.cell]+key.piece];if(mark)return false;mark=1;return true;};
    for(uint32_t id=0;id<state.cells.size();++id){const auto&cell=state.cells[id];if(!cell||!cell->pieces)continue;for(uint32_t n=0;n<cell->pieces->size();++n){Key start{id,n};if(!visit(start))continue;Component group;group.authoredOwner=cell->pieces->at(n).authoredOwner;group.pieces.push_back(start);
            for(size_t at=0;at<group.pieces.size();++at){if(!budget.take(budget.left.graphVisits))return false;const auto key=group.pieces[at];const auto&entry=*state.cells[key.cell];group.volume+=entry.pieces->at(key.piece).volume;for(auto other:entry.edges[key.piece])if(visit(other))group.pieces.push_back(other);}state.components.push_back(std::move(group));}}
    std::stable_sort(state.components.begin(),state.components.end(),[](const auto&a,const auto&b){return a.volume>b.volume;});return true;
}
inline bool reconnect(State&state,const std::set<uint32_t>&changed,Budget&budget){std::set<uint32_t>dirty=changed;dirty.insert(state.pendingConnectivity.begin(),state.pendingConnectivity.end());std::set<uint32_t>affected=dirty;std::vector<uint32_t>ids;const Vec3 pad{.000002f,.000002f,.000002f};
    for(auto id:dirty){auto box=state.source->material->bounds.at(id);box.lo=box.lo-pad;box.hi=box.hi+pad;state.source->material->index.queryBounds(box,ids);affected.insert(ids.begin(),ids.end());}
    for(auto id:affected){if(!state.cells[id])continue;auto copy=std::make_shared<Cell>(*state.cells[id]);copy->edges.assign(copy->pieces?copy->pieces->size():0,{});state.cells[id]=std::move(copy);}
    for(auto id:affected){auto entry=std::const_pointer_cast<Cell>(state.cells[id]);if(!entry||!entry->pieces)continue;auto box=state.source->material->bounds[id];box.lo=box.lo-pad;box.hi=box.hi+pad;state.source->material->index.queryBounds(box,ids);
        for(uint32_t piece=0;piece<entry->pieces->size();++piece){const auto&a=entry->pieces->at(piece);for(auto other:ids){const auto&neighbor=state.cells[other];if(!neighbor||!neighbor->pieces)continue;for(uint32_t n=0;n<neighbor->pieces->size();++n){if(id==other&&n==piece)continue;const auto&b=neighbor->pieces->at(n);if(a.authoredOwner!=b.authoredOwner)continue;if(touching(a,b,budget))entry->edges[piece].push_back({other,n});if(!budget.complete)return false;}}}}
    refreshSupportMeasures(state,dirty);
    if(!components(state,budget))return false;state.pendingConnectivity.clear();return true;
}
inline std::shared_ptr<const State>prepare(const MaterialCells&cells,std::shared_ptr<const storm_ship_structure::Layout>layout={}){
    if(!layout&&cells.convex)layout=cells.convex->source->layout;auto next=std::make_shared<State>();if(cells.convex&&cells.convex->source->material==cells.source&&cells.convex->source->layout==layout)*next=*cells.convex;else{
        auto source=buildSource(cells.source,std::move(layout));if(!source)return {};next->source=source;next->cells.resize(cells.source->solids.size());
        // Establish immutable authorship with the same positive-piece graph
        // used after cuts. One source prism can cross disconnected authored
        // support regions; a per-prism union falsely releases those on first hit.
        std::set<uint32_t>authored;
        for(uint32_t id=0;id<source->pieces.size();++id)if(source->pieces[id]){auto cell=std::make_shared<Cell>();cell->boundary=cells.source->solids[id];cell->pieces=source->pieces[id];next->cells[id]=std::move(cell);authored.insert(id);}
        Budget budget;if(!reconnect(*next,authored,budget))return {};
        std::vector<std::vector<uint32_t>>owners(source->pieces.size());
        for(uint32_t id:authored)owners[id].resize(source->pieces[id]->size());
        for(uint32_t owner=0;owner<next->components.size();++owner){auto&component=next->components[owner];component.authoredOwner=owner;for(auto key:component.pieces)owners[key.cell][key.piece]=owner;}
        for(uint32_t id:authored){auto parts=std::make_shared<std::vector<Piece>>(*source->pieces[id]);for(size_t n=0;n<parts->size();++n)parts->at(n).authoredOwner=owners[id][n];source->pieces[id]=parts;auto cell=std::make_shared<Cell>(*next->cells[id]);cell->pieces=std::move(parts);
            // Numerical directed contacts may point back to an already visited
            // authored component. Apply the same owner fence as reconnect now,
            // so the immutable graph remains valid when pieces are removed.
            for(size_t n=0;n<cell->edges.size();++n)std::erase_if(cell->edges[n],[&](Key key){return owners[id][n]!=owners[key.cell][key.piece];});
            next->cells[id]=std::move(cell);}
    }
    std::set<uint32_t>changed;for(uint32_t id=0;id<next->cells.size();++id){if(!next->source->pieces[id])continue;const auto current=boundary(cells,id);if(next->cells[id]&&next->cells[id]->boundary==current)continue;auto cell=std::make_shared<Cell>();cell->boundary=current;
        if(current==cells.source->solids[id])cell->pieces=next->source->pieces[id];else{std::vector<Piece>pieces;if(!decompose(*next->source->pieces[id],*current,pieces))return {};cell->pieces=std::make_shared<const std::vector<Piece>>(std::move(pieces));}next->cells[id]=std::move(cell);changed.insert(id);}
    Budget budget;if((!changed.empty()||!next->pendingConnectivity.empty())&&!reconnect(*next,changed,budget))return {};next->owner=&cells;return next;
}
struct Cut {std::vector<Halfspace>planes;Bounds bounds{};Vec3 origin{};bool charred=false;};
struct Body {std::vector<Piece>pieces;std::vector<uint32_t>sources;std::shared_ptr<const State>reference;std::vector<uint32_t>supports;Bounds bounds{};Vec3 centroid{};double volume=0,surfaceArea=0;};
struct ContactSnapshot {std::shared_ptr<const MaterialCells>cells;std::shared_ptr<const storm_ship_structure::State>strength;uint64_t revision=0;std::shared_ptr<const std::vector<uint8_t>>retainedSupports;};
struct ContactLimits:Limits {size_t tetrahedra=512,bodyGroups=storm_hull_damage::MaxFragmentGroups,passes=3;
    ContactLimits(){candidateCells=64;convexPieces=512;planeSplits=8192;contactPairs=4096;graphVisits=32768;}};
struct PhysicalDelta {ContactSnapshot before,after;Plane field;std::vector<Cut>ablation;std::vector<Body>released;Mesh caps;std::vector<uint32_t>changedCells;std::vector<std::shared_ptr<const PhysicalDelta>>steps;};
// Several contacts in one simulation step update physics immediately. The
// renderer consumes one combined delta after that step, retaining each body's
// original transfer identity and exactly the first visual source snapshot.
inline std::shared_ptr<const PhysicalDelta> combineContactDeltas(
    const std::shared_ptr<const PhysicalDelta>&before,const std::shared_ptr<const PhysicalDelta>&next){
    if(!before)return next;
    if(!next||before->after.revision!=next->before.revision||before->after.cells!=next->before.cells||
       before->released.size()+next->released.size()>storm_hull_damage::MaxFragmentGroups||
       before->field.beams.size()+next->field.beams.size()>storm_hull_damage::MaxBeamSamples)return {};
    auto combined=std::make_shared<PhysicalDelta>(*before);
    if(combined->steps.empty())combined->steps.push_back(before);
    combined->steps.push_back(next);combined->after=next->after;
    combined->field.beams.insert(combined->field.beams.end(),next->field.beams.begin(),next->field.beams.end());
    combined->ablation.insert(combined->ablation.end(),next->ablation.begin(),next->ablation.end());
    combined->released.insert(combined->released.end(),next->released.begin(),next->released.end());
    appendMesh(combined->caps,next->caps);
    combined->changedCells.insert(combined->changedCells.end(),next->changedCells.begin(),next->changedCells.end());
    std::sort(combined->changedCells.begin(),combined->changedCells.end());
    combined->changedCells.erase(std::unique(combined->changedCells.begin(),combined->changedCells.end()),combined->changedCells.end());
    return combined;
}
struct ContactEdit {bool complete=false;ContactSnapshot next;std::vector<uint32_t>changedCells;std::vector<Body>detached;double spentWork=0,remainingWork=0;std::shared_ptr<const PhysicalDelta>physicalDelta;};
template<class Fn>inline void forEachBodyPiece(const Body&body,Bounds box,Fn&&fn){
    if(!body.reference){for(const auto&piece:body.pieces)if(overlapBounds(box,piece.bounds))fn(piece);return;}
    std::vector<uint32_t>ids;body.reference->source->material->index.queryBounds(box,ids);for(auto id:ids){const auto&cell=body.reference->cells[id];if(!cell||!cell->pieces)continue;for(const auto&piece:*cell->pieces)if(std::binary_search(body.supports.begin(),body.supports.end(),piece.supportCell)&&overlapBounds(box,piece.bounds))fn(piece);}
}
template<class Fn>inline void forEachRetainedPiece(const ContactSnapshot&snapshot,Bounds box,Fn&&fn){if(!snapshot.cells||!snapshot.cells->convex)return;std::vector<uint32_t>ids;snapshot.cells->source->index.queryBounds(box,ids);for(auto id:ids){const auto&cell=snapshot.cells->convex->cells[id];if(!cell||!cell->pieces)continue;for(const auto&piece:*cell->pieces)if((!snapshot.retainedSupports||piece.supportCell==storm_ship_structure::NoCell||(piece.supportCell<snapshot.retainedSupports->size()&&snapshot.retainedSupports->at(piece.supportCell)))&&overlapBounds(box,piece.bounds))fn(piece);}}
inline bool traceBody(const Body&body,Vec3 start,Vec3 end,float&fraction,Vec3&normal){bool hit=false;const auto direction=end-start;const auto box=storm_ship_hull::merge(Bounds{start,start},{end,end});forEachBodyPiece(body,box,[&](const Piece&piece){float low=0,high=std::min(1.f,fraction);Vec3 entry=storm_ship_hull::normalized(direction)*-1.f;for(const auto&face:piece.faces){const float distance=face.plane.distance-storm_ship_hull::dot(face.plane.normal,start),slope=storm_ship_hull::dot(face.plane.normal,direction);if(std::abs(slope)<1.e-12f){if(distance<0)return;continue;}const float t=distance/slope;if(slope<0){if(t>low){low=t;entry=face.plane.normal;}}else high=std::min(high,t);if(low>high)return;}if(low<=high&&low<=fraction){fraction=low;normal=entry;hit=true;}});return hit;}
inline void measureBody(Body&body){bool first=true;double total=0;Vec3 moment{};body.surfaceArea=0;const auto add=[&](const Piece&piece){body.bounds=first?piece.bounds:storm_ship_hull::merge(body.bounds,piece.bounds);first=false;total+=piece.volume;moment=moment+center(piece)*float(piece.volume);for(const auto&face:piece.faces)body.surfaceArea+=polygonArea(face.vertices);};
    if(body.reference){for(auto id:body.supports){const auto&value=body.reference->supportMeasures.at(id);if(value.volume<=0)continue;body.bounds=first?value.bounds:storm_ship_hull::merge(body.bounds,value.bounds);first=false;total+=value.volume;moment=moment+value.moment;body.surfaceArea+=value.area;}}else for(const auto&piece:body.pieces)add(piece);body.volume=total;body.centroid=total>0?moment*float(1./total):Vec3{};}
inline Body expandBody(const Body&body){if(!body.reference)return body;Body result=body;result.reference.reset();for(auto id:body.sources){const auto&cell=body.reference->cells[id];if(cell&&cell->pieces)for(auto piece:*cell->pieces)if(std::binary_search(body.supports.begin(),body.supports.end(),piece.supportCell)){for(auto&face:piece.faces)face.fresh=face.partition;result.pieces.push_back(std::move(piece));}}return result;}
// Defined by the material executor, after its paid-crush and role helpers.
inline ContactEdit tryContactEdit(const ContactSnapshot&,const Plane&,const std::map<int,float>&,double,const ContactLimits&);
inline bool projectContactDelta(const Input&,const PhysicalDelta&,Projection&);
struct Edit {std::shared_ptr<const MaterialCells>cells;std::vector<uint32_t>changed;Mesh caps;bool complete=false;};
inline bool compatible(const MaterialCells&cells){return cells.convex&&cells.convex->owner==&cells;}
inline std::vector<std::vector<Vertex>>visibleFace(const State&state,Key key,const Face&face){std::vector<std::vector<Vertex>>fragments{face.vertices};Bounds box{face.vertices.front().position,face.vertices.front().position};for(const auto&v:face.vertices)box=storm_ship_hull::merge(box,{v.position,v.position});std::vector<uint32_t>nearby;state.source->material->index.queryBounds(box,nearby);
    for(auto id:nearby){const auto&cell=state.cells[id];if(!cell||!cell->pieces)continue;for(uint32_t n=0;n<cell->pieces->size()&&!fragments.empty();++n){const Key other{id,n};if(other==key)continue;const auto&piece=cell->pieces->at(n);if(!overlapBounds(box,piece.bounds))continue;if(key<other&&aligned(face,piece))continue;std::vector<std::vector<Vertex>>next;for(auto&polygon:fragments){auto outside=outsideFace(std::move(polygon),piece);next.insert(next.end(),std::make_move_iterator(outside.begin()),std::make_move_iterator(outside.end()));}fragments=std::move(next);}}
    return fragments;
}
inline Edit edit(std::shared_ptr<const MaterialCells>original,const std::vector<Cut>&cuts,Limits limits={},bool workerPrepare=true,bool rebuildConnectivity=true){Edit result;result.cells=original;if(!original||!original->source)return result;const auto prepared=compatible(*original)?original->convex:workerPrepare?prepare(*original):nullptr;if(!prepared)return result;
    auto state=std::make_shared<State>(*prepared);auto cells=std::make_shared<MaterialCells>(*original);Budget budget(limits);std::set<uint32_t>changed,cleared;std::vector<uint32_t>ids;
    for(const auto&cut:cuts){storm_geometry_work::checkpoint();original->source->index.queryBounds(cut.bounds,ids);if(!budget.take(budget.left.candidateCells,ids.size()))return result;
        for(auto id:ids){const auto&prior=state->cells[id];if(!prior||!prior->pieces)continue;std::vector<Piece>next;double before=0,after=0;
            for(auto piece:*prior->pieces){if(!cleared.count(id))for(auto&f:piece.faces)f.fresh=false;before+=piece.volume;if(!overlapBounds(piece.bounds,cut.bounds)){next.push_back(std::move(piece));continue;}if(!budget.take(budget.left.convexPieces))return result;if(!subtract(piece,cut.planes,next,budget,nullptr,cut.charred))return result;}
            for(const auto&p:next)after+=p.volume;cleared.insert(id);if(before-after<=std::max(1.e-12,before*1.e-8))continue;if(!coalesce(next,budget))return result;auto entry=std::make_shared<Cell>(*prior);entry->pieces=std::make_shared<const std::vector<Piece>>(std::move(next));state->cells[id]=std::move(entry);changed.insert(id);}}
    if(changed.empty()){result.complete=true;return result;}
    for(auto id:changed){auto entry=std::make_shared<Cell>(*state->cells[id]);entry->boundary=std::make_shared<const Mesh>(mesh(*entry->pieces));cells->changed[id]=entry->boundary;state->cells[id]=std::move(entry);}
    if(rebuildConnectivity){if(!reconnect(*state,changed,budget))return result;}
    else{
        // Physics needs the changed positive volumes now. Full fine-grained
        // connectivity is a worker projection, not a per-contact traversal.
        state->pendingConnectivity.insert(changed.begin(),changed.end());
        refreshSupportMeasures(*state,changed);
    }
    for(auto id:changed){const auto&entry=*state->cells[id];for(uint32_t n=0;n<entry.pieces->size();++n)for(const auto&face:entry.pieces->at(n).faces)if(face.fresh)for(const auto&polygon:visibleFace(*state,{id,n},face))storm_ship_hull::fan(polygon,result.caps[face.object]);}
    state->owner=cells.get();cells->convex=std::move(state);result.cells=std::move(cells);result.changed.assign(changed.begin(),changed.end());result.complete=true;return result;
}
inline std::vector<Component>detached(const State&state){std::set<uint32_t>retained;std::vector<Component>out;for(const auto&component:state.components){if(retained.insert(component.authoredOwner).second)continue;out.push_back(component);}return out;}
inline bool hasDetached(const State&state){std::set<uint32_t>retained;for(const auto&component:state.components)if(!retained.insert(component.authoredOwner).second)return true;return false;}
inline std::shared_ptr<const MaterialCells>ready(std::shared_ptr<const MaterialCells>cells,std::shared_ptr<const storm_ship_structure::Layout>layout={}){if(!cells||(compatible(*cells)&&cells->convex->pendingConnectivity.empty()&&(!layout||cells->convex->source->layout==layout)))return cells;auto next=std::make_shared<MaterialCells>(*cells);next->convex=prepare(*next,std::move(layout));return next->convex?next:nullptr;}
inline std::shared_ptr<const MaterialCells>take(std::shared_ptr<const MaterialCells>original,const std::vector<Component>&groups,Budget&budget,std::vector<uint32_t>*changedOutput=nullptr){
    if(!original||!compatible(*original))return {};
    auto cells=std::make_shared<MaterialCells>(*original);auto state=std::make_shared<State>(*original->convex);
    // Finish only actual shape edits. Removing whole positive pieces cannot
    // create a new edge: filter/remap the existing exact graph instead of
    // repeating solid Booleans for every neighbor of a large released section.
    if(!state->pendingConnectivity.empty()&&!reconnect(*state,{},budget))return {};
    std::map<uint32_t,std::set<uint32_t>>removed;for(const auto&group:groups)for(auto key:group.pieces)removed[key.cell].insert(key.piece);
    std::map<uint32_t,std::vector<uint32_t>>mapping;std::set<uint32_t>changed;
    for(const auto&entry:removed){
        if(entry.first>=state->cells.size()||!state->cells[entry.first]||!state->cells[entry.first]->pieces)return {};
        auto&indices=mapping[entry.first];indices.resize(state->cells[entry.first]->pieces->size(),UINT32_MAX);uint32_t next=0;
        for(uint32_t n=0;n<indices.size();++n)if(!entry.second.count(n))indices[n]=next++;
        changed.insert(entry.first);
    }
    for(uint32_t id=0;id<state->cells.size();++id){const auto&old=state->cells[id];if(!old||!old->pieces)continue;
        const auto own=mapping.find(id);bool affected=own!=mapping.end();
        if(!affected)for(const auto&edges:old->edges)for(auto key:edges)if(mapping.count(key.cell)){affected=true;break;}
        if(!affected)continue;
        auto cell=std::make_shared<Cell>(*old);
        if(own!=mapping.end()){
            std::vector<Piece>pieces;for(uint32_t n=0;n<old->pieces->size();++n)if(own->second[n]!=UINT32_MAX)pieces.push_back(old->pieces->at(n));
            cell->pieces=std::make_shared<const std::vector<Piece>>(std::move(pieces));
            cell->boundary=std::make_shared<const Mesh>(mesh(*cell->pieces));cells->changed[id]=cell->boundary;
        }
        cell->edges.assign(cell->pieces->size(),{});
        for(uint32_t n=0;n<old->pieces->size();++n){const auto to=own==mapping.end()?n:own->second[n];if(to==UINT32_MAX)continue;
            for(auto key:old->edges[n]){if(const auto found=mapping.find(key.cell);found!=mapping.end()){
                    if(key.piece>=found->second.size())return {};key.piece=found->second[key.piece];if(key.piece==UINT32_MAX)continue;}
                cell->edges[to].push_back(key);}}
        state->cells[id]=std::move(cell);
    }
    refreshSupportMeasures(*state,changed);if(!components(*state,budget))return {};
    state->owner=cells.get();cells->convex=std::move(state);if(changedOutput)changedOutput->assign(changed.begin(),changed.end());return cells;
}
// Worker-only realization of an already committed support ownership mask.
// The old positive pieces remain shared by CPU bodies until this projection.
inline std::shared_ptr<const MaterialCells>materialize(const ContactSnapshot&snapshot,Mesh&caps){
    if(!snapshot.cells||!compatible(*snapshot.cells))return {};if(!snapshot.retainedSupports)return ready(snapshot.cells);const auto&before=*snapshot.cells->convex;Component removed;
    for(uint32_t id=0;id<before.cells.size();++id){const auto&cell=before.cells[id];if(!cell||!cell->pieces)continue;for(uint32_t p=0;p<cell->pieces->size();++p){const auto support=cell->pieces->at(p).supportCell;if(support<snapshot.retainedSupports->size()&&!snapshot.retainedSupports->at(support))removed.pieces.push_back({id,p});}}
    if(removed.pieces.empty())return ready(snapshot.cells);Budget budget;std::vector<uint32_t>changed;auto cells=take(snapshot.cells,{removed},budget,&changed);if(!cells)return {};
    std::set<uint32_t>affected(changed.begin(),changed.end());std::vector<uint32_t>nearby;const Vec3 pad{.000002f,.000002f,.000002f};for(auto id:changed){auto box=cells->source->bounds[id];box.lo=box.lo-pad;box.hi=box.hi+pad;cells->source->index.queryBounds(box,nearby);affected.insert(nearby.begin(),nearby.end());}
    const std::vector<uint32_t>sources(affected.begin(),affected.end());std::vector<Mesh>sourceCaps(sources.size());
    if(!storm_geometry_work::parallelFor(sources.size(),[&](size_t n){
        storm_geometry_work::checkpoint(true);const auto id=sources[n];const auto&cell=cells->convex->cells[id];if(!cell||!cell->pieces)return true;
        for(uint32_t p=0;p<cell->pieces->size();++p)for(const auto&face:cell->pieces->at(p).faces)if(face.partition){storm_geometry_work::checkpoint();const auto visible=visibleFace(*cells->convex,{id,p},face);if(visible.empty())continue;
            // Keys of untouched sources remain stable. In a changed source,
            // locate the same retained positive piece by support and vertices.
            const auto&old=*before.cells[id]->pieces;uint32_t prior=UINT32_MAX,surviving=0;for(uint32_t n=0;n<old.size();++n){const auto support=old[n].supportCell;if(support<snapshot.retainedSupports->size()&&!snapshot.retainedSupports->at(support))continue;if(surviving++==p){prior=n;break;}}
            if(prior==UINT32_MAX)return false;const auto previous=visibleFace(before,{id,prior},face);for(const auto&polygon:visible){std::vector<Triangle>triangles;storm_ship_hull::fan(polygon,triangles);for(const auto&t:triangles){std::vector<Triangle>fresh;if(previous.empty())fresh.push_back(t);else if(!subtractFaces(t,previous,fresh))return false;auto&out=sourceCaps[n][face.object];out.insert(out.end(),fresh.begin(),fresh.end());}}}
        return true;
    }))return {};
    // Keep authored source/face order independent of dispatch completion order.
    for(auto&source:sourceCaps)for(auto&object:source){auto&out=caps[object.first];out.insert(out.end(),std::make_move_iterator(object.second.begin()),std::make_move_iterator(object.second.end()));}
    return cells;
}
// One role projection for worker edits and already-paid contact deltas. Only
// actual released convex volumes select faces; source UV/material roles survive.
inline void prepareDecorations(Surface&surface,const std::map<int,float>&wood){
    if(surface.decorations)return;
    Surface decoration;
    for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(surface,role))
        if(!wood.count(o.first))surfaceRole(decoration,role).emplace(o.first,o.second);
    auto topology=materialTopology(decoration);
    std::map<size_t,std::shared_ptr<DecorationComponent>>components;
    for(size_t n=0;n<topology.faces.size();++n){const auto&ref=topology.faces[n];
        const auto&t=surfaceRole(decoration,ref.role).at(ref.object)[ref.triangle];
        auto&component=components[topology.root(n)];
        if(!component){component=std::make_shared<DecorationComponent>();component->bounds=storm_ship_structure::detail::triangleBounds(t);}
        else component->bounds=storm_ship_hull::merge(component->bounds,storm_ship_structure::detail::triangleBounds(t));
        component->roles[ref.role][ref.object].push_back(t);
    }
    auto cached=std::make_shared<DecorationList>();cached->reserve(components.size());
    for(auto&entry:components)cached->push_back(std::move(entry.second));surface.decorations=std::move(cached);
}
inline bool partitionSurface(const Surface&source,const MaterialCells&retained,const std::vector<Body>&bodies,const std::map<int,float>&wood,std::vector<Surface>&out,std::set<int>*changedObjects=nullptr){
    if(!compatible(retained))return false;std::vector<Surface>groups(bodies.size()+1);struct Ref{uint32_t body,piece;};std::vector<Ref>refs;std::vector<Bounds>boxes;
    for(uint32_t n=0;n<bodies.size();++n)for(uint32_t p=0;p<bodies[n].pieces.size();++p){refs.push_back({n,p});boxes.push_back(bodies[n].pieces[p].bounds);}storm_ship_hull::Broadphase index;index.build(boxes);std::vector<uint32_t>nearby;Surface decoration;
    const auto touchingTriangle=[&](const Triangle&t,const Piece&piece){for(const auto&face:piece.faces){std::vector<Triangle>triangles;storm_ship_hull::fan(face.vertices,triangles);for(const auto&other:triangles)if(supportTrianglesTouch(t,other))return true;}return false;};
    struct Chunk{unsigned role;int object;const std::vector<Triangle>*triangles;size_t first,last;std::map<unsigned,std::vector<Triangle>>groups;bool changed=false;};
    std::vector<Chunk>chunks;
    for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(source,role)){
        surfaceRole(groups[0],role).try_emplace(o.first);if(!wood.count(o.first))continue;
        for(size_t first=0;first<o.second.size();first+=512)chunks.push_back({role,o.first,&o.second,first,std::min(first+512,o.second.size()),{},false});
    }
    if(!storm_geometry_work::parallelFor(chunks.size(),[&](size_t n){
        storm_geometry_work::checkpoint(true);auto&chunk=chunks[n];std::vector<uint32_t>candidates;
        for(size_t i=chunk.first;i<chunk.last;++i){const auto&t=chunk.triangles->at(i);storm_geometry_work::checkpoint();std::vector<std::vector<Vertex>>remaining{{t.begin(),t.end()}};index.queryBounds(storm_ship_structure::detail::triangleBounds(t),candidates);chunk.changed|=!candidates.empty();
            for(auto id:candidates){const auto ref=refs[id];const auto&piece=bodies[ref.body].pieces[ref.piece];std::vector<std::vector<Vertex>>outside;for(auto polygon:remaining){for(const auto&face:piece.faces){std::vector<Vertex>in,rejected;linearSplit(polygon,face.plane,in,rejected);if(rejected.size()>=3&&polygonArea(rejected)>1.e-12f)outside.push_back(std::move(rejected));polygon=std::move(in);if(polygon.size()<3)break;}if(polygon.size()>=3)storm_ship_hull::fan(polygon,chunk.groups[ref.body+1]);}remaining=std::move(outside);if(remaining.empty())break;}
            for(const auto&polygon:remaining)storm_ship_hull::fan(polygon,chunk.groups[0]);
        }return true;
    }))return false;
    for(auto&chunk:chunks){if(chunk.changed&&changedObjects)changedObjects->insert(chunk.object);
        for(auto&part:chunk.groups){auto&triangles=surfaceRole(groups[part.first],chunk.role)[chunk.object];triangles.insert(triangles.end(),std::make_move_iterator(part.second.begin()),std::make_move_iterator(part.second.end()));}
    }
    auto decorations=source.decorations;
    if(!decorations){for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(source,role))if(!wood.count(o.first))surfaceRole(decoration,role).emplace(o.first,o.second);prepareDecorations(decoration,wood);decorations=decoration.decorations;}
    std::vector<DecorationList>members(groups.size());
    const Vec3 pad{.000002f,.000002f,.000002f};
    for(const auto&component:*decorations){std::set<unsigned>supports;auto candidate=component->bounds;candidate.lo=candidate.lo-pad;candidate.hi=candidate.hi+pad;index.queryBounds(candidate,nearby);
        if(!nearby.empty())for(unsigned role=0;role<4&&supports.size()<=1&&!supports.count(0);++role)for(const auto&o:component->roles[role]){
            if(supports.count(0)||supports.size()>1)break;
            for(const auto&t:o.second){auto box=storm_ship_structure::detail::triangleBounds(t);box.lo=box.lo-pad;box.hi=box.hi+pad;index.queryBounds(box,nearby);for(auto n:nearby){const auto part=refs[n];if(touchingTriangle(t,bodies[part.body].pieces[part.piece]))supports.insert(part.body+1);}
                retained.source->index.queryBounds(box,nearby);for(auto sourceID:nearby){const auto&cell=retained.convex->cells[sourceID];if(!cell||!cell->pieces)continue;for(const auto&piece:*cell->pieces)if(overlapBounds(box,piece.bounds)&&touchingTriangle(t,piece)){supports.insert(0);break;}}
                if(supports.count(0)||supports.size()>1)break;
            }
        }
        const unsigned target=supports.size()==1?*supports.begin():0;members[target].push_back(component);
        for(unsigned role=0;role<4;++role)for(const auto&o:component->roles[role]){
            auto&triangles=surfaceRole(groups[target],role)[o.first];triangles.insert(triangles.end(),o.second.begin(),o.second.end());
            if(target&&changedObjects)changedObjects->insert(o.first);
        }
    }
    for(size_t n=0;n<groups.size();++n)groups[n].decorations=std::make_shared<const DecorationList>(std::move(members[n]));
    out=std::move(groups);return true;
}
}

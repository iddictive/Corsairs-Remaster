#pragma once
#include "ship_hull_geometry.hpp"
#include "ship_hull_damage_bridge.hpp"
#include <map>
#include <memory>
#include <limits>
#include <numeric>

// Coarse load paths, not a replacement solid or a second rendered hull. Only
// authored wood contributes occupancy, section and finite contact material.
namespace storm_ship_structure {
using storm_ship_hull::Vec3;
using storm_ship_hull::Bounds;
using storm_ship_hull::Vertex;
using storm_ship_hull::Triangle;
using BeamSample=storm_hull_damage::BeamSample;
inline constexpr uint32_t NoCell=UINT32_MAX;
struct Cell {
    Bounds bounds{};Vec3 centroid{};float area=0,volume=0,reserveLoad=0;
    uint32_t owner=0;
    std::vector<uint32_t>bonds;
};
struct Bond {
    uint32_t a=0,b=0;uint8_t axis=0;float coordinate=0,section=0,capacity=0;
    Bounds bounds{};
};
struct MaterialTetra {std::array<Vec3,4>points{};float sign=1,thickness=0;int object=0;};
struct ContactMaterial {
    std::vector<MaterialTetra>tetrahedra;storm_ship_hull::Broadphase index;
    std::vector<uint32_t>sourceIDs;
    std::shared_ptr<const ContactMaterial>base;
    std::map<uint32_t,std::shared_ptr<const ContactMaterial>>changed;
};
struct Layout {
    Bounds bounds{};std::array<uint32_t,3>divisions{1,1,1};Vec3 step{};
    std::vector<Cell>cells;std::vector<Bond>bonds;std::vector<int32_t>gridToCell;
    std::shared_ptr<const ContactMaterial>contact;double area=0,volume=0;uint64_t fingerprint=0;
};
struct State {
    std::shared_ptr<const Layout>layout;
    std::vector<float>cellHealth,bondHealth;
    // A destroyed node and a released node both cease supporting the retained
    // hull. DamageResult distinguishes removed material from released bodies.
    std::vector<uint8_t>alive;
    // Derived geometry cache: seed/load and actual fracture rebuild this once;
    // ordinary health-only marks share its immutable finite-contact BVH.
    std::shared_ptr<const ContactMaterial>contact;
    // Derived physical capacity, rebuilt from saved material by seed. Unlike
    // health these values do not contain latent mechanical weakening.
    std::vector<float>physicalCellHealth,physicalBondHealth;
};
struct DamageResult {
    struct Crush {uint32_t beam=0,cell=0;double volume=0;};
    State state;std::vector<uint32_t>brokenCells,brokenBonds,releasedCells,retainedCells;
    std::vector<Crush>crushed;
    // Components after failure, before non-owner islands are released; the
    // Largest child PER authored owner is retained; use releasedCells rather
    // than assuming there was only one connected assembly before the strike.
    std::vector<std::vector<uint32_t>>components;
    bool changed=false;double spentWork=0,remainingWork=0;
};
inline float coordinate(Vec3 p,unsigned axis){return axis==0?p.x:axis==1?p.y:p.z;}
inline Vec3 axisVector(unsigned axis,float sign=1){return axis==0?Vec3{sign,0,0}:axis==1?Vec3{0,sign,0}:Vec3{0,0,sign};}
inline uint32_t gridIndex(const Layout&l,uint32_t x,uint32_t y,uint32_t z){return (z*l.divisions[1]+y)*l.divisions[0]+x;}
inline std::array<uint32_t,3> gridCoordinate(const Layout&l,Vec3 p){
    std::array<uint32_t,3>q{};for(unsigned a=0;a<3;++a){const float s=coordinate(l.step,a);
        q[a]=s>0?uint32_t(std::clamp(int(std::floor((coordinate(p,a)-coordinate(l.bounds.lo,a))/s)),0,int(l.divisions[a])-1)):0;}return q;
}
inline Bounds gridBounds(const Layout&l,uint32_t x,uint32_t y,uint32_t z){
    const Vec3 low=l.bounds.lo+Vec3{l.step.x*x,l.step.y*y,l.step.z*z};
    return {low,{x+1==l.divisions[0]?l.bounds.hi.x:low.x+l.step.x,y+1==l.divisions[1]?l.bounds.hi.y:low.y+l.step.y,z+1==l.divisions[2]?l.bounds.hi.z:low.z+l.step.z}};
}
inline Bounds cellBounds(const Layout&l,uint32_t cell){return l.cells.at(cell).bounds;}
inline uint32_t cellAt(const Layout&l,Vec3 p){
    if(l.gridToCell.empty()||!storm_ship_hull::finite(p)||p.x<l.bounds.lo.x-1.e-5f||p.y<l.bounds.lo.y-1.e-5f||p.z<l.bounds.lo.z-1.e-5f||p.x>l.bounds.hi.x+1.e-5f||p.y>l.bounds.hi.y+1.e-5f||p.z>l.bounds.hi.z+1.e-5f)return NoCell;
    const auto q=gridCoordinate(l,p);const int32_t id=l.gridToCell[gridIndex(l,q[0],q[1],q[2])];return id<0?NoCell:uint32_t(id);
}
inline void queryCells(const Layout&l,Bounds bounds,std::vector<uint32_t>&out){
    out.clear();if(l.gridToCell.empty()||!storm_ship_hull::overlaps(l.bounds,bounds))return;
    const auto lo=gridCoordinate(l,bounds.lo),hi=gridCoordinate(l,bounds.hi);
    for(uint32_t z=lo[2];z<=hi[2];++z)for(uint32_t y=lo[1];y<=hi[1];++y)for(uint32_t x=lo[0];x<=hi[0];++x){storm_geometry_work::checkpoint();const int32_t id=l.gridToCell[gridIndex(l,x,y,z)];if(id>=0)out.push_back(uint32_t(id));}
}
inline std::vector<Vertex>clipBounds(std::vector<Vertex>polygon,Bounds b){
    std::vector<Vertex>inside,outside;for(unsigned a=0;a<3&&!polygon.empty();++a)for(float sign:{-1.f,1.f}){
        storm_ship_hull::split(polygon,{axisVector(a,sign),sign*coordinate(sign<0?b.lo:b.hi,a)},inside,outside);polygon.swap(inside);}return polygon;
}
inline std::vector<Vertex>clipCell(const std::vector<Vertex>&polygon,const Layout&l,uint32_t id){return clipBounds(polygon,cellBounds(l,id));}
inline float polygonArea(const std::vector<Vertex>&polygon){float area=0;for(size_t n=1;n+1<polygon.size();++n)area+=storm_ship_hull::area({polygon[0],polygon[n],polygon[n+1]});return area;}
namespace detail {
struct Section {double low=0,high=0;Bounds bounds{};bool hasBounds=false;};
struct Measure {std::vector<double>area,volume;std::vector<Vec3>weighted;std::map<std::pair<uint32_t,uint32_t>,Section>sections;};
using Face=std::vector<Vec3>;
using Polyhedron=std::vector<Face>;
inline Bounds triangleBounds(const Triangle&t){Bounds b{t[0].position,t[0].position};for(unsigned n=1;n<3;++n)b=storm_ship_hull::merge(b,{t[n].position,t[n].position});return b;}
inline Bounds tetraBounds(const MaterialTetra&t){Bounds b{t.points[0],t.points[0]};for(unsigned n=1;n<4;++n)b=storm_ship_hull::merge(b,{t.points[n],t.points[n]});return b;}
inline float faceArea(const Face&f){double area=0;for(size_t n=1;n+1<f.size();++n)area+=storm_ship_hull::length(storm_ship_hull::cross(f[n]-f[0],f[n+1]-f[0]))*.5;return float(area);}
inline Polyhedron tetraPoly(const MaterialTetra&t){
    constexpr unsigned faces[4][3]={{0,1,2},{0,3,1},{1,3,2},{2,3,0}};Polyhedron result;Vec3 center{};for(auto p:t.points)center=center+p*.25f;for(const auto&f:faces){Face face{t.points[f[0]],t.points[f[1]],t.points[f[2]]};if(storm_ship_hull::dot(storm_ship_hull::cross(face[1]-face[0],face[2]-face[0]),center-face[0])>0)std::swap(face[1],face[2]);result.push_back(std::move(face));}return result;
}
inline Polyhedron clipPoly(const Polyhedron&poly,const storm_ship_hull::Plane&plane){
    Polyhedron result;Face intersections;bool removed=false;for(const auto&face:poly){Face out;for(size_t n=0;n<face.size();++n){const auto a=face[n],b=face[(n+1)%face.size()];const float da=storm_ship_hull::dot(plane.normal,a)-plane.distance,db=storm_ship_hull::dot(plane.normal,b)-plane.distance;const bool ai=da<=0,bi=db<=0;removed|=!ai;if(ai)out.push_back(a);if(ai!=bi){const auto p=a+(b-a)*std::clamp(da/(da-db),0.f,1.f);out.push_back(p);bool found=false;for(auto q:intersections)if(storm_ship_hull::dot(q-p,q-p)<1.e-12f){found=true;break;}if(!found)intersections.push_back(p);}}if(out.size()>=3&&faceArea(out)>1.e-10f)result.push_back(std::move(out));}
    if(removed&&intersections.size()>=3){Vec3 center{};for(auto p:intersections)center=center+p*(1.f/intersections.size());const auto basis=storm_ship_hull::frame(plane.normal);std::sort(intersections.begin(),intersections.end(),[&](Vec3 a,Vec3 b){a=a-center;b=b-center;return std::atan2(storm_ship_hull::dot(a,basis.bitangent),storm_ship_hull::dot(a,basis.tangent))<std::atan2(storm_ship_hull::dot(b,basis.bitangent),storm_ship_hull::dot(b,basis.tangent));});if(faceArea(intersections)>1.e-10f)result.push_back(std::move(intersections));}return result;
}
inline Polyhedron clipBox(Polyhedron poly,Bounds b){for(unsigned axis=0;axis<3&&!poly.empty();++axis)for(float sign:{-1.f,1.f})poly=clipPoly(poly,{axisVector(axis,sign),sign*coordinate(sign<0?b.lo:b.hi,axis)});return poly;}
inline double polyVolume(const Polyhedron&poly){if(poly.empty())return 0;const Vec3 origin=poly[0][0];double volume=0;for(const auto&face:poly)for(size_t n=1;n+1<face.size();++n)volume+=double(storm_ship_hull::dot(face[0]-origin,storm_ship_hull::cross(face[n]-origin,face[n+1]-origin)))/6.;return std::abs(volume);}
template<class Mesh>void addSolid(ContactMaterial&material,const Mesh&solid,const std::map<int,float>&wood){
    bool first=true;Bounds bounds{};for(const auto&o:solid)if(wood.count(o.first))for(const auto&t:o.second){const auto b=triangleBounds(t);bounds=first?b:storm_ship_hull::merge(bounds,b);first=false;}if(first)return;const auto origin=(bounds.lo+bounds.hi)*.5f;
    // Signed tetrahedra preserve holes and concave current cells. Their
    // intersections sum to the material volume; inner and outer faces are not
    // separate doses and a short ray can meet only the real inner timber.
    for(const auto&o:solid){const auto found=wood.find(o.first);if(found==wood.end()||found->second<=0)continue;for(const auto&t:o.second){storm_geometry_work::checkpoint();const double volume=double(storm_ship_hull::dot(t[0].position-origin,storm_ship_hull::cross(t[1].position-origin,t[2].position-origin)))/6.;if(std::abs(volume)<=1.e-12)continue;material.tetrahedra.push_back({{origin,t[0].position,t[1].position,t[2].position},volume<0?-1.f:1.f,found->second,o.first});}}
}
inline void indexMaterial(ContactMaterial&material){std::vector<Bounds>boxes;boxes.reserve(material.tetrahedra.size());for(const auto&t:material.tetrahedra)boxes.push_back(tetraBounds(t));material.index.build(boxes);}
template<class Mesh>void addSource(ContactMaterial&material,uint32_t id,const Mesh&solid,const std::map<int,float>&wood){const size_t start=material.tetrahedra.size();addSolid(material,solid,wood);material.sourceIDs.insert(material.sourceIDs.end(),material.tetrahedra.size()-start,id);}
template<class Surface>std::shared_ptr<const ContactMaterial>contactMaterial(const Surface&surface,const std::map<int,float>&wood){
    auto material=std::make_shared<ContactMaterial>();
    if(surface.cells){for(uint32_t id=0;id<surface.cells->source->solids.size();++id){const auto changed=surface.cells->changed.find(id);const auto&mesh=changed==surface.cells->changed.end()?*surface.cells->source->solids[id]:*changed->second;addSource(*material,id,mesh,wood);}}
    else if(surface.material){for(const auto&solid:*surface.material)addSolid(*material,solid,wood);}
    else if(surface.timber){for(const auto&prism:*surface.timber){if(!wood.count(prism.object))continue;std::map<int,std::vector<Triangle>>solid;const auto center=(prism.vertices[0]+prism.vertices[1]+prism.vertices[2]+prism.vertices[3]+prism.vertices[4]+prism.vertices[5])*(1.f/6.f);constexpr unsigned faces[8][3]={{0,1,2},{3,5,4},{0,3,4},{0,4,1},{1,4,5},{1,5,2},{2,5,0},{0,5,3}};for(const auto&f:faces){Triangle t;for(unsigned n=0;n<3;++n)t[n].position=prism.vertices[f[n]];if(storm_ship_hull::dot(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position),center-t[0].position)>0)std::swap(t[1],t[2]);solid[prism.object].push_back(t);}addSolid(*material,solid,wood);}}
    indexMaterial(*material);return material;
}
inline std::vector<std::pair<const ContactMaterial*,uint32_t>>queryMaterial(const ContactMaterial&material,Bounds box){
    std::vector<std::pair<const ContactMaterial*,uint32_t>>result;std::vector<uint32_t>ids,overrides;const auto&base=material.base?*material.base:material;base.index.queryBounds(box,ids);for(uint32_t id:ids){const uint32_t source=id<base.sourceIDs.size()?base.sourceIDs[id]:NoCell;if(material.changed.count(source)){overrides.push_back(source);continue;}result.emplace_back(&base,id);}std::sort(overrides.begin(),overrides.end());overrides.erase(std::unique(overrides.begin(),overrides.end()),overrides.end());for(uint32_t source:overrides){const auto&local=material.changed.at(source);local->index.queryBounds(box,ids);for(uint32_t id:ids)result.emplace_back(local.get(),id);}return result;
}
inline Measure measure(const Layout&l,const ContactMaterial&material){
    const size_t count=l.gridToCell.size();Measure result;result.area.resize(count);result.volume.resize(count);result.weighted.resize(count);
    for(const auto&t:material.tetrahedra){storm_geometry_work::checkpoint();const auto box=tetraBounds(t);if(!storm_ship_hull::overlaps(box,l.bounds))continue;const auto lo=gridCoordinate(l,box.lo),hi=gridCoordinate(l,box.hi);const auto original=tetraPoly(t);
            for(uint32_t z=lo[2];z<=hi[2];++z)for(uint32_t y=lo[1];y<=hi[1];++y)for(uint32_t x=lo[0];x<=hi[0];++x){storm_geometry_work::checkpoint();const uint32_t grid=gridIndex(l,x,y,z);const auto bounds=gridBounds(l,x,y,z);const auto poly=clipBox(original,bounds);const double volume=polyVolume(poly)*t.sign;if(std::abs(volume)<=1.e-12)continue;result.volume[grid]+=volume;result.area[grid]+=volume/t.thickness;result.weighted[grid]=result.weighted[grid]+(bounds.lo+bounds.hi)*float(volume*.5);
                const uint32_t q[3]{x,y,z};
                for(unsigned axis=0;axis<3;++axis)for(unsigned side=0;side<2;++side){if((!side&&!q[axis])||(side&&q[axis]+1==l.divisions[axis]))continue;uint32_t neighbor[3]{x,y,z};neighbor[axis]=side?q[axis]+1:q[axis]-1;const uint32_t other=gridIndex(l,neighbor[0],neighbor[1],neighbor[2]);const auto key=std::minmax(grid,other);const float at=coordinate(side?bounds.hi:bounds.lo,axis);
                    for(const auto&face:poly){bool onPlane=true;for(auto p:face)onPlane&=std::abs(coordinate(p,axis)-at)<=2.e-5f;if(!onPlane)continue;const float area=faceArea(face);if(area<=1.e-10f)continue;auto&section=result.sections[key];(grid<other?section.low:section.high)+=double(area)*t.sign;Bounds support{face[0],face[0]};for(auto p:face)support=storm_ship_hull::merge(support,{p,p});section.bounds=section.hasBounds?storm_ship_hull::merge(section.bounds,support):support;section.hasBounds=true;}
                }
            }
    }return result;
}
inline bool validState(const State&s){if(!s.layout||s.cellHealth.size()!=s.layout->cells.size()||s.bondHealth.size()!=s.layout->bonds.size()||s.alive.size()!=s.layout->cells.size())return false;for(float h:s.cellHealth)if(!std::isfinite(h)||h<0||h>1)return false;for(float h:s.bondHealth)if(!std::isfinite(h)||h<0||h>1)return false;return true;}
inline std::vector<storm_ship_hull::Plane>beamPlanes(const BeamSample&b){
    const Vec3 start{b.start[0],b.start[1],b.start[2]},direction{b.direction[0],b.direction[1],b.direction[2]};const auto tangent=b.tangent==std::array<float,3>{}?storm_ship_hull::frame(direction).tangent:Vec3{b.tangent[0],b.tangent[1],b.tangent[2]},bitangent=storm_ship_hull::cross(direction,tangent);std::vector<storm_ship_hull::Plane>result{{direction,storm_ship_hull::dot(direction,start)+b.depth},{direction*-1.f,-storm_ship_hull::dot(direction,start)+(b.backDepth<0?b.radius*.15625f:b.backDepth)}};
    if(b.crossSection.empty()){for(unsigned n=0;n<8;++n){const float angle=float(n)*.7853981633974483f;const auto normal=tangent*std::cos(angle)+bitangent*std::sin(angle);result.push_back({normal,storm_ship_hull::dot(normal,start)+b.radius});}}
    else for(size_t n=0;n<b.crossSection.size();++n){const auto&a=b.crossSection[n],&c=b.crossSection[(n+1)%b.crossSection.size()];const auto normal=storm_ship_hull::normalized(tangent*(c[1]-a[1])+bitangent*(a[0]-c[0]));result.push_back({normal,storm_ship_hull::dot(normal,start+tangent*a[0]+bitangent*a[1])});}return result;
}
struct Contact {uint32_t cell=0;double volume=0;float travel=0;};
inline std::vector<Contact>contacts(const Layout&l,const ContactMaterial&material,const BeamSample&b){
    std::map<uint32_t,Contact>byCell;const auto planes=beamPlanes(b);const Vec3 start{b.start[0],b.start[1],b.start[2]},direction{b.direction[0],b.direction[1],b.direction[2]};
    const auto tangent=b.tangent==std::array<float,3>{}?storm_ship_hull::frame(direction).tangent:Vec3{b.tangent[0],b.tangent[1],b.tangent[2]},bitangent=storm_ship_hull::cross(direction,tangent);Bounds box{start,start};float radius=b.radius*1.082393f;for(float depth:{-(b.backDepth<0?b.radius*.15625f:b.backDepth),b.depth})for(unsigned n=0;n<(b.crossSection.empty()?8u:unsigned(b.crossSection.size()));++n){const float angle=.3926990816987241f+float(n)*.7853981633974483f;const auto xy=b.crossSection.empty()?std::array<float,2>{radius*std::cos(angle),radius*std::sin(angle)}:b.crossSection[n];const auto point=start+direction*depth+tangent*xy[0]+bitangent*xy[1];box=storm_ship_hull::merge(box,{point,point});}
    for(const auto&candidate:queryMaterial(material,box)){const auto&t=candidate.first->tetrahedra[candidate.second];storm_geometry_work::checkpoint();auto overlap=tetraPoly(t);for(const auto&p:planes){overlap=clipPoly(overlap,p);if(overlap.empty())break;}if(overlap.empty())continue;Bounds overlapBounds{overlap[0][0],overlap[0][0]};for(const auto&face:overlap)for(auto p:face)overlapBounds=storm_ship_hull::merge(overlapBounds,{p,p});std::vector<uint32_t>cells;queryCells(l,overlapBounds,cells);
        for(uint32_t cell:cells){const auto local=clipBox(overlap,l.cells[cell].bounds);const double volume=polyVolume(local)*t.sign;if(std::abs(volume)<=1.e-12)continue;float travel=std::numeric_limits<float>::infinity();for(const auto&face:local)for(auto p:face)travel=std::min(travel,storm_ship_hull::dot(p-start,direction));auto [it,inserted]=byCell.emplace(cell,Contact{cell,0,travel});it->second.volume+=volume;it->second.travel=std::min(it->second.travel,travel);}}
    std::vector<Contact>result;for(const auto&entry:byCell)if(entry.second.volume>1.e-10)result.push_back(entry.second);std::sort(result.begin(),result.end(),[](const auto&a,const auto&b){return a.travel==b.travel?a.cell<b.cell:a.travel<b.travel;});return result;
}
inline double beamVolume(const BeamSample&b){double area=0;if(b.crossSection.empty())area=8.*std::tan(.3926990816987241)*b.radius*b.radius;else for(size_t n=0;n<b.crossSection.size();++n){const auto&a=b.crossSection[n],&c=b.crossSection[(n+1)%b.crossSection.size()];area+=(double(a[0])*c[1]-double(a[1])*c[0])*.5;}return std::max(0.,area*b.depth);}
inline void overload(DamageResult&result){
    auto&s=result.state;const auto&l=*s.layout;
    // Explicit coarse game load reserve, not a gravity-only stress simulation:
    // intact incident section has 5x reserve. Remaining material still carries
    // its baseline demand after supports are lost; a thin last rail is finite.
    // The work released here belongs to that standing structural load rather
    // than being charged a second time to the arriving weapon's energy budget.
    for(unsigned pass=0;pass<4;++pass){std::vector<uint32_t>failed;for(uint32_t id=0;id<l.cells.size();++id){storm_geometry_work::checkpoint();if(!s.alive[id]||s.cellHealth[id]<=0||l.cells[id].reserveLoad<=0)continue;double capacity=0;for(uint32_t edge:l.cells[id].bonds){const auto&bond=l.bonds[edge];const uint32_t other=bond.a==id?bond.b:bond.a;if(s.alive[other])capacity+=bond.capacity*s.bondHealth[edge];}if(capacity<=0||capacity+1.e-10>=l.cells[id].reserveLoad*s.cellHealth[id])continue;for(uint32_t edge:l.cells[id].bonds)if(s.bondHealth[edge]>0)failed.push_back(edge);}if(failed.empty())break;for(uint32_t edge:failed)if(s.bondHealth[edge]>0){s.bondHealth[edge]=0;result.brokenBonds.push_back(edge);result.changed=true;}}
}
inline void islands(DamageResult&result){
    auto&s=result.state;const auto&l=*s.layout;std::vector<uint8_t>visited(s.alive.size());for(uint32_t start=0;start<s.alive.size();++start){if(!s.alive[start]||s.cellHealth[start]<=0||visited[start])continue;result.components.emplace_back();auto&group=result.components.back();group.push_back(start);visited[start]=1;for(size_t n=0;n<group.size();++n)for(uint32_t edge:l.cells[group[n]].bonds){storm_geometry_work::checkpoint();if(s.bondHealth[edge]<=0)continue;const auto&bond=l.bonds[edge];const uint32_t next=bond.a==group[n]?bond.b:bond.a;if(s.alive[next]&&s.cellHealth[next]>0&&!visited[next]){visited[next]=1;group.push_back(next);}}}
    const auto weight=[&](const std::vector<uint32_t>&group){double total=0;for(uint32_t id:group)total+=l.cells[id].volume*s.cellHealth[id];return total;};std::stable_sort(result.components.begin(),result.components.end(),[&](const auto&a,const auto&b){return weight(a)>weight(b);});
    std::vector<uint8_t>retainedOwner(l.cells.size());for(const auto&component:result.components){const uint32_t owner=l.cells[component[0]].owner;if(!retainedOwner[owner]){retainedOwner[owner]=1;result.retainedCells.insert(result.retainedCells.end(),component.begin(),component.end());}else for(uint32_t id:component){s.alive[id]=0;result.releasedCells.push_back(id);}}if(!result.releasedCells.empty())result.changed=true;
}
}
template<class Surface> std::shared_ptr<const Layout>build(const Surface&surface,const std::map<int,float>&wood){
    auto layout=std::make_shared<Layout>();layout->contact=detail::contactMaterial(surface,wood);bool first=true;for(const auto&t:layout->contact->tetrahedra){storm_geometry_work::checkpoint();const auto b=detail::tetraBounds(t);layout->bounds=first?b:storm_ship_hull::merge(layout->bounds,b);first=false;}if(first)return layout;
    const auto extent=layout->bounds.hi-layout->bounds.lo;float shortest=std::numeric_limits<float>::infinity();for(unsigned a=0;a<3;++a)if(coordinate(extent,a)>.001f)shortest=std::min(shortest,coordinate(extent,a));if(!std::isfinite(shortest))return layout;const float pitch=std::max(.1f,shortest/5.f);
    for(unsigned a=0;a<3;++a)layout->divisions[a]=uint32_t(std::clamp(int(std::ceil(coordinate(extent,a)/pitch)),1,64));while(uint64_t(layout->divisions[0])*layout->divisions[1]*layout->divisions[2]>8192){const auto largest=std::max_element(layout->divisions.begin(),layout->divisions.end());*largest=std::max(1u,*largest-1);}
    layout->step={extent.x/layout->divisions[0],extent.y/layout->divisions[1],extent.z/layout->divisions[2]};layout->gridToCell.assign(size_t(layout->divisions[0])*layout->divisions[1]*layout->divisions[2],-1);auto measured=detail::measure(*layout,*layout->contact);
    for(uint32_t z=0;z<layout->divisions[2];++z)for(uint32_t y=0;y<layout->divisions[1];++y)for(uint32_t x=0;x<layout->divisions[0];++x){const uint32_t grid=gridIndex(*layout,x,y,z);if(measured.volume[grid]<=1.e-10)continue;layout->gridToCell[grid]=int32_t(layout->cells.size());Cell cell;cell.bounds=gridBounds(*layout,x,y,z);cell.area=float(measured.area[grid]);cell.volume=float(measured.volume[grid]);cell.centroid=measured.weighted[grid]*(1.f/cell.volume);layout->cells.push_back(cell);layout->area+=cell.area;layout->volume+=cell.volume;}
    for(const auto&entry:measured.sections){const auto&section=entry.second;const int32_t a=layout->gridToCell[entry.first.first],b=layout->gridToCell[entry.first.second];const float area=float(std::min(section.low,section.high));if(a<0||b<0||area<=1.e-8f)continue;const auto&ca=layout->cells[a];const auto&cb=layout->cells[b];unsigned axis=0;for(unsigned n=0;n<3;++n)if(std::abs(coordinate(ca.bounds.hi,n)-coordinate(cb.bounds.lo,n))<1.e-5f&&coordinate(layout->step,n)>0){axis=n;break;}Bond bond;bond.a=uint32_t(a);bond.b=uint32_t(b);bond.axis=uint8_t(axis);bond.coordinate=coordinate(ca.bounds.hi,axis);bond.section=area;bond.capacity=area*std::max(.01f,coordinate(layout->step,axis));bond.bounds=section.bounds;const uint32_t id=uint32_t(layout->bonds.size());layout->cells[a].bonds.push_back(id);layout->cells[b].bonds.push_back(id);layout->bonds.push_back(bond);}
    for(auto&cell:layout->cells)for(uint32_t edge:cell.bonds)cell.reserveLoad+=layout->bonds[edge].capacity*.2f;
    std::vector<uint8_t>visited(layout->cells.size());uint32_t owner=0;for(uint32_t start=0;start<layout->cells.size();++start){if(visited[start])continue;std::vector<uint32_t>group{start};visited[start]=1;for(size_t n=0;n<group.size();++n){auto&cell=layout->cells[group[n]];cell.owner=owner;for(uint32_t edge:cell.bonds){const auto&bond=layout->bonds[edge];const uint32_t next=bond.a==group[n]?bond.b:bond.a;if(!visited[next]){visited[next]=1;group.push_back(next);}}}++owner;}
    uint64_t hash=1469598103934665603ull;const auto add=[&](uint64_t value){for(unsigned n=0;n<8;++n){hash^=uint8_t(value>>(8*n));hash*=1099511628211ull;}};for(unsigned a=0;a<3;++a){add(layout->divisions[a]);add(uint64_t(int64_t(std::llround(coordinate(layout->bounds.lo,a)*10000.))));add(uint64_t(int64_t(std::llround(coordinate(layout->bounds.hi,a)*10000.))));}for(const auto&cell:layout->cells){add(uint64_t(std::llround(cell.volume*1.e8)));add(uint64_t(std::llround(cell.area*1.e8)));}for(const auto&bond:layout->bonds){add(bond.a);add(bond.b);add(uint64_t(std::llround(bond.section*1.e8)));}layout->fingerprint=hash;return layout;
}
inline State intact(std::shared_ptr<const Layout>layout){State result;result.layout=std::move(layout);if(result.layout){result.cellHealth.assign(result.layout->cells.size(),1);result.bondHealth.assign(result.layout->bonds.size(),1);result.alive.assign(result.layout->cells.size(),1);result.contact=result.layout->contact;result.physicalCellHealth=result.cellHealth;result.physicalBondHealth=result.bondHealth;}return result;}
template<class Surface>void refreshContacts(State&state,const Surface&current,const std::map<int,float>&wood){
    if(!state.layout)return;state.contact=detail::contactMaterial(current,wood);
}
template<class Surface> State seed(std::shared_ptr<const Layout>layout,const Surface&current,const std::map<int,float>&wood){
    auto state=intact(std::move(layout));if(!state.layout||state.layout->cells.empty())return state;const auto&l=*state.layout;state.contact=detail::contactMaterial(current,wood);auto measured=detail::measure(l,*state.contact);
    for(uint32_t grid=0;grid<l.gridToCell.size();++grid)if(l.gridToCell[grid]>=0){const uint32_t id=uint32_t(l.gridToCell[grid]);state.cellHealth[id]=float(std::clamp(measured.volume[grid]/l.cells[id].volume,0.,1.));state.alive[id]=state.cellHealth[id]>1.e-6f;}
    // A previously cut seam cannot recover strength merely because both coarse
    // zones still contain plenty of untouched planking.
    std::map<std::pair<uint32_t,uint32_t>,double>sections;for(const auto&entry:measured.sections){const int32_t a=l.gridToCell[entry.first.first],b=l.gridToCell[entry.first.second];if(a>=0&&b>=0)sections[{uint32_t(a),uint32_t(b)}]+=std::min(entry.second.low,entry.second.high);}
    for(size_t n=0;n<l.bonds.size();++n){const auto&b=l.bonds[n];state.bondHealth[n]=float(std::clamp(sections[{b.a,b.b}]/b.section,0.,1.));if(!state.alive[b.a]||!state.alive[b.b])state.bondHealth[n]=0;}state.physicalCellHealth=state.cellHealth;state.physicalBondHealth=state.bondHealth;return state;
}
inline bool validState(const State&state){return detail::validState(state);}
template<class Surface>State repair(const State&before,const Surface&oldMaterial,const Surface&newMaterial,const std::map<int,float>&wood,double remainingHealthQuotaArea){
    if(!validState(before))return before;const auto previous=seed(before.layout,oldMaterial,wood),physical=seed(before.layout,newMaterial,wood);State after=before;after.contact=physical.contact;after.physicalCellHealth=physical.physicalCellHealth;after.physicalBondHealth=physical.physicalBondHealth;const auto&l=*before.layout;std::vector<float>restored(l.cells.size());
    for(size_t n=0;n<l.cells.size();++n){const float added=std::max(0.f,physical.cellHealth[n]-previous.cellHealth[n]);if(added>0){const float old=before.alive[n]?before.cellHealth[n]:0.f;after.cellHealth[n]=std::min(physical.cellHealth[n],old+added);after.alive[n]=after.cellHealth[n]>1.e-6f;restored[n]=std::max(0.f,after.cellHealth[n]-old);}}
    // Actual material restoration already paid for its added outer area. Only
    // the caller's unused quota may restore latent damage in existing timber.
    double quota=std::max(0.,remainingHealthQuotaArea);for(size_t n=0;n<l.cells.size()&&quota>0;++n){if(!after.alive[n]||l.cells[n].area<=0)continue;const float gain=float(std::min(double(std::max(0.f,physical.cellHealth[n]-after.cellHealth[n])),quota/l.cells[n].area));after.cellHealth[n]+=gain;restored[n]+=gain;quota-=gain*l.cells[n].area;}
    for(size_t n=0;n<l.bonds.size();++n){const auto&bond=l.bonds[n];if(!after.alive[bond.a]||!after.alive[bond.b])continue;const float geometric=std::max(0.f,physical.bondHealth[n]-previous.bondHealth[n]),support=std::max(restored[bond.a],restored[bond.b]);if(geometric>0||support>0)after.bondHealth[n]=std::min(physical.bondHealth[n],before.bondHealth[n]+std::max(geometric,support));}return after;
}
// Actual thermal ablation has already spent the weapon's work and changed the
// material. Reconcile just its sparse capacity delta; it must not issue another
// whole-zone subtraction or spend the same finite volume as hidden HP damage.
// alreadyPaid refreshes physical sections and caps paid health instead of
// debiting it; its caller finalizes overload/islands once after this refresh.
template<class Surface>DamageResult reconcile(const State&before,const Surface&oldMaterial,const Surface&newMaterial,const std::map<int,float>&wood,const std::vector<uint32_t>*changedIDs=nullptr,bool alreadyPaid=false){
    DamageResult result;result.state=before;if(!validState(before)||!oldMaterial.cells||!newMaterial.cells||oldMaterial.cells->source!=newMaterial.cells->source)return result;
    std::vector<uint32_t>ids;if(changedIDs)ids=*changedIDs;else{for(const auto&entry:oldMaterial.cells->changed)ids.push_back(entry.first);for(const auto&entry:newMaterial.cells->changed)ids.push_back(entry.first);}std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
    const auto&source=*oldMaterial.cells->source;ContactMaterial oldLocal,newLocal;std::map<uint32_t,std::shared_ptr<const ContactMaterial>>overrides;
    for(uint32_t id:ids){storm_geometry_work::checkpoint();if(id>=source.solids.size())continue;const auto oldChanged=oldMaterial.cells->changed.find(id),newChanged=newMaterial.cells->changed.find(id);const auto&oldSolid=oldChanged==oldMaterial.cells->changed.end()?source.solids[id]:oldChanged->second;const auto&newSolid=newChanged==newMaterial.cells->changed.end()?source.solids[id]:newChanged->second;if(oldSolid==newSolid)continue;detail::addSource(oldLocal,id,*oldSolid,wood);detail::addSource(newLocal,id,*newSolid,wood);auto local=std::make_shared<ContactMaterial>();detail::addSource(*local,id,*newSolid,wood);detail::indexMaterial(*local);overrides[id]=std::move(local);}
    if(overrides.empty())return result;auto&s=result.state;const auto&l=*s.layout;const auto oldMeasure=detail::measure(l,oldLocal),newMeasure=detail::measure(l,newLocal);
    // These arrays are derived from physical material on load, not saved state.
    // Older in-memory callers pay a single initialization; ordinary marks only
    // measure changed authored source solids and visit the small coarse graph.
    if(s.physicalCellHealth.size()!=l.cells.size()||s.physicalBondHealth.size()!=l.bonds.size()){const auto physical=seed(before.layout,oldMaterial,wood);s.physicalCellHealth=physical.physicalCellHealth;s.physicalBondHealth=physical.physicalBondHealth;}
    for(uint32_t grid=0;grid<l.gridToCell.size();++grid){const int32_t index=l.gridToCell[grid];if(index<0)continue;const uint32_t id=uint32_t(index);const double loss=std::max(0.,oldMeasure.volume[grid]-newMeasure.volume[grid]);if(loss<=std::max(1.e-10,double(l.cells[id].volume)*1.e-6))continue;const float fraction=float(loss/l.cells[id].volume);s.physicalCellHealth[id]=std::max(0.f,s.physicalCellHealth[id]-fraction);s.cellHealth[id]=alreadyPaid?std::min(s.cellHealth[id],s.physicalCellHealth[id]):std::max(0.f,s.cellHealth[id]-fraction);if(s.physicalCellHealth[id]<=1.e-6f){s.physicalCellHealth[id]=0;s.cellHealth[id]=0;s.alive[id]=0;}if(s.cellHealth[id]<=1.e-6f){s.cellHealth[id]=0;for(uint32_t edge:l.cells[id].bonds)if(s.bondHealth[edge]>0){s.bondHealth[edge]=0;result.brokenBonds.push_back(edge);}}}
    std::map<std::pair<uint32_t,uint32_t>,double>lostSections;
    for(const auto&entry:oldMeasure.sections){const auto current=newMeasure.sections.find(entry.first);const detail::Section empty;const auto&section=current==newMeasure.sections.end()?empty:current->second;const double loss=std::max({0.,entry.second.low-section.low,entry.second.high-section.high});const int32_t a=l.gridToCell[entry.first.first],b=l.gridToCell[entry.first.second];if(a>=0&&b>=0&&loss>0)lostSections[{uint32_t(a),uint32_t(b)}]+=loss;}
    for(uint32_t edge=0;edge<l.bonds.size();++edge){const auto&bond=l.bonds[edge];const auto lost=lostSections.find({bond.a,bond.b});if(lost==lostSections.end()||lost->second<=std::max(1.e-10,double(bond.section)*1.e-6))continue;const float fraction=float(lost->second/bond.section);s.physicalBondHealth[edge]=std::max(0.f,s.physicalBondHealth[edge]-fraction);const float old=s.bondHealth[edge];s.bondHealth[edge]=alreadyPaid?std::min(old,s.physicalBondHealth[edge]):std::max(0.f,old-fraction);if(s.bondHealth[edge]<=1.e-6f){s.bondHealth[edge]=0;if(old>0)result.brokenBonds.push_back(edge);}}
    // A removal remains inside its authored source bounds, so the immutable
    // base BVH also indexes sparse replacement cells. No whole-hull rebuild.
    auto contact=before.contact?before.contact:l.contact;if(contact){const auto&base=contact->base?*contact->base:*contact;if(base.sourceIDs.size()!=base.tetrahedra.size())contact=detail::contactMaterial(oldMaterial,wood);auto next=std::make_shared<ContactMaterial>();next->base=contact->base?contact->base:contact;next->changed=contact->changed;for(auto&entry:overrides)next->changed[entry.first]=std::move(entry.second);s.contact=std::move(next);}else s.contact=detail::contactMaterial(newMaterial,wood);
    result.changed=true;if(!alreadyPaid){detail::overload(result);std::sort(result.brokenBonds.begin(),result.brokenBonds.end());result.brokenBonds.erase(std::unique(result.brokenBonds.begin(),result.brokenBonds.end()),result.brokenBonds.end());detail::islands(result);}return result;
}
namespace detail {
template<class Plane> DamageResult damagePatches(const State&before,const Plane&plane,double wholeArea=0,bool finalize=true,const std::vector<std::vector<Contact>>*preparedContacts=nullptr){
    DamageResult result;result.state=before;if(!detail::validState(before)||plane.beams.empty())return result;auto&s=result.state;const auto&l=*s.layout;const auto contactMaterial=s.contact?s.contact:l.contact;if(!contactMaterial)return result;
    for(uint32_t beamIndex=0;beamIndex<plane.beams.size();++beamIndex){const auto&beam=plane.beams[beamIndex];storm_geometry_work::checkpoint();if(!storm_hull_damage::validCrossSection(beam)||beam.depth<=0||beam.radius<=0)continue;const bool mechanical=beam.contactStrength>0||beam.specificWork>0;if(mechanical&&beam.specificWork<=0)continue;double remaining=mechanical?double(beam.specificWork)/beam.contactStrength*(wholeArea>0?wholeArea:l.area)*l.volume/std::max(1.e-12,l.area):beamVolume(beam);if(!std::isfinite(remaining)||remaining<=0)continue;const double initial=remaining;
        const auto found=preparedContacts?preparedContacts->at(beamIndex):contacts(l,*contactMaterial,beam);
        for(const auto&contact:found){const uint32_t id=contact.cell;if(!s.alive[id]||s.cellHealth[id]<=0)continue;const double capacity=double(l.cells[id].volume)*s.cellHealth[id];std::vector<uint32_t>rails;double railCapacity=0;for(uint32_t edge:l.cells[id].bonds){const auto&b=l.bonds[edge];const uint32_t neighbor=b.a==id?b.b:b.a;if(s.bondHealth[edge]>0&&s.alive[neighbor]){rails.push_back(edge);railCapacity+=b.capacity*s.bondHealth[edge];}}const double local=mechanical?capacity+railCapacity:std::min(capacity,contact.volume);const double dose=std::min(remaining,local);if(dose<=1.e-12)continue;remaining-=dose;
            // Spend this work once: local crushing/ablation and the load carried
            // by existing rails share one budget. Fewer surviving rails receive
            // a larger share, and an overloaded rail passes only its remainder.
            const double nodeWork=std::min(capacity,rails.empty()?dose:dose*.65),structural=dose-nodeWork;const float old=s.cellHealth[id];s.cellHealth[id]=float(std::max(0.,double(old)-nodeWork/l.cells[id].volume));if(s.cellHealth[id]!=old)result.changed=true;
            double load=structural;for(unsigned pass=0;pass<4&&load>1.e-12&&!rails.empty();++pass){double stiffness=0;for(uint32_t edge:rails)stiffness+=l.bonds[edge].section; if(stiffness<=1.e-12)break;double unspent=0;std::vector<uint32_t>survivors;for(uint32_t edge:rails){const double capacity=l.bonds[edge].capacity*s.bondHealth[edge],share=load*l.bonds[edge].section/stiffness,taken=std::min(share,capacity);const float oldBond=s.bondHealth[edge];s.bondHealth[edge]=float(std::max(0.,double(oldBond)-taken/l.bonds[edge].capacity));if(s.bondHealth[edge]!=oldBond)result.changed=true;if(s.bondHealth[edge]<=1.e-6f){s.bondHealth[edge]=0;result.brokenBonds.push_back(edge);}else survivors.push_back(edge);unspent+=share-taken;}load=unspent;rails.swap(survivors);}
            const double crush=std::min(load,double(l.cells[id].volume)*s.cellHealth[id]);if(crush>0){s.cellHealth[id]=float(std::max(0.,double(s.cellHealth[id])-crush/l.cells[id].volume));load-=crush;result.changed=true;}remaining+=load;
            // Physical crushing spends only this already-paid local node work.
            // Surplus work weakens structure; it cannot ablate wood outside
            // the actual finite contacted material.
            if(mechanical&&nodeWork+crush>1.e-12)result.crushed.push_back({beamIndex,id,std::min(contact.volume,nodeWork+crush)});
            if(s.cellHealth[id]<=1.e-6f){s.cellHealth[id]=0;s.alive[id]=0;result.brokenCells.push_back(id);for(uint32_t edge:l.cells[id].bonds)if(s.bondHealth[edge]>0){s.bondHealth[edge]=0;result.brokenBonds.push_back(edge);}}
            if(remaining<=1.e-12)break;
        }result.spentWork+=initial-remaining;result.remainingWork+=remaining;
    }
    if(finalize){if(result.changed)overload(result);std::sort(result.brokenBonds.begin(),result.brokenBonds.end());result.brokenBonds.erase(std::unique(result.brokenBonds.begin(),result.brokenBonds.end()),result.brokenBonds.end());if(result.changed)islands(result);}return result;
}
}
inline void finalizeDamage(DamageResult&result){if(result.changed)detail::overload(result);std::sort(result.brokenBonds.begin(),result.brokenBonds.end());result.brokenBonds.erase(std::unique(result.brokenBonds.begin(),result.brokenBonds.end()),result.brokenBonds.end());if(result.changed)detail::islands(result);}
template<class Plane> DamageResult damage(const State&state,const Plane&plane,double wholeArea=0,bool finalize=true){return detail::damagePatches(state,plane,wholeArea,finalize);}
// Compatibility call for the material owner: the geometry cache is refreshed
// at its writer, never rebuilt for every read of an unchanged surface.
template<class Plane,class Surface> DamageResult damage(const State&state,const Plane&plane,const Surface&,const std::map<int,float>&,double wholeArea=0,bool finalize=true){return damage(state,plane,wholeArea,finalize);}
}

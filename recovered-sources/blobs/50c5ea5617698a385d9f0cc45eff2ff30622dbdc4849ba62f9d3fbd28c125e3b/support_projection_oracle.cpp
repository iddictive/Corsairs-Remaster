#include "ship_hull_contact_geometry.hpp"
#include <sys/resource.h>
#include <ctime>
#include "ship_wood_surface_profiles.hpp"
#include "rdf.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <cassert>
#include <iomanip>
using namespace storm_ship_laser;
template<class T>std::vector<T>read(std::ifstream&f,size_t n){std::vector<T>v(n);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));assert(f);return v;}
size_t count(const Mesh&m){size_t n=0;for(const auto&o:m)n+=o.second.size();return n;}
namespace storm::ship::material::convex {
inline std::shared_ptr<const MaterialCells>referenceMaterialize(const ContactSnapshot&snapshot,Mesh&caps){
    if(!snapshot.cells||!compatible(*snapshot.cells))return {};if(!snapshot.retainedSupports)return ready(snapshot.cells);const auto&before=*snapshot.cells->convex;Component removed;
    for(uint32_t id=0;id<before.cells.size();++id){const auto&cell=before.cells[id];if(!cell||!cell->pieces)continue;for(uint32_t p=0;p<cell->pieces->size();++p){const auto support=cell->pieces->at(p).supportCell;if(support<snapshot.retainedSupports->size()&&!snapshot.retainedSupports->at(support))removed.pieces.push_back({id,p});}}
    if(removed.pieces.empty())return ready(snapshot.cells);Budget budget;std::vector<uint32_t>changed;auto cells=take(snapshot.cells,{removed},budget,&changed);if(!cells)return {};
    std::set<uint32_t>affected(changed.begin(),changed.end());std::vector<uint32_t>nearby;const Vec3 pad{.000002f,.000002f,.000002f};for(auto id:changed){auto box=cells->source->bounds[id];box.lo=box.lo-pad;box.hi=box.hi+pad;cells->source->index.queryBounds(box,nearby);affected.insert(nearby.begin(),nearby.end());}
    for(auto id:affected){const auto&cell=cells->convex->cells[id];if(!cell||!cell->pieces)continue;for(uint32_t p=0;p<cell->pieces->size();++p)for(const auto&face:cell->pieces->at(p).faces)if(face.partition){storm_geometry_work::checkpoint();const auto visible=visibleFace(*cells->convex,{id,p},face);if(visible.empty())continue;
            // Keys of untouched sources remain stable. In a changed source,
            // locate the same retained positive piece by support and vertices.
            const auto&old=*before.cells[id]->pieces;uint32_t prior=UINT32_MAX,surviving=0;for(uint32_t n=0;n<old.size();++n){const auto support=old[n].supportCell;if(support<snapshot.retainedSupports->size()&&!snapshot.retainedSupports->at(support))continue;if(surviving++==p){prior=n;break;}}
            if(prior==UINT32_MAX)return {};const auto previous=visibleFace(before,{id,prior},face);for(const auto&polygon:visible){std::vector<Triangle>triangles;storm_ship_hull::fan(polygon,triangles);for(const auto&t:triangles){std::vector<Triangle>fresh;if(previous.empty())fresh.push_back(t);else if(!subtractFaces(t,previous,fresh))return {};auto&out=caps[face.object];out.insert(out.end(),fresh.begin(),fresh.end());}}}}
    return cells;
}
inline bool referencePartitionSurface(const Surface&source,const MaterialCells&retained,const std::vector<Body>&bodies,const std::map<int,float>&wood,std::vector<Surface>&out,std::set<int>*changedObjects=nullptr){
    if(!compatible(retained))return false;std::vector<Surface>groups(bodies.size()+1);struct Ref{uint32_t body,piece;};std::vector<Ref>refs;std::vector<Bounds>boxes;
    for(uint32_t n=0;n<bodies.size();++n)for(uint32_t p=0;p<bodies[n].pieces.size();++p){refs.push_back({n,p});boxes.push_back(bodies[n].pieces[p].bounds);}storm_ship_hull::Broadphase index;index.build(boxes);std::vector<uint32_t>nearby;Surface decoration;
    const auto touchingTriangle=[&](const Triangle&t,const Piece&piece){for(const auto&face:piece.faces){std::vector<Triangle>triangles;storm_ship_hull::fan(face.vertices,triangles);for(const auto&other:triangles)if(supportTrianglesTouch(t,other))return true;}return false;};
    for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(source,role)){surfaceRole(groups[0],role).try_emplace(o.first);if(!wood.count(o.first))continue;for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<std::vector<Vertex>>remaining{{t.begin(),t.end()}};index.queryBounds(storm_ship_structure::detail::triangleBounds(t),nearby);if(changedObjects&&!nearby.empty())changedObjects->insert(o.first);
            for(auto id:nearby){const auto ref=refs[id];const auto&piece=bodies[ref.body].pieces[ref.piece];std::vector<std::vector<Vertex>>outside;for(auto polygon:remaining){for(const auto&face:piece.faces){std::vector<Vertex>in,rejected;linearSplit(polygon,face.plane,in,rejected);if(rejected.size()>=3&&polygonArea(rejected)>1.e-12f)outside.push_back(std::move(rejected));polygon=std::move(in);if(polygon.size()<3)break;}if(polygon.size()>=3)storm_ship_hull::fan(polygon,surfaceRole(groups[ref.body+1],role)[o.first]);}remaining=std::move(outside);if(remaining.empty())break;}
            for(const auto&polygon:remaining)storm_ship_hull::fan(polygon,surfaceRole(groups[0],role)[o.first]);}}
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
int main(int argc,char**argv){assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);const auto h=read<RDF_HEAD>(f,1)[0];assert(h.version==RDF_VERSION);const auto names=read<char>(f,h.name_size);read<int32_t>(f,h.names);const auto textures=read<int32_t>(f,h.ntextures);const auto materials=read<RDF_MATERIAL>(f,h.nmaterials);read<RDF_LIGHT>(f,h.nlights);read<RDF_LABEL>(f,h.nlabels);const auto objects=read<RDF_OBJECT>(f,h.nobjects);const auto triangles=read<RDF_TRIANGLE>(f,h.ntriangles);const auto buffers=read<RDF_VERTEXBUFF>(f,h.nvrtbuffs);std::vector<std::vector<uint8_t>>vertices;for(const auto&buffer:buffers)vertices.push_back(read<uint8_t>(f,buffer.size));Mesh original;std::map<int,float>wood;
 std::cout<<std::setprecision(12)<<"GM objects="<<h.nobjects<<" triangles="<<h.ntriangles<<" bboxCenter="<<h.bbox_center.x<<","<<h.bbox_center.y<<","<<h.bbox_center.z<<" bboxSize="<<h.bbox_size.x<<","<<h.bbox_size.y<<","<<h.bbox_size.z<<"\n";
 for(size_t o=0;o<objects.size();++o){const auto&ob=objects[o];auto&out=original[o];const auto&buffer=buffers.at(ob.vertex_buff);unsigned stride=36+(buffer.type&3)*8+(buffer.type>>2)*8;assert(!(buffer.type>>2));for(int n=0;n<ob.ntriangles;++n){Triangle t;for(unsigned j=0;j<3;++j)t[j]=storm_ship_hull::decode(vertices[ob.vertex_buff].data()+size_t(ob.svertex+triangles[ob.striangle+n].vindex[j])*stride,stride);out.push_back(t);}const auto&mat=materials.at(ob.material);const char*texture="";if(mat.texture_type[0]==RDF_TEXTURE_TYPE::TEXTURE_BASE){texture=names.data()+textures.at(mat.texture[0]);const float thickness=storm_ship_wood::thickness("ships/Flyingdutchman1/flyingdutchman1.gm",texture);if(thickness>0)wood[o]=thickness;}std::cout<<"OBJECT "<<o<<" faces="<<out.size()<<" texture="<<texture<<" wood="<<(wood.count(o)?wood.at(o):0)<<"\n";}

 const auto wall=[](auto start){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
 const auto allStart=std::chrono::steady_clock::now();const auto cpuStart=std::clock();auto authored=timberSurface(original,wood);Surface current=storm::ship::material::rebase(authored);current.supportOriginal=storm::ship::material::supportSource(original);auto cells=std::make_shared<MaterialCells>();cells->source=storm::ship::material::cellSource(authored,wood);current.cells=cells;if(!storm::ship::material::trimReleasedCells(current))return 2;current=storm::ship::material::rebase(std::move(current));
 auto layout=storm_ship_structure::build(authored,wood);current.structure=std::make_shared<const storm_ship_structure::State>(storm_ship_structure::seed(layout,current,wood));std::printf("BOOTSTRAP_SEED wall_ms=%.3f cells=%zu graph=%zu\n",wall(allStart),cells->source->solids.size(),layout->cells.size());std::fflush(stdout);
 const auto cacheStart=std::chrono::steady_clock::now();if(!storm::ship::material::preparePhysicalCache(current))return 3;std::printf("BOOTSTRAP_CACHE wall_ms=%.3f\n",wall(cacheStart));std::fflush(stdout);

 using namespace storm::ship::material::convex;
 storm::ship::material::convex::prepareDecorations(current,wood);
 for(int mode:{0,1,2}){
  auto mask=std::make_shared<std::vector<uint8_t>>(layout->cells.size(),1);Body body;body.reference=current.cells->convex;
  for(uint32_t id=0;id<layout->cells.size();++id){const auto&cell=layout->cells[id];bool remove=mode==0?cell.centroid.x>0:mode==1?id%7==0:cell.bounds.lo.z>0;
   if(remove){(*mask)[id]=0;body.supports.push_back(id);const auto&ids=current.cells->convex->source->supportSources[id];body.sources.insert(body.sources.end(),ids.begin(),ids.end());}}
  std::sort(body.sources.begin(),body.sources.end());body.sources.erase(std::unique(body.sources.begin(),body.sources.end()),body.sources.end());measureBody(body);auto expanded=expandBody(body);
  ContactSnapshot snapshot{current.cells,current.structure,1,mask};Mesh fastCaps,oldCaps;auto start=std::chrono::steady_clock::now();std::shared_ptr<const MaterialCells>fast;{storm_geometry_work::Scope scope(storm_geometry_work::allowance());fast=materialize(snapshot,fastCaps);}const auto fastMs=wall(start);start=std::chrono::steady_clock::now();auto reference=referenceMaterialize(snapshot,oldCaps);const auto oldMs=wall(start);
  if(!fast||!reference)return 20;const auto fastHash=storm::ship::material::fingerprint(fastCaps,{}),oldHash=storm::ship::material::fingerprint(oldCaps,{});std::printf("CAP_ORACLE mode=%d faces=%zu/%zu hash_equal=%d fast_ms=%.3f old_ms=%.3f\n",mode,count(fastCaps),count(oldCaps),fastHash==oldHash,fastMs,oldMs);std::fflush(stdout);if(fastHash!=oldHash)return 21;
  std::vector<Surface>x,y;std::set<int>changed,oldChanged;start=std::chrono::steady_clock::now();{storm_geometry_work::Scope scope(storm_geometry_work::allowance());if(!partitionSurface(current,*fast,{expanded},wood,x,&changed))return 22;}const auto splitMs=wall(start);start=std::chrono::steady_clock::now();if(!referencePartitionSurface(current,*reference,{expanded},wood,y,&oldChanged))return 23;const auto referenceSplitMs=wall(start);
  if(x.size()!=y.size()||changed!=oldChanged)return 25;
  for(size_t group=0;group<x.size();++group)for(unsigned role=0;role<4;++role)
    if(storm::ship::material::fingerprint(surfaceRole(x[group],role),{})!=storm::ship::material::fingerprint(surfaceRole(y[group],role),{}))return 26;
  double areaError=0,momentError=0,uvError=0;size_t fastFaces=0,oldFaces=0;
  const auto metrics=[](const std::vector<Triangle>&faces){std::array<double,7>a{};for(const auto&t:faces){const double w=storm_ship_hull::area(t);a[0]+=w;for(const auto&v:t){a[1]+=w*v.position.x/3;a[2]+=w*v.position.y/3;a[3]+=w*v.position.z/3;a[4]+=w*v.uv[0]/3;a[5]+=w*v.uv[1]/3;}a[6]+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6.;}return a;};
  for(unsigned group=0;group<x.size();++group)for(unsigned role=0;role<4;++role){const auto&a=surfaceRole(x[group],role),&c=surfaceRole(y[group],role);std::set<int>objects;for(const auto&o:a)objects.insert(o.first);for(const auto&o:c)objects.insert(o.first);for(auto object:objects){const std::vector<Triangle>empty;const auto&fa=a.count(object)?a.at(object):empty;const auto&fc=c.count(object)?c.at(object):empty;fastFaces+=fa.size();oldFaces+=fc.size();const auto am=metrics(fa),cm=metrics(fc);const double ae=std::abs(am[0]-cm[0]);areaError=std::max(areaError,ae);for(unsigned n=1;n<4;++n)momentError=std::max(momentError,std::abs(am[n]-cm[n]));for(unsigned n=4;n<6;++n)uvError=std::max(uvError,std::abs(am[n]-cm[n]));
   if(ae>std::max(.0001,cm[0]*.00001)){std::printf("AREA_MISMATCH group=%u role=%u object=%d area=%.9f/%.9f\n",group,role,object,am[0],cm[0]);return 24;}
  }}
  std::printf("PARTITION_ORACLE mode=%d faces=%zu/%zu area_error=%.9f moment_error=%.9f uv_error=%.9f fast_ms=%.3f old_ms=%.3f\n",mode,fastFaces,oldFaces,areaError,momentError,uvError,splitMs,referenceSplitMs);std::fflush(stdout);
  if(mode==2){auto cancel=std::make_shared<std::atomic<bool>>(true);storm_geometry_work::Scope scope(storm_geometry_work::allowance(),cancel);std::vector<Surface>rejected;
   bool failed=false;try{failed=!partitionSurface(current,*fast,{expanded},wood,rejected);}catch(const storm_geometry_work::Cancelled&){failed=true;}
   if(!failed||!rejected.empty())return 27;
   if(storm_geometry_work::parallelFor(64,[](size_t){storm_geometry_work::checkpoint(true);return true;}))return 28;
   std::puts("CANCELLATION joined=1 output_unpublished=1");}
 }
 return 0;
}

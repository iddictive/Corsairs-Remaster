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
inline std::shared_ptr<const MaterialCells>referenceTake(std::shared_ptr<const MaterialCells>original,const std::vector<Component>&groups,Budget&budget,std::vector<uint32_t>*changedOutput=nullptr){if(!original||!compatible(*original))return {};std::map<uint32_t,std::set<uint32_t>>removed;for(const auto&group:groups)for(auto key:group.pieces)removed[key.cell].insert(key.piece);auto cells=std::make_shared<MaterialCells>(*original);auto state=std::make_shared<State>(*original->convex);std::set<uint32_t>changed;
    for(const auto&entry:removed){if(entry.first>=state->cells.size()||!state->cells[entry.first])return {};auto cell=std::make_shared<Cell>(*state->cells[entry.first]);std::vector<Piece>pieces;for(uint32_t n=0;n<cell->pieces->size();++n)if(!entry.second.count(n))pieces.push_back(cell->pieces->at(n));cell->pieces=std::make_shared<const std::vector<Piece>>(std::move(pieces));cell->boundary=std::make_shared<const Mesh>(mesh(*cell->pieces));cells->changed[entry.first]=cell->boundary;state->cells[entry.first]=std::move(cell);changed.insert(entry.first);}
    if(!reconnect(*state,changed,budget))return {};state->owner=cells.get();cells->convex=std::move(state);if(changedOutput)changedOutput->assign(changed.begin(),changed.end());return cells;
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
 const auto check=[&](std::shared_ptr<const MaterialCells>input,int mode){Component removed;bool first=true;
   for(uint32_t id=0;id<input->convex->cells.size();++id){const auto&cell=input->convex->cells[id];if(!cell||!cell->pieces)continue;for(uint32_t n=0;n<cell->pieces->size();++n){const auto&p=cell->pieces->at(n);const bool select=mode==1?first:mode==2?p.supportCell%7==0:mode==3?id%19==0&&n==0:mode==4?id%37==0:false;if(select){removed.pieces.push_back({id,n});if(mode==1)first=false;}}}
   const std::vector<Component>groups=removed.pieces.empty()?std::vector<Component>{}:std::vector<Component>{removed};Budget fastBudget,oracleBudget;
   auto start=std::chrono::steady_clock::now();auto fast=take(input,groups,fastBudget);const auto fastMs=wall(start);start=std::chrono::steady_clock::now();auto oracle=referenceTake(input,groups,oracleBudget);const auto oracleMs=wall(start);
   if(!fast||!oracle||fast->convex->cells.size()!=oracle->convex->cells.size())std::exit(20);
   for(size_t id=0;id<fast->convex->cells.size();++id){const auto&a=fast->convex->cells[id],&b=oracle->convex->cells[id];if(bool(a)!=bool(b))std::exit(21);if(!a)continue;if(a->pieces->size()!=b->pieces->size()||a->edges.size()!=b->edges.size())std::exit(22);
    for(size_t n=0;n<a->edges.size();++n){auto x=a->edges[n],y=b->edges[n];std::sort(x.begin(),x.end());std::sort(y.begin(),y.end());if(x!=y){std::printf("EDGE_MISMATCH mode=%d cell=%zu piece=%zu owner=%u fast=%zu oracle=%zu\n",mode,id,n,a->pieces->at(n).authoredOwner,x.size(),y.size());for(auto key:x)if(std::find(y.begin(),y.end(),key)==y.end())std::printf("FAST_ONLY %u:%u owner=%u\n",key.cell,key.piece,fast->convex->cells[key.cell]->pieces->at(key.piece).authoredOwner);for(auto key:y)if(std::find(x.begin(),x.end(),key)==x.end())std::printf("ORACLE_ONLY %u:%u owner=%u\n",key.cell,key.piece,oracle->convex->cells[key.cell]->pieces->at(key.piece).authoredOwner);std::fflush(stdout);std::exit(23);}if(std::abs(a->pieces->at(n).volume-b->pieces->at(n).volume)>1.e-12)std::exit(24);}}
   const auto componentsKey=[](const State&state){std::map<std::vector<Key>,double>out;for(auto c:state.components){std::sort(c.pieces.begin(),c.pieces.end());out.emplace(std::move(c.pieces),c.volume);}return out;};
   auto x=componentsKey(*fast->convex),y=componentsKey(*oracle->convex);if(x.size()!=y.size())std::exit(25);for(const auto&e:x){auto found=y.find(e.first);if(found==y.end()||std::abs(e.second-found->second)>1.e-8*std::max(1.,e.second))std::exit(26);}
   for(size_t n=0;n<fast->convex->supportMeasures.size();++n)if(std::abs(fast->convex->supportMeasures[n].volume-oracle->convex->supportMeasures[n].volume)>1.e-10)std::exit(27);
   std::printf("FILTER_ORACLE mode=%d removed=%zu components=%zu fast_ms=%.3f oracle_ms=%.3f graph_equal=1 volume_equal=1\n",mode,removed.pieces.size(),x.size(),fastMs,oracleMs);std::fflush(stdout);
 };
 for(int mode:{0,1,2,3})check(current.cells,mode);
 const Piece*largest=nullptr;for(const auto&cell:current.cells->convex->cells)if(cell&&cell->pieces)for(const auto&p:*cell->pieces)if(!largest||p.volume>largest->volume)largest=&p;
 Cut field;field.origin=center(*largest);const auto radius=(largest->bounds.hi-largest->bounds.lo)*.1f;field.bounds={field.origin-radius,field.origin+radius};for(unsigned axis=0;axis<3;++axis)for(float sign:{-1.f,1.f})field.planes.push_back({storm_ship_structure::axisVector(axis,sign),sign*storm_ship_structure::coordinate(sign<0?field.bounds.lo:field.bounds.hi,axis)});
 auto edited=edit(current.cells,{field},{},false,false);if(!edited.complete||edited.changed.empty()||edited.cells->convex->pendingConnectivity.empty())return 28;check(edited.cells,4);
 return 0;
}

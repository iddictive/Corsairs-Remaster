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
int main(int argc,char**argv){assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);const auto h=read<RDF_HEAD>(f,1)[0];assert(h.version==RDF_VERSION);const auto names=read<char>(f,h.name_size);read<int32_t>(f,h.names);const auto textures=read<int32_t>(f,h.ntextures);const auto materials=read<RDF_MATERIAL>(f,h.nmaterials);read<RDF_LIGHT>(f,h.nlights);read<RDF_LABEL>(f,h.nlabels);const auto objects=read<RDF_OBJECT>(f,h.nobjects);const auto triangles=read<RDF_TRIANGLE>(f,h.ntriangles);const auto buffers=read<RDF_VERTEXBUFF>(f,h.nvrtbuffs);std::vector<std::vector<uint8_t>>vertices;for(const auto&buffer:buffers)vertices.push_back(read<uint8_t>(f,buffer.size));Mesh original;std::map<int,float>wood;
 std::cout<<std::setprecision(12)<<"GM objects="<<h.nobjects<<" triangles="<<h.ntriangles<<" bboxCenter="<<h.bbox_center.x<<","<<h.bbox_center.y<<","<<h.bbox_center.z<<" bboxSize="<<h.bbox_size.x<<","<<h.bbox_size.y<<","<<h.bbox_size.z<<"\n";
 for(size_t o=0;o<objects.size();++o){const auto&ob=objects[o];auto&out=original[o];const auto&buffer=buffers.at(ob.vertex_buff);unsigned stride=36+(buffer.type&3)*8+(buffer.type>>2)*8;assert(!(buffer.type>>2));for(int n=0;n<ob.ntriangles;++n){Triangle t;for(unsigned j=0;j<3;++j)t[j]=storm_ship_hull::decode(vertices[ob.vertex_buff].data()+size_t(ob.svertex+triangles[ob.striangle+n].vindex[j])*stride,stride);out.push_back(t);}const auto&mat=materials.at(ob.material);const char*texture="";if(mat.texture_type[0]==RDF_TEXTURE_TYPE::TEXTURE_BASE){texture=names.data()+textures.at(mat.texture[0]);const float thickness=storm_ship_wood::thickness("ships/Flyingdutchman1/flyingdutchman1.gm",texture);if(thickness>0)wood[o]=thickness;}std::cout<<"OBJECT "<<o<<" faces="<<out.size()<<" texture="<<texture<<" wood="<<(wood.count(o)?wood.at(o):0)<<"\n";}

 const auto wall=[](auto start){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
 const auto allStart=std::chrono::steady_clock::now();const auto cpuStart=std::clock();auto authored=timberSurface(original,wood);Surface current=storm::ship::material::rebase(authored);current.supportOriginal=storm::ship::material::supportSource(original);auto cells=std::make_shared<MaterialCells>();cells->source=storm::ship::material::cellSource(authored,wood);current.cells=cells;if(!storm::ship::material::trimReleasedCells(current))return 2;current=storm::ship::material::rebase(std::move(current));
 auto layout=storm_ship_structure::build(authored,wood);current.structure=std::make_shared<const storm_ship_structure::State>(storm_ship_structure::seed(layout,current,wood));std::printf("BOOTSTRAP_SEED wall_ms=%.3f cells=%zu graph=%zu\n",wall(allStart),cells->source->solids.size(),layout->cells.size());std::fflush(stdout);

 auto sourcePieces=storm::ship::material::convex::buildSource(current.cells->source,current.structure->layout);if(!sourcePieces)return 2;double positive=0,boundary=0;uint64_t digest=1469598103934665603ull;size_t parts=0,vertexCount=0;for(const auto&cell:sourcePieces->pieces)if(cell)for(const auto&p:*cell){++parts;positive+=p.volume;for(const auto&f:p.faces)for(const auto&v:f.vertices){++vertexCount;const auto words=storm::ship::material::vertexWords(v);digest=storm::ship::material::hashBytes(words.data(),sizeof(words),digest);}}for(uint32_t id=0;id<current.cells->source->solids.size();++id)boundary+=closedCutVolume(materialCell(*current.cells,id));std::printf("ORACLES source_boundary_closedCutVolume=%.12f authored_positive_piece_volume=%.12f parts=%zu vertices=%zu geometry_digest=%llu wall_ms=%.3f\n",boundary,positive,parts,vertexCount,(unsigned long long)digest,wall(allStart));
}

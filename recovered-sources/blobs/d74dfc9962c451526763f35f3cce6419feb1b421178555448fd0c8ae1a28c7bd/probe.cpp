#include "ship_laser_geometry.hpp"
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
 Input input;input.surface=std::make_shared<const Surface>(timberSurface(original,wood));input.wood=wood;input.plane={{0,0,1},h.bbox_center.z+h.bbox_size.z*.25f,false};input.fragmentLimit=8;input.enclosedFragments=true;input.closedVolume=closedVolume(canonicalMesh(*input.surface));input.generation=42;input.revision=0;
 std::cout<<"INPUT plane=(0,0,1) distance="<<input.plane.distance<<" charred="<<input.plane.charred<<" roughness="<<input.plane.roughness<<" valid="<<valid(input.plane)<<" limit="<<input.fragmentLimit<<" canonicalClosed="<<input.closedVolume<<" physicalClosed="<<closedCutVolume(renderMesh(*input.surface))<<" canonicalFaces="<<count(canonicalMesh(*input.surface))<<" renderFaces="<<count(renderMesh(*input.surface))<<std::endl;
 const auto started=std::chrono::steady_clock::now();Projection prepared;bool ok=false;try{ok=prepare(input,prepared);}catch(const std::exception&e){std::cout<<"PREPARE_EXCEPTION "<<e.what()<<"\n";}std::cout<<"PREPARE ok="<<ok<<" changed="<<prepared.result.changed<<" fragments="<<prepared.result.fragments.size()<<" canonicalFaces="<<count(prepared.canonical)<<" renderFaces="<<count(prepared.render)<<" keptVolume="<<prepared.keptVolume<<" removedFraction="<<prepared.result.removedFraction<<" cpu_ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<<std::endl;
 if(ok){for(size_t n=0;n<prepared.fragmentRender.size();++n)std::cout<<"FRAGMENT "<<n<<" area="<<prepared.result.fragments[n].surfaceArea<<" faces="<<count(prepared.fragmentRender[n])<<" physicalClosed="<<closedCutVolume(prepared.fragmentRender[n])<<std::endl;return 0;}
 // On an exact failure only, expose which existing stage rejected this input.
 const auto split=splitSurface(*input.surface,input.plane,input.wood);std::cout<<"SPLIT complete="<<split.complete<<" keptArea="<<split.keptArea<<" detachedArea="<<split.detachedArea<<" seam="<<split.seam.size()<<std::endl;
 return 2;
}

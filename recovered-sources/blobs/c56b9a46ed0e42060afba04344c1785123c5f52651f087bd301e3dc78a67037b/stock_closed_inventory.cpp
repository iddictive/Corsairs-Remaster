#include "ship_laser_geometry.hpp"
#include "ship_wood_surface_profiles.hpp"
#include "rdf.h"
#include <fstream>
#include <cassert>
#include <cstdio>
#include <chrono>
using namespace storm_ship_laser;
template<class T>std::vector<T>read(std::ifstream&f,size_t n){std::vector<T>v(n);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));assert(f);return v;}
int main(int argc,char**argv){for(int a=1;a<argc;++a){const char*path=argv[a];std::ifstream f(path,std::ios::binary);const auto h=read<RDF_HEAD>(f,1)[0];assert(h.version==RDF_VERSION);const auto names=read<char>(f,h.name_size);read<int32_t>(f,h.names);const auto textures=read<int32_t>(f,h.ntextures);const auto materials=read<RDF_MATERIAL>(f,h.nmaterials);read<RDF_LIGHT>(f,h.nlights);read<RDF_LABEL>(f,h.nlabels);const auto objects=read<RDF_OBJECT>(f,h.nobjects);const auto triangles=read<RDF_TRIANGLE>(f,h.ntriangles);const auto buffers=read<RDF_VERTEXBUFF>(f,h.nvrtbuffs);std::vector<std::vector<uint8_t>>vertices;for(const auto&buffer:buffers)vertices.push_back(read<uint8_t>(f,buffer.size));Mesh original;std::map<int,float>wood;
 for(size_t o=0;o<objects.size();++o){const auto&ob=objects[o];const auto&buffer=buffers.at(ob.vertex_buff);const unsigned stride=36+(buffer.type&3)*8+(buffer.type>>2)*8;assert(!(buffer.type>>2));for(int n=0;n<ob.ntriangles;++n){Triangle t;for(unsigned j=0;j<3;++j)t[j]=storm_ship_hull::decode(vertices[ob.vertex_buff].data()+size_t(ob.svertex+triangles[ob.striangle+n].vindex[j])*stride,stride);original[int(o)].push_back(t);}const auto&mat=materials.at(ob.material);if(mat.texture_type[0]==RDF_TEXTURE_TYPE::TEXTURE_BASE){const float thickness=storm_ship_wood::thickness(path,names.data()+textures.at(mat.texture[0]));if(thickness>0)wood[int(o)]=thickness;}}
 const auto t=std::chrono::steady_clock::now();const double volume=closedVolume(original);std::printf("ROOT %s closedVolume=%.12g wood=%zu objects=%zu\n",path,volume,wood.size(),original.size());assert(wood.empty()||volume==0);
 if(std::string(path).find("Galeon_l1.gm")!=std::string::npos){const auto surface=timberSurface(std::move(original),wood);const auto canonical=canonicalMesh(surface),material=renderMesh(surface);auto index=indexFaces(material);std::vector<uint32_t>scratch;scratch.reserve(index.references.size());std::printf("TOPOLOGY_CACHE_BENCH path=%s physicalFaces=%zu cachedCPUPreparationMs=%.6f includesVolume=1 excludesExistingClipping=1 nativeFPSAcceptance=0\n",path,index.references.size(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());}
}}

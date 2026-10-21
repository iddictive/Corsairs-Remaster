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
int main(int argc,char**argv){assert(argc==3);std::ifstream f(argv[1],std::ios::binary);assert(f);const auto h=read<RDF_HEAD>(f,1)[0];assert(h.version==RDF_VERSION);const auto names=read<char>(f,h.name_size);read<int32_t>(f,h.names);const auto textures=read<int32_t>(f,h.ntextures);const auto materials=read<RDF_MATERIAL>(f,h.nmaterials);read<RDF_LIGHT>(f,h.nlights);read<RDF_LABEL>(f,h.nlabels);const auto objects=read<RDF_OBJECT>(f,h.nobjects);const auto triangles=read<RDF_TRIANGLE>(f,h.ntriangles);const auto buffers=read<RDF_VERTEXBUFF>(f,h.nvrtbuffs);std::vector<std::vector<uint8_t>>vertices;for(const auto&buffer:buffers)vertices.push_back(read<uint8_t>(f,buffer.size));Mesh original;std::map<int,float>wood;
 std::cout<<std::setprecision(12)<<"GM objects="<<h.nobjects<<" triangles="<<h.ntriangles<<" bboxCenter="<<h.bbox_center.x<<","<<h.bbox_center.y<<","<<h.bbox_center.z<<" bboxSize="<<h.bbox_size.x<<","<<h.bbox_size.y<<","<<h.bbox_size.z<<"\n";
 for(size_t o=0;o<objects.size();++o){const auto&ob=objects[o];auto&out=original[o];const auto&buffer=buffers.at(ob.vertex_buff);unsigned stride=36+(buffer.type&3)*8+(buffer.type>>2)*8;assert(!(buffer.type>>2));for(int n=0;n<ob.ntriangles;++n){Triangle t;for(unsigned j=0;j<3;++j)t[j]=storm_ship_hull::decode(vertices[ob.vertex_buff].data()+size_t(ob.svertex+triangles[ob.striangle+n].vindex[j])*stride,stride);out.push_back(t);}const auto&mat=materials.at(ob.material);const char*texture="";if(mat.texture_type[0]==RDF_TEXTURE_TYPE::TEXTURE_BASE){texture=names.data()+textures.at(mat.texture[0]);const float thickness=storm_ship_wood::thickness(argv[1],texture);if(thickness>0)wood[o]=thickness;}std::cout<<"OBJECT "<<o<<" faces="<<out.size()<<" texture="<<texture<<" wood="<<(wood.count(o)?wood.at(o):0)<<"\n";}
 const auto ray=[](const Mesh&m,Vec3 start,Vec3 end){float hit=2.f,a,b;for(const auto&o:m)for(const auto&t:o.second)hit=std::min(hit,trace(t,start,end,a,b));return hit;};
 auto surface=std::make_shared<const Surface>(timberSurface(original,wood));
 std::ifstream doses(argv[2]);unsigned samples,batch=0;std::vector<BeamSample>all;
 while(doses>>samples){Plane plane;for(unsigned n=0;n<samples;++n){BeamSample b;for(auto&v:b.start)doses>>v;for(auto&v:b.direction)doses>>v;for(auto&v:b.tangent)doses>>v;doses>>b.radius>>b.depth;assert(doses);plane.beams.push_back(b);all.push_back(b);}const auto&d=plane.beams.front().direction;plane.normal={d[0],d[1],d[2]};
  Input input;input.surface=surface;input.wood=wood;input.plane=plane;input.fragmentLimit=8;input.enclosedFragments=true;
  Projection output;bool ok=prepare(input,output);std::cout<<"BATCH "<<batch++<<" ok="<<ok<<" changed="<<output.result.changed<<" fragments="<<output.fragmentRender.size()<<" openings="<<output.result.openings.size()<<"\n";assert(ok);surface=output.kept;
 }
 auto rendered=renderMesh(*surface);for(unsigned n=0;n<all.size();++n){auto&b=all[n];auto start=beamStart(b),d=beamDirection(b),a=start-d,beyond=start+d*3.f;float before=ray(original,a,beyond),after=ray(rendered,a,beyond);std::cout<<"CONTACT "<<n<<" hitBefore="<<before<<" hitAfter="<<after<<" distanceRemoved="<<(after-before)*4.f<<"\n";}
 std::cout<<"sourceFaces="<<count(original)<<" retainedFaces="<<count(rendered)<<"\n";
}

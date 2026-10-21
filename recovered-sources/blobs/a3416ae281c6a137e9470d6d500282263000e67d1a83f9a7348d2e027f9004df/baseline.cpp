#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include "gpu_skinning_bridge.hpp"
constexpr int D3DUSAGE_WRITEONLY=1,D3DFVF_XYZ=2,D3DFVF_NORMAL=4,D3DFVF_DIFFUSE=8,D3DFVF_TEX1=16;
struct FrameFlightRing {static constexpr size_t count=3;size_t current=0;};
size_t uploads=0,uploadedBytes=0,validations=0,liveFacades=0;
struct FakeBuffer {std::vector<unsigned char>data;FakeBuffer(size_t n):data(n){}};
auto newBuffer(size_t n){return std::make_shared<FakeBuffer>(n);}
void copyTracked(void*d,const void*s,size_t n){++uploads;uploadedBytes+=n;std::memcpy(d,s,n);}
struct VertexBuffer {std::vector<unsigned char>bytes;int usage=0,fvf=0;bool gpuSkinned=false;std::shared_ptr<FakeBuffer>gpuSkinningBuffer,skinningRing[3];size_t skinningRingBytes[3]{};VertexBuffer(size_t n):bytes(n){++liveFacades;}void Release(){delete this;}~VertexBuffer(){--liveFacades;}};
struct Device {FrameFlightRing flightRing;std::map<const storm::metal::skinning::AnimatedVertex*,VertexBuffer*>skinningSources;
bool acceptSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices,uint32_t total,uint32_t bones){if(!vertices||!total||!bones||bones>256)return false;for(uint32_t i=0;i<total;++i){++validations;uint32_t packed=vertices[i].packedBones;if((packed&255u)>=bones||((packed>>8)&255u)>=bones)return false;}const size_t bytes=size_t(total)*sizeof(*vertices);auto found=skinningSources.find(vertices);VertexBuffer*facade=nullptr;if(found!=skinningSources.end()){if(found->second->bytes.size()==bytes)facade=found->second;else{found->second->Release();skinningSources.erase(found);}}if(!facade){facade=new VertexBuffer(bytes);facade->usage=D3DUSAGE_WRITEONLY;facade->fvf=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1;facade->gpuSkinned=true;skinningSources.emplace(vertices,facade);}const size_t slot=flightRing.current%FrameFlightRing::count;if(!facade->skinningRing[slot]||facade->skinningRingBytes[slot]!=bytes){facade->skinningRing[slot]=newBuffer(bytes);if(!facade->skinningRing[slot])return false;facade->skinningRingBytes[slot]=bytes;}copyTracked(facade->skinningRing[slot]->data.data(),vertices,bytes);facade->gpuSkinningBuffer=facade->skinningRing[slot];return true;}};
int main(){using namespace storm::metal::skinning;std::vector<AnimatedVertex>v(6000);for(size_t i=0;i<v.size();++i){v[i].packedBones=1|(2<<8);v[i].position[0]=float(i);}Device d;
for(int frame=0;frame<6;++frame){d.flightRing.current=frame%3;for(int instance=0;instance<20;++instance){assert(d.acceptSkinningVertices(v.data(),v.size(),3));assert(std::memcmp(d.skinningSources.at(v.data())->gpuSkinningBuffer->data.data(),v.data(),v.size()*44)==0);}}
assert(uploads==120&&validations==120*v.size());printf("BASELINE120calls: %zu uploads/%zu bytes, %zu bone validations\n",uploads,uploadedBytes,validations);for(auto&entry:d.skinningSources)entry.second->Release();assert(liveFacades==0);}

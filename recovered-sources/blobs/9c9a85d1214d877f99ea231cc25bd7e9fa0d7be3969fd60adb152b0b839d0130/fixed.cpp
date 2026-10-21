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
bool failAllocation=false;auto newBuffer(size_t n){if(failAllocation){failAllocation=false;return std::shared_ptr<FakeBuffer>{};}return std::make_shared<FakeBuffer>(n);}
void copyTracked(void*d,const void*s,size_t n){++uploads;uploadedBytes+=n;std::memcpy(d,s,n);}
struct VertexBuffer {std::vector<unsigned char>bytes;int usage=0,fvf=0;bool gpuSkinned=false;std::shared_ptr<FakeBuffer>gpuSkinningBuffer,skinningRing[3];size_t skinningRingBytes[3]{};VertexBuffer(size_t n):bytes(n){++liveFacades;}void Release(){delete this;}~VertexBuffer(){--liveFacades;}};
struct SkinningSource {VertexBuffer*buffer;uint32_t maximumBone;};
struct Device {FrameFlightRing flightRing;std::map<const storm::metal::skinning::AnimatedVertex*,SkinningSource>skinningSources;
bool acceptSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices,uint32_t total,uint32_t bones){
 if(!vertices||!total||!bones||bones>256)return false;
 const size_t bytes=size_t(total)*sizeof(*vertices);auto found=skinningSources.find(vertices);VertexBuffer*facade=nullptr;
 if(found!=skinningSources.end()&&found->second.buffer->bytes.size()==bytes){
  if(found->second.maximumBone>=bones)return false;
  facade=found->second.buffer;
 }else{
  uint32_t maximumBone=0;
  for(uint32_t i=0;i<total;++i){++validations;uint32_t packed=vertices[i].packedBones;const uint32_t maximum=std::max(packed&255u,(packed>>8)&255u);if(maximum>=bones)return false;maximumBone=std::max(maximumBone,maximum);}
  if(found!=skinningSources.end()){found->second.buffer->Release();skinningSources.erase(found);}
  facade=new VertexBuffer(bytes);facade->usage=D3DUSAGE_WRITEONLY;facade->fvf=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1;facade->gpuSkinned=true;skinningSources.emplace(vertices,SkinningSource{facade,maximumBone});
 }
 // AVERTEX0 is immutable between geometry-owner unlock/release notifications.
 // Upload each of the existing flight slots once; every character still uploads
 // its own animation palette in beginSkinningPose.
 const size_t slot=flightRing.current%FrameFlightRing::count;
 if(!facade->skinningRing[slot]||facade->skinningRingBytes[slot]!=bytes){
  facade->skinningRing[slot]=newBuffer(bytes);if(!facade->skinningRing[slot])return false;
  facade->skinningRingBytes[slot]=bytes;copyTracked(facade->skinningRing[slot]->data.data(),vertices,bytes);
 }
 facade->gpuSkinningBuffer=facade->skinningRing[slot];return true;
}
void invalidateSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices){auto found=skinningSources.find(vertices);if(found==skinningSources.end())return;found->second.buffer->Release();skinningSources.erase(found);}};
using namespace storm::metal::skinning;
namespace GEOS {using ID=int;}
bool fakeBegin(void*,void*,const Matrix4x4*,uint32_t){return true;}
void* fakeBind(void*,const AnimatedVertex*,uint32_t,uint32_t,uint32_t){return nullptr;}
void fakeEnd(void*){}
constexpr int SHIFT_VALUE=999999999,INVALID_BUFFER_ID=-1;
struct AnimationVB {void*buff=nullptr;};AnimationVB avb;
AnimationVB*AnimationBuffer(int id){return id==SHIFT_VALUE?&avb:nullptr;}
struct Renderer{void UnLockVertexBuffer(int){}void ReleaseVertexBuffer(int){}};
struct GEOM_SERVICE_R{Renderer*RenderService;void UnlockVertexBuffer(int vb);void ReleaseVertexBuffer(int vb);};
void GEOM_SERVICE_R::UnlockVertexBuffer(GEOS::ID vb)
{
    if (vb == INVALID_BUFFER_ID)
        return;
    if (vb >= SHIFT_VALUE)
        if (const auto *buffer = AnimationBuffer(vb))
            storm::metal::skinning::invalidateVertices(
                static_cast<const storm::metal::skinning::AnimatedVertex *>(buffer->buff));

    if (RenderService && vb < SHIFT_VALUE)
        RenderService->UnLockVertexBuffer(vb);
}
void GEOM_SERVICE_R::ReleaseVertexBuffer(GEOS::ID vb)
{
    if (vb == INVALID_BUFFER_ID)
        return;
    if (RenderService)
        if (vb >= SHIFT_VALUE)
        {
            if (auto *buffer = AnimationBuffer(vb))
            {
                storm::metal::skinning::invalidateVertices(
                    static_cast<const storm::metal::skinning::AnimatedVertex *>(buffer->buff));
                delete[] static_cast<char *>(buffer->buff);
                *buffer = {};
            }
        }
        else
            RenderService->ReleaseVertexBuffer(vb);
}
int main(){Device d;Renderer renderer;GEOM_SERVICE_R geometry{&renderer};
Provider provider{&d,[](void*c,const AnimatedVertex*v,uint32_t n,uint32_t b){return static_cast<Device*>(c)->acceptSkinningVertices(v,n,b);},fakeBegin,fakeBind,fakeEnd,[](void*c,const AnimatedVertex*v){static_cast<Device*>(c)->invalidateSkinningVertices(v);}};
registerProvider(&provider);
std::vector<AnimatedVertex>v(6000);for(size_t i=0;i<v.size();++i){v[i].packedBones=1|(2<<8);v[i].position[0]=float(i);}avb.buff=v.data();
for(int frame=0;frame<6;++frame){d.flightRing.current=frame%3;for(int instance=0;instance<20;++instance){assert(acceptVertices(v.data(),v.size(),3));assert(std::memcmp(d.skinningSources.at(v.data()).buffer->gpuSkinningBuffer->data.data(),v.data(),v.size()*44)==0);}}
assert(uploads==3&&validations==v.size());printf("FIXED120calls: %zu uploads/%zu bytes, %zu bone validations\n",uploads,uploadedBytes,validations);
auto retained=d.skinningSources.at(v.data()).buffer->gpuSkinningBuffer;const auto original=retained->data;auto*old=d.skinningSources.at(v.data()).buffer;
assert(!acceptVertices(v.data(),v.size(),2));assert(d.skinningSources.at(v.data()).buffer==old);assert(uploads==3);
// Authoritative unlock notification handles replacement at the exact same address.
v[0].position[0]=700;geometry.UnlockVertexBuffer(SHIFT_VALUE);assert(d.skinningSources.empty());assert(liveFacades==0);assert(acceptVertices(v.data(),v.size(),3));assert(retained->data==original);assert(d.skinningSources.at(v.data()).buffer->gpuSkinningBuffer->data!=original);
// Changed addressing and invalid new content do not replace prior valid state.
assert(!acceptVertices(v.data(),v.size()+1,2));assert(d.skinningSources.size()==1);
geometry.UnlockVertexBuffer(SHIFT_VALUE);v[2].packedBones=255;assert(!acceptVertices(v.data(),v.size(),3));assert(d.skinningSources.empty());v[2].packedBones=2;
failAllocation=true;assert(!acceptVertices(v.data(),v.size(),3));assert(acceptVertices(v.data(),v.size(),3));geometry.UnlockVertexBuffer(SHIFT_VALUE);assert(liveFacades==0);
// Real release callback removes every source and its three ring slots before free.
for(int n=0;n<4000;++n){avb.buff=new char[sizeof(AnimatedVertex)*4];auto*source=static_cast<AnimatedVertex*>(avb.buff);std::memset(source,0,sizeof(AnimatedVertex)*4);for(int slot=0;slot<3;++slot){d.flightRing.current=slot;assert(acceptVertices(source,4,1));}assert(d.skinningSources.size()==1&&liveFacades==1);geometry.ReleaseVertexBuffer(SHIFT_VALUE);assert(!avb.buff&&d.skinningSources.empty()&&liveFacades==0);}
// Empty/invalid notifications and legacy five-field providers preserve ABI behavior.
geometry.UnlockVertexBuffer(-1);geometry.ReleaseVertexBuffer(-1);invalidateVertices(nullptr);
Provider legacy{nullptr,nullptr,fakeBegin,fakeBind,fakeEnd};registerProvider(&legacy);invalidateVertices(v.data());registerProvider(nullptr);invalidateVertices(v.data());
puts("PASS seeded rings, cached-bone reject, exact-address replacement, invalid replacement preservation, allocation retry,4000 authoritative release cycles and optionalcallback");}

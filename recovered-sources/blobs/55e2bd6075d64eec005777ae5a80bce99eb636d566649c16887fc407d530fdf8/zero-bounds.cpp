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
int main(){Device d;storm::metal::skinning::AnimatedVertex v[3]{};v[0].position[0]=2;v[1].position[1]=3;v[2].position[2]=4;for(auto&x:v)x.weight=1;assert(d.acceptSkinningVertices(v,3,1));auto*f=d.skinningSources.at(v).buffer;float oldMax=0;for(int i=0;i<3;i++){float xyz[3];std::memcpy(xyz,f->bytes.data()+i*44,12);for(float c:xyz)oldMax=std::max(oldMax,c);}assert(oldMax==0);assert(std::memcmp(f->gpuSkinningBuffer->data.data(),v,sizeof(v))==0);d.invalidateSkinningVertices(v);std::puts("REPRO: accepted source vertices (2,3,4), raw CPU bounds still collapse to (0,0,0); GPU ring correctly holds source");}

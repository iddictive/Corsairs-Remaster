#include "render_device_api.hpp"
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
using namespace storm_render;
struct V {float x,y,z; uint32_t color;};
void check(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
void quad(std::vector<V>&v,float x0,float y0,float x1,float y1,float z,uint32_t c){V a{x0,y0,z,c},b{x1,y0,z,c},d{x0,y1,z,c},e{x1,y1,z,c};v.insert(v.end(),{a,b,e,a,e,d});}
int main(){Presentation p{};p.width=p.height=256;Device*d=nullptr;check(succeeded(createMetalDevice(nullptr,&p,&d)),"device");Surface *target=nullptr,*read=nullptr;check(succeeded(d->GetRenderTarget(&target)),"target");check(succeeded(d->CreateOffscreenPlainSurface(256,256,AssetFormat::A8R8G8B8,&read)),"read");
Matrix id{};id._11=id._22=id._33=id._44=1;d->SetTransform(TransformWorld,&id);d->SetTransform(TransformView,&id);d->SetRenderState(RenderCullMode,CullNone);d->SetRenderState(RenderLighting,false);d->SetRenderState(RenderFog,false);d->SetRenderState(RenderBlend,false);d->SetRenderState(RenderDepthTest,true);d->SetRenderState(RenderDepthWrite,true);d->SetFVF(vertexFormat::xyz|vertexFormat::diffuse);d->SetTextureStageState(0,StageColorOp,TextureOpSelectArg1);d->SetTextureStageState(0,StageColorArg1,textureArg::diffuse);d->SetTextureStageState(0,StageAlphaOp,TextureOpSelectArg1);d->SetTextureStageState(0,StageAlphaArg1,textureArg::diffuse);d->SetTextureStageState(1,StageColorOp,TextureOpDisable);
auto frame=[&](bool reverse,bool order,int step,bool coplanar){float travel=step*.01f;float scale=6.f+step*.001f;Matrix proj{};proj._11=proj._22=scale;proj._34=1;proj._33=reverse?.115f/(.115f-32000.f):32000.f/(32000.f-.115f);proj._43=reverse?.115f*32000.f/(32000.f-.115f):-32000.f*.115f/(32000.f-.115f);d->SetTransform(TransformProjection,&proj);d->SetRenderState(RenderDepthCompare,reverse?CompareGreaterEqual:CompareLessEqual);check(succeeded(d->Clear(clear::color|clear::depth,0xff101828,reverse?0:1)),"clear");d->BeginScene();std::vector<V> island,ship,water,patch;
// Synthetic island surface pair and a small foreground ship silhouette; no game assets.
quad(island,-900,-100,900,900,8000-travel,0xff268348);quad(patch,-600,100,650,700,8000+(coplanar?0.f:.2f)-travel,0xffa88a3d);
quad(ship,-50,-60,50,10,6000-travel,0xffb02c2c);quad(ship,-4,10,4,190,6000-travel,0xffb02c2c);quad(ship,4,25,75,165,6000-travel,0xffe8ddd0);
quad(water,-2000,-1100,2000,-100,7800-travel,0xff205da0);
auto draw=[&](std::vector<V>&v){check(succeeded(d->DrawPrimitiveUP(PrimitiveTriangleList,v.size()/3,v.data(),sizeof(V))),"draw");};draw(water);if(order){draw(patch);draw(island);}else{draw(island);draw(patch);}draw(ship);d->EndScene();check(succeeded(d->GetRenderTargetData(target,read)),"readback");LockedRect lr{};check(succeeded(read->LockRect(&lr,nullptr,lock::readOnly)),"lock");std::vector<uint32_t> out(65536);for(int y=0;y<256;y++)memcpy(out.data()+y*256,(char*)lr.pBits+y*lr.Pitch,1024);read->UnlockRect();return out;};
for(bool coplanar:{false,true})for(bool reverse:{false,true}){int sum=0,peak=0,motion=0,flips=0,shipmin=65536,sea=0,land=0;std::vector<uint32_t> previous;for(int i=0;i<=20;i++){auto a=frame(reverse,false,i,coplanar);auto b=frame(reverse,true,i,coplanar);int diff=0,ship=0;for(size_t j=0;j<a.size();j++){diff+=a[j]!=b[j];if(!previous.empty())motion+=a[j]!=previous[j];if(i==0){sea+=a[j]==0xff205da0;land+=a[j]==0xff268348||a[j]==0xffa88a3d;}ship+=a[j]==0xffb02c2c||a[j]==0xffe8ddd0;if(!previous.empty() && a[j]!=previous[j] && a[j]!=b[j])flips++;}shipmin=std::min(shipmin,ship);sum+=diff;peak=std::max(peak,diff);previous=std::move(a);}printf("case=%s projection=%s frames=21 order_diff_sum=%d peak=%d temporal_changed_sum=%d temporal_changed_and_order_dependent=%d visible_ship_min=%d water_frame0=%d island_frame0=%d\n",coplanar?"coplanar_control":"0.2m_gap",reverse?"reverse":"forward",sum,peak,motion,flips,shipmin,sea,land);}
read->Release();target->Release();d->Release();}

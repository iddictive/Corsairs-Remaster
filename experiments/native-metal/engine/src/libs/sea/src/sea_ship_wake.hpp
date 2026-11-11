#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// Transient CPU height field. Execute owns the writer; realize and WaveXZ only
// read the immutable published snapshot after execute has completed.
struct SeaShipWake {
 static constexpr int N=256; static constexpr float DX=4.f, DT=1.f/60.f;
 struct Source {float x,z,dx,dz,length,beam,speed;};
 struct Sample {float h=0, sx=0, sz=0;};
 struct Snapshot {
  int ox=0,oz=0; float heightBound=0.f; std::vector<float> h;
  Sample sample(float x,float z) const {
   const float gx=x/DX-float(ox),gz=z/DX-float(oz);
   if (!(gx>=1.f && gz>=1.f && gx<float(N-2) && gz<float(N-2))) return {};
   const int ix=int(gx),iz=int(gz);const float u=gx-ix,v=gz-iz;
   const float a=h[iz*N+ix],b=h[iz*N+ix+1],c=h[(iz+1)*N+ix],d=h[(iz+1)*N+ix+1];
   Sample r{(a+(b-a)*u)*(1-v)+(c+(d-c)*u)*v,((b-a)*(1-v)+(d-c)*v)/DX,((c-a)*(1-u)+(d-b)*u)/DX};
   if(gx<13.f||gz<13.f||gx>float(N-14)||gz>float(N-14)){
    auto taper=[](float q,float& derivative){float d=std::min(q-1.f,float(N-2)-q);if(d>=12.f){derivative=0.f;return 1.f;}float t=std::clamp(d/12.f,0.f,1.f);derivative=(q<float(N)*.5f?1.f:-1.f)*(6.f*t*(1.f-t))/(12.f*DX);return t*t*(3.f-2.f*t);};
    float dx,dz,wx=taper(gx,dx),wz=taper(gz,dz);r.sx=r.sx*wx*wz+r.h*dx*wz;r.sz=r.sz*wx*wz+r.h*dz*wx;r.h*=wx*wz;
   }
   return r;
  }
 };
 std::vector<float> prev=std::vector<float>(N*N),cur=prev,next=prev;
 std::shared_ptr<const Snapshot> snapshot;
 int ox=0,oz=0; float accumulator=0; bool positioned=false;
 static float maxAbs(const std::vector<float>& values) {
#if defined(__ARM_NEON)
  float32x4_t maxv=vdupq_n_f32(0.f);size_t i=0;for(;i+4<=values.size();i+=4)maxv=vmaxq_f32(maxv,vabsq_f32(vld1q_f32(values.data()+i)));float result=vmaxvq_f32(maxv);for(;i<values.size();i++)result=std::max(result,std::abs(values[i]));return result;
#else
  float result=0.f;for(float x:values)result=std::max(result,std::abs(x));return result;
#endif
 }
 static float maxMotion(const std::vector<float>& a,const std::vector<float>& b) {
#if defined(__ARM_NEON)
  float32x4_t m=vdupq_n_f32(0.f);size_t i=0;for(;i+4<=a.size();i+=4){auto x=vld1q_f32(a.data()+i),y=vld1q_f32(b.data()+i);m=vmaxq_f32(m,vmaxq_f32(vabsq_f32(x),vabsq_f32(vsubq_f32(x,y))));}float result=vmaxvq_f32(m);for(;i<a.size();i++)result=std::max({result,std::abs(a[i]),std::abs(a[i]-b[i])});return result;
#else
  float result=0.f;for(size_t i=0;i<a.size();i++)result=std::max({result,std::abs(a[i]),std::abs(a[i]-b[i])});return result;
#endif
 }
 void clear(){std::fill(prev.begin(),prev.end(),0.f);std::fill(cur.begin(),cur.end(),0.f);std::fill(next.begin(),next.end(),0.f);snapshot.reset();positioned=false;accumulator=0;}
 void recenter(float x,float z){
  int nx=int(std::floor(x/DX))-N/2,nz=int(std::floor(z/DX))-N/2;
  if(!positioned){ox=nx;oz=nz;positioned=true;return;}
  if(std::abs(nx-ox)<16 && std::abs(nz-oz)<16)return;
  const int sx=nx-ox,sz=nz-oz;if(!sx&&!sz)return;
  auto move=[&](std::vector<float>&a){std::fill(next.begin(),next.end(),0.f);const int x0=std::max(1,1-sx),x1=std::min(N-1,N-1-sx),z0=std::max(1,1-sz),z1=std::min(N-1,N-1-sz);if(x0<x1&&z0<z1)for(int z=z0;z<z1;z++)std::copy_n(a.data()+(z+sz)*N+x0+sx,x1-x0,next.data()+z*N+x0);a.swap(next);};
  move(cur);move(prev);ox=nx;oz=nz;
 }
 void step(const std::vector<Source>& sources){
  constexpr float k=(16.f*DT/DX)*(16.f*DT/DX);
  for(int z=1;z<N-1;z++)for(int x=1;x<N-1;x++){int i=z*N+x;const int edge=std::min({x,z,N-1-x,N-1-z});const float damp=.994f*std::min(1.f,edge/12.f);next[i]=cur[i]+damp*(cur[i]-prev[i])+k*(cur[i-1]+cur[i+1]+cur[i-N]+cur[i+N]-4*cur[i]);}
  for(const auto&s:sources){if(!std::isfinite(s.x)||!std::isfinite(s.z)||!std::isfinite(s.dx)||!std::isfinite(s.dz)||!std::isfinite(s.length)||!std::isfinite(s.beam)||!std::isfinite(s.speed)||std::abs(s.speed)<.1f)continue;const float a=std::clamp(s.length*.32f,4.f,80.f),b=std::clamp(s.beam*.7f,2.f,40.f),r=3.f*std::max(a,b);if(s.x+r<float(ox)*DX||s.x-r>float(ox+N)*DX||s.z+r<float(oz)*DX||s.z-r>float(oz+N)*DX)continue;int x0=std::max(1,int(std::floor((s.x-r)/DX))-ox),x1=std::min(N-2,int(std::ceil((s.x+r)/DX))-ox),z0=std::max(1,int(std::floor((s.z-r)/DX))-oz),z1=std::min(N-2,int(std::ceil((s.z+r)/DX))-oz);
   for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){const float px=float(x+ox)*DX-s.x,pz=float(z+oz)*DX-s.z;float u=(px*s.dx+pz*s.dz)/a,v=(-px*s.dz+pz*s.dx)/b;if(std::abs(u)>3||std::abs(v)>3)continue;next[z*N+x]+=DT*DT*.003f*std::clamp(s.speed,-100.f,100.f)*std::clamp(s.beam,0.f,60.f)*u*std::exp(-.5f*(u*u+v*v));}}
  prev.swap(cur);cur.swap(next);
 }
 void advance(float seconds,float cameraX,float cameraZ,const std::vector<Source>& sources){
  if(!std::isfinite(cameraX)||!std::isfinite(cameraZ)||std::abs(cameraX)>1.e7f||std::abs(cameraZ)>1.e7f||!std::isfinite(seconds)||seconds<=0)return;
  if(sources.empty()&&!snapshot)return;
  recenter(cameraX,cameraZ);accumulator+=std::min(seconds,4*DT);int count=0;while(accumulator>=DT&&count++<4){step(sources);accumulator-=DT;}
  if(count){if(sources.empty()&&maxMotion(cur,prev)<1.e-5f){clear();return;}auto s=std::make_shared<Snapshot>();s->ox=ox;s->oz=oz;s->h=cur;s->heightBound=maxAbs(s->h);snapshot=std::move(s);}
 }
};

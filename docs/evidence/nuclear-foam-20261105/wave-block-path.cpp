#include "shared/sea_nuclear_wave.hpp"
#include "sea_geomorph.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace std;
using Clock=chrono::steady_clock;
static SeaNuclearWaveSample sample(const vector<SeaNuclearWave>& waves,float x,float z){SeaNuclearWaveSample r;for(auto &w:waves){auto q=EvaluateSeaNuclearWave(w,x,z);r.height+=q.height;r.slopeX+=q.slopeX;r.slopeZ+=q.slopeZ;r.foam=max(r.foam,q.foam);r.load=max(r.load,q.load);}CompressSeaNuclearWaveSample(r);return r;}
static double ms(Clock::time_point a,Clock::time_point b){return chrono::duration<double,milli>(b-a).count();}
int main(){
 vector<SeaNuclearWave> waves; for(int i=0;i<3600;i++){SeaNuclearWave w{float((i%24)*125-1500),float(((i/24)%20)*125-1250),0,0,true,1};w.Advance(1.f+float(i)/60);if(w.active)waves.push_back(w);}
 constexpr int n=20, m=16, lod=8; constexpr float grid=.07f, scale=.5f, step=grid*(1<<lod), side=step*m; vector<SeaNuclearWaveSample> direct;direct.reserve(n*n*(m+1)*(m+1));
 auto start=Clock::now();for(int bz=0;bz<n;bz++)for(int bx=0;bx<n;bx++){float z=-n*side/2+bz*side;for(int iz=0;iz<=m;iz++,z+=step){float x=-n*side/2+bx*side;for(int ix=0;ix<=m;ix++,x+=step){float px=x,pz=z;SeaMorphGrid(px,pz,0,0,lod,grid,scale);direct.push_back(sample(waves,px,pz));}}}auto mid=Clock::now();
 size_t index=0,total=0;float maxh=0,maxsx=0,maxsz=0,maxfoam=0,maxload=0; for(int bz=0;bz<n;bz++)for(int bx=0;bx<n;bx++){float x1=-n*side/2+bx*side,z1=-n*side/2+bz*side;float loX=numeric_limits<float>::infinity(),loZ=loX,hiX=-loX,hiZ=-loX,z=z1;for(int iz=0;iz<=m;iz++,z+=step){float x=x1;for(int ix=0;ix<=m;ix++,x+=step){float px=x,pz=z;SeaMorphGrid(px,pz,0,0,lod,grid,scale);loX=min(loX,px);hiX=max(hiX,px);loZ=min(loZ,pz);hiZ=max(hiZ,pz);}}
 vector<SeaNuclearWave> c;for(auto &w:waves)if(SeaNuclearWaveIntersectsBox(w,loX,loZ,hiX,hiZ))c.push_back(w);total+=c.size();z=z1;for(int iz=0;iz<=m;iz++,z+=step){float x=x1;for(int ix=0;ix<=m;ix++,x+=step){float px=x,pz=z;SeaMorphGrid(px,pz,0,0,lod,grid,scale);auto v=sample(c,px,pz),o=direct[index++];maxh=max(maxh,abs(v.height-o.height));maxsx=max(maxsx,abs(v.slopeX-o.slopeX));maxsz=max(maxsz,abs(v.slopeZ-o.slopeZ));maxfoam=max(maxfoam,abs(v.foam-o.foam));maxload=max(maxload,abs(v.load-o.load));}}}auto end=Clock::now();
 volatile float sink=0;auto ship0=Clock::now();for(int i=0;i<3600;i++){auto s=sample(waves,float((i%60)*8-240),float((i/60)*8-240));sink=sink+s.height;}auto ship1=Clock::now();
 int misses=0, comparisons=0;for(float base: {0.f,100000.f,1000000.f})for(float offset: {-0.0625f,0.f,0.0625f})for(float age: {1.f,4.f,8.f,20.f}){SeaNuclearWave w{base,base,0,0,true,1};w.Advance(age);float r=w.Front();for(float radial: {r-w.width,r-w.width*.5f,r,r+w.width*.5f,r+w.width})for(int k=-1;k<=1;k++){float x=base+radial+offset, z=base+float(k)*.0625f;float lo=x,hi=x+16.f;for(int b=0;b<2;b++){float left=lo+b*16.f,right=hi+b*16.f;bool candidate=SeaNuclearWaveIntersectsBox(w,left,z,right,z+16.f);for(float px: {left,(left+right)*.5f,right}){auto q=EvaluateSeaNuclearWave(w,px,z);if((q.height!=0||q.slopeX!=0||q.slopeZ!=0||q.foam!=0)&&!candidate)misses++;comparisons++;}}}}
 printf("active=%zu vertices=%zu full_scan_ms=%.3f filtered_with_double_morph_copy_compression_ms=%.3f mean_candidates=%.2f\n",waves.size(),direct.size(),ms(start,mid),ms(mid,end),double(total)/(n*n));printf("max_delta height=%g slopeX=%g slopeZ=%g foam=%g load=%g\n",maxh,maxsx,maxsz,maxfoam,maxload);printf("scalar_3600_ship_samples_ms=%.3f checksum=%.5f boundary_false_negatives=%d/%d\n",ms(ship0,ship1),double(sink),misses,comparisons);return maxh!=0||maxsx!=0||maxsz!=0||maxfoam!=0||maxload!=0||misses;
}

#include "shared/sea_nuclear_wave.hpp"
#include <vector>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <algorithm>
using namespace std;
int main(){
 vector<SeaNuclearWave> w; for(int i=0;i<480;i++){SeaNuclearWave a{float((i%24)*125-1500),float((i/24)*125-1250),0,0,true,1};a.Advance(1.f+float(i%60)/10);w.push_back(a);} 
 double exact=0,filtered=0; double maxdiff=0; int n=0; size_t candidates=0;
 auto now=[](){return chrono::steady_clock::now();}; auto us=[](auto a, auto b){return chrono::duration<double,milli>(b-a).count();};
 vector<float> baseline; baseline.reserve(102400);
 auto start=now(); for(int bz=0;bz<20;bz++)for(int bx=0;bx<20;bx++)for(int z=0;z<16;z++)for(int x=0;x<16;x++){float px=-1600+bx*160+x*10,pz=-1600+bz*160+z*10,h=0;for(auto &a:w)h+=EvaluateSeaNuclearWave(a,px,pz).height;baseline.push_back(h);exact+=h;} auto t1=now();
 for(int bz=0;bz<20;bz++)for(int bx=0;bx<20;bx++){float minX=-1600+bx*160,minZ=-1600+bz*160,maxX=minX+150,maxZ=minZ+150;vector<SeaNuclearWave> c; for(auto &a:w){float nx=max({minX-a.x,0.f,a.x-maxX}),nz=max({minZ-a.z,0.f,a.z-maxZ}),fx=max(abs(minX-a.x),abs(maxX-a.x)),fz=max(abs(minZ-a.z),abs(maxZ-a.z));float outer=a.Front()+a.width*a.lengthScale,inner=max(0.f,a.Front()-a.width*a.lengthScale); if(nx*nx+nz*nz<outer*outer && fx*fx+fz*fz>inner*inner)c.push_back(a);}candidates+=c.size();for(int z=0;z<16;z++)for(int x=0;x<16;x++){float px=minX+x*10,pz=minZ+z*10,h=0;for(auto &a:c)h+=EvaluateSeaNuclearWave(a,px,pz).height;maxdiff=max(maxdiff,double(abs(h-baseline[n++])));filtered+=h;}}auto t2=now();
 SeaNuclearWave a{0,0,0,0,true,1};a.Advance(8);float height=EvaluateSeaNuclearWave(a,a.Front(),0).height;float x=a.Front()+25,eps=.01f;float num=(EvaluateSeaNuclearWave(a,x+eps,0).height-EvaluateSeaNuclearWave(a,x-eps,0).height)/(2*eps);float analytic=EvaluateSeaNuclearWave(a,x,0).slopeX;printf("direct_ms=%.3f filtered_ms=%.3f mean_candidates=%.2f/480 max_height_difference=%.8f checksum=%.4f/%.4f\n",us(start,t1),us(t1,t2),double(candidates)/400,maxdiff,exact,filtered);printf("8sec_active=%d 8sec_front_height=%.4f slope_numeric=%.6f analytic=%.6f\n",a.active,height,num,analytic);return maxdiff>1e-5;}

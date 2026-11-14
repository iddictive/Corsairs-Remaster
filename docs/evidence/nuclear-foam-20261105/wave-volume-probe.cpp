#include "shared/sea_nuclear_wave.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>
using namespace std;
int main(){for(int tier: {1,10}){vector<SeaNuclearWave>w;for(int i=0;i<3300;i++){SeaNuclearWave a{float((i*197)%3000-1500),float((i*311)%3000-1500),0,0,true,float(tier)};a.Advance((3300-i)*.016f);if(a.active)w.push_back(a);}double mean=0,variance=0;float lo=1e9,hi=-1e9;int over40=0,N=3000;for(int i=0;i<N;i++){float x=-1600+fmodf(i*197.53f,3200),z=-1600+fmodf(i*319.41f,3200);SeaNuclearWaveSample s;for(auto&a:w){auto q=EvaluateSeaNuclearWave(a,x,z);s.height+=q.height;}CompressSeaNuclearWaveSample(s);mean+=s.height;variance+=s.height*s.height;lo=min(lo,s.height);hi=max(hi,s.height);over40+=s.height>40;}printf("tier=%d waves=%zu mean=%.3f rms=%.3f min=%.3f max=%.3f above_40=%d/%d\n",tier,w.size(),mean/N,sqrt(variance/N),lo,hi,over40,N);}}

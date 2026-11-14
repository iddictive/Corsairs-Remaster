#include "shared/sea_nuclear_wave.hpp"
#include <cstdio>
#include <cmath>
int main(){ SeaNuclearWave w{0,0,0,0,true,1}; w.Advance(4.f); float x=w.Front(), single=EvaluateSeaNuclearWave(w,x,0).height; auto h=[&](int n){return 12.f*std::asinh(n*single/12.f);}; printf("single=%.4f three=%.4f four=%.4f 480=%.4f uncompressed480=%.4f\n",single,h(3),h(4),h(480),480*single); while(w.active)w.Advance(.01f);printf("lifetime_to_2cm=%.2fs distance=%.0fm\n",w.age,w.Front()); }

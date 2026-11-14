#include "missile_trail_vfx.hpp"
#include <chrono>
#include <cstdio>
using namespace storm_metal;
int main(){MissileTrailVfx v;auto t=std::chrono::steady_clock::now();for(int frame=0;frame<1000;++frame){v.beginFrame(.016f);for(int i=0;i<64;++i)v.emitSegment({float(i*3),0,0},{float(i*3)+20,0,0},.016f,MissileVfxType::Kalibr);}auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();std::printf("64 emitters x 1000 frames CPU %.3f ms (%.4f ms/frame), stored %zu\n",elapsed,elapsed/1000,v.storedCount());}

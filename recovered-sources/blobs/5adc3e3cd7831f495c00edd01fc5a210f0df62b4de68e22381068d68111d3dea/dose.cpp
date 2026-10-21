#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/ship_laser_geometry.hpp"
#include <fstream>
#include <cstdio>
using namespace storm_ship_laser;
int main(int argc,char**argv){
 std::ifstream f(argv[1]);unsigned n;f>>n;std::vector<BeamSample> samples;
 for(unsigned i=0;i<n;++i){BeamSample b;for(auto&x:b.start)f>>x;for(auto&x:b.direction)f>>x;for(auto&x:b.tangent)f>>x;f>>b.radius>>b.depth;samples.push_back(b);}
 auto previous=samples[0],next=samples[1];auto p=beamStart(next),dir=beamDirection(next),axis=beamDirection(previous);
 const float exit=(previous.depth-storm_ship_hull::dot(p-beamStart(previous),axis))/storm_ship_hull::dot(dir,axis);
 bool inside=true;const auto planes=beamPlanes(previous);for(unsigned i=1;i<planes.size();++i)inside&=storm_ship_hull::dot(planes[i].normal,p+dir*exit)<=planes[i].distance+.0001f;
 std::vector<BeamSample> result{next};continueBeamDose(result,{previous});float actual=storm_ship_hull::length(beamStart(result[0])-p);
 std::printf("saved_adjacent_samples=%u lateral_drift=%g prior_bore_radius=%g ray_inside_prior=%d expected_free_path=%g continued=%g\n",n,storm_ship_hull::length((p-beamStart(previous))-axis*storm_ship_hull::dot(p-beamStart(previous),axis)),previous.radius,inside,exit,actual);
 if(!(inside&&exit>.1f&&std::abs(actual-exit)<.001f))return 2;
 const auto fixed=[](std::array<float,3> start,std::array<float,3>direction){BeamSample b;b.start=start;b.direction=direction;b.depth=.4f;return b;};
 const auto prior=fixed({0,0,0},{0,0,1});
 const auto unchanged=[&](BeamSample b){auto original=b;continueBeamDose(b,{prior});return b.start==original.start&&b.depth==original.depth&&b.radius==original.radius&&b.direction==original.direction;};
 if(!unchanged(fixed({.2f,0,0},{0,0,1}))||!unchanged(fixed({0,0,0},{0,0,-1}))||!unchanged(fixed({0,0,.5f},{0,0,1}))||!unchanged(fixed({0,0,-.1f},{0,0,1})))return 3;
 auto grazing=fixed({0,0,0},{.6f,0,.8f});if(!unchanged(grazing))return 4;
 auto front=fixed({.01f,0,-.01f},{0,0,1});continueBeamDose(front,{prior});if(std::abs(front.start[2]-.4f)>.0001f)return 5;
 auto world=prior;world.start={1600,120,2400};auto moved=world;moved.start[0]+=.008f;continueBeamDose(moved,{world});if(std::abs(moved.start[2]-2400.4f)>.001f)return 6;
 std::vector<BeamSample> pending;float delivered=0,stored=0,deepest=0;
 for(auto b:samples){delivered+=b.depth;const auto original=b;continueBeamDose(b,pending);if(b.depth!=original.depth||b.radius!=original.radius||b.direction!=original.direction)return 7;pending.push_back(b);deepest=std::max(deepest,storm_ship_hull::dot(beamStart(b)-beamStart(samples[0]),beamDirection(samples[0]))+b.depth);}
 for(const auto&b:pending)stored+=b.depth;
 std::printf("negative_independent/reverse/ahead/behind/grazing=PASS front/translated=PASS delivered=%g stored=%g deepest=%g\n",delivered,stored,deepest);
 if(std::abs(delivered-stored)>1.e-6||deepest<.3f)return 8;
 std::vector<BeamSample> chain;for(int i=0;i<5;++i){auto b=fixed({i%2?.008f:0.f,0,0},{0,0,1});continueBeamDose(b,chain);chain.push_back(b);}
 if(std::abs(chain.back().start[2]-1.6f)>.0001f)return 9;
 auto predecessor=prior;predecessor.depth=.8f;
 auto batch=std::vector<BeamSample>{fixed({0,0,0},{0,0,1}),fixed({.008f,0,.4f},{0,0,1})};
 continueBeamDose(batch,{predecessor});if(std::abs(batch[0].start[2]-.8f)>.0001f||std::abs(batch[1].start[2]-1.2f)>.0001f)return 10;
 auto once=batch;continueBeamDose(batch,{predecessor});if(batch[0].start!=once[0].start||batch[1].start!=once[1].start)return 11;
 std::puts("jittered_chain/atomic_retarget/idempotence/dose_conservation=PASS");return 0;
}

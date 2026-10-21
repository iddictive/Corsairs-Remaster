#include "ship_contact_frame.hpp"
#include <chrono>
#include <iostream>
#include <cstdlib>
using namespace storm::ship::contact;
int main() {
 std::vector<FrameBody> fleet;
 for (int i=0;i<32;++i) { FrameBody f; f.body.mass=1000; f.body.start={{i*200.,0},.1*i}; f.body.end={{i*200.+.05,0.02},.1*i+.001}; f.height=12;
  for(int p=0;p<51;++p) {double a=p*6.283185307179586/51;f.body.hull.push_back({4*std::cos(a),15*std::sin(a)});}fleet.push_back(f); }
 auto result=integrateFrame(fleet,.016);
 if(result.exhausted||!result.impacts.empty()) return 1;
 for(size_t i=0;i<fleet.size();++i) if(result.motions[i].touched||result.motions[i].finalPose.position.x!=fleet[i].body.end.position.x||result.motions[i].finalPose.position.z!=fleet[i].body.end.position.z||result.motions[i].finalPose.yaw!=fleet[i].body.end.yaw) return 2;
 std::cout<<"separated32 prepare="<<detail::prepareCalls<<" query="<<detail::queryCalls<<" untouched="<<result.motions.size()<<"\n";
 const auto begin=std::chrono::steady_clock::now();
 for(int frame=0;frame<100;++frame) {auto r=integrateFrame(fleet,.016);if(r.motions.size()!=32)std::abort();}
 std::cout<<"100-frame-ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()<<"\n";
}

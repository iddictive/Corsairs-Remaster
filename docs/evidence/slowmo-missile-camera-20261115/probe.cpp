#include <cstdint>
using entid_t = uint64_t;
#include "phonk_ordnance.hpp"
#include <cassert>
#include <iostream>
using namespace storm::ship::phonk;
int main() {
    NuclearMissile m{}; m.age=1.f; m.duration=10.f;
    m.launch=CVECTOR(0.f); m.target=CVECTOR(9000.f,0.f,0.f);
    m.manualFlight=true; m.manualVelocity=CVECTOR(2000.f,0.f,0.f);
    double remainder=0; float oldError=0, newError=0;
    int oldFrozen=0, newFrozen=0; float lastOld=-1, lastNew=-1;
    for(int n=0;n<120;++n) {
        const double dt=8.*.1+remainder;
        const auto whole=static_cast<unsigned>(dt); remainder=dt-whole;
        m.age+=whole*.001f;
        const float t=missileRenderAge(m,float(remainder));
        const auto body=nuclearPosition(m,t);
        const auto oldCamera=nuclearPosition(m,m.age);
        const auto camera=nuclearPosition(m,missileRenderAge(m,float(remainder)));
        oldError=std::max(oldError,std::abs(body.x-oldCamera.x));
        newError=std::max(newError,std::abs(body.x-camera.x));
        oldFrozen+=oldCamera.x==lastOld; newFrozen+=camera.x==lastNew;
        lastOld=oldCamera.x; lastNew=camera.x;
        assert(nuclearVelocity(m,t).x==2000.f);
        const auto again=nuclearPosition(m,missileRenderAge(m,float(remainder)));
        assert(again.x==camera.x); // repeated draw/pause cannot advance sample
    }
    assert(oldError>1.f && newError==0.f && oldFrozen>0 && newFrozen==0);
    assert(missileRenderAge(m,0)==m.age); // normal speed and pause
    m.age=m.duration-.0001f; assert(missileRenderAge(m,.9f)==m.duration);
    std::cout<<"PASS 0.1x: old camera error="<<oldError<<" new="<<newError<<"; frozen old="<<oldFrozen<<" new="<<newFrozen<<"; repeat/pause/normal/lifetime PASS\n";
}

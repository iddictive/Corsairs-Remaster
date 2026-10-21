#include <vector>
#include <algorithm>
#include <iostream>
namespace storm_hull_damage {constexpr unsigned MaxBeamSamples=16;}
namespace storm {namespace ship {namespace phonk {constexpr unsigned maxLaserCuts=32;}}}
namespace storm_render {constexpr int TransformView=0;}
constexpr float laserSampleTime=.035f;
struct State {std::vector<int> cuts,pendingBeam;float sampleClock=.04f;};
struct Work {std::vector<int> requests;bool active=false;};
struct Receiver {State phonkLaser;Work phonkLaserWork;bool admit(){return !(phonkLaser.cuts.size() + phonkLaserWork.requests.size() + bool(phonkLaserWork.active) >=
            storm::ship::phonk::maxLaserCuts);}};
struct CMatrix {int camera=0;};
struct Renderer {unsigned refreshes=0;int currentCamera=77;void GetTransform(int,CMatrix&view){++refreshes;view.camera=currentCamera;}};
struct Emitter {State phonkLaser;Receiver&receiver;Renderer*pRS;bool commitBeam(){return receiver.admit();}void frame(){    if (phonkLaser.sampleClock < laserSampleTime) return;
    if (phonkLaser.pendingBeam.size() >= storm_hull_damage::MaxBeamSamples && !commitBeam()) return;
    const float sampleTime = std::min(phonkLaser.sampleClock, .1f);
    phonkLaser.sampleClock = 0.f;
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
}};
int main(){
struct Case{unsigned cuts,queued,pending;bool active;const char*name;};
const Case cases[]={{31,0,16,false,"room_for_one"},{32,0,15,false,"full_receiver_partial_batch"},{32,0,16,false,"full_receiver_full_batch"},{31,1,16,false,"queued_reservation_full"},{31,0,16,true,"active_reservation_full"}};
for(const auto&c:cases){Receiver receiver;receiver.phonkLaser.cuts.resize(c.cuts);receiver.phonkLaserWork.requests.resize(c.queued);receiver.phonkLaserWork.active=c.active;Renderer renderer;Emitter emitter{{},receiver,&renderer};emitter.phonkLaser.pendingBeam.resize(c.pending);emitter.frame();renderer.currentCamera=99;emitter.phonkLaser.sampleClock=.04f;emitter.frame();std::cout<<c.name<<','<<receiver.admit()<<','<<renderer.refreshes<<','<<emitter.phonkLaser.pendingBeam.size()<<'\n';}
}

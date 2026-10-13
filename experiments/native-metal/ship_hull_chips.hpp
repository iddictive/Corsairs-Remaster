#pragma once
#include "ship_hull_geometry.hpp"
namespace storm_ship_hull {
class ChipLease {
 inline static unsigned activeBursts=0;bool held=false;
 public:
 static constexpr unsigned particlesPerBurst=8,maxBursts=8,maxPopulation=particlesPerBurst*maxBursts;
 ChipLease()=default;ChipLease(const ChipLease&)=delete;ChipLease&operator=(const ChipLease&)=delete;
 bool acquire(){if(held)return true;if(activeBursts>=maxBursts)return false;++activeBursts;held=true;return true;}
 void release(){if(held){--activeBursts;held=false;}}
 ~ChipLease(){release();}
 static unsigned liveBursts(){return activeBursts;}
};
struct Chip {Vec3 position,velocity,angles,spin;float age=0,life=1.6f;bool alive=true;};
inline Chip launchChip(Vec3 position,Vec3 normal,uint32_t&seed,float radius){auto f=frame(normal);Chip c;
 c.position=position+normal*.015f;c.velocity=normal*(2.f+randomUnit(seed)*4.f)+f.tangent*((randomUnit(seed)-.5f)*4.f)+f.bitangent*((randomUnit(seed)-.5f)*4.f)+Vec3{0,1.5f+randomUnit(seed)*2.f,0};
 c.angles={randomUnit(seed)*6.28f,randomUnit(seed)*6.28f,randomUnit(seed)*6.28f};c.spin={(randomUnit(seed)-.5f)*12.f,(randomUnit(seed)-.5f)*12.f,(randomUnit(seed)-.5f)*12.f};c.life=1.2f+randomUnit(seed)*.8f;return c;}
// Game-step seconds only. An absent/paused execute step never advances chips.
inline void step(Chip&c,float seconds){if(!c.alive||!std::isfinite(seconds)||seconds<=0)return;float dt=std::min(seconds,std::max(0.f,c.life-c.age));Vec3 gravity{0,-9.81f,0};c.position=c.position+c.velocity*dt+gravity*(.5f*dt*dt);c.velocity=c.velocity+gravity*dt;c.angles=c.angles+c.spin*dt;c.age+=dt;if(c.age>=c.life-1.e-6f)c.alive=false;}
}

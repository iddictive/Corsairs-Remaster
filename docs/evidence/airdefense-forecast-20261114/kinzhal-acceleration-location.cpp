#include <cstdint>
using entid_t=uint64_t;
#include "phonk_ordnance.hpp"
#include <cstdio>
using namespace storm::ship::phonk;
float shotVariation(uint32_t serial, uint32_t owner, uint32_t salt)
{
    uint32_t bits = serial * 0x9e3779b9u ^ owner ^ salt;
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    return float(bits & 65535u) / 65535.f;
}
void varyMissile(NuclearMissile &shot, uint32_t owner, float configuredSpeed = 1.f, float configuredAcceleration = 1.f)
{
    shot.damageScale = .9f + .2f * shotVariation(shot.shotId, owner, 0x4cf5ad43u);
    shot.blastScale = .96f + .08f * shotVariation(shot.shotId, owner, 0xd7125ab3u);
    shot.flightVariation = {
        .97f + .06f * shotVariation(shot.shotId, owner, 0x178be123u),
        .98f + .04f * shotVariation(shot.shotId, owner, 0x57d312a5u),
        .96f + .08f * shotVariation(shot.shotId, owner, 0xac882f41u), configuredSpeed, configuredAcceleration,
        .3f + 1.35f * shotVariation(shot.shotId, owner, 0xc14d9b73u)};
    shot.flightVariation.arcPattern = std::min(2u, uint32_t(shotVariation(shot.shotId, owner, 0x3f10cd25u) * 3.f));
    shot.duration = missile::duration(shot.type,
        {shot.launch.x, shot.launch.y, shot.launch.z},
        {shot.target.x, shot.target.y, shot.target.z}, shot.flightVariation);
    shot.nominalMeanSpeed = std::sqrt(~(shot.target - shot.launch)) / std::max(.001f, shot.duration);
}
int main(){for(float dist:{1000.f,2000.f,4000.f,8000.f}){float sumT=0,sumRemain=0,sumSpeed=0;int early=0,middle=0,late=0;for(int n=0;n<100;n++){uint32_t attacker=2+(19+n)*2;NuclearMissile m;m.active=true;m.type=missile::Type::Kinzhal;m.owner=attacker;m.shotId=1;m.launch=CVECTOR(0,10,dist);m.target=CVECTOR((shotVariation(1,attacker,102)*2-1)*80,5.5f,(shotVariation(1,attacker,103)*2-1)*80);varyMissile(m,attacker);float best=0,tm=0,sp=0,rem=0;for(float t=.06f;t<m.duration-.06f;t+=.02f){auto v=nuclearVelocity(m,t);float v2=~v;if(v2<1)continue;auto a=(nuclearVelocity(m,t+.04f)-nuclearVelocity(m,t-.04f))*(1.f/.08f);a-=v*((a|v)/v2);float g=std::sqrt(~a)/9.81f;if(g>best){best=g;tm=t;sp=std::sqrt(v2);rem=std::sqrt(~(nuclearPosition(m,t)-m.target));}}float phase=tm/m.duration;sumT+=phase;sumRemain+=rem;sumSpeed+=sp;if(phase<.33f)early++;else if(phase<.67f)middle++;else late++;if(n==0)printf("example dist %.0f peak %.1fg at %.2fs/%.2fs remaining %.0fm speed %.0fm/s arcPattern %u arcScale %.2f\n",dist,best,tm,m.duration,rem,sp,m.flightVariation.arcPattern,m.flightVariation.arcScale);}printf("dist %.0f phase %.2f remaining %.0fm speed %.0f early %d middle %d late %d\n",dist,sumT/100,sumRemain/100,sumSpeed/100,early,middle,late);}}
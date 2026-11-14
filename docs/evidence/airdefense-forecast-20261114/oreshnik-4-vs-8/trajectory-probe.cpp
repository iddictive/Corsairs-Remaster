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
int main(){for(int test=0;test<2;test++){int trial=test?128:27;float dist=test?8000.f:4000.f;uint32_t owner=2+(19+trial)*2;NuclearMissile m;m.active=true;m.type=missile::Type::Oreshnik;m.owner=owner;m.shotId=1;m.launch=CVECTOR(0.f,10.f,dist);m.target=CVECTOR((shotVariation(1,owner,102)*2-1)*80,5.5f,(shotVariation(1,owner,103)*2-1)*80);varyMissile(m,owner);float apex=0,best=-1e9;for(float t=0;t<m.duration;t+=.005f){auto p=nuclearPosition(m,t);if(p.y>best){best=p.y;apex=t;}}printf("distance=%.0f trial=%d duration=%.3f apex=%.3f height=%.1f\n",dist,trial,m.duration,apex,best);for(float t: {test?12.75f:8.625f,apex,test?14.775f:11.425f}){auto p=nuclearPosition(m,t),v=nuclearVelocity(m,t),a=(nuclearVelocity(m,t+.04f)-nuclearVelocity(m,t-.04f))*(1.f/.08f);float v2=~v;auto lat=a-v*((a|v)/v2);printf("t=%.3f h=%.0f speed=%.0f vy=%.0f lateral=%.0f m/s2 %.1fg\n",t,p.y,std::sqrt(v2),v.y,std::sqrt(~lat),std::sqrt(~lat)/9.81f);}}}
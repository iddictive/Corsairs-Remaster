#include "phonk_air_intercept_batch.hpp"
#include <cstdint>
using entid_t=uint64_t;
#include "phonk_ordnance.hpp"
#include <cstdio>
using namespace storm::ship::phonk;
struct SEA_BASE{float WaveXZ(float,float){return 0.f;}};
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
void initializeKinzhalTiming(NuclearMissile &shot)
{
    if (shot.type != missile::Type::Kinzhal || shot.kinzhalTiming) return;
    constexpr int count = KinzhalTiming::samples;
    auto timing = std::make_shared<KinzhalTiming>();
    timing->referenceDuration = shot.duration;
    std::array<CVECTOR, count + 1> position;
    std::array<float, count + 1> speed{}, cap{}, distance{};
    const float dt = shot.duration / float(count);
    for (int i = 0; i <= count; ++i) {
        const float time = dt * i;
        position[i] = nuclearPosition(shot, time);
        const auto velocity = nuclearVelocity(shot, time);
        const float speed2 = ~velocity;
        speed[i] = std::sqrt(speed2);
        cap[i] = speed[i];
        if (i > 0 && i < count && speed2 > 1.f) {
            auto acceleration = (nuclearPosition(shot, time + dt) + nuclearPosition(shot, time - dt) -
                                 position[i] * 2.f) * (1.f / (dt * dt));
            acceleration -= velocity * ((acceleration | velocity) / speed2);
            const float curvature = std::sqrt(~acceleration) / speed2;
            if (curvature > 1.e-8f) cap[i] = std::min(cap[i], std::sqrt(50.f * 9.81f / curvature));
        }
        if (i > 0) distance[i] = std::sqrt(~(position[i] - position[i-1]));
    }
    constexpr float tangentialAcceleration = 1000.f;
    for (int i = count-1; i >= 0; --i)
        cap[i] = std::min(cap[i], std::sqrt(cap[i+1]*cap[i+1] + 2.f*tangentialAcceleration*distance[i+1]));
    for (int i = 1; i <= count; ++i)
        cap[i] = std::min(cap[i], std::sqrt(cap[i-1]*cap[i-1] + 2.f*tangentialAcceleration*distance[i]));
    for (int i = 1; i <= count; ++i)
        timing->time[i] = timing->time[i-1] + 2.f*distance[i]/std::max(.01f, cap[i]+cap[i-1]);
    for (int i = 0; i <= count; ++i)
        timing->rate[i] = speed[i] > .01f ? cap[i]/speed[i] : 1.f;
    if (!std::isfinite(timing->time[count]) || timing->time[count] <= 0.f) return;
    shot.duration = timing->time[count];
    shot.nominalMeanSpeed = std::sqrt(~(shot.target - shot.launch)) / shot.duration;
    shot.kinzhalTiming = std::move(timing);
}
void advanceEvasion(NuclearMissile &shot, float dt)
{
    if (shot.evasionStart < 0.f || !(dt > 0.f) ||
        (shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr)) return;
    NuclearMissile base = shot;
    base.evasionStart = -1.f;
    const float sample = std::min({.025f, shot.age, shot.duration - shot.age});
    const float before = shot.age - sample, after = shot.age + sample;
    CVECTOR baseline(0.f);
    if (shot.age - before > .001f && after - shot.age > .001f)
        baseline = (nuclearPosition(base, after) + nuclearPosition(base, before) -
                    nuclearPosition(base, shot.age) * 2.f) * (1.f / (sample * sample));
    const auto velocity = nuclearVelocity(shot, shot.age);
    const float speed2 = ~velocity;
    if (speed2 > 1.f) baseline -= velocity * ((baseline | velocity) / speed2);
    const bool escaping = shot.age - shot.evasionStart < shot.evasionDuration;
    // Reserve headroom for moving-target guidance and discrete integration.
    const float limit = (shot.type == missile::Type::Kinzhal ? 55.f : 30.f) * 9.81f;
    CVECTOR desired = escaping ? shot.evasionOffset * limit :
        shot.evasionPosition * (-2.f / (.5f * .5f)) + shot.evasionVelocity * (-2.f / .5f);
    if (speed2 > 1.f) desired -= velocity * ((desired | velocity) / speed2);
    const float desired2 = ~desired;
    if (desired2 > limit * limit) desired *= limit / std::sqrt(desired2);
    if (shot.type == missile::Type::Kinzhal) {
        const float aa = ~desired, bb = 2.f * (baseline | desired), cc = (~baseline) - limit * limit;
        if (aa > .0001f) {
            const float disc = bb * bb - 4.f * aa * cc;
            const float maximum = disc >= 0.f ? (-bb + std::sqrt(disc)) / (2.f * aa) : 0.f;
            desired *= std::clamp(maximum, 0.f, 1.f);
        }
    }
    shot.evasionPosition += shot.evasionVelocity * dt + desired * (.5f * dt * dt);
    shot.evasionVelocity += desired * dt;
    shot.evasionStateAge = shot.age + dt;
    const auto &last = shot.evasionHistory[(shot.evasionHistoryNext + shot.evasionHistory.size() - 1u) % shot.evasionHistory.size()];
    if (shot.evasionStateAge - last.age >= .010f) {
        shot.evasionHistory[shot.evasionHistoryNext] = {shot.evasionStateAge, shot.evasionPosition, shot.evasionVelocity};
        shot.evasionHistoryNext = (shot.evasionHistoryNext + 1u) % shot.evasionHistory.size();
        shot.evasionHistoryCount = std::min(unsigned(shot.evasionHistory.size()), shot.evasionHistoryCount + 1u);
    }
}
void evadeInterceptor(NuclearMissile &shot, const CVECTOR &defender, SEA_BASE *sea, const CVECTOR *threat = nullptr)
{
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.continueVelocity) return;
    (void)defender;
    const unsigned attempt = shot.evasionAttempts++;
    if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart <
        (shot.type == missile::Type::Kalibr ? kalibrEvasionDuration : shot.evasionDuration)) return;
    const bool firstEvasion = shot.evasionStart < 0.f;
    shot.evasionStart = shot.age;
    if (firstEvasion) {
        shot.evasionPosition = shot.evasionVelocity = CVECTOR(0.f);
        shot.evasionStateAge = shot.age;
        shot.evasionHistory[0] = {shot.age, CVECTOR(0.f), CVECTOR(0.f)};
        shot.evasionHistoryNext = shot.evasionHistoryCount = 1;
    }
    shot.evasionOffset = CVECTOR(0.f);
    const auto velocity = nuclearVelocity(shot, shot.age);
    CVECTOR sideways(-velocity.z, 0.f, velocity.x);
    if ((~sideways) < .001f) return;
    float sign = shotVariation(shot.shotId, uint32_t(shot.owner), 0x1cd773b5u ^ (attempt * 0x85ebca6bu)) < .5f ? -1.f : 1.f;
    if (shot.type == missile::Type::Kalibr && threat) {
        const float side = ((*threat - nuclearPosition(shot, shot.age)) | (!sideways));
        if (std::abs(side) > .1f) sign = side > 0.f ? -1.f : 1.f;
    }
    const float variation = shotVariation(shot.shotId, uint32_t(shot.owner), 0x38f5a7c3u ^ attempt);
    float vertical = shot.type == missile::Type::Kalibr ? 2.f + 4.f * variation :
        variation < .12f ? -(45.f + 40.f * variation / .12f) : 10.f + 20.f * variation;
    if (sea && vertical < 0.f) {
        float clearance = std::numeric_limits<float>::infinity();
        for (const float elapsed : {.25f, .75f, 1.25f, 1.75f}) {
            const auto point = nuclearPosition(shot, std::min(shot.duration, shot.age + elapsed));
            clearance = std::min(clearance, point.y - sea->WaveXZ(point.x, point.z));
        }
        if (std::isfinite(clearance)) vertical = std::max(vertical, -std::max(0.f, clearance - 8.f));
    }
    shot.evasionOffset = !sideways * (sign * (shot.type == missile::Type::Kinzhal ? 100.f : kalibrEvasionOffsetLimit) * (.65f + .7f * variation));
    shot.evasionOffset.y = vertical;
    if ((~shot.evasionOffset) > .0001f) shot.evasionOffset = !shot.evasionOffset;
}
int main(){for(auto type:{missile::Type::Kinzhal,missile::Type::Kalibr})for(float d:{1000.f,4000.f,8000.f})for(float phase:{.1f,.4f,.7f,.9f}){float worst=0,mean=0,maxoff=0,terminal=0;int over=0;for(unsigned n=1;n<=100;n++){NuclearMissile x;x.launch=CVECTOR(0.f,10.f,d);x.target=CVECTOR(0.f,10.f,0.f);x.type=type;x.owner=1;x.shotId=n;varyMissile(x,1);initializeKinzhalTiming(x);x.age=x.duration*phase;x.evasionDuration=type==missile::Type::Kinzhal?.3f:.4f;evadeInterceptor(x,CVECTOR(0.f),nullptr);auto base=x;base.evasionStart=-1.f;auto old=nuclearVelocity(x,x.age);float peak=0;for(int i=0;i<4000 && x.age+.01f<x.duration;i++){advanceEvasion(x,.01f);x.age+=.01f;auto v=nuclearVelocity(x,x.age);auto a=(v-old)*100.f;auto speed2=~v;if(speed2>1) a-=v*((a|v)/speed2);peak=std::max(peak,std::sqrt(~a)/9.81f);old=v;if(i==29){float off=std::sqrt(~(nuclearPosition(x,x.age)-nuclearPosition(base,x.age)));mean+=off;maxoff=std::max(maxoff,off);}}terminal+=std::sqrt(~(nuclearPosition(x,x.age)-nuclearPosition(base,x.age)));worst=std::max(worst,peak);over+=peak>82.f;}printf("%s %.0fm phase %.1f meanOffset=.3s %.2fm max %.2fm terminal %.1fm peakG %.1f above82 %d/100\n",type==missile::Type::Kinzhal?"Kinzhal":"Kalibr",d,phase,mean/100,maxoff,terminal/100,worst,over);}}
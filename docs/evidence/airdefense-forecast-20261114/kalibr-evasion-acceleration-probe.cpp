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
struct SEA_BASE {float WaveXZ(float,float){return 0.f;}};
void evadeInterceptor(NuclearMissile &shot, const CVECTOR &defender, SEA_BASE *sea, const CVECTOR *threat = nullptr)
{
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.continueVelocity) return;
    (void)defender;
    const unsigned attempt = shot.evasionAttempts++;
    if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart <
        (shot.type == missile::Type::Kalibr ? kalibrEvasionDuration : shot.evasionDuration)) return;
    shot.evasionStart = shot.age;
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
    if (shot.type == missile::Type::Kalibr) {
        const float length = std::sqrt(~shot.evasionOffset);
        if (length > kalibrEvasionOffsetLimit) shot.evasionOffset *= kalibrEvasionOffsetLimit / length;
    }
    if (shot.type == missile::Type::Kinzhal && shot.kinzhalTiming) {
        NuclearMissile base = shot;
        base.evasionOffset = CVECTOR(0.f);
        float fraction = 1.f;
        // Keep margin for discrete-step transitions and moving-target guidance.
        constexpr float step = .025f, limit = 55.f * 9.81f;
        const float end = std::min(shot.duration - step, shot.age + shot.evasionDuration);
        for (float time = shot.age + step; time < end; time += step) {
            const auto acceleration = [&](const NuclearMissile &sample) {
                return (nuclearPosition(sample, time + step) + nuclearPosition(sample, time - step) -
                        nuclearPosition(sample, time) * 2.f) * (1.f / (step * step));
            };
            const auto velocity = nuclearVelocity(base, time);
            const float speed2 = ~velocity;
            if (speed2 < 1.f) continue;
            auto baseline = acceleration(base);
            baseline -= velocity * ((baseline | velocity) / speed2);
            auto change = acceleration(shot) - acceleration(base);
            change -= velocity * ((change | velocity) / speed2);
            const float aa = ~change, bb = 2.f * (baseline | change), cc = (~baseline) - limit*limit;
            if (cc >= 0.f) { fraction = 0.f; break; }
            if (aa < .0001f) continue;
            const float discriminant = bb*bb - 4.f*aa*cc;
            if (discriminant < 0.f) { fraction = 0.f; break; }
            fraction = std::min(fraction, std::clamp((-bb + std::sqrt(discriminant)) / (2.f*aa), 0.f, 1.f));
        }
        shot.evasionOffset *= fraction;
    }
}
int main(){float peak=0;int above=0;for(int n=0;n<1000;n++){NuclearMissile m;m.active=true;m.type=missile::Type::Kalibr;m.owner=n;m.shotId=n+1;m.launch=CVECTOR(0.f,10.f,1000.f);m.target=CVECTOR(0.f,5.f,0.f);varyMissile(m,n);m.age=m.duration*.5f;evadeInterceptor(m,CVECTOR(0.f),nullptr);NuclearMissile base=m;base.evasionOffset=CVECTOR(0.f);float best=0;for(float t=m.age+.05f;t<m.age+kalibrEvasionDuration-.05f;t+=.025f){auto delta=[&](float u){return nuclearPosition(m,u)-nuclearPosition(base,u);};auto a=(delta(t+.05f)+delta(t-.05f)-delta(t)*2.f)*(1.f/.0025f);best=std::max(best,std::sqrt(~a)/9.81f);}peak=std::max(peak,best);above+=best>30.5f;}printf("max additional acceleration %.2fg above30.5 %d/1000\n",peak,above);return above>0;}
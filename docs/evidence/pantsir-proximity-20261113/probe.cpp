#include "phonk_air_intercept.hpp"
#include <cstdint>
using entid_t=uint64_t;
#include "phonk_ordnance.hpp"
#include <limits>
#include <cstdio>
using namespace storm::ship::phonk;
struct SEA_BASE { float WaveXZ(float,float) {return 0.f;} };
CVECTOR turnMissileToward(const CVECTOR &velocity, const CVECTOR &aim, float maxAngle)
{
    const float speed = std::sqrt(~velocity), aimLength = std::sqrt(~aim);
    if (!std::isfinite(speed) || !std::isfinite(aimLength) || speed < .001f || aimLength < .001f ||
        !std::isfinite(maxAngle) || maxAngle <= 0.f) return velocity;
    const CVECTOR direction = velocity * (1.f / speed), desired = aim * (1.f / aimLength);
    const float cosine = std::clamp(direction | desired, -1.f, 1.f);
    const float angle = std::acos(cosine);
    if (angle <= maxAngle) return desired * speed;
    auto tangent = desired - direction * cosine;
    if ((~tangent) < 1.e-8f) {
        tangent = direction ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~tangent) < 1.e-8f) tangent = direction ^ CVECTOR(1.f, 0.f, 0.f);
    }
    tangent = !tangent;
    return (direction * std::cos(maxAngle) + tangent * std::sin(maxAngle)) * speed;
}
struct InterceptorProfile { float planningSpeed, initialSpeed, boostAcceleration, sustainAcceleration, maxSpeed; };
constexpr InterceptorProfile interceptorProfile(bool patriot)
{
    // Planning uses an average speed; Patriot's motor has a separate speed cap.
    return patriot ? InterceptorProfile{1600.f, 80.f, 850.f, 250.f, 1800.f}
                   : InterceptorProfile{600.f, 0.f, 0.f, 0.f, 600.f};
}
bool patriotMotor(float age) { return age < 2.5f || (age >= 4.f && age < 5.f); }
void initializePatriot(OrdnanceState::Interceptor &i, const CVECTOR &observed)
{
    i.patriot = true;
    i.duration = 15.f;
    i.position = i.previousPosition = i.launch;
    auto direction = observed - i.launch;
    if ((~direction) < .001f) direction = CVECTOR(0.f, 1.f, 0.f);
    direction = !direction;
    auto side = direction ^ CVECTOR(0.f, 1.f, 0.f);
    if ((~side) < .001f) side = direction ^ CVECTOR(1.f, 0.f, 0.f);
    side = !side;
    const auto up = !(direction ^ side);
    uint32_t bits = i.serial * 0x9e3779b9u ^ uint32_t(i.missileShip);
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    i.speedScale = .96f + .08f * float(bits & 65535u) / 65535.f;
    i.accelerationScale = .96f + .08f * float(bits >> 16) / 65535.f;
    const float angle = 6.2831853f * float(bits & 65535u) / 65535.f;
    const float radius = 16.5f * std::sqrt(float(bits >> 16) / 65535.f);
    // Each launch starts on its own gently displaced course; guidance then
    // corrects naturally towards the tracked target.
    const float launchSpread = .02f + .035f * float(bits >> 16) / 65535.f;
    const auto launchDirection = !(direction + side * (launchSpread * std::cos(angle)) + up * (launchSpread * std::sin(angle)));
    i.velocity = launchDirection * (interceptorProfile(true).initialSpeed * i.speedScale);
    i.seekerBias = side * (radius * std::cos(angle)) + up * (radius * std::sin(angle));
}
// Fixed simulated steps, with a bounded catch-up budget. The debt is retained.
// Tracking uses only this interceptor's stable missile identity, never another contact.
void advancePatriot(OrdnanceState::Interceptor &i, const NuclearMissile *shot, float elapsed)
{
    constexpr float step = .025f;
    bool contact = false;
    i.previousPosition = i.position;
    i.diverting = false;
    i.simulationDebt += std::max(0.f, elapsed);
    for (unsigned n = 0; n < 16 && i.simulationDebt + .000001f >= step && i.age < i.duration; ++n) {
        const float remaining = std::max(0.f, i.simulationDebt - step);
        CVECTOR observed = i.target, targetVelocity(0.f);
        if (shot) {
            const float observedAge = std::max(0.f, shot->age - remaining);
            observed = nuclearPosition(*shot, observedAge);
            targetVelocity = nuclearVelocity(*shot, observedAge);
        }
        const float speed = std::sqrt(~i.velocity);
        const auto relativePosition = observed - i.position;
        const float distance = std::sqrt(~relativePosition);
        const float closing = distance > .001f ? -(targetVelocity | relativePosition) / distance : 0.f;
        float lead = std::clamp(distance / std::max(400.f, speed + closing), 0.f, 2.f);
        if (shot) lead = std::min(lead, std::max(0.f, shot->duration - shot->age));
        auto aim = shot ? observed + targetVelocity * lead + i.seekerBias : i.target;
        if (shot) aim.y = std::max(i.launch.y, aim.y);
        const auto oldVelocity = i.velocity;
        if (shot || !i.missileShip)
            i.velocity = turnMissileToward(i.velocity, aim - i.position, 3.f * step);
        const auto correction = i.velocity - oldVelocity;
        if ((~correction) > .01f) { i.diverting = true; i.divertExhaust = correction * -1.f; }
        constexpr auto profile = interceptorProfile(true);
        const float acceleration = (i.age < 2.5f ? profile.boostAcceleration : (i.age >= 4.f && i.age < 5.f ? profile.sustainAcceleration : 0.f)) * i.accelerationScale;
        const float nextSpeed = std::min(profile.maxSpeed * i.speedScale, speed + acceleration * step);
        if ((~i.velocity) > .001f) i.velocity = !i.velocity * nextSpeed;
        const auto from = i.position;
        i.position += i.velocity * step;
        i.age += step;
        i.simulationDebt = remaining;
        const auto relative = from - (shot ? observed - targetVelocity * step : i.target);
        const auto movement = (i.position - from) - (shot ? targetVelocity * step : CVECTOR(0.f));
        const float length = ~movement;
        const float along = length > .001f ? std::clamp(-(relative | movement) / length, 0.f, 1.f) : 0.f;
        if ((~(relative + movement * along)) <= i.blastRadius * i.blastRadius && nextSpeed >= 350.f) {
            contact = shot || !i.missileShip;
            i.target = contact && shot ? nuclearPosition(*shot, shot->age) : i.position;
            i.patriotContact = contact;
            i.duration = i.age;
            break;
        }
    }
    if (i.age >= i.duration && !contact) i.target = i.position;
}
float shotVariation(uint32_t serial, uint32_t owner, uint32_t salt)
{
    uint32_t bits = serial * 0x9e3779b9u ^ owner ^ salt;
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    return float(bits & 65535u) / 65535.f;
}
float interceptorAimSpread(float detectionRange, float flight)
{
    return 22.f + std::min(45.f, std::max(0.f, detectionRange) * .0025f + std::max(0.f, flight) * .1f);
}
CVECTOR interceptorAimError(const CVECTOR &start, const CVECTOR &target, float detectionRange, float flight,
                            uint32_t serial, uint32_t owner)
{
    auto forward = target - start;
    if ((~forward) < .001f) return CVECTOR(0.f);
    forward = !forward;
    auto side = forward ^ CVECTOR(0.f, 1.f, 0.f);
    if ((~side) < .001f) side = forward ^ CVECTOR(1.f, 0.f, 0.f);
    side = !side;
    const auto up = !(forward ^ side);
    const float angle = shotVariation(serial, owner, 0x3573a23bu) * 6.2831853f;
    // A longer observation distance and flight time widen the predicted corridor.
    const float spread = interceptorAimSpread(detectionRange, flight);
    const float radius = spread * std::sqrt(shotVariation(serial, owner, 0xb12a77e1u));
    return side * (std::cos(angle) * radius) + up * (std::sin(angle) * radius);
}
float interceptorTimingUncertainty(float flight)
{
    return .05f + std::min(.04f, std::max(0.f, flight) * .001f);
}
CVECTOR interceptorTrackingError(const NuclearMissile &shot, float flight, uint32_t serial, uint32_t owner)
{
    // Timing uncertainty models a finite-quality speed and meeting estimate.
    // The proximity fuse still decides hits geometrically.
    const float uncertainty = interceptorTimingUncertainty(flight);
    const float timeError = (2.f * shotVariation(serial, owner, 0x33f9a8d1u) - 1.f) * uncertainty;
    // The speed estimate is 120 ms old; acceleration or turning during that
    // interval produces a forecast error rather than perfect future knowledge.
    const float observedAt = std::max(0.f, shot.age - .12f);
    const float sampleTime = shot.age - observedAt;
    const CVECTOR staleSpeed = shot.type == missile::Type::Shtorm ?
        (sampleTime > 0.f ? (nuclearPosition(shot, shot.age) - nuclearPosition(shot, observedAt)) * (1.f / sampleTime) : CVECTOR(0.f)) :
        nuclearVelocity(shot, observedAt);
    return staleSpeed * timeError;
}
bool interceptorEstimate(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                         float speed, float limit, float &flight, CVECTOR &target)
{
    const auto rel = position - start;
    const float a = (velocity | velocity) - speed * speed;
    const float b = 2.f * (rel | velocity);
    const float c = (rel | rel);
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || speed <= 0.f || limit <= 0.f) return false;
    float t = std::numeric_limits<float>::infinity();
    if (std::abs(a) < .001f) {
        if (b < 0.f) t = -c / b;
    } else {
        const float discriminant = b*b - 4.f*a*c;
        if (discriminant >= 0.f && std::isfinite(discriminant)) {
            const float root = std::sqrt(discriminant);
            const float t1 = (-b-root) / (2.f*a), t2 = (-b+root) / (2.f*a);
            if (t1 > 0.f) t = t1;
            if (t2 > 0.f) t = std::min(t, t2);
        }
    }
    if (!std::isfinite(t) || t > limit) return false;
    flight = std::max(.05f, t);
    target = position + velocity * flight;
    return std::isfinite(~target);
}
bool interceptorEstimateAccelerating(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                                     const CVECTOR &acceleration, float speed, float limit,
                                     float &flight, CVECTOR &target)
{
    if (!(speed > 0.f) || !(limit > 0.f) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) { return position + velocity*t + acceleration*(.5f*t*t); };
    float searchLimit = limit;
    float linearFlight = 0.f;
    CVECTOR linearTarget;
    if (((position-start) | acceleration) < 0.f &&
        interceptorEstimate(start, position, velocity, speed, limit, linearFlight, linearTarget))
        searchLimit = std::min(limit, linearFlight);
    float previous = 0.f;
    for (unsigned n = 1; n <= 32; ++n) {
        float next = searchLimit * float(n) / 32.f;
        if ((~(forecast(next) - start)) <= speed*speed*next*next) {
            for (unsigned k = 0; k < 12; ++k) {
                const float mid = (previous + next) * .5f;
                if ((~(forecast(mid) - start)) <= speed*speed*mid*mid) next = mid; else previous = mid;
            }
            flight = std::max(.05f, next);
            target = forecast(flight);
            return std::isfinite(~target);
        }
        previous = next;
    }
    return false;
}
bool interceptorEstimateShtorm(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                               const CVECTOR &acceleration, float speed, float limit,
                               float &flight, CVECTOR &target)
{
    if (!(speed > 0.f) || !(limit > 0.f) || !std::isfinite(speed) || !std::isfinite(limit) ||
        !std::isfinite(~position) || !std::isfinite(~velocity) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) {
        // Extrapolate measured curvature for at most three seconds, then coast.
        const float curveTime = std::min(t, 3.f);
        return position + velocity*t + acceleration*(curveTime*(t - .5f*curveTime));
    };
    const auto reachable = [&](float t) {
        const auto offset = forecast(t) - start;
        return std::isfinite(~offset) && (~offset) <= speed * speed * t * t;
    };
    // The selected endpoint remains fixed after the interceptor is launched.
    float previous = 0.f;
    // A short interception window can disappear between coarse samples over a
    // long forecast; resolve the first three seconds before the distant leg.
    const float nearLimit = std::min(limit, 3.f);
    for (unsigned n = 1; n <= 64; ++n) {
        float next = n <= 32 ? nearLimit * float(n) / 32.f :
            nearLimit + (limit - nearLimit) * float(n - 32) / 32.f;
        if (reachable(next)) {
            for (unsigned k = 0; k < 12; ++k) {
                const float mid = (previous + next) * .5f;
                if (reachable(mid)) next = mid; else previous = mid;
            }
            flight = std::max(.05f, next);
            target = forecast(flight);
            return std::isfinite(~target);
        }
        previous = next;
    }
    return false;
}
void observeShtorm(const NuclearMissile &shot, const CVECTOR &position, CVECTOR &velocity, CVECTOR &acceleration)
{
    constexpr float interval = .12f;
    const float elapsed = std::min(std::max(0.f, shot.age), interval);
    velocity = elapsed > 0.f ? (position - nuclearPosition(shot, shot.age - elapsed)) * (1.f / elapsed) : CVECTOR(0.f);
    acceleration = CVECTOR(0.f);
    if (shot.age < 2.f * interval) return;
    const auto previous = nuclearPosition(shot, shot.age - interval);
    const auto earlier = nuclearPosition(shot, shot.age - 2.f * interval);
    velocity = (position - previous) * (1.f / interval);
    acceleration = (position - previous * 2.f + earlier) * (1.f / (interval * interval));
    const float magnitudeSquared = ~acceleration;
    if (!std::isfinite(magnitudeSquared)) { acceleration = CVECTOR(0.f); return; }
    const float maxAcceleration = shot.type == missile::Type::Oreshnik ? 900.f : shot.type == missile::Type::Kinzhal ? 1500.f : 250.f;
    if (magnitudeSquared > maxAcceleration * maxAcceleration)
        acceleration = acceleration * (maxAcceleration / std::sqrt(magnitudeSquared));
    if (shot.type == missile::Type::Kinzhal || shot.type == missile::Type::Shtorm)
        velocity += acceleration * (.5f * interval);
}
bool observeBallistic(missile::Type type) { return type == missile::Type::Shtorm || type == missile::Type::Oreshnik || type == missile::Type::Kinzhal; }
CVECTOR observedMissileAcceleration(const NuclearMissile &shot, const CVECTOR &staleVelocity)
{
    if (shot.type != missile::Type::Kalibr || shot.age < .12f) return CVECTOR(0.f);
    const auto current = nuclearVelocity(shot, shot.age);
    const float speed = std::sqrt(~current);
    if (!std::isfinite(speed) || speed < 1.f) return CVECTOR(0.f);
    const auto direction = current * (1.f / speed);
    const float along = std::clamp(((current-staleVelocity) | direction) / .12f, 0.f, 90.f);
    return direction * along;
}
bool interceptorEstimateCruise(const NuclearMissile &shot, const CVECTOR &start, const CVECTOR &position,
                               const CVECTOR &staleVelocity, float speed, float limit,
                               float &flight, CVECTOR &target)
{
    const auto acceleration = observedMissileAcceleration(shot, staleVelocity);
    const auto velocity = staleVelocity + acceleration * std::min(.12f, std::max(0.f, shot.age));
    return interceptorEstimateAccelerating(start, position, velocity, acceleration, speed, limit, flight, target);
}
bool interceptorForwardSector(const CVECTOR &forward, const CVECTOR &origin, const CVECTOR &target)
{
    const float x = target.x - origin.x, z = target.z - origin.z;
    const float forwardSquared = forward.x * forward.x + forward.z * forward.z;
    const float distanceSquared = x * x + z * z;
    if (!std::isfinite(forwardSquared) || !std::isfinite(distanceSquared) || forwardSquared < .001f) return false;
    if (distanceSquared < .001f) return true;
    // Allow a modest sector behind the beam, but not an almost-aft launch.
    return forward.x * x + forward.z * z >= -.70710678f * std::sqrt(forwardSquared * distanceSquared);
}
void evadeInterceptor(NuclearMissile &shot, const CVECTOR &defender, SEA_BASE *sea)
{
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.continueVelocity) return;
    const unsigned attempt = shot.evasionAttempts++;
    if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart < (shot.type == missile::Type::Kalibr ? 3.5f : 2.f)) return;
    shot.evasionStart = shot.age;
    shot.evasionOffset = CVECTOR(0.f);
    const float distance = std::sqrt(~(nuclearPosition(shot, shot.age) - defender));
    const float room = std::clamp((distance - 250.f) / 1750.f, 0.f, 1.f);
    const float chance = shot.type == missile::Type::Kinzhal ? 1.f : .25f * room;
    if (shotVariation(shot.shotId, uint32_t(shot.owner), 0x711f57bdu ^ (attempt * 0x9e3779b9u)) >= chance) return;
    const auto velocity = nuclearVelocity(shot, shot.age);
    CVECTOR sideways(-velocity.z, 0.f, velocity.x);
    if ((~sideways) < .001f) return;
    const float sign = shotVariation(shot.shotId, uint32_t(shot.owner), 0x1cd773b5u ^ (attempt * 0x85ebca6bu)) < .5f ? -1.f : 1.f;
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
    shot.evasionOffset = !sideways * (sign * (shot.type == missile::Type::Kinzhal ? 100.f : 18.f) * (.65f + .7f * variation));
    shot.evasionOffset.y = vertical;
}

int main(){
 const char* names[]={"Kinzhal","Kalibr","Shtorm","Oreshnik"}; missile::Type types[]={missile::Type::Kinzhal,missile::Type::Kalibr,missile::Type::Shtorm,missile::Type::Oreshnik};
 for(float range:{800.f,1600.f,2400.f,4000.f,8000.f}) for(int typ=0;typ<4;typ++) for(int pat=0;pat<1;pat++){
 int acquired=0, fired=0, hits=0, evaded=0; float sumTime=0; constexpr float dt=.010f;
 for(unsigned serial=1;serial<=500;serial++){
 NuclearMissile m; m.active=true;m.type=types[typ];m.launch=CVECTOR((shotVariation(serial,2,100)*2.f-1.f)*1000.f,10.f,10000.f);m.target=CVECTOR((shotVariation(serial,2,102)*2.f-1.f)*80.f,5.5f,(shotVariation(serial,2,103)*2.f-1.f)*80.f);m.owner=2;m.shotId=serial;m.flightVariation.speedScale=.92f+.16f*shotVariation(serial,2,104);m.flightVariation.rampScale=.92f+.16f*shotVariation(serial,2,105);m.flightVariation.arcScale=.6f+.8f*shotVariation(serial,2,106);m.duration=missile::duration(m.type,{m.launch.x,m.launch.y,m.launch.z},{m.target.x,m.target.y,m.target.z},m.flightVariation);
 const CVECTOR ship(0.f,0.f,0.f), start(0.f,5.5f,0.f); CVECTOR p,v,a; bool found=false;
 for(m.age=.25f;m.age<m.duration;m.age+=dt){p=nuclearPosition(m,m.age); if(observeBallistic(m.type))observeShtorm(m,p,v,a);else v=nuclearVelocity(m,m.age); if((~(p-ship))<=range*range && ((p-ship)|v)<0.f && v.x*v.x+v.z*v.z>=1.f){found=true;break;}}
 if(!found) continue; acquired++;
 uint32_t bits=serial*0x9e3779b9u;bits^=bits>>16;bits*=0x7feb352du;bits^=bits>>15;float scale=.96f+.08f*float(bits&65535u)/65535.f;
 float flight=0;CVECTOR target;float speed=pat?1600.f:600.f;float limit=std::min(60.f,m.duration-m.age);auto stale=observeBallistic(m.type)?v:nuclearVelocity(m,std::max(0.f,m.age-.12f));
 bool ok=observeBallistic(m.type)?interceptorEstimateShtorm(start,p,v,a,speed/scale,limit,flight,target):m.type==missile::Type::Kalibr?interceptorEstimateCruise(m,start,p,stale,speed/scale,limit,flight,target):interceptorEstimate(start,p,stale,speed/scale,limit,flight,target);
 if(!ok&&pat&&m.type==missile::Type::Oreshnik){target=p;flight=std::clamp(std::sqrt(~(p-start))/speed,.15f,15.f);ok=true;} if(!ok)continue;fired++;
 target+=interceptorAimError(start,target,std::sqrt(~(p-start)),flight,serial,1)+interceptorTrackingError(m,flight,serial,1);
 float distance=std::sqrt(~(target-start));float arc=std::clamp(distance*.02f,3.f,20.f);float lateral=(float(bits>>16)/65535.f*2.f-1.f)*std::min(12.f,distance*.015f);
 OrdnanceState::Interceptor i{start,target,0.f,flight,arc,lateral,serial,2,serial,0}; i.blastRadius=15.f+15.f*shotVariation(serial,1,0x6c3874a5u); if(pat)initializePatriot(i,p);
 bool swept=false; for(;m.age<m.duration && i.age<i.duration;){
 float step=pat?dt:std::min(dt,i.duration-i.age); auto i0=interceptorPosition(i,i.age/i.duration);
 if(pat)advancePatriot(i,&m,step);else i.age=std::min(i.duration,i.age+step);
 auto i1=interceptorPosition(i,i.age/i.duration);
 if(!i.evasionTriggered && (~(nuclearPosition(m,m.age)-i1))<=300.f*300.f){i.evasionTriggered=true;evadeInterceptor(m,ship,nullptr);if((~m.evasionOffset)>.001f)evaded++;}
 auto m0=nuclearPosition(m,m.age);m.age=std::min(m.duration,m.age+step);auto m1=nuclearPosition(m,m.age);
 if(!pat){namespace ai=storm::air_intercept;auto v=[](CVECTOR p){return ai::Vec3{p.x,p.y,p.z};};ai::Body b{1,1,1,i.blastRadius,2.,{{0.,1.,v(i0),v(i1)}}};ai::Body t{2,2,2,0.,2.,{{0.,1.,v(m0),v(m1)}}};auto found=ai::find(std::vector<ai::Interceptor>{{b,i.blastRadius}},std::vector<ai::Body>{t},[](auto a,auto b){return a!=b;});if(found.contact){swept=true;break;}}
 }
 bool hit=swept || (i.age>=i.duration && (!pat||i.patriotContact) && (~(nuclearPosition(m,m.age)-i.target)) <= (i.blastRadius*i.blastRadius)); if(hit)hits++;sumTime+=i.age;
 }
 printf("range %.0f %-8s %-7s acquired %d/500 fired %d hits %d hit/shot %.1f%% hit/launch %.1f%% evade %d meanFlight %.2f\n",range,names[typ],pat?"Patriot":"Pantsir",acquired,fired,hits,hits*.2f,fired?100.f*hits/fired:0.f,evaded,fired?sumTime/fired:0.f);
 }
}

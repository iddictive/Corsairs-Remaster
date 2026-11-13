#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Arcade-scale flight and geometry for the Phonk ship. These profiles are
// deliberately independent of warhead and impact handling.
namespace storm::ship::phonk::missile
{
enum class Type : uint32_t { Kinzhal = 0, Kalibr = 1, Shtorm = 2, Oreshnik = 4 };

struct Vec3 { float x, y, z; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x*s, a.y*s, a.z*s}; }
inline float magnitude(Vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

inline void guide(Vec3 desiredPoint, float dt, float speed, Vec3 &target, Vec3 &velocity)
{
    if (!std::isfinite(dt) || dt <= 0.f || !std::isfinite(speed) || speed <= 0.f) return;
    const auto correction = desiredPoint - target;
    const float length = magnitude(correction);
    if (!std::isfinite(length)) return;
    if (length <= .001f) { velocity = {0.f,0.f,0.f}; return; }
    const auto desired = correction * (std::min(speed, length / dt) / length);
    const auto change = desired - velocity;
    const float changeLength = magnitude(change);
    if (std::isfinite(changeLength) && changeLength > .001f)
        velocity = velocity + change * (std::min(changeLength, 1800.f * dt) / changeLength);
    const auto step = velocity * dt;
    // A retained correction velocity must not carry the aim point past its
    // destination and start an alternating left/right correction loop.
    if (step.x*correction.x + step.y*correction.y + step.z*correction.z >= length*length) {
        target = desiredPoint;
        velocity = {0.f,0.f,0.f};
    } else target = target + step;
}

struct Profile { float speed, minDuration, maxAltitude, altitudeFraction; };
inline float blastPower(float distance, float radius)
{
    if (!std::isfinite(distance) || !std::isfinite(radius) || radius <= 0.f || distance >= radius) return 0.f;
    const float remaining = 1.f - std::max(0.f, distance) / radius;
    return remaining * remaining;
}
inline Profile profile(Type type)
{
    // Gameplay values, not real-world performance specifications.
    return type == Type::Oreshnik ? Profile{3800.f, 3.f, 9000.f, 1.2f}
         : type == Type::Shtorm ? Profile{800.f, 2.f, 1000.f, .25f}
         : type == Type::Kalibr ? Profile{1300.f, 2.f, 42.f, .035f}
                                : Profile{1800.f, 1.2f, 650.f, .16f};
}
// Integral of a smooth velocity ramp, in seconds at cruise speed. The ramp
// reaches cruise without easing back down at the target.
inline float travelTime(float seconds, float ramp, float initial)
{
    if (seconds >= ramp) return seconds - (1.f-initial)*ramp*.5f;
    const float u = std::max(0.f, seconds) / ramp;
    return ramp * (initial*u + (1.f-initial)*(u*u*u - .5f*u*u*u*u));
}
inline float rampTime(Type type) { return type == Type::Oreshnik ? 6.f : type == Type::Shtorm ? 3.5f : type == Type::Kalibr ? 27.1f : 2.f; }
inline float initialSpeed(Type type) { return type == Type::Oreshnik ? .002f : type == Type::Shtorm ? .05f : type == Type::Kalibr ? 0.f : .1f; }
// One stable value per shot; callers choose the random values and retain them.
// startScale applies to absolute start speed, not its fraction of cruise speed.
struct FlightVariation { float speedScale = 1.f, rampScale = 1.f, startScale = 1.f, configuredSpeed = 1.f, configuredAcceleration = 1.f, arcScale = 1.f; unsigned arcPattern = 0; };
inline float flightScale(float scale) { return std::isfinite(scale) ? std::clamp(scale, .92f, 1.08f) : 1.f; }
inline float settingScale(float scale) { return std::isfinite(scale) ? std::clamp(scale, .25f, 4.f) : 1.f; }
inline float variedCruise(Type type, FlightVariation v) { return profile(type).speed * flightScale(v.speedScale) * settingScale(v.configuredSpeed); }
inline float variedRamp(Type type, FlightVariation v) { return rampTime(type) * flightScale(v.rampScale) * settingScale(v.configuredSpeed) / settingScale(v.configuredAcceleration); }
inline float variedInitial(Type type, FlightVariation v)
{
    return initialSpeed(type) * flightScale(v.startScale) / (flightScale(v.speedScale) * settingScale(v.configuredSpeed));
}
inline constexpr float kalibrEjectTime = .65f;
inline constexpr float kalibrTurnTime = .28f;
inline constexpr float kalibrEjectHeight = 36.f;
// Fictional game units: near-original acceleration with a finite speed cap.
inline constexpr float kalibrAcceleration = 48.f;
inline float variedKalibrAcceleration(FlightVariation v)
{
    return kalibrAcceleration * flightScale(v.speedScale) / flightScale(v.rampScale) * settingScale(v.configuredSpeed) * settingScale(v.configuredAcceleration);
}
inline float arcAltitude(Type type, float distance, FlightVariation variation)
{
    const auto p = profile(type);
    const float scale = std::isfinite(variation.arcScale) ? std::clamp(variation.arcScale, .3f, 1.65f) : 1.f;
    const float base = type == Type::Oreshnik ? std::max(1200.f, distance * p.altitudeFraction) : distance * p.altitudeFraction;
    return std::min(p.maxAltitude, base) *
        (type == Type::Kinzhal ? scale : type == Type::Kalibr ?
            .35f + 1.05f * (scale - .3f) / 1.35f : .9f + .2f * (scale - .3f) / 1.35f);
}
inline float arcProgress(Type type, float travel, FlightVariation variation)
{
    if (type == Type::Kinzhal && variation.arcPattern == 1) return travel * travel * travel;
    if (type == Type::Kinzhal && variation.arcPattern == 2) return 1.f - (1.f-travel)*(1.f-travel);
    return travel;
}
// Arc length of the Oreshnik parabola through parameter u. Parameterizing by
// distance rather than horizontal progress keeps speed bounded on steep arcs.
inline double ballisticPrimitive(double horizontal, double z)
{
    if (horizontal <= .001) return .5*z*std::abs(z);
    return .5 * (z*std::sqrt(horizontal*horizontal + z*z) +
                 horizontal*horizontal*std::asinh(z/horizontal));
}
inline double ballisticLength(double horizontal, double vertical, double altitude, double u)
{
    if (altitude <= .001f) return std::sqrt(horizontal*horizontal + vertical*vertical) * u;
    const double initial = vertical + 4.f*altitude;
    return (ballisticPrimitive(horizontal, initial) - ballisticPrimitive(horizontal, initial - 8.f*altitude*u)) / (8.f*altitude);
}
inline float ballisticParameter(float horizontal, float vertical, float altitude, float fraction)
{
    if (fraction <= 0.f || fraction >= 1.f) return fraction;
    if (altitude <= .001f) return fraction;
    const double initial = vertical + 4. * altitude, span = 8. * altitude;
    const double first = ballisticPrimitive(horizontal, initial);
    const double length = (first - ballisticPrimitive(horizontal, initial - span)) / span;
    if (length <= .001f) return fraction;
    const double desired = length * fraction;
    double lo = 0., hi = 1., u = fraction;
    for (unsigned i = 0; i < 12; ++i) {
        const double error = (first - ballisticPrimitive(horizontal, initial - span*u)) / span - desired;
        if (std::abs(error) <= length * 1e-10) break;
        if (error < 0.f) lo = u; else hi = u;
        const double slope = vertical + 4.*altitude*(1.-2.*u);
        const double derivative = std::sqrt(double(horizontal)*horizontal + slope*slope);
        const double next = derivative > .001 ? u - error / derivative : (lo+hi)*.5;
        u = next > lo && next < hi ? next : (lo+hi)*.5f;
    }
    return float(u);
}
inline float duration(Type type, Vec3 start, Vec3 target, FlightVariation variation = {})
{
    const auto d = target - start;
    if (type == Type::Kalibr) {
        const float horizontalDistance = std::sqrt(d.x*d.x + d.z*d.z);
        // A vertical-only target still gets a finite powered phase.
        const float distance = horizontalDistance > 0.f ? horizontalDistance : std::abs(d.y);
        const float acceleration = variedKalibrAcceleration(variation), cap = variedCruise(type, variation);
        const float capDistance = cap * cap / (2.f * acceleration);
        const float powered = distance <= capDistance ? std::sqrt(2.f * distance / acceleration)
                                                     : cap / acceleration + (distance - capDistance) / cap;
        return kalibrEjectTime + kalibrTurnTime + std::max(.1f, powered);
    }
    const auto p = profile(type);
    const float horizontal = std::sqrt(d.x*d.x + d.z*d.z);
    const float altitude = arcAltitude(type, horizontal, variation);
    // Approximate the actual 3D curve at launch, including its chosen arc.
    // A separate derivative bound limits instantaneous 3D speed without
    // treating the steepest part of the trajectory as the entire path.
    float distance = 0.f;
    Vec3 previous{0.f, 0.f, 0.f};
    for (unsigned i = 1; type != Type::Oreshnik && i <= 24; ++i) {
        const float u = float(i) / 24.f;
        const float arc = arcProgress(type, u, variation);
        const Vec3 point = d*u + Vec3{0.f, 4.f*altitude*arc*(1.f-arc), 0.f};
        distance += magnitude(point - previous);
        previous = point;
    }
    if (type == Type::Oreshnik) {
        distance = ballisticLength(horizontal, d.y, altitude, 1.f);
    } else {
        const float arcSlope = 4.f * altitude * (type == Type::Kinzhal && variation.arcPattern == 1 ? 3.f :
                                                 type == Type::Kinzhal && variation.arcPattern == 2 ? 2.f : 1.f);
        const float slopeBound = std::sqrt(horizontal*horizontal + (std::abs(d.y) + arcSlope)*(std::abs(d.y) + arcSlope));
        const float speedLimit = (type == Type::Shtorm ? 1400.f : 5000.f) *
                                 flightScale(variation.speedScale) * settingScale(variation.configuredSpeed);
        distance = std::max(distance, slopeBound * variedCruise(type, variation) / speedLimit);
    }
    const float ramp = variedRamp(type, variation), initial = variedInitial(type, variation);
    const float needed = distance / variedCruise(type, variation);
    float flight = needed + (1.f-initial)*ramp*.5f;
    if (flight < ramp) {
        float lo = 0.f, hi = ramp;
        for (unsigned n = 0; n < 20; ++n) {
            const float mid = (lo+hi)*.5f;
            if (travelTime(mid, ramp, initial) < needed) lo = mid; else hi = mid;
        }
        flight = hi;
    }
    return std::max(p.minDuration, flight);
}
inline float ignitionFraction(float flightDuration)
{
    return std::min((kalibrEjectTime + kalibrTurnTime) / std::max(.1f, flightDuration), .999999f);
}
// Weight of the linear target interpolation. During vertical ejection the
// missile does not follow target movement. Arc altitude also depends on the
// target's horizontal distance and has its own contribution to velocity.
inline float trajectoryProgress(Type type, float t, float flightDuration, FlightVariation variation = {})
{
    t = std::clamp(t, 0.f, 1.f);
    if (flightDuration <= 0.f) return 0.f;
    const float ejectEnd = type == Type::Kalibr ? ignitionFraction(flightDuration) : 0.f;
    if (t <= ejectEnd) return 0.f;
    const float poweredTime = flightDuration * (1.f-ejectEnd);
    const float elapsed = flightDuration * (t-ejectEnd);
    if (type == Type::Kalibr) {
        const float acceleration = variedKalibrAcceleration(variation), cap = variedCruise(type, variation);
        const auto distanceAt = [&](float time) {
            const float ramp = cap / acceleration;
            return time < ramp ? .5f * acceleration * time * time : cap * (time - .5f * ramp);
        };
        return distanceAt(elapsed) / distanceAt(poweredTime);
    }
    return travelTime(elapsed, variedRamp(type, variation), variedInitial(type, variation)) /
           travelTime(poweredTime, variedRamp(type, variation), variedInitial(type, variation));
}
inline Vec3 position(Type type, Vec3 start, Vec3 target, float t, float flightDuration = 0.f, FlightVariation variation = {})
{
    t = std::clamp(t, 0.f, 1.f);
    const auto d = target - start;
    const float distance = std::sqrt(d.x*d.x+d.z*d.z);
    const float variationScale = std::isfinite(variation.arcScale) ? std::clamp(variation.arcScale, .3f, 1.65f) : 1.f;
    const float altitude = arcAltitude(type, distance, variation);
    const float total = flightDuration > 0.f ? flightDuration : duration(type, start, target, variation);
    float travel = trajectoryProgress(type, t, total, variation);
    if (type == Type::Oreshnik)
        travel = ballisticParameter(distance, d.y, altitude, travel);
    if (type != Type::Kalibr) {
        const float arc = arcProgress(type, travel, variation);
        return start + d*travel + Vec3{0.f, 4.f*altitude*arc*(1.f-arc), 0.f};
    }
    // Deck ejection loses upward speed, then carries that velocity into the turn.
    const float elapsedFlight = t * total;
    const float horizontal = std::sqrt(d.x*d.x + d.z*d.z);
    const float fx = horizontal > .001f ? d.x / horizontal : 0.f;
    const float fz = horizontal > .001f ? d.z / horizontal : 1.f;
    const float sideSpeed = (float(variation.arcPattern % 3u) - 1.f) * 3.f;
    const float forwardSpeed = 1.f + (variationScale - .3f);
    const Vec3 ejectVelocity{fx*forwardSpeed - fz*sideSpeed, kalibrEjectHeight / (3.f * kalibrEjectTime),
                             fz*forwardSpeed + fx*sideSpeed};
    if (elapsedFlight < kalibrEjectTime) {
        const float u = elapsedFlight / kalibrEjectTime;
        const float elapsed = u * kalibrEjectTime;
        return start + Vec3{ejectVelocity.x * elapsed, kalibrEjectHeight * ((5.f/3.f)*u - (2.f/3.f)*u*u),
                            ejectVelocity.z * elapsed};
    }
    const float lift = std::clamp(travel / (.11f + .035f * float(variation.arcPattern % 3u)), 0.f, 1.f);
    const float dive = std::clamp((1.f-travel) / (.15f + .035f * float(variation.arcPattern % 3u)), 0.f, 1.f);
    const float smoothLift = lift*lift*(3.f-2.f*lift);
    const float smoothDive = dive*dive*(3.f-2.f*dive);
    const Vec3 ejected = start + Vec3{ejectVelocity.x * kalibrEjectTime, kalibrEjectHeight,
                                     ejectVelocity.z * kalibrEjectTime};
    const Vec3 turnAcceleration{fx * 180.f, -ejectVelocity.y / kalibrTurnTime, fz * 180.f};
    if (elapsedFlight < kalibrEjectTime + kalibrTurnTime) {
        const float elapsed = elapsedFlight - kalibrEjectTime;
        return ejected + ejectVelocity * elapsed + turnAcceleration * (.5f * elapsed * elapsed);
    }
    const Vec3 raised = ejected + ejectVelocity * kalibrTurnTime + turnAcceleration * (.5f * kalibrTurnTime * kalibrTurnTime);
    const Vec3 turnVelocity = ejectVelocity + turnAcceleration * kalibrTurnTime;
    const float elapsed = elapsedFlight - kalibrEjectTime - kalibrTurnTime;
    const float turn = elapsed / .6f;
    const float momentum = elapsed / (1.f + turn*turn) * (1.f-travel);
    return raised + (target-raised)*travel + turnVelocity*momentum + Vec3{0.f, altitude*smoothLift*smoothDive, 0.f};
}

// Length is along +z. Ring radius and fin positions are model-relative;
// renderer transforms these triangles into the missile's live orientation.
struct Vertex { Vec3 position; uint32_t color; };
constexpr std::array<uint32_t, 12> materialShades(uint32_t color)
{
    constexpr std::array<unsigned, 12> light{230, 248, 255, 247, 224, 195, 170, 155, 162, 181, 202, 220};
    std::array<uint32_t, 12> shades{};
    for (unsigned i = 0; i < shades.size(); ++i) {
        const auto channel = [&](unsigned shift) { return (((color >> shift) & 255u) * light[i] / 255u) << shift; };
        shades[i] = 0xff000000u | channel(16) | channel(8) | channel(0);
    }
    return shades;
}
template<class Emit> inline void geometry(Type type, Emit emit)
{
    constexpr unsigned sides = 12;
    constexpr float tau = 6.283185307179586f;
    const bool cruise = type == Type::Kalibr;
    const bool heavy = type == Type::Shtorm;
    const bool ballistic = type == Type::Oreshnik;
    // Stylized ballistic carrier, not a scale reconstruction. Keep the tail
    // at -5 so the existing motor emitter remains attached to the model.
    const std::array<float, 6> z = ballistic ? std::array<float,6>{-5.f,-4.5f,5.f,9.f,12.5f,15.f}
                                  : heavy ? std::array<float,6>{-6.f,-5.2f,-.5f,3.f,5.2f,6.f}
                                  : cruise ? std::array<float,6>{-4.f,-3.4f,-.4f,2.2f,4.f,4.5f}
                                          : std::array<float,6>{-5.f,-4.3f,-.4f,2.4f,4.5f,5.3f};
    const std::array<float, 6> radius = ballistic ? std::array<float,6>{1.2f,1.55f,1.55f,1.48f,.9f,0.f}
                                       : heavy ? std::array<float,6>{.7f,1.05f,1.05f,1.05f,.62f,0.f}
                                       : cruise ? std::array<float,6>{.42f,.6f,.6f,.6f,.28f,0.f}
                                               : std::array<float,6>{.46f,.72f,.72f,.72f,.38f,0.f};
    static constexpr auto tail = materialShades(0xff667072u);
    static constexpr auto kinzhalBody = materialShades(0xff737c78u);
    static constexpr auto kinzhalAft = materialShades(0xff59645fu);
    static constexpr auto kinzhalNose = materialShades(0xff4b5651u);
    static constexpr auto kalibrBody = materialShades(0xff747e6cu);
    static constexpr auto kalibrAft = materialShades(0xff626c5cu);
    static constexpr auto kalibrNose = materialShades(0xff515d4du);
    static constexpr auto shtormBody = materialShades(0xff485765u);
    static constexpr auto shtormAft = materialShades(0xff354351u);
    static constexpr auto shtormNose = materialShades(0xff253543u);
    static constexpr auto ballisticBody = materialShades(0xff777d6bu);
    static constexpr auto ballisticAft = materialShades(0xff5c6455u);
    static constexpr auto ballisticNose = materialShades(0xff60695du);
    const auto point = [&](unsigned ring, unsigned side) {
        const float a = tau * float(side % sides) / float(sides);
        return Vec3{radius[ring]*std::cos(a), radius[ring]*std::sin(a), z[ring]};
    };
    for (unsigned ring=0; ring+1<z.size(); ++ring)
        for (unsigned side=0; side<sides; ++side) {
            const auto &shades = ring == 0 ? tail : ring >= 3 ?
                (ballistic ? ballisticNose : heavy ? shtormNose : cruise ? kalibrNose : kinzhalNose) : ring == 1 ?
                (ballistic ? ballisticAft : heavy ? shtormAft : cruise ? kalibrAft : kinzhalAft) : (ballistic ? ballisticBody : heavy ? shtormBody : cruise ? kalibrBody : kinzhalBody);
            const uint32_t nearColor = shades[side], nextColor = shades[(side+1)%sides];
            const Vertex a{point(ring,side),nearColor}, b{point(ring,side+1),nextColor};
            const Vertex c{point(ring+1,side+1),nextColor}, d{point(ring+1,side),nearColor};
            emit(a,b,c); emit(a,c,d);
        }
    for (unsigned fin=0; fin<(ballistic ? 0u : 4u); ++fin) {
        const float a = tau * float(fin) / 4.f;
        const Vec3 radial{std::cos(a),std::sin(a),0.f};
        const float root = heavy ? .96f : cruise ? .54f : .65f;
        const float span = heavy ? 1.85f : cruise ? 1.65f : 1.25f;
        const float forward = heavy ? -3.1f : cruise ? -.2f : -2.7f;
        const Vertex a0{radial*root+Vec3{0.f,0.f,forward},heavy ? 0xff697c89u : 0xff8e9996u};
        const Vertex b0{radial*root+Vec3{0.f,0.f,heavy ? -5.2f : -3.9f},heavy ? 0xff465967u : 0xff6c7674u};
        const Vertex c0{radial*span+Vec3{0.f,0.f,heavy ? -4.9f : -3.5f},heavy ? 0xff91a6b1u : 0xffbec6bfu};
        emit(a0,b0,c0); emit(c0,b0,a0);
    }
}

}

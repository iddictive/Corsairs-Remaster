#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Arcade-scale flight and geometry for the Phonk ship. These profiles are
// deliberately independent of warhead and impact handling.
namespace storm::ship::phonk::missile
{
enum class Type : uint32_t { Kinzhal = 0, Kalibr = 1, Shtorm = 2 };

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
    return type == Type::Shtorm ? Profile{700.f, 2.f, 1000.f, .25f}
         : type == Type::Kalibr ? Profile{260.f, 2.f, 42.f, .035f}
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
inline float rampTime(Type type) { return type == Type::Shtorm ? 3.5f : type == Type::Kalibr ? 3.f : 2.f; }
inline float initialSpeed(Type type) { return type == Type::Shtorm ? .05f : type == Type::Kalibr ? 0.f : .1f; }
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
inline constexpr float kalibrEjectTime = 1.2f;
inline constexpr float kalibrEjectHeight = 36.f;
// Fictional game units: sqrt(2 * acceleration * 3000) == 520.
inline constexpr float kalibrAcceleration = 520.f * 520.f / 6000.f;
inline float variedKalibrAcceleration(FlightVariation v)
{
    return kalibrAcceleration * flightScale(v.speedScale) / flightScale(v.rampScale) * settingScale(v.configuredSpeed) * settingScale(v.configuredAcceleration);
}
inline float duration(Type type, Vec3 start, Vec3 target, FlightVariation variation = {})
{
    const auto d = target - start;
    if (type == Type::Kalibr) {
        const float horizontalDistance = std::sqrt(d.x*d.x + d.z*d.z);
        // A vertical-only target still gets a finite powered phase.
        const float distance = horizontalDistance > 0.f ? horizontalDistance : std::abs(d.y);
        return kalibrEjectTime + std::max(.1f, std::sqrt(2.f * distance / variedKalibrAcceleration(variation)));
    }
    const auto p = profile(type);
    const float distance = magnitude(d);
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
    return std::min(kalibrEjectTime / std::max(.1f, flightDuration), .999999f);
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
        const float poweredFraction = elapsed / poweredTime;
        return poweredFraction * poweredFraction;
    }
    return travelTime(elapsed, variedRamp(type, variation), variedInitial(type, variation)) /
           travelTime(poweredTime, variedRamp(type, variation), variedInitial(type, variation));
}
inline Vec3 position(Type type, Vec3 start, Vec3 target, float t, float flightDuration = 0.f, FlightVariation variation = {})
{
    t = std::clamp(t, 0.f, 1.f);
    const auto d = target - start;
    const auto p = profile(type);
    const float distance = std::sqrt(d.x*d.x+d.z*d.z);
    const float variationScale = std::isfinite(variation.arcScale) ? std::clamp(variation.arcScale, .3f, 1.65f) : 1.f;
    const float altitude = std::min(p.maxAltitude, distance * p.altitudeFraction) *
        (type == Type::Kinzhal ? variationScale : type == Type::Kalibr ?
            .35f + 1.05f * (variationScale - .3f) / 1.35f : .9f + .2f * (variationScale - .3f) / 1.35f);
    const float total = flightDuration > 0.f ? flightDuration : duration(type, start, target, variation);
    const float travel = trajectoryProgress(type, t, total, variation);
    if (type != Type::Kalibr) {
        float arcProgress = travel;
        if (type == Type::Kinzhal && variation.arcPattern == 1) arcProgress = travel * travel * travel;
        if (type == Type::Kinzhal && variation.arcPattern == 2) arcProgress = 1.f - (1.f - travel) * (1.f - travel);
        return start + d*travel + Vec3{0.f, 4.f*altitude*arcProgress*(1.f-arcProgress), 0.f};
    }
    // Vertical ejection precedes ignition and the cruise turn.
    const float ejectEnd = ignitionFraction(total);
    if (t < ejectEnd) {
        const float u = t / ejectEnd;
        // Launch ejection starts with upward momentum instead of appearing
        // stationary over the deck before it begins to climb.
        return start + Vec3{0.f, kalibrEjectHeight * (.8f*u + .2f*u*u), 0.f};
    }
    const float lift = std::clamp(travel / (.11f + .035f * float(variation.arcPattern % 3u)), 0.f, 1.f);
    const float dive = std::clamp((1.f-travel) / (.15f + .035f * float(variation.arcPattern % 3u)), 0.f, 1.f);
    const float smoothLift = lift*lift*(3.f-2.f*lift);
    const float smoothDive = dive*dive*(3.f-2.f*dive);
    const Vec3 raised = start + Vec3{0.f,kalibrEjectHeight,0.f};
    // Carry ejection velocity into the powered turn. The correction vanishes
    // at the target while the forward motion continues accelerating.
    const float elapsed = total * (t-ejectEnd);
    const float ejectVelocity = 1.2f * kalibrEjectHeight / (total * ejectEnd);
    const float turn = elapsed / 2.f;
    const float momentum = ejectVelocity * elapsed / (1.f + turn*turn) * (1.f-travel);
    return raised + (target-raised)*travel + Vec3{0.f, altitude*smoothLift*smoothDive + momentum, 0.f};
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
    const std::array<float, 6> z = heavy ? std::array<float,6>{-6.f,-5.2f,-.5f,3.f,5.2f,6.f}
                                  : cruise ? std::array<float,6>{-4.f,-3.4f,-.4f,2.2f,4.f,4.5f}
                                          : std::array<float,6>{-5.f,-4.3f,-.4f,2.4f,4.5f,5.3f};
    const std::array<float, 6> radius = heavy ? std::array<float,6>{.7f,1.05f,1.05f,1.05f,.62f,0.f}
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
    const auto point = [&](unsigned ring, unsigned side) {
        const float a = tau * float(side % sides) / float(sides);
        return Vec3{radius[ring]*std::cos(a), radius[ring]*std::sin(a), z[ring]};
    };
    for (unsigned ring=0; ring+1<z.size(); ++ring)
        for (unsigned side=0; side<sides; ++side) {
            const auto &shades = ring == 0 ? tail : ring >= 3 ?
                (heavy ? shtormNose : cruise ? kalibrNose : kinzhalNose) : ring == 1 ?
                (heavy ? shtormAft : cruise ? kalibrAft : kinzhalAft) : (heavy ? shtormBody : cruise ? kalibrBody : kinzhalBody);
            const uint32_t nearColor = shades[side], nextColor = shades[(side+1)%sides];
            const Vertex a{point(ring,side),nearColor}, b{point(ring,side+1),nextColor};
            const Vertex c{point(ring+1,side+1),nextColor}, d{point(ring+1,side),nearColor};
            emit(a,b,c); emit(a,c,d);
        }
    for (unsigned fin=0; fin<4; ++fin) {
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

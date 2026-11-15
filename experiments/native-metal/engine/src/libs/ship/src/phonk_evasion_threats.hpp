#pragma once

#include "phonk_missile_profiles.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

// Selection only: the caller owns observation, flight integration, hostility,
// manual control, collision resolution, and the total acceleration limit.
namespace storm::ship::phonk::evasion
{
using Vec3 = missile::Vec3;

struct ThreatId {
    uint64_t owner = 0, generation = 0;
    uint32_t serial = 0;
    bool fort = false, test = false;
    auto key() const { return std::tie(owner, generation, serial, fort, test); }
    bool operator<(const ThreatId &other) const { return key() < other.key(); }
    bool operator==(const ThreatId &other) const { return key() == other.key(); }
};
struct Threat { ThreatId id; Vec3 position{}, velocity{}; float radius = 0.f; };
struct Reaction { ThreatId id; double detectedAt = 0., delay = 0.; };
// Value-owned: planning copies do not mutate live missile state.
struct State { std::vector<Reaction> reactions; };
struct Input {
    double time = 0.; // monotonic simulation seconds, same instant as all positions
    Vec3 position{}, velocity{};
    float acceleration = 0.f; // candidate lateral acceleration, world units/s²
    float waterClearance = 0.f; // position.y minus current local water height
    float waterReserve = 0.f; // clearance the caller wishes to retain
    uint64_t missileSeed = 0; // stable identity-derived seed for reaction variation
};
struct Result { Vec3 direction{}; unsigned observed = 0, ready = 0; };

namespace detail
{
inline double dot(Vec3 a, Vec3 b) { return double(a.x)*b.x + double(a.y)*b.y + double(a.z)*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline Vec3 unit(Vec3 v) { const double n = std::sqrt(dot(v,v)); return n > 1.e-9 ? v * float(1./n) : Vec3{}; }
inline bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
inline uint64_t mix(uint64_t x) {
    x ^= x >> 30; x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27; x *= UINT64_C(0x94d049bb133111eb); return x ^ (x >> 31);
}
inline double delay(ThreatId id, uint64_t missileSeed) {
    uint64_t x = mix(missileSeed ^ mix(id.owner) ^ mix(id.generation) ^
                     mix(uint64_t(id.serial) | (uint64_t(id.fort) << 32) | (uint64_t(id.test) << 33)));
    return .08 + .17 * double(x & 0xffffffu) / double(0xffffffu);
}
struct Observation { ThreatId id; Vec3 relative, relativeVelocity; double closest, radius; };
}

// `threats` contains live, hostile interceptors, including ones assigned to
// other missiles. A given identity must occur at most once. Invalid input leaves
// state untouched; an empty valid snapshot retires all observations.
// Reaction starts on the first sampled frame within radius + closing*(2*delay+.15).
// Existing observations remain active while approaching even outside that zone.
inline Result select(State &state, const Input &in, std::span<const Threat> threats)
{
    using namespace detail;
    Result result;
    if (!std::isfinite(in.time) || !finite(in.position) || !finite(in.velocity) ||
        !std::isfinite(in.acceleration) || in.acceleration <= 0.f ||
        !std::isfinite(in.waterClearance) || !std::isfinite(in.waterReserve)) return result;

    std::vector<Threat> ordered(threats.begin(), threats.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) { return a.id < b.id; });
    std::vector<Reaction> next;
    std::vector<Observation> ready;
    for (const auto &threat : ordered) {
        if (!next.empty() && next.back().id == threat.id) continue;
        if (!finite(threat.position) || !finite(threat.velocity) || !std::isfinite(threat.radius) || threat.radius < 0.f) continue;
        const Vec3 r = threat.position - in.position, v = threat.velocity - in.velocity;
        const double distance2 = dot(r,r), speed2 = dot(v,v), rv = dot(r,v);
        if (!(distance2 > 1.e-12) || !(speed2 > 1.e-12) || !(rv < 0.)) continue;
        const double distance = std::sqrt(distance2), closing = -rv / distance;
        const auto old = std::find_if(state.reactions.begin(), state.reactions.end(),
                                      [&](const auto &x) { return x.id == threat.id; });
        const double reactionDelay = old == state.reactions.end() ? delay(threat.id, in.missileSeed) : old->delay;
        if (old == state.reactions.end() && distance > double(threat.radius) + closing * (2. * reactionDelay + .15)) continue;
        const Reaction reaction = old == state.reactions.end() ? Reaction{threat.id, in.time, reactionDelay} : *old;
        next.push_back(reaction);
        if (in.time >= reaction.detectedAt + reaction.delay)
            ready.push_back({threat.id, r, v, -rv/speed2, std::max(1.e-6, double(threat.radius))});
    }
    state.reactions = std::move(next);
    result.observed = unsigned(state.reactions.size()); result.ready = unsigned(ready.size());
    if (ready.empty()) return result;
    // Evaluate at most the 2 most immediate threats by time-to-closest-approach
    if (ready.size() > 2) {
        std::partial_sort(ready.begin(), ready.begin() + 2, ready.end(),
            [](const auto &a, const auto &b) { return a.closest < b.closest; });
        ready.resize(2);
    }
    const Vec3 forward = unit(in.velocity);
    if (dot(forward,forward) < .5) return result;
    Vec3 side = unit(cross(forward, {0.f,1.f,0.f}));
    if (dot(side,side) < .5) side = unit(cross(forward, {1.f,0.f,0.f}));
    const Vec3 up = unit(cross(side, forward));
    // All candidates are lateral to current velocity. Score uses a constant-
    // velocity estimate from observations, not known future interceptor motion.
    double best = -1.; bool found = false;
    constexpr unsigned count = 12;
    for (unsigned i = 0; i < count; ++i) {
        const double angle = 6.283185307179586 * double(i) / count;
        const Vec3 direction = side * float(std::cos(angle)) + up * float(std::sin(angle));
        // Check only the next .15 seconds against the current water height.
        // This is a local bound, not a terrain or trajectory collision guarantee.
        const double clearance = double(in.waterClearance) + double(in.velocity.y)*.15 +
                                 .5 * double(direction.y) * in.acceleration * .15 * .15;
        if (clearance < in.waterReserve) continue;
        double score = 0.;
        for (const auto &o : ready) {
            const double displacement = .5 * in.acceleration * o.closest * o.closest;
            const double x = o.relative.x + o.relativeVelocity.x*o.closest - direction.x*displacement;
            const double y = o.relative.y + o.relativeVelocity.y*o.closest - direction.y*displacement;
            const double z = o.relative.z + o.relativeVelocity.z*o.closest - direction.z*displacement;
            const double normalized = (x*x+y*y+z*z)/(o.radius*o.radius);
            score += 1. / (1. + normalized);
        }
        if (!found || score < best) { best = score; result.direction = direction; found = true; }
    }
    return result;
}
}

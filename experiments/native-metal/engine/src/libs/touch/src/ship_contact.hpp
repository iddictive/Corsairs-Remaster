#pragma once
#ifndef STORM_SHIP_CONTACT_HPP
#define STORM_SHIP_CONTACT_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

// Planar rigid hull queries. Yaw follows Storm's RotateAroundY convention:
// positive yaw turns local +z toward world +x.
namespace storm::ship::contact
{
struct Vec2
{
    double x{}, z{};
    Vec2 operator+(Vec2 b) const { return {x + b.x, z + b.z}; }
    Vec2 operator-(Vec2 b) const { return {x - b.x, z - b.z}; }
    Vec2 operator-() const { return {-x, -z}; }
    Vec2 operator*(double s) const { return {x * s, z * s}; }
    Vec2 operator/(double s) const { return {x / s, z / s}; }
    Vec2 &operator+=(Vec2 b) { x += b.x; z += b.z; return *this; }
};
inline Vec2 operator*(double s, Vec2 v) { return v * s; }
inline double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.z * b.z; }
inline double cross(Vec2 a, Vec2 b) { return a.x * b.z - a.z * b.x; }
inline double length(Vec2 v) { return std::hypot(v.x, v.z); }

struct Pose { Vec2 position{}; double yaw{}; };
struct Body
{
    std::vector<Vec2> hull; // local hull; unordered/duplicate points are convexified
    double mass{};
    Pose start{}, end{};
};
struct Contact
{
    bool hit{};
    double time{};
    Vec2 point{}, normal{}; // normal points from A toward B
    double penetration{};
    // The query stopped on the separated side within its spatial tolerance,
    // or exhausted its advancement budget. This is a safe early stop, not an
    // assertion that the surfaces already overlap.
    bool conservative{};
    double separation{}; // signed SAT gap; positive means still separated
};
struct Response
{
    Vec2 deltaVA{}, deltaVB{}; // world metres/second
    double deltaWA{}, deltaWB{}; // Storm yaw radians/second
    double normalClosing{}, energyLost{}, effectiveMass{};
    double energyLostA{}, energyLostB{}, specificEnergyA{}, specificEnergyB{};
    // Kinetic loss for a fraction s of this same equal/opposite impulse:
    // s * workLinear - .5 * s*s * workQuadratic.
    double workLinear{}, workQuadratic{};
};

inline constexpr double restitution = 0.08;
inline constexpr double friction = 0.30;

namespace detail
{
inline constexpr std::size_t maxHullPoints = 128;
inline constexpr int maxAdvances = 64;
inline bool finite(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.z); }
inline bool finite(Pose p) { return finite(p.position) && std::isfinite(p.yaw); }
// SHIP::Move/TouchMove expose unwrapped yaw. Preserve complete turns on long
// frames; shortest-angle interpolation would silently discard their sweep.
inline double yawTravel(const Body &b) { return b.end.yaw - b.start.yaw; }
inline Vec2 rotate(Vec2 v, double a)
{
    const double c = std::cos(a), s = std::sin(a);
    return {c * v.x + s * v.z, c * v.z - s * v.x};
}
inline Pose poseAt(const Body &b, double t)
{
    return {b.start.position + (b.end.position - b.start.position) * t,
            b.start.yaw + yawTravel(b) * t};
}
struct Hull
{
    std::vector<Vec2> points;
    double radius{}, inertia{};
};
inline double extentRadius(const std::vector<Vec2> &points)
{
    if (points.empty()) return 0;
    Vec2 lo = points.front(), hi = lo;
    for (auto p : points)
    {
        if (!finite(p)) return std::numeric_limits<double>::infinity();
        lo.x = std::min(lo.x, p.x); lo.z = std::min(lo.z, p.z);
        hi.x = std::max(hi.x, p.x); hi.z = std::max(hi.z, p.z);
    }
    return std::hypot((hi.x - lo.x) * .5, (hi.z - lo.z) * .5);
}
inline std::vector<Vec2> convexHull(std::vector<Vec2> p)
{
    // Orientation precision belongs to the material's extent, not its
    // distance from the declared ship/body origin.
    const double radius = extentRadius(p);
    if (!std::isfinite(radius) || radius <= 0) return {};
    std::sort(p.begin(), p.end(), [](Vec2 a, Vec2 c) { return a.x < c.x || (a.x == c.x && a.z < c.z); });
    p.erase(std::unique(p.begin(), p.end(), [](Vec2 a, Vec2 c) { return a.x == c.x && a.z == c.z; }), p.end());
    if (p.size() < 3) return {};
    // Monotone-chain hull also removes zero-length and collinear edges.
    std::vector<Vec2> chain(2 * p.size());
    std::size_t n = 0;
    const double areaEpsilon = 1e-14 * radius * radius;
    if (!std::isfinite(areaEpsilon)) return {};
    for (auto v : p)
    {
        while (n >= 2 && cross(chain[n - 1] - chain[n - 2], v - chain[n - 1]) <= areaEpsilon) --n;
        chain[n++] = v;
    }
    const std::size_t lower = n + 1;
    for (std::size_t i = p.size() - 1; i-- > 0;)
    {
        const auto v = p[i];
        while (n >= lower && cross(chain[n - 1] - chain[n - 2], v - chain[n - 1]) <= areaEpsilon) --n;
        chain[n++] = v;
    }
    if (n < 4) return {};
    chain.resize(n - 1);
    return chain;
}
inline Hull prepare(const Body &b)
{
    Hull h;
    if (b.hull.size() < 3 || b.hull.size() > maxHullPoints ||
        !std::isfinite(b.mass) || b.mass <= 0 || !finite(b.start) || !finite(b.end) ||
        !finite(b.end.position - b.start.position) || !std::isfinite(b.end.yaw - b.start.yaw))
        return h;
    auto chain = convexHull(b.hull);
    if (chain.empty()) return {};
    for (auto v : b.hull) h.radius = std::max(h.radius, length(v));
    const double extent = extentRadius(chain);
    const double areaEpsilon = 1e-14 * extent * extent;
    const auto origin = chain.front();
    double area2 = 0, moment12 = 0;
    Vec2 centroid{};
    for (std::size_t i = 0; i < chain.size(); ++i)
    {
        const auto a = chain[i] - origin, c = chain[(i + 1) % chain.size()] - origin;
        const double w = cross(a, c);
        area2 += w;
        moment12 += w * (dot(a, a) + dot(a, c) + dot(c, c));
        centroid += (a + c) * w;
    }
    // Uniform lamina moment about the declared body origin, not its centroid:
    // the engine's position/motion owner is that origin.
    if (!(area2 > areaEpsilon) || !std::isfinite(moment12)) return {};
    centroid = centroid / (3 * area2);
    h.inertia = b.mass * (moment12 / (6 * area2) + 2 * dot(origin, centroid) + dot(origin, origin));
    if (!(h.inertia > 0) || !std::isfinite(h.inertia)) return {};
    h.points = std::move(chain);
    return h;
}
struct WorldHull
{
    std::array<Vec2, maxHullPoints> points{};
    std::size_t count{};
};
inline WorldHull world(const Hull &h, Pose p)
{
    WorldHull w;
    w.count = h.points.size();
    for (std::size_t i = 0; i < w.count; ++i) w.points[i] = rotate(h.points[i], p.yaw) + p.position;
    return w;
}
struct Projection { double lo{}, hi{}; };
inline Projection project(const WorldHull &h, Vec2 axis)
{
    Projection p{dot(h.points[0], axis), dot(h.points[0], axis)};
    for (std::size_t i = 1; i < h.count; ++i)
    {
        const double d = dot(h.points[i], axis);
        p.lo = std::min(p.lo, d); p.hi = std::max(p.hi, d);
    }
    return p;
}
struct Query { double separation{-std::numeric_limits<double>::infinity()}; Vec2 normal{}; };
inline Query query(const WorldHull &a, const WorldHull &b)
{
    Query q;
    // Maximum signed projection gap is a lower bound on distance. Testing all
    // edge normals detects edge crossings, containment and exact boundaries.
    for (const auto *h : {&a, &b})
        for (std::size_t i = 0; i < h->count; ++i)
        {
            const Vec2 edge = h->points[(i + 1) % h->count] - h->points[i];
            const double l = length(edge);
            if (!(l > 0)) continue;
            const Vec2 axis{-edge.z / l, edge.x / l};
            const auto pa = project(a, axis), pb = project(b, axis);
            const double forward = pb.lo - pa.hi, backward = pa.lo - pb.hi;
            const double gap = std::max(forward, backward);
            if (gap > q.separation)
            {
                q.separation = gap;
                q.normal = forward >= backward ? axis : -axis;
            }
        }
    return q;
}
inline Vec2 contactPoint(const WorldHull &a, const WorldHull &b, Vec2 n, double tolerance)
{
    const Vec2 tangent{-n.z, n.x};
    const auto pa = project(a, n), pb = project(b, n);
    double alo = std::numeric_limits<double>::infinity(), ahi = -alo;
    double blo = alo, bhi = -alo;
    for (std::size_t i = 0; i < a.count; ++i)
        if (pa.hi - dot(a.points[i], n) <= tolerance)
        { const double t = dot(a.points[i], tangent); alo = std::min(alo, t); ahi = std::max(ahi, t); }
    for (std::size_t i = 0; i < b.count; ++i)
        if (dot(b.points[i], n) - pb.lo <= tolerance)
        { const double t = dot(b.points[i], tangent); blo = std::min(blo, t); bhi = std::max(bhi, t); }
    const double lo = std::max(alo, blo), hi = std::min(ahi, bhi);
    // Face/face contacts use the centre of their common span; a corner/face
    // contact uses that corner. For separated features use nearest endpoints.
    const double t = lo <= hi ? (lo + hi) * 0.5 :
        (ahi < blo ? (ahi + blo) * 0.5 : (bhi + alo) * 0.5);
    return n * ((pa.hi + pb.lo) * 0.5) + tangent * t;
}
inline Vec2 pointVelocity(Vec2 linear, double angular, Vec2 lever)
{
    return linear + Vec2{angular * lever.z, -angular * lever.x};
}
inline bool sweptRadiusOverlap(Vec2 relativeStart, Vec2 relativeTravel, double radiusA, double radiusB)
{
    const double travel2 = dot(relativeTravel, relativeTravel);
    if (!finite(relativeStart) || !std::isfinite(travel2)) return false;
    const double closestTime = travel2 > 0 ? std::clamp(-dot(relativeStart, relativeTravel) / travel2, 0.0, 1.0) : 0;
    const double tolerance = 1e-7 * std::max(1.0, radiusA + radiusB);
    return !(length(relativeStart + relativeTravel * closestTime) > radiusA + radiusB + tolerance);
}
} // namespace detail

inline Contact collidePrepared(const Body &a, const Body &b, const detail::Hull &ha, const detail::Hull &hb)
{
    if (ha.points.empty() || hb.points.empty()) return {};
    const Vec2 relativeStart = b.start.position - a.start.position;
    const Vec2 relativeTravel = (b.end.position - b.start.position) - (a.end.position - a.start.position);
    if (!detail::sweptRadiusOverlap(relativeStart, relativeTravel, ha.radius, hb.radius)) return {};
    const double tolerance = 1e-7 * std::max(1.0, ha.radius + hb.radius);
    const double angularTravel = std::abs(detail::yawTravel(a)) * ha.radius + std::abs(detail::yawTravel(b)) * hb.radius;
    double time = 0;
    for (int iteration = 0; iteration <= detail::maxAdvances; ++iteration)
    {
        const auto wa = detail::world(ha, detail::poseAt(a, time));
        const auto wb = detail::world(hb, detail::poseAt(b, time));
        const auto q = detail::query(wa, wb);
        if (!std::isfinite(q.separation) || !detail::finite(q.normal)) return {};
        if (q.separation <= tolerance || iteration == detail::maxAdvances)
            return {true, time, detail::contactPoint(wa, wb, q.normal, tolerance * 4), q.normal,
                    std::max(0.0, -q.separation), q.separation > 0, q.separation};
        // For this fixed separating axis, translation closes at an exact rate;
        // each rotation contributes at most radius * angular travel. Thus this
        // step cannot cross the first contact, including at extreme speeds.
        const double closingBound = std::max(0.0, -dot(relativeTravel, q.normal)) + angularTravel;
        if (!(closingBound > 0) || !std::isfinite(closingBound) ||
            q.separation > closingBound * (1 - time) + tolerance) return {};
        const double step = 0.95 * q.separation / closingBound;
        if (!(step > 0) || time + step == time)
            return {true, time, detail::contactPoint(wa, wb, q.normal, tolerance * 4), q.normal, 0, true, q.separation};
        time = std::min(1.0, time + step);
    }
    return {};
}

inline Contact collide(const Body &a, const Body &b)
{
    const auto ha = detail::prepare(a), hb = detail::prepare(b);
    return collidePrepared(a, b, ha, hb);
}

struct Twist
{
    Vec2 linear{};
    double angular{};
};

// A selected, inelastic unilateral constraint. Select it from the complete
// motion once, then apply the SAME linear map to its temporary contributions.
// Re-selecting from a current alone would give it a different active constraint.
struct NormalConstraint
{
    Vec2 normal{};
    double lever{}, inverseMass{}, inverseInertia{}, inverseEffectiveMass{};
    bool active{};
    Twist apply(Twist velocity) const
    {
        if (!active) return velocity;
        const double impulse = (dot(velocity.linear, normal) - lever * velocity.angular) *
            inverseEffectiveMass;
        return {velocity.linear - normal * (impulse * inverseMass),
                velocity.angular + lever * impulse * inverseInertia};
    }
};

inline NormalConstraint constrainNormal(const Body &body, const Contact &contact, Twist velocity)
{
    NormalConstraint result;
    const auto hull = detail::prepare(body);
    const double normalLength = length(contact.normal);
    if (!contact.hit || hull.points.empty() || !(normalLength > 0) ||
        !detail::finite(contact.point) || !std::isfinite(contact.time) ||
        contact.time < 0 || contact.time > 1) return result;
    result.normal = contact.normal / normalLength;
    const auto arm = contact.point - detail::poseAt(body, contact.time).position;
    result.lever = cross(arm, result.normal);
    result.inverseMass = 1 / body.mass;
    result.inverseInertia = 1 / hull.inertia;
    const double effective = result.inverseMass + result.lever * result.lever * result.inverseInertia;
    const double closing = dot(velocity.linear, result.normal) - result.lever * velocity.angular;
    if (!(effective > 0) || !std::isfinite(effective) || !std::isfinite(closing) || closing <= 0)
        return {};
    result.inverseEffectiveMass = 1 / effective;
    result.active = true;
    return result;
}

inline Response resolvePrepared(const Body &a, const Body &b, const detail::Hull &ha,
                                const detail::Hull &hb, const Contact &contact, double seconds, bool fixedB = false)
{
    Response out;
    if (!contact.hit || !std::isfinite(seconds) || seconds <= 0 ||
        !std::isfinite(contact.time) || contact.time < 0 || contact.time > 1 ||
        !detail::finite(contact.point) || !detail::finite(contact.normal)) return out;
    const double normalLength = length(contact.normal);
    if (ha.points.empty() || (!fixedB && hb.points.empty()) || !(normalLength > 0)) return out;
    const Vec2 n = contact.normal / normalLength, t{-n.z, n.x};
    const Vec2 ra = contact.point - detail::poseAt(a, contact.time).position;
    const Vec2 rb = fixedB ? Vec2{} : contact.point - detail::poseAt(b, contact.time).position;
    const Vec2 va = (a.end.position - a.start.position) / seconds;
    const Vec2 vb = fixedB ? Vec2{} : (b.end.position - b.start.position) / seconds;
    const double wa = detail::yawTravel(a) / seconds, wb = fixedB ? 0 : detail::yawTravel(b) / seconds;
    const Vec2 relative = detail::pointVelocity(va, wa, ra) - detail::pointVelocity(vb, wb, rb);
    const double closing = dot(relative, n);
    if (!detail::finite(relative) || !std::isfinite(closing) || closing <= 0) return out;
    const double invMA = 1 / a.mass, invMB = fixedB ? 0 : 1 / b.mass;
    const double invIA = 1 / ha.inertia, invIB = fixedB ? 0 : 1 / hb.inertia;
    const double an = cross(ra, n), bn = cross(rb, n);
    const double knA = invMA + an * an * invIA, knB = invMB + bn * bn * invIB;
    const double kn = knA + knB;
    if (!(kn > 0) || !std::isfinite(kn)) return out;
    const double at = cross(ra, t), bt = cross(rb, t);
    const double ktA = invMA + at * at * invIA, ktB = invMB + bt * bt * invIB, kt = ktA + ktB;
    if (!(kt > 0) || !std::isfinite(kt)) return {};
    const double knt = an * at * invIA + bn * bt * invIB;
    const double tangentSpeed = dot(relative, t), normalChange = (1 + restitution) * closing;
    const double determinant = kn * kt - knt * knt;
    double jn = normalChange / kn, jt = 0;
    if (determinant > 1e-12 * kn * kt)
    {
        // Solve normal restitution and sticking together. A sequential tangent
        // impulse can otherwise push a corner back through the other hull.
        const double stickN = (normalChange * kt - tangentSpeed * knt) / determinant;
        const double stickT = (tangentSpeed * kn - normalChange * knt) / determinant;
        if (stickN > 0 && std::abs(stickT) <= friction * stickN)
        { jn = stickN; jt = stickT; }
        else
        {
            const double tangentAfterNormal = tangentSpeed - knt * jn;
            const double sign = tangentAfterNormal >= 0 ? 1.0 : -1.0;
            const double slidingMass = kn + sign * friction * knt;
            if (slidingMass > 1e-12 * kn)
            { jn = normalChange / slidingMass; jt = sign * friction * jn; }
        }
    }
    auto dissipated = [&] {
        return jn * closing + jt * tangentSpeed -
               0.5 * (kn * jn * jn + 2 * knt * jn * jt + kt * jt * jt);
    };
    double loss = dissipated();
    // Pathological ill-conditioned lever arms must never inject energy. The
    // frictionless restitution solution remains finite and conservative.
    if (!(loss >= 0))
    { jn = normalChange / kn; jt = 0; loss = dissipated(); }
    const Vec2 impulse = n * jn + t * jt;
    out.deltaVA = -impulse * invMA; out.deltaVB = impulse * invMB;
    out.deltaWA = cross(ra, impulse) * invIA; out.deltaWB = -cross(rb, impulse) * invIB;
    out.normalClosing = closing; out.effectiveMass = 1 / kn;
    const double complianceA = invMA * dot(impulse, impulse) + std::pow(cross(ra, impulse), 2) * invIA;
    const double complianceB = invMB * dot(impulse, impulse) + std::pow(cross(rb, impulse), 2) * invIB;
    out.energyLostA = std::max(0.0, loss) * complianceA / (complianceA + complianceB);
    out.energyLostB = std::max(0.0, loss) * complianceB / (complianceA + complianceB);
    out.energyLost = out.energyLostA + out.energyLostB;
    out.workLinear = jn * closing + jt * tangentSpeed;
    out.workQuadratic = kn * jn * jn + 2 * knt * jn * jt + kt * jt * jt;
    out.specificEnergyA = out.energyLostA / a.mass; out.specificEnergyB = fixedB ? 0 : out.energyLostB / b.mass;
    if (!detail::finite(out.deltaVA) || !detail::finite(out.deltaVB) || !std::isfinite(out.deltaWA) ||
        !std::isfinite(out.deltaWB) || !std::isfinite(out.energyLost) || !std::isfinite(out.specificEnergyA) ||
        !std::isfinite(out.specificEnergyB)) return {};
    return out;
}

inline Response resolve(const Body &a, const Body &b, const Contact &contact, double seconds, bool fixedB = false)
{
    const auto ha = detail::prepare(a), hb = fixedB ? detail::Hull{} : detail::prepare(b);
    return resolvePrepared(a, b, ha, hb, contact, seconds, fixedB);
}

// A successful physical fracture spends finite work instead of first stopping
// both complete hulls. Scaling the SAME impulse preserves linear/angular
// momentum; its smaller quadratic root cannot add kinetic energy.
inline Response limitWork(Response out, double work)
{
    if (!std::isfinite(work) || work < 0 || !(out.energyLost > 0) || work >= out.energyLost) return out;
    const double a = out.workLinear, b = out.workQuadratic;
    if (!(a > 0) || !(b >= 0) || !std::isfinite(a) || !std::isfinite(b)) return out;
    const double discriminant = std::max(0.0, a * a - 2 * b * work);
    const double denominator = a + std::sqrt(discriminant);
    const double scale = denominator > 0 ? std::clamp(2 * work / denominator, 0.0, 1.0) : 0;
    const double shareA = out.energyLostA / out.energyLost, shareB = out.energyLostB / out.energyLost;
    out.deltaVA = out.deltaVA * scale; out.deltaVB = out.deltaVB * scale;
    out.deltaWA *= scale; out.deltaWB *= scale;
    out.energyLostA = work * shareA; out.energyLostB = work * shareB;
    out.specificEnergyA *= work / out.energyLost; out.specificEnergyB *= work / out.energyLost;
    out.energyLost = work;
    out.workLinear *= scale; out.workQuadratic *= scale * scale;
    return out;
}
} // namespace storm::ship::contact
#endif // STORM_SHIP_CONTACT_HPP

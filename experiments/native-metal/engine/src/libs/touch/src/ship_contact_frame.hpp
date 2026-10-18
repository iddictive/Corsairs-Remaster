#pragma once

#include "ship_contact_material.hpp"
#include <functional>
#include <memory>
#include <set>
#include <tuple>

namespace storm::ship::contact
{
// This world exists for one prediction only. Native ship state remains the
// velocity/pose owner; apply each touched Motion once after this function.
struct FrameBody
{
    Body body;
    bool wreck{};
    double yStart{}, yEnd{}, height{}; // centre and full height; <=0 disables the band
    // Complete published physical snapshot, including an authoritative empty
    // set. Missing/unprepared snapshots keep the conservative hull envelope.
    std::shared_ptr<const MaterialSnapshot> material;
    double materialY{}; // model origin relative to the vertical-band centre
};
struct Motion
{
    Pose finalPose{};
    Vec2 deltaV{};
    double deltaW{};
    bool touched{};
};
struct Impact
{
    std::size_t a{}, b{};
    Vec2 point{}, normal{}; // A toward B, at the event with greatest dissipated work
    Vec2 localPointA{}, localPointB{}, localNormalA{}, localNormalB{};
    double height{}; // centre of the overlapping vertical bands at this impact
    double normalClosing{}, normalImpulse{}; // accumulated compressive impulse, mass * m/s
    double energyLostA{}, energyLostB{}, specificEnergyA{}, specificEnergyB{};
    double physicalPaidSpecificA{}, physicalPaidSpecificB{};
};
struct FrameLimits
{
    // Callers may lower these for admission/probes. Absolute caps below keep
    // an accidental oversized budget from turning a frame into an unbounded job.
    std::size_t maxEvents{256}, maxPairAdvances{64}, positionIterations{32};
    std::size_t materialCandidates{4096}, fracturePasses{3};
};
struct FractureEvent
{
    std::size_t a{}, b{};
    Pose poseA{}, poseB{};
    Contact contact{};
    double height{}, elapsed{}, remaining{};
    Response proposed{};
    Vec2 velocityA{},velocityB{};
    double angularA{},angularB{};
};
struct FractureCommit
{
    // This is an already committed CPU physical edit, never queued work. Its
    // exact work cannot exceed proposed loss; rendering may follow later.
    bool complete{};
    double workA{}, workB{};
    bool replaceA{}, replaceB{};
    std::shared_ptr<const MaterialSnapshot> materialA, materialB;
    double massA{-1},massB{-1};
    std::function<void(const Response&)> applyTransferredImpulse;
};
using FractureCallback = std::function<FractureCommit(const FractureEvent &)>;
struct FrameResult
{
    std::vector<Motion> motions;
    std::vector<Impact> impacts; // at most one row per unordered pair
    bool exhausted{};
};

namespace frame_detail
{
struct Proxy
{
    Body sweep;
    detail::Hull hull;
    Pose pose{};
    Vec2 velocity{}, initialVelocity{};
    double angular{}, initialAngular{};
    bool touched{}, blocked{};
    std::shared_ptr<const MaterialSnapshot> material;
};
inline double tolerance(const Proxy &a, const Proxy &b)
{
    return 1e-7 * std::max(1.0, a.hull.radius + b.hull.radius);
}
inline void predict(Proxy &p, double seconds)
{
    p.sweep.start = p.pose;
    p.sweep.end = p.blocked ? p.pose :
        Pose{p.pose.position + p.velocity * seconds, p.pose.yaw + p.angular * seconds};
}
inline void advance(std::vector<Proxy> &proxies, double seconds)
{
    for (auto &p : proxies)
    {
        if (p.hull.points.empty() || p.blocked) continue;
        p.pose.position += p.velocity * seconds;
        p.pose.yaw += p.angular * seconds;
    }
}
// Intersect the time interval in which linear vertical bands overlap. This
// admits a later landing while rejecting an airborne planar crossing.
inline bool verticalInterval(const FrameBody &a, const FrameBody &b,
                             double elapsed, double remaining, double seconds,
                             double &lo, double &hi)
{
    lo = 0; hi = 1;
    if (!(a.height > 0 && b.height > 0) || !std::isfinite(a.height) || !std::isfinite(b.height) ||
        !std::isfinite(a.yStart) || !std::isfinite(a.yEnd) ||
        !std::isfinite(b.yStart) || !std::isfinite(b.yEnd)) return true;
    const double start = (b.yStart - a.yStart) +
                         ((b.yEnd - b.yStart) - (a.yEnd - a.yStart)) * elapsed / seconds;
    const double travel = ((b.yEnd - b.yStart) - (a.yEnd - a.yStart)) * remaining / seconds;
    const double extent = 0.5 * (a.height + b.height);
    if (travel == 0) return std::abs(start) <= extent;
    double enter = (-extent - start) / travel, leave = (extent - start) / travel;
    if (enter > leave) std::swap(enter, leave);
    lo = std::max(0.0, enter); hi = std::min(1.0, leave);
    return lo <= hi;
}
inline Body slice(const Body &b, double lo, double hi)
{
    Body out = b;
    out.start = detail::poseAt(b, lo); out.end = detail::poseAt(b, hi);
    return out;
}
// A face can start touching while one end rotates inward. Resolve at the most
// closing end of its common support span, rather than its stationary midpoint.
inline Contact closingPoint(const Proxy &a, const Proxy &b, Pose pa, Pose pb, Contact c)
{
    const double angularA = a.blocked ? 0 : a.angular, angularB = b.blocked ? 0 : b.angular;
    if (angularA == angularB) return c;
    const auto wa = detail::world(a.hull, pa), wb = detail::world(b.hull, pb);
    const Vec2 t{-c.normal.z, c.normal.x};
    const auto an = detail::project(wa, c.normal), bn = detail::project(wb, c.normal);
    const double tol = tolerance(a, b) * 4;
    double alo = std::numeric_limits<double>::infinity(), ahi = -alo, blo = alo, bhi = -alo;
    for (std::size_t i = 0; i < wa.count; ++i)
        if (an.hi - dot(wa.points[i], c.normal) <= tol)
        { const double x = dot(wa.points[i], t); alo = std::min(alo, x); ahi = std::max(ahi, x); }
    for (std::size_t i = 0; i < wb.count; ++i)
        if (dot(wb.points[i], c.normal) - bn.lo <= tol)
        { const double x = dot(wb.points[i], t); blo = std::min(blo, x); bhi = std::max(bhi, x); }
    const double lo = std::max(alo, blo), hi = std::min(ahi, bhi);
    if (lo <= hi)
        c.point = c.normal * (0.5 * (an.hi + bn.lo)) + t * (angularA > angularB ? hi : lo);
    return c;
}
// A separating plane is safe while EVERY B/A vertex difference remains
// nonnegative. It may rotate with either hull: a fixed world plane otherwise
// mistakes opposite ends of a sliding face for a rapidly closing contact.
// Each plane's quadratic lower bound is independently safe; choose the longest
// proved interval. There is no time nudge or relaxed penetration tolerance.
inline double releaseInterval(const Proxy &a, const Proxy &b, Pose pa, Pose pb, Vec2 normal,
                              double horizon)
{
    double longest = 0;
    const double slack = tolerance(a, b) * 2;
    const double angularA = a.blocked ? 0 : a.angular, angularB = b.blocked ? 0 : b.angular;
    const Vec2 velocityA = a.blocked ? Vec2{} : a.velocity, velocityB = b.blocked ? Vec2{} : b.velocity;
    const Vec2 relativePosition = pb.position - pa.position, relativeVelocity = velocityB - velocityA;
    const Vec2 normalDerivative{normal.z, -normal.x};
    for (double planeAngular : {0.0, angularA, angularB})
    {
        double safe = std::numeric_limits<double>::infinity();
        const double positionalRate = dot(relativeVelocity, normal) +
                                      planeAngular * dot(relativePosition, normalDerivative);
        const double positionalCurvature = planeAngular * planeAngular *
            (length(relativePosition) + length(relativeVelocity) * horizon) +
            2 * std::abs(planeAngular) * length(relativeVelocity);
        for (auto la : a.hull.points)
        {
            const Vec2 ra = detail::rotate(la, pa.yaw);
            for (auto lb : b.hull.points)
            {
                const Vec2 rb = detail::rotate(lb, pb.yaw);
                const double gap = dot(relativePosition + rb - ra, normal) + slack;
                const double rate = positionalRate + dot(
                    detail::pointVelocity({}, angularB - planeAngular, rb) -
                    detail::pointVelocity({}, angularA - planeAngular, ra), normal);
                const double curvature = positionalCurvature +
                    std::pow(angularA - planeAngular, 2) * length(ra) +
                    std::pow(angularB - planeAngular, 2) * length(rb);
                if (!(gap > 0)) return 0;
                double root = std::numeric_limits<double>::infinity();
                if (curvature > 0)
                {
                    const double disc = std::sqrt(rate * rate + 2 * curvature * gap);
                    // Rationalized branch avoids cancellation for closing gaps.
                    root = rate >= 0 ? (rate + disc) / curvature : 2 * gap / (disc - rate);
                }
                else if (rate < 0) root = gap / -rate;
                safe = std::min(safe, root);
            }
        }
        longest = std::max(longest, safe);
    }
    return longest;
}
struct Event
{
    bool hit{}, barrier{};
    double time{1};
    std::size_t a{}, b{};
    Contact contact{};
    Response response{};
};
inline Event nextConvexPair(const FrameBody &fa, const FrameBody &fb, const Proxy &a, const Proxy &b,
                      double elapsed, double remaining, double seconds, const FrameLimits &limits)
{
    double lo, hi;
    if ((fa.wreck && fb.wreck) || (a.blocked && b.blocked) || a.hull.points.empty() || b.hull.points.empty() ||
        !(a.sweep.mass>0)||!(b.sweep.mass>0)||
        !verticalInterval(fa, fb, elapsed, remaining, seconds, lo, hi)) return {};
    if (!detail::sweptRadiusOverlap(b.sweep.start.position - a.sweep.start.position,
            (b.sweep.end.position - b.sweep.start.position) - (a.sweep.end.position - a.sweep.start.position),
            a.hull.radius, b.hull.radius)) return {};
    const double tol = tolerance(a, b);
    for (std::size_t iteration = 0; ; ++iteration)
    {
        Body sa = slice(a.sweep, lo, hi), sb = slice(b.sweep, lo, hi);
        auto c = collide(sa, sb);
        if (!c.hit) return {};
        const double at = lo + (hi - lo) * c.time;
        const Pose pa = detail::poseAt(sa, c.time), pb = detail::poseAt(sb, c.time);
        c = closingPoint(a, b, pa, pb, c);
        // resolve needs a positive duration even for a zero-width vertical
        // overlap. Use an equivalent unit-time sweep at that instant.
        Body ra = sa, rb = sb;
        double duration = remaining * (hi - lo);
        if (!(duration > 0))
        {
            ra.start = pa; ra.end = a.blocked ? pa : Pose{pa.position + a.velocity, pa.yaw + a.angular};
            rb.start = pb; rb.end = b.blocked ? pb : Pose{pb.position + b.velocity, pb.yaw + b.angular};
            duration = 1; c.time = 0;
        }
        const auto response = c.separation <= tol ? resolve(ra, rb, c, duration) : Response{};
        c.time = at;
        if (c.separation <= tol && (c.penetration > tol || response.normalClosing > 0))
            return {true, a.blocked || b.blocked, at, 0, 0, c, response};
        const double left = remaining * (hi - at);
        const double release = releaseInterval(a, b, pa, pb, c.normal, left);
        if (release >= left || !(left > 0)) return {};
        if (iteration >= limits.maxPairAdvances || !(release > 0) || !std::isfinite(release) ||
            at + release / remaining == at)
            return {true, true, at, 0, 0, c, {}};
        lo = at + release / remaining;
    }
}
inline Event nextPair(const FrameBody &fa, const FrameBody &fb, const Proxy &a, const Proxy &b,
                      double elapsed, double remaining, double seconds, const FrameLimits &limits)
{
    const auto envelope = [&] { return nextConvexPair(fa, fb, a, b, elapsed, remaining, seconds, limits); };
    double verticalLo, verticalHi;
    if ((fa.wreck && fb.wreck) || (a.blocked && b.blocked) || a.hull.points.empty() || b.hull.points.empty() ||
        !(a.sweep.mass>0)||!(b.sweep.mass>0)||
        !verticalInterval(fa, fb, elapsed, remaining, seconds, verticalLo, verticalHi) ||
        !detail::sweptRadiusOverlap(b.sweep.start.position - a.sweep.start.position,
            (b.sweep.end.position - b.sweep.start.position) - (a.sweep.end.position - a.sweep.start.position),
            a.hull.radius, b.hull.radius)) return {};
    if (!a.material && !b.material) return envelope();
    const auto *rootA = a.material ? &materialRoot(*a.material) : nullptr;
    const auto *rootB = b.material ? &materialRoot(*b.material) : nullptr;
    if ((rootA && rootA->pieces.empty()) || (rootB && rootB->pieces.empty())) return {};
    if ((rootA && rootA->index.empty()) || (rootB && rootB->index.empty())) return envelope();
    constexpr auto leaf = std::numeric_limits<std::size_t>::max();
    const auto node = [&](const Proxy &p, const FrameBody &f, std::size_t index)
    {
        if (p.material) return materialRoot(*p.material).index[index];
        MaterialNode result; result.piece = 0;
        result.bounds = {INFINITY, -INFINITY, INFINITY, -INFINITY, -.5 * f.height, .5 * f.height, p.hull.radius};
        for (auto point : p.hull.points)
        {
            result.bounds.loX = std::min(result.bounds.loX, point.x); result.bounds.hiX = std::max(result.bounds.hiX, point.x);
            result.bounds.loZ = std::min(result.bounds.loZ, point.z); result.bounds.hiZ = std::max(result.bounds.hiZ, point.z);
        }
        return result;
    };
    const auto swept = [&](const MaterialBounds &local, const Proxy &p, const FrameBody &f)
    {
        MaterialBounds result{INFINITY, -INFINITY, INFINITY, -INFINITY, INFINITY, -INFINITY, local.radius};
        for (const auto pose : {p.sweep.start, p.sweep.end})
            for (const double x : {local.loX, local.hiX}) for (const double z : {local.loZ, local.hiZ})
            {
                const auto v = detail::rotate({x, z}, pose.yaw) + pose.position;
                result.loX = std::min(result.loX, v.x); result.hiX = std::max(result.hiX, v.x);
                result.loZ = std::min(result.loZ, v.z); result.hiZ = std::max(result.hiZ, v.z);
            }
        const double arc = local.radius * std::abs(detail::yawTravel(p.sweep));
        result.loX -= arc; result.hiX += arc; result.loZ -= arc; result.hiZ += arc;
        const double modelY = p.material ? f.materialY : 0;
        for (double at : {elapsed, elapsed + remaining})
        {
            const double center = f.yStart + (f.yEnd - f.yStart) * (at / seconds) + modelY;
            result.lowerY = std::min(result.lowerY, center + local.lowerY);
            result.upperY = std::max(result.upperY, center + local.upperY);
        }
        return result;
    };
    Event first; std::size_t visited = 0, leafPairs = 0;
    const auto part = [](const Proxy &p, const detail::Hull &h)
    {
        Proxy q; q.sweep = p.sweep; q.sweep.hull = h.points; q.hull = h;
        q.pose = p.pose; q.velocity = p.velocity; q.angular = p.angular; q.blocked = p.blocked;
        return q;
    };
    std::vector<std::pair<std::size_t, std::size_t>> pending{{0, 0}};
    std::set<std::tuple<bool, std::size_t, bool, std::size_t>> replacedPairs;
    while (!pending.empty())
    {
        if (++visited > limits.materialCandidates * 8) return envelope();
        const auto pair = pending.back(); pending.pop_back();
        const auto ca = node(a, fa, pair.first), cb = node(b, fb, pair.second);
        if ((a.material && !materialSupportRetained(*a.material, ca.supportCell)) ||
            (b.material && !materialSupportRetained(*b.material, cb.supportCell))) continue;
        const auto wa = swept(ca.bounds, a, fa), wb = swept(cb.bounds, b, fb);
        if (wa.hiX < wb.loX || wb.hiX < wa.loX || wa.hiZ < wb.loZ || wb.hiZ < wa.loZ ||
            wa.upperY < wb.lowerY || wb.upperY < wa.lowerY) continue;
        if (ca.piece == leaf || cb.piece == leaf)
        {
            const double extentA = wa.hiX - wa.loX + wa.hiZ - wa.loZ + wa.upperY - wa.lowerY;
            const double extentB = wb.hiX - wb.loX + wb.hiZ - wb.loZ + wb.upperY - wb.lowerY;
            if (ca.piece == leaf && (cb.piece != leaf || extentA >= extentB))
            { pending.push_back({ca.left, pair.second}); pending.push_back({ca.right, pair.second}); }
            else
            { pending.push_back({pair.first, cb.left}); pending.push_back({pair.first, cb.right}); }
            continue;
        }
        const MaterialPiece *originalA = rootA ? &rootA->pieces[ca.piece] : nullptr;
        const MaterialPiece *originalB = rootB ? &rootB->pieces[cb.piece] : nullptr;
        const auto *replacementA = originalA ? materialReplacement(*a.material, originalA->source) : nullptr;
        const auto *replacementB = originalB ? materialReplacement(*b.material, originalB->source) : nullptr;
        if ((replacementA && replacementA->empty()) || (replacementB && replacementB->empty())) continue;
        if (replacementA || replacementB)
        {
            const auto key = std::make_tuple(bool(replacementA), replacementA ? std::size_t(originalA->source) : ca.piece,
                                             bool(replacementB), replacementB ? std::size_t(originalB->source) : cb.piece);
            if (!replacedPairs.insert(key).second) continue;
        }
        const auto countA = replacementA ? replacementA->size() : 1, countB = replacementB ? replacementB->size() : 1;
        for (std::size_t ia = 0; ia < countA; ++ia) for (std::size_t ib = 0; ib < countB; ++ib)
        {
            const auto *pieceA = replacementA ? &(*replacementA)[ia] : originalA;
            const auto *pieceB = replacementB ? &(*replacementB)[ib] : originalB;
            if ((pieceA && !materialSupportRetained(*a.material, pieceA->supportCell)) ||
                (pieceB && !materialSupportRetained(*b.material, pieceB->supportCell))) continue;
            if (++leafPairs > limits.materialCandidates) return envelope();
            const double lowerA = pieceA ? pieceA->lowerY : ca.bounds.lowerY;
            const double upperA = pieceA ? pieceA->upperY : ca.bounds.upperY;
            const double lowerB = pieceB ? pieceB->lowerY : cb.bounds.lowerY;
            const double upperB = pieceB ? pieceB->upperY : cb.bounds.upperY;
            FrameBody pa, pb;
            pa.wreck = fa.wreck; pb.wreck = fb.wreck;
            const double offsetA = a.material ? fa.materialY : 0, offsetB = b.material ? fb.materialY : 0;
            pa.yStart = fa.yStart + offsetA + .5 * (lowerA + upperA); pa.yEnd = fa.yEnd + offsetA + .5 * (lowerA + upperA);
            pb.yStart = fb.yStart + offsetB + .5 * (lowerB + upperB); pb.yEnd = fb.yEnd + offsetB + .5 * (lowerB + upperB);
            pa.height = upperA - lowerA; pb.height = upperB - lowerB;
            const auto &ha = pieceA ? pieceA->hull : a.hull;
            const auto &hb = pieceB ? pieceB->hull : b.hull;
            auto candidate = nextConvexPair(pa, pb, part(a, ha), part(b, hb), elapsed, remaining, seconds, limits);
            if (candidate.hit && (!first.hit || candidate.time < first.time)) first = candidate;
            // A persistent contact has already supplied the earliest possible
            // witness. Later leaves cannot beat t=0 (ties keep this first leaf),
            // so do not repeat exact CCD for the rest of the overlapping timber.
            if (first.hit && first.time == 0)
            {
                first.response = resolve(a.sweep, b.sweep, first.contact, remaining > 0 ? remaining : 1.0);
                return first;
            }
        }
    }
    if (first.hit)
    {
        // Contact pieces locate the witness; the complete body's existing
        // inertia and mass still own the resulting impulse and torque.
        first.response = resolve(a.sweep, b.sweep, first.contact, remaining > 0 ? remaining : 1.0);
    }
    return first;
}
inline void project(Proxy &a, Proxy &b, const Contact &c, double extra)
{
    const double scale = std::max(a.sweep.mass, b.sweep.mass);
    const double ma = a.sweep.mass / scale, mb = b.sweep.mass / scale;
    const Vec2 shift = c.normal * (c.penetration + extra);
    a.pose.position += -shift * (mb / (ma + mb));
    b.pose.position += shift * (ma / (ma + mb));
    a.touched = b.touched = true;
}
} // namespace frame_detail

inline FrameResult integrateFrame(const std::vector<FrameBody> &bodies, double seconds,
                                  FrameLimits limits = {}, const FractureCallback &fracture = {})
{
    FrameResult out;
    out.motions.resize(bodies.size());
    for (std::size_t i = 0; i < bodies.size(); ++i) out.motions[i].finalPose = bodies[i].body.end;
    if (!(seconds > 0) || !std::isfinite(seconds)) return out;
    limits.maxEvents = std::min<std::size_t>(limits.maxEvents, 4096);
    limits.maxPairAdvances = std::min<std::size_t>(limits.maxPairAdvances, 256);
    limits.positionIterations = std::min<std::size_t>(limits.positionIterations, 128);
    limits.materialCandidates = std::min<std::size_t>(limits.materialCandidates, 16384);
    limits.fracturePasses = std::min(limits.fracturePasses, limits.maxEvents);
    std::vector<frame_detail::Proxy> proxies;
    proxies.reserve(bodies.size());
    for (const auto &b : bodies)
    {
        frame_detail::Proxy p;
        p.sweep = b.body; p.pose = b.body.start; p.hull = detail::prepare(b.body);
        p.material = b.material;
        if (!p.hull.points.empty())
        {
            p.velocity = p.initialVelocity = (b.body.end.position - b.body.start.position) / seconds;
            p.angular = p.initialAngular = detail::yawTravel(b.body) / seconds;
            if (!detail::finite(p.velocity) || !std::isfinite(p.angular)) p.hull = {};
        }
        proxies.push_back(std::move(p));
    }
    std::vector<double> greatestWork;
    std::vector<std::size_t> fracturePasses(bodies.size() * bodies.size());
    double elapsed = 0, remaining = seconds;
    std::size_t events = 0;
    std::size_t subStepIterations = 0;
    // A resolved event spends one event slot. Each barrier blocks at least
    // one new body; blocked pairs are excluded by nextPair.
    const std::size_t maxSubStepIterations = limits.maxEvents + proxies.size() + 1;
    while (remaining > 0 && subStepIterations++ < maxSubStepIterations)
    {
        for (auto &p : proxies) frame_detail::predict(p, remaining);
        frame_detail::Event event;
        for (std::size_t a = 0; a < proxies.size(); ++a)
            for (std::size_t b = a + 1; b < proxies.size(); ++b)
            {
                auto candidate = frame_detail::nextPair(bodies[a], bodies[b], proxies[a], proxies[b],
                                                        elapsed, remaining, seconds, limits);
                if (candidate.hit && (!event.hit || candidate.time < event.time))
                { event = candidate; event.a = a; event.b = b; }
            }
        if (!event.hit)
        { frame_detail::advance(proxies, remaining); elapsed = seconds; remaining = 0; break; }
        const double travel = remaining * event.time;
        frame_detail::advance(proxies, travel); elapsed += travel; remaining -= travel;
        // Stop only the unresolved component at its last proven safe time.
        // Other bodies retain their full frame; a later arrival at a blocked
        // component is conservatively stopped too, without invented work.
        if (event.barrier || events >= limits.maxEvents)
        {
            out.exhausted = true;
            proxies[event.a].blocked = proxies[event.b].blocked = true;
            bool allBlocked = true;
            for (const auto &p : proxies)
            {
                if (!p.hull.points.empty() && !p.blocked) { allBlocked = false; break; }
            }
            if (allBlocked)
            {
                remaining = 0;
                elapsed = seconds;
                break;
            }
            continue;
        }
        ++events;
        auto &a = proxies[event.a]; auto &b = proxies[event.b];
        auto r = event.response;
        const double massBeforeA=a.sweep.mass,massBeforeB=b.sweep.mass;
        double massAfterA=-1,massAfterB=-1;
        std::function<void(const Response&)> applyTransferredImpulse;
        double paidA = 0, paidB = 0;
        const auto &bodyA = bodies[event.a], &bodyB = bodies[event.b];
        const double ya = bodyA.yStart + (bodyA.yEnd - bodyA.yStart) * (elapsed / seconds);
        const double yb = bodyB.yStart + (bodyB.yEnd - bodyB.yStart) * (elapsed / seconds);
        const double height = (std::max(ya - bodyA.height * .5, yb - bodyB.height * .5) +
                               std::min(ya + bodyA.height * .5, yb + bodyB.height * .5)) * .5;
        auto &passes = fracturePasses[event.a * bodies.size() + event.b];
        if (fracture && r.energyLost > 0 && passes < limits.fracturePasses)
        {
            ++passes;
            const auto commit = fracture({event.a, event.b, a.pose, b.pose, event.contact,
                                           height, elapsed, remaining, r,a.velocity,b.velocity,a.angular,b.angular});
            if (commit.complete && std::isfinite(commit.workA) && std::isfinite(commit.workB) &&
                commit.workA >= 0 && commit.workB >= 0 &&
                commit.workA + commit.workB <= r.energyLost && (commit.replaceA || commit.replaceB) &&
                (!commit.replaceA || commit.materialA) && (!commit.replaceB || commit.materialB))
            {
                if (commit.replaceA) a.material = commit.materialA;
                if (commit.replaceB) b.material = commit.materialB;
                paidA = commit.workA; paidB = commit.workB;
                massAfterA=commit.massA;massAfterB=commit.massB;applyTransferredImpulse=commit.applyTransferredImpulse;
                r = limitWork(r, paidA + paidB);
                r.energyLostA = paidA; r.energyLostB = paidB;
                r.specificEnergyA = paidA / a.sweep.mass; r.specificEnergyB = paidB / b.sweep.mass;
                // The old witness no longer owns penetration correction. Its
                // replacement pieces are queried again at this same TOI.
                event.contact.penetration = 0;
            }
        }
        if (r.normalClosing > 0)
        {
            a.velocity += r.deltaVA; b.velocity += r.deltaVB;
            a.angular += r.deltaWA; b.angular += r.deltaWB;
            a.touched = b.touched = true;
            if (r.energyLost > 0)
            {
                std::size_t row = 0;
                while (row < out.impacts.size() &&
                       (out.impacts[row].a != event.a || out.impacts[row].b != event.b)) ++row;
                if (row == out.impacts.size())
                { out.impacts.push_back({}); greatestWork.push_back(-1); }
                auto &impact = out.impacts[row];
                if (r.energyLost > greatestWork[row])
                {
                    greatestWork[row] = r.energyLost;
                    impact.a = event.a; impact.b = event.b;
                    impact.point = event.contact.point; impact.normal = event.contact.normal;
                    impact.localPointA = detail::rotate(impact.point - a.pose.position, -a.pose.yaw);
                    impact.localPointB = detail::rotate(impact.point - b.pose.position, -b.pose.yaw);
                    impact.localNormalA = detail::rotate(impact.normal, -a.pose.yaw);
                    impact.localNormalB = detail::rotate(-impact.normal, -b.pose.yaw);
                    impact.height = height;
                }
                impact.normalClosing = std::max(impact.normalClosing, r.normalClosing);
                impact.normalImpulse += std::max(0.0, -dot(r.deltaVA, event.contact.normal) * a.sweep.mass);
                impact.energyLostA += r.energyLostA; impact.energyLostB += r.energyLostB;
                impact.physicalPaidSpecificA += paidA/massBeforeA; impact.physicalPaidSpecificB += paidB/massBeforeB;
                impact.specificEnergyA += r.specificEnergyA;
                impact.specificEnergyB += r.specificEnergyB;
            }
        }
        if(applyTransferredImpulse)applyTransferredImpulse(r);
        const auto refreshMass=[](frame_detail::Proxy&p,double mass){
            if(mass<0||!std::isfinite(mass))return;
            p.hull.inertia*=mass/p.sweep.mass;p.sweep.mass=mass;
        };
        refreshMass(a,massAfterA);refreshMass(b,massAfterB);
        if (event.contact.penetration > frame_detail::tolerance(a, b))
            frame_detail::project(a, b, event.contact, frame_detail::tolerance(a, b) * 4);
    }
    if (remaining > 0)
    {
        out.exhausted = true;
        // A defensive iteration stop has not queried the remaining sweep.
        // Keep its last safe poses rather than tunnel through later contacts.
        for (auto &p : proxies) p.blocked = true;
        elapsed = seconds;
        remaining = 0;
    }
    // Projection changes positions only. It cannot produce impact work. The
    // extra margin stops initial/crowded overlaps from consuming endless TOIs.
    bool overlap = false;
    for (std::size_t pass = 0; pass <= limits.positionIterations; ++pass)
    {
        overlap = false;
        for (std::size_t i = 0; i < proxies.size(); ++i)
            for (std::size_t j = i + 1; j < proxies.size(); ++j)
            {
                auto &a = proxies[i]; auto &b = proxies[j]; double lo, hi;
                if (a.hull.points.empty() || b.hull.points.empty() || (bodies[i].wreck && bodies[j].wreck) ||
                    !frame_detail::verticalInterval(bodies[i], bodies[j], elapsed, 0, seconds, lo, hi)) continue;
                if (!detail::sweptRadiusOverlap(b.pose.position - a.pose.position, {},
                                                a.hull.radius, b.hull.radius)) continue;
                frame_detail::predict(a, 0); frame_detail::predict(b, 0);
                const auto q = frame_detail::nextPair(bodies[i], bodies[j], a, b, seconds, 0, seconds, limits).contact;
                const double tol = frame_detail::tolerance(a, b);
                if (!q.hit || !(q.separation < -tol)) continue;
                overlap = true;
                if (pass < limits.positionIterations)
                    frame_detail::project(a, b, {true, 0, {}, q.normal, -q.separation, false, q.separation}, tol * 4);
            }
        if (!overlap) break;
    }
    out.exhausted = out.exhausted || overlap;
    for (std::size_t i = 0; i < proxies.size(); ++i)
    {
        const auto &p = proxies[i];
        if (p.hull.points.empty()) continue;
        auto &motion = out.motions[i];
        motion.touched = p.touched || (p.blocked &&
            (p.pose.position.x != bodies[i].body.end.position.x ||
             p.pose.position.z != bodies[i].body.end.position.z || p.pose.yaw != bodies[i].body.end.yaw));
        if (motion.touched)
        {
            motion.finalPose = p.pose;
            motion.deltaV = p.velocity - p.initialVelocity;
            motion.deltaW = p.angular - p.initialAngular;
        }
    }
    return out;
}
} // namespace storm::ship::contact

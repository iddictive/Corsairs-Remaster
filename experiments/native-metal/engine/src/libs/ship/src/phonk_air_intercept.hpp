#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

// Stateless, piecewise-linear air interception. No engine or SHIP dependency.
// Times share one frame clock. Segments have start < end, are contiguous or
// disjoint and do not overlap within one body; a shared endpoint is continuous.
// Positions and times must be finite, radii finite and nonnegative, and IDs
// unique within each input collection. All coordinates must be small enough
// that double arithmetic does not overflow. Caller owns input validation.
// A body exists at segment endpoints but strictly before expires (the first
// surface/split/removal event). Thus another event at expires wins a tie.
// order is the caller's deterministic event priority (lower first), then ID.
// Hostility is directional: hostile(interceptor owner, missile owner).
// Revalidate live identities and recompute after EACH applied event.
namespace storm::air_intercept
{
struct Vec3 { double x = 0, y = 0, z = 0; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3 operator*(Vec3 a, double s) { return {a.x*s,a.y*s,a.z*s}; }
inline double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }

struct Segment { double start, end; Vec3 from, to; };
struct Body
{
    uint64_t id;
    int64_t owner;
    uint64_t order;
    double radius; // contact sphere; for targets also expands splash radius
    double expires; // exclusive, may be +infinity
    std::vector<Segment> segments;
};
struct Interceptor {
    Body body; double splashRadius; bool detonateOnExpiry = false;
    double fuseDelay = 0.;
    double pendingDetonation = std::numeric_limits<double>::infinity();
    double sensorStart = 0.;
};
struct Contact
{
    double time;
    uint64_t interceptor, missile;
    uint64_t interceptorOrder, missileOrder;
    Vec3 position; // interceptor centre at contact
};
struct Result { std::optional<Contact> contact; std::vector<uint64_t> splash; };

namespace detail
{
inline Vec3 at(const Segment &s, double t)
{
    return s.from + (s.to-s.from)*((t-s.start)/(s.end-s.start));
}
// Bound roundoff from interpolation and subtracting world-space coordinates.
// The tolerance scales with coordinate magnitude, not a fixed gameplay unit.
inline bool inSplash(Vec3 center, Vec3 target, double radius)
{
    const Vec3 d=target-center;
    const double scale=std::max({std::abs(center.x),std::abs(center.y),std::abs(center.z),
                                 std::abs(target.x),std::abs(target.y),std::abs(target.z),radius});
    const double tolerance=4*std::numeric_limits<double>::epsilon()*scale;
    return std::hypot(d.x,d.y,d.z)<=radius+tolerance;
}
struct Box { double lo[3], hi[3]; };
inline Box bounds(const Segment &s, double r)
{
    return {{std::min(s.from.x,s.to.x)-r,std::min(s.from.y,s.to.y)-r,std::min(s.from.z,s.to.z)-r},
            {std::max(s.from.x,s.to.x)+r,std::max(s.from.y,s.to.y)+r,std::max(s.from.z,s.to.z)+r}};
}
inline bool overlaps(const Box &a, const Box &b)
{
    return a.lo[1]<=b.hi[1] && b.lo[1]<=a.hi[1] && a.lo[2]<=b.hi[2] && b.lo[2]<=a.hi[2];
}
inline bool contact(const Body &a, const Segment &sa, const Body &b, const Segment &sb, double &t)
{
    const double lo = std::max(sa.start,sb.start), hi = std::min(sa.end,sb.end);
    if (lo > hi || lo >= a.expires || lo >= b.expires) return false;
    const Vec3 p = at(sa,lo)-at(sb,lo);
    const Vec3 v = (sa.to-sa.from)*(1/(sa.end-sa.start))-(sb.to-sb.from)*(1/(sb.end-sb.start));
    const double radius = a.radius+b.radius;
    const double distance = std::hypot(p.x,p.y,p.z);
    // Factoring avoids a negative/positive residual from differently rounded
    // equal squared distances at an exact initial tangent.
    const double c = (distance-radius)*(distance+radius);
    double dt = 0;
    if (c > 0) {
        const double aa = dot(v,v), halfB = dot(p,v);
        if (aa == 0 || halfB >= 0) return false;
        const double discriminant = halfB*halfB-aa*c;
        if (discriminant < 0) return false;
        // Stable smaller quadratic root avoids cancellation for distant entry.
        dt = c/(-halfB+std::sqrt(discriminant));
    }
    t = lo+dt;
    return t <= hi && t < a.expires && t < b.expires;
}
struct Ref { size_t body, segment; Box box; bool interceptor; };
struct Endpoint { double x; size_t ref; bool end; };
}

// Returns the minimum (time, interceptor order, interceptor ID, missile order,
// missile ID). Splash is sorted by (missile order, ID); it includes every live hostile
// target whose sphere touches the blast sphere at that exact time. The direct
// target is always included on a confirmed contact. No contact implies empty splash.
// Dynamic allocation is unbounded except by available memory. Sweep-and-prune
// is conservative and has quadratic worst-case overlap; no persistent state.
template<class Hostile>
Result find(const std::vector<Interceptor> &interceptors, const std::vector<Body> &missiles, Hostile hostile)
{
    std::vector<detail::Ref> refs;
    auto append = [&](const Body &b, size_t i, bool isInterceptor) {
        for (size_t s=0; s<b.segments.size(); ++s)
            if (b.segments[s].start < b.expires)
                refs.push_back({i,s,detail::bounds(b.segments[s],b.radius),isInterceptor});
    };
    for (size_t i=0;i<interceptors.size();++i) append(interceptors[i].body,i,true);
    for (size_t i=0;i<missiles.size();++i) append(missiles[i],i,false);
    std::vector<detail::Endpoint> ends;
    ends.reserve(refs.size()*2);
    for (size_t i=0;i<refs.size();++i) {
        ends.push_back({refs[i].box.lo[0],i,false});
        ends.push_back({refs[i].box.hi[0],i,true});
    }
    std::sort(ends.begin(),ends.end(),[](const auto &a,const auto &b) {
        return std::tie(a.x,a.end,a.ref)<std::tie(b.x,b.end,b.ref);
    });
    std::set<size_t> active[2];
    Result result;
    size_t winner = 0;
    for (const auto &e:ends) {
        const auto &r=refs[e.ref];
        const int side=r.interceptor ? 1 : 0;
        if (e.end) { active[side].erase(e.ref); continue; }
        for (size_t j:active[1-side]) {
            const auto &other=refs[j];
            if (!detail::overlaps(r.box,other.box)) continue;
            const auto &ir = r.interceptor ? r : other;
            const auto &mr = r.interceptor ? other : r;
            const Body &ib=interceptors[ir.body].body, &mb=missiles[mr.body];
            if (result.contact && std::max(ib.segments[ir.segment].start,mb.segments[mr.segment].start)>result.contact->time)
                continue;
            if (!hostile(ib.owner,mb.owner)) continue;
            double time;
            if (!detail::contact(ib,ib.segments[ir.segment],mb,mb.segments[mr.segment],time)) continue;
            if (!result.contact ||
                std::tie(time,ib.order,ib.id,mb.order,mb.id)<
                std::tie(result.contact->time,result.contact->interceptorOrder,result.contact->interceptor,
                         result.contact->missileOrder,result.contact->missile)) {
                result.contact=Contact{time,ib.id,mb.id,ib.order,mb.order,detail::at(ib.segments[ir.segment],time)};
                winner=ir.body;
            }
        }
        active[side].insert(e.ref);
    }
    if (!result.contact) return result;
    const Contact &first=*result.contact;
    const Interceptor &selected=interceptors[winner];
    std::vector<std::pair<uint64_t,uint64_t>> splash;
    for (const Body &m:missiles) {
        if (first.time>=m.expires || !hostile(selected.body.owner,m.owner)) continue;
        if (m.id==first.missile) { splash.emplace_back(m.order,m.id); continue; }
        for (const Segment &s:m.segments) {
            if (first.time<s.start || first.time>s.end) continue;
            const double radius=selected.splashRadius+m.radius;
            if (detail::inSplash(first.position,detail::at(s,first.time),radius)) splash.emplace_back(m.order,m.id);
            break;
        }
    }
    std::sort(splash.begin(),splash.end());
    for (const auto &entry:splash) result.splash.push_back(entry.second);
    return result;
}
}

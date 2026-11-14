#pragma once

#include "phonk_air_intercept.hpp"

namespace storm::air_intercept
{
// A batch assumes immutable trajectories, ownership and hostility throughout
// the supplied interval. The caller must clip expires at earlier non-solver
// events and rerun if an accepted event changes another body's trajectory or
// hostility. The hostility predicate must be pure for the duration of resolve.
// A delayed trigger arms its interceptor without consuming a missile; the
// later detonation consumes the interceptor and geometrically selected splash.
// A zero-delay contact retains the original direct-target semantics.
// The function predicts only: the owner must revalidate identities when applying.
struct BatchEvent { Contact contact; std::vector<uint64_t> splash; bool armed = false; };

namespace batch_detail
{
struct Ref { size_t body; bool interceptor; detail::Box box; };
struct Candidate { Contact contact; size_t interceptor, missile; bool detonation = false; };

inline bool bodyBounds(const Body &body, detail::Box &out)
{
    bool present=false;
    for (const auto &s:body.segments) {
        if (s.start>=body.expires) continue;
        auto box=detail::bounds(s,body.radius);
        if (!present) { out=box; present=true; }
        else for (int axis=0;axis<3;++axis) {
            out.lo[axis]=std::min(out.lo[axis],box.lo[axis]);
            out.hi[axis]=std::max(out.hi[axis],box.hi[axis]);
        }
    }
    return present;
}
}

// One conservative body-level sweep generates each pair once; narrow phase
// retains its earliest piecewise-linear contact. Storage is proportional to
// bodies and contacting pairs; there are no fixed capacities. Dense overlaps
// can still require quadratic work and memory. No body state is modified.
template<class Hostile>
std::vector<BatchEvent> resolve(const std::vector<Interceptor> &interceptors,
                                const std::vector<Body> &missiles, Hostile hostile)
{
    std::vector<batch_detail::Ref> refs;
    auto append=[&](const Body &body,size_t index,bool interceptor) {
        detail::Box box{};
        if (batch_detail::bodyBounds(body,box)) refs.push_back({index,interceptor,box});
    };
    for (size_t i=0;i<interceptors.size();++i) append(interceptors[i].body,i,true);
    for (size_t i=0;i<missiles.size();++i) append(missiles[i],i,false);
    std::vector<detail::Endpoint> endpoints;
    endpoints.reserve(refs.size()*2);
    for (size_t i=0;i<refs.size();++i) {
        endpoints.push_back({refs[i].box.lo[0],i,false});
        endpoints.push_back({refs[i].box.hi[0],i,true});
    }
    std::sort(endpoints.begin(),endpoints.end(),[](const auto &a,const auto &b) {
        return std::tie(a.x,a.end,a.ref)<std::tie(b.x,b.end,b.ref);
    });
    std::set<size_t> active[2];
    std::vector<batch_detail::Candidate> candidates;
    for (const auto &e:endpoints) {
        const auto &r=refs[e.ref]; const int side=r.interceptor?1:0;
        if (e.end) { active[side].erase(e.ref); continue; }
        for (size_t j:active[1-side]) {
            const auto &other=refs[j];
            if (!detail::overlaps(r.box,other.box)) continue;
            const auto &ir=r.interceptor?r:other, &mr=r.interceptor?other:r;
            const Body &ib=interceptors[ir.body].body, &mb=missiles[mr.body];
            if (std::isfinite(interceptors[ir.body].pendingDetonation)) continue;
            if (!hostile(ib.owner,mb.owner)) continue;
            double best=std::numeric_limits<double>::infinity();
            Vec3 position{};
            for (const auto &a:ib.segments) for (const auto &b:mb.segments) {
                const double start=std::max(a.start,interceptors[ir.body].sensorStart);
                if (start>=a.end || std::max(start,b.start)>=best) continue;
                const Segment armed{start,a.end,detail::at(a,start),a.to};
                double time;
                if (detail::contact(ib,armed,mb,b,time) && time<best) {
                    best=time; position=detail::at(a,time);
                }
            }
            if (std::isfinite(best))
                candidates.push_back({{best,ib.id,mb.id,ib.order,mb.order,position},ir.body,mr.body});
        }
        active[side].insert(e.ref);
    }
    // An unguided interceptor also detonates at its planned endpoint. Treat
    // that as a timed event, not a check against end-of-frame positions.
    const size_t noMissile = std::numeric_limits<size_t>::max();
    for (size_t i=0;i<interceptors.size();++i) {
        const auto &it=interceptors[i];
        const double detonation = std::min(it.pendingDetonation,
            it.detonateOnExpiry ? it.body.expires : std::numeric_limits<double>::infinity());
        if (!std::isfinite(detonation)) continue;
        for (const auto &s:it.body.segments) {
            if (detonation < s.start || detonation > s.end) continue;
            candidates.push_back({{detonation,it.body.id,0,it.body.order,0,
                                   detail::at(s,detonation)},i,noMissile,true});
            break;
        }
    }
    const auto earlier=[](const auto &a,const auto &b) {
        const auto &x=a.contact, &y=b.contact;
        return std::tie(x.time,x.interceptorOrder,x.interceptor,x.missileOrder,x.missile)<
               std::tie(y.time,y.interceptorOrder,y.interceptor,y.missileOrder,y.missile);
    };
    std::sort(candidates.begin(),candidates.end(),earlier);
    std::vector<unsigned char> used(interceptors.size()), dead(missiles.size());
    std::vector<BatchEvent> result;
    for (size_t cursor=0; cursor<candidates.size(); ++cursor) {
        const auto c=candidates[cursor];
        if (used[c.interceptor] || (c.missile != noMissile && dead[c.missile])) continue;
        if (!c.detonation && c.missile==noMissile) continue;
        const auto &selected=interceptors[c.interceptor];
        if (!c.detonation && selected.fuseDelay > 0.) {
            used[c.interceptor]=1;
            result.push_back({c.contact,{},true});
            const double due=std::min(c.contact.time+selected.fuseDelay, selected.body.expires);
            for (const auto &s:selected.body.segments) {
                if (due < s.start || due > s.end) continue;
                batch_detail::Candidate next{{due,selected.body.id,0,selected.body.order,0,detail::at(s,due)},c.interceptor,noMissile,true};
                candidates.insert(std::upper_bound(candidates.begin()+cursor+1,candidates.end(),next,earlier),next);
                used[c.interceptor]=0;
                break;
            }
            // Suppress any additional contact candidates after this trigger.
            for (size_t k=cursor+1;k<candidates.size();++k)
                if (candidates[k].interceptor==c.interceptor && !candidates[k].detonation)
                    candidates[k].missile=noMissile;
            continue;
        }
        used[c.interceptor]=1;
        std::vector<std::pair<std::pair<uint64_t,uint64_t>,size_t>> hits;
        for (size_t i=0;i<missiles.size();++i) {
            const auto &m=missiles[i];
            if (dead[i] || c.contact.time>=m.expires || !hostile(selected.body.owner,m.owner)) continue;
            bool hit=!c.detonation && c.missile != noMissile && i==c.missile;
            if (!hit) for (const auto &s:m.segments) {
                if (c.contact.time<s.start || c.contact.time>s.end) continue;
                hit=detail::inSplash(c.contact.position,detail::at(s,c.contact.time),selected.splashRadius+m.radius);
                break;
            }
            if (hit) hits.push_back({{m.order,m.id},i});
        }
        std::sort(hits.begin(),hits.end());
        BatchEvent event{c.contact,{}};
        event.splash.reserve(hits.size());
        for (const auto &hit:hits) { dead[hit.second]=1; event.splash.push_back(hit.first.second); }
        result.push_back(std::move(event));
    }
    return result;
}
}

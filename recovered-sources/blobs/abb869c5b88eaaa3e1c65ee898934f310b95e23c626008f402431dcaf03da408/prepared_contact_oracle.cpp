#include "engine/src/libs/touch/src/ship_contact_frame.hpp"
#include <random>
#include <cstdio>
namespace storm::ship::contact {
inline Contact referenceCollide(const Body &a, const Body &b)
{
    const auto ha = detail::prepare(a), hb = detail::prepare(b);
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

inline Response referenceResolve(const Body &a, const Body &b, const Contact &contact, double seconds, bool fixedB = false)
{
    Response out;
    if (!contact.hit || !std::isfinite(seconds) || seconds <= 0 ||
        !std::isfinite(contact.time) || contact.time < 0 || contact.time > 1 ||
        !detail::finite(contact.point) || !detail::finite(contact.normal)) return out;
    const auto ha = detail::prepare(a), hb = fixedB ? detail::Hull{} : detail::prepare(b);
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


}

int main(){using namespace storm::ship::contact;std::mt19937_64 rng(42);std::uniform_real_distribution<double>d(-20,20),angle(-3.2,3.2),extent(.01,15);int hits=0;
 for(int n=0;n<2000;++n){Body a,b;a.mass=100;b.mass=200;const double x=extent(rng),z=extent(rng),u=extent(rng),v=extent(rng);a.hull={{-x,-z},{x,-z},{x,z},{-x,z}};b.hull={{-u,-v},{u,-v},{u,v},{-u,v}};
  a.start={{d(rng),d(rng)},angle(rng)};a.end={{d(rng),d(rng)},angle(rng)};b.start={{d(rng),d(rng)},angle(rng)};b.end={{d(rng),d(rng)},angle(rng)};
  if(n%7==0)a.end=a.start;if(n%11==0)b.end=b.start;if(n%19==0)a.mass=0;if(n%23==0)a.hull.resize(2);if(n%31==0)a.end.yaw+=12.6;
  auto old=referenceCollide(a,b);auto prepared=collidePrepared(a,b,detail::prepare(a),detail::prepare(b));
  if(old.hit!=prepared.hit||old.time!=prepared.time||old.point.x!=prepared.point.x||old.point.z!=prepared.point.z||old.normal.x!=prepared.normal.x||old.normal.z!=prepared.normal.z||old.penetration!=prepared.penetration||old.conservative!=prepared.conservative||old.separation!=prepared.separation)return 1;hits+=old.hit;
  for(bool fixed:{false,true})for(double duration:{0.,.001,1.,8.}){
   const auto reference=referenceResolve(a,b,old,duration,fixed),fast=resolvePrepared(a,b,detail::prepare(a),detail::prepare(b),old,duration,fixed);
   const auto values=[](const Response&r){return std::array<double,17>{r.deltaVA.x,r.deltaVA.z,r.deltaVB.x,r.deltaVB.z,r.deltaWA,r.deltaWB,r.normalClosing,r.energyLost,r.effectiveMass,r.energyLostA,r.energyLostB,r.specificEnergyA,r.specificEnergyB,r.workLinear,r.workQuadratic};};
   if(values(reference)!=values(fast))return 2;
   if(a.mass>0&&b.mass>0){Body ua=a,ub=b;ua.hull=detail::convexHull(ua.hull);ub.hull=detail::convexHull(ub.hull);ua.mass=ub.mass=1;auto ha=detail::prepare(ua),hb=detail::prepare(ub);ha.inertia*=a.mass;hb.inertia*=b.mass;
    Body ma=a,mb=b;ma.hull=ua.hull;mb.hull=ub.hull;const auto materialOld=referenceResolve(ma,mb,old,duration,fixed);ma.hull.clear();mb.hull.clear();
    if(values(materialOld)!=values(resolvePrepared(ma,mb,ha,hb,old,duration,fixed)))return 3;
   }
  }
 }
 std::printf("PREPARED_CCD_ORACLE cases=2000 hits=%d exact_all_fields=1 response_cases=16000 fixed_invalid_duration=1 stationary_invalid_mass_invalid_hull_full_turn=1\n",hits);
}

#pragma once
#include "c_vector.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Presentation geometry shared by the manual solution and its numerical probe.
// No engine events, random draws, damage, or saved state.
namespace manual_aim
{
inline bool finite(const CVECTOR &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
// Shared visual water contact. The ray can continue below it (the laser cuts
// submerged hulls), while its visible air segment ends on the actual wave.
template<class Height>
float waterContact(const CVECTOR &a, const CVECTOR &b, Height height)
{
    if (!finite(a) || !finite(b)) return 2.f;
    const auto gap = [&](float t) { const auto p = a + (b - a) * t; return p.y - height(p.x, p.z); };
    float previous = 0.f, before = gap(0.f);
    if (before < 0.f) return 0.f;
    const int samples = std::clamp(int(std::ceil((b - a).GetLength() / 4.f)), 2, 32);
    for (int n = 1; n <= samples; ++n)
    {
        const float t = float(n) / samples, after = gap(t);
        if (before >= 0.f && after <= 0.f)
        {
            float lo = previous, hi = t;
            for (int iteration = 0; iteration < 9; ++iteration)
            {
                const float mid = (lo + hi) * .5f;
                if (gap(mid) > 0.f) lo = mid; else hi = mid;
            }
            return hi;
        }
        previous = t; before = after;
    }
    return 2.f;
}
inline bool reachable(const CVECTOR &from, const CVECTOR &to, float speed, float gravity)
{
    const CVECTOR d = to - from;
    const double r2 = double(d.x)*d.x + double(d.z)*d.z;
    const double b = double(gravity)*d.y - double(speed)*speed;
    const double c = r2 + double(d.y)*d.y;
    return finite(from) && finite(to) && std::isfinite(speed) && std::isfinite(gravity) && speed > 0.f && gravity > 0.f && r2 > 1e-8 &&
           b < 0.0 && b*b - double(gravity)*gravity*c >= 0.0;
}
inline float maximumRangeElevation(float height,float speed,float minAngle,float maxAngle,float gravity)
{
    const float optimum=std::atan2(speed,std::sqrt(speed*speed+2.f*gravity*std::max(0.f,height)));
    return std::clamp(optimum,minAngle,maxAngle);
}
inline bool farRayLimit(const CVECTOR &camera,const CVECTOR &muzzle,const CVECTOR &flatDirection,
                        float muzzleRange,float &limit)
{
    const CVECTOR offset(camera.x-muzzle.x,0.f,camera.z-muzzle.z);
    const double along=offset|flatDirection;
    const double perpendicular=std::max(0.0,double(~offset)-along*along);
    const double discriminant=double(muzzleRange)*muzzleRange-perpendicular;
    if(!finite(camera)||!finite(muzzle)||!finite(flatDirection)||!std::isfinite(muzzleRange)||
       muzzleRange<=0.f||discriminant<0.0)return false;
    const double far=-along+std::sqrt(discriminant);
    if(!std::isfinite(far)||far<=0.0)return false;
    limit=std::min(limit,float(far));return true;
}
struct Trajectory
{
    CVECTOR origin, velocity, acceleration;
    CVECTOR at(float t) const { return origin + velocity*t + acceleration*(0.5f*t*t); }
    CVECTOR tangent(float t) const { return velocity + acceleration*t; }
    float timeToHeight(float height) const
    {
        const double g = -acceleration.y, h = double(origin.y)-height, v = velocity.y;
        if (g <= 0.0 || h < 0.0) return 0.f;
        const double root = std::sqrt(v*v + 2.0*g*h);
        return float(v >= 0.0 ? (v+root)/g : 2.0*h/(root-v));
    }
};
inline Trajectory trajectory(const CVECTOR &muzzle, const CVECTOR &target, float speed, float angle,
                             float azimuthDelta, float heightScale, float gravity, bool fixedFrame = false)
{
    const CVECTOR d = target-muzzle;
    const float raw = fixedFrame ? 0.f : std::atan2(d.y, std::sqrt(d.x*d.x+d.z*d.z));
    const float s = std::sin(raw), c = std::cos(raw);
    const float A = c*c+heightScale*s*s, B = (1.f-heightScale)*s*c, C = s*s+heightScale*c*c;
    const float yaw = std::atan2(d.x,d.z)+azimuthDelta;
    const CVECTOR forward(std::sin(yaw),0.f,std::cos(yaw));
    const float vx = speed*(A*std::cos(angle)+B*std::sin(angle));
    const float vy = speed*(B*std::cos(angle)+C*std::sin(angle));
    return {muzzle, forward*vx+CVECTOR(0.f,vy,0.f),
            forward*(-gravity*B)+CVECTOR(0.f,-gravity*C,0.f)};
}
template<class Hit> struct MarchResult { float end; Hit hit; };
// Preview callers supply a pure nearest-receiver query. A missed hull sample
// continues along this SAME trajectory; it is not reprojected along a normal.
template<class Trace>
auto firstContact(const Trajectory &curve,float limit,int steps,Trace trace)
{
    using Hit=decltype(trace(curve.origin,curve.origin));
    MarchResult<Hit> result{0.f,{}};
    if(!std::isfinite(limit)||limit<=0.f||steps<=0)return result;
    CVECTOR previous=curve.origin;
    for(int i=1;i<=steps;++i)
    {
        const float t=limit*float(i)/steps;
        const CVECTOR next=curve.at(t);
        const Hit hit=trace(previous,next);
        if(hit.valid)
            return MarchResult<Hit>{limit*(float(i-1)+hit.fraction)/steps,hit};
        previous=next;
    }
    return MarchResult<Hit>{limit,{}};
}
inline float corridorOpacity(float progress,bool ready)
{
    const float u=std::clamp(progress,0.f,1.f);
    return (.009f+.036f*u)*(ready?1.f:.55f);
}
// Independent optional budgets: refining one contact cell never consumes a
// later cell's required coverage. Mandatory shared samples are reserved first.
template<size_t Cells> struct ContactBudget
{
    std::array<int,Cells> detail{},patch{};
    bool take(int cell,bool localPatch)
    {
        if(cell<0)return true;
        if(size_t(cell)>=Cells)return false;
        auto &used=localPatch?patch[cell]:detail[cell];
        const int limit=localPatch?8:16;
        if(used>=limit)return false;
        ++used;return true;
    }
    int remaining(int cell) const
    {return cell>=0&&size_t(cell)<Cells?16-detail[cell]:0;}
};
// A detected receiver surrounded by another receiver needs a small local patch,
// not an all-or-nothing mixed triangle. All emitted vertices are real contacts.
template<class Vertex,class Sample,class Emit>
void contactPatch(const Vertex &center,float du,float dv,Sample sample,Emit emit)
{
    if(!center.hit.valid||du<=0.f||dv<=0.f)return;
    const std::array<Vertex,4> ring{{sample(center.yaw+du,center.elevation),
        sample(center.yaw,center.elevation+dv),sample(center.yaw-du,center.elevation),
        sample(center.yaw,center.elevation-dv)}};
    for(size_t i=0;i<ring.size();++i)
    {
        const auto &a=ring[i],&b=ring[(i+1)%ring.size()];
        if(a.hit.valid&&b.hit.valid&&a.hit.surface==center.hit.surface&&b.hit.surface==center.hit.surface)
            emit(center,a,b);
    }
}
// The union of these same-first-water faces is independent of the stopped air
// hull. A mixed receiver never becomes a face spanning its physical shadow.
template<class Vertex,class Emit>
void waterContactFan(const Vertex &a,const Vertex &b,const Vertex &c,const Vertex &d,
                     const Vertex &center,Emit emit)
{
    if(!center.hit.valid||center.hit.surface!=0)return;
    const std::array<Vertex,4> ring{{a,b,c,d}};
    for(size_t i=0;i<ring.size();++i)
    {
        const auto &x=ring[i],&y=ring[(i+1)%ring.size()];
        if(x.hit.valid&&y.hit.valid&&x.hit.surface==0&&y.hit.surface==0)emit(center,x,y);
    }
}
struct Point { float x,y; };
inline float cross(Point a,Point b,Point c)
{ return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); }
inline std::vector<Point> hull(std::vector<Point> p)
{
    std::sort(p.begin(),p.end(),[](Point a,Point b){return a.x<b.x || (a.x==b.x && a.y<b.y);});
    p.erase(std::unique(p.begin(),p.end(),[](Point a,Point b){return a.x==b.x&&a.y==b.y;}),p.end());
    if(p.size()<3) return p;
    std::vector<Point> h(2*p.size()); size_t k=0;
    for(const auto &q:p){while(k>=2&&cross(h[k-2],h[k-1],q)<=0.f)--k;h[k++]=q;}
    const size_t lower=k;
    for(size_t i=p.size()-1;i-->0;){while(k>lower&&cross(h[k-2],h[k-1],p[i])<=0.f)--k;h[k++]=p[i];}
    h.resize(k-1);return h;
}
struct SlabPlane {float x,y,offset0,offset1;};
inline std::vector<SlabPlane> slabPlanes(const std::vector<Point> &a,const std::vector<Point> &b)
{
    std::vector<Point> directions;
    auto add=[&](float x,float y)
    {
        const float length=std::sqrt(x*x+y*y);if(length<1e-8f)return;
        x/=length;y/=length;
        for(const auto &n:directions)if((n.x==x&&n.y==y)||n.x*x+n.y*y>1.f-1e-7f)return;
        directions.push_back({x,y});
    };
    for(const auto *h:{&a,&b})
    {
        if(h->size()>=3)
            for(size_t i=0;i<h->size();++i)
            {const auto p=(*h)[i],q=(*h)[(i+1)%h->size()];add(q.y-p.y,p.x-q.x);}
        else if(h->size()==2)
        {
            const float x=(*h)[1].x-(*h)[0].x,y=(*h)[1].y-(*h)[0].y;
            add(x,y);add(-x,-y);add(y,-x);add(-y,x);
        }
        else {add(1.f,0.f);add(-1.f,0.f);add(0.f,1.f);add(0.f,-1.f);}
    }
    auto support=[](const std::vector<Point> &h,Point n)
    {
        float value=-1e20f;for(const auto &p:h)value=std::max(value,n.x*p.x+n.y*p.y);
        return value+(h.size()<3?.025f:0.f);
    };
    std::vector<SlabPlane> planes;
    if(a.empty()||b.empty())return planes;
    for(Point n:directions)planes.push_back({n.x,n.y,support(a,n),support(b,n)});
    return planes;
}
// Fixed angular sampling of a convex contour: no dependence on locator ordering,
// no corner-index swapping, and the same winding on either broadside.
inline Point boundary(const std::vector<Point> &h,Point center,float angle)
{
    const Point d{std::cos(angle),std::sin(angle)};
    if(h.size()==1)return h.front();
    if(h.size()==2)
    {
        const Point edge{h[1].x-h[0].x,h[1].y-h[0].y};
        if(std::fabs(edge.x*d.y-edge.y*d.x)>1e-6f*std::sqrt(edge.x*edge.x+edge.y*edge.y))return center;
        const float a=(h[0].x-center.x)*d.x+(h[0].y-center.y)*d.y;
        const float b=(h[1].x-center.x)*d.x+(h[1].y-center.y)*d.y;
        return a>b?h[0]:h[1];
    }
    float t=1e20f;
    for(size_t i=0;i<h.size();++i)
    {
        const Point a=h[i],b=h[(i+1)%h.size()];
        const Point e{b.x-a.x,b.y-a.y};
        const float den=e.x*d.y-e.y*d.x;
        const float num=e.x*(center.y-a.y)-e.y*(center.x-a.x);
        if(den < -1e-7f) t=std::min(t,-num/den);
    }
    if(!std::isfinite(t)||t<0.f||t>1e10f)t=0.f;
    return {center.x+d.x*t,center.y+d.y*t};
}
// Keep all convex-hull corner angles when lofting adjacent sections. A fixed
// radial polygon alone inscribes (and can almost erase) a long tilted gun line.
inline std::vector<float> angularEvents(const std::vector<Point> &a,const std::vector<Point> &b,int regular)
{
    constexpr float tau=6.2831853071795864769f;
    std::vector<float> angles;
    for(int i=0;i<regular;++i)angles.push_back(tau*i/regular);
    for(const auto *h:{&a,&b})for(Point p:*h)
    {
        float angle=std::atan2(p.y,p.x);
        if(angle<0.f)angle+=tau;
        angles.push_back(angle);
    }
    std::sort(angles.begin(),angles.end());
    angles.erase(std::unique(angles.begin(),angles.end(),[](float x,float y){return std::fabs(x-y)<1e-7f;}),angles.end());
    return angles;
}
// A common plane can cross a quadratic path in either direction, or twice.
// Never discard a legitimate off-axis gun because its projected motion reverses.
// Immutable projection terms shared by all stations of one current trajectory.
struct PlaneQuery
{
    double a=0.,b=0.,origin=0.;float end;bool valid;
    PlaneQuery(const Trajectory &path,const CVECTOR &axis,float limit):end(limit),
        valid(finite(path.origin)&&finite(path.velocity)&&finite(path.acceleration)&&finite(axis)&&
            std::isfinite(limit)&&limit>=0.f&&~axis>=1e-12f)
    {
        if(valid){a=.5*double(path.acceleration|axis);b=double(path.velocity|axis);origin=double(path.origin|axis);}
    }
    template<class Visit> void forEach(float station,Visit visit) const
    {
        if(!valid||!std::isfinite(station))return;
        std::array<float,2> times;size_t count=0;
        const double c=origin-station;
        auto add=[&](double t)
        {
            if(std::isfinite(t)&&t>=-1e-5&&t<=double(end)+1e-5)
                times[count++]=float(std::clamp(t,0.0,double(end)));
        };
        if(std::fabs(a)<1e-10)
        {
            if(std::fabs(b)>1e-10)add(-c/b);
            else if(std::fabs(c)<1e-5){add(0.0);add(end);}
        }
        else
        {
            const double discriminant=b*b-4.0*a*c;
            if(discriminant>=-1e-8)
            {
                const double root=std::sqrt(std::max(0.0,discriminant));
                const double q=-.5*(b+std::copysign(root,b));
                if(std::fabs(q)>1e-12){add(q/a);add(c/q);}else add(-b/(2.0*a));
            }
        }
        if(count==2&&times[1]<times[0])std::swap(times[0],times[1]);
        if(count)visit(times[0]);
        if(count==2&&std::fabs(times[1]-times[0])>=1e-5f)visit(times[1]);
    }
};
template<class Visit>
inline void forEachPlaneTime(const Trajectory &path,const CVECTOR &axis,float station,float end,Visit visit)
{
    PlaneQuery(path,axis,end).forEach(station,visit);
}
inline std::vector<float> planeTimes(const Trajectory &path,const CVECTOR &axis,float station,float end)
{
    std::vector<float> times;
    forEachPlaneTime(path,axis,station,end,[&](float t){times.push_back(t);});
    return times;
}
inline bool segmentBox(const CVECTOR &a,const CVECTOR &b,const CVECTOR &lo,const CVECTOR &hi)
{
    if(!finite(a)||!finite(b)||!finite(lo)||!finite(hi))return false;
    for(int i=0;i<3;++i)if(lo.v[i]>hi.v[i])return false;
    float begin=0.f,end=1.f;
    for(int i=0;i<3;++i)
    {
        const float d=b.v[i]-a.v[i];
        if(std::fabs(d)<1e-8f){if(a.v[i]<lo.v[i]||a.v[i]>hi.v[i])return false;continue;}
        float x=(lo.v[i]-a.v[i])/d,y=(hi.v[i]-a.v[i])/d;
        if(x>y)std::swap(x,y);
        begin=std::max(begin,x);end=std::min(end,y);
        if(begin>end)return false;
    }
    return true;
}
// Compensate both-axis projection zoom before expanding angular support by at
// most 50% at the deck camera's 5x limit. Ordinary FOV/aspect policy is preserved.
inline float rangeApertureSlope(float referenceSpan,float projectionScale,float magnification)
{
    const float zoom=std::isfinite(magnification)?std::clamp(magnification,1.f,5.f):1.f;
    const float growth=1.f+.125f*(zoom-1.f);
    return referenceSpan*zoom*growth/std::max(.01f,std::fabs(projectionScale));
}
// Range selection never redirects the centered camera ray toward a peripheral
// mast/hull point. It supplies only the point's view-axis depth.
inline CVECTOR centerRangeTarget(const CVECTOR &camera,const CVECTOR &forward,float depth)
{
    return camera+forward*depth;
}
inline bool insideRangeAperture(const CVECTOR &delta,const CVECTOR &forward,
                               const CVECTOR &right,const CVECTOR &up,float tx,float ty)
{
    const float z=delta|forward;
    return z>.01f && std::fabs(delta|right)<=z*tx && std::fabs(delta|up)<=z*ty;
}
} // namespace manual_aim

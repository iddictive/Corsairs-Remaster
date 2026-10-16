#pragma once

#include "ship_hull_geometry.hpp"
#include <limits>

// Continuous separating intervals for a translating convex body and a triangle.
// Face/edge axes follow Eberly, DynamicCollisionDetection, sections 1 and 3:
// https://www.geometrictools.com/Documentation/DynamicCollisionDetection.pdf
// Rotation belongs to the caller's bounded rigid-motion steps, not this query.
namespace storm::ship::fragment_contact
{
using storm_ship_hull::Vec3;
using storm_ship_hull::dot;
using storm_ship_hull::cross;
using storm_ship_hull::length;
using storm_ship_hull::normalized;
using storm_ship_hull::finite;

struct Proxy
{
    std::array<Vec3, 32> vertices{};
    std::array<Vec3, 28> faceNormals{};
    std::array<Vec3, 36> edgeDirections{};
    unsigned count = 0, faceCount = 0, edgeCount = 0;
    bool valid() const { return count >= 3 && faceCount && edgeCount; }
};
struct Hit
{
    bool hit = false;
    float fraction = 1.f;
    Vec3 normal{}, point{}; // receiver -> fragment; point lies on receiver
};

// Unit-mass impulse at the actual contact, with the same scalar inertia used
// by the body integrator. A grazing seam cannot entrain a piece at ship speed:
// tangential work is bounded by Coulomb friction and the admitted normal load.
inline Vec3 contactImpulse(Vec3 relative, Vec3 lever, Vec3 normal, float inverseInertia)
{
    const float inward = dot(relative, normal);
    if (!(inward < 0.f)) return {};
    const auto rn = cross(lever, normal);
    const float normalImpulse = -inward * 1.12f / (1.f + dot(rn, rn) * inverseInertia);
    const auto push = normal * normalImpulse;
    const auto after = relative + push + cross(cross(lever, push) * inverseInertia, lever);
    const auto tangent = after - normal * dot(after, normal);
    const float speed = length(tangent);
    if (speed < 1.e-7f) return push;
    const auto direction = tangent * (1.f / speed), rt = cross(lever, direction);
    const float friction = std::min(speed / (1.f + dot(rt, rt) * inverseInertia), .18f * normalImpulse);
    return push - direction * friction;
}

namespace detail
{
inline double projection(Vec3 p, Vec3 n)
{ return double(p.x) * n.x + double(p.y) * n.y + double(p.z) * n.z; }
template<size_t N> inline bool addDirection(std::array<Vec3, N> &out, unsigned &count, Vec3 axis)
{
    const float size = length(axis);
    if (size < 1.e-7f) return true;
    axis = axis * (1.f / size);
    for (unsigned n = 0; n < count; ++n)
        if (std::abs(dot(out[n], axis)) > 1.f - 1.e-6f) return true;
    if (count == N) return false;
    out[count++] = axis;
    return true;
}
inline Vec3 closestTriangle(Vec3 p, const std::array<Vec3, 3> &t)
{
    const auto a = t[0], b = t[1], c = t[2], ab = b-a, ac = c-a, ap = p-a;
    const float d1 = dot(ab, ap), d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a;
    const auto bp = p-b;
    const float d3 = dot(ab, bp), d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return b;
    const float vc = d1*d4-d3*d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return a + ab*(d1/(d1-d3));
    const auto cp = p-c;
    const float d5 = dot(ab, cp), d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return c;
    const float vb = d5*d2-d1*d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return a + ac*(d2/(d2-d6));
    const float va = d3*d6-d5*d4;
    if (va <= 0 && d4-d3 >= 0 && d5-d6 >= 0)
        return b + (c-b)*((d4-d3)/((d4-d3)+(d5-d6)));
    const float inverse = 1.f/(va+vb+vc);
    return a + ab*(vb*inverse) + ac*(vc*inverse);
}
}

// Build only on separation, from actual surface support vertices. Supporting
// planes identify convex faces; an edge belongs to two nonparallel faces.
// A planar shell additionally needs its polygon's in-plane boundary normals.
inline Proxy pointProxy(const std::array<Vec3, 14> &points)
{
    Proxy p;
    float scale = 1.f;
    for (auto v : points) { if (!finite(v)) return {}; scale = std::max(scale, length(v-points[0])); }
    const float epsilon = scale * 2.e-6f;
    for (auto v : points)
    {
        bool duplicate = false;
        for (unsigned n = 0; n < p.count; ++n) if (length(v-p.vertices[n]) <= epsilon) { duplicate = true; break; }
        if (!duplicate) p.vertices[p.count++] = v;
    }
    if (p.count < 3) return {};
    std::array<uint32_t, 28> faceMasks{};
    Vec3 plane{};
    bool planar = true;
    for (unsigned a = 0; a < p.count; ++a)
        for (unsigned b = a+1; b < p.count; ++b)
            for (unsigned c = b+1; c < p.count; ++c)
            {
                auto normal = cross(p.vertices[b]-p.vertices[a], p.vertices[c]-p.vertices[a]);
                if (length(normal) <= epsilon * epsilon) continue;
                normal = normalized(normal);
                float low = 0, high = 0;
                uint32_t mask = 0;
                for (unsigned n = 0; n < p.count; ++n)
                {
                    const float d = dot(normal, p.vertices[n]-p.vertices[a]);
                    low = std::min(low, d); high = std::max(high, d);
                    if (std::abs(d) <= epsilon) mask |= uint32_t(1u << n);
                }
                if (low < -epsilon && high > epsilon) continue;
                if (low < -epsilon || high > epsilon) planar = false;
                if (high > epsilon) normal = normal * -1.f;
                bool duplicate = false;
                for (unsigned n = 0; n < p.faceCount; ++n) if (faceMasks[n] == mask) { duplicate = true; break; }
                if (duplicate) continue;
                if (p.faceCount == p.faceNormals.size()) return {};
                faceMasks[p.faceCount] = mask;
                p.faceNormals[p.faceCount++] = normal;
                plane = normal;
            }
    if (!p.faceCount) return {};
    for (unsigned a = 0; a < p.count; ++a)
        for (unsigned b = a+1; b < p.count; ++b)
        {
            auto edge = normalized(p.vertices[b]-p.vertices[a]);
            if (planar)
            {
                auto side = cross(edge, plane);
                float low = 0, high = 0;
                for (unsigned n = 0; n < p.count; ++n)
                { const float d = dot(side, p.vertices[n]-p.vertices[a]); low = std::min(low, d); high = std::max(high, d); }
                if (low < -epsilon && high > epsilon) continue;
                if (!detail::addDirection(p.faceNormals, p.faceCount, side) ||
                    !detail::addDirection(p.edgeDirections, p.edgeCount, edge)) return {};
            }
            else
            {
                const uint32_t pair = uint32_t((1u << a) | (1u << b));
                bool edgeFound = false;
                for (unsigned i = 0; i < p.faceCount && !edgeFound; ++i)
                    if ((faceMasks[i] & pair) == pair)
                        for (unsigned j = i+1; j < p.faceCount; ++j)
                            if ((faceMasks[j] & pair) == pair && std::abs(dot(p.faceNormals[i], p.faceNormals[j])) < 1.f-1.e-6f)
                            { edgeFound = true; break; }
                if (edgeFound && !detail::addDirection(p.edgeDirections, p.edgeCount, edge)) return {};
            }
        }
    return p.valid() ? p : Proxy{};
}

// fragmentProperties supplies extremal vertices in these exact directions.
// Their convex hull is INSIDE the mesh and can miss metres of visible timber.
// Intersect the supporting halfspaces instead: a fourteen-plane outer k-DOP
// contains every authored vertex, with at most 24 vertices and 36 edges.
inline Proxy makeProxy(const std::array<Vec3, 14> &points)
{
    constexpr std::array<Vec3,14> directions{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},
        {1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},{-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1}}};
    std::array<Vec3,14> normals; std::array<float,14> distances;
    float scale = 1.f;
    for (auto point : points) { if (!finite(point)) return {}; scale = std::max(scale,length(point-points[0])); }
    const float epsilon = scale * 4.e-6f;
    const auto boxFallback = [&] {
        std::array<Vec3,14> corners{}; unsigned count=0;
        for (float x : {points[1].x-epsilon,points[0].x+epsilon})
            for (float y : {points[3].y-epsilon,points[2].y+epsilon})
                for (float z : {points[5].z-epsilon,points[4].z+epsilon}) corners[count++]={x,y,z};
        for (;count<14;++count) corners[count]=corners[0];
        return pointProxy(corners);
    };
    for (unsigned n=0;n<14;++n) { normals[n]=normalized(directions[n]); distances[n]=dot(normals[n],points[n])+epsilon; }
    Proxy p; std::array<uint16_t,32> masks{};
    for (unsigned a=0;a<14;++a) for (unsigned b=a+1;b<14;++b) for (unsigned c=b+1;c<14;++c)
    {
        const auto bc=cross(normals[b],normals[c]); const float determinant=dot(normals[a],bc);
        if (std::abs(determinant)<1.e-6f) continue;
        const auto vertex=(bc*distances[a]+cross(normals[c],normals[a])*distances[b]+
                           cross(normals[a],normals[b])*distances[c])*(1.f/determinant);
        uint16_t mask=0; bool outside=false;
        for (unsigned n=0;n<14;++n) { const float d=dot(normals[n],vertex)-distances[n];
            if (d>epsilon) { outside=true;break; } if (std::abs(d)<=epsilon) mask|=uint16_t(1u<<n); }
        if (outside) continue;
        unsigned index=0; for (;index<p.count;++index) if (length(vertex-p.vertices[index])<epsilon*2.f) break;
        if (index==p.count) { if (p.count==p.vertices.size()) return boxFallback(); p.vertices[p.count++]=vertex; }
        masks[index]|=mask;
    }
    for (unsigned n=0;n<14;++n) {
        unsigned count=0;for(unsigned v=0;v<p.count;++v)if(masks[v]&(1u<<n))++count;
        if(count>=3&&!detail::addDirection(p.faceNormals,p.faceCount,normals[n]))return boxFallback();
    }
    for (unsigned a=0;a<p.count;++a) for (unsigned b=a+1;b<p.count;++b)
    {
        const auto common=uint16_t(masks[a]&masks[b]); bool edge=false;
        for (unsigned i=0;i<14&&!edge;++i) if (common&(1u<<i))
            for (unsigned j=i+1;j<14;++j) if ((common&(1u<<j))&&std::abs(dot(normals[i],normals[j]))<.9999f) { edge=true;break; }
        if (edge&&!detail::addDirection(p.edgeDirections,p.edgeCount,p.vertices[b]-p.vertices[a])) return boxFallback();
    }
    return p.valid()?p:boxFallback();
}

inline storm_ship_hull::Bounds bounds(const Proxy &body, Vec3 travel={}, float padding=0.f)
{
    storm_ship_hull::Bounds result{body.vertices[0],body.vertices[0]};
    for (unsigned n=0;n<body.count;++n) for (auto p : {body.vertices[n],body.vertices[n]+travel})
        result=storm_ship_hull::merge(result,{p,p});
    const Vec3 margin{padding,padding,padding}; result.lo=result.lo-margin;result.hi=result.hi+margin;return result;
}

// Continuous timber-likeness, rather than interpreting open meshes as volume.
// Thin, small separated boards keep wood buoyancy; large hollow assemblies
// retain trapped air briefly and then flood. Neither is a measured density.
inline float timberBuoyancy(Vec3 dimensions)
{
    const float small=std::min({dimensions.x,dimensions.y,dimensions.z});
    const float large=std::max({dimensions.x,dimensions.y,dimensions.z});
    return (1.f-std::clamp(small/.8f,0.f,1.f))*(1.f-std::clamp((large-3.f)/9.f,0.f,1.f));
}
inline float waterDensity(float wetAge,float timber,float age)
{
    const float delay=12.f+33.f*timber, duration=18.f+27.f*timber;
    const float flooded=std::clamp((wetAge-delay)/duration,0.f,1.f);
    const float saturated=.86f+(.74f*(1.f-timber));
    const float retirement=std::clamp((age-75.f)/15.f,0.f,1.f);
    return .6f+(saturated-.6f)*flooded+std::max(0.f,1.6f-saturated)*retirement;
}

namespace detail
{
struct Point2 {float x=0,y=0;};
inline Point2 operator+(Point2 a,Point2 b){return {a.x+b.x,a.y+b.y};}
inline Point2 operator-(Point2 a,Point2 b){return {a.x-b.x,a.y-b.y};}
inline Point2 operator*(Point2 a,float b){return {a.x*b,a.y*b};}
inline float cross2(Point2 a,Point2 b){return a.x*b.y-a.y*b.x;}
inline float distance2(Point2 a){return a.x*a.x+a.y*a.y;}
struct Feature {std::array<Point2,32>points{};unsigned count=0;};
inline Feature supportFeature(const Proxy&body,Vec3 travel,float fraction,Vec3 normal,double support,
                              float sign,float padding,Vec3 origin,Vec3 x,Vec3 y)
{
    Feature feature;
    for(unsigned n=0;n<body.count;++n)if(sign*(projection(body.vertices[n],normal)-support)<=padding+1.e-4f){
        const auto p=body.vertices[n]+travel*fraction-origin;
        feature.points[feature.count++]={dot(p,x),dot(p,y)};
    }
    std::sort(feature.points.begin(),feature.points.begin()+feature.count,[](Point2 a,Point2 b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});
    unsigned unique=0;for(unsigned n=0;n<feature.count;++n)
        if(!unique||distance2(feature.points[n]-feature.points[unique-1])>1.e-10f)feature.points[unique++]=feature.points[n];
    feature.count=unique;if(unique<=2)return feature;
    std::array<Point2,64>hull;unsigned count=0;
    for(unsigned n=0;n<unique;++n){while(count>=2&&cross2(hull[count-1]-hull[count-2],feature.points[n]-hull[count-1])<=1.e-8f)--count;hull[count++]=feature.points[n];}
    const unsigned lower=count;
    for(unsigned n=unique-1;n-->0;){while(count>lower&&cross2(hull[count-1]-hull[count-2],feature.points[n]-hull[count-1])<=1.e-8f)--count;hull[count++]=feature.points[n];}
    feature.count=count-1;std::copy_n(hull.begin(),feature.count,feature.points.begin());return feature;
}
inline Point2 closestSegment(Point2 p,Point2 a,Point2 b)
{
    const auto ab=b-a;const float size=distance2(ab);if(size<1.e-12f)return a;
    const auto d=p-a;return a+ab*std::clamp((d.x*ab.x+d.y*ab.y)/size,0.f,1.f);
}
inline bool contains(const Feature&f,Point2 p)
{
    if(!f.count)return false;
    if(f.count==1)return distance2(p-f.points[0])<=1.e-8f;
    if(f.count==2)return distance2(p-closestSegment(p,f.points[0],f.points[1]))<=1.e-8f;
    for(unsigned n=0;n<f.count;++n)if(cross2(f.points[(n+1)%f.count]-f.points[n],p-f.points[n]) < -1.e-5f)return false;
    return true;
}
struct Witness {Vec3 point{};double closing=std::numeric_limits<double>::infinity();};
// A contact impulse needs ONE common spatial witness. Independent face means
// can lie on the spin axis even though a support tip is entering the other body.
// Intersect the bounded projected support features, then minimize relative
// normal motion on that common patch. Equal minima average to retain centered
// face/face translation; angular minima retain the actually approaching edge.
inline Witness pairWitness(const Proxy&a,Vec3 travelA,const Proxy&b,Vec3 travelB,float fraction,
                           Vec3 normal,double low,double high,float padding,Vec3 angularA,Vec3 centerA,Vec3 angularB,Vec3 centerB)
{
    const auto basis=storm_ship_hull::frame(normal);const auto origin=(centerA+centerB)*.5f;
    const auto fa=supportFeature(a,travelA,fraction,normal,low,1.f,padding,origin,basis.tangent,basis.bitangent);
    const auto fb=supportFeature(b,travelB,fraction,normal,high,-1.f,padding,origin,basis.tangent,basis.bitangent);
    const float plane=float((low+high+projection(travelA+travelB,normal)*fraction)*.5-projection(origin,normal));
    Witness result;Vec3 sum{};unsigned count=0;
    const auto record=[&](Point2 point){const auto world=origin+basis.tangent*point.x+basis.bitangent*point.y+normal*plane;
        const double closing=projection(travelA+cross(angularA,world-centerA-travelA*fraction)-travelB-cross(angularB,world-centerB-travelB*fraction),normal);
        if(closing<result.closing-1.e-7){result.closing=closing;sum=world;count=1;}
        else if(std::abs(closing-result.closing)<=1.e-7){sum=sum+world;++count;}};
    bool intersects=false;
    for(unsigned n=0;n<fa.count;++n)if(contains(fb,fa.points[n])){record(fa.points[n]);intersects=true;}
    for(unsigned n=0;n<fb.count;++n)if(contains(fa,fb.points[n])){record(fb.points[n]);intersects=true;}
    for(unsigned i=0;i<fa.count;++i)for(unsigned j=0;j<fb.count;++j){
        const auto p=fa.points[i],q=fb.points[j],r=fa.points[(i+1)%fa.count]-p,s=fb.points[(j+1)%fb.count]-q;
        const float divisor=cross2(r,s);if(std::abs(divisor)<1.e-10f)continue;
        const float t=cross2(q-p,s)/divisor,u=cross2(q-p,r)/divisor;
        if(t>=-1.e-6f&&t<=1.f+1.e-6f&&u>=-1.e-6f&&u<=1.f+1.e-6f){record(p+r*std::clamp(t,0.f,1.f));intersects=true;}}
    if(!intersects){
        float nearest=std::numeric_limits<float>::infinity();
        const auto candidate=[&](Point2 p,Point2 q){const float d=distance2(p-q);
            if(d<nearest-1.e-8f){nearest=d;result.closing=std::numeric_limits<double>::infinity();count=0;}
            if(d<=nearest+1.e-8f)record((p+q)*.5f);};
        for(unsigned i=0;i<fa.count;++i)for(unsigned j=0;j<fb.count;++j){
            candidate(fa.points[i],closestSegment(fa.points[i],fb.points[j],fb.points[(j+1)%fb.count]));
            candidate(closestSegment(fb.points[j],fa.points[i],fa.points[(i+1)%fa.count]),fb.points[j]);}
    }
    if(count)result.point=sum*(1.f/count);return result;
}
}

// Same translating-convex SAT as the triangle query below. This bounded proxy
// pair path adds no retained mesh copies or per-pair allocation.
inline Hit sweepPair(const Proxy &a,Vec3 travelA,const Proxy &b,Vec3 travelB,
                     float padding=0.f,Vec3 angularA={},Vec3 centerA={},Vec3 angularB={},Vec3 centerB={})
{
    Hit hit;
    if (!a.valid()||!b.valid()||!finite(travelA)||!finite(travelB)||!finite(angularA)||!finite(angularB)||!finite(centerA)||!finite(centerB)||!std::isfinite(padding)||padding<0.f) return hit;
    const auto travel=travelA-travelB;
    double enter=0,exit=1,gap=-std::numeric_limits<double>::infinity();Vec3 normal{},initial{};
    const auto axis=[&](Vec3 candidate) {
        if (length(candidate)<1.e-7f) return true;const auto n=normalized(candidate);
        double alo=detail::projection(a.vertices[0],n),ahi=alo,blo=detail::projection(b.vertices[0],n),bhi=blo;
        for (unsigned i=1;i<a.count;++i) { const double v=detail::projection(a.vertices[i],n);alo=std::min(alo,v);ahi=std::max(ahi,v); }
        for (unsigned i=1;i<b.count;++i) { const double v=detail::projection(b.vertices[i],n);blo=std::min(blo,v);bhi=std::max(bhi,v); }
        const double forward=alo-bhi,backward=blo-ahi;
        if (std::max(forward,backward)>gap) { gap=std::max(forward,backward);initial=forward>=backward?n:n*-1.f; }
        const double velocity=detail::projection(travel,n);
        if (std::abs(velocity)<1.e-12) return alo-padding<=bhi+1.e-6&&ahi+padding>=blo-1.e-6;
        double first=(blo-ahi-padding)/velocity,last=(bhi-alo+padding)/velocity;
        if (first>last)std::swap(first,last);
        if (first>enter) { enter=first;normal=n*(velocity>0?-1.f:1.f); }
        exit=std::min(exit,last);return enter<=exit+1.e-7&&exit>=0&&enter<=1;
    };
    for (unsigned n=0;n<a.faceCount;++n) if(!axis(a.faceNormals[n]))return hit;
    for (unsigned n=0;n<b.faceCount;++n) if(!axis(b.faceNormals[n]))return hit;
    for (unsigned i=0;i<a.edgeCount;++i)for(unsigned j=0;j<b.edgeCount;++j)
        if(!axis(cross(a.edgeDirections[i],b.edgeDirections[j])))return hit;
    if(enter<=1.e-6)normal=initial;
    double low=std::numeric_limits<double>::infinity(),high=-low;
    for(unsigned i=0;i<a.count;++i)low=std::min(low,detail::projection(a.vertices[i],normal));
    for(unsigned i=0;i<b.count;++i)high=std::max(high,detail::projection(b.vertices[i],normal));
    const float fraction=float(std::clamp(enter,0.,1.));
    const auto witness=detail::pairWitness(a,travelA,b,travelB,fraction,normal,low,high,padding,angularA,centerA,angularB,centerB);
    if(enter<=1.e-6&&witness.closing>=-1.e-7)return hit;
    hit.hit=true;hit.fraction=fraction;hit.normal=normal;hit.point=witness.point;return hit;
}

inline Vec3 pairImpulse(Vec3 relative,Vec3 leverA,Vec3 leverB,Vec3 normal,
                        float inverseMassA,float inverseMassB,float inverseInertiaA,float inverseInertiaB,bool planarReceiver=false)
{
    const float inward=dot(relative,normal);if(!(inward<0.f))return {};
    const auto linearB=[&](Vec3 v){return Vec3{v.x*inverseMassB,planarReceiver?0.f:v.y*inverseMassB,v.z*inverseMassB};};
    const auto angularB=[&](Vec3 v){return Vec3{planarReceiver?0.f:v.x*inverseInertiaB,v.y*inverseInertiaB,planarReceiver?0.f:v.z*inverseInertiaB};};
    const auto effective=[&](Vec3 direction) { const auto a=cross(leverA,direction),b=cross(leverB,direction);
        return inverseMassA+dot(direction,linearB(direction))+dot(a,a)*inverseInertiaA+dot(b,angularB(b)); };
    const float load=-1.12f*inward/std::max(1.e-9f,effective(normal));const auto push=normal*load;
    const auto after=relative+push*inverseMassA+linearB(push)+
        cross(cross(leverA,push)*inverseInertiaA,leverA)+cross(angularB(cross(leverB,push)),leverB);
    const auto tangent=after-normal*dot(after,normal);const float speed=length(tangent);
    if(speed<1.e-7f)return push;const auto direction=tangent*(1.f/speed);
    return push-direction*std::min(speed/std::max(1.e-9f,effective(direction)),.18f*load);
}

// The supplied transform must be rigid, as MODEL node/world transforms are.
template<class Transform> inline Proxy transformed(const Proxy &local, Transform point)
{
    if (!local.valid()) return {};
    Proxy world = local;
    const auto origin = point(Vec3{});
    for (unsigned n = 0; n < world.count; ++n) world.vertices[n] = point(local.vertices[n]);
    for (unsigned n = 0; n < world.faceCount; ++n) world.faceNormals[n] = normalized(point(local.faceNormals[n])-origin);
    for (unsigned n = 0; n < world.edgeCount; ++n) world.edgeDirections[n] = normalized(point(local.edgeDirections[n])-origin);
    return world;
}

inline Hit sweep(const Proxy &body, Vec3 translation, const std::array<Vec3, 3> &triangle, float padding = 0.f,
                 Vec3 angularDisplacement = {}, Vec3 rotationCenter = {})
{
    Hit result;
    if (!body.valid() || !finite(translation) || !finite(angularDisplacement) || !finite(rotationCenter) ||
        !std::isfinite(padding) || padding < 0.f) return result;
    for (auto v : triangle) if (!finite(v)) return result;
    const std::array<Vec3, 3> edges{{triangle[1]-triangle[0], triangle[2]-triangle[1], triangle[0]-triangle[2]}};
    const auto triangleNormal = cross(edges[0], edges[1]);
    if (length(triangleNormal) < 1.e-8f) return result;
    double enter = 0, exit = 1, initialGap = -std::numeric_limits<double>::infinity();
    Vec3 enterNormal{}, initialNormal{};
    const auto axis = [&](Vec3 candidate)
    {
        const float magnitude = length(candidate);
        if (magnitude < 1.e-7f) return true;
        const auto normal = candidate * (1.f/magnitude);
        double lo = detail::projection(body.vertices[0], normal), hi = lo;
        for (unsigned n = 1; n < body.count; ++n)
        { const double d = detail::projection(body.vertices[n], normal); lo = std::min(lo, d); hi = std::max(hi, d); }
        double tlo = detail::projection(triangle[0], normal), thi = tlo;
        for (unsigned n = 1; n < 3; ++n)
        { const double d = detail::projection(triangle[n], normal); tlo = std::min(tlo, d); thi = std::max(thi, d); }
        const double forward = lo-thi, backward = tlo-hi;
        const double gap = std::max(forward, backward);
        if (gap > initialGap) { initialGap = gap; initialNormal = forward >= backward ? normal : normal*-1.f; }
        const double velocity = detail::projection(translation, normal);
        if (std::abs(velocity) < 1.e-12) return lo-padding <= thi+1.e-6 && hi+padding >= tlo-1.e-6;
        double first = (tlo-hi-padding)/velocity, last = (thi-lo+padding)/velocity;
        const Vec3 outward = normal * (velocity > 0 ? -1.f : 1.f);
        if (first > last) std::swap(first, last);
        if (first > enter) { enter = first; enterNormal = outward; }
        exit = std::min(exit, last);
        return enter <= exit+1.e-7 && exit >= 0 && enter <= 1;
    };
    if (!axis(triangleNormal)) return result;
    for (unsigned n = 0; n < body.faceCount; ++n) if (!axis(body.faceNormals[n])) return result;
    for (unsigned n = 0; n < body.edgeCount; ++n)
        for (auto edge : edges) if (!axis(cross(body.edgeDirections[n], edge))) return result;
    // These axes are necessary for coplanar triangles/zero-thickness shells.
    for (auto edge : edges) if (!axis(cross(triangleNormal, edge))) return result;
    const auto supportMotion = [&](Vec3 normal)
    {
        double minimum = std::numeric_limits<double>::infinity();
        for (unsigned n = 0; n < body.count; ++n)
            minimum = std::min(minimum, detail::projection(body.vertices[n], normal));
        double closing = std::numeric_limits<double>::infinity();
        for (unsigned n = 0; n < body.count; ++n)
            if (detail::projection(body.vertices[n], normal)-minimum <= padding+1.e-4)
                closing = std::min(closing, detail::projection(translation +
                    cross(angularDisplacement, body.vertices[n]-rotationCenter), normal));
        return closing;
    };
    if (enter <= 1.e-6)
    {
        // Ignore the departing cut seam and tangent resting contacts, including
        // their conservative padding. Inward motion still resolves at t=0.
        // A face average cancels opposite tip velocities during pure spin.
        // The minimum support-point velocity instead admits any inward tip.
        if (supportMotion(initialNormal) >= -1.e-7) return result;
        enterNormal = initialNormal;
    }
    result.hit = true; result.fraction = float(std::clamp(enter, 0., 1.)); result.normal = enterNormal;
    double minimum = std::numeric_limits<double>::infinity();
    for (unsigned n = 0; n < body.count; ++n) minimum = std::min(minimum, detail::projection(body.vertices[n], result.normal));
    const double closing = supportMotion(result.normal);
    Vec3 point{}; unsigned count = 0;
    for (unsigned n = 0; n < body.count; ++n)
        if (detail::projection(body.vertices[n], result.normal)-minimum <= padding+1.e-4 &&
            detail::projection(translation + cross(angularDisplacement, body.vertices[n]-rotationCenter),
                               result.normal) <= closing+1.e-5)
        { point = point + body.vertices[n]; ++count; }
    point = point*(1.f/float(count)) + translation*result.fraction;
    result.point = detail::closestTriangle(point, triangle);
    return result;
}
}

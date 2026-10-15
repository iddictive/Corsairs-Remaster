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
    std::array<Vec3, 14> vertices{};
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
inline Proxy makeProxy(const std::array<Vec3, 14> &points)
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
    std::array<uint16_t, 28> faceMasks{};
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
                uint16_t mask = 0;
                for (unsigned n = 0; n < p.count; ++n)
                {
                    const float d = dot(normal, p.vertices[n]-p.vertices[a]);
                    low = std::min(low, d); high = std::max(high, d);
                    if (std::abs(d) <= epsilon) mask |= uint16_t(1u << n);
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
                const uint16_t pair = uint16_t((1u << a) | (1u << b));
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

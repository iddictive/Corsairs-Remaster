#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>

namespace storm_airburst_hull {
struct Point { double x, y, z; };
inline Point operator+(Point a, Point b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Point operator-(Point a, Point b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Point operator*(Point a, double s) { return {a.x*s,a.y*s,a.z*s}; }
inline double dot(Point a, Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Point cross(Point a, Point b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline bool finite(Point p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }

inline Point closestSegment(Point p, Point a, Point b)
{
    const Point edge=b-a;
    const double length2=dot(edge,edge);
    return a+edge*(length2>0.0 ? std::clamp(dot(p-a,edge)/length2,0.0,1.0) : 0.0);
}

// Plane projection inside the triangle; otherwise minimize over all three
// segments. Unlike Triangle::FindClosestPoint, no edge/vertex is chosen by order.
inline Point closestTriangle(Point p, Point a, Point b, Point c)
{
    const Point normal=cross(b-a,c-a);
    const double length2=dot(normal,normal);
    if (length2>0.0)
    {
        const Point projected=p-normal*(dot(p-a,normal)/length2);
        if (dot(cross(b-a,projected-a),normal)>=0.0 &&
            dot(cross(c-b,projected-b),normal)>=0.0 &&
            dot(cross(a-c,projected-c),normal)>=0.0)
            return projected;
    }
    Point nearest=closestSegment(p,a,b);
    double distance2=dot(p-nearest,p-nearest);
    for (Point candidate : {closestSegment(p,b,c),closestSegment(p,c,a)})
    {
        const double candidate2=dot(p-candidate,p-candidate);
        if (candidate2<distance2) { nearest=candidate; distance2=candidate2; }
    }
    return nearest;
}

struct Query
{
    Point center{}, closest{};
    Point origin{}, axes[3]={{1,0,0},{0,1,0},{0,0,1}};
    double distance2=4.0;
    bool valid=false;
    void triangle(Point a, Point b, Point c)
    {
        if (!finite(a)||!finite(b)||!finite(c)) return;
        const auto world=[this](Point p) { return origin+axes[0]*p.x+axes[1]*p.y+axes[2]*p.z; };
        a=world(a); b=world(b); c=world(c);
        const Point candidate=closestTriangle(center,a,b,c);
        const double candidate2=dot(center-candidate,center-candidate);
        if (candidate2<=distance2)
        { closest=candidate; distance2=candidate2; valid=true; }
    }
};

// GEOS callbacks have no user-data slot. The game invokes Clip synchronously;
// nested scopes restore both the callback recipient and the trace preservation.
inline thread_local Query *activeQuery=nullptr;
struct QueryScope
{
    Query *previous;
    explicit QueryScope(Query &query) : previous(activeQuery) { activeQuery=&query; }
    ~QueryScope() { activeQuery=previous; }
    QueryScope(const QueryScope &)=delete;
    QueryScope &operator=(const QueryScope &)=delete;
};
struct TraceScope
{
    int &trace;
    unsigned char previous[sizeof(int)];
    const bool preserve;
    explicit TraceScope(int &id) : trace(id),preserve(activeQuery!=nullptr)
    { if (preserve) std::memcpy(previous,&trace,sizeof(trace)); }
    ~TraceScope() { if (preserve) std::memcpy(&trace,previous,sizeof(trace)); }
};
}

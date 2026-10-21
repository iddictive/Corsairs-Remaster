
#include "matrix.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include <unordered_map>
using entid_t=uint64_t;
struct TRIANGLE{CVECTOR vrt[3];};struct PLANE{};
struct ATTRIBUTES{bool resident=true;const char*GetAttribute(const char*){return resident?"PuertoRico":"";}};
struct Entity{virtual ~Entity()=default;};
struct MODEL:Entity{CMatrix mtx;float hit=.3f;int updates{},clips{};void Update(){++updates;}float Trace(const CVECTOR&,const CVECTOR&){return hit;}bool GetCollideTriangle(TRIANGLE&t){t.vrt[0]=mtx.Pos();return true;}bool Clip(PLANE*,int,CVECTOR&,float,bool(*fn)(const CVECTOR*,int)){++clips;CVECTOR v=mtx.Pos();return fn(&v,1);} };
struct ISLAND_BASE:Entity{struct WorldSurface{entid_t model;CMatrix authored;};std::vector<WorldSurface>surfaces;auto GetWorldSurfaces(){return surfaces;}};
struct FakeCore{std::unordered_map<entid_t,Entity*>entities;entid_t island=2;Entity*GetEntityPointer(entid_t id){auto i=entities.find(id);return i==entities.end()?nullptr:i->second;}entid_t GetEntityId(const char*){return island;}}core;
struct Models{float hit=.8f;bool VisibleTest(const CVECTOR&,const CVECTOR&){return hit>=1;}float Trace(const CVECTOR&,const CVECTOR&){return hit;}bool GetCollideTriangle(TRIANGLE&t)const{t.vrt[0]=CVECTOR(1.f,0.f,0.f);return hit<=1;}void Clip(PLANE*,int,CVECTOR&,float,bool(*)(const CVECTOR*,int)){} };
struct Location{ATTRIBUTES*AttributesPointer{};Models model;bool hasResidentCollision{},hasResidentTriangle{};TRIANGLE residentTriangle{};ISLAND_BASE*ResidentIsland()const;bool VisibleTest(const CVECTOR&,const CVECTOR&);float Trace(const CVECTOR&,const CVECTOR&);bool GetCollideTriangle(TRIANGLE&)const;void Clip(PLANE*,int,CVECTOR&,float,bool(*)(const CVECTOR*,int));};
ISLAND_BASE *Location::ResidentIsland() const
{
    // The script marks only contexts whose physical environment remains owned
    // by the current naval region. Interiors retain their own model geometry.
    const char *resident = AttributesPointer ? AttributesPointer->GetAttribute("PortResident") : nullptr;
    if (!resident || !resident[0]) return nullptr;
    return dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));
}
template<class Query>
void QueryResidentSurfaces(ISLAND_BASE &island, Query query)
{
    for (const auto &surface : island.GetWorldSurfaces())
    {
        auto *receiver = static_cast<MODEL *>(core.GetEntityPointer(surface.model));
        if (!receiver) continue;
        struct RestoreMatrix
        {
            MODEL &receiver;
            CMatrix visible;
            ~RestoreMatrix() { receiver.mtx = visible; receiver.Update(); }
        } restore{*receiver, receiver->mtx};
        receiver->mtx = surface.authored;
        receiver->Update();
        query(*receiver);
    }
}
bool Location::VisibleTest(const CVECTOR &p1, const CVECTOR &p2)
{
    if (!ResidentIsland()) return model.VisibleTest(p1, p2);
    return Trace(p1, p2) >= 1.0f;
}
float Location::Trace(const CVECTOR &src, const CVECTOR &dst)
{
    float nearest = model.Trace(src, dst);
    auto *island = ResidentIsland();
    hasResidentCollision = island != nullptr;
    if (!island) return nearest;
    hasResidentTriangle = model.GetCollideTriangle(residentTriangle);
    QueryResidentSurfaces(*island, [&](MODEL &receiver) {
        const float hit = receiver.Trace(src, dst);
        if (std::isfinite(hit) && hit >= 0.0f && hit <= 1.0f && hit < nearest)
        {
            nearest = hit;
            hasResidentTriangle = receiver.GetCollideTriangle(residentTriangle);
        }
    });
    return nearest;
}
bool Location::GetCollideTriangle(TRIANGLE &triangle) const
{
    if (!hasResidentCollision) return model.GetCollideTriangle(triangle);
    triangle = residentTriangle;
    return hasResidentTriangle;
}
void Location::Clip(PLANE *planes, int32_t numPlanes, CVECTOR &center, float radius,
                    bool (*callback)(const CVECTOR *, int32_t))
{
    model.Clip(planes, numPlanes, center, radius, callback);
    if (auto *island = ResidentIsland())
        QueryResidentSurfaces(*island, [&](MODEL &receiver) {
            receiver.Clip(planes, numPlanes, center, radius, callback);
        });
}
float clipX=0;bool clip(const CVECTOR*v,int){clipX=v->x;return true;}
int main(){ISLAND_BASE island;MODEL world;core.entities[2]=&island;core.entities[3]=&world;CMatrix authored;authored.Pos()=CVECTOR(100.f,0.f,0.f);world.mtx.Pos()=CVECTOR(200.f,0.f,0.f);island.surfaces.push_back({3,authored});ATTRIBUTES attrs;Location location;location.AttributesPointer=&attrs;TRIANGLE tri{};CVECTOR a(0.f,10.f,0.f),b(0.f,-10.f,0.f);
assert(location.Trace(a,b)==.3f);assert(location.GetCollideTriangle(tri)&&tri.vrt[0].x==100.f);assert(world.mtx.Pos().x==200.f);assert(!location.VisibleTest(a,b));
world.hit=2.f;assert(location.Trace(a,b)==.8f);assert(location.GetCollideTriangle(tri)&&tri.vrt[0].x==1.f);assert(world.mtx.Pos().x==200.f);
world.hit=.1f;assert(location.Trace(a,b)==.1f);location.Clip(nullptr,0,a,3,clip);assert(world.clips==1&&clipX==100.f&&world.mtx.Pos().x==200.f);
attrs.resident=false;assert(location.Trace(a,b)==.8f);assert(location.GetCollideTriangle(tri)&&tri.vrt[0].x==1.f);location.Clip(nullptr,0,a,3,clip);assert(world.clips==1);
std::puts("PASS: actual-source resident adapter nearest own/live island receiver, changed mutable topology, authored triangle/clip frame, matrix restoration, ordinary nonresident negative");}

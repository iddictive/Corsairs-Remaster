#include "touch.h"
#include "ship_contact_frame.hpp"
#include "ship_shore_contact.hpp"

#include "core.h"
#include "collide.h"
#include "model.h"
#include "ship_mast_support.hpp"
#include "ship_damage_trace.hpp"
#include <cstdlib>
#include <limits>
#include <set>
#include <tuple>

#include "character.h"
#include "entity.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"

CREATE_CLASS(TOUCH)

namespace {
// Keep ordinary contact motion and HP while the structural ram format is off.
bool structuralContactEnabled()
{
    static const bool enabled = [] {
        const char *value = std::getenv("STORM_METAL_STRUCTURAL_CONTACT");
        return value && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}
using MastVec=storm_ship_laser::Vec3;
using MastTriangle=storm_ship_laser::Triangle;
MastVec mastVector(const CVECTOR &v){return {v.x,v.y,v.z};}
CVECTOR mastVector(MastVec v){return CVECTOR(v.x,v.y,v.z);}
struct MastSweep {
    std::array<MastVec,4> quad,worldQuad;
    const SHIP_MAST_CONTACT_FRAME *receiver=nullptr;
    double seconds=0.,closing=0.,approachA=0.;
    MastVec point{};
    float time=1.f;
    bool contact=false,previousOverlap=false;
};
// The contact witness must keep clipped polygon edges. MODEL's bounded Clip
// already rejects far receiver hierarchies before enumerating any triangles.
thread_local MastSweep *activeMastSweep=nullptr;
bool mastAxisHit(MastVec a,MastVec b,const MastTriangle &triangle,MastVec &point) {
    using namespace storm_ship_hull;
    const auto raw=cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position);
    if(length(raw)<1.e-12f)return false;const auto normal=normalized(raw);
    const float first=dot(a-triangle[0].position,normal),last=dot(b-triangle[0].position,normal);
    if(std::abs(first-last)>1.e-12f){
        const float time=first/(first-last);if(time<0.f||time>1.f)return false;
        point=a+(b-a)*time;return storm_ship_laser::pointOnFace(point,triangle);
    }
    if(std::abs(first)>storm::ship::mast_support::tolerance||!storm::ship::mast_support::segmentSupported(a,b,triangle))return false;
    point=storm_ship_laser::closestFacePoint((a+b)*.5f,triangle);return true;
}
bool mastSweepPolygon(const CVECTOR *vertices,int32_t count) {
    using namespace storm_ship_hull;
    auto &query=*activeMastSweep;
    CMatrix receiverCurrent=query.receiver->current; // Native vector math has non-const member APIs.
    for(int32_t n=1;n+1<count;++n){
        MastTriangle receiver{};const CVECTOR world[]={vertices[0],vertices[n],vertices[n+1]};
        for(unsigned c=0;c<3;++c){CVECTOR local;receiverCurrent.MulToInv(world[c],local);receiver[c].position=mastVector(local);}
        const auto raw=cross(receiver[1].position-receiver[0].position,receiver[2].position-receiver[0].position);
        if(length(raw)<1.e-12f)continue;const auto normal=normalized(raw);
        MastVec oldPoint;query.previousOverlap|=mastAxisHit(query.quad[0],query.quad[1],receiver,oldPoint);
        for(unsigned half=0;half<2;++half){
            const unsigned corners[2][3]={{0,1,2},{0,2,3}};MastTriangle swept{};
            for(unsigned c=0;c<3;++c)swept[c].position=query.quad[corners[half][c]];
            auto contacts=storm::ship::mast_support::intersection(swept,receiver);
            const auto u=swept[1].position-swept[0].position,v=swept[2].position-swept[0].position;
            const double uu=dot(u,u),uv=dot(u,v),vv=dot(v,v),denominator=uu*vv-uv*uv;
            // Pure axial travel has a degenerate ruled surface. Its swept set
            // is exactly the longest collinear axis interval, not sampled rays.
            if(denominator<=1.e-16&&half==0){
                unsigned first=0,last=1;float distance=0;
                for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const float d=length(query.quad[b]-query.quad[a]);if(d>distance){distance=d;first=a;last=b;}}
                MastVec point;if(mastAxisHit(query.quad[first],query.quad[last],receiver,point))contacts.push_back(point);
            }
            for(auto point:contacts){
                float time=1.f;
                if(denominator>1.e-16){const auto delta=point-swept[0].position;const double du=dot(delta,u),dv=dot(delta,v);
                    const double b=(du*vv-dv*uv)/denominator,c=(dv*uu-du*uv)/denominator;time=float(half?b+c:c);}
                time=std::clamp(time,0.f,1.f);
                const auto base=query.quad[0]+(query.quad[3]-query.quad[0])*time,tip=query.quad[1]+(query.quad[2]-query.quad[1])*time;
                const auto axis=tip-base;const float size=dot(axis,axis);if(size<=1.e-12f)continue;
                const float along=std::clamp(dot(point-base,axis)/size,0.f,1.f);
                const auto travel=(query.quad[3]-query.quad[0])*(1.f-along)+(query.quad[2]-query.quad[1])*along;
                const double closing=std::max(0.,-double(dot(travel,normal))/query.seconds);
                query.contact=true;
                if(closing>query.closing){
                    query.closing=closing;query.point=point;query.time=time;
                    const auto worldTravel=(query.worldQuad[3]-query.worldQuad[0])*(1.f-along)+
                        (query.worldQuad[2]-query.worldQuad[1])*along;
                    CVECTOR localTravel;receiverCurrent.MulToInvNorm(mastVector(worldTravel),localTravel);
                    query.approachA=-double(dot(mastVector(localTravel),normal))/query.seconds;
                }
            }
        }
    }
    return true;
}
}

// TOUCH still owns the early hull impulse solver. Its child sees all final
// ship poses and publishes only after every actual-geometry query completes.
class TOUCH_MASTS final:public Entity {
    using Key=std::tuple<uint64_t,uint64_t,int32_t,uint64_t,uint64_t,uint64_t>;
    std::set<Key> contacts;
  public:
    bool Init() override{return true;}
    void ProcessStage(Stage stage,uint32_t delta) override {
        if(stage!=Stage::execute||!delta)return;
        std::vector<SHIP_MAST_CONTACT_FRAME>frames;
        std::vector<HULL_CONTACT_HANDLING>handling;
        for(const auto id:core.GetEntityIds("ship"))if(auto *ship=dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id))){
            SHIP_MAST_CONTACT_FRAME frame;if(ship->GetMastContactFrame(frame)){
                frames.push_back(std::move(frame));handling.push_back(ship->GetHullContactHandling());
            }
        }
        struct Impact{size_t own,other,mast;CVECTOR point;double work;};
        std::vector<Impact> impacts;std::set<Key>next;
        const double seconds=double(delta)*.001;
        for(size_t own=0;own<frames.size();++own)for(size_t other=0;other<frames.size();++other){
            if(own==other)continue;const auto &a=frames[own],&b=frames[other];
            auto *model=dynamic_cast<MODEL *>(core.GetEntityPointer(b.model));if(!model)continue;
            CMatrix ownPrevious=a.previous,ownCurrent=a.current,receiverPrevious=b.previous,receiverCurrent=b.current;
            for(size_t index=0;index<a.masts.size();++index){const auto &mast=a.masts[index];
                MastSweep query;query.receiver=&b;query.seconds=seconds;
                const CVECTOR endpoints[]={ownPrevious*mast.base,ownPrevious*mast.tip,ownCurrent*mast.tip,ownCurrent*mast.base};
                for(unsigned c=0;c<4;++c){CVECTOR local;(c<2?receiverPrevious:receiverCurrent).MulToInv(endpoints[c],local);query.quad[c]=mastVector(local);query.worldQuad[c]=mastVector(endpoints[c]);}
                // Linear within-frame relative-axis motion. The native Clip
                // hierarchy/BVH selects only triangles in this finite patch.
                CVECTOR lo=receiverCurrent*mastVector(query.quad[0]),hi=lo;
                for(unsigned c=1;c<4;++c){const auto p=receiverCurrent*mastVector(query.quad[c]);for(unsigned axis=0;axis<3;++axis){lo.v[axis]=std::min(lo.v[axis],p.v[axis]);hi.v[axis]=std::max(hi.v[axis],p.v[axis]);}}
                const float epsilon=storm::ship::mast_support::tolerance;lo-=CVECTOR(epsilon);hi+=CVECTOR(epsilon);
                const PLANE planes[6]={{1,0,0,hi.x},{-1,0,0,-lo.x},{0,1,0,hi.y},{0,-1,0,-lo.y},{0,0,1,hi.z},{0,0,-1,-lo.z}};
                const auto center=(lo+hi)*.5f;
                auto *previous=activeMastSweep;activeMastSweep=&query;
                model->Clip(planes,6,center,std::sqrt(~(hi-center)),mastSweepPolygon);activeMastSweep=previous;
                if(!query.contact)continue;
                const Key key{uint64_t(a.ship),uint64_t(a.model),mast.number,uint64_t(b.ship),uint64_t(b.model),mast.generation};next.insert(key);
                if(query.previousOverlap||contacts.count(key)||!(query.closing>0.))continue;
                const double share=b.mass/(a.mass+b.mass);
                // Mast-only contact uses the same admitted speed envelope as
                // hull contact; boosted travel cannot repay its raw motor energy here.
                const double closing=storm::ship::contact::admittedClosingSpeed(query.approachA,
                    query.closing-query.approachA,handling[own].arcade?handling[own].cruiseSpeed:0.,
                    handling[other].arcade?handling[other].cruiseSpeed:0.);
                const double work=.5*closing*closing*share*share*(1.-storm::ship::contact::restitution*storm::ship::contact::restitution);
                const CVECTOR first=receiverPrevious*mastVector(query.point),last=receiverCurrent*mastVector(query.point);
                impacts.push_back({own,other,index,first+(last-first)*query.time,work});
            }
        }
        contacts=std::move(next);
        for(const auto &impact:impacts){const auto &frame=frames[impact.own];
            if(auto *ship=dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(frame.ship)))
                ship->ApplyMastContact(frame,frame.masts[impact.mast],frames[impact.other].ship,impact.point,impact.work);
        }
    }
};
CREATE_CLASS(TOUCH_MASTS)

void TOUCH::SetDevices()
{
    entid_t ent;

    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
}

bool TOUCH::Init()
{
    SetDevices();
    return true;
}

TOUCH::TOUCH()
    : pRS(nullptr), fCollisionDepth(0), iDeltaTime(0), pShips{}
{
    iNumShips = 0;
    fScale = 6.0f;
    dwDeltaTime = 0;
    pIslandBase = nullptr;
    bUseTouch = false;
}

TOUCH::~TOUCH()
{
    if(mastContacts)core.EraseEntity(mastContacts);
    for (int32_t i = 0; i < iNumShips; i++)
        STORM_DELETE(pShips[i]);
}

uint64_t TOUCH::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case MSG_SHIP_RELEASE: {
        const auto shipID = message.EntityID();
        for (int32_t i = 0; i < iNumShips; ++i)
            if (pShips[i]->eID == shipID)
            {
                delete pShips[i];
                --iNumShips;
                pShips[i] = pShips[iNumShips];
                pShips[iNumShips] = nullptr;
                break;
            }
        break;
    }
    case MSG_SHIP_CREATE:
        pShips[iNumShips] = new TOUCH_SHIP{};
        pShips[iNumShips]->eID = message.EntityID();
        iNumShips++;
        break;
        /*                case EVENT_Entity_DELETED:
                  GET_DATA(temp_long,pTemp);    GET_DATA(id,pTemp);
                  for(i=0; i<iNumShips; i++) if(pShips[i]->eID == id)
                  {
                    delete pShips[i];
                    iNumShips--;
                    pShips[i] = pShips[iNumShips];
                    break;
                  }
                break;
              }*/
        break;
    }
    return 1;
}

uint64_t dwRdtsc;
CVECTOR vGlobalRecoil;

void TOUCH::Realize(uint32_t DeltaTime)
{
    DrawShips();
}

void TOUCH::Execute(uint32_t dwCoreDeltaTime)
{
    // GUARD(void TOUCH::Execute(uint32_t dwCoreDeltaTime))
    int32_t i;
    entid_t ent;
    if (!pIslandBase)
        pIslandBase = static_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));

    // std::this_thread::sleep_for(std::chrono::milliseconds(150));

    RDTSC_B(dwRdtsc);

    if (dwCoreDeltaTime == 0)
        return;
    for (i = 0; i < iNumShips; i++)
    {
        pShips[i]->pShip = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[i]->eID));
        if (pShips[i]->pShip && (!pShips[i]->iNumVContour ||
            pShips[i]->contourRevision != pShips[i]->pShip->GetHullContourRevision()))
        {
            pShips[i]->pShip->BuildContour(&pShips[i]->vContour[0], pShips[i]->iNumVContour);
            pShips[i]->pShip->BuildContour(&pShips[i]->vCoastContour[0], pShips[i]->iNumCoastContour, 1.0f);
            pShips[i]->contourRevision = pShips[i]->pShip->GetHullContourRevision();
        }
    }

    if(!mastContacts)for(i=0;i<iNumShips;++i)if(pShips[i]->pShip){
        mastContacts=core.CreateEntity("TOUCH_MASTS");
        if(mastContacts)core.AddToLayer(pShips[i]->pShip->GetMastContactLayer(),mastContacts,std::numeric_limits<priority_t>::max());
        break;
    }

    iDeltaTime = dwCoreDeltaTime;
    for (i = 0; i < iNumShips; ++i) TouchIsland(i);
    ResolveShips();

    // if (core.Controls->GetAsyncKeyState('5')) fScale -= 0.1f;
    // if (core.Controls->GetAsyncKeyState('6')) fScale += 0.1f;

    RDTSC_E(dwRdtsc);
}

CVECTOR TOUCH::GetPoint(float x, float y, float xx, float yy, float xscale, float yscale, float fCos, float fSin,
                        storm_render::Point ss)
{
    CVECTOR vPos;
    vPos.x = xscale * xx;
    vPos.z = yscale * yy;
    RotateAroundY(vPos.x, vPos.z, fCos, fSin);
    vPos.x += x + ss.x / 2;
    vPos.z = -vPos.z + y + ss.y / 2;
    return vPos;
}

void TOUCH::DrawLine(std::vector<RS_LINE2D> &aLines, float x1, float y1, float x2, float y2, uint32_t dwColor)
{
    RS_LINE2D l1, l2;
    l1.rhw = 0.5f;
    l2.rhw = 0.5f;
    l1.dwColor = dwColor;
    l2.dwColor = dwColor;
    l1.vPos = CVECTOR(x1, y1, 1.0f);
    l2.vPos = CVECTOR(x2, y2, 1.0f);

    aLines.emplace_back(l1);
    aLines.emplace_back(l2);
}

void TOUCH::DrawShips()
{
    int32_t i, j;
    CVECTOR p1, p2;
    storm_render::Point ss;
    std::vector<RS_LINE2D> aLines;

    ss.x = 800;
    ss.y = 600;

    auto *pOur = static_cast<SHIP_BASE *>(pShips[0]->pShip);
    if (!pOur)
        return;
    const auto center_x = pOur->State.vPos.x;
    const auto center_z = pOur->State.vPos.z;
    const auto delta = pOur->State.vAng.y - PI;
    const auto fCosCenter = cosf(delta);
    const auto fSinCenter = sinf(delta);

    for (i = 0; i < iNumShips; i++)
        if (pShips[i])
        {
            auto *o3d = static_cast<SHIP_BASE *>(pShips[i]->pShip);
            pShips[i]->TP[0].vPos = o3d->State.vPos;
            pShips[i]->TP[0].vAng = o3d->State.vAng;
            BuildContour(i);
            auto x = fScale * -(o3d->State.vPos.x - center_x);
            auto y = fScale * (o3d->State.vPos.z - center_z);
            RotateAroundY(x, y, fCosCenter, fSinCenter);
            const auto fCos = cosf((i == 0) ? 0.0f : -delta - PI + o3d->State.vAng.y);
            const auto fSin = sinf((i == 0) ? 0.0f : -delta - PI + o3d->State.vAng.y);
            const auto xscale = fScale;
            const auto yscale = fScale;

            if (pShips[i]->vKickPoint.x != 0.0f || pShips[i]->vKickPoint.z != 0.0f)
            {
                p1 = GetPoint(x, y, pShips[i]->vKickPoint.x, pShips[i]->vKickPoint.z, fScale, fScale, fCos, fSin, ss);
                p2 = GetPoint(x, y, 0.0f, 0.0f, fScale, fScale, fCos, fSin, ss);
                DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, 0xFFFFFF);
                // p1 = pShips[i]->vKickAngle;
                // p1 = GetPoint(x,y,pShips[i]->vKick.x,pShips[i]->vKick.z,fScale,fScale,fCos,fSin,ss);
                // DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, 0xFF0000);
            }
            // DrawLine(aLines, p1.x-0.5f, p1.z, p1.x+0.5f, p1.z, 0xFFFFFF);

            for (j = 0; j < pShips[i]->iNumVContour; j++)
            {
                p1 = GetPoint(x, y, pShips[i]->vContour[j].x, pShips[i]->vContour[j].z, xscale, yscale, fCos, fSin, ss);
                const auto nextj = (j == pShips[i]->iNumVContour - 1) ? 0 : j + 1;
                p2 = GetPoint(x, y, pShips[i]->vContour[nextj].x, pShips[i]->vContour[nextj].z, xscale, yscale, fCos,
                              fSin, ss);

                uint32_t dwColor = 0x00FF1F;
                DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, dwColor);
                float fRes;
                // dwColor = 0xFFFFFF;
                if (pIslandBase &&
                    pIslandBase->GetDepth(pShips[i]->vContourTemp[0][j].x, pShips[i]->vContourTemp[0][j].z, &fRes))
                {
                    if (fRes == 0.0f)
                        dwColor = 0xFFFF00;
                }
                DrawLine(aLines, p1.x - 0.5f, p1.z, p1.x + 0.5f, p1.z, dwColor);
                DrawLine(aLines, p2.x - 0.5f, p2.z, p2.x + 0.5f, p2.z, dwColor);
            }
        }

    if (aLines.size())
        pRS->DrawLines2D(&aLines[0], aLines.size() / 2, "Line");
}

BOOL TOUCH::BuildContour(int32_t ship_idx)
{
    int32_t i, j;

    auto *pTS = pShips[ship_idx];

    for (j = 0; j < 2; j++)
    {
        const auto fCos = cosf(pTS->TP[j].vAng.y);
        const auto fSin = sinf(pTS->TP[j].vAng.y);
        const auto x = pTS->TP[j].vPos.x;
        const auto z = pTS->TP[j].vPos.z;
        for (i = 0; i < pTS->iNumVContour; i++)
        {
            auto xx = pTS->vContour[i].x;
            auto zz = pTS->vContour[i].z;
            RotateAroundY(xx, zz, fCos, fSin);
            pTS->vContourTemp[j][i].x = x + xx;
            pTS->vContourTemp[j][i].z = z + zz;
        }
    }
    return true;
}

BOOL TOUCH::IsIntersectIsland(int32_t idx, IslandContact &contact)
{
    auto *ship = pShips[idx];
    if (iDeltaTime <= 0 || !pIslandBase || !ship->pShip || ship->pShip->HasRigidFlight() ||
        !ship->pShip->TouchMove(iDeltaTime, &ship->TP[0], &ship->TP[1]))
        return false;
    const bool wet = ship->pShip->HasWaterContact();
    std::vector<CVECTOR> contour;
    float lower = 0.f, upper = 0.f;
    if (!wet && !ship->pShip->GetHullContactContour(contour, lower, upper)) return false;
    const int count = wet ? ship->iNumCoastContour : int(contour.size());
    bool found = false;
    unsigned contacts = 0;
    for (int point = 0; point < count; ++point)
        for (int level = 0; level < (wet ? 1 : 3); ++level)
        {
            CVECTOR ends[2];
            for (int pose = 0; pose < 2; ++pose)
            {
                const auto local = wet ? ship->vCoastContour[point] : contour[point];
                float x = local.x, z = local.z;
                RotateAroundY(x, z, cosf(ship->TP[pose].vAng.y), sinf(ship->TP[pose].vAng.y));
                const float y = wet ? 0.f : lower + (upper - lower) * (.5f * level) +
                    ship->TP[pose].vPos.y - ship->TP[0].vPos.y;
                ends[pose] = CVECTOR(ship->TP[pose].vPos.x + x, y, ship->TP[pose].vPos.z + z);
            }
            const CVECTOR travel = ends[1] - ends[0];
            if (travel.x == 0.f && travel.y == 0.f && travel.z == 0.f) continue;
            const auto pointAt = [&](float time)
            {
                const auto local = wet ? ship->vCoastContour[point] : contour[point];
                const float yaw = ship->TP[0].vAng.y +
                    (ship->TP[1].vAng.y - ship->TP[0].vAng.y) * time;
                float x = local.x, z = local.z;
                RotateAroundY(x, z, cosf(yaw), sinf(yaw));
                const auto center = ship->TP[0].vPos + (ship->TP[1].vPos - ship->TP[0].vPos) * time;
                return CVECTOR(center.x + x, ends[0].y + travel.y * time, center.z + z);
            };

            // Query the same published island mesh as weapons and grounding.
            // GetCollideTriangle belongs to the actual traced model, not ISLAND's stub.
            float time = pIslandBase->Trace(ends[0], ends[1]);
            CVECTOR normal(0.f);
            bool initialDepthOverlap = false;
            const bool exact = std::isfinite(time) && time >= 0.f && time <= 1.f;
            if (exact)
            {
                auto *collision = static_cast<COLLIDE *>(core.GetService("coll"));
                auto *model = collision ? static_cast<COLLISION_OBJECT *>(
                    core.GetEntityPointer(collision->GetObjectID())) : nullptr;
                TRIANGLE triangle;
                if (model && model->GetCollideTriangle(triangle))
                    normal = -((triangle.vrt[1] - triangle.vrt[0]) ^ (triangle.vrt[2] - triangle.vrt[0]));
            }
            else if (!wet || !pIslandBase->CheckDepthCollision(ends[0], ends[1], fCollisionDepth, 0.f))
                continue;

            if (wet && !exact)
            {
                constexpr int depthSteps = 12;
                // Refine the existing depth barrier; never push an already
                // overlapping hull out by a fixed distance or rotate its pose.
                float safe = 0.f, blocked = 1.f;
                if (pIslandBase->CheckDepthCollision(ends[0], ends[0], fCollisionDepth, 0.f))
                {
                    initialDepthOverlap = true;
                    // A coarse-cell gradient can oppose the actual route to
                    // deeper water. Admit only sampled monotone retreat from
                    // an overlap, rejecting mesh hits on the sampled segments.
                    float startDepth, endDepth;
                    bool retreat = pIslandBase->GetCurrentImmersion() == 0.f &&
                        pIslandBase->GetGroundDepth(ends[0].x, ends[0].z, ends[0].y, startDepth) &&
                        std::isfinite(startDepth) &&
                        pIslandBase->GetGroundDepth(ends[1].x, ends[1].z, ends[1].y, endDepth) &&
                        std::isfinite(endDepth) && endDepth < startDepth;
                    CVECTOR previous = ends[0];
                    float previousDepth = retreat ? startDepth : 0.f;
                    for (int step = 1; retreat && step <= depthSteps; ++step)
                    {
                        const auto next = pointAt(float(step) / float(depthSteps));
                        float depth;
                        const float crossing = pIslandBase->Trace(previous, next);
                        retreat = std::isfinite(crossing) && crossing > 1.f &&
                            pIslandBase->GetGroundDepth(next.x, next.z, next.y, depth) &&
                            std::isfinite(depth) && depth <= previousDepth;
                        if (retreat) previousDepth = depth;
                        previous = next;
                    }
                    if (retreat) continue;
                    blocked = 0.f;
                }
                else for (int iteration = 0; iteration < depthSteps; ++iteration)
                {
                    const float middle = (safe + blocked) * .5f;
                    if (pIslandBase->CheckDepthCollision(ends[0], pointAt(middle), fCollisionDepth, 0.f))
                        blocked = middle;
                    else safe = middle;
                }
                time = safe;
            }
            CVECTOR hit = pointAt(time);
            normal.y = 0.f;
            if (~normal < .000001f && initialDepthOverlap)
            {
                auto *collision = static_cast<COLLIDE *>(core.GetService("coll"));
                storm::ship::shore::embeddedNormal(pIslandBase, collision, ends[0],
                    std::max(2.f, ship->pShip->GetBoxsize().z), normal);
            }
            if (~normal < .000001f && wet)
            {
                // The depth field supplies the coast normal when a shallow
                // raster cell has no horizontal triangle crossing. Expanding
                // central differences also cover a cell-sized initial overlap.
                for (float step = .5f; step <= std::max(2.f, ship->pShip->GetBoxsize().z); step *= 2.f)
                {
                    float xp = -500.f, xm = -500.f, zp = -500.f, zm = -500.f;
                    pIslandBase->GetGroundDepth(hit.x + step, hit.z, 0.f, xp);
                    pIslandBase->GetGroundDepth(hit.x - step, hit.z, 0.f, xm);
                    pIslandBase->GetGroundDepth(hit.x, hit.z + step, 0.f, zp);
                    pIslandBase->GetGroundDepth(hit.x, hit.z - step, 0.f, zm);
                    normal = CVECTOR(xp - xm, 0.f, zp - zm);
                    if (~normal > .000001f) break;
                }
            }
            if (!std::isfinite(normal.x) || !std::isfinite(normal.z) || ~normal <= .000001f) continue;
            normal = !normal;
            if (exact)
            {
                const auto planePoint = ends[0] + travel * time;
                if (((ends[0] - planePoint) | normal) <= 0.f &&
                    ((ends[1] - planePoint) | normal) > 0.f)
                {
                    // A rotating bow follows an arc, not its endpoint chord.
                    // Refine that same face on the separated side of the arc.
                    float safe = 0.f, blocked = 1.f;
                    for (int iteration = 0; iteration < 16; ++iteration)
                    {
                        const float middle = (safe + blocked) * .5f;
                        if (((pointAt(middle) - planePoint) | normal) > 0.f) blocked = middle;
                        else safe = middle;
                    }
                    time = safe;
                    hit = pointAt(time);
                }
            }
            const auto center = ship->TP[0].vPos + (ship->TP[1].vPos - ship->TP[0].vPos) * time;
            const auto lever = hit - center;
            const auto linear = (ship->TP[1].vPos - ship->TP[0].vPos) * (1000.f / float(iDeltaTime));
            const float angular = (ship->TP[1].vAng.y - ship->TP[0].vAng.y) * (1000.f / float(iDeltaTime));
            const float closing = ((linear + CVECTOR(angular * lever.z, 0.f, -angular * lever.x)) | normal);
            // Separating and tangential movement must remain available even
            // when a saved hull starts inside a shore cell.
            if (!(closing > .0001f)) continue;
            if (!found || time < contact.time)
            {
                contact.point = hit;
                contact.normal = normal;
                contact.time = time;
                contact.closing = closing;
                found = true;
                contacts = 1;
            }
            else if (std::abs(time - contact.time) < .00001f && (normal | contact.normal) > .99f)
            {
                // Simultaneous points on a flat bow/side share one resultant;
                // contour enumeration must not choose an arbitrary torque arm.
                contact.point += (hit - contact.point) / float(++contacts);
                contact.closing = std::max(contact.closing, closing);
            }
        }
    return found;
}

int32_t TOUCH::GetTouchPoint(int32_t iIdx, const CVECTOR &vPos)
{
    return 1;
}

void TOUCH::TouchIsland(int32_t idx)
{
    auto *ship = pShips[idx]->pShip;
    if (!ship || ship->isDead()) return;
    IslandContact contact;
    if (!IsIntersectIsland(idx, contact)) return;
    const auto impact = contact;
    auto localPoint = impact.point - pShips[idx]->TP[0].vPos;
    auto localNormal = impact.normal;
    const float yaw = -pShips[idx]->TP[0].vAng.y;
    RotateAroundY(localPoint.x, localPoint.z, cosf(yaw), sinf(yaw));
    RotateAroundY(localNormal.x, localNormal.z, cosf(yaw), sinf(yaw));
    const auto accepted = [&] {
        storm_ship_damage_trace::Phase timing("contact-island-event", uint64_t(ship->GetId()));
        return core.Event(SHIP_SHIP2ISLAND_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()), -1,
            METERS2KNOTS(impact.closing), 0.f, GetTouchPoint(idx, localPoint),
            impact.point.x, impact.point.y, impact.point.z);
    }();
    const float fraction = accepted && std::isfinite(accepted->GetFloat()) ?
        std::clamp(accepted->GetFloat(), 0.f, 1.f) : 0.f;
    ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[idx]->eID));
    if (!ship || ship->isDead()) return;

    // Reuse the hull impulse owner with an immovable shore: weak restitution,
    // signed r x J / I, friction and nonincreasing kinetic energy.
    const float seconds = float(iDeltaTime) * .001f;
    storm::ship::contact::Body body;
    std::vector<CVECTOR> contour;
    float lower, upper;
    if (ship->HasWaterContact())
        contour.assign(pShips[idx]->vCoastContour, pShips[idx]->vCoastContour + pShips[idx]->iNumCoastContour);
    else if (!ship->GetHullContactContour(contour, lower, upper)) return;
    for (const auto &point : contour) body.hull.push_back({point.x, point.z});
    body.mass = ship->GetHullContactMass();
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        const auto &before = pShips[idx]->TP[0];
        const auto &after = pShips[idx]->TP[1];
        CVECTOR travel = after.vPos - before.vPos;
        travel.y = 0.f;
        const float turn = after.vAng.y - before.vAng.y;
        body.start = {{before.vPos.x, before.vPos.z}, before.vAng.y};
        body.end = {{after.vPos.x, after.vPos.z}, after.vAng.y};
        const storm::ship::contact::Contact query{true, contact.time,
            {contact.point.x, contact.point.z}, {contact.normal.x, contact.normal.z}};
        const auto response = storm::ship::contact::resolve(body, {}, query, seconds, true);
        if (!(response.normalClosing > 0)) break;
        const CVECTOR delta(float(response.deltaVA.x), 0.f, float(response.deltaVA.z));
        const CVECTOR velocity = travel / seconds + delta;
        const CVECTOR endpoint = before.vPos + travel * contact.time + velocity * (seconds * (1.f - contact.time));
        const float endYaw = before.vAng.y + turn * contact.time +
            (turn / seconds + float(response.deltaWA)) * (seconds * (1.f - contact.time));
        ship->ApplyHullContactMotion(delta, float(response.deltaWA), endpoint, endYaw, uint32_t(iDeltaTime));
        contact = IslandContact{};
        if (!IsIntersectIsland(idx, contact)) break;
        if (iteration == 3)
        {
            // Impact restitution/friction has spent its budget. Constrain only
            // the remaining inward point velocity; do not erase the tangent or
            // separating rotation with a start-pose stop.
            const auto &start = pShips[idx]->TP[0];
            const auto &end = pShips[idx]->TP[1];
            body.start = {{start.vPos.x, start.vPos.z}, start.vAng.y};
            body.end = {{end.vPos.x, end.vPos.z}, end.vAng.y};
            const storm::ship::contact::Twist desired{
                {(end.vPos.x - start.vPos.x) / seconds, (end.vPos.z - start.vPos.z) / seconds},
                (end.vAng.y - start.vAng.y) / seconds};
            const storm::ship::contact::Contact remaining{true, contact.time,
                {contact.point.x, contact.point.z}, {contact.normal.x, contact.normal.z}};
            const auto constraint = storm::ship::contact::constrainNormal(body, remaining, desired);
            const auto constrained = constraint.apply(desired);
            const CVECTOR delta(float(constrained.linear.x - desired.linear.x), 0.f,
                                float(constrained.linear.z - desired.linear.z));
            const CVECTOR endpoint(start.vPos.x + float(constrained.linear.x * seconds), start.vPos.y,
                                   start.vPos.z + float(constrained.linear.z * seconds));
            const float yaw = start.vAng.y + float(constrained.angular * seconds);
            ship->ApplyHullContactMotion(delta, float(constrained.angular - desired.angular),
                                        endpoint, yaw, uint32_t(iDeltaTime));
            if (IsIntersectIsland(idx, contact))
            {
                // Re-query all contour points after projection: another face
                // may still block a corner. Retain its certified native-arc
                // prefix rather than unconditionally returning to the start.
                const auto before = pShips[idx]->TP[0], after = pShips[idx]->TP[1];
                const float prefix = std::max(0.f, contact.time - .00001f);
                const CVECTOR travel = after.vPos - before.vPos;
                const float turn = after.vAng.y - before.vAng.y;
                const CVECTOR velocity(travel.x / seconds, 0.f, travel.z / seconds);
                ship->ApplyHullContactMotion(velocity * (prefix - 1.f),
                    turn / seconds * (prefix - 1.f), before.vPos + travel * prefix,
                    before.vAng.y + turn * prefix, uint32_t(iDeltaTime));
            }
        }
    }
    // The existing shore event still owns HP/perks. Only admitted inward work
    // reaches the same finite material-fracture owner as ship/ship impacts.
    ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[idx]->eID));
    if (structuralContactEnabled() && ship && fraction > 0.f && impact.closing > 0.f)
    {
        double work = .5 * double(impact.closing) * impact.closing;
        if (fraction < 1.f)
        {
            const double strength = ship->GetHullContactStrength(localPoint.z, localNormal.z);
            const double admitted = (work / (work + strength)) * fraction;
            work = std::min(work, strength * admitted / (1.0 - admitted));
        }
        ship->ApplyHullImpact(impact.point, impact.normal, float(work), -1);
    }
}

void TOUCH::ResolveShips()
{
    using namespace storm::ship::contact;
    const bool structuralContact = structuralContactEnabled();
    const double seconds = double(iDeltaTime) * .001;
    if (!(seconds > 0)) return;
    std::vector<FrameBody> bodies;
    std::vector<int> indices;
    std::vector<HULL_CONTACT_MATERIAL> materials;
    for (int i = 0; i < iNumShips; ++i)
    {
        auto *entry = pShips[i];
        if (!entry->pShip || entry->pShip->HasRigidFlight() || !entry->pShip->TouchMove(iDeltaTime, &entry->TP[0], &entry->TP[1])) continue;
        FrameBody frame;
        if (entry->pShip->isDead() || entry->pShip->GetHullContourRevision())
        {
            std::vector<CVECTOR> contour;
            float lower, upper;
            if (!entry->pShip->GetHullContactContour(contour, lower, upper)) continue;
            for (const auto &point : contour) frame.body.hull.push_back({point.x, point.z});
            frame.yStart = (lower + upper) * .5;
            frame.yEnd = frame.yStart + entry->TP[1].vPos.y - entry->TP[0].vPos.y;
            frame.height = std::max(.001f, upper - lower);
        }
        else
        {
            if (entry->iNumCoastContour < 3 || entry->iNumCoastContour > 128) continue;
            for (int n = 0; n < entry->iNumCoastContour; ++n)
                frame.body.hull.push_back({entry->vCoastContour[n].x, entry->vCoastContour[n].z});
            frame.yStart = entry->TP[0].vPos.y;
            frame.yEnd = entry->TP[1].vPos.y;
            frame.height = entry->pShip->GetBoxsize().y;
        }
        frame.body.mass = entry->pShip->GetHullContactMass();
        frame.body.start = {{entry->TP[0].vPos.x, entry->TP[0].vPos.z}, entry->TP[0].vAng.y};
        frame.body.end = {{entry->TP[1].vPos.x, entry->TP[1].vPos.z}, entry->TP[1].vAng.y};
        frame.wreck = entry->pShip->isDead();
        const auto handling = entry->pShip->GetHullContactHandling();
        frame.contactSpeedLimit = handling.arcade ? handling.cruiseSpeed : 0.;
        frame.contactTurnLimit = handling.turnLimit;
        frame.contactCruiseSpeed = handling.cruiseSpeed;
        HULL_CONTACT_MATERIAL material;
        if (structuralContact && entry->pShip->GetHullContactMaterial(material))
            frame.material = material.prepared;
        frame.materialY = entry->TP[0].vPos.y - frame.yStart;
        bodies.push_back(std::move(frame));
        indices.push_back(i);
        materials.push_back(std::move(material));
    }
    if (bodies.size() < 2) return;
    const auto currentShip = [&](size_t n)
    {
        return static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[indices[n]]->eID));
    };
    const auto fracture = [&](const FractureEvent &event)
    {
        FractureCommit committed;
        std::vector<std::pair<size_t,std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>>>transferred;
        const auto attempt = [&](size_t own, size_t other, Pose pose, Vec2 outward,
                                  double available, bool sideA)
        {
            auto *ship = currentShip(own); auto *opponent = currentShip(other);
            if (!ship || !opponent || !bodies[own].material || !materials[own].prepared) return;
            const double mass = bodies[own].body.mass;
            constexpr double elasticEnergy = .25;
            const double specific = std::max(0.0, available / mass - elasticEnergy);
            if (!(specific > 0)) return;
            const auto localPoint = detail::rotate(event.contact.point - pose.position, -pose.yaw);
            const auto localNormal = detail::rotate(outward, -pose.yaw);
            const Vec2 along{outward.z, -outward.x};
            double loAlong = -INFINITY, hiAlong = INFINITY;
            for (const auto side : {event.a, event.b})
            {
                const Pose at = side == event.a ? event.poseA : event.poseB;
                double lo = INFINITY, hi = -INFINITY;
                for (const auto p : bodies[side].body.hull)
                {
                    const double t = dot(detail::rotate(p, at.yaw) + at.position - event.contact.point, along);
                    lo = std::min(lo, t); hi = std::max(hi, t);
                }
                loAlong = std::max(loAlong, lo); hiAlong = std::min(hiAlong, hi);
            }
            double loY = -INFINITY, hiY = INFINITY;
            for (const auto side : {event.a, event.b})
            {
                const auto &b = bodies[side];
                const double center = b.yStart + (b.yEnd - b.yStart) * (event.elapsed / seconds);
                loY = std::max(loY, center - b.height * .5 - event.height);
                hiY = std::min(hiY, center + b.height * .5 - event.height);
            }
            if (!(hiAlong - loAlong > .001) || !(hiY - loY > .001)) return;
            double normalLow = INFINITY, normalHigh = -INFINITY;
            for (const auto p : bodies[own].body.hull)
            { const double d = dot(p, localNormal); normalLow = std::min(normalLow, d); normalHigh = std::max(normalHigh, d); }
            HULL_CONTACT_BREAK request;
            const auto *entry = pShips[indices[own]];
            const double originY = entry->TP[0].vPos.y +
                (entry->TP[1].vPos.y - entry->TP[0].vPos.y) * (event.elapsed / seconds);
            request.point = CVECTOR(float(localPoint.x), float(event.height - originY), float(localPoint.z));
            request.normal = CVECTOR(float(localNormal.x), 0.f, float(localNormal.z));
            request.crossSection = {{{float(loY), float(loAlong)}}, {{float(hiY), float(loAlong)}},
                                    {{float(hiY), float(hiAlong)}}, {{float(loY), float(hiAlong)}}};
            request.depth = float(normalHigh - normalLow + .05);
            request.specificWork = specific;
            request.otherCharacterIndex = GetIndex(opponent->GetACharacter());
            request.expectedRevision = materials[own].revision;
            request.eventWorldPosition = CVECTOR(float(pose.position.x), float(originY), float(pose.position.z));
            request.eventWorldYaw = float(pose.yaw);
            const auto velocity=sideA?event.velocityA:event.velocityB;
            request.eventWorldVelocity=CVECTOR(float(velocity.x),
                float((entry->TP[1].vPos.y-entry->TP[0].vPos.y)/seconds),float(velocity.z));
            request.eventYawVelocity=float(sideA?event.angularA:event.angularB);
            HULL_CONTACT_BREAK_RESULT result;
            if (!ship->CommitHullContactBreak(request, result)) return;
            const auto projected = result.material.prepared;
            if (!projected || !std::isfinite(result.spentSpecificWork) || result.spentSpecificWork < 0 ||
                result.spentSpecificWork > specific) return;
            materials[own] = result.material;
            bodies[own].material = projected;
            bodies[own].body.mass=result.retainedMass;
            transferred.push_back({own,result.physicalDelta});
            committed.complete = true;
            if (sideA)
            { committed.workA = result.spentSpecificWork * mass; committed.replaceA = true; committed.materialA = projected;committed.massA=result.retainedMass; }
            else
            { committed.workB = result.spentSpecificWork * mass; committed.replaceB = true; committed.materialB = projected;committed.massB=result.retainedMass; }
        };
        attempt(event.a, event.b, event.poseA, event.contact.normal, event.proposed.energyLostA, true);
        attempt(event.b, event.a, event.poseB, -event.contact.normal, event.proposed.energyLostB, false);
        committed.applyTransferredImpulse=[transferred=std::move(transferred),&currentShip,a=event.a](const Response&r){
            for(const auto&entry:transferred)if(auto*ship=currentShip(entry.first)){
                const auto dv=entry.first==a?r.deltaVA:r.deltaVB;
                const double dw=entry.first==a?r.deltaWA:r.deltaWB;
                ship->ApplyHullContactBodyImpulse(entry.second,CVECTOR(float(dv.x),0.f,float(dv.z)),float(dw));
            }
        };
        return committed;
    };
    FrameLimits limits;
    // One strong ram can cross several real timber sections. The material
    // transaction already bounds its admitted fields; three unrelated passes
    // must not turn the rest of that same impact back into an intact wall.
    limits.fracturePasses=structuralContact ? storm_hull_damage::MaxBeamSamples : 0;
    const auto result = integrateFrame(bodies, seconds, limits,
        structuralContact ? FractureCallback(fracture) : FractureCallback{});
    // Publish material loss at the actual local contact, using the currently
    // presented model transform. A fast TOI may be far from that old transform.
    for (const auto &impact : result.impacts)
    {
        const auto publish = [&](size_t own, size_t other, Vec2 localPoint, Vec2 localNormal,
                                  double specificEnergy, double physicalPaidSpecific)
        {
            auto *ship = currentShip(own);
            auto *opponent = currentShip(other);
            // Do not repeatedly charge elastic low-speed contacts as rams.
            // Only closing energy dissipated by the solver can damage material;
            // continuing propulsion supplies no separate pressure-damage rate.
            constexpr double elasticEnergy = .25; // J per unit mass, m²/s²
            specificEnergy = std::max(0.0, specificEnergy - elasticEnergy);
            if (!ship || !opponent || !(specificEnergy > 0)) return;
            const CVECTOR point(float(localPoint.x), 0.f, float(localPoint.z));
            const CVECTOR normal(float(localNormal.x), 0.f, float(localNormal.z));
            const int owner = GetIndex(opponent->GetACharacter());
            double materialWork = specificEnergy;
            if (!ship->isDead())
            {
                const auto attributes = ship->GetAShip();
                const float maximum = attributes ? attributes->GetAttributeAsFloat("HP") : 0.f;
                if (!(maximum > 0.f) || !std::isfinite(maximum)) return;
                // Specific crush energy grows with closing speed squared; the bow's
                // axial structure resists more than planking hit across the grain.
                const double strength = ship->GetHullContactStrength(localPoint.z, localNormal.z);
                const double damageFraction = specificEnergy / (specificEnergy + strength);
                const float hp = float(maximum * damageFraction);
                const auto accepted = [&] {
                    storm_ship_damage_trace::Phase timing("contact-ship-event", uint64_t(ship->GetId()));
                    return core.Event(SHIP_SHIP2SHIP_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()),
                               owner, hp * .1f, 0.f, 1,
                               float(impact.point.x), float(impact.height),
                               float(impact.point.z));
                }();
                if (accepted)
                {
                    const float value = accepted->GetFloat();
                    const double scale = std::isfinite(value) ? std::clamp(double(value), 0.0, 1.0) : 0.0;
                    if (scale < 1.0)
                    {
                        // The same Phonk budget protects both HP and material.
                        // Convert accepted hull damage back through the existing
                        // impact curve; scaling saturated impact energy is unsafe.
                        const double fraction = damageFraction * scale;
                        materialWork = std::min(materialWork, strength * fraction / (1.0 - fraction));
                    }
                }
            }
            // The contact-step snapshot already owns this paid fracture work.
            // Only uncommitted residual work may enter asynchronous damage.
            materialWork = std::max(0.0, materialWork - physicalPaidSpecific);
            ship = currentShip(own);
            if (!ship || !(materialWork > 0)) return;
            const auto matrix = ship->GetMatrix();
            if (structuralContact && matrix)
            {
                auto worldPoint = *matrix * point;
                worldPoint.y = float(impact.height);
                ship->ApplyHullImpact(worldPoint, *matrix * normal - matrix->Pos(),
                                      float(materialWork), owner);
            }
        };
        publish(impact.a, impact.b, impact.localPointA, impact.localNormalA, impact.specificEnergyA, impact.physicalPaidSpecificA);
        publish(impact.b, impact.a, impact.localPointB, impact.localNormalB, impact.specificEnergyB, impact.physicalPaidSpecificB);
    }
    for (size_t n = 0; n < result.motions.size(); ++n)
    {
        const auto &motion = result.motions[n];
        if (!motion.touched) continue;
        auto *ship = currentShip(n);
        if (!ship) continue;
        ship->ApplyHullContactMotion(CVECTOR(float(motion.deltaV.x), 0.f, float(motion.deltaV.z)),
            float(motion.deltaW), CVECTOR(float(motion.finalPose.position.x), 0.f, float(motion.finalPose.position.z)),
            float(motion.finalPose.yaw), uint32_t(iDeltaTime));
    }
}

uint32_t TOUCH::AttributeChanged(ATTRIBUTES *pAttribute)
{
    if (*pAttribute == "CollisionDepth")
    {
        fCollisionDepth = pAttribute->GetAttributeAsFloat();
        return 0;
    }
    return 0;
}

#include "ship_phonk_laser.hpp"
#include "ship_debris_bridge.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <limits>
#include <thread>
using storm::ship::phonk::LaserFragment;
namespace fc=storm::ship::fragment_contact;
namespace sl=storm_ship_laser;
using entid_t=uint64_t;using layer_index_t=int;using priority_t=int;
constexpr int ISLAND_TRACE=3;
float KNOTS2METERS(float x){return x*.514444f;}
struct PLANE{float x,y,z,d;};
struct Entity{enum class Stage{execute,realize,other};virtual bool Init(){return true;}virtual ~Entity()=default;virtual void ProcessStage(Stage,uint32_t){}entid_t GetId(){return 500;}};
#define CREATE_CLASS(X)
struct GEOS{
 uint64_t generation=1,owner=7,next=100;std::map<uint64_t,sl::FragmentPiece>material;unsigned released=0,captureCalls=0,replaceCalls=0;bool failInput=false,failGPU=false;std::function<void()>onRelease;
 uint64_t GeometryGeneration(){return generation;}
 void ReleaseLaserFragment(uint64_t o,uint64_t token){assert(o==owner);if(onRelease)onRelease();released+=material.erase(token);}
 bool TraceLaserFragment(uint64_t o,uint64_t g,uint64_t token,const sl::Vec3&a,const sl::Vec3&b,float&f,sl::Vec3&n){auto i=material.find(token);return o==owner&&g==generation&&i!=material.end()&&sl::traceFragment(i->second,a,b,f,n);}
 bool CaptureFragmentInput(uint64_t o,uint64_t g,uint64_t token,const sl::Plane&p,unsigned limit,sl::FragmentInput&out){++captureCalls;auto i=material.find(token);if(failInput||o!=owner||g!=generation||i==material.end())return false;out.owner=o;out.generation=g;out.token=token;out.successorLimit=limit;out.input.surface=i->second.surface;out.input.plane=p;out.input.fragmentLimit=limit;return true;}
 bool ReplaceLaserFragment(uint64_t o,sl::FragmentProjection&&p,std::vector<sl::Fragment>&out){++replaceCalls;if(failGPU||o!=owner||p.owner!=owner||p.generation!=generation||!material.contains(p.token))return false;std::vector<std::pair<uint64_t,sl::FragmentPiece>>staged;for(auto&v:p.successors){auto id=++next;out.push_back(v.properties);out.back().fragment=id;staged.push_back({id,std::move(v)});}material.erase(p.token);for(auto&v:staged)material.emplace(v.first,std::move(v.second));return true;}
};
struct NODE{GEOS*geo;};
struct MODEL{CMatrix mtx;GEOS geo;NODE node{&geo};storm_ship_hull::Bounds box{{900,900,900},{901,901,901}};std::vector<std::array<CVECTOR,3>>triangles;unsigned clips=0;
 NODE*GetNode(int n){return n?nullptr:&node;}NODE*FindNode(const char*){return &node;}
 void Clip(const PLANE*,int,CVECTOR,float,bool(*polygon)(const CVECTOR*,int32_t)){++clips;for(auto&t:triangles)if(!polygon(t.data(),3))break;}
};
struct SEA_BASE{float WaveXZ(float,float){return -1000.f;}};
struct Core{std::vector<entid_t>ships,islands;std::map<entid_t,void*>entities;std::map<entid_t,int>erased;entid_t serial=10000;
 const std::vector<entid_t>&GetEntityIds(const char*){return ships;}const std::vector<entid_t>&GetEntityIds(int layer){return islands;}
 void*GetEntityPointer(entid_t id){auto i=entities.find(id);return i==entities.end()?nullptr:i->second;}
 entid_t GetEntityId(const char*){return 0;}entid_t CreateEntity(const char*){return ++serial;}void EraseEntity(entid_t id){++erased[id];entities.erase(id);}
 void RemoveFromLayer(int,entid_t){}void AddToLayer(int,entid_t,int){}template<class...T>void Send_Message(T...){}uint64_t GetRDeltaTime(){return 16;}
}core;
class SHIP{public:
 bool bMounted=true;uint64_t blots_id=7,modelID=100;MODEL model;bool dead=false;
 struct StateType{CVECTOR vAng=CVECTOR(0.f),vSpeed=CVECTOR(0.f),vPos=CVECTOR(0.f),vBoxSize=CVECTOR(2.f),vRotate=CVECTOR(0.f);float fWeight=200;}State;
 struct Drift{bool initialized=false;float worldX=0,worldZ=0;}phonkDrift;struct Flight{float velocity=0;}phonkFlight;
 storm::ship::phonk::LaserState phonkLaser;CVECTOR applied=CVECTOR(0.f);float appliedYaw=0,discardedY=0;
 MODEL*GetModel(){return &model;}entid_t GetModelEID(){return modelID;}bool isDead(){return dead;}double GetHullContactMass(){return State.fWeight;}
 void ApplyHullContactMotion(const CVECTOR&v,float yaw,const CVECTOR&,float,uint32_t milliseconds){assert(milliseconds==0);applied+=CVECTOR(v.x,0.f,v.z);discardedY+=std::abs(v.y);appliedYaw+=yaw;}
 void DrawPhonkLaser(){}void DrawPhonkOrdnance(){}
};
class SHIP_CUT_FRAGMENTS final : public Entity
{
 public:
    using Piece = storm::ship::phonk::LaserFragment;
    using Hit = storm::ship::debris::Hit;
    struct Orphan { entid_t model = 0; uint64_t owner = 0; double time = 0.; std::vector<Piece> pieces; };
    struct Group { entid_t model; uint64_t owner; double time; std::vector<Piece> *pieces; };
    struct Request { Hit hit; storm_ship_laser::Plane plane; };
    struct Work { Request request; storm_ship_laser::FragmentProjection projection; bool valid = false; };
    layer_index_t executeLayer{}, realizeLayer{};
    bool attached = false;
    entid_t chipAssets = 0;
    std::vector<Orphan> orphans;
    std::vector<Request> requests;
    std::optional<Request> active;
    std::future<Work> future;
    std::vector<Group> Groups();
    Piece *Find(const Hit &, Group *group = nullptr);
    void AdvanceCutFragments();
    void LimitBodies();
    void PollDamage();
    void DrawOrphans();
    void Chips(const CVECTOR &, const CVECTOR &, float radius);
  public:
    bool Init() override
    {
        chipAssets = core.CreateEntity("ship_hull_chip_assets");
        return true;
    }
    ~SHIP_CUT_FRAGMENTS() override
    {
        // The worker owns values only. Joining is confined to sea teardown;
        // an ordinary empty-scene retirement waits for its completed future.
        if (future.valid()) future.wait();
        for (const auto &orphan : orphans) core.EraseEntity(orphan.model);
        if (chipAssets) core.EraseEntity(chipAssets);
    }
    bool Adopt(SHIP &);
    Hit Trace(const CVECTOR &, const CVECTOR &);
    bool Queue(const Hit &, const storm_ship_laser::Plane &);
    bool Impact(const Hit &, const CVECTOR &, float massKg, int32_t attacker);
    bool Position(const Hit &, CVECTOR &);
    bool Beam(const Hit &, const CVECTOR &worldDirection, float depth);
    bool Crush(const Hit &, const CVECTOR &worldPoint, const CVECTOR &worldNormal, float energy);
    void Blast(const CVECTOR &, float radius);
    void Attach(layer_index_t nextExecute, layer_index_t nextRealize)
    {
        if (attached && executeLayer == nextExecute && realizeLayer == nextRealize) return;
        if (attached)
        {
            core.RemoveFromLayer(executeLayer, GetId());
            core.RemoveFromLayer(realizeLayer, GetId());
        }
        executeLayer = nextExecute; realizeLayer = nextRealize; attached = true;
        core.AddToLayer(executeLayer, GetId(), std::numeric_limits<priority_t>::max());
        core.AddToLayer(realizeLayer, GetId(), 65532);
    }
    void ProcessStage(Stage stage, uint32_t delta) override
    {
        if ((stage != Stage::execute && stage != Stage::realize) || !delta) return;
        const auto ships = core.GetEntityIds("ship");
        if (ships.empty() && orphans.empty() && !future.valid()) { core.EraseEntity(GetId()); return; }
        // One late-frame owner advances the entire fleet together. A per-ship
        // pass cannot resolve reciprocal debris contact from coherent poses.
        if(stage==Stage::execute){
            for (auto &orphan : orphans) orphan.time += delta * .001;
            AdvanceCutFragments();
            PollDamage();
            LimitBodies();
            return;
        }
        const auto visible = core.GetEntityIds(realizeLayer);
        for (const auto id : ships)
            if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted)
            {
                if (std::find(visible.begin(), visible.end(), id) != visible.end())
                {
                    ship->DrawPhonkLaser();
                    ship->DrawPhonkOrdnance();
                }
            }
        DrawOrphans();
    }
};
void SHIP_CUT_FRAGMENTS::DrawOrphans(){} // Native rendering excluded.
GEOS*fragmentGeometry(MODEL*m,const std::string&){return m?&m->geo:nullptr;}
bool completeModelBounds(MODEL*m,storm_ship_hull::Bounds&b){b=m->box;return true;}
bool finite(const CVECTOR&v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct FragmentSweep{fc::Proxy shape;sl::Vec3 translation,angular,center;float padding=0;fc::Hit hit;};
thread_local FragmentSweep*activeFragmentSweep=nullptr;
float fragmentScale(const CMatrix &world)
{
    const float squared = ~world.Vx();
    if (!std::isfinite(squared) || squared < 1.e-12f ||
        std::abs((~world.Vy()) - squared) > .001f * squared ||
        std::abs((~world.Vz()) - squared) > .001f * squared) return 0.f;
    return std::sqrt(squared);
}
CMatrix fragmentPose(const LaserFragment &piece, float seconds)
{
    CMatrix rotation, basis = piece.world;
    const CVECTOR center = basis * piece.center;
    basis.Pos() = CVECTOR(0.f);
    rotation.BuildMatrix(piece.spin * seconds);
    CMatrix pose = basis * rotation;
    pose.Pos() = center + piece.velocity * seconds - pose * piece.center;
    return pose;
}
bool fragmentPolygon(const CVECTOR *vertices, int32_t count)
{
    auto &query = *activeFragmentSweep;
    for (int32_t n = 1; n + 1 < count; ++n)
    {
        const std::array<storm_ship_hull::Vec3, 3> triangle{{
            {vertices[0].x, vertices[0].y, vertices[0].z},
            {vertices[n].x, vertices[n].y, vertices[n].z},
            {vertices[n + 1].x, vertices[n + 1].y, vertices[n + 1].z}}};
        const auto hit = storm::ship::fragment_contact::sweep(query.shape, query.translation, triangle,
                                                              query.padding, query.angular, query.center);
        if (hit.hit && hit.fraction < query.hit.fraction) query.hit = hit;
    }
    return !query.hit.hit || query.hit.fraction > 0.f;
}
std::vector<SHIP_CUT_FRAGMENTS::Group> SHIP_CUT_FRAGMENTS::Groups()
{
    std::vector<Group> groups;
    for (const auto id : core.GetEntityIds("ship"))
        if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted && ship->GetModel())
            groups.push_back({ship->GetModelEID(), uint64_t(ship->blots_id), ship->phonkLaser.time, &ship->phonkLaser.fragments});
    for (auto &orphan : orphans)
        if (core.GetEntityPointer(orphan.model)) groups.push_back({orphan.model, orphan.owner, orphan.time, &orphan.pieces});
    return groups;
}
SHIP_CUT_FRAGMENTS::Piece *SHIP_CUT_FRAGMENTS::Find(const Hit &hit, Group *found)
{
    for (const auto &group : Groups())
        if (uint64_t(group.model) == hit.model && group.owner == hit.owner)
        {
            auto *geometry = fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)), hit.node);
            if (!geometry || geometry->GeometryGeneration() != hit.generation) return nullptr;
            for (auto &piece : *group.pieces)
                if (piece.token == hit.token && piece.generation == hit.generation && piece.node == hit.node)
                { if (found) *found = group; return &piece; }
        }
    return nullptr;
}
bool SHIP_CUT_FRAGMENTS::Adopt(SHIP &ship)
{
    if (ship.phonkLaser.fragments.empty() || !ship.GetModel()) return false;
    // Bound retained MODEL/material ownership, including beached fragments.
    if (orphans.size() >= 32)
    { core.EraseEntity(orphans.front().model); orphans.erase(orphans.begin()); }
    try { orphans.push_back({ship.GetModelEID(), uint64_t(ship.blots_id), ship.phonkLaser.time, {}}); }
    catch (const std::exception &) { return false; }
    orphans.back().pieces = std::move(ship.phonkLaser.fragments);
    return true;
}
SHIP_CUT_FRAGMENTS::Hit SHIP_CUT_FRAGMENTS::Trace(const CVECTOR &from, const CVECTOR &to)
{
    Hit nearest;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!finite(from) || !finite(to)) return nearest;
    for (const auto &group : Groups())
        for (auto &piece : *group.pieces)
        {
            auto *geometry = fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)), piece.node);
            if (!geometry || geometry->GeometryGeneration() != piece.generation) continue;
            const float scale = fragmentScale(piece.world);
            if (!scale) continue;
            CVECTOR a, b; piece.world.MulToInv(from, a); piece.world.MulToInv(to, b);
            a *= 1.f / (scale * scale); b *= 1.f / (scale * scale);
            CVECTOR lo = piece.support[0], hi = lo;
            for (const auto &p : piece.support) for (unsigned n = 0; n < 3; ++n)
            { lo.v[n] = std::min(lo.v[n], p.v[n]); hi.v[n] = std::max(hi.v[n], p.v[n]); }
            if (!storm_ship_hull::rayBounds({{lo.x,lo.y,lo.z},{hi.x,hi.y,hi.z}}, {a.x,a.y,a.z}, {b.x,b.y,b.z})) continue;
            float fraction = 2.f; storm_ship_hull::Vec3 normal;
            if (!geometry->TraceLaserFragment(group.owner, piece.generation, piece.token,
                {a.x,a.y,a.z}, {b.x,b.y,b.z}, fraction, normal) || fraction >= nearest.fraction) continue;
            nearest = {uint64_t(group.model), group.owner, piece.token, piece.generation, piece.node, fraction,
                a + (b-a)*fraction, CVECTOR(normal.x,normal.y,normal.z)};
        }
#endif
    return nearest;
}
bool SHIP_CUT_FRAGMENTS::Queue(const Hit &hit, const storm_ship_laser::Plane &plane)
{
    if (!Find(hit) || plane.beams.empty() || !storm_ship_laser::valid(plane)) return false;
    for (auto &request : requests)
        if (request.hit.model == hit.model && request.hit.owner == hit.owner && request.hit.token == hit.token &&
            request.hit.generation == hit.generation && request.hit.node == hit.node && request.plane.charred == plane.charred)
        {
            for (const auto &sample : plane.beams)
            {
                bool merged = false;
                for (auto &prior : request.plane.beams)
                    if (prior.start == sample.start && prior.direction == sample.direction && prior.radius == sample.radius)
                    {
                        const float depth = prior.depth + sample.depth;
                        if (!std::isfinite(depth)) return false;
                        prior.depth = depth; merged = true; break;
                    }
                if (!merged && request.plane.beams.size() < storm_hull_damage::MaxBeamSamples)
                    request.plane.beams.push_back(sample);
            }
            return true;
        }
    if (requests.size() >= 64) return false;
    requests.push_back({hit, plane});
    return true;
}
bool SHIP_CUT_FRAGMENTS::Crush(const Hit &identity, const CVECTOR &point, const CVECTOR &normal, float energy)
{
    if (!finite(point) || !finite(normal) || !std::isfinite(energy) || energy < 80.f) return false;
    auto *piece = Find(identity);
    if (!piece) return false;
    const float scale = fragmentScale(piece->world);
    if (!scale) return false;
    Hit hit = identity;
    piece->world.MulToInv(point, hit.point); piece->world.MulToInvNorm(normal, hit.normal);
    hit.point *= 1.f / (scale * scale);
    if ((~hit.normal) < .1f) return false;
    hit.normal = !hit.normal;
    storm_ship_laser::Plane plane; plane.charred = false;
    storm_hull_damage::BeamSample beam;
    const auto direction = -hit.normal;
    beam.start = {hit.point.x,hit.point.y,hit.point.z}; beam.direction = {direction.x,direction.y,direction.z};
    const auto tangent = storm_ship_laser::beamTangent(beam); beam.tangent = {tangent.x,tangent.y,tangent.z};
    beam.radius = std::clamp(.06f + std::sqrt(energy)*.003f,.07f,1.2f);
    beam.depth = std::clamp((std::sqrt(energy)-std::sqrt(80.f))*.002f,.005f,.8f);
    if (!storm_hull_damage::validPhysicalBeamDimensions(beam.radius, beam.depth)) return false;
    beam.radius /= scale; beam.depth /= scale;
    plane.beams.push_back(beam);
    return Queue(hit, plane);
}
bool SHIP_CUT_FRAGMENTS::Impact(const Hit &hit, const CVECTOR &velocity, float massKg, int32_t)
{
    auto *piece = Find(hit);
    if (!piece || !finite(velocity) || !std::isfinite(massKg) || massKg <= 0.f) return false;
    namespace fc = storm::ship::fragment_contact;
    const auto vector=[](CVECTOR v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    const float mass = std::max(.5f,piece->mass > 0.f ? piece->mass : piece->surfaceArea*24.f);
    const auto point = piece->world*hit.point, lever = point-piece->world*piece->center;
    const auto outward = !(piece->world*hit.normal-piece->world.Pos());
    const auto normal = -outward; // from the incoming projectile toward this body
    const auto relative = piece->velocity+(piece->spin^lever)-velocity;
    const auto impulse = fc::pairImpulse(vector(relative),vector(lever),{},vector(normal),
        1.f/mass,1.f/massKg,.35f/(mass*piece->radiusSquared),0.f);
    const CVECTOR push(impulse.x,impulse.y,impulse.z);
    piece->velocity += push/mass; piece->spin += (lever^push)*(.35f/(mass*piece->radiusSquared));
    const float inward = std::max(0.f,-(relative|normal));
    return Crush(hit, point, outward, .5f*(mass*massKg/(mass+massKg))*inward*inward);
}
bool SHIP_CUT_FRAGMENTS::Position(const Hit &hit,CVECTOR &point)
{
    auto *piece=Find(hit);if(!piece)return false;point=piece->world*hit.point;return true;
}
bool SHIP_CUT_FRAGMENTS::Beam(const Hit &hit,const CVECTOR &worldDirection,float depth)
{
    storm_hull_damage::BeamSample beam;
    if (!storm_hull_damage::validPhysicalBeamDimensions(beam.radius, depth)) return false;
    auto *piece=Find(hit);if(!piece||!finite(worldDirection))return false;
    const float scale=fragmentScale(piece->world);if(!scale)return false;
    CVECTOR direction;piece->world.MulToInvNorm(worldDirection,direction);
    if((~direction)<.1f)return false;direction=!direction;
    beam.start={hit.point.x,hit.point.y,hit.point.z};beam.direction={direction.x,direction.y,direction.z};
    const auto tangent=storm_ship_laser::beamTangent(beam);beam.tangent={tangent.x,tangent.y,tangent.z};
    beam.radius/=scale;beam.depth=depth/scale;
    storm_ship_laser::Plane plane;plane.beams.push_back(beam);return Queue(hit,plane);
}
void SHIP_CUT_FRAGMENTS::LimitBodies()
{
    std::erase_if(orphans,[&](auto &orphan){
        if(!orphan.pieces.empty()&&core.GetEntityPointer(orphan.model))return false;
        core.EraseEntity(orphan.model);return true;
    });
    auto groups=Groups();size_t count=0;for(const auto &group:groups)count+=group.pieces->size();
    if(count<=256)return;
    struct Candidate {float age;Hit hit;};
    std::vector<Candidate> oldest;oldest.reserve(count);
    for(const auto &group:groups)for(const auto &piece:*group.pieces)
        oldest.push_back({piece.age,{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node}});
    std::stable_sort(oldest.begin(),oldest.end(),[](const auto&a,const auto&b){return a.age>b.age;});
    oldest.resize(count-256);
    const auto key=[](const Hit &hit){return std::tie(hit.model,hit.owner,hit.node,hit.token);};
    std::sort(oldest.begin(),oldest.end(),[&](const auto&a,const auto&b){return key(a.hit)<key(b.hit);});
    for(const auto &group:groups)std::erase_if(*group.pieces,[&](auto &piece){
        const auto target=std::tie(group.model,group.owner,piece.node,piece.token);
        const auto found=std::lower_bound(oldest.begin(),oldest.end(),target,[&](const auto &candidate,const auto &value){return key(candidate.hit)<value;});
        if(found==oldest.end()||key(found->hit)!=target)return false;
        if(auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node))
            geometry->ReleaseLaserFragment(group.owner,piece.token);
        return true;
    });
}
void SHIP_CUT_FRAGMENTS::Chips(const CVECTOR &point, const CVECTOR &normal, float radius)
{
    const auto chips = core.CreateEntity("ship_hull_chips");
    if (!chips) return;
    core.Send_Message(chips,"lfffffffl",storm_hull_damage::ChipBurstMessage,point.x,point.y,point.z,
        normal.x,normal.y,normal.z,std::clamp(radius,.12f,.7f),int32_t(core.GetRDeltaTime()+uint64_t(chips)));
    core.AddToLayer(executeLayer,chips,0); core.AddToLayer(realizeLayer,chips,36);
}
void SHIP_CUT_FRAGMENTS::PollDamage()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (future.valid())
    {
        if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        Work work;
        try { work = future.get(); } catch (const std::exception &) { active.reset(); }
        Group group{};
        auto *piece = work.valid ? Find(work.request.hit, &group) : nullptr;
        if (piece && work.projection.changed)
        {
            try
            {
                Piece original = *piece;
                const size_t index = size_t(piece-group.pieces->data());
                auto &successors = work.projection.successors;
                // Sub-hand-sized material becomes the same bounded floating
                // chip effect as ordinary hull damage; large boards stay bodies.
                std::erase_if(successors, [](const auto &next) {
                    const auto size=next.properties.bounds.hi-next.properties.bounds.lo;
                    return next.properties.surfaceArea<.025f || std::max({size.x,size.y,size.z})<.12f;
                });
                std::vector<Piece> newborn; newborn.reserve(successors.size());
                float area=0.f;for(const auto &next:successors)area+=next.properties.surfaceArea;
                const float mass=std::max(.5f,original.mass>0.f?original.mass:original.surfaceArea*24.f);
                for(const auto &next:successors)
                {
                    Piece body=original;const auto &props=next.properties;
                    body.token=0;body.center=CVECTOR(props.centroid.x,props.centroid.y,props.centroid.z);
                    body.surfaceArea=props.surfaceArea;body.mass=mass*props.surfaceArea/std::max(area,1.e-8f);
                    body.velocity=original.velocity+(original.spin^(original.world*(body.center-original.center)-original.world.Pos()));
                    body.radiusSquared=.01f;
                    for(size_t n=0;n<body.support.size();++n){const auto p=props.support[n];body.support[n]=CVECTOR(p.x,p.y,p.z);
                        body.radiusSquared=std::max(body.radiusSquared,~(body.support[n]-body.center));}
                    body.contactShape=storm::ship::fragment_contact::makeProxy(props.support);
                    if(!body.contactShape.valid())throw std::runtime_error("invalid secondary contact shape");
                    const auto size=props.bounds.hi-props.bounds.lo;
                    body.timber=storm::ship::fragment_contact::timberBuoyancy(size);
                    body.time=group.time;newborn.push_back(std::move(body));
                }
                // Prepare every consumer allocation before geometry retires the
                // old token. A queued laser dose follows the same local material.
                std::vector<Request> continued;continued.reserve(64);
                std::vector<std::pair<size_t,size_t>> retarget;retarget.reserve(64);
                for(const auto &queued:requests)
                {
                    const bool same=queued.hit.model==work.request.hit.model&&queued.hit.owner==work.request.hit.owner&&
                        queued.hit.node==work.request.hit.node&&queued.hit.token==work.request.hit.token&&queued.hit.generation==work.request.hit.generation;
                    if(!same){if(continued.size()<64)continued.push_back(queued);continue;}
                    auto next=queued;
                    if(next.plane.charred)storm_ship_laser::continueBeamDose(next.plane.beams,work.request.plane.beams);
                    for(size_t n=0;n<successors.size()&&continued.size()<64;++n)
                    {
                        const auto &bounds=successors[n].properties.bounds;bool intersects=false;
                        for(const auto &beam:next.plane.beams)intersects|=storm_ship_laser::beamIntersectsBox(beam,(bounds.lo+bounds.hi)*.5f,(bounds.hi-bounds.lo)*.5f);
                        if(intersects){retarget.emplace_back(continued.size(),n);continued.push_back(next);}
                    }
                }
                group.pieces->reserve(group.pieces->size()-1+newborn.size());
                auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),original.node);
                std::vector<storm_ship_laser::Fragment> committed;
                if(geometry&&geometry->ReplaceLaserFragment(group.owner,std::move(work.projection),committed))
                {
                    // Ordered metadata is the GEOM transaction contract.
                    for(size_t n=0;n<newborn.size();++n)newborn[n].token=committed[n].fragment;
                    for(const auto &[request,n]:retarget)continued[request].hit.token=committed[n].fragment;
                    group.pieces->erase(group.pieces->begin()+index);
                    for(auto &body:newborn)group.pieces->push_back(std::move(body));
                    requests.swap(continued);
                    const auto point=original.world*work.request.hit.point;
                    const auto normal=original.world*work.request.hit.normal-original.world.Pos();
                    Chips(point,normal,.25f);
                }
            }
            catch(const std::exception &) { /* Keep the old body on failed preparation/upload. */ }
        }
        active.reset();
    }
    while(!requests.empty())
    {
        Request request=std::move(requests.front());requests.erase(requests.begin());
        Group group{};if(!Find(request.hit,&group))continue;
        auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),request.hit.node);
        storm_ship_laser::FragmentInput input;
        if(!geometry||!geometry->CaptureFragmentInput(group.owner,request.hit.generation,request.hit.token,request.plane,8,input))continue;
        try
        {
            active=request;
            future=std::async(std::launch::async,[request=std::move(request),input=std::move(input)]()mutable{
                Work work;work.request=std::move(request);
                try{work.valid=storm_ship_laser::prepareFragment(input,work.projection);}catch(const std::exception &){}
                return work;
            });
        }
        catch(const std::exception &){active.reset();}
        break; // One immutable CPU job for the entire fleet.
    }
#endif
}
void SHIP_CUT_FRAGMENTS::Blast(const CVECTOR &center,float radius)
{
    if(!finite(center)||!std::isfinite(radius)||radius<=0.f)return;
    for(const auto &group:Groups())for(auto &piece:*group.pieces)
    {
        const auto middle=piece.world*piece.center;auto away=middle-center;
        const float distance=std::sqrt(~away);if(distance>=radius)continue;
        if(distance<.01f)away=CVECTOR(0.f,1.f,0.f);else away*=1.f/distance;
        const float strength=1.f-distance/radius;
        piece.velocity+=away*(35.f*strength);
        auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation||!fragmentScale(piece.world))continue;
        CVECTOR axis;piece.world.MulToInvNorm(away,axis);axis=!axis;
        float reach=.01f;
        for(const auto &point:piece.support)reach=std::max(reach,std::sqrt(~(point-piece.center))+.01f);
        const auto from=piece.center-axis*reach,to=piece.center+axis*reach;
        float fraction=2.f;storm_ship_hull::Vec3 normal;
        if(!geometry->TraceLaserFragment(group.owner,piece.generation,piece.token,
            {from.x,from.y,from.z},{to.x,to.y,to.z},fraction,normal))continue;
        Hit hit{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node,fraction,
            from+(to-from)*fraction,CVECTOR(normal.x,normal.y,normal.z)};
        const auto outward=!(piece.world*hit.normal-piece.world.Pos());
        Crush(hit,piece.world*hit.point,outward,2.e5f*strength*strength);
    }
}
void SHIP_CUT_FRAGMENTS::AdvanceCutFragments()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    LimitBodies(); // Admission precedes even the newborn/zero-elapsed return.
    namespace fc=storm::ship::fragment_contact;
    using storm::ship::phonk::LaserFragment;
    const auto vector=[](const CVECTOR&v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    const auto native=[](storm_ship_hull::Vec3 v){return CVECTOR(v.x,v.y,v.z);};
    struct Body { Hit identity; LaserFragment*piece;float start; };
    struct Receiver { SHIP*ship;MODEL*model;storm_ship_hull::Bounds bounds;CVECTOR velocity,delta=CVECTOR(0.f);float yaw=0,deltaYaw=0,mass=0,inertia=0; };
    std::vector<Body>bodies;std::vector<Receiver>receivers;
    auto *pSea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    auto groups = Groups();
    float elapsed=0.f;
    for (const auto &group : groups) for (const auto &piece : *group.pieces)
        elapsed = std::max(elapsed,float(std::max(0.,group.time-piece.time)));
    for(const auto id:core.GetEntityIds("ship"))if(auto*ship=static_cast<SHIP*>(core.GetEntityPointer(id));ship&&ship->bMounted){
        auto*model=ship->GetModel();storm_ship_hull::Bounds bounds;
        if(!model||!completeModelBounds(model,bounds))continue;
        const float c=std::cos(ship->State.vAng.y),s=std::sin(ship->State.vAng.y);
        CVECTOR velocity(KNOTS2METERS(ship->State.vSpeed.x)*c+KNOTS2METERS(ship->State.vSpeed.z)*s,
            ship->State.vSpeed.y,KNOTS2METERS(ship->State.vSpeed.z)*c-KNOTS2METERS(ship->State.vSpeed.x)*s);
        if(ship->phonkDrift.initialized)velocity=CVECTOR(KNOTS2METERS(ship->phonkDrift.worldX),ship->phonkFlight.velocity,KNOTS2METERS(ship->phonkDrift.worldZ));
        if(ship->isDead())velocity=CVECTOR(0.f);
        const float mass=std::max(100.f,float(ship->GetHullContactMass()*50.)); // approximate kg/cwt; same cargo owner as TOUCH
        const float radius=std::max(1.f,ship->State.vBoxSize.x*ship->State.vBoxSize.x+ship->State.vBoxSize.z*ship->State.vBoxSize.z);
        receivers.push_back({ship,model,bounds,velocity,CVECTOR(0.f),ship->isDead()?0.f:ship->State.vRotate.y,0.f,
            ship->isDead()?0.f:mass,ship->isDead()?0.f:mass*radius/12.f});
    }
    // The island owner already enrolls shore, seabed and static world MODELs
    // in ISLAND_TRACE. Its own ISLAND::Clip is deliberately an empty stub.
    for(const auto id:core.GetEntityIds(ISLAND_TRACE))if(auto*model=static_cast<MODEL*>(core.GetEntityPointer(id))){
        storm_ship_hull::Bounds bounds;if(completeModelBounds(model,bounds))receivers.push_back({nullptr,model,bounds,CVECTOR(0.f)});
    }
    if(!(elapsed>0.f))return;
    std::vector<float>boundaries{0.f,elapsed};
    for(const auto &group:groups)for(auto&piece:*group.pieces){
        auto*geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)continue;
        const float seconds=float(std::max(0.,group.time-piece.time));
        piece.time=group.time;
        if(!(seconds>0.f))continue;
        if(!(piece.mass>0.f)){
            CVECTOR lo=piece.support[0],hi=lo;
            for(const auto&p:piece.support)for(unsigned n=0;n<3;++n){lo.v[n]=std::min(lo.v[n],p.v[n]);hi.v[n]=std::max(hi.v[n],p.v[n]);}
            const auto size=hi-lo;piece.timber=fc::timberBuoyancy(vector(size));
            // Measured GM surface area, not inferred closed volume. The
            // fallback serves only older transient fixtures without that field.
            const float area=piece.surfaceArea>0.f?piece.surfaceArea:2.f*(size.x*size.y+size.y*size.z+size.z*size.x);
            piece.mass=std::max(.5f,area*24.f); // effective 4cm timber skin at600kg/m3
        }
        const float start=std::max(0.f,elapsed-seconds);boundaries.push_back(start);
        bodies.push_back({{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node},&piece,start});
    }
    std::sort(boundaries.begin(),boundaries.end());boundaries.erase(std::unique(boundaries.begin(),boundaries.end()),boundaries.end());
    std::vector<size_t>active,order;active.reserve(bodies.size());order.reserve(bodies.size());
    std::vector<fc::Proxy>shapes(bodies.size());std::vector<storm_ship_hull::Bounds>swept(bodies.size());
    std::vector<float>padding(bodies.size());std::vector<bool>blocked(bodies.size());
    struct Contact {size_t a=0,b=SIZE_MAX,receiver=SIZE_MAX;float fraction=1.f;CVECTOR normal=CVECTOR(0.f),point=CVECTOR(0.f),velocity=CVECTOR(0.f);};
    for(size_t interval=1;interval<boundaries.size();++interval){
        const float duration=boundaries[interval]-boundaries[interval-1];
        // Consume the full interval, with20ms steps only up to2.56s.
        // A larger hitch is bounded to128 coarser steps, not a20ms guarantee.
        const unsigned steps=unsigned(std::clamp(std::ceil(duration/.02f),1.f,128.f));const float dt=duration/steps;
        active.clear();for(size_t n=0;n<bodies.size();++n)if(bodies[n].start<=boundaries[interval-1])active.push_back(n);
        for(unsigned step=0;step<steps;++step){
            for(auto n:active){auto&piece=*bodies[n].piece;const auto center=piece.world*piece.center;float low=center.y,high=center.y;
                for(const auto&sample:piece.support){const float y=(piece.world*sample).y;low=std::min(low,y);high=std::max(high,y);}
                const float water=pSea?pSea->WaveXZ(center.x,center.z):low-1.f,depth=std::max(.05f,high-low);
                const float submerged=std::clamp((water-low)/depth,0.f,1.f);piece.wetAge+=submerged*dt;piece.age+=dt;
                const float density=fc::waterDensity(piece.wetAge,piece.timber,piece.age);
                piece.velocity.y+=9.81f*(submerged/density-1.f)*dt;
                piece.velocity*=std::exp(-(.015f+submerged*1.6f)*dt);piece.spin*=std::exp(-(.04f+submerged*1.1f)*dt);
                if(submerged>0.f){CVECTOR torque(0.f);for(const auto&sample:piece.support){const auto point=piece.world*sample;
                    const float localWater=pSea?pSea->WaveXZ(point.x,point.z):water;
                    torque+=(point-center)^CVECTOR(0.f,9.81f*std::clamp((localWater-point.y)/depth,0.f,1.f)/density,0.f);}
                    piece.spin+=torque*(dt/(piece.support.size()*piece.radiusSquared));}
            }
            float remaining=dt,progress=0.f;
            // Bounded continuous event solve. The final search freezes only
            // bodies with an unresolved contact; free bodies consume all time.
            // No admitted body is advanced through a contact on budget expiry.
            for(unsigned iteration=0;iteration<=8&&remaining>1.e-7f;++iteration){
                Contact nearest;std::fill(blocked.begin(),blocked.end(),false);
                for(auto n:active){auto&piece=*bodies[n].piece;
                    shapes[n]=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(piece.world*native(p));});
                    padding[n]=std::sqrt(piece.radiusSquared*(~piece.spin))*remaining+.005f;
                    if(!shapes[n].valid()){blocked[n]=true;continue;}
                    swept[n]=fc::bounds(shapes[n],vector(piece.velocity*remaining),padding[n]);
                    const auto center=piece.world*piece.center;
                    for(size_t r=0;r<receivers.size();++r){const auto&receiver=receivers[r];
                        const float untilEnd=elapsed-(boundaries[interval-1]+step*dt+progress);
                        const auto origin=receiver.model->mtx.Pos();
                        const auto snapshot=[&](const CVECTOR&point,float until){CMatrix rotation;rotation.BuildRotateY(receiver.yaw*until);
                            return origin+rotation*(point-origin+receiver.velocity*until);};
                        FragmentSweep query;query.shape=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(snapshot(piece.world*native(p),untilEnd));});
                        const auto mappedCenter=snapshot(center,untilEnd);
                        const auto travel=snapshot(center+piece.velocity*remaining,untilEnd-remaining)-mappedCenter;
                        CMatrix rotation;rotation.BuildRotateY(receiver.yaw*untilEnd);
                        const auto angular=(rotation*piece.spin-CVECTOR(0.f,receiver.yaw,0.f))*remaining;
                        query.translation=vector(travel);query.angular=vector(angular);query.center=vector(mappedCenter);
                        query.padding=std::sqrt(piece.radiusSquared*(~angular))+.005f;
                        const auto box=fc::bounds(query.shape,query.translation,query.padding);
                        if(!storm_ship_hull::overlaps(box,receiver.bounds))continue;
                        const PLANE planes[6]={{1,0,0,box.hi.x},{-1,0,0,-box.lo.x},{0,1,0,box.hi.y},{0,-1,0,-box.lo.y},{0,0,1,box.hi.z},{0,0,-1,-box.lo.z}};
                        const auto middle=(box.lo+box.hi)*.5f;auto*previous=activeFragmentSweep;activeFragmentSweep=&query;
                        receiver.model->Clip(planes,6,native(middle),storm_ship_hull::length(box.hi-middle),fragmentPolygon);activeFragmentSweep=previous;
                        if(!query.hit.hit)continue;blocked[n]=true;
                        if(query.hit.fraction>=nearest.fraction)continue;
                        const float until=untilEnd-remaining*query.hit.fraction;CMatrix undo;undo.BuildRotateY(-receiver.yaw*until);
                        nearest={n,SIZE_MAX,r,query.hit.fraction,undo*native(query.hit.normal),
                            origin-receiver.velocity*until+undo*(native(query.hit.point)-origin),CVECTOR(0.f)};
                        nearest.velocity=receiver.velocity+receiver.delta+
                            (CVECTOR(0.f,receiver.yaw+receiver.deltaYaw,0.f)^(nearest.point-origin+receiver.velocity*until));
                    }
                }
                // Sweep-and-prune rejects distant pieces before SAT. Eight
                // event iterations bound solver work independently of contacts.
                order=active;std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return swept[a].lo.x<swept[b].lo.x;});
                for(size_t i=0;i<order.size();++i)for(size_t j=i+1;j<order.size();++j){const auto a=order[i],b=order[j];
                    if(swept[b].lo.x>swept[a].hi.x)break;if(!storm_ship_hull::overlaps(swept[a],swept[b]))continue;
                    auto&pa=*bodies[a].piece;auto&pb=*bodies[b].piece;
                    const auto hit=fc::sweepPair(shapes[a],vector(pa.velocity*remaining),shapes[b],vector(pb.velocity*remaining),
                        padding[a]+padding[b],vector(pa.spin*remaining),vector(pa.world*pa.center),vector(pb.spin*remaining),vector(pb.world*pb.center));
                    if(!hit.hit)continue;blocked[a]=blocked[b]=true;
                    if(hit.fraction<nearest.fraction)nearest={a,b,SIZE_MAX,hit.fraction,native(hit.normal),native(hit.point),CVECTOR(0.f)};
                }
                if(iteration==8){for(auto n:active)if(!blocked[n])bodies[n].piece->world=fragmentPose(*bodies[n].piece,remaining);break;}
                const float advance=remaining*nearest.fraction;
                for(auto n:active)if(shapes[n].valid())bodies[n].piece->world=fragmentPose(*bodies[n].piece,advance);
                progress+=advance;remaining-=advance;if(nearest.fraction>=1.f)break;
                auto&a=*bodies[nearest.a].piece;const auto leverA=nearest.point-a.world*a.center;
                const float invMassA=1.f/a.mass,invInertiaA=.35f*invMassA/a.radiusSquared;
                CVECTOR leverB(0.f),velocityB=nearest.velocity;float invMassB=0.f,invInertiaB=0.f;
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;leverB=nearest.point-b.world*b.center;
                    velocityB=b.velocity+(b.spin^leverB);invMassB=1.f/b.mass;invInertiaB=.35f*invMassB/b.radiusSquared;}
                else if(nearest.receiver!=SIZE_MAX){const auto&r=receivers[nearest.receiver];leverB=nearest.point-r.model->mtx.Pos();
                    if(r.mass>0.f){invMassB=1.f/r.mass;invInertiaB=1.f/r.inertia;}}
                const auto impulse=native(fc::pairImpulse(vector(a.velocity+(a.spin^leverA)-velocityB),vector(leverA),vector(leverB),vector(nearest.normal),
                    invMassA,invMassB,invInertiaA,invInertiaB,nearest.receiver!=SIZE_MAX));
                // Capture material-local requests while the contact is known;
                // no fragment vector changes until the fleet solve has ended.
                const float inward=std::max(0.f,-((a.velocity+(a.spin^leverA)-velocityB)|nearest.normal));
                const float work=.5f*std::abs(impulse|nearest.normal)*inward/1.12f;
                Crush(bodies[nearest.a].identity,nearest.point,nearest.normal,work);
                if(nearest.b!=SIZE_MAX)Crush(bodies[nearest.b].identity,nearest.point,nearest.normal*-1.f,work);
                a.velocity+=impulse*invMassA;a.spin+=(leverA^impulse)*invInertiaA;
                const float receiverNormalMass=nearest.receiver!=SIZE_MAX?invMassB*(nearest.normal.x*nearest.normal.x+nearest.normal.z*nearest.normal.z):invMassB;
                const float share=invMassA/std::max(1.e-8f,invMassA+receiverNormalMass);a.world.Pos()+=nearest.normal*(.012f*share);
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;b.velocity-=impulse*invMassB;b.spin-=(leverB^impulse)*invInertiaB;
                    b.world.Pos()-=nearest.normal*(.012f*(1.f-share));}
                else if(nearest.receiver!=SIZE_MAX){auto&r=receivers[nearest.receiver];r.delta-=CVECTOR(impulse.x*invMassB,0.f,impulse.z*invMassB);
                    r.deltaYaw-=(leverB^impulse).y*invInertiaB;}
            }
        }
    }
    // Late impulses update native/Phonk velocity through the existing owner.
    // Zero milliseconds deliberately skips its pre-Execute pose prediction.
    for(auto&r:receivers)if(r.ship&&r.mass>0.f&&((~r.delta)>0.f||r.deltaYaw!=0.f))
        r.ship->ApplyHullContactMotion(r.delta,r.deltaYaw,r.ship->State.vPos,r.ship->State.vAng.y,0);
    for(const auto &group:groups)std::erase_if(*group.pieces,[&](auto&piece){
        auto*geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)return true;
        bool submerged=piece.age>.5f;for(const auto&sample:piece.support){const auto point=piece.world*sample;
            const float water=pSea?pSea->WaveXZ(point.x,point.z):-200.f;if(point.y>=water-2.f){submerged=false;break;}}
        if(!submerged&&piece.age<180.f)return false; // finite stranded-wreck lifetime
        geometry->ReleaseLaserFragment(group.owner,piece.token);return true;
    });
    std::erase_if(orphans,[&](auto &orphan){if(!orphan.pieces.empty()&&core.GetEntityPointer(orphan.model))return false;
        core.EraseEntity(orphan.model);return true;});
#endif
}
sl::Vertex vertex(sl::Vec3 p){sl::Vertex v;v.position=p;return v;}
sl::Mesh box(float radius=1,bool reverse=false){sl::Mesh m;sl::Vec3 p[8]={{-radius,-radius,-radius},{radius,-radius,-radius},{radius,radius,-radius},{-radius,radius,-radius},{-radius,-radius,radius},{radius,-radius,radius},{radius,radius,radius},{-radius,radius,radius}};int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};for(auto&q:f)for(auto ids:{std::array<int,3>{q[0],q[1],q[2]},std::array<int,3>{q[0],q[2],q[3]}}){sl::Triangle t={vertex(p[ids[0]]),vertex(p[ids[1]]),vertex(p[ids[2]])};if(reverse)std::swap(t[1],t[2]);for(auto&v:t)v.normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));m[0].push_back(t);}return m;}
sl::Plane bore(){sl::Plane p{{0,0,-1},0};sl::BeamSample b;b.start={0,0,1.01f};b.direction={0,0,-1};b.radius=.13f;b.depth=.4f;p.beams={b};return p;}
LaserFragment body(CVECTOR pos,CVECTOR velocity=CVECTOR(0.f),uint64_t token=1){LaserFragment p;auto props=sl::fragmentProperties(box());p.token=token;p.generation=1;p.surfaceArea=props.surfaceArea;p.mass=props.surfaceArea*24.f;p.radiusSquared=3;p.contactShape=fc::makeProxy(props.support);p.world.BuildMatrix(CVECTOR(0.f),pos);p.velocity=velocity;for(int n=0;n<14;++n)p.support[n]=CVECTOR(props.support[n].x,props.support[n].y,props.support[n].z);return p;}
void setup(const std::vector<SHIP*>&ships){core={};entid_t id=1;for(auto*s:ships){s->modelID=100+id;s->blots_id=s->model.geo.owner=7+id;core.ships.push_back(id);core.entities[id++]=s;core.entities[s->modelID]=&s->model;for(auto&p:s->phonkLaser.fragments)s->model.geo.material.emplace(p.token,sl::standaloneFragment(box()));}}
void setup(std::initializer_list<SHIP*>ships){setup(std::vector<SHIP*>(ships));}
std::vector<std::unique_ptr<SHIP>> population(int ships,int pieces){std::vector<std::unique_ptr<SHIP>> result;std::vector<SHIP*>refs;for(int n=0;n<ships;++n){auto a=std::make_unique<SHIP>();for(int j=0;j<pieces;++j)a->phonkLaser.fragments.push_back(body(CVECTOR((n*pieces+j)*5.f,20.f,0.f),CVECTOR(0.f),j+1));refs.push_back(a.get());result.push_back(std::move(a));}setup(refs);return result;}
size_t count(const std::vector<std::unique_ptr<SHIP>>&fleet){size_t n=0;for(auto&a:fleet)n+=a->phonkLaser.fragments.size();return n;}
auto hit(SHIP&s,uint64_t token=1){return storm::ship::debris::Hit{s.modelID,s.blots_id,token,1,"",.25f,CVECTOR(0.f,0.f,1.f),CVECTOR(0.f,0.f,1.f)};}
void deliver(SHIP_CUT_FRAGMENTS&s,const storm::ship::debris::Hit&h,sl::FragmentProjection p){SHIP_CUT_FRAGMENTS::Work w;w.request={h,bore()};w.projection=std::move(p);w.valid=true;std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();promise.set_value(std::move(w));s.PollDamage();}
int scaleProbe(){
 int failures=0;
 for(float scale:{.5f,2.f}){
  SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];
  p.world.BuildMatrix(CVECTOR(.2f,.7f,-.3f),CVECTOR(10.f,20.f,30.f));p.world.Vx()*=scale;p.world.Vy()*=scale;p.world.Vz()*=scale;
  const auto check=[&](bool pass,const char*name){std::cout<<(pass?"PASS ":"FAIL ")<<"scale="<<scale<<" "<<name<<"\n";if(!pass)++failures;};
  const auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));
  std::cout<<"scale="<<scale<<" trace fraction="<<h.fraction<<" localZ="<<h.point.z<<"\n";
  check(h&&std::abs(h.fraction-.25f)<2.e-5f&&(~(h.point-CVECTOR(0.f,0.f,1.f)))<1.e-8f,"Trace physical ray keeps known local boundary/fraction");
  auto known=hit(a);auto point=p.world*known.point;CVECTOR current;
  check(s.Position(known,current)&&(~(point-current))<1.e-10f,"Position maps exact local material point to world");
  const auto direction=(p.world*CVECTOR(0.f,0.f,-1.f)-p.world.Pos())/scale;
  bool accepted=s.Beam(known,direction,.2f);
  if(accepted){const auto&b=s.requests.back().plane.beams[0];std::cout<<"scale="<<scale<<" Beam local radius="<<b.radius<<" depth="<<b.depth<<"\n";check(std::abs(b.radius-.16f/scale)<1.e-6f&&std::abs(b.depth-.2f/scale)<1.e-6f,"Beam dimensions preserve physical metres");}
  else check(false,"Beam valid physical input accepted");
  s.requests.clear();const auto outward=-direction;
  accepted=s.Crush(known,point,outward,10000.f);
  const float radius=.36f,depth=(100.f-std::sqrt(80.f))*.002f;
  if(accepted){const auto&b=s.requests.back().plane.beams[0];std::cout<<"scale="<<scale<<" Crush local startZ="<<b.start[2]<<" radius="<<b.radius<<" depth="<<b.depth<<"\n";check(std::abs(b.start[2]-1.f)<2.e-5f&&std::abs(b.radius-radius/scale)<1.e-5f&&std::abs(b.depth-depth/scale)<1.e-5f,"Crush uses exact local material hit and physical dimensions");}
  else check(false,"Crush admitted physical input accepted");
  for(float physicalDepth:{.001f,1000.f}){
   s.requests.clear();accepted=s.Beam(known,direction,physicalDepth);
   check(accepted&&s.requests.size()==1&&std::abs(s.requests[0].plane.beams[0].depth-physicalDepth/scale)<std::max(1.e-6f,physicalDepth*1.e-6f),"Beam inclusive physical min/max survives local scale");
  }
  s.requests.clear();check(!s.Beam(known,direction,1000.5f)&&s.requests.empty(),"Beam physical depth above maximum rejected before conversion");
  for(auto dimensions:{std::pair<float,float>{.01f,.001f},{2.f,1000.f}}){
   const bool physical=storm_hull_damage::validPhysicalBeamDimensions(dimensions.first,dimensions.second);
   auto plane=bore();plane.beams[0].radius=dimensions.first/scale;plane.beams[0].depth=dimensions.second/scale;s.requests.clear();
   check(physical&&s.Queue(known,plane),"Shared physical inclusive limits plus finite-positive local Queue");
  }
  check(!storm_hull_damage::validPhysicalBeamDimensions(2.01f,.2f)&&!storm_hull_damage::validPhysicalBeamDimensions(.16f,1000.5f),"Shared physical maxima cannot be laundered through scale");
 }
 {
  SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;
  auto mesh=box();for(auto&o:mesh)for(auto&t:o.second)for(auto&v:t){v.position.x*=5.f;v.position.y*=.5f;v.position.z*=.5f;}
  auto physical=sl::standaloneFragment(std::move(mesh));a.model.geo.material[1]=physical;auto&p=a.phonkLaser.fragments[0];const auto&props=physical.properties;
  p.surfaceArea=props.surfaceArea;p.mass=props.surfaceArea*24.f;p.radiusSquared=25.5f;p.contactShape=fc::makeProxy(props.support);
  for(int n=0;n<14;++n)p.support[n]=CVECTOR(props.support[n].x,props.support[n].y,props.support[n].z);
  s.Blast(CVECTOR(0.f,25.f,0.f),10.f);bool changed=false,prepared=false;
  if(s.requests.size()==1){sl::FragmentInput input;auto&request=s.requests[0];if(a.model.geo.CaptureFragmentInput(a.blots_id,1,1,request.plane,8,input)){sl::FragmentProjection projection;prepared=sl::prepareFragment(input,projection);changed=prepared&&projection.changed;}const auto&beam=request.plane.beams[0];std::cout<<"Blast10x1x1 startLocalY="<<beam.start[1]<<" boundaryY=.5 depth="<<beam.depth<<" prepared="<<prepared<<" changed="<<changed<<"\n";}
  const bool pass=s.requests.size()==1&&prepared&&changed;std::cout<<(pass?"PASS ":"FAIL ")<<"Blast above long thin retained box reaches real surface with finite cutter\n";if(!pass)++failures;
 }
 return failures;
}
int main(){
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];p.world.BuildMatrix(CVECTOR(0.f,1.57079632679f,0.f),CVECTOR(10.f,20.f,30.f));a.model.mtx.BuildMatrix(CVECTOR(0.f),CVECTOR(-80.f,0.f,0.f));auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));assert(h&&std::abs(h.fraction-.25f)<1.e-6f&&(~(h.point-CVECTOR(0.f,0.f,1.f)))<1.e-8f);assert(!s.Trace(CVECTOR(-80.f,0.f,2.f),CVECTOR(-80.f,0.f,-2.f)));auto stale=h;stale.owner++;assert(!s.Queue(stale,bore()));stale=h;stale.generation++;assert(!s.Queue(stale,bore()));stale=h;stale.token++;assert(!s.Queue(stale,bore()));assert(s.Queue(h,bore()));auto size=a.phonkLaser.fragments.size(),capacity=a.phonkLaser.fragments.capacity();assert(s.Crush(h,p.world*h.point,p.world*h.normal-p.world.Pos(),10000));assert(a.phonkLaser.fragments.size()==size&&a.phonkLaser.fragments.capacity()==capacity&&s.requests.size()==2);std::cout<<"PASS translated/rotated detached trace, parent exclusion, identity negatives, queue preserves vector\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f),CVECTOR(3.f,0.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto model=a.modelID;assert(s.Adopt(a)&&a.phonkLaser.fragments.empty()&&s.orphans[0].model==model);core.ships.clear();core.entities.erase(1);s.ProcessStage(Entity::Stage::execute,0);assert(s.orphans[0].time==0&&s.orphans[0].pieces[0].age==0);s.ProcessStage(Entity::Stage::execute,20);auto&p=s.orphans[0].pieces[0];assert(p.age>.019f&&p.world.Pos().x>.05f&&core.erased[model]==0);assert(s.Trace(CVECTOR(p.world.Pos().x,p.world.Pos().y,2.f),CVECTOR(p.world.Pos().x,p.world.Pos().y,-2.f)));auto age=p.age;s.AdvanceCutFragments();assert(p.age==age);p.age=180;s.ProcessStage(Entity::Stage::execute,20);assert(s.orphans.empty()&&core.erased[model]==1);s.ProcessStage(Entity::Stage::execute,20);assert(core.erased[model]==1);std::cout<<"PASS orphan move ownership, parent removal, clock/physics/query, pause and exact-once model retirement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=a.phonkLaser.fragments[0].time=.2;s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].age==0);std::cout<<"PASS newborn time no double advancement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f),CVECTOR(3.f,2.f,1.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&original=a.phonkLaser.fragments[0];original.spin=CVECTOR(0.f,2.f,0.f);a.phonkLaser.time=.4;auto h=hit(a);sl::FragmentProjection p;p.owner=h.owner;p.generation=1;p.token=1;p.changed=true;for(auto offset:{-2.f,2.f}){auto mesh=box(.5f);for(auto&o:mesh)for(auto&t:o.second)for(auto&v:t)v.position.x+=offset;p.successors.push_back(sl::standaloneFragment(std::move(mesh)));}auto expected=p.successors;const auto old=original;deliver(s,h,std::move(p));assert(a.phonkLaser.fragments.size()==2&&!a.model.geo.material.contains(1));for(size_t n=0;n<2;++n){auto&b=a.phonkLaser.fragments[n];const auto&props=expected[n].properties;CVECTOR v(3.f,2.f,1.f-2.f*props.centroid.x);assert(b.token==101+n&&b.time==.4&&(~(b.spin-old.spin))<1.e-12f&&(~(b.velocity-v))<1.e-9f&&std::abs(b.surfaceArea-props.surfaceArea)<1.e-7f&&b.contactShape.valid());}assert(std::abs(a.phonkLaser.fragments[0].mass+a.phonkLaser.fragments[1].mass-old.mass)<1.e-5);s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].age==0);std::cout<<"PASS exact successor metadata/token/order/mass and rigid-point momentum; commit newborn no double step\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto h=hit(a);sl::FragmentProjection p;p.owner=h.owner;p.generation=1;p.token=1;p.changed=true;p.successors.push_back(sl::standaloneFragment(box(.5f)));a.model.geo.failGPU=true;deliver(s,h,p);assert(a.phonkLaser.fragments.size()==1&&a.phonkLaser.fragments[0].token==1&&a.model.geo.material.contains(1));a.model.geo.failGPU=false;a.model.geo.failInput=true;assert(s.Queue(h,bore()));s.PollDamage();assert(!s.future.valid()&&a.phonkLaser.fragments[0].token==1);std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();a.model.geo.generation=2;SHIP_CUT_FRAGMENTS::Work work;work.valid=true;work.request={h,bore()};work.projection=p;promise.set_value(std::move(work));auto calls=a.model.geo.replaceCalls;s.PollDamage();assert(a.model.geo.replaceCalls==calls&&a.phonkLaser.fragments[0].token==1);std::cout<<"PASS GPU/input failure preserves old body and generation changed during worker rejects commit\n";}
 {SHIP a,b;a.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f,0.f),CVECTOR(50.f,0.f,0.f)));b.phonkLaser.fragments.push_back(body(CVECTOR(3.f,10.f,0.f),CVECTOR(-50.f,0.f,0.f)));setup({&a,&b});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=b.phonkLaser.time=.2;s.AdvanceCutFragments();assert(a.phonkLaser.fragments.size()==1&&b.phonkLaser.fragments.size()==1&&s.requests.size()>=2&&a.model.geo.replaceCalls==0&&b.model.geo.replaceCalls==0);assert(a.phonkLaser.fragments[0].world.Pos().x<b.phonkLaser.fragments[0].world.Pos().x);std::cout<<"PASS actual contact Crush queues through solver without body replacement\n";}
 {SHIP a,b;a.phonkLaser.fragments.push_back(body(CVECTOR(-1.f,10.f,0.f)));b.phonkLaser.fragments.push_back(body(CVECTOR(1.f,10.f,0.f)));a.phonkLaser.fragments[0].spin=CVECTOR(0.f,0.f,1.f);setup({&a,&b});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=b.phonkLaser.time=.02;s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].velocity.x<-.01f&&b.phonkLaser.fragments[0].velocity.x>.01f);std::cout<<"PASS pure-spin contact impulse\n";}
 {SHIP a,deck;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,3.f,0.f),CVECTOR(0.f,-20.f,0.f)));setup({&a,&deck});deck.State.fWeight=1;deck.model.box={{-100,-.001f,-100},{100,.001f,100}};deck.model.triangles.push_back({CVECTOR(-100.f,0.f,-100.f),CVECTOR(100.f,0.f,-100.f),CVECTOR(0.f,0.f,100.f)});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=.2;s.AdvanceCutFragments();assert(deck.discardedY==0&&deck.applied.y==0&&deck.appliedYaw==0&&a.phonkLaser.fragments[0].world.Pos().y>.98f);std::cout<<"PASS native planar receiver preserves deck support and zero vertical/pitch/roll response\n";}
 {auto fleet=population(10,30);SHIP_CUT_FRAGMENTS s;bool beforeAnyStep=false;for(auto&a:fleet)a->model.geo.onRelease=[&]{if(count(fleet)==300){beforeAnyStep=true;for(auto&ship:fleet)for(auto&b:ship->phonkLaser.fragments)assert(b.age==0&&b.time==0);}};s.AdvanceCutFragments();assert(beforeAnyStep&&count(fleet)==256);unsigned retired=0;for(auto&a:fleet)retired+=a->model.geo.released;assert(retired==44);for(auto&a:fleet){a->model.geo.onRelease={};a->phonkLaser.time=.02;}s.AdvanceCutFragments();assert(count(fleet)==256);for(auto&a:fleet)for(auto&b:a->phonkLaser.fragments)assert(b.age>.019f);std::cout<<"PASS global cap300 to256 before zero-elapsed solver admission, then all admitted bodies step\n";}
 {std::vector<std::unique_ptr<SHIP>>ships;SHIP_CUT_FRAGMENTS s;core={};for(int n=0;n<33;++n){auto a=std::make_unique<SHIP>();a->modelID=100+n;a->blots_id=a->model.geo.owner=200+n;a->phonkLaser.fragments.push_back(body(CVECTOR(n*5.f,20.f,0.f)));a->model.geo.material.emplace(1,sl::standaloneFragment(box()));core.entities[a->modelID]=&a->model;assert(s.Adopt(*a));ships.push_back(std::move(a));}assert(s.orphans.size()==32&&core.erased[100]==1);for(auto&o:s.orphans)o.pieces[0].age=180;s.ProcessStage(Entity::Stage::execute,20);assert(s.orphans.empty());for(int n=100;n<133;++n)assert(core.erased[n]==1);std::cout<<"PASS orphan MODEL cap32 and finite dry/beached lifetime exact-once retirement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto shell=box();sl::appendMesh(shell,box(.8f,true));a.model.geo.material[1]=sl::standaloneFragment(std::move(shell));assert(s.Queue(hit(a),bore()));s.PollDamage();assert(s.future.valid());s.future.wait();s.PollDamage();assert(a.model.geo.replaceCalls==1&&a.phonkLaser.fragments.size()==2);std::cout<<"PASS actual immutable async prepareFragment worker through scene commit (closed hollow input)\n";}
 {auto fleet=population(16,16);auto&a=*fleet[0];SHIP_CUT_FRAGMENTS s;sl::FragmentProjection p;p.owner=a.blots_id;p.generation=1;p.token=1;p.changed=true;p.successors={sl::standaloneFragment(box(.5f)),sl::standaloneFragment(box(.5f))};a.model.geo.next=1000;SHIP_CUT_FRAGMENTS::Work w;w.valid=true;w.request={hit(a),bore()};w.projection=std::move(p);std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();promise.set_value(std::move(w));s.ProcessStage(Entity::Stage::execute,20);assert(count(fleet)==256&&a.model.geo.replaceCalls==1);for(auto&ship:fleet)for(auto&b:ship->phonkLaser.fragments)assert(b.age==0);std::cout<<"PASS full ProcessStage trims post-Poll successors to256 before next render/solve, newborn no advancement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto h=s.Trace(CVECTOR(0.f,20.f,2.f),CVECTOR(0.f,20.f,-2.f));assert(h&&h.normal.z>.99f);bool incoming=s.Impact(h,CVECTOR(0.f,0.f,-100.f),10.f,0);float incomingZ=a.phonkLaser.fragments[0].velocity.z;assert(incoming&&incomingZ<0&&s.requests.size()==1&&s.requests[0].plane.beams[0].direction[2]<-.99f);auto oldVelocity=a.phonkLaser.fragments[0].velocity;s.requests.clear();bool outgoing=s.Impact(h,CVECTOR(0.f,0.f,100.f),10.f,0);assert(!outgoing&&(~(a.phonkLaser.fragments[0].velocity-oldVelocity))<1.e-12f&&s.requests.empty());std::cout<<"PASS front incoming velocity-Z pushes/cuts inward, bodyDeltaZ="<<incomingZ<<"; outgoing velocity+Z adds zero impulse/crush\n";}
 {auto fleet=population(10,30);for(auto&a:fleet){a->phonkLaser.time=.2;for(auto&b:a->phonkLaser.fragments)b.time=.2;}SHIP_CUT_FRAGMENTS s;s.AdvanceCutFragments();assert(count(fleet)==256);for(auto&a:fleet)for(auto&b:a->phonkLaser.fragments)assert(b.age==0&&b.time==.2);std::cout<<"PASS newborn300 population bounded to256 without spurious step\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];p.world.BuildMatrix(CVECTOR(0.f,1.57079632679f,0.f),CVECTOR(10.f,20.f,30.f));auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));CVECTOR point;assert(s.Position(h,point)&&(~(point-p.world*h.point))<1.e-10f);p.world.Pos().x+=4;assert(s.Position(h,point)&&std::abs(point.x-15.f)<1.e-5f);CVECTOR direction=p.world*CVECTOR(0.f,0.f,-1.f)-p.world.Pos();assert(s.Beam(h,direction,.2f)&&s.requests.size()==1);const auto&beam=s.requests[0].plane.beams[0];assert(std::abs(beam.start[2]-1.f)<1.e-6f&&beam.direction[2]<-.99f&&beam.depth==.2f);auto stale=h;stale.token++;assert(!s.Position(stale,point)&&!s.Beam(stale,direction,.2f));assert(!s.Beam(h,CVECTOR(0.f),.2f)&&!s.Beam(h,direction,0.f));std::cout<<"PASS Position follows current rotated/translating material; Beam queues local hit/direction with stale/zero negatives\n";}
 int failures=scaleProbe();std::cout<<"SCALE contract failures="<<failures<<"; thin timber overlapping recut remains known kernel rejection; no native scene acceptance\n";return failures?1:0;
}

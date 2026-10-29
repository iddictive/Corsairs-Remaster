#include "ship.h"
#include "ship_contact_material.hpp"
#include "ship_reactive_collision.hpp"
#include "collision_triangles.hpp"
#include "core.h"
#include "controls.h"
#include "math_inlines.h"
#include <bit>
#include <unordered_set>

namespace
{
using Parts = std::vector<std::vector<roundhouse::ReactivePoint>>;
Parts HullParts(const HULL_CONTACT_MATERIAL &material)
{
    using namespace storm::ship::contact;
    Parts result;
    if (!material.prepared) return result;
    const auto &snapshot = *material.prepared;
    const auto append = [&](const MaterialPiece &piece) {
        if (!materialSupportRetained(snapshot, piece.supportCell)) return;
        auto vertices = roundhouse::ReactivePrismVertices(piece);
        if (vertices.size() >= 6) result.push_back(std::move(vertices));
    };
    std::unordered_set<uint32_t> replacements;
    for (const auto &piece : materialRoot(snapshot).pieces)
    {
        if (const auto *changed = materialReplacement(snapshot, piece.source))
        {
            if (replacements.insert(piece.source).second) for (const auto &p : *changed) append(p);
        }
        else append(piece);
    }
    return result;
}

Parts CollisionParts(MODEL *model, uint64_t owner, const HULL_CONTACT_MATERIAL &material)
{
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return {};
    storm::ship::material::convex::ContactSnapshot snapshot;
    std::shared_ptr<const storm::ship::contact::MaterialSnapshot> prepared;
    if (!root->geo->GetHullContactSnapshot(owner, snapshot, prepared) ||
        prepared != material.prepared || snapshot.revision != material.revision) return {};
    const auto &m = root->loc_mtx;
    return roundhouse::ReactiveCollisionParts(snapshot, {m.Vx().x,m.Vx().y,m.Vx().z,
        m.Vy().x,m.Vy().y,m.Vy().z,m.Vz().x,m.Vz().y,m.Vz().z,m.Pos().x,m.Pos().y,m.Pos().z});
}

CVECTOR Angles(const CMatrix &m)
{
    const float pitch = std::asin(std::clamp(-m.m[2][1], -1.f, 1.f));
    if (std::abs(std::cos(pitch)) > .0001f)
        return CVECTOR(pitch, std::atan2(m.m[2][0], m.m[2][2]), std::atan2(m.m[0][1], m.m[1][1]));
    return CVECTOR(pitch, std::atan2(-m.m[0][2], m.m[0][0]), 0.f);
}

bool Held(const char *name)
{
    CONTROL_STATE state{};
    core.Controls->GetControlState(name, state);
    return state.state == CST_ACTIVE || state.state == CST_ACTIVATED;
}

CVECTOR WorldSpeed(const SHIP_STATE &state)
{
    const float c = std::cos(state.vAng.y), s = std::sin(state.vAng.y);
    return CVECTOR(KNOTS2METERS(state.vSpeed.x * c + state.vSpeed.z * s), state.vSpeed.y,
                   KNOTS2METERS(state.vSpeed.z * c - state.vSpeed.x * s));
}

uint64_t HashWord(uint64_t hash, uint64_t word) { return (hash ^ word) * 1099511628211ULL; }
uint64_t SurfaceStamp(uint64_t hash, NODE *node)
{
    if (!node) return hash;
    hash = HashWord(hash, unsigned(node->flags & (NODE::CLIP_ENABLE | NODE::CLIP_ENABLE_TREE)));
    if (node->geo)
    {
        hash = HashWord(hash, reinterpret_cast<uintptr_t>(node->geo));
        hash = HashWord(hash, node->geo->GeometryGeneration());
        hash = HashWord(hash, node->geo->CollisionRevision());
    }
    for (float v : node->loc_mtx.matrix) hash = HashWord(hash, std::bit_cast<uint32_t>(v));
    for (auto *child : node->next) hash = SurfaceStamp(hash, child);
    return hash;
}
}

bool SHIP::PreparePhonkJetWorld()
{
    // Fixed installed engine rating at the hull's authored full-load design
    // displacement. Taking cargo aboard changes acceleration, not this force.
    phonkJet.installedThrust = (State.fWeight+std::max(0,SP.iCapacity))*50.f*9.81f*1.6f;
    if (!phonkJet.world) phonkJet.world = std::make_shared<roundhouse::ReactiveWorld>();
    HULL_CONTACT_MATERIAL material;
    if (!GetHullContactMaterial(material) || !material.prepared)
    {
        EnsureHullMaterial();
        if (!GetHullContactMaterial(material) || !material.prepared) return false;
    }
    if (phonkJet.hull.material != material.prepared || phonkJet.hull.revision != material.revision)
    {
        if (!phonkJet.world->SetHull(HullParts(material), CollisionParts(GetModel(), uint64_t(blots_id), material))) return false;
        phonkJet.hull = {material.prepared, material.revision};
    }
    // Draft can change after cargo/flood reconciliation without a material
    // revision. The shape owner caches this query by shape and water height.
    const float displaced = phonkJet.world->SubmergedFraction(State.fShipImmersion + SP.fWaterLine);
    if (!(displaced > .0001f)) return false;
    phonkJet.buoyancy = 1.f / displaced;
    const auto surfaces = pIsland ? pIsland->GetWorldSurfaces() : std::vector<ISLAND_BASE::WorldSurface>{};
    uint64_t stamp = 1469598103934665603ULL;
    for (const auto &surface : surfaces)
    {
        auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(surface.model));
        stamp = HashWord(stamp, uint64_t(surface.model));
        for (float v : surface.authored.matrix) stamp = HashWord(stamp, std::bit_cast<uint32_t>(v));
        stamp = SurfaceStamp(stamp, model ? model->GetNode(0) : nullptr);
    }
    const auto position = GetPos();
    const float bodyRadius = .5f * std::sqrt(~State.vBoxSize);
    const float margin = bodyRadius + std::sqrt(~WorldSpeed(State)) * .25f + 32.f;
    if (stamp != phonkJet.terrainStamp ||
        std::sqrt(~(position - phonkJet.terrainCenter)) + margin > phonkJet.terrainRadius)
    {
        const float radius = std::max(600.f, margin * 2.f);
        std::vector<CVECTOR> triangles;
        storm::physics::TriangleSink sink(triangles);
        for (const auto &surface : surfaces)
        {
            auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(surface.model));
            if (!model) continue;
            struct Restore { MODEL &model; CMatrix matrix; ~Restore() { model.mtx = matrix; model.Update(); } } restore{*model, model->mtx};
            model->mtx = surface.authored;
            model->Update();
            model->Clip(nullptr, 0, position, radius, storm::physics::TriangleSink::Add);
        }
        if (!phonkJet.world->SetStaticTriangles(triangles)) return false;
        phonkJet.terrainStamp = stamp;
        phonkJet.terrainCenter = position;
        phonkJet.terrainRadius = radius;
    }
    return true;
}

bool SHIP::AdvancePhonkJet(uint32_t milliseconds, bool prediction)
{
    const bool thrust = PhonkMotionAllowed() && Held("Phonk_Jet");
    if (!phonkJet.active && (!thrust || prediction)) return false;
    if (!pSea || bSetFixed || !milliseconds) return phonkJet.active;
    if (!PreparePhonkJetWorld())
    {
        if (phonkJet.active && !phonkJet.preparationReported)
            core.Trace("Phonk jet: ship %llu contact projection pending; retaining pose/momentum and retrying", uint64_t(GetId()));
        phonkJet.preparationReported = true;
        return phonkJet.active;
    }
    const float seconds = DELTA_TIME(milliseconds);
    const float draft = State.fShipImmersion + SP.fWaterLine;
    roundhouse::ReactiveBody body;
    body.pose = CMatrix(State.vAng, State.vPos + CVECTOR(fXOffset, -draft, fZOffset));
    body.velocity = WorldSpeed(State);
    body.angularVelocity = State.vRotate;
    body.mass = float(GetHullContactMass() * 50.);
    if (!phonkJet.active)
    {
        if (phonkDrift.initialized)
        {
            body.velocity.x = KNOTS2METERS(phonkDrift.worldX);
            body.velocity.z = KNOTS2METERS(phonkDrift.worldZ);
        }
        body.velocity.y += phonkFlight.velocity;
    }
    const auto position = body.pose.Pos();
    roundhouse::ReactiveFluid fluid;
    fluid.height = pSea->WaveXZ(position.x, position.z);
    fluid.normal = !CVECTOR(pSea->WaveXZ(position.x-1.f, position.z) - pSea->WaveXZ(position.x+1.f, position.z),
                            2.f, pSea->WaveXZ(position.x, position.z-1.f) - pSea->WaveXZ(position.x, position.z+1.f));
    fluid.buoyancy = phonkJet.buoyancy;
    const float pitch = (Held("Ship_SailUp") || Held("Ship_SailUp1") ? .65f : 0.f) -
                        (Held("Ship_SailDown") || Held("Ship_SailDown1") ? .65f : 0.f);
    const float roll = (Held("Ship_TurnLeft") ? .65f : 0.f) - (Held("Ship_TurnRight") ? .65f : 0.f);
    phonkJet.forces.clear();
    if (thrust)
    {
        const float force = phonkJet.installedThrust;
        const auto &root = storm::ship::contact::materialRoot(*phonkJet.hull.material);
        float bottom = 0.f;
        for (const auto &piece : root.pieces) bottom = std::min(bottom, float(piece.lowerY));
        for (unsigned i = 0; i < 4; ++i)
        {
            const float x = i & 1 ? 1.f : -1.f, z = i & 2 ? 1.f : -1.f;
            phonkJet.forces.push_back({CVECTOR(x*State.vBoxSize.x*.3f, bottom-.1f, z*State.vBoxSize.z*.3f),
                CVECTOR(0.f, force*.25f*std::clamp(1.f-pitch*z+roll*x, 0.f, 2.f), 0.f)});
        }
    }
    std::vector<roundhouse::ReactiveNeighbor> neighbors;
    std::vector<SHIP *> ships;
    std::vector<CVECTOR> beforeVelocity, beforeAngular;
    for (auto id : core.GetEntityIds("ship"))
    {
        auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || !ship->bMounted || ship->HasRigidFlight()) continue;
        const float range = .5f*(std::sqrt(~State.vBoxSize)+std::sqrt(~ship->State.vBoxSize)) +
            std::sqrt(~body.velocity)*seconds + 8.f;
        if (~(ship->GetPos()-GetPos()) > range*range) continue;
        HULL_CONTACT_MATERIAL material;
        if (!ship->GetHullContactMaterial(material) || !material.prepared) continue;
        auto &cached = phonkJet.neighbors[id];
        if (cached.material != material.prepared || cached.revision != material.revision)
        {
            if (!phonkJet.world->SetNeighborHull(uint64_t(id), HullParts(material),
                CollisionParts(ship->GetModel(), uint64_t(ship->blots_id), material))) continue;
            cached = {material.prepared, material.revision};
        }
        roundhouse::ReactiveBody other;
        other.pose = CMatrix(ship->GetAng(), ship->GetPos() - CVECTOR(0.f, ship->State.fShipImmersion+ship->SP.fWaterLine, 0.f));
        other.velocity = WorldSpeed(ship->State);
        other.angularVelocity = CVECTOR(0.f, ship->State.vRotate.y, 0.f);
        other.mass = float(ship->GetHullContactMass()*50.);
        if (!(other.mass > 0.f)) continue;
        neighbors.push_back({uint64_t(id), other});
        ships.push_back(ship); beforeVelocity.push_back(other.velocity); beforeAngular.push_back(other.angularVelocity);
    }
    if (!phonkJet.world->Step(body, phonkJet.forces, fluid, seconds, &neighbors))
    {
        if (phonkJet.active && !phonkJet.preparationReported)
            core.Trace("Phonk jet: ship %llu invalid rigid step (Jolt update bits 0x%x); retaining pose/momentum and retrying",
                uint64_t(GetId()), phonkJet.world->UpdateError());
        phonkJet.preparationReported = true;
        return phonkJet.active;
    }
    phonkJet.preparationReported = false;
    phonkJet.active = true;
    phonkJet.wet = phonkJet.world->Wet();
    State.vAng = Angles(body.pose);
    State.vPos = body.pose.Pos() - CVECTOR(fXOffset, -draft, fZOffset);
    const float c = std::cos(State.vAng.y), s = std::sin(State.vAng.y);
    State.vSpeed = CVECTOR(METERS2KNOTS(body.velocity.x*c-body.velocity.z*s), body.velocity.y,
                           METERS2KNOTS(body.velocity.x*s+body.velocity.z*c));
    State.vRotate = body.angularVelocity;
    if (!prediction)
    {
        phonkDrift.reset(); phonkFlight.reset();
        bGroundSupported = bGroundFalling = bLandContact = bShip2Strand = false;
        groundWaterVelocityY = 0.f;
        for (std::size_t i = 0; i < neighbors.size(); ++i)
            ships[i]->ApplyHullContactMotion(neighbors[i].body.velocity-beforeVelocity[i],
                neighbors[i].body.angularVelocity.y-beforeAngular[i].y, ships[i]->GetPos(), ships[i]->GetAng().y, 0);
        if (thrust) ApplyPhonkJetExhaust(seconds);
        const bool settled = !thrust && phonkJet.wet &&
            std::abs(State.vPos.y-fluid.height)<.25f && std::abs(body.velocity.y)<.3f &&
            ~body.angularVelocity<.01f && std::abs(State.vAng.x)<.18f && std::abs(State.vAng.z)<.18f;
        phonkJet.settledSeconds = settled ? phonkJet.settledSeconds+seconds : 0.f;
        if (phonkJet.settledSeconds > 2.f)
        {
            if (isDead()) { vOldPos=State.vPos; vOldAng=State.vAng; vCurDeadDir=CVECTOR(0.f); }
            phonkJet = {};
        }
    }
    return true;
}

void SHIP::ApplyPhonkJetExhaust(float seconds)
{
    const float draft = State.fShipImmersion + SP.fWaterLine;
    CMatrix frame(State.vAng, State.vPos+CVECTOR(fXOffset,-draft,fZOffset));
    const CVECTOR direction = -frame.Vy();
    const auto owner = GetACharacter() ? int32_t(GetACharacter()->GetAttributeAsDword("index", uint32_t(-1))) : -1;
    for (const auto &jet : phonkJet.forces)
    {
        if (!(jet.force.y > 0.f)) continue;
        const CVECTOR from = frame*jet.point, to = from+direction*35.f;
        entid_t receiver = 0;
        float nearest = pIsland ? pIsland->TraceWorldWeapon(from,to,receiver) : 2.f;
        SHIP *hit = nullptr;
        for (auto id : core.GetEntityIds("ship"))
        {
            auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(id));
            if (!ship || ship==this || !ship->bMounted || !ship->GetModel()) continue;
            const float t = ship->GetModel()->TraceLaserMaterial(from,to);
            if (t <= 1.f && t < nearest) { nearest=t; hit=ship; }
        }
        if (!hit) continue;
        const auto impulse = direction*(jet.force.y*seconds);
        const float mass = float(hit->GetHullContactMass()*50.);
        if (!(mass>0.f)) continue;
        const auto point = from+(to-from)*nearest;
        HULL_CONTACT_MATERIAL material;
        if (!hit->GetHullContactMaterial(material) || !material.prepared) continue;
        auto &cached = phonkJet.neighbors[hit->GetId()];
        if (cached.material!=material.prepared || cached.revision!=material.revision)
        {
            if (!phonkJet.world->SetNeighborHull(uint64_t(hit->GetId()),HullParts(material),
                CollisionParts(hit->GetModel(), uint64_t(hit->blots_id), material))) continue;
            cached={material.prepared,material.revision};
        }
        roundhouse::ReactiveBody other;
        other.pose=CMatrix(hit->GetAng(),hit->GetPos()-CVECTOR(0.f,hit->State.fShipImmersion+hit->SP.fWaterLine,0.f));
        other.velocity=WorldSpeed(hit->State);other.angularVelocity=CVECTOR(0.f,hit->State.vRotate.y,0.f);other.mass=mass;
        const auto before=other;
        if (phonkJet.world->ImpulseNeighbor(uint64_t(hit->GetId()),other,impulse,point))
            hit->ApplyHullContactMotion(other.velocity-before.velocity,other.angularVelocity.y-before.angularVelocity.y,
                                       hit->GetPos(),hit->GetAng().y,0);
#ifdef STORM_METAL_SHIP_FIRE
        // Exhaust energy: dm = F dt / v, E = 1/2 dm v^2. Fire owns heat
        // transfer, material response and damage; no separate HP script.
        hit->IgniteShipFire(point, .5f*jet.force.y*650.f*seconds, owner, false);
#endif
    }
}

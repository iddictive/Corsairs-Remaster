#include "roundhouse_ragdoll.hpp"
#include "reactive_body.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <string>
#include <unordered_map>

namespace roundhouse
{
namespace
{
constexpr JPH::ObjectLayer StaticLayer = 0;
constexpr JPH::ObjectLayer MovingLayer = 1;
constexpr std::size_t MaxRagdolls = 64;
constexpr std::size_t MaxParts = 15;
constexpr float FixedStep = 1.0f / 60.0f;
constexpr int MaxSteps = 4;

class BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface
{
  public:
    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return JPH::BroadPhaseLayer(static_cast<JPH::uint8>(layer));
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return layer.GetValue() == 0 ? "Location" : "Ragdoll";
    }
#endif
};

class BroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
  public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer other) const override
    {
        return layer == MovingLayer || other.GetValue() == MovingLayer;
    }
};

class ObjectFilter final : public JPH::ObjectLayerPairFilter
{
  public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        return a == MovingLayer || b == MovingLayer;
    }
};

// Jolt's registration is process-global; the last live location releases it.
// A foreign Jolt owner, if one is added later, retains its own registration.
class Runtime
{
  public:
    Runtime() : ownsFactory_(JPH::Factory::sInstance == nullptr)
    {
        if (ownsFactory_)
        {
            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
        }
    }
    ~Runtime()
    {
        if (ownsFactory_)
        {
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
    }

  private:
    bool ownsFactory_;
};

std::shared_ptr<Runtime> AcquireRuntime()
{
    static std::mutex mutex;
    static std::weak_ptr<Runtime> shared;
    const std::lock_guard lock(mutex);
    auto result = shared.lock();
    if (!result)
    {
        result = std::make_shared<Runtime>();
        shared = result;
    }
    return result;
}

bool Finite(const CVECTOR &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool Finite(const CMatrix &m)
{
    return std::all_of(std::begin(m.matrix), std::end(m.matrix), [](float v) { return std::isfinite(v); });
}

JPH::Vec3 Vec(const CVECTOR &v) { return {v.x, v.y, v.z}; }

JPH::Mat44 Matrix(const CMatrix &m)
{
    return {JPH::Vec4(m.matrix[0], m.matrix[1], m.matrix[2], 0.0f),
            JPH::Vec4(m.matrix[4], m.matrix[5], m.matrix[6], 0.0f),
            JPH::Vec4(m.matrix[8], m.matrix[9], m.matrix[10], 0.0f),
            JPH::Vec4(m.matrix[12], m.matrix[13], m.matrix[14], 1.0f)};
}

CMatrix Matrix(const JPH::Mat44 &m)
{
    CMatrix result;
    for (int col = 0; col != 4; ++col)
        m.GetColumn4(col).StoreFloat4(reinterpret_cast<JPH::Float4 *>(&result.matrix[4 * col]));
    return result;
}

JPH::Mat44 BodyFrame(const BodyDefinition &body, const std::vector<CMatrix> &pose)
{
    const auto &joint = pose[body.joint];
    const JPH::Vec3 start = Vec(joint.v.pos);
    const JPH::Vec3 end = body.endJoint >= 0 ? Vec(pose[body.endJoint].v.pos) : start;
    const JPH::Vec3 y = (body.endJoint >= 0 ? end - start : Vec(joint.v.vy)).NormalizedOr(JPH::Vec3::sAxisY());
    const JPH::Vec3 hint = Vec(joint.v.vx);
    JPH::Vec3 projected = hint - y * hint.Dot(y);
    // Arm segments can be parallel to the animated joint's X axis. Do not
    // normalize floating-point cancellation into a non-orthogonal frame.
    if (projected.LengthSq() < 1.0e-6f)
        projected = Vec(joint.v.vz).Cross(y);
    const JPH::Vec3 z = projected.NormalizedOr(y.GetNormalizedPerpendicular()).Cross(y).Normalized();
    const JPH::Vec3 x = y.Cross(z);
    return {JPH::Vec4(x, 0.0f), JPH::Vec4(y, 0.0f), JPH::Vec4(z, 0.0f),
            JPH::Vec4((start + end) * 0.5f, 1.0f)};
}

bool ValidRig(const RigDefinition &rig, const std::vector<CMatrix> &pose)
{
    const int count = static_cast<int>(pose.size());
    if (count == 0 || rig.jointParents.size() != pose.size() || rig.bindWorld.size() != pose.size() ||
        rig.bodies.empty() || rig.bodies.size() > MaxParts || rig.headBody < 0 ||
        rig.headBody >= static_cast<int>(rig.bodies.size()))
        return false;
    for (int i = 0; i != count; ++i)
    {
        if (!Finite(pose[i]) || !Finite(rig.bindWorld[i]) || rig.jointParents[i] < -1 || rig.jointParents[i] >= count)
            return false;
    }
    for (int i = 0; i != count; ++i)
    {
        int ancestor = i;
        for (int n = 0; ancestor >= 0; ++n)
        {
            if (n >= count)
                return false;
            ancestor = rig.jointParents[ancestor];
        }
    }
    std::vector<bool> mapped(count, false);
    for (std::size_t i = 0; i != rig.bodies.size(); ++i)
    {
        const auto &body = rig.bodies[i];
        if (body.joint < 0 || body.joint >= count || mapped[body.joint] || body.endJoint < -1 || body.endJoint >= count ||
            body.parentBody < -1 || body.parentBody >= static_cast<int>(i) ||
            (i == 0 ? body.parentBody != -1 : body.parentBody == -1) ||
            !std::isfinite(body.radius) || body.radius <= 0.0f || !std::isfinite(body.mass) || body.mass <= 0.0f ||
            !std::isfinite(body.swingRadians) || body.swingRadians < 0.0f || body.swingRadians > JPH::JPH_PI ||
            !std::isfinite(body.twistMinRadians) || !std::isfinite(body.twistMaxRadians) ||
            body.twistMinRadians < -JPH::JPH_PI || body.twistMaxRadians > JPH::JPH_PI ||
            body.twistMinRadians > body.twistMaxRadians)
            return false;
        if (body.endJoint >= 0 &&
            ((Vec(pose[body.endJoint].v.pos) - Vec(pose[body.joint].v.pos)).LengthSq() < 1.0e-8f ||
             (Vec(rig.bindWorld[body.endJoint].v.pos) - Vec(rig.bindWorld[body.joint].v.pos)).LengthSq() < 1.0e-8f))
            return false;
        mapped[body.joint] = true;
    }
    return true;
}
} // namespace

struct RagdollWorld::Impl
{
    struct Entry
    {
        JPH::Ref<JPH::Ragdoll> ragdoll;
        std::vector<JPH::Mat44> capturedPose;
        std::vector<JPH::Mat44> inverseBodies;
        std::vector<int> boneBody;
    };

    struct World
    {
        // Registration must outlive every Jolt object below.
        std::shared_ptr<Runtime> runtime = AcquireRuntime();
        BroadPhaseLayers layers;
        BroadPhaseFilter broadFilter;
        ObjectFilter objectFilter;
        JPH::TempAllocatorImpl allocator{8 * 1024 * 1024};
        JPH::JobSystemSingleThreaded jobs{JPH::cMaxPhysicsJobs};
        JPH::PhysicsSystem physics;
        JPH::BodyID staticBody;
        std::unordered_map<Handle, Entry> entries;
        float accumulator = 0.0f;

        World()
        {
            physics.Init(1024, 0, 8192, 8192, layers, broadFilter, objectFilter);
            physics.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
        }
        ~World()
        {
            for (auto &[handle, entry] : entries)
                entry.ragdoll->RemoveFromPhysicsSystem();
            entries.clear();
            if (!staticBody.IsInvalid())
            {
                physics.GetBodyInterface().RemoveBody(staticBody);
                physics.GetBodyInterface().DestroyBody(staticBody);
            }
        }

        bool SetMesh(const std::vector<CVECTOR> &vertices)
        {
            JPH::BodyID replacement;
            auto &bodies = physics.GetBodyInterface();
            if (!vertices.empty())
            {
                JPH::TriangleList triangles;
                triangles.reserve(vertices.size() / 3);
                for (std::size_t i = 0; i != vertices.size(); i += 3)
                    triangles.emplace_back(Vec(vertices[i]), Vec(vertices[i + 1]), Vec(vertices[i + 2]));
                auto shape = JPH::MeshShapeSettings(triangles).Create();
                if (shape.HasError())
                    return false;
                JPH::BodyCreationSettings settings(shape.Get(), JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
                                                  JPH::EMotionType::Static, StaticLayer);
                settings.mFriction = 0.7f;
                replacement = bodies.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
                if (replacement.IsInvalid())
                    return false;
            }
            if (!staticBody.IsInvalid())
            {
                bodies.RemoveBody(staticBody);
                bodies.DestroyBody(staticBody);
            }
            staticBody = replacement;
            for (auto &[handle, entry] : entries)
                entry.ragdoll->Activate();
            return true;
        }
    };

    std::vector<CVECTOR> staticVertices;
    std::unique_ptr<World> world;
    Handle nextHandle = 1;
};

RagdollWorld::RagdollWorld() = default;
RagdollWorld::~RagdollWorld() = default;

bool RagdollWorld::SetStaticTriangles(const std::vector<CVECTOR> &vertices)
{
    if (vertices.size() % 3 != 0 || !std::all_of(vertices.begin(), vertices.end(), [](const auto &v) { return Finite(v); }))
        return false;
    if (!impl_)
        impl_ = std::make_unique<Impl>();
    if (impl_->world && !impl_->world->SetMesh(vertices))
        return false;
    impl_->staticVertices = vertices;
    return true;
}

Handle RagdollWorld::Create(const RigDefinition &rig, const std::vector<CMatrix> &pose,
                           const CVECTOR &impulse, const CVECTOR &impulsePoint)
{
    if (!impl_ || impl_->staticVertices.empty() || !Finite(impulse) || !Finite(impulsePoint) || !ValidRig(rig, pose))
        return 0;
    if (!impl_->world)
    {
        auto world = std::make_unique<Impl::World>();
        if (!world->SetMesh(impl_->staticVertices))
            return 0;
        impl_->world = std::move(world);
    }
    auto &world = *impl_->world;
    if (world.entries.size() >= MaxRagdolls)
        return 0;
    JPH::Ref<JPH::RagdollSettings> settings = new JPH::RagdollSettings();
    settings->mSkeleton = new JPH::Skeleton();
    settings->mParts.resize(rig.bodies.size());
    std::vector<JPH::Mat44> bindBodies, currentBodies;
    bindBodies.reserve(rig.bodies.size());
    currentBodies.reserve(rig.bodies.size());
    Impl::Entry entry;
    entry.boneBody.assign(pose.size(), -1);
    for (std::size_t i = 0; i != rig.bodies.size(); ++i)
    {
        const auto &definition = rig.bodies[i];
        settings->mSkeleton->AddJoint(std::to_string(i), definition.parentBody);
        bindBodies.push_back(BodyFrame(definition, rig.bindWorld));
        currentBodies.push_back(BodyFrame(definition, pose));
        entry.inverseBodies.push_back(currentBodies.back().InversedRotationTranslation());
        entry.boneBody[definition.joint] = static_cast<int>(i);
        auto &part = settings->mParts[i];
        const float length = definition.endJoint >= 0
                                 ? (Vec(pose[definition.endJoint].v.pos) - Vec(pose[definition.joint].v.pos)).Length()
                                 : 0.0f;
        if (length > 2.0f * definition.radius)
            part.SetShape(new JPH::CapsuleShape(0.5f * length - definition.radius, definition.radius));
        else
            part.SetShape(new JPH::SphereShape(definition.radius));
        part.mPosition = JPH::RVec3(currentBodies.back().GetTranslation());
        part.mRotation = currentBodies.back().GetQuaternion();
        part.mMotionType = JPH::EMotionType::Dynamic;
        part.mObjectLayer = MovingLayer;
        part.mMotionQuality = JPH::EMotionQuality::LinearCast;
        part.mLinearDamping = 0.12f;
        part.mAngularDamping = 0.3f;
        part.mFriction = 0.7f;
        part.mRestitution = 0.0f;
        part.mEnhancedInternalEdgeRemoval = true;
        part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        part.mMassPropertiesOverride.mMass = definition.mass;
        if (definition.parentBody >= 0)
        {
            const int parent = definition.parentBody;
            const JPH::Mat44 parentDelta = currentBodies[parent] * bindBodies[parent].InversedRotationTranslation();
            const JPH::Mat44 childDelta = currentBodies[i] * bindBodies[i].InversedRotationTranslation();
            JPH::Ref<JPH::SwingTwistConstraintSettings> constraint = new JPH::SwingTwistConstraintSettings();
            constraint->mPosition1 = constraint->mPosition2 = JPH::RVec3(Vec(pose[definition.joint].v.pos));
            constraint->mTwistAxis1 = parentDelta.Multiply3x3(bindBodies[i].GetAxisY());
            constraint->mPlaneAxis1 = parentDelta.Multiply3x3(bindBodies[i].GetAxisX());
            constraint->mTwistAxis2 = childDelta.Multiply3x3(bindBodies[i].GetAxisY());
            constraint->mPlaneAxis2 = childDelta.Multiply3x3(bindBodies[i].GetAxisX());
            constraint->mNormalHalfConeAngle = definition.swingRadians;
            constraint->mPlaneHalfConeAngle = definition.swingRadians;
            constraint->mTwistMinAngle = definition.twistMinRadians;
            constraint->mTwistMaxAngle = definition.twistMaxRadians;
            constraint->mMaxFrictionTorque = 0.5f;
            part.mToParent = constraint;
        }
    }
    settings->DisableParentChildCollisions(currentBodies.data());
    settings->CalculateConstraintPriorities();
    if (!settings->Stabilize())
        return 0;
    Handle handle;
    do
    {
        handle = impl_->nextHandle++;
    } while (handle == 0 || world.entries.count(handle) != 0);
    entry.ragdoll = settings->CreateRagdoll(handle, handle, &world.physics);
    if (entry.ragdoll == nullptr)
        return 0;
    entry.capturedPose.reserve(pose.size());
    for (std::size_t i = 0; i != pose.size(); ++i)
    {
        entry.capturedPose.push_back(Matrix(pose[i]));
        if (entry.boneBody[i] < 0)
        {
            int ancestor = rig.jointParents[i];
            while (ancestor >= 0 && entry.boneBody[ancestor] < 0)
                ancestor = rig.jointParents[ancestor];
            entry.boneBody[i] = ancestor < 0 ? 0 : entry.boneBody[ancestor];
        }
    }
    entry.ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
    world.physics.GetBodyInterface().AddImpulse(entry.ragdoll->GetBodyID(rig.headBody), Vec(impulse), JPH::RVec3(Vec(impulsePoint)));
    world.entries.emplace(handle, std::move(entry));
    return handle;
}

void RagdollWorld::Step(float seconds)
{
    if (!impl_ || !impl_->world || !(seconds > 0.0f) || !std::isfinite(seconds))
        return;
    auto &world = *impl_->world;
    if (world.entries.empty() || world.physics.GetNumActiveBodies(JPH::EBodyType::RigidBody) == 0)
    {
        world.accumulator = 0.0f;
        return;
    }
    world.accumulator = std::min(world.accumulator + seconds, FixedStep * MaxSteps);
    for (int steps = 0; steps < MaxSteps && world.accumulator >= FixedStep; ++steps)
    {
        world.physics.Update(FixedStep, 1, &world.allocator, &world.jobs);
        world.accumulator -= FixedStep;
    }
}

bool RagdollWorld::GetPose(Handle handle, std::vector<CMatrix> &pose) const
{
    if (!impl_ || !impl_->world)
        return false;
    const auto &world = *impl_->world;
    const auto found = world.entries.find(handle);
    if (found == world.entries.end())
        return false;
    const auto &entry = found->second;
    std::vector<JPH::Mat44> deltas;
    deltas.reserve(entry.inverseBodies.size());
    for (std::size_t i = 0; i != entry.inverseBodies.size(); ++i)
        deltas.push_back(world.physics.GetBodyInterface().GetWorldTransform(entry.ragdoll->GetBodyID(static_cast<int>(i))) * entry.inverseBodies[i]);
    pose.resize(entry.capturedPose.size());
    for (std::size_t i = 0; i != pose.size(); ++i)
        pose[i] = Matrix(deltas[entry.boneBody[i]] * entry.capturedPose[i]);
    return true;
}

bool RagdollWorld::IsSleeping(Handle handle) const
{
    if (!impl_ || !impl_->world)
        return false;
    const auto found = impl_->world->entries.find(handle);
    return found != impl_->world->entries.end() && !found->second.ragdoll->IsActive();
}

void RagdollWorld::Remove(Handle handle)
{
    if (!impl_ || !impl_->world)
        return;
    auto &entries = impl_->world->entries;
    const auto found = entries.find(handle);
    if (found == entries.end())
        return;
    found->second.ragdoll->RemoveFromPhysicsSystem();
    entries.erase(found);
    if (entries.empty())
        impl_->world.reset();
}

void RagdollWorld::Clear()
{
    if (impl_)
        impl_->world.reset();
}

std::size_t RagdollWorld::Size() const
{
    return impl_ && impl_->world ? impl_->world->entries.size() : 0;
}

struct ReactiveWorld::Impl
{
    // Both native consumers retain the same process-global registration lease.
    std::shared_ptr<Runtime> runtime = AcquireRuntime();
    BroadPhaseLayers layers;
    BroadPhaseFilter broadFilter;
    ObjectFilter objectFilter;
    // The 8192-contact solver requests a 3.75 MiB block even for one active
    // body. Keep the existing native 8 MiB budget, with Jolt's overflow owner.
    JPH::TempAllocatorImplWithMallocFallback allocator{8 * 1024 * 1024};
    JPH::JobSystemSingleThreaded jobs{JPH::cMaxPhysicsJobs};
    JPH::PhysicsSystem physics;
    JPH::RefConst<JPH::Shape> hull;
    JPH::RefConst<JPH::Shape> collisionHull;
    JPH::MassProperties hullMass;
    std::unordered_map<uint64_t, JPH::RefConst<JPH::Shape>> neighborShapes;
    std::unordered_map<uint64_t, JPH::MassProperties> neighborMass;
    JPH::BodyID terrain, moving;
    bool wet = false;
    JPH::EPhysicsUpdateError updateError = JPH::EPhysicsUpdateError::None;
    float submergedHeight = NAN, submergedFraction = 0.f;

    Impl()
    {
        physics.Init(1024, 0, 8192, 8192, layers, broadFilter, objectFilter);
        physics.SetGravity(JPH::Vec3(0.f, -9.81f, 0.f));
    }
    void Remove(JPH::BodyID &id)
    {
        if (id.IsInvalid()) return;
        auto &bodies = physics.GetBodyInterface();
        bodies.RemoveBody(id);
        bodies.DestroyBody(id);
        id = JPH::BodyID();
    }
    ~Impl() { Remove(moving); Remove(terrain); }
    JPH::BodyID Hydrate(const JPH::Shape *shape, const JPH::MassProperties &mass,
                        const ReactiveBody &body, bool planar)
    {
        const auto frame = Matrix(body.pose);
        JPH::BodyCreationSettings settings(shape, frame.GetTranslation(), frame.GetQuaternion(),
            JPH::EMotionType::Dynamic, planar ? StaticLayer : MovingLayer);
        // The immutable retained shape owns these integrals. Cargo changes
        // scale the same tensor; a retained-shape revision replaces the cache.
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        settings.mMassPropertiesOverride = mass;
        settings.mMassPropertiesOverride.ScaleToMass(body.mass);
        settings.mMassPropertiesOverride.mInertia(3, 3) = 1.f;
        settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
        settings.mLinearDamping = settings.mAngularDamping = 0.f;
        settings.mMaxLinearVelocity = 10000.f;
        settings.mMaxAngularVelocity = 100.f;
        if (planar)
        {
            settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationZ |
                JPH::EAllowedDOFs::RotationY;
            settings.mGravityFactor = 0.f;
        }
        auto &bodies = physics.GetBodyInterface();
        auto id = bodies.CreateAndAddBody(settings, JPH::EActivation::Activate);
        if (id.IsInvalid()) return id;
        const auto angular = Vec(body.angularVelocity);
        bodies.SetLinearAndAngularVelocity(id, Vec(body.velocity) +
            angular.Cross(frame.GetRotation() * shape->GetCenterOfMass()), angular);
        return id;
    }
    void Read(JPH::BodyID id, const JPH::Shape *shape, ReactiveBody &body)
    {
        auto &bodies = physics.GetBodyInterface();
        body.pose = Matrix(bodies.GetWorldTransform(id));
        const auto angular = bodies.GetAngularVelocity(id);
        const auto velocity = bodies.GetLinearVelocity(id) - angular.Cross(bodies.GetRotation(id) * shape->GetCenterOfMass());
        body.velocity = CVECTOR(velocity.GetX(), velocity.GetY(), velocity.GetZ());
        body.angularVelocity = CVECTOR(angular.GetX(), angular.GetY(), angular.GetZ());
    }
};

ReactiveWorld::ReactiveWorld() : impl_(std::make_unique<Impl>()) {}
ReactiveWorld::~ReactiveWorld() = default;

static JPH::RefConst<JPH::Shape> ReactiveShape(const std::vector<std::vector<ReactivePoint>> &parts)
{
    JPH::StaticCompoundShapeSettings compound;
    for (const auto &part : parts)
    {
        if (part.size() < 4) return nullptr;
        double lower[3] = {INFINITY, INFINITY, INFINITY};
        double upper[3] = {-INFINITY, -INFINITY, -INFINITY};
        for (const auto &v : part)
        {
            const double coordinates[] = {v.x, v.y, v.z};
            for (unsigned axis = 0; axis < 3; ++axis)
            {
                if (!std::isfinite(coordinates[axis])) return nullptr;
                lower[axis] = std::min(lower[axis], coordinates[axis]);
                upper[axis] = std::max(upper[axis], coordinates[axis]);
            }
        }
        double center[3], scale[3];
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            center[axis] = .5 * (lower[axis] + upper[axis]);
            // This is the decorator's minimum nonzero scale, not a geometry
            // thickness floor: dividing vertices by it preserves every point.
            scale[axis] = std::max(.5 * (upper[axis] - lower[axis]), 1e-6);
        }
        JPH::Array<JPH::Vec3> points;
        for (const auto &v : part)
            points.emplace_back(float((v.x-center[0])/scale[0]),
                float((v.y-center[1])/scale[1]), float((v.z-center[2])/scale[2]));
        // Jolt's absolute initial-triangle threshold rejects valid tiny native
        // prisms. Build in normalized space, then restore the affine geometry.
        JPH::ConvexHullShapeSettings settings(points, 0.f);
        settings.mHullTolerance = 1e-7f;
        auto shape = settings.Create();
        if (shape.HasError()) return nullptr;
        JPH::RefConst<JPH::Shape> scaled = new JPH::ScaledShape(shape.Get(),
            JPH::Vec3(float(scale[0]), float(scale[1]), float(scale[2])));
        compound.AddShape(JPH::Vec3(float(center[0]), float(center[1]), float(center[2])),
            JPH::Quat::sIdentity(), scaled);
    }
    if (compound.mSubShapes.empty()) return nullptr;
    auto result = compound.Create();
    return result.HasError() ? nullptr : result.Get();
}

bool ReactiveWorld::SetHull(const std::vector<std::vector<ReactivePoint>> &parts)
{
    return SetHull(parts, {});
}

static JPH::RefConst<JPH::Shape> ReactiveCollider(const JPH::Shape *original,
    const std::vector<std::vector<ReactivePoint>> &parts)
{
    if (parts.empty()) return original;
    auto collision = ReactiveShape(parts);
    if (!collision) return nullptr;
    return new JPH::OffsetCenterOfMassShape(collision,
        original->GetCenterOfMass() - collision->GetCenterOfMass());
}

bool ReactiveWorld::SetHull(const std::vector<std::vector<ReactivePoint>> &parts,
                           const std::vector<std::vector<ReactivePoint>> &collisionParts)
{
    auto result = ReactiveShape(parts);
    if (!result) return false;
    auto collision = ReactiveCollider(result, collisionParts);
    if (!collision) return false;
    impl_->Remove(impl_->moving);
    impl_->hull = result;
    impl_->collisionHull = collision;
    impl_->hullMass = result->GetMassProperties();
    impl_->submergedHeight = NAN;
    return true;
}

bool ReactiveWorld::SetNeighborHull(uint64_t key, const std::vector<std::vector<ReactivePoint>> &parts)
{
    return SetNeighborHull(key, parts, {});
}

bool ReactiveWorld::SetNeighborHull(uint64_t key, const std::vector<std::vector<ReactivePoint>> &parts,
                                   const std::vector<std::vector<ReactivePoint>> &collisionParts)
{
    auto result = ReactiveShape(parts);
    if (!result) return false;
    auto collision = ReactiveCollider(result, collisionParts);
    if (!collision) return false;
    impl_->neighborShapes[key] = collision;
    impl_->neighborMass[key] = result->GetMassProperties();
    return true;
}

bool ReactiveWorld::ImpulseNeighbor(uint64_t key, ReactiveBody &body, const CVECTOR &impulse, const CVECTOR &worldPoint)
{
    const auto shape = impl_->neighborShapes.find(key);
    if (shape == impl_->neighborShapes.end() || !Finite(impulse) || !Finite(worldPoint) || !(body.mass>0.f)) return false;
    auto id = impl_->Hydrate(shape->second, impl_->neighborMass.at(key), body, true);
    if (id.IsInvalid()) return false;
    impl_->physics.GetBodyInterface().AddImpulse(id, Vec(impulse), JPH::RVec3(worldPoint.x, worldPoint.y, worldPoint.z));
    impl_->Read(id, shape->second, body);
    impl_->Remove(id);
    return true;
}

void ReactiveWorld::ClearNeighbors()
{
    impl_->neighborShapes.clear();
    impl_->neighborMass.clear();
}

bool ReactiveWorld::SetStaticTriangles(const std::vector<CVECTOR> &triangles)
{
    if (triangles.empty()) { impl_->Remove(impl_->terrain); return true; }
    JPH::TriangleList mesh;
    for (std::size_t i = 0; i + 2 < triangles.size(); i += 3)
        mesh.emplace_back(JPH::Float3(triangles[i].x, triangles[i].y, triangles[i].z),
                          JPH::Float3(triangles[i+1].x, triangles[i+1].y, triangles[i+1].z),
                          JPH::Float3(triangles[i+2].x, triangles[i+2].y, triangles[i+2].z));
    auto result = JPH::MeshShapeSettings(mesh).Create();
    if (result.HasError()) return false;
    auto &bodies = impl_->physics.GetBodyInterface();
    JPH::BodyCreationSettings settings(result.Get(), JPH::RVec3::sZero(),
        JPH::Quat::sIdentity(), JPH::EMotionType::Static, StaticLayer);
    const auto next = bodies.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
    if (next.IsInvalid()) return false;
    impl_->Remove(impl_->terrain);
    impl_->terrain = next;
    return true;
}

float ReactiveWorld::SubmergedFraction(float localWaterHeight) const
{
    if (!impl_->hull || !std::isfinite(localWaterHeight)) return 0.f;
    if (impl_->submergedHeight == localWaterHeight) return impl_->submergedFraction;
    float total = 0.f, submerged = 0.f;
    JPH::Vec3 center;
    const auto com = impl_->hull->GetCenterOfMass();
    impl_->hull->GetSubmergedVolume(JPH::Mat44::sTranslation(com),
        JPH::Vec3::sReplicate(1.f), JPH::Plane(JPH::Vec3::sAxisY(), -localWaterHeight),
        total, submerged, center
#ifdef JPH_DEBUG_RENDERER
        , JPH::RVec3::sZero()
#endif
    );
    impl_->submergedHeight = localWaterHeight;
    impl_->submergedFraction = total > 0.f ? submerged / total : 0.f;
    return impl_->submergedFraction;
}

CVECTOR ReactiveWorld::CenterOfMass() const
{
    if (!impl_->hull) return CVECTOR(0.f);
    const auto c = impl_->hull->GetCenterOfMass();
    return CVECTOR(c.GetX(), c.GetY(), c.GetZ());
}

CVECTOR ReactiveWorld::AngularTorque(const CVECTOR &localAcceleration, float mass) const
{
    if (!impl_->hull || !(mass > 0.f)) return CVECTOR(0.f);
    auto properties = impl_->hullMass;
    properties.ScaleToMass(mass);
    const auto torque = properties.mInertia.Multiply3x3(Vec(localAcceleration));
    return CVECTOR(torque.GetX(), torque.GetY(), torque.GetZ());
}

bool ReactiveWorld::Step(ReactiveBody &state, const std::vector<ReactiveForce> &forces,
                         const ReactiveFluid &fluid, float seconds,
                         std::vector<ReactiveNeighbor> *neighbors, const ReactiveDrive &drive)
{
    impl_->updateError = JPH::EPhysicsUpdateError::None;
    if (!impl_->hull || !Finite(state.pose) || !Finite(state.velocity) ||
        !Finite(state.angularVelocity) || !std::isfinite(seconds) || seconds <= 0.f ||
        !std::isfinite(state.mass) || state.mass <= 0.f ||
        !Finite(drive.airForce) || !Finite(drive.waterForce) ||
        !std::isfinite(drive.referenceSubmerged) || drive.referenceSubmerged <= 0.f) return false;
    auto &w = *impl_;
    auto &bodies = w.physics.GetBodyInterface();
    // Rehydration also makes TOUCH prediction read-only for native state.
    w.Remove(w.moving);
    w.moving = w.Hydrate(w.collisionHull, w.hullMass, state, false);
    if (w.moving.IsInvalid()) return false;
    std::vector<std::pair<ReactiveNeighbor *, JPH::BodyID>> others;
    struct ReleaseNeighbors
    {
        Impl &world;
        decltype(others) &bodies;
        ~ReleaseNeighbors() { for (auto &entry : bodies) world.Remove(entry.second); }
    } release{w, others};
    if (neighbors) for (auto &neighbor : *neighbors)
    {
        const auto shape = w.neighborShapes.find(neighbor.key);
        if (shape == w.neighborShapes.end()) continue;
        // The ordinary native sea owner constrains these three DOFs. The
        // flying body remains unconstrained and receives the opposing impulse.
        auto id = w.Hydrate(shape->second, w.neighborMass.at(neighbor.key), neighbor.body, true);
        if (id.IsInvalid()) continue;
        others.emplace_back(&neighbor, id);
    }
    // Integrate the complete frame in steps no larger than Jolt's ordinary
    // 60 Hz cadence. LinearCast CCD remains enabled for fast translation.
    const unsigned steps = std::max(1u, unsigned(std::ceil(seconds * 60.f)));
    const float dt = seconds / steps;
    w.wet = false;
    for (unsigned step = 0; step < steps; ++step)
    {
        const auto q = bodies.GetRotation(w.moving);
        JPH::Vec3 force = JPH::Vec3::sZero(), torque = JPH::Vec3::sZero();
        for (const auto &jet : forces)
        {
            const auto f = q * Vec(jet.force);
            force += f;
            torque += (q * (Vec(jet.point) - w.hull->GetCenterOfMass())).Cross(f);
        }
        force += Vec(drive.airForce);
        float waterWeight = 0.f;
        const auto surface = JPH::RVec3(state.pose.v.pos.x, fluid.height, state.pose.v.pos.z);
        const auto normal = Vec(fluid.normal);
        // Transform the entire retained bound, including attitude, each step.
        // Only a wholly dry bound may bypass per-part submerged-volume work.
        const auto bound = w.hull->GetLocalBounds().Transformed(bodies.GetCenterOfMassTransform(w.moving));
        const float clearance = normal.Dot(bound.GetCenter() - surface) -
            normal.Abs().Dot(bound.GetExtent());
        if (clearance <= .001f)
        {
            bool applied = false;
            {
                JPH::BodyLockWrite lock(w.physics.GetBodyLockInterface(), w.moving);
                if (!lock.Succeeded()) return false;
                auto &body = lock.GetBody();
                const auto plane = JPH::Plane::sFromPointAndNormal(surface - body.GetCenterOfMassPosition(), normal);
                float total = 0.f, submerged = 0.f;
                JPH::Vec3 center;
                w.hull->GetSubmergedVolume(JPH::Mat44::sRotation(body.GetRotation()), JPH::Vec3::sOne(),
                    plane, total, submerged, center
#ifdef JPH_DEBUG_RENDERER
                    , body.GetCenterOfMassPosition()
#endif
                );
                waterWeight = total > 0.f && drive.referenceSubmerged > 0.f ?
                    std::clamp(submerged / (total*drive.referenceSubmerged), 0.f, 1.f) : 0.f;
                auto dragReference = Vec(fluid.velocity);
                if (drive.nativeHorizontal)
                {
                    // Selective drag adapter, not a physical fluid velocity:
                    // native sailing owns translational XZ resistance once.
                    // Jolt retains vertical/rotational water response.
                    const auto atBuoyancy = body.GetLinearVelocity() + body.GetAngularVelocity().Cross(center);
                    dragReference.SetX(atBuoyancy.GetX()); dragReference.SetZ(atBuoyancy.GetZ());
                }
                applied = body.ApplyBuoyancyImpulse(total, submerged, center, fluid.buoyancy,
                    .5f, .1f, dragReference, w.physics.GetGravity(), dt);
            }
            if (applied) bodies.ActivateBody(w.moving);
            w.wet |= applied;
        }
        force += waterWeight * Vec(drive.waterForce);
        if (force.LengthSq() > 0.f || torque.LengthSq() > 0.f)
            bodies.AddForceAndTorque(w.moving, force, torque, JPH::EActivation::Activate);
        w.updateError = w.physics.Update(dt, 1, &w.allocator, &w.jobs);
        if (w.updateError != JPH::EPhysicsUpdateError::None) return false;
    }
    auto next = state;
    w.Read(w.moving, w.hull, next);
    if (!Finite(next.pose) || !Finite(next.velocity) || !Finite(next.angularVelocity)) return false;
    std::vector<ReactiveBody> projected;
    projected.reserve(others.size());
    for (auto &[neighbor, id] : others)
    {
        projected.push_back(neighbor->body);
        auto &body = projected.back();
        w.Read(id, w.neighborShapes.at(neighbor->key), body);
        if (!Finite(body.pose) || !Finite(body.velocity) || !Finite(body.angularVelocity)) return false;
    }
    state = next;
    for (size_t i = 0; i < others.size(); ++i) others[i].first->body = projected[i];
    return true;
}

bool ReactiveWorld::Wet() const { return impl_->wet; }
uint32_t ReactiveWorld::UpdateError() const { return uint32_t(impl_->updateError); }
} // namespace roundhouse

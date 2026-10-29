#include "location.h"
#include "roundhouse_ragdoll.hpp"
#include "core.h"
#include "collision_triangles.hpp"
#include <vector>

roundhouse::RagdollWorld *Location::GetRagdollWorld()
{
    if (ragdollWorld) return ragdollWorld.get();
    std::vector<CVECTOR> triangles;
    CVECTOR center(0.0f,0.0f,0.0f);
    {
        storm::physics::TriangleSink sink(triangles);
        Clip(nullptr,0,center,100000.0f,storm::physics::TriangleSink::Add);
    }
    if (triangles.empty())
    {
        core.Trace("Roundhouse: location has no collision triangles");
        return nullptr;
    }
    auto world = std::make_unique<roundhouse::RagdollWorld>();
    if (!world->SetStaticTriangles(triangles)) return nullptr;
    ragdollWorld = std::move(world);
    return ragdollWorld.get();
}

void Location::StepRagdolls(float seconds)
{
    if (ragdollWorld) ragdollWorld->Step(seconds);
}

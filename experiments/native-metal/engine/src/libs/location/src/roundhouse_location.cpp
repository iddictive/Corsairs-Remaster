#include "location.h"
#include "roundhouse_ragdoll.hpp"
#include "core.h"
#include <vector>

namespace
{
// Clip's existing callback has no user-data argument. Location evaluation is
// synchronous on the game thread; retain no geometry outside this call.
thread_local std::vector<CVECTOR> *collectedTriangles = nullptr;
bool CollectTriangles(const CVECTOR *vertices, int32_t count)
{
    if (!collectedTriangles || !vertices || count < 3) return true;
    for (int32_t i = 1; i + 1 < count; ++i)
    {
        if (~((vertices[i]-vertices[0]) ^ (vertices[i+1]-vertices[0])) < 1.0e-10f) continue;
        collectedTriangles->push_back(vertices[0]);
        collectedTriangles->push_back(vertices[i]);
        collectedTriangles->push_back(vertices[i+1]);
    }
    return true;
}
}

roundhouse::RagdollWorld *Location::GetRagdollWorld()
{
    if (ragdollWorld) return ragdollWorld.get();
    std::vector<CVECTOR> triangles;
    collectedTriangles = &triangles;
    CVECTOR center(0.0f,0.0f,0.0f);
    try { Clip(nullptr,0,center,100000.0f,CollectTriangles); }
    catch (...) { collectedTriangles = nullptr; throw; }
    collectedTriangles = nullptr;
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

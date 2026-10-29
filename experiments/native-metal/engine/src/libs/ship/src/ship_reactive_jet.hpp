#pragma once
#include "reactive_body.hpp"
#include "ship_base.h"
#include <unordered_map>

namespace storm::ship::phonk
{
struct JetHullCache
{
    std::shared_ptr<const contact::MaterialSnapshot> material;
    uint64_t revision = 0;
};
struct ReactiveJet
{
    std::shared_ptr<roundhouse::ReactiveWorld> world;
    JetHullCache hull;
    std::unordered_map<entid_t, JetHullCache> neighbors;
    std::vector<roundhouse::ReactiveForce> forces;
    bool active = false, wet = false, preparationReported = false;
    float settledSeconds = 0.f, buoyancy = 1.f, installedThrust = 0.f;
    uint64_t terrainStamp = 0;
    CVECTOR terrainCenter = CVECTOR(0.f);
    float terrainRadius = 0.f;
};
}

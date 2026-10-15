#pragma once

#include "c_vector.h"
#include "matrix.h"
#include "ship_hull_damage_bridge.hpp"
#include "ship_fragment_contact.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace storm::ship::phonk
{
struct LaserPlane
{
    CVECTOR normal = CVECTOR(0.f);
    float distance = 0.f;
    bool charred = true;
    CVECTOR grain = CVECTOR(0.f, 1.f, 0.f);
    float roughness = 0.f, spacing = 1.f, phase = 0.f;
};
struct LaserFragment
{
    std::string node;
    uint64_t token = 0, generation = 0;
    CMatrix world;
    CVECTOR center = CVECTOR(0.f), velocity = CVECTOR(0.f), spin = CVECTOR(0.f);
    std::array<CVECTOR, 14> support{};
    storm::ship::fragment_contact::Proxy contactShape;
    float radiusSquared = 1.f, age = 0.f, wetAge = 0.f;
    double time = 0.;
};
struct LaserEdge
{
    CVECTOR a = CVECTOR(0.f), b = CVECTOR(0.f);
    float age = 0.f;
};
struct LaserBreach
{
    CVECTOR point = CVECTOR(0.f);
    float length = 0.f;
    float age = 0.f;
};
struct LaserBinding
{
    std::string node;
    uint64_t generation = 0;
    unsigned planes = 0;
};
struct LaserRay
{
    CVECTOR muzzle = CVECTOR(0.f), endpoint = CVECTOR(0.f), localHit = CVECTOR(0.f);
    uint64_t target = 0;
};
struct HullWork
{
    CVECTOR point = CVECTOR(0.f), normal = CVECTOR(0.f);
    float energy = 0.f, age = 2.f;
    int32_t other = -1;
};
struct LaserState
{
    double time = 0.; // native Execute clock; transient, never saved
    float reload = 0.f, beamAge = 1.f;
    float burn = 0.f, triggerAge = 0.f, sampleClock = 0.f;
    uint64_t contact = 0;
    CVECTOR firstHit = CVECTOR(0.f), lastHit = CVECTOR(0.f);
    CVECTOR muzzle = CVECTOR(0.f), endpoint = CVECTOR(0.f);
    std::vector<LaserRay> rays; // gun-local origins and each ray's own occlusion result
    float charge = 1.f;
    std::vector<LaserPlane> cuts;
    std::vector<LaserFragment> fragments;
    std::vector<LaserEdge> edges;
    std::vector<LaserBreach> breaches;
    std::vector<LaserBinding> bindings;
    HullWork impact;
    std::vector<CVECTOR> contour;
    std::vector<CVECTOR> originalKeel;
    float originalWaterplaneArea = 0.f;
    unsigned contourRevision = 0;
    float remaining = 1.f;
    float flooded = 0.f, floodCapacity = 0.f, floodClock = 0.f;
    int floodOwner = -1;
    bool wetCut = false;
    bool restored = false;
    bool structuralFailure = false, failureReported = false;
};
constexpr float laserReload = .20f, laserSampleTime = .035f, laserBurnTime = .65f;
constexpr float laserBeamLife = .10f;
constexpr unsigned maxLaserCuts = storm_hull_damage::MaxSplitPlanes;
}

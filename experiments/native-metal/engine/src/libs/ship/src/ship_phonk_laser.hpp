#pragma once

#include "c_vector.h"
#include "matrix.h"
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
};
struct LaserFragment
{
    std::string node;
    uint64_t token = 0;
    CMatrix world;
    CVECTOR center = CVECTOR(0.f), velocity = CVECTOR(0.f), spin = CVECTOR(0.f);
    float age = 0.f;
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
struct LaserState
{
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
    std::vector<CVECTOR> contour;
    unsigned contourRevision = 0;
    float remaining = 1.f;
    float flooded = 0.f, floodCapacity = 0.f, floodClock = 0.f;
    int floodOwner = -1;
    bool wetCut = false;
    bool restored = false;
};
constexpr float laserReload = .20f, laserSampleTime = .035f, laserBurnTime = .65f;
constexpr float laserBeamLife = .10f, laserFragmentLife = 6.f;
constexpr unsigned maxLaserCuts = 32, maxLaserFragments = 64;
}

#pragma once

#include "c_vector.h"
#include "matrix.h"
#include "ship_hull_damage_bridge.hpp"
#include "ship_fragment_contact.hpp"
#include "ship_debris_bridge.hpp"
#include "ship_laser_cloth_bridge.hpp"
#include "ship_laser_geometry.hpp"
#include "ship_mast_support.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <future>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace storm::ship::material::convex { struct Body; struct PhysicalDelta; }

namespace storm::ship::phonk
{
struct LaserPlane
{
    CVECTOR normal = CVECTOR(0.f);
    float distance = 0.f;
    bool charred = true;
    CVECTOR grain = CVECTOR(0.f, 1.f, 0.f);
    float roughness = 0.f, spacing = 1.f, phase = 0.f;
    std::vector<storm_hull_damage::BeamSample> beams;
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
    float surfaceArea = 0.f, mass = 0.f, timber = 0.f;
    double time = 0.;
    std::shared_ptr<const storm::ship::material::convex::Body> physical;
    std::shared_ptr<const storm::ship::material::convex::PhysicalDelta> transfer;
    size_t transferIndex=0;
    CVECTOR contactOrigin=CVECTOR(0.f);
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
    float area = 0.f; // Finite removed skin; zero retains legacy separating-seam growth.
    float wetArea = 0.f; // derived by the actual4Hz ingress/wave sampler; never saved
    uint64_t source=UINT64_MAX;
};
struct LaserMastContact
{
    int receiver = 0;
    uint64_t receiverGeneration = 0, mastGeneration = 0;
    std::array<storm_ship_laser::Vec3,4> transform{}; // receiver-local origin and axes
    std::shared_ptr<const storm::ship::mast_support::Contact> contact;
};
struct LaserBinding
{
    std::string node;
    uint64_t generation = 0;
    unsigned planes = 0;
    uint64_t restoreGeneration = 0;
    unsigned restoreCount = 0;
    std::vector<storm_ship_laser::Plane> restoreSnapshot;
    uint64_t materialGeneration=0,materialRevision=0,savedGeometry=0,savedCollision=0;
    std::string materialRestorePayload;
    std::vector<storm_ship_laser::Plane>materialRestoreTail;
    std::vector<LaserMastContact> mastContacts;
};
struct LaserRay
{
    CVECTOR muzzle = CVECTOR(0.f), endpoint = CVECTOR(0.f), localHit = CVECTOR(0.f);
    uint64_t target = 0;
    bool worldTarget = false;
    storm::ship::debris::Hit fragment;
    storm::ship::laser_cloth::Hit cloth;
};
struct HullWork
{
    CVECTOR point = CVECTOR(0.f), normal = CVECTOR(0.f);
    float energy = 0.f, age = 2.f;
    int32_t other = -1;
};
struct LaserCutRequest
{
    LaserPlane plane;
    int32_t owner = -1;
    uint64_t attacker = 0;
    bool payPartialDamage = true;
    size_t predecessors = 0;
    CVECTOR impulse = CVECTOR(0.f);
    std::optional<HullWork> impact;
    float impactRadius = 0.f;
    double queuedAt=0.;
    bool releaseOnly=false;
    uint64_t rejectedFence=0;
};
struct LaserCutTarget
{
    int node = 0;
    std::string name;
    CMatrix local;
    storm_ship_laser::Input input;
    storm_ship_laser::Projection projection;
    storm_ship_laser::Result result;
    std::vector<LaserMastContact> mastContacts;
    bool mastSupportReceiver = false;
    bool supportOnly = false;
};
struct LaserCutWork
{
    uint64_t epoch = 0, model = 0;
    std::string modelName;
    LaserCutRequest request;
    std::vector<LaserCutTarget> targets;
    std::vector<CVECTOR> originalContour, contour, originalKeel;
    float originalArea = 0.f;
    unsigned physicalNodes = 0;
    bool valid = false;
    int failedNode=-1;
    std::string failure;
};
// Load can reset LaserState while the value-only worker retires. Its future
// stays here: destroying a running std::async future would join the frame.
struct LaserCutQueue
{
    uint64_t epoch = 0;
    std::deque<LaserCutRequest> requests;
    std::optional<LaserCutRequest> active;
    std::future<LaserCutWork> future;
    std::shared_ptr<std::atomic<bool>> cancel;
};
struct LaserStroke
{
    uint64_t target=0;
    bool world=false;
    CVECTOR from=CVECTOR(0.f),to=CVECTOR(0.f),muzzleFrom=CVECTOR(0.f),muzzleTo=CVECTOR(0.f);
    float seconds=0.f;
    uint32_t steps=1,next=0;
};
struct LaserDose
{
    uint64_t target=0;
    bool world=false;
    std::vector<storm_hull_damage::BeamSample> beams;
};
struct LaserState
{
    std::vector<LaserFragment> contactBodies;
    std::shared_ptr<const storm::ship::material::convex::PhysicalDelta> contactTransfer;
    uint64_t contactToken=uint64_t(1)<<63;
    double time = 0.; // native Execute clock; transient, never saved
    float reload = 0.f, beamAge = 1.f;
    float burn = 0.f, triggerAge = 0.f, sampleClock = 0.f;
    uint64_t contact = 0;
    bool contactWorld = false;
    storm::ship::debris::Hit fragment;
    CVECTOR firstHit = CVECTOR(0.f), lastHit = CVECTOR(0.f);
    CVECTOR muzzle = CVECTOR(0.f), endpoint = CVECTOR(0.f);
    std::vector<LaserRay> rays; // gun-local origins and each ray's own occlusion result
    std::vector<storm_hull_damage::BeamSample> pendingBeam;
    uint64_t pendingContact = 0;
    bool pendingWorld = false;
    std::deque<LaserDose> pendingDoses;
    std::optional<LaserStroke> stroke; // One admitted interval; transient, never SAVE history.
    std::optional<LaserCutRequest> deferredRelease;
    float charge = 1.f;
    std::vector<LaserPlane> cuts;
    bool materialReady=false;
    uint64_t materialRevision=0;
    uint64_t cutSequence=0,cutHistoryBase=0,materialPendingRevision=0;
    double materialRepairPercent=0.,materialRepairTarget=0.;
    bool materialResetPending=false,materialRepairPending=false;
    bool materialSavePending=false;
    std::vector<LaserFragment> fragments;
    std::vector<LaserEdge> edges;
    std::vector<LaserBreach> breaches;
    std::map<uint64_t,size_t> breachIndex;
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
constexpr float laserReload = .20f, laserSampleTime = .035f, laserBurnTime = .04f;
constexpr float laserBeamLife = .10f;
constexpr unsigned maxLaserCuts = storm_hull_damage::MaxCutRecords;
}

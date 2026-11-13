#pragma once

#include "c_vector.h"
#include <cstdint>

// AIBalls is the sole owner of projectile identity, hostility and destruction.
// Its adapter registers these callbacks while its entity exists, and clears
// them before destruction. Calls are on the simulation thread. Each operation
// scans at most `budget` projectiles and advances its independent cursor fairly.
// acquire selects an inbound hostile projectile/group within range of origin;
// Position, velocity and acceleration use world units and the same seconds
// as the interceptor update. Predict with position + velocity*t +
// acceleration*(0.5*t*t); acceleration includes the projectile's trajectory
// scaling and is not necessarily purely vertical.
// intercept atomically rechecks live identity and hostility, destroys at most
// `maximum` rounds within radius, and uses a bounded partial-selection policy
// so a salvo is not guaranteed to be wiped out. Unknown, neutral and friendly
// owners must always survive. No pointers or vector indices escape a callback.
namespace storm::ship::phonk::airdefense
{
struct Contact { CVECTOR position{}, velocity{}, acceleration{}; float timeToApproach = 0.f; };
struct Bridge
{
    void *context = nullptr;
    bool (*acquire)(void *, int32_t defender, CVECTOR origin, float range,
                    uint32_t budget, uint32_t *cursor, Contact *out) = nullptr;
    uint32_t (*intercept)(void *, int32_t defender, CVECTOR center, float radius,
                          uint32_t budget, uint32_t *cursor, uint32_t maximum) = nullptr;
};
inline Bridge bridge{};
inline bool available() { return bridge.context && bridge.acquire && bridge.intercept; }
}

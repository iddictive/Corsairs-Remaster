#pragma once

#include "c_vector.h"
#include <cstdint>
#include <string>

namespace storm::ship::debris
{
// Identity and hit coordinates stay on the detached material while it moves.
// The scene owner validates all four identities again before committing damage.
struct Hit
{
    uint64_t model = 0, owner = 0, token = 0, generation = 0;
    std::string node;
    float fraction = 2.f;
    CVECTOR point = CVECTOR(0.f), normal = CVECTOR(0.f);
    explicit operator bool() const { return token && fraction >= 0.f && fraction <= 1.f; }
};

Hit Trace(const CVECTOR &from, const CVECTOR &to);
bool Impact(const Hit &, const CVECTOR &worldVelocity, float massKg, int32_t attacker);
bool Position(const Hit &, CVECTOR &worldPoint);
void Blast(const CVECTOR &center, float radius);
}

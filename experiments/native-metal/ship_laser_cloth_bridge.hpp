#pragma once

#include "c_vector.h"
#include <cstdint>
#include <string>

namespace storm::ship::laser_cloth
{
struct Hit
{
    uint64_t ship = 0, incarnation = 0;
    std::string node;
    int32_t group = -1, cell = -1;
    float fraction = 2.f;
    CVECTOR point = CVECTOR(0.f), normal = CVECTOR(0.f);
    explicit operator bool() const
    { return ship && incarnation && cell >= 0 && fraction >= 0.f && fraction <= 1.f; }
};

Hit Trace(const CVECTOR &from, const CVECTOR &to, uint64_t excludeShip = 0);
bool Apply(const Hit &, float deliveredSeconds, int32_t attackerIndex);
}

#pragma once
#include <array>
#include <cstdint>

namespace storm_world_cut {
// Derived scene coverage only. Positions use the receiving geometry's frame;
// field is the persistent record index, face its canonical cutter-face index.
struct CapTriangle {
    uint32_t field=0,face=0;
    std::array<std::array<float,3>,3>positions{};
};
}

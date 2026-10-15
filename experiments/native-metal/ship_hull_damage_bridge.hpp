#pragma once
#include <cstdint>

namespace storm_hull_damage {
// Native accepted-hit message; the existing MSG_BLOTS_HIT remains a decal.
// lffffffslffll: code, world position xyz, outward world normal xyz,
// exact traced node name (empty means root), object index, world radius,
// confirmed HP loss, ship ExecuteLayer, ship RealizeLayer.
// Capture node/triangle before the script callback; send only after HP admission.
inline constexpr int32_t HullHitMessage=0x53484431;
inline constexpr int32_t ChipBurstMessage=0x53484331;
inline constexpr float MinHitRadius=.08f,MaxHitRadius=.36f,MaxMergedRadius=.36f;
inline constexpr unsigned MaxSplitPlanes=32,MaxFragmentGroups=8;
inline constexpr unsigned MaxFragmentBuffers=MaxSplitPlanes*MaxFragmentGroups;
}

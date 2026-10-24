#pragma once

#include "animation.h"
#include "entity.h"
#include "roundhouse_ragdoll.hpp"
#include <memory>
#include <vector>

// One character owns the companion player, overrides and physical body handle.
// Bind data comes exclusively from the authored rig manifest.
struct RoundhouseState
{
    enum class Phase { Idle, Kick, Falling, GetUp };
    Phase phase = Phase::Idle;
    bool supported = false;
    bool kickSupported = false;
    bool contacted = false;
    bool hasPreviousHeel = false;
    float time = 0.0f;
    float kickDuration = 1.6f;
    float strikeStartTime = 0.0f;
    float strikeEndTime = 0.0f;
    float getupDuration = 2.0f;
    int head = -1;
    int heel = -1;
    CVECTOR heelOffset = {};
    CVECTOR headOffset = {};
    CVECTOR previousHeel = {};
    CVECTOR strikeOrigin = {};
    CVECTOR origin = {};
    CVECTOR recoveryPosition = {};
    int recoveryNode = -1;
    float yaw = 0.0f;
    roundhouse::Handle handle = 0;
    roundhouse::RigDefinition rig;
    std::unique_ptr<Animation> bank;
    std::vector<CMatrix> blendFrom;
    std::vector<CMatrix> pose;
    entid_t aimTarget = invalid_entity;
};

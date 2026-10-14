#pragma once

#include "attributes.h"
#include "ship_hull_geometry.hpp"
#include "ship_hull_damage_bridge.hpp"
#include <string>

namespace storm_hull_damage
{
inline constexpr unsigned SavedMarkLimit = 256;

// One persisted Ship.Blots schema, shared by live BLOTS and map visuals.
// Empty node is the hull root; malformed records remain ignored exactly as
// Blots::LoadBlot did before introducing another reconstruction consumer.
struct SavedHullCut
{
    std::string node;
    storm_ship_hull::Impact cut{};
    float hp = 0.f;
};

inline bool readSavedHullCut(ATTRIBUTES *record, SavedHullCut &output)
{
    if (!record || record->GetAttributeAsDword("kind") != 1) return false;
    for (const char *key : {"node", "object", "rnd", "x", "y", "z", "nx", "ny", "nz", "ru", "rv", "depth", "hp"})
        if (!record->GetAttributeClass(key)) return false;
    SavedHullCut value;
    value.node = record->GetAttributeClass("node")->GetValue();
    value.cut.object = int32_t(record->GetAttributeAsDword("object"));
    value.cut.seed = record->GetAttributeAsDword("rnd");
    value.cut.center = {record->GetAttributeAsFloat("x"), record->GetAttributeAsFloat("y"), record->GetAttributeAsFloat("z")};
    value.cut.normal = {record->GetAttributeAsFloat("nx"), record->GetAttributeAsFloat("ny"), record->GetAttributeAsFloat("nz")};
    value.cut.radiusU = record->GetAttributeAsFloat("ru"); value.cut.radiusV = record->GetAttributeAsFloat("rv");
    value.cut.halfDepth = record->GetAttributeAsFloat("depth"); value.hp = record->GetAttributeAsFloat("hp");
    if (!storm_ship_hull::valid(value.cut) || value.cut.radiusU > MaxMergedRadius ||
        value.cut.radiusV > MaxMergedRadius || !std::isfinite(value.hp) || value.hp <= 0.f) return false;
    output = std::move(value);
    return true;
}
}

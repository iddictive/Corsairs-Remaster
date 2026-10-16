#pragma once

#include "core.h"

// Borrow the script's location for one actor callback. Native active context,
// camera and the canonical Location/character objects are never changed.
class LocationScriptContext final
{
public:
    explicit LocationScriptContext(entid_t location);
    ~LocationScriptContext();
    LocationScriptContext(const LocationScriptContext &) = delete;
    LocationScriptContext &operator=(const LocationScriptContext &) = delete;

private:
    VDATA *loaded = nullptr;
    DATA *previousReference = nullptr;
    DATA *borrowedReference = nullptr;
    ATTRIBUTES *previousAttributes = nullptr;
    entid_t previousLocation = {};
    entid_t activeLocation = {};
    uint64_t activeGeneration = 0;
    uint32_t previousIndex = 0;
    int32_t previousCaptured = 0;
    int32_t borrowedCaptured = 0;
};

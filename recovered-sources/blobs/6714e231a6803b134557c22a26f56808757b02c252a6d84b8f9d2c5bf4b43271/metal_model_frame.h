#pragma once

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
#include "shared/metal_model_frame.hpp"

class MODEL;
class NODE;

// A null node denotes coordinates already authored in the model's frame.
// Returns rigid eligibility; known animated/nonrigid frames retain their identity.
// Unknown models and detached/foreign nodes remain empty and uncached.
bool StormMetalGetModelFrame(MODEL* model, const NODE* node, StormMetalObjectFrame& frame);
#endif

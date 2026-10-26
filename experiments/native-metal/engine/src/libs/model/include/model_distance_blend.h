#pragma once

#include <cstdint>

class MODEL;

namespace storm_model
{
// A composed surface can share one transient fade with its live attachments.
void RealizeWithDistanceBlend(MODEL &model, uint32_t deltaTime, float distanceBlend);
}

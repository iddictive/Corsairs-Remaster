#pragma once
#include <algorithm>
#include <cmath>

namespace progressive_shadow {

inline float sunRadiusTexels(float receiverDepth, float blockerDepth,
                             float depthWorldSpan, float worldTexel) {
  if (!(blockerDepth < receiverDepth) || worldTexel <= 0.f) return .75f;
  const float separation = (receiverDepth - blockerDepth) * depthWorldSpan;
  return std::clamp(.75f + separation * .035f / worldTexel, .75f, 5.f);
}

inline float pointRadiusTexels(float receiverMajor, float blockerMajor) {
  if (!(blockerMajor < receiverMajor) || blockerMajor <= .1f) return .75f;
  const float penumbraRatio = (receiverMajor - blockerMajor) / blockerMajor;
  return std::clamp(.75f + penumbraRatio * 10.24f, .75f, 4.f);
}

} // namespace progressive_shadow
